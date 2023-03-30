#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdio.h>
#include<linux/rtc.h>
#include<sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <string.h>

int rtcfd,ret=0;
int irqcount=0;
int data;
struct rtc_time rtc_tm;
struct rtc_wkalrm rtc_alm;
struct rtc_test rtc_test;

struct rtc_test{
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
};



void isleapyear(int year)
{
  if((year%4==0&&year%100!=0)||year%400==0)
   	{
   	  printf("is leap year\n");
   	}
   else
   	{
   	   printf("is not leap year\n");
   	}
}


int getSystemTime(char *systime)
{   
    time_t timer;
    struct tm* t_tm;
    time(&timer);
    t_tm = localtime(&timer);
	sprintf(systime, "%4d-%02d-%02d %02d:%02d:%02d\n", t_tm->tm_year+1900,
    t_tm->tm_mon+1, t_tm->tm_mday, t_tm->tm_hour, t_tm->tm_min, t_tm->tm_sec);
	
    printf("today is %4d-%02d-%02d %02d:%02d:%02d\n", t_tm->tm_year+1900,
    t_tm->tm_mon+1, t_tm->tm_mday, t_tm->tm_hour, t_tm->tm_min, t_tm->tm_sec);
    return 0;
}   

int SetSystemTime(unsigned int year, unsigned int month, unsigned int day, unsigned int hour, unsigned int minute, unsigned int second)
{
	struct tm stTm={0};
    struct timeval tv; 
	stTm.tm_year = year - 1900;
	stTm.tm_mon = month - 1;
	stTm.tm_mday = day;
	stTm.tm_hour = hour;
	stTm.tm_min = minute;
	stTm.tm_sec = second;

    time_t timep = mktime(&stTm);
    tv.tv_sec = timep;  
    tv.tv_usec = 0;  
    if(settimeofday (&tv, (struct timezone *) 0) < 0)  
    {  
        printf("Set system datatime error!\n");
        return -1;  
    }
    printf("Set system datatime %d-%d-%d-%02d-%02d-%02d\n",year, month, day, hour, minute,second);
    return 0; 

}

int rtc_readtime(char *time)
{
   rtcfd=open("/dev/rtc0",O_RDONLY);
   if(rtcfd<0)
   	{
   	  printf("open erro\n");
	  goto err;
   	}
   ret=ioctl(rtcfd,RTC_RD_TIME,&rtc_tm);
   if(ret<0)
   	{
   	   printf("rtc_rd_time ioctl erro\n");
	   goto err;
   	}

   sprintf(time, "%d-%d-%d-%02d-%02d-%02d\n",rtc_tm.tm_year+1900,rtc_tm.tm_mon+1,rtc_tm.tm_mday,rtc_tm.tm_hour,rtc_tm.tm_min,rtc_tm.tm_sec);
   printf("readtime:%d-%d-%d-%02d-%02d-%02d\n",rtc_tm.tm_year+1900,rtc_tm.tm_mon+1,rtc_tm.tm_mday,rtc_tm.tm_hour,rtc_tm.tm_min,rtc_tm.tm_sec);
   isleapyear(rtc_tm.tm_year);
   err:
    close(rtcfd);
    return 0;
}

int sethwtime_to_sys()
{
    unsigned int year = 0;
    unsigned int month = 0;
    unsigned int day = 0;
    unsigned int hour = 0;
    unsigned int min = 0;
    unsigned int sec = 0;
    //rtc_readtime();
    year = rtc_tm.tm_year+1900;
    month = rtc_tm.tm_mon;
    day = rtc_tm.tm_mday;
    hour = rtc_tm.tm_hour;
    min = rtc_tm.tm_min;
    sec = rtc_tm.tm_sec;
    SetSystemTime(year, month, day, hour, min, sec);
    return 0;
}

#if 0
int SetSystemTime(struct rtc_time tm)  
{  
    //struct rtc_time tm;  
    struct tm _tm;  
    struct timeval tv;  
    time_t timep;  
    //sscanf(dt, "%d-%d-%d %d:%d:%d", &tm.tm_year,  
    //    &tm.tm_mon, &tm.tm_mday,&tm.tm_hour,  
    //    &tm.tm_min, &tm.tm_sec);  
    _tm.tm_sec = tm.tm_sec;  
    _tm.tm_min = tm.tm_min;  
    _tm.tm_hour = tm.tm_hour;  
    _tm.tm_mday = tm.tm_mday;  
    _tm.tm_mon = tm.tm_mon - 1;  
    _tm.tm_year = tm.tm_year - 1900;  
  
    timep = mktime(&_tm);  
    tv.tv_sec = timep;  
    tv.tv_usec = 0;  
    if(settimeofday (&tv, (struct timezone *) 0) < 0)  
    {  
        printf("Set system datatime error!/n");  
        return -1;
    }
    return 0;
}  
#endif


   	   

