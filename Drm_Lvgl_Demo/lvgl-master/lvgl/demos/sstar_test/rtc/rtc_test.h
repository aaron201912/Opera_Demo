#ifndef _RTCDEV_H_
#define _RTCDEV_H_
#if defined (__cplusplus)
extern "C" {
#endif

int rtc_settime(unsigned int year, unsigned int month, unsigned int day, unsigned int hour, unsigned int minute, unsigned int second);
int rtc_readtime(char *time);
int SetSystemTime(unsigned int year, unsigned int month, unsigned int day, unsigned int hour, unsigned int minute, unsigned int second);
int getSystemTime(char *time);
int setsystime_to_hw();

#if defined (__cplusplus)
}
#endif
#endif