#include <stdio.h>  
#include <linux/types.h>  
#include <stdlib.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/ioctl.h>  
#include <errno.h>  
#include <assert.h>  
#include <string.h>  
#include <linux/i2c.h>  
#include <linux/i2c-dev.h> 
#include "i2c_test.h"

int i2cfd = -1;
unsigned int slave_addr = 0, reg_addr = 0, value = 0;

static int i2c_write(int i2cfd, unsigned char slave_addr, unsigned char reg_addr, unsigned char value)
{
    unsigned char inbuf[1024] = {0};
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];
    int i;

    messages[0].addr  = slave_addr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(inbuf);
    messages[0].buf   = inbuf;

    //The first byte indicates which register we will write
    inbuf[0] = reg_addr;

    /* 
     * The second byte indicates the value to write.  Note that for many 第二个字节表示要写入的值。
     * devices, we can write multiple, sequential registers at once by
     * simply making outbuf bigger.
     */
	inbuf[1] = value;
	for(i = 1; i < 1024; i++)
	{
		inbuf[i] = value;
	}

    /* Transfer the i2c packets to the kernel and verify it worked 将i2c数据包传输到内核，并验证其是否正常工作*/
    packets.msgs  = messages;
    packets.nmsgs = 1;
    if(ioctl(i2cfd, I2C_RDWR, &packets) < 0) 
    {
        perror("Unable to send data");
        return 1;
    }
	printf("len:%d\n",sizeof(inbuf));
	printf("value:%x\n",value);
    return 0;
}

static int i2c_read(int i2cfd, unsigned char slave_addr, unsigned char reg_addr, unsigned char *value) 
{
    unsigned char inbuf, outbuf;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];
    /*
     * In order to read a register, we first do a "dummy write" by writing
     * 0 bytes to the register we want to read from.  This is similar to
     * the packet in set_i2c_register, except it‘s 1 byte rather than 2.
     */
    inbuf = reg_addr;		//发送要读取的寄存器首地址
    messages[0].addr  = slave_addr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(inbuf);
    messages[0].buf   = &inbuf;

    /* The data will get returned in this structure */
    messages[1].addr  = slave_addr;
    messages[1].flags = I2C_M_RD/* | I2C_M_NOSTART*/;
    messages[1].len   = sizeof(outbuf);
    messages[1].buf   = &outbuf;

    /* Send the request to the kernel and get the result back */
    packets.msgs      = messages;
    packets.nmsgs     = 2;
    if(ioctl(i2cfd, I2C_RDWR, &packets) < 0) 
    {
        perror("Unable to send data");
        return 1;
    }
	printf("value:%x\n",outbuf);

    return 0;
}

int i2cdev_deinit()
{
	if (i2cfd > 0)
	{
		close(i2cfd);
		i2cfd = -1;
	}
	return 0;
}


int i2cdev_init(int argc, char *argv[])
{
	if(argc < 5)
    {
        printf("Usage: \n%s i2c r[w] slave_addr reg_addr [value]\n", argv[0]);
        return 0;
    }

    char FileName[50] = {0};
    sprintf(FileName, "/dev/i2c-%s", argv[1]);
    i2cfd = open(FileName, O_RDWR);
    if(!i2cfd)
    {
        printf("can not open file %s\n", FileName);
        return 0;
    }

    sscanf(argv[3], "%x", &slave_addr);  
    sscanf(argv[4], "%x", &reg_addr);  
	printf("slave addr:%x\n",slave_addr);
	printf("reg addr:%x\n",reg_addr);
    if(!strcmp(argv[2], "r"))
	{
		i2c_read(i2cfd, slave_addr, reg_addr, (unsigned char *)&value);
	}
    else if(argc > 4 && !strcmp(argv[2],"w"))
	{
		sscanf(argv[5],"%x",&value);
		//while(1)
		//{
			i2c_write(i2cfd, slave_addr, reg_addr, value);
		//	sleep(1);
		//}
		
	}
}

#if 0
int main(int argc, char **argv)
{
    //int i2cfd;
    //unsigned int slave_addr = 0, reg_addr = 0, value = 0;


    i2cdev_init(argc, argv);

	getchar();

	i2cdev_deinit();
    return 0;
}
#endif

