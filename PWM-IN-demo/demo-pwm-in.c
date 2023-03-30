#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#define pwm_in "/sys/devices/virtual/mstar/pwmin/"

int pwm_in_duty_fd = -1;

/*
echo 2 1 > /sys/devices/virtual/mstar/pwmin/pwm_in_en  
echo 2 1 > /sys/devices/virtual/mstar/pwmin/pwm_in_det  
echo 2 1 > /sys/devices/virtual/mstar/pwmin/pwm_in_rst  
echo 2 1 0 > /sys/devices/virtual/mstar/pwmin/pwm_in_pul_cal  
echo 2 2 > /sys/devices/virtual/mstar/pwmin/pwm_in_edge  
echo 2 7 > /sys/devices/virtual/mstar/pwmin/pwm_in_timer_div  
echo 2 2 > /sys/devices/virtual/mstar/pwmin/pwm_in_pul_div  
echo 2 > /sys/devices/virtual/mstar/pwmin/pwm_in_get
cat /sys/devices/virtual/mstar/pwmin/pwm_in_get
*/

int pwm_in_init(void)
{
	// # enable/disable PWMIN$k的输入捕获功能  
	system("echo 2 0 > /sys/devices/virtual/mstar/pwmin/pwm_in_en");
	// # enable/disable PWMIN$k的detect功能，即PWMIN1可以捕获到PWMIN0的波形数据  
	system("echo 2 1 > /sys/devices/virtual/mstar/pwmin/pwm_in_det");
    printf("[%s %d] set pwm_in_det\n",__func__,__LINE__);
	// # enable/disable PWMIN$k的reset功能  
	system("echo 2 1 > /sys/devices/virtual/mstar/pwmin/pwm_in_rst");
    printf("[%s %d] set pwm_in_rst\n",__func__,__LINE__);
	// # enable/disable PWMIN$k的脉冲个数计数功能  
	system("echo 2 1 0 > /sys/devices/virtual/mstar/pwmin/pwm_in_pul_cal");  
    printf("[%s %d] set pwm_in_pul_cal\n",__func__,__LINE__);
	// # 设置PWM$k输入捕获的边沿，0表示捕获下边沿，1表示捕获上边沿，2表示捕获双边沿  
	system("echo 2 2 > /sys/devices/virtual/mstar/pwmin/pwm_in_edge");
    printf("[%s %d] set pwm_in_edge\n",__func__,__LINE__);
	// # 设置timer时钟的分频  
	system("echo 2 0 > /sys/devices/virtual/mstar/pwmin/pwm_in_timer_div");  
    printf("[%s %d] set pwm_in_timer_div\n",__func__,__LINE__);
	// # 设置第2^n个脉冲之后再捕获波形  
	system("echo 2 0 > /sys/devices/virtual/mstar/pwmin/pwm_in_pul_div");
    printf("[%s %d] set pwm_in_pul_div\n",__func__,__LINE__);
    // # enable/disable PWMIN$k的输入捕获功能  
	system("echo 2 1 > /sys/devices/virtual/mstar/pwmin/pwm_in_en");
    printf("[%s %d] set pwm_in_en\n",__func__,__LINE__);
    sleep(2);

    pwm_in_duty_fd = open("/sys/devices/virtual/mstar/pwmin/pwm_in_get", O_RDWR);
    if(pwm_in_duty_fd == -1)
    {
        printf("[%s %d] open /sys/devices/virtual/mstar/pwmin/pwm_in_get failed\n", __func__,__LINE__);
        return -1;
    }
}

int pwm_in_deinit(void)
{

}

int pwm_in_get_duty(void)
{
    char pwm_date[128] = {0};
    int fd = -1;
	// # 获取PWMIN$k的波形周期占空比信息
    //system("echo 2 > /sys/devices/virtual/mstar/pwmin/pwm_in_get");
    //system("cat /sys/devices/virtual/mstar/pwmin/pwm_in_get");
    lseek(pwm_in_duty_fd, 0, SEEK_SET);
    fd = write(pwm_in_duty_fd, "2", 1);
    if(fd < 0)
    {
        printf("[%s %d] write /sys/devices/virtual/mstar/pwmin/pwm_in_get fail, fd is %d\n",__func__,__LINE__,fd);
        perror("read:");
        return -1;
    }
    fd = -1;

    printf("[%s %d] set pwm_in_get\n",__func__,__LINE__);
    lseek(pwm_in_duty_fd, 0, SEEK_SET);
    fd = read(pwm_in_duty_fd, pwm_date, 128);
    if(fd < 0)
    {
        printf("[%s %d] read /sys/devices/virtual/mstar/pwmin/pwm_in_get fail, fd is %d\n",__func__,__LINE__,fd);
        perror("read:");
        return -1;
    }
    printf("[%s %d] get duty is [%s]\n",__func__,__LINE__,pwm_date);
}

/*
int pwm_in_get_pulse(void)
{
	// # 获取PWMIN$k的脉冲个数信息  
	system("echo 2 > /sys/devices/virtual/mstar/pwmin/pwm_in_get_pulse");
    printf("[%s %d] set pwm_in_get_pulse\n",__func__,__LINE__);
}
*/

int main(void)
{
    pwm_in_init();
    printf("[%s %d] init pwm-int\n",__func__,__LINE__);

    while(1)
    {
        pwm_in_get_duty();
        printf("[%s %d] get pwm-in\n",__func__,__LINE__);
        sleep(1);
    }
}