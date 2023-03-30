#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>
int wdt_fd = -1;
int wdtEixt = 0;
pthread_t wdtThreadID;

void *WatchDogKeepAlive()
{
	if(wdt_fd < 0)
    {
        printf("[%s %d] watch dog hasn't been inited\n",__func__,__LINE__);
        return 0;
    }
	while (!wdtEixt)
	{
    	if(-EINVAL == ioctl(wdt_fd, WDIOC_KEEPALIVE, 0))
    	{
        	printf("[%s %d] ioctl return -EINVAL\n", __func__, __LINE__);
        	return -1;
    	}
		sleep(1);
	}
	
    return 0;
}

static int initDog = 0;

int WatchDogInit(int timeout)
{
	wdtEixt = 0;
	printf("[%s %d] init watch dog, timeout:%ds\n", __func__,__LINE__, timeout);

	if (initDog == 0)
	{
	    wdt_fd = open("/dev/watchdog", O_WRONLY);

	    if(wdt_fd == -1)
	    {
	        printf("[%s %d] open /dev/watchdog failed\n", __func__,__LINE__);
	        return -1;
	    }
	    if(-EINVAL == ioctl(wdt_fd, WDIOC_SETTIMEOUT, &timeout))
	    {
	        printf("[%s %d] ioctl return -EINVAL\n", __func__, __LINE__);
	    }
		initDog = 1;	
	}
    
	pthread_create(&wdtThreadID, NULL, WatchDogKeepAlive, NULL);
	
}

int WatchDogDeinit()
{
	wdtEixt = 1;
	
	pthread_join(wdtThreadID, NULL);

	#if 0
    int option = WDIOS_DISABLECARD;
    int ret = ioctl(wdt_fd, WDIOC_SETOPTIONS, &option);
    printf("[%s %s] WDIOC_SETOPTIONS %d WDIOS_DISABLECARD=%d\n", __func__,__LINE__, ret, option);

    if(wdt_fd != -1)
    {
        close(wdt_fd);
        wdt_fd = -1;
    }
	#endif
	
    return 0;
}

#if 0
int main()
{
    int loop = 0;
    
    WatchDogInit(5);

    loop = 3;
    while(loop--)
    {
        sleep(3);
        WatchDogKeepAlive(wdt_fd);
    }

    printf("Stop to feed dog, let system reboot...\n");

    return 0;
}
#endif
