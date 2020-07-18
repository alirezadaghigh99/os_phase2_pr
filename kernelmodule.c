#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/rcupdate.h>
#include <linux/fdtable.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/dcache.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/pid.h>
#include <linux/pid_namespace.h>
#include <linux/kallsyms.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>
#include <asm/cacheflush.h>
#include <asm/paravirt.h>

#include <linux/uidgid.h>
#define MODULE
#define LINUX
#define __KERNEL__
#define GFP_KERNEL      (__GFP_RECLAIM | __GFP_IO | __GFP_FS)
#define CR0_PROT 0x00010000

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alireza Nima Sina");
MODULE_DESCRIPTION("phase1");


static dev_t task_device = 0;
static struct cdev *task_cdev;
static struct class *task_class;
static int starter = 0;
int numOfUsers = 0;
int numOfFiles = 0;
unsigned long *sysCallTable;
asmlinkage int (*addressCall)(const char*, int, int);
struct User
{
    int id;
    int userPrivacy;
}users[100];
struct File
{
    char path[50];
    int filePrivacy;
}files[100];
struct  {
    char array[1000];
}char_arr;

asmlinkage int device_open(const char* file, int flags , int mode)
{
	uid_t userId = get_current_user()->uid.val;
    int userAccessTemp = 1;
    int fileAccessTemp = 1;
    for (int i = 0; i < 100; i++)
    {
        if (users[i].id == userId)
        {
            userAccessTemp = users[i].userPrivacy;
            break;
        }
        
    }
    for (int i = 0; i < 100; i++)
    {
        if (strcmp(files[i].path , file) == 0 )
        {
            fileAccessTemp = files[i].filePrivacy;
            break;
        }
        
    }
    
    
}

static ssize_t device_read(struct file *file, char *buf, size_t size, loff_t *offset){
    //int count = *offset;
    
    //copy_to_user(buf, user_buff, j);
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
    
    int i=0;
    int j=0;
    int x;
    while (1)
    {
        x=0;
	users[j].id = 0;
        while (char_arr.array[i]!='%')
        {
	    
            int c = char_arr.array[i] - '0';
	    users[j].id *= 10;
	    users[j].id += c;
            i++;
            x++;
        }
        i++;
        users[j].userPrivacy = char_arr.array[i] - '0';
        i+=2;
        j++;
        if (char_arr.array[i]=='?')
        {
            i++;
            break;
        }
    }
    numOfUsers = j;
    j=0;
    while (1)
    {
        x=0;
        while (char_arr.array[i]!='%')
        {
            
            files[j].path[x]=char_arr.array[i];
            i++;
            x++;
        }
        i++;
        files[j].filePrivacy = char_arr.array[i] - '0';
        i+=2;
        j++;
        if (char_arr.array[i]=='\0')
        {
            break;
        }
    }
    numOfFiles = j;
    for (int i = 0; i < j; i++)
    {
        printk(KERN_ALERT "name of file %d \n",users[i].id);
    }
    sysCallTable = kallsyms_lookup_name("sys_call_table");
    addressCall = (void*) sysCallTable[__NR_open];
    unsigned long cr0 = read_cr0();
    write_cr0(cr0 & ~CR0_PROT);
    sysCallTable[__NR_open] = (unsigned long)(&device_open);
    write_cr0(cr0);
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