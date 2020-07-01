#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/rcupdate.h>
#include <linux/pid.h>
#include <linux/fs_struct.h>
#include <linux/fdtable.h>
#include <linux/dcache.h>

#define MODULE
#define LINUX
#define __KERNEL__
#define GFP_KERNEL      (__GFP_RECLAIM | __GFP_IO | __GFP_FS)

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alireza Nima Sina");
MODULE_DESCRIPTION("phase1");


static dev_t task_device = 0;
static struct cdev *task_cdev;
static struct class *task_class;
int num = 0;

struct  {
    char array[7];
}char_arr;

static ssize_t device_read(struct file *file, char *buf, size_t size, loff_t *offset){
    int count = *offset;
    char user_buff[10000];
    pid_t pid = num;
    struct task_struct *tsk;
    tsk = pid_task(find_vpid(pid), PIDTYPE_PID);
    int state = tsk->state;
    unsigned long nvcsw = tsk->nvcsw;
    unsigned long nvcswTemp = nvcsw;
    unsigned long nivcsw = tsk->nivcsw;
    unsigned long nivcswTemp = nivcsw;
    int lenOfNivcsw = 0;
    int l2 = 0;
    
    int lenOfNvcsw = 0;
    int l1 = 0;
    while(nvcswTemp > 0){
        lenOfNvcsw++;
        nvcswTemp /= 10;
    }
    while(nivcswTemp > 0){
        lenOfNivcsw++;
        nivcswTemp/= 10;
    }

    char nvcswRev[lenOfNvcsw];
    char nivcswRev[lenOfNivcsw];
    int k = 0;
    int len = 0;
    int temp = state;
    while(temp > 0) {
        len++;
        temp /= 10;
    }
    int j = 0;
    char revState[len];
    while(state > 0) {
        revState[j] = '0' + state % 10;
        state /= 10;
        j++;
    }
    while (nvcsw > 0){
        nvcswRev[l1] = '0' + nvcsw % 10;
        nvcsw /= 10;
        l1++;
    }
    while (nivcsw > 0){
        nivcswRev[l2] = '0' + nivcsw % 10;
        nivcsw /= 10;
        l2++;
    }
    char states[len];
    for (j = 0 ; j < len ; j++){
        user_buff[j] = revState[len - j - 1];

    }
    user_buff[j] = '\n';
    j++;
    int cu = j;
    for (j = cu ; j < cu + lenOfNvcsw ; j++){
        user_buff[j] = nvcswRev[lenOfNvcsw - j + 1];
    }
    user_buff[j] = '\n';
    j++;
    cu = j;
    for (j = cu ; j < cu + lenOfNivcsw ; j++){
        user_buff[j] = nivcswRev[lenOfNivcsw + lenOfNvcsw + len - j + 1];
    }
    user_buff[j] = '\n';
    j++;
    cu = j; 
    struct files_struct *current_files;
    struct fdtable *files_table;
    unsigned int *fds;
    int i = 0;
    struct path files_path;
    char *cwd;
    char *buffer_2 = (char *) kmalloc(100 * sizeof(char), GFP_KERNEL); 
    current_files = tsk->files;
    files_table = files_fdtable(current_files);
    size_t i1;
    while (files_table->fd[i] != NULL) {
        files_path = files_table->fd[i]->f_path;
        cwd = d_path(&files_path, buffer_2, 100 * sizeof(char));
        i1 = 0;

        while (cwd[i1] != '\0') {
            user_buff[j] = cwd[i1];
            i1++;
            j++;
        }
    	user_buff[j] = '\n';
    	j++;
        i++;
    }
    user_buff[j] = '\0';
    copy_to_user(buf, user_buff, j);
    return 1;
}

static loff_t device_llseek(struct file *file, loff_t position, int whence)
{
    if (whence == 0) {
	if (position > 92)
            position = 92;
    	if (position < 0)
            position = 0;        
    	file->f_pos = position;
    }  
    return position;
}
//new added
static ssize_t device_write(struct file *filp,const char *buf,size_t count,loff_t *offset)
{
    unsigned long ret;
    if (count > sizeof(char_arr.array) - 1)
        return -EINVAL;
    ret = copy_from_user(char_arr.array, buf, count);
    if (ret)
        return -EFAULT;
    char_arr.array[count] = '\0';
    int x;
    int i;
    for (i = count - 1; i >= 0; i--) {
	num *= 10;
	x = char_arr.array[i] - '0';
	num += x;
    }
    return count;
}

const struct file_operations task_file_operation = {
    .owner = THIS_MODULE,
    .read = device_read,
    .llseek = device_llseek,
    .write = device_write,
};

static int task_init(void){
    int rc = alloc_chrdev_region(&task_device, 0, 1, "first_phase");

    if (rc < 0) {
        printk(KERN_ALERT "registeration failed\n");
        return rc;
    }

    task_cdev = cdev_alloc();

    if (task_cdev == NULL) {
        printk(KERN_ALERT "cdev allocation failed\n");
        unregister_chrdev_region(task_device, 1);
	return -1;
    }

    cdev_init(task_cdev, &task_file_operation);
    rc = cdev_add(task_cdev, task_device, 1);

    if (rc < 0) {
        printk(KERN_ALERT "cdev addition failed\n");
        unregister_chrdev_region(task_device, 1);
	return -1;
    }

   task_class = class_create(THIS_MODULE, "first_phase");

    if (!task_class) {
        printk(KERN_ALERT "class creation failed\n");
    	cdev_del(task_cdev);
        unregister_chrdev_region(task_device, 1);
	return -1;
    }

    if (!device_create(task_class, NULL, task_device, NULL, "first_phase")) {
        printk(KERN_ALERT "Failed to create device");
    	class_destroy(task_class);
        cdev_del(task_cdev);
        unregister_chrdev_region(task_device, 1);
	return -1;
    }

    printk(KERN_ALERT "module loaded!\n");
    return rc;
}

static void task_exit(void) {
    device_destroy(task_class, task_device);
    class_destroy(task_class);
    cdev_del(task_cdev);
    unregister_chrdev_region(task_device, 1);
    printk(KERN_ALERT "module unloaded\n");
}

module_init(task_init);
module_exit(task_exit);
