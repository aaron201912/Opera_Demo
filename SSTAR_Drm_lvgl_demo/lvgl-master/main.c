/*
 * test.c
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "lv_obj.h"
#include "sstar_port.h"
#include "lv_label.h"
#include "lv_disp.h"
#include "lv_style.h"
#include "lv_demos.h"
#include "widgets/lv_demo_widgets.h"
#include "music/lv_demo_music.h"
#include "benchmark/lv_demo_benchmark.h"
#include "sstar_test/lv_demo_test.h"
//#include "ui.h"

void *tick_thread(void * data)
{
    (void)data;

    while(1) {
        usleep(1000);
        lv_tick_inc(1); /*Tell LittelvGL that 5 milliseconds were elapsed*/
    }

    return 0;
}

unsigned int _GetTime0()
{
    struct timespec ts;
    unsigned int ms;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ms = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    if(ms == 0)
    {
        ms = 1;
    }
    return ms;
}
int main(int argc, char *argv[])
{
    pthread_t pt;


    if (0 != sstar_lv_init()) {
        printf("ERR: sstar_lv_init failed.\n");
        return -1;
    }
	#if 0
    if (0 == strcmp(argv[1], "a")) {
        lv_demo_music();
    } else if (0 == strcmp(argv[1], "b")) {
        lv_demo_widgets();
    } else if (0 == strcmp(argv[1], "c")) {
        lv_demo_benchmark();
    } else if (0 == strcmp(argv[1], "d")) {
        lv_demo_menu();
    }
	#endif
	lv_demo_menu();

    pthread_create(&pt, NULL, tick_thread, NULL);
    while(1) {
		#if 1
        unsigned int curr = _GetTime0();
        lv_task_handler();
        unsigned int time_diff = _GetTime0() - curr;
        if (time_diff < 10) {
            usleep(( 10 - time_diff ) * 1000);
        }
		#endif
		usleep(20 * 1000);
    }
    pthread_join(pt, NULL);

    sstar_lv_deinit();
    return 0;
}
