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

int WatchDogInit(int timeout)
{
    printf("[%s %d] init watch dog, timeout:%ds\n", __func__,__LINE__, timeout);
    int wdt_fd = -1;
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

    return wdt_fd;
}

int WatchDogDeinit(int wdt_fd)
{
    if(wdt_fd <= 0)

    printf("[%s %d] watch dog hasn't been inited\n",__func__,__LINE__);

    int option = WDIOS_DISABLECARD;
    int ret = ioctl(wdt_fd, WDIOC_SETOPTIONS, &option);
    printf("[%s %s] WDIOC_SETOPTIONS %d WDIOS_DISABLECARD=%d\n", __func__,__LINE__, ret, option);

    if(wdt_fd != -1)
    {
        close(wdt_fd);
        wdt_fd = -1;
    }
    return 0;
}

int WatchDogKeepAlive(int wdt_fd)
{
    if(wdt_fd < 0)
    {
        printf("[%s %d] watch dog hasn't been inited\n",__func__,__LINE__);
        return 0;
    }
    if(-EINVAL == ioctl(wdt_fd, WDIOC_KEEPALIVE, 0))
    {
        printf("[%s %d] ioctl return -EINVAL\n", __func__, __LINE__);
        return -1;
    }
    return 0;
}

int main()
{
    int loop = 0;
    int wdt_fd = -1;
    wdt_fd = WatchDogInit(5);

    loop = 3;
    while(loop--)
    {
        sleep(3);
        WatchDogKeepAlive(wdt_fd);
    }

    printf("Stop to feed dog, let system reboot...\n");

    return 0;
}