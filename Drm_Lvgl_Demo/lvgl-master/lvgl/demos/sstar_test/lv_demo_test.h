/**
 * @file lv_demo_test.h
 *
 */

#ifndef LV_DEMO_TEST_H
#define LV_DEMO_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_demos.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    LV_TEST_NONE = 0,
	LV_TEST_VDEC,
	LV_TEST_PWM,
	LV_TEST_SAR,
	LV_TEST_AUDIO,
	LV_TEST_IPU,
	LV_TEST_SPI,
	LV_TEST_UART,
	LV_TEST_WDT,
	LV_TEST_IR,
}lv_test_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_demo_test(void);
void lv_demo_chart_5(void);
void lv_demo_menu(void);


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_WIDGETS_H*/

