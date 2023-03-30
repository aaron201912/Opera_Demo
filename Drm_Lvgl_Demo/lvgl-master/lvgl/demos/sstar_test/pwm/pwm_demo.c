#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define PWM_DEV			"/sys/class/pwm/pwmchip0"
#define PWM_PERIOD		"period"
#define PWM_DUTYCYCLE	"duty_cycle"
#define PWM_POLARITY	"polarity"
#define PWM_ENABLE		"enable"
#define PWM_NORMAL		"normal"
#define PWM_INVERSED	"inversed"

int bPwmExit = 0;
pthread_t pwmThreadID;

// export pwm port
int ExportPwm(int nPwm)
{
	int fd = -1;
	char pwmExport[256];
	char pwmDev[256];
	
	memset(pwmDev, 0, sizeof(pwmDev));
	sprintf(pwmDev, "%s/pwm%d", PWM_DEV, nPwm);
	if (!access(pwmDev, R_OK))
	{
		printf("pwm%d already exists\n", nPwm);
		return 0;
	}
	
	memset(pwmExport, 0, sizeof(pwmExport));
	sprintf(pwmExport, "%s/export", PWM_DEV);
	fd = open(pwmExport, O_WRONLY);
	
	if (fd < 0)
	{
		printf("failed to export pwm%d\n", nPwm);
		return -1;
	}
	else
	{
		char gpioPort[10];
		memset(gpioPort, 0, sizeof(gpioPort));
		sprintf(gpioPort, "%d", nPwm);
		write(fd, gpioPort, strlen(gpioPort));
		printf("export gpio port: %d\n", nPwm);
		close(fd);
		return 0;
	}
}

// set pwm period
int SetPwmPeriod(int nPwm, int period)
{
	int fd = -1;
	char pwmDev[256];
	memset(pwmDev, 0, sizeof(pwmDev));
	sprintf(pwmDev, "%s/pwm%d/period", PWM_DEV, nPwm);
	
	fd = open(pwmDev, O_RDWR);
	
	if (fd < 0)
	{
		printf("failed to set pwm%d period\n", nPwm);
		return -1;
	}
	else
	{
		char szPeriod[16] = {0};
		sprintf(szPeriod, "%d", period);
		write(fd, szPeriod, sizeof(szPeriod));
		printf("set pwm%d period: %s\n", nPwm, szPeriod);
		close(fd);
		return 0;
	}
}	

// get pwm period
int GetPwmPeriod(int nPwm, int *pPeriod)
{
	int fd = -1;
	char pwmDev[256];
	memset(pwmDev, 0, sizeof(pwmDev));
	sprintf(pwmDev, "%s/pwm%d/period", PWM_DEV, nPwm);
	
	fd = open(pwmDev, O_RDWR);
	
	if (fd < 0)
	{
		printf("failed to get pwm%d period\n", nPwm);
		return -1;
	}
	else
	{
		char szPeriod[16] = {0};
		read(fd, szPeriod, sizeof(szPeriod));
		*pPeriod = atoi(szPeriod);
		printf("get pwm%d period: %d\n", nPwm, *pPeriod);
		close(fd);
		return 0;
	}
}

// set pwm duty_cycle
int SetPwmDutyCycle(int nPwm, int dutyCycle)
{
	int fd = -1;
	char pwmDev[256];
	memset(pwmDev, 0, sizeof(pwmDev));
	sprintf(pwmDev, "%s/pwm%d/duty_cycle", PWM_DEV, nPwm);

	printf("set duty %s \n", pwmDev);
	fd = open(pwmDev, O_RDWR);
	
	if (fd < 0)
	{
		printf("failed to set pwm%d duty_cycle\n", nPwm);
		return -1;
	}
	else
	{
		char szDutyCycle[16] = {0};
		sprintf(szDutyCycle, "%d", dutyCycle);
		write(fd, szDutyCycle, sizeof(szDutyCycle));
		printf("set pwm%d duty_cycle: %s\n", nPwm, szDutyCycle);
		close(fd);
		return 0;
	}
}

