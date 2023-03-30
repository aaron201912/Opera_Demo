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
pthread_t sarThreadID;
int sarExit = 0;

void sar_adc_getvalue(void)
{
    //获取通道0的值
    while (!sarExit)
    {
	    system("echo 0 > /sys/devices/virtual/mstar/pwmout/adc_data");
	    //获取通道1的值
	    system("echo 1 > /sys/devices/virtual/mstar/pwmout/adc_data");
	    //获取通道2的值
	    system("echo 2 > /sys/devices/virtual/mstar/pwmout/adc_data");

		sleep(1);
    }
}

int sar_adc_init(void)
{
	sarExit = 0;
	//启动PWM_ADC
    system("echo 1 > /sys/devices/virtual/mstar/pwmout/adc_en");
    //设置通道数为24且启用所有SAR_ADC系列通道
    system("echo 24 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 > /sys/devices/virtual/mstar/pwmout/adc_regu_seq");
    //自由触发模式且每次触发采集所有通道
    system("echo 14 1 > /sys/devices/virtual/mstar/pwmout/adc_regu_tri");
    //周期性采集
    system("echo 1 > /sys/devices/virtual/mstar/pwmout/adc_regu_con");
    //开启采集
    system("echo 1 > /sys/devices/virtual/mstar/pwmout/adc_regu_star");
    //自由触发模式下通道采样次数为1，采样时间间隔为255
    system("echo 1 255 > /sys/devices/virtual/mstar/pwmout/adc_freerun");

	pthread_create(&sarThreadID, NULL, sar_adc_getvalue, NULL);

    return 0;
}

int sar_adc_deinit(void)
{
	sarExit = 1;
	pthread_join(sarThreadID, NULL);
	//停止采集
    system("echo 0 > /sys/devices/virtual/mstar/pwmout/adc_regu_star");
    //关闭PWM_ADC
    system("echo 0 > /sys/devices/virtual/mstar/pwmout/adc_en");
}



#if 0
int main(void)
{
    int sar_fd = -1;

    sar_fd = sar_adc_init();
    printf("[%s %d] init sar\n",__func__,__LINE__);
    if(sar_fd < 0)
    {
        printf("[%s %d] init sar failed\n",__func__,__LINE__);
        return -1;
    }

    while (1)
    {
        sar_adc_getvalue();
        printf("[%s %d] get sar value\n",__func__,__LINE__);
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
#endif