int rtc_settime1(struct rtc_test hw_time)
{

   struct timeval tv0,tv1;
   //long currpts0,currpts1;
   double costms =0;
   rtcfd=open("/dev/rtc0",O_RDONLY);
   if(rtcfd<0)
   	{
   	  printf("open erro\n");
	  goto err;
   	}

   ret=ioctl(rtcfd,RTC_RD_TIME,&rtc_tm);
   if(ret<0)
   	{
   	   printf("rtc_rd_time ioctl erro\n");
	   goto err;
   	}
   //printf("请输入设定时间:\n");
   printf("%d-%d-%d %d:%d:%d\n",hw_time.year,hw_time.month,hw_time.day,hw_time.hour,hw_time.minute,hw_time.second);
   rtc_tm.tm_year=hw_time.year-1900;
   rtc_tm.tm_mon=hw_time.month;
   rtc_tm.tm_mday=hw_time.day;
   rtc_tm.tm_hour=hw_time.hour;
   rtc_tm.tm_min=hw_time.minute;
   rtc_tm.tm_sec=hw_time.second;
   gettimeofday(&tv0,0);
   //currpts0 = tv0.tv_sec*1000000 + tv0.tv_usec;
   ret=ioctl(rtcfd,RTC_SET_TIME,&rtc_tm);
   if(ret<0)
   	{
   	   printf("rtc_set_time ioctl erro\n");
	   goto err;
   	}
   gettimeofday(&tv1,0);
   //currpts1 = tv1.tv_sec*1000000 + tv1.tv_usec;
   costms = (double)(1000* (tv1.tv_sec-tv0.tv_sec)+(tv1.tv_usec-tv0.tv_usec)/1000.0);
   printf("cost time = %-0.2f ms\n",costms);
   //printf("test!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! PTS[%d]1 = [%ld] diff[%ld]\n", index, currpts1, currpts1-currpts0);
   printf("settime:%d-%d-%d-%02d-%02d-%02d\n",rtc_tm.tm_year+1900,rtc_tm.tm_mon+1,rtc_tm.tm_mday,rtc_tm.tm_hour,rtc_tm.tm_min,rtc_tm.tm_sec);
   
   err:
    close(rtcfd);
    return costms;
}

int setsystime_to_hw()
{
	int ret = 0;
	struct rtc_test rtc_hw;
    time_t timer;
    struct tm* t_tm;
    time(&timer);
    t_tm = localtime(&timer);
    printf("today is %4d-%02d-%02d %02d:%02d:%02d\n", t_tm->tm_year+1900,
    t_tm->tm_mon+1, t_tm->tm_mday, t_tm->tm_hour, t_tm->tm_min, t_tm->tm_sec);
    rtc_hw.year = t_tm->tm_year+1900;
    rtc_hw.month = t_tm->tm_mon+1;
    rtc_hw.day = t_tm->tm_mday;
    rtc_hw.hour = t_tm->tm_hour;
    rtc_hw.minute = t_tm->tm_min;
    rtc_hw.second = t_tm->tm_sec;
    ret = rtc_settime1(rtc_hw);
    return ret;

}

   
int rtc_settime(unsigned int year, unsigned int month, unsigned int day, unsigned int hour, unsigned int minute, unsigned int second)
{

   struct timeval tv0,tv1;
   //long currpts0,currpts1;
   double costms =0;
   rtcfd=open("/dev/rtc0",O_RDONLY);
   if(rtcfd<0)
   	{
   	  printf("open erro\n");
	  return -1;
   	}

   ret=ioctl(rtcfd,RTC_RD_TIME,&rtc_tm);
   if(ret<0)
   	{
   	   printf("rtc_rd_time ioctl erro\n");
	   return -1;
   	}
   rtc_tm.tm_year=year-1900;
   rtc_tm.tm_mon=month-1;
   rtc_tm.tm_mday=day;
   rtc_tm.tm_hour=hour;
   rtc_tm.tm_min=minute;
   rtc_tm.tm_sec=second;
   gettimeofday(&tv0,0);
   //currpts0 = tv0.tv_sec*1000000 + tv0.tv_usec;
   ret=ioctl(rtcfd,RTC_SET_TIME,&rtc_tm);
   if(ret<0)
   	{
   	   printf("rtc_set_time ioctl erro\n");
	   return -1;
   	}
   gettimeofday(&tv1,0);
   //currpts1 = tv1.tv_sec*1000000 + tv1.tv_usec;
   costms = (double)(1000* (tv1.tv_sec-tv0.tv_sec)+(tv1.tv_usec-tv0.tv_usec)/1000.0);
   printf("cost time = %-0.2f ms\n",costms);
   //printf("test!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! PTS[%d]1 = [%ld] diff[%ld]\n", index, currpts1, currpts1-currpts0);
   printf("settime:%d-%d-%d %02d:%02d:%02d\n",rtc_tm.tm_year+1900,rtc_tm.tm_mon+1,rtc_tm.tm_mday,rtc_tm.tm_hour,rtc_tm.tm_min,rtc_tm.tm_sec);
   close(rtcfd);
   return costms;
}




