#ifndef _WATCHDOGDEV_H_
#define _WATCHDOGDEV_H_
#if defined (__cplusplus)
extern "C" {
#endif

int WatchDogInit(int timeout);
int WatchDogDeinit();
#if defined (__cplusplus)
}
#endif

#endif