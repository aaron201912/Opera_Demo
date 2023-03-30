#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <linux/input.h>


#define IR_DEV "/dev/input/event0"

static struct input_event data;
int irfd = -1;
int irEixt = 0;
pthread_t irThreadID;

#if 0
int main(int argv,char **argc)
{
    int ret;
    irfd = open(IR_DEV, O_RDWR);
    if(irfd < 0)
    {
        printf("file to get ir dev\n");
        return -1;
    }
    printf("fd is : %d\n",irfd);
    while(1)
    {
        memset(&data, 0, sizeof(data));
        ret = read(irfd, &data, sizeof(data));
        switch(data.type)
        {
            case EV_SYN:
                if(data.value)
                {
                    printf("the 0x%04x key down\n", data.code);
                }
                break;
            case EV_KEY:
                if(data.value)
                {
                    printf("the 0x%04x key down\n", data.code);
                }
                else
                {
                    printf("the 0x%04x key up\n", data.code);
                }
                break;
            case EV_MSC:
                printf("MISC get value 0x%04x\n", data.value);
            default:
                break;
        }
    }
    return 0;
}
#endif
void *irReadDeal()
{
	int ret = 0;
	while(!irEixt)
    {
        memset(&data, 0, sizeof(data));
        ret = read(irfd, &data, sizeof(data));     
       	switch(data.type)
        {
            case EV_SYN:
                if(data.value)
                {
                    printf("the 0x%04x key down\n", data.code);
                }
                break;
            case EV_KEY:
                if(data.value)
                {
                    printf("the 0x%04x key down\n", data.code);
                }
                else
                {
                    printf("the 0x%04x key up\n", data.code);
                }
                break;
            case EV_MSC:
                printf("MISC get value 0x%04x\n", data.value);
            default:
                break;
        }
		usleep(10 *1000);
    }
    return 0;
}

int ir_init()
{
	irEixt = 0;
    irfd = open(IR_DEV, O_RDWR);
    if(irfd < 0)
    {
        printf("file to get ir dev\n");
        return -1;
    }
	pthread_create(&irThreadID, NULL, irReadDeal, NULL);
}

void ir_deinit()
{

	if (irfd < 0)
		printf("ir deinit fail fd %d\n", irfd);
	else
	{
		irEixt  = 1;
	    if(irThreadID)
	    {
	        pthread_join(irThreadID, NULL);
	    }
		close(irfd);
		irfd = -1;
	}
}

