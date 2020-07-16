#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
struct User
{
    int id;
    char userPrivacy;
};
struct File
{
    char path[50];
    int filePrivacy;
};

int main(){
    long pointer;
    char buffer[10000];
    int offset;
    int numberOfUsers;
    int numberOfFiles;
    struct User users[numberOfUsers];
    struct File files[numberOfFiles];
    int id , state;
    char path[50];
    

    printf("enter number of users: ");
    scanf("%d",&numberOfUsers);
    printf("enter number of files: ");
    scanf("%d",&numberOfFiles);
    for (int i = 0; i < numberOfUsers; i++)
    {
        printf("enter id and state of %dth user: ",i+1);
        scanf("%d %d",&id , &state);
        users[i].id = id;
        users[i].userPrivacy = state;
    }
    for (int i = 0; i < numberOfFiles; i++)
    {
        printf("enter path and state of %dth file: ",i+1);
        scanf("%s %d",files[i].path , &state);
        files[i].filePrivacy = state;
    }
    
    
    int file_device = open("/dev/first_phase" , O_RDWR);
    char pid[7];
    int i = 0, c;
    int copy = offset;
    while (copy > 0) {
	c = copy % 10;
	pid[i] = '0' + c;
	i++;
	copy /= 10;
    }
    write(file_device, pid, i);
    int count = 0, s;
    int temp = 0;
  //  while(count != 10) {
	// usleep(period * 1000000);
    //     lseek(file_device, i + 10000 * (count + 1), SEEK_SET);
    //     pointer = read(file_device, buffer, 10000);
	// printf("after %d seconds:\n", period * (count + 1));
    //     for (s = 0; s < 10000; s++) {
    //         if (buffer[s] == '\0') {
    //         	printf("\n");
	//     	break;
	//     }
	//     printf("%c", buffer[s]);
    // 	}
	// count++;
    // }
}
