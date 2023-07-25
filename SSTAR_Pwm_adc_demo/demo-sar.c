#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#define SAR_adc_en          "/sys/devices/virtual/mstar/pwmout/adc_en"
#define SAR_adc_regu_seq    "/sys/devices/virtual/mstar/pwmout/adc_regu_seq"
#define SAR_adc_regu_tri    "/sys/devices/virtual/mstar/pwmout/adc_regu_tri"
#define SAR_adc_regu_con    "/sys/devices/virtual/mstar/pwmout/adc_regu_con"
#define SAR_adc_regu_star   "/sys/devices/virtual/mstar/pwmout/adc_regu_star"
#define SAR_adc_freerun     "/sys/devices/virtual/mstar/pwmout/adc_freerun"
#define SAR_adc_data        "/sys/devices/virtual/mstar/pwmout/adc_data"

int adc_data_fd = -1;

int sar_adc_init(void)
{
    system("echo 1 > /sys/devices/virtual/mstar/pwmout/adc_chan_cover");
	//启动PWM_ADC
    system("echo 1 > /sys/devices/virtual/mstar/pwmout/adc_en");
    //设置通道数为23且启用所有SAR_ADC系列通道
    system("echo 23 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 > /sys/devices/virtual/mstar/pwmout/adc_regu_seq");
    //自由触发模式且每次触发采集所有通道
    system("echo 14 1 > /sys/devices/virtual/mstar/pwmout/adc_regu_tri");
    //周期性采集
    system("echo 1 > /sys/devices/virtual/mstar/pwmout/adc_regu_con");
    //开启采集
    system("echo 1 > /sys/devices/virtual/mstar/pwmout/adc_regu_star");
    //自由触发模式下通道采样次数为1，采样时间间隔为255
    system("echo 1 255 > /sys/devices/virtual/mstar/pwmout/adc_freerun");

    adc_data_fd = open("/sys/devices/virtual/mstar/pwmout/adc_data", O_RDWR);
    if(adc_data_fd == -1)
    {
        printf("[%s %d] open /sys/devices/virtual/mstar/pwmout/adc_data failed\n", __func__,__LINE__);
        return -1;
    }

    return 0;
}

int sar_adc_deinit(void)
{
    //停止采集
    system("echo 0 > /sys/devices/virtual/mstar/pwmout/adc_regu_star");
    //关闭PWM_ADC
    system("echo 0 > /sys/devices/virtual/mstar/pwmout/adc_en");
}

int sar_adc_getvalue(int channel)
{
    //获取通道0的值
    // system("echo 0 > /sys/devices/virtual/mstar/pwmout/adc_data");
    //获取通道1的值
    // system("echo 1 > /sys/devices/virtual/mstar/pwmout/adc_data");
    //获取通道2的值
    // system("echo 2 > /sys/devices/virtual/mstar/pwmout/adc_data");
    
    char adc_date[128] = {0};
    int fd = -1;
    
    lseek(adc_data_fd, 0, SEEK_SET);

    fd = write(adc_data_fd, &channel, 1);
    if(fd < 0)
    {
        printf("[%s %d] write /sys/devices/virtual/mstar/pwmout/adc_data fail, fd is %d\n",__func__,__LINE__,fd);
        return -1;
    }
    fd = -1;

    lseek(adc_data_fd, 0, SEEK_SET);

    fd = read(adc_data_fd, adc_date, 128);
    if(fd < 0)
    {
        printf("[%s %d] read /sys/devices/virtual/mstar/pwmout/adc_data fail, fd is %d\n",__func__,__LINE__,fd);
        perror("read:");
        return -1;
    }
    printf("[%s %d] get adc is [%s]\n",__func__,__LINE__,adc_date);
}

void argv_help(void)
{
	printf("Usage:\n");
	printf("\t1. demo [channel Id] [channel Id] [channel Id] [channel Id] [channel Id]\n");
}

int main(int argc, char **argv)
{
    int sar_fd = -1;
    int i=0;

    if(argc < 2)
    {
        printf("[%s %d] chaneel need >=1 \n",__func__,__LINE__);
        argv_help();
        return -1;
    }
    else if(argc >= 25)
    {
        printf("[%s %d] argc need <=23 \n",__func__,__LINE__);
        argv_help();
        return -1;
    }

    sar_fd = sar_adc_init();
    printf("[%s %d] init sar\n",__func__,__LINE__);
    if(sar_fd < 0)
    {
        printf("[%s %d] init sar failed\n",__func__,__LINE__);
        return -1;
    }

    while (1)
    {
        for(i=1; i<argc; i++)
        {
            printf("[%s %d] get sar value, channel=%d\n",__func__,__LINE__,argv[i][0]-48);
            sar_adc_getvalue(argv[i][0]);
        }
        sleep(5);
    }

    sar_fd = sar_adc_deinit();
    printf("[%s %d] deinit sar\n",__func__,__LINE__);
    if(sar_fd < 0)
    {
        printf("[%s %d] deinit sar failed\n",__func__,__LINE__);
        return -1;
    }

    return 0;
}