// get pwm duty_cycle
int GetPwmDutyCycle(int nPwm, int *pDutyCycle)
{
	int fd = -1;
	char pwmDev[256];
	memset(pwmDev, 0, sizeof(pwmDev));
	sprintf(pwmDev, "%s/pwm%d/duty_cycle", PWM_DEV, nPwm);
	
	fd = open(pwmDev, O_RDWR);
	
	if (fd < 0)
	{
		printf("failed to get pwm%d duty_cycle\n", nPwm);
		return -1;
	}
	else
	{
		char szDutyCycle[16] = {0};
		read(fd, szDutyCycle, sizeof(szDutyCycle));
		*pDutyCycle = atoi(szDutyCycle);
		printf("get pwm%d duty_cycle: %d\n", nPwm, *pDutyCycle);
		close(fd);
		return 0;
	}
}

// set pwm polarity
int SetPwmPolarity(int nPwm, int polarity)
{
	int fd = -1;
	char pwmDev[256];
	memset(pwmDev, 0, sizeof(pwmDev));
	sprintf(pwmDev, "%s/pwm%d/polarity", PWM_DEV, nPwm);
	
	fd = open(pwmDev, O_RDWR);
	
	if (fd < 0)
	{
		printf("failed to set pwm%d polarity\n", nPwm);
		return -1;
	}
	else
	{
		char szPolarity[16] = {0};
		sprintf(szPolarity, "%d", polarity);
		write(fd, szPolarity, sizeof(szPolarity));
		printf("set pwm%d polarity: %s\n", nPwm, szPolarity);
		close(fd);
		return 0;
	}
}

// get pwm polarity
int GetPwmPolarity(int nPwm, int *pPolarity)
{
	int fd = -1;
	char pwmDev[256];
	memset(pwmDev, 0, sizeof(pwmDev));
	sprintf(pwmDev, "%s/pwm%d/polarity", PWM_DEV, nPwm);
	
	fd = open(pwmDev, O_RDWR);
	
	if (fd < 0)
	{
		printf("failed to get pwm%d polarity\n", nPwm);
		return -1;
	}
	else
	{
		char szPolarity[16] = {0};
		read(fd, szPolarity, sizeof(szPolarity));
		*pPolarity = atoi(szPolarity);
		printf("get pwm%d polarity: %d\n", nPwm, *pPolarity);
		close(fd);
		return 0;
	}
}

// set pwm enable
int SetPwmEnable(int nPwm, int enable)
{
	int fd = -1;
	char pwmDev[256];
	memset(pwmDev, 0, sizeof(pwmDev));
	sprintf(pwmDev, "%s/pwm%d/enable", PWM_DEV, nPwm);
	
	fd = open(pwmDev, O_RDWR);
	
	if (fd < 0)
	{
		printf("failed to set pwm%d enable\n", nPwm);
		return -1;
	}
	else
	{
		char szEnable[16] = {0};
		sprintf(szEnable, "%d", enable);
		write(fd, szEnable, sizeof(szEnable));
		printf("set pwm%d enable: %s\n", nPwm, szEnable);
		close(fd);
		return 0;
	}
}

// get pwm enable
int GetPwmEnable(int nPwm, int *pEnable)
{
	int fd = -1;
	char pwmDev[256];
	memset(pwmDev, 0, sizeof(pwmDev));
	sprintf(pwmDev, "%s/pwm%d/enable", PWM_DEV, nPwm);
	
	fd = open(pwmDev, O_RDWR);
	
	if (fd < 0)
	{
		printf("failed to get pwm%d enable\n", nPwm);
		return -1;
	}
	else
	{
		char szEnable[16] = {0};
		read(fd, szEnable, sizeof(szEnable));
		*pEnable = atoi(szEnable);
		printf("get pwm%d enable: %d\n", nPwm, *pEnable);
		close(fd);
		return 0;
	}
}