int set_alarm()
{  
   rtcfd=open("/dev/rtc0",O_RDONLY);
   if(rtcfd<0)
   	{
   	  printf("open erro\n");
	   goto err;
   	}
   ret=ioctl(rtcfd,RTC_RD_TIME,&rtc_tm);
   rtc_alm.enabled = 1;
   memcpy(&rtc_alm.time, &rtc_tm,sizeof(rtc_tm));
   rtc_alm.time.tm_sec+=5;
   if(rtc_alm.time.tm_sec>=60)
   	{
   	   rtc_alm.time.tm_sec%=60;
	   rtc_alm.time.tm_min++;
   	}
   if(rtc_alm.time.tm_min==60)
   	{
   	   rtc_alm.time.tm_min%=60;
	   rtc_alm.time.tm_hour++;
   	}
   if(rtc_alm.time.tm_hour==24)
   	{
   	   rtc_alm.time.tm_hour=0;
   	}
    printf("set alm:%d-%d-%d-%02d-%02d-%02d\n",rtc_alm.time.tm_year+1900, rtc_alm.time.tm_mon, rtc_alm.time.tm_mday,rtc_alm.time.tm_hour, rtc_alm.time.tm_min, rtc_alm.time.tm_sec);
    ret=ioctl(rtcfd, RTC_WKALM_SET, &rtc_alm);
    if(ret<0)
    {
       printf("set alarm fail\n");
	    goto err; 
    }
    err:
     close(rtcfd);
     return 0;

}

int read_alarm()
{
   rtcfd=open("/dev/rtc0",O_RDONLY);
   if(rtcfd<0)
   	{
   	  printf("open erro\n");
	  goto err;
   	}
   ret=ioctl(rtcfd,RTC_WKALM_RD,&rtc_alm);
   if(ret<0)
   	{
   	   printf("rtc_wkalm_rd ioctl erro\n");
	   goto err;
   	}
   printf("read alm:%d-%d-%d-%02d-%02d-%02d\n",rtc_alm.time.tm_year+1900,rtc_alm.time.tm_mon,rtc_alm.time.tm_mday,rtc_alm.time.tm_hour, rtc_alm.time.tm_min, rtc_alm.time.tm_sec);	
   err:
    close(rtcfd);
    return 0;

} 
#if 0
int main(void)
{
   printf("gets option 0:rtc_readtime 1:rtc_settime 2:set_alarm 3:read_alarm 4:set_systemtime 5:get_systemtime 6:sethwtime_to_sys 7:setsystime_to_hw\n");
   int num;
   scanf("%d",&num);
   switch (num)
   	{
   	   case 0:
	   	      rtc_readtime();
			  break;
	   case 1:
	   	      rtc_settime(2022,10,21,17,39,0);
              printf("set default hw time\n");
			  break;
	   case 2:
	   	      set_alarm();
			  break;
	   case 3:
   	          read_alarm();
			  break;
	   case 4:
   	          SetSystemTime(2022,10,21,17,39,0);
              printf("set default system time\n");
			  break;
	   case 5:
   	          getSystemTime();
			  break;
	   case 6:
   	          sethwtime_to_sys();
			  break;
	   case 7:
   	          setsystime_to_hw();
			  break;

	   default:
	   	break;
   	}
   return 0;
}
#endif