// set pwm attr
int SetPwmAttribute(int nPwm, char *pAttr, int value)
{
	int fd = -1;
	char pwmDev[256];
	memset(pwmDev, 0, sizeof(pwmDev));
	sprintf(pwmDev, "%s/pwm%d/%s", PWM_DEV, nPwm, pAttr);
	
	fd = open(pwmDev, O_RDWR);
	
	if (fd < 0)
	{
		printf("failed to set pwm%d %s\n", nPwm, pAttr);
		return -1;
	}
	else
	{
		char szValue[16] = {0};
		
		if (!strcmp(pAttr, PWM_POLARITY))
		{
			if (value == 1)
				strcpy(szValue, PWM_INVERSED);
			else if (!value)
				strcpy(szValue, PWM_NORMAL);
			else
			{
				printf("Invalid polarity parameter\n");
				return -1;
			}
		}
		else
			sprintf(szValue, "%d", value);
		
		write(fd, szValue, sizeof(szValue));
		printf("set pwm%d %s: %s\n", nPwm, pAttr, szValue);
		close(fd);
		return 0;
	}
}

// get pwm attr
int GetPwmAttribute(int nPwm, char *pAttr, int *pValue)
{
	int fd = -1;
	char pwmDev[256];
	memset(pwmDev, 0, sizeof(pwmDev));
	sprintf(pwmDev, "%s/pwm%d/%s", PWM_DEV, nPwm, pAttr);
	
	fd = open(pwmDev, O_RDWR);
	
	if (fd < 0)
	{
		printf("failed to get pwm%d %s\n", nPwm, pAttr);
		return -1;
	}
	else
	{
		char szValue[16] = {0};
		read(fd, szValue, sizeof(szValue));
		
		if (!strcmp(pAttr, PWM_POLARITY))
		{
			if (!strncmp(szValue, PWM_INVERSED, strlen(PWM_INVERSED)))
			{
				*pValue = 1;
			}
			else if (!strncmp(szValue, PWM_NORMAL, strlen(PWM_NORMAL)))
			{
				*pValue = 0;
			}
			else
			{
				printf("Invalid polarity\n");
				return -1;
			}
		}
		else
		{
			*pValue = atoi(szValue);
			//printf("get pwm%d %s: %d\n", nPwm, pAttr, *pValue);
		}
		
		printf("get pwm%d %s: %s\n", nPwm, pAttr, szValue);
		
		close(fd);
		return 0;
	}
}
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
	if (pwm_in_duty_fd != -1)
	close(pwm_in_duty_fd);

	pwm_in_duty_fd = -1;
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

#if 0
void display_help(int argc, char **argv)
{
	printf("Usage:\n");
	printf("\t1. %s -r [pwmId]\n",argv[0]);
	printf("\t2. %s -w [pwmId] -p [period] -d [duty_cycle] -o [polarity] -e [enable] \n",argv[0]); 
	printf("\t\tpolarity: \n");
	printf("\t\t\t0 - normal\n");
	printf("\t\t\t1 - inversed\n");
	printf("\t\tenable:\n");
	printf("\t\t\t0 - disable\n");
	printf("\t\t\t1 - enable\n");
}

int main(int argc, char **argv)
{
	char ch = 0;
	char value[2];
	int opt = 0;
	int pwmId = 0;
	int isRead = 0;
	int period = 0;
	int duty_cycle = 0;
	int polarity = 0;
	int enable = 0;
	
	if (argc < 3)
	{
		display_help(argc, argv);
		return -1;
	}
	
	while ((opt = getopt(argc, argv, "r:w:p:d:o:e:h::")) != -1)
	{
		switch(opt)
		{
			case 'r':
				{
					if (argc == 3)
					{
						isRead = 1;
						pwmId = atoi(optarg);
					}
					else
					{
						printf("input cmd error\n");
						display_help(argc, argv);
						return -1;
					}
				}
				break;
			case 'w':
				if (argc > 3)
				{
					isRead = 0;
					pwmId = atoi(optarg);
				}
				else
				{
					printf("input cmd error\n");
					display_help(argc, argv);
				}
				break;
			case 'p':
				period = atoi(optarg);
				break;
			case 'd':
				duty_cycle = atoi(optarg);
				break;
			case 'o':
				polarity = atoi(optarg);
				break;
			case 'e':
				enable = atoi(optarg);
				break;
			case '?':
				{
					switch (optopt)
					{
						case 'r':
						case 'w':
							printf("input parameter is missing pwm devId\n");
							break;
						case 'p':
							printf("input parameter is missing period\n");
							break;
						case 'd':
							printf("input parameter is missing duty cycle\n");
							break;
						case 'o':
							printf("input parameter is missing polarity\n");
							break;
						case 'e':
							printf("input parameter is missing enable\n");
							break;
						default:
							printf("invalid opt\n");
							break;
					}
				}
				break;
			default:
				display_help(argc, argv);
				return -1;
		}
	}
	
	if (ExportPwm(pwmId))
		return -1;
	
	if (isRead)
	{
		if (GetPwmAttribute(pwmId, PWM_PERIOD, &period))
			return -1;
			
		if (GetPwmAttribute(pwmId, PWM_DUTYCYCLE, &duty_cycle))
			return -1;
		
		if (GetPwmAttribute(pwmId, PWM_POLARITY, &polarity))
			return -1;
		
		if (GetPwmAttribute(pwmId, PWM_ENABLE, &enable))
			return -1;
	}
	else
	{
		if (SetPwmAttribute(pwmId, PWM_PERIOD, period))
			return -1;
			
		if (SetPwmAttribute(pwmId, PWM_DUTYCYCLE, duty_cycle))
			return -1;
		
		if (SetPwmAttribute(pwmId, PWM_POLARITY, polarity))
			return -1;
		
		if (SetPwmAttribute(pwmId, PWM_ENABLE, enable))
			return -1;		
	}
	
	return 0;
}
#endif
void *pwmGetDuty()
{
	while(!bPwmExit)
	{
		pwm_in_get_duty();
        printf("[%s %d] get pwm-in\n",__func__,__LINE__);
        sleep(2);
	}
}

int pwm_out_init(int pwmId, int period, int duty_cycle, int polarity)
{
	system("/customer/riu_w 0x103c 0x65 0x1111");
	
	if (ExportPwm(pwmId))
		return -1;
	
	if (SetPwmAttribute(pwmId, PWM_DUTYCYCLE, 0))
		return -1;
		
	if (SetPwmAttribute(pwmId, PWM_PERIOD, 0))
		return -1;
	
	if (SetPwmAttribute(pwmId, PWM_PERIOD, period))
		return -1;
		
	if (SetPwmAttribute(pwmId, PWM_DUTYCYCLE, duty_cycle))
		return -1;
	
	if (SetPwmAttribute(pwmId, PWM_POLARITY, polarity))
		return -1;
	
	if (SetPwmAttribute(pwmId, PWM_ENABLE, 1))
		return -1;	
	return 0;
}

int pwm_init(int pwmId, int period, int duty_cycle, int polarity)
{
	bPwmExit = 0;
	int ret = -1;
	if (pwm_out_init(pwmId, period, duty_cycle, polarity))
		return -1;
	
	pwm_in_init();
	
	pthread_create(&pwmThreadID, NULL, pwmGetDuty, NULL);
	return 0;
}

int pwm_deinit(int pwmId)
{
	bPwmExit = 1;
	
	SetPwmEnable(pwmId, 0);
	if(pwmThreadID)
    {
        pthread_join(pwmThreadID, NULL);
    }
	
	pwm_in_deinit();
	
	system("/customer/riu_w 0x103c 0x65 0x0 ");
	return 0;
}

