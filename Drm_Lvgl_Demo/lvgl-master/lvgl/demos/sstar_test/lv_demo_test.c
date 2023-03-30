#include "lv_demo_test.h"

#define PWM_POLARITY	"polarity"

lv_palette_t palettecolor[] = {
            LV_PALETTE_BLUE, LV_PALETTE_GREEN, LV_PALETTE_BLUE_GREY,  LV_PALETTE_ORANGE,
            LV_PALETTE_RED, LV_PALETTE_PURPLE, LV_PALETTE_TEAL, _LV_PALETTE_LAST };
lv_obj_t * sliderVal;
lv_obj_t * sliderVal_audio;

lv_test_t testOpt;
extern int flag;
static int sensor_flag = 0;
static lv_style_t style_word;
lv_obj_t * audiotestresult;
lv_obj_t * spitext;
lv_obj_t * uarttext;
lv_obj_t * sartext;
lv_obj_t * irtext;
lv_obj_t * pwmtext;
lv_obj_t * wdttext;
lv_obj_t * rtctext;
lv_obj_t * rtctiletext;

int year = 1970;
int month = 1;
int day = 1;
int hour = 0;
int minute = 0;
int second = 0;

lv_obj_t * rtcbtn_sub1;
lv_obj_t * rtcbtn_sub2;

int vpwm = 0;
int spimodeflag = 0;
int uartmodeflag = 0;
int wdtmodeflag = 0;
int sarmodeflag = 0;
int irmodeflag = 0;
int pwmmodeflag = 0;


static const char *device = "/dev/ttyS1"; 
static uint32_t baudrate = 115200; 
static uint32_t pwmperiod = 10000;
static uint32_t pwmduty = 5000;
static uint32_t polarity = 0;
static uint32_t golbalperiod = 0;

static void event_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
		
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void dropdownpwm_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
		int id = 0;
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
		id = lv_dropdown_get_selected(obj);
        printf("pwm Option: %s id %d \n", buf, id);
		if (id == 0){
			pwmperiod = 10000;
			pwmduty = vpwm * pwmperiod / 100;
		}
		else if (id == 1){
			pwmperiod = 125000;
			pwmduty = vpwm * pwmperiod / 100;
		}
		else if (id == 2){
			pwmperiod = 250000;
			pwmduty = vpwm * pwmperiod / 100;
		}
		else if (id == 3){
			pwmperiod = 500000;
			pwmduty = vpwm * pwmperiod / 100;
		}
		else if (id == 4){
			pwmperiod = 1000000;
			pwmduty = vpwm * pwmperiod / 100;
		}		
		else{
			pwmperiod = 10000;
			pwmduty = vpwm * pwmperiod / 100;
		}		
	}
}

static void pwm_polarity_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
		int id = 0;
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
		id = lv_dropdown_get_selected(obj);
        printf("pwm polarity Option: %s id %d \n", buf, id);
		if (id == 0){
			polarity = 0;
		}
		else if (id == 1){
			polarity = 1;
		}
		else{
			polarity = 0;
		}	

		if (pwmmodeflag == 1)
		{
			SetPwmAttribute(0, PWM_POLARITY, polarity);
		}
	}
}

static void dropdown2_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
		int id = 0;
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
		id = lv_dropdown_get_selected(obj);
        printf("boundrate: %s id %d \n", buf, id);
		if (id == 0)
			baudrate = 115200;
		else if (id == 1)
			baudrate = 921600;
		else
			baudrate = 115200;	
    }
}

static void dropdown1_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
		int id = 0;
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
		id = lv_dropdown_get_selected(obj);
        printf("UART Option: %s id %d \n", buf, id);
		if (id == 0)
			device = "/dev/ttyS1";
		else if (id == 1)
			device = "/dev/ttyS2";
		else if (id == 2)
			device = "/dev/ttyS3";
		else if (id == 3)
			device = "/dev/ttyS4";
		else
			device = "/dev/ttyS1";		
	}
}

static void sar_event_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
		
		testOpt = LV_TEST_SAR;
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void vdec_event_handler(lv_event_t *e)
{
	if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
		flag = 1;
		vdecinit();
		testOpt = LV_TEST_VDEC;
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void ipu_event_handler(lv_event_t *e)
{
	if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
		flag = 1;
		lv_demo_init_ipu_sensor();
		testOpt = LV_TEST_IPU;
    }
	else
	{
		printf("########event_handler \n");
	}	
}

int mp3flag = 0;
static void mp3_start_handler(lv_event_t *e)
{
	if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
		printf("########mp3_event_handler \n");
		testOpt = LV_TEST_AUDIO;
		if (mp3flag == 0)
		{
			lv_demo_init_mp3_ffplayer();
			mp3flag = 1;
			lv_label_set_text(audiotestresult, "\n\nplaying......\n");
		}
    }	
}

static void mp3_stop_handler(lv_event_t *e)
{
	if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
		if (mp3flag == 1)
		{
			lv_demo_deinit_mp3_ffplayer();
			mp3flag = 0;
		}
		lv_label_set_text(audiotestresult, "\n\n\n");
	}
}

static void mp3_event_handler(lv_event_t *e)
{
	if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
		printf("########mp3_event_handler \n");
		testOpt = LV_TEST_AUDIO;
		if (mp3flag == 1)	
			lv_label_set_text(audiotestresult, "\n\nplaying......\n");	
		else
			lv_label_set_text(audiotestresult, "\n\n\n");
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void audio_switch_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *obj = lv_event_get_user_data(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
    	printf("state is  %d \n", lv_obj_get_state(obj));
    }
}

static void spi_mode1_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
		
		testOpt = LV_TEST_SPI;
		if (spimodeflag == 0)
		{
			spimodeflag = 1;
			lv_label_set_text(spitext, "\n\nspidev0 72000000 testing......\n");
			spidev_init("/dev/spidev0.0", NULL, NULL, 72000000, 1);
			spidev_deinit();
			spimodeflag = 0;
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void spi_mode2_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
		
		testOpt = LV_TEST_SPI;
		if (spimodeflag == 0)
		{
			spimodeflag = 2;
			lv_label_set_text(spitext, "\n\nspidev0 TX 123456 testing......\n");
			spidev_init("/dev/spidev0.0", "123456", NULL, 500000, 0);	
			spidev_deinit();
			spimodeflag = 0;
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}
static void spi_mode3_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
		
		testOpt = LV_TEST_SPI;
		if (spimodeflag == 0)
		{
			spimodeflag = 3;
			lv_label_set_text(spitext, "\n\nspidev0/spidev1 testing......\n");
			spidev_init("/dev/spidev0.0", NULL, NULL, 72000000, 1);
			spidev_deinit();
			spidev_init("/dev/spidev1.0", NULL, NULL, 72000000, 1);
			spidev_deinit();
			spimodeflag = 0;
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}
static void uart_event_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
		
		testOpt = LV_TEST_UART;
		if (uartmodeflag == 1)
		lv_label_set_text(uarttext, "\nuart testing......\n");	
		else
		lv_label_set_text(uarttext, "\n");
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void str_event_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
		system("echo mem > /sys/power/state");
    }
	else
	{
		printf("########event_handler \n");
	}	
}


static void uart_start_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {		
		testOpt = LV_TEST_UART;
		if (uartmodeflag == 0)
		{		
			lv_label_set_text(uarttext, "\n\nuart testing......\n");
			uart_init(device, baudrate);
			uartmodeflag = 1;
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void uart_stop_handler(lv_event_t *e)
{
	char result[50];
	int ErrNum = 0, SuccessNum = 0;
	if(lv_event_get_code(e) == LV_EVENT_CLICKED) {		
		if (uartmodeflag == 1)
		{				
			uart_deinit();
			ErrNum = uart_Get_Errnum();
			SuccessNum = uart_Get_Successnum();
			sprintf(result,  "\n\nuart test result:\n Errnum: %d; Sucnum: %d;\n", ErrNum, SuccessNum);
			lv_label_set_text(uarttext, result);
			uartmodeflag = 0;
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void pwm_event_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
		
		testOpt = LV_TEST_PWM;
		if (pwmmodeflag == 1)
		lv_label_set_text(pwmtext, "\npwm testing......\n");	
		else
		lv_label_set_text(pwmtext, "\n");
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void pwm_start_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {		
		testOpt = LV_TEST_PWM;
		if (pwmmodeflag == 0)
		{		
			lv_label_set_text(pwmtext, "\npwm testing......\n");
			pwmduty = vpwm * pwmperiod / 100;
			printf("period %d pwmduty %d vpwm %d golbalperiod %d\n", pwmperiod, pwmduty, vpwm, golbalperiod);
			pwm_init(0, pwmperiod, pwmduty, polarity);
			pwmmodeflag = 1;
			golbalperiod = pwmperiod;
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void pwm_stop_handler(lv_event_t *e)
{
	char result[50];
	int ErrNum = 0, SuccessNum = 0;
	if(lv_event_get_code(e) == LV_EVENT_CLICKED) {		
		if (pwmmodeflag == 1)
		{				
			lv_label_set_text(pwmtext, "\n\n");
			pwm_deinit(0);
			//golbalperiod = 0;
			//pwmmodeflag = 0;
			pwmmodeflag = 0;
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}
static void wdt_event_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {		
		if(wdtmodeflag == 1)
		{
			lv_label_set_text(wdttext, "\nwatchdog feeding......\n");
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void wdt_start_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {		
		testOpt = LV_TEST_WDT;
		if(wdtmodeflag == 0)
		{
			WatchDogInit(10);
			wdtmodeflag = 1;
			lv_label_set_text(wdttext, "\n timeout 10s watchdog feeding...\n");
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void wdt_stop_handler(lv_event_t *e)
{
	if(lv_event_get_code(e) == LV_EVENT_CLICKED) {		
		if (wdtmodeflag == 1)
		{							
			WatchDogDeinit();
			wdtmodeflag = 0;
			lv_label_set_text(wdttext, "\nwatchdog stop feeding......\n");
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void rtc_setsystime_handler(lv_event_t *e)
{
	char systime[30];
	char time[30];
	int ret = 0;
	if(lv_event_get_code(e) == LV_EVENT_CLICKED) {	
			if (year > 1970 && month > 0 && month < 13 && day > 0 && day < 32
				&& hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && second >= 0 && second < 60)
			{
				ret = rtc_settime(year,month,day,hour,minute,second);
				sprintf(systime, "settime costime %d \n", ret);
				lv_label_set_text(rtctiletext, "\n");
				lv_label_set_text(rtctext, systime);
			}
			else
			{
				lv_label_set_text(rtctiletext, "\n");
				lv_label_set_text(rtctext, "\nsettime is error\n");
			}						
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void rtc_getsystime_handler(lv_event_t *e)
{
	char systime[30];
	char time[30];
	int ret = 0;
	if(lv_event_get_code(e) == LV_EVENT_CLICKED) {				
		rtc_readtime(time);
		lv_label_set_text(rtctiletext, time);
		lv_label_set_text(rtctext, "\n");	
	}
	else
	{
		printf("########event_handler \n");
	}	
}

static void sar_start_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {		
		testOpt = LV_TEST_SAR;
		if(sarmodeflag == 0)
		{
			lv_label_set_text(sartext, "\n\nsar testing......\n");
			sar_adc_init();
			sarmodeflag = 1;
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void sar_stop_handler(lv_event_t *e)
{
	if(lv_event_get_code(e) == LV_EVENT_CLICKED) {		
		if (sarmodeflag == 1)
		{				
			lv_label_set_text(sartext, "\n\n\n");
			sar_adc_deinit();
			sarmodeflag = 0;
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void ir_start_handler(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {		
		testOpt = LV_TEST_IR;
		if(irmodeflag == 0)
		{
			lv_label_set_text(irtext, "\n\nir testing......\n");
			ir_init();
			irmodeflag = 1;
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void ir_stop_handler(lv_event_t *e)
{
	if(lv_event_get_code(e) == LV_EVENT_CLICKED) {		
		printf("########irmodeflag %d \n", irmodeflag);
		if (irmodeflag == 1)
		{				
			printf("########@@@irmodeflag %d \n", irmodeflag);
			lv_label_set_text(irtext, "\n\n");			
			ir_deinit();			
			irmodeflag = 0;
		}
    }
	else
	{
		printf("########event_handler \n");
	}	
}

static void ta_event_cb_1(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = lv_event_get_user_data(e);
	const char * txt = lv_textarea_get_text(ta);
	
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);	
		lv_obj_add_flag(rtcbtn_sub1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(rtcbtn_sub2, LV_OBJ_FLAG_HIDDEN);
    }

    if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rtcbtn_sub1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rtcbtn_sub2, LV_OBJ_FLAG_HIDDEN);
    }

	if (code == LV_EVENT_VALUE_CHANGED)
	{
		year = atoi(txt);
		//printf("lv_textarea_get_text %s  year time %d \n", txt, year);
	}
}
static void ta_event_cb_2(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = lv_event_get_user_data(e);
	const char * txt = lv_textarea_get_text(ta);
	
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);	
		lv_obj_add_flag(rtcbtn_sub1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(rtcbtn_sub2, LV_OBJ_FLAG_HIDDEN);
    }

    if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rtcbtn_sub1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rtcbtn_sub2, LV_OBJ_FLAG_HIDDEN);
    }

	if (code == LV_EVENT_VALUE_CHANGED)
	{
		month = atoi(txt);
		//printf("lv_textarea_get_text %s month time %d \n", txt, month);
	}
}
static void ta_event_cb_3(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = lv_event_get_user_data(e);
	const char * txt = lv_textarea_get_text(ta);
	
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);	
		lv_obj_add_flag(rtcbtn_sub1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(rtcbtn_sub2, LV_OBJ_FLAG_HIDDEN);
    }

    if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rtcbtn_sub1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rtcbtn_sub2, LV_OBJ_FLAG_HIDDEN);
    }

	if (code == LV_EVENT_VALUE_CHANGED)
	{
		day = atoi(txt);
		//printf("lv_textarea_get_text %s day time %d \n", txt, day);
	}
}

static void ta_event_cb_4(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = lv_event_get_user_data(e);
	const char * txt = lv_textarea_get_text(ta);
	
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);	
		lv_obj_add_flag(rtcbtn_sub1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(rtcbtn_sub2, LV_OBJ_FLAG_HIDDEN);
    }

    if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rtcbtn_sub1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rtcbtn_sub2, LV_OBJ_FLAG_HIDDEN);
    }

	if (code == LV_EVENT_VALUE_CHANGED)
	{
		hour = atoi(txt);
		//printf("lv_textarea_get_text %s hour time %d \n", txt, hour);
	}
}
static void ta_event_cb_5(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = lv_event_get_user_data(e);
	const char * txt = lv_textarea_get_text(ta);
	
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);	
		lv_obj_add_flag(rtcbtn_sub1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(rtcbtn_sub2, LV_OBJ_FLAG_HIDDEN);
    }

    if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rtcbtn_sub1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rtcbtn_sub2, LV_OBJ_FLAG_HIDDEN);
    }

	if (code == LV_EVENT_VALUE_CHANGED)
	{
		minute = atoi(txt);
		//printf("lv_textarea_get_text %s minute time %d \n", txt, minute);
	}
}

static void ta_event_cb_6(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = lv_event_get_user_data(e);
	const char * txt = lv_textarea_get_text(ta);
	
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);	
		lv_obj_add_flag(rtcbtn_sub1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(rtcbtn_sub2, LV_OBJ_FLAG_HIDDEN);
    }

    if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rtcbtn_sub1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(rtcbtn_sub2, LV_OBJ_FLAG_HIDDEN);
    }

	if (code == LV_EVENT_VALUE_CHANGED)
	{
		second = atoi(txt);
		//printf("lv_textarea_get_text %s second time %d \n", txt, second);
	}
}


void lv_example_keyboard(lv_obj_t *parent)
{
	lv_obj_t *text;
	/*Create a keyboard to use it with an of the text areas*/
    lv_obj_t *kb = lv_keyboard_create(parent);
	lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_size(kb,  LV_HOR_RES / 2, LV_VER_RES / 2);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
	
    /*Create a text area. The keyboard will write here*/
    lv_obj_t * ta;
    ta = lv_textarea_create(parent);
    lv_obj_align(ta, LV_ALIGN_TOP_LEFT, 20, 40);
    lv_obj_add_event_cb(ta, ta_event_cb_1, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta, 100, 50);
	lv_textarea_set_max_length(ta, 4);

	text = lv_label_create(parent);
	lv_label_set_text(text, "-");
	lv_obj_align_to(text, ta, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
	
    ta = lv_textarea_create(parent);
    lv_obj_align(ta, LV_ALIGN_OUT_TOP_LEFT, 140, 40);
    lv_obj_add_event_cb(ta, ta_event_cb_2, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta, 70, 50);
	lv_textarea_set_max_length(ta, 2);

	text = lv_label_create(parent);
	lv_label_set_text(text, "-");
	lv_obj_align_to(text, ta, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
	
	ta = lv_textarea_create(parent);
    lv_obj_align(ta, LV_ALIGN_TOP_LEFT, 230, 40);
    lv_obj_add_event_cb(ta, ta_event_cb_3, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta, 70, 50);
	lv_textarea_set_max_length(ta, 2);

	ta = lv_textarea_create(parent);
    lv_obj_align(ta, LV_ALIGN_TOP_LEFT, 320, 40);
    lv_obj_add_event_cb(ta, ta_event_cb_4, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta, 70, 50);
	lv_textarea_set_max_length(ta, 2);

	text = lv_label_create(parent);
	lv_label_set_text(text, ":");
	lv_obj_align_to(text, ta, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
	
	ta = lv_textarea_create(parent);
    lv_obj_align(ta, LV_ALIGN_TOP_LEFT, 410, 40);
    lv_obj_add_event_cb(ta, ta_event_cb_5, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta, 70, 50);
	lv_textarea_set_max_length(ta, 2);

	text = lv_label_create(parent);
	lv_label_set_text(text, ":");
	lv_obj_align_to(text, ta, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
	
	ta = lv_textarea_create(parent);
    lv_obj_align(ta, LV_ALIGN_TOP_LEFT, 500, 40);
    lv_obj_add_event_cb(ta, ta_event_cb_6, LV_EVENT_ALL, kb);
    lv_obj_set_size(ta, 70, 50);
	lv_textarea_set_max_length(ta, 2);
	
    lv_keyboard_set_textarea(kb, ta);
}

static void back_event_handler(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_t * menu = lv_event_get_user_data(e);

	
	if (testOpt == LV_TEST_VDEC)
	{
		flag = 0;
		//sstar_sensor_deinit();
		vdecdeinit();
		printf("exit vdec\n");
	}
	else if (testOpt == LV_TEST_PWM)
	{
		printf("exit pwm\n");
	}
	else if (testOpt == LV_TEST_SAR)
	{
		printf("exit sar\n");
	}
	else if (testOpt == LV_TEST_AUDIO)
	{
		printf("exit mp3 ffplayer\n");

	}
	else if (testOpt == LV_TEST_IPU)
	{
		flag = 0;
		lv_demo_deinit_ipu_sensor();
		//sstar_sensor_deinit();
		printf("exit IPU\n");
	}
	else if (testOpt == LV_TEST_SPI)
	{
		while(spimodeflag != 0)
		{
			printf("waiting spi over");
		}
	}
	else if (testOpt == LV_TEST_UART)
	{
		printf("exit uart\n");
	}
	else
	{
		
	}
	
	testOpt = LV_TEST_NONE;
}

void lv_demo_test(void)
{
	lv_obj_t * test;
	lv_obj_t * butlabel1;
	lv_obj_t * butlabel2;
	test = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 45);
    lv_obj_set_style_text_font(lv_scr_act(), LV_FONT_DEFAULT, 0);
	
	lv_obj_t * btn1 = lv_btn_create(test);
    lv_obj_add_flag(btn1, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn1, lv_palette_main(palettecolor[2]), 0);
    //lv_obj_set_style_bg_color(btn1, lv_color_white(), LV_STATE_CHECKED);
    //lv_obj_set_style_pad_all(btn1, 10, 0);
    lv_obj_set_style_radius(btn1, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_CLICKED, test);
    lv_obj_set_style_shadow_width(btn1, 0, 0);
	lv_obj_set_size(btn1, LV_DPX(150), LV_DPX(50)); /*设置按键大小*/
    lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -100);
	
	butlabel1 = lv_label_create(btn1);
    lv_label_set_text(butlabel1, "BUTTON 1");
	lv_obj_align_to(butlabel1, btn1, LV_ALIGN_CENTER, 0, 0);
	
	#if 1
	lv_obj_t * btn2 = lv_btn_create(test);
	lv_obj_add_flag(btn2, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn2, lv_palette_main(palettecolor[3]), 0);
	//lv_obj_set_style_bg_color(btn2, lv_color_white(), LV_STATE_CHECKED);
	//lv_obj_set_style_pad_all(btn2, 10, 0);
	lv_obj_set_style_radius(btn2, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_CLICKED, test);
	lv_obj_set_style_shadow_width(btn2, 0, 0);
	lv_obj_set_size(btn2, LV_DPX(150), LV_DPX(50)); /*设置按键大小*/
	lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 100);
	
	butlabel2 = lv_label_create(btn2);
	lv_label_set_text(butlabel2, "BUTTON 2");
	lv_obj_align_to(butlabel2, btn2, LV_ALIGN_CENTER, 0, 0);
	#endif
}

static void pwm_slider_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    int32_t v = lv_slider_get_value(obj);   
	char buf[7];
	
	snprintf(buf, 7, "\n%u\n", v);
	printf("PWM value %d \n", v);
	
	lv_label_set_text(sliderVal, buf);
	if (pwmmodeflag == 1)
	{
		SetPwmDutyCycle(6, golbalperiod * v / 100);
	}

	vpwm = v;
}

static void audio_slider_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    int32_t v = lv_slider_get_value(obj);   
	char buf[7];
	
	snprintf(buf, 7, "\n%u\n", v);
	printf("audio value %d \n", v);
	
	lv_label_set_text(sliderVal_audio, buf);
	//if (mp3flag == 1)
	{
		set_volume(v);
	}
}

/**
 * Display 1000 data points with zooming and scrolling.
 * See how the chart changes drawing mode (draw only vertical lines) when
 * the points get too crowded.
 */
#define LV_IMG_SLIDER_RANGE 10

void lv_demo_test_sar(lv_obj_t * menu, lv_obj_t * main_page)
{
	lv_obj_t * label;
	lv_obj_t * sub_page = lv_menu_page_create(menu, NULL);

	sartext = lv_label_create(sub_page);
	lv_obj_add_style(sartext, &style_word, 0);
	lv_label_set_text(sartext, "\n\n\n");
	
	lv_obj_t * btn_sub1 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub1, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub1, lv_palette_main(palettecolor[3]), 0);
	lv_obj_set_style_radius(btn_sub1, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub1, sar_start_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub1, 0, 0);
	lv_obj_set_size(btn_sub1, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub1, LV_ALIGN_CENTER, -150, 0);
	
	label = lv_label_create(btn_sub1);
	lv_label_set_text(label, "START");
	lv_obj_align_to(label, btn_sub1, LV_ALIGN_CENTER, 0, 0);


	lv_obj_t * btn_sub2 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub2, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub2, lv_palette_main(palettecolor[4]), 0);
	lv_obj_set_style_radius(btn_sub2, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub2, sar_stop_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub2, 0, 0);
	lv_obj_set_size(btn_sub2, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub2, LV_ALIGN_CENTER, 150, 0);
	
	label = lv_label_create(btn_sub2);
	lv_label_set_text(label, "STOP");
	lv_obj_align_to(label, btn_sub2, LV_ALIGN_CENTER, 0, 0);

	
	lv_obj_t * btn = lv_btn_create(main_page);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_add_event_cb(btn, sar_event_handler, LV_EVENT_CLICKED, main_page);
	lv_obj_set_size(btn, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -50, 120);
	
	label = lv_label_create(btn);
	lv_label_set_text(label, "SAR");
	lv_obj_align_to(label, btn, LV_ALIGN_CENTER, 0, 0);
	
	lv_menu_set_load_page_event(menu, btn, sub_page);
}

void lv_demo_test_pwm(lv_obj_t * menu, lv_obj_t * main_page)
{
	lv_obj_t * label;
	lv_obj_t * sub_page = lv_menu_page_create(menu, NULL);
	
	pwmtext = lv_label_create(sub_page);
	lv_obj_add_style(pwmtext, &style_word, 0);
	lv_label_set_text(pwmtext, "\n");
	
	label = lv_label_create(sub_page);
	lv_label_set_text(label, "Period:");
	
	/*Create a normal drop down list*/
    lv_obj_t * dd1 = lv_dropdown_create(sub_page);
    lv_dropdown_set_options(dd1, "10000 \n"
                                "125000 \n"
                                "250000 \n"
                                "500000 \n"
                                "1000000 \n");
                              
    lv_obj_align(dd1, LV_ALIGN_CENTER, -150, -20);	
    lv_obj_add_event_cb(dd1, dropdownpwm_event_handler, LV_EVENT_ALL, NULL);
	
	label = lv_label_create(sub_page);
	lv_label_set_text(label, "polarity:");
	
	lv_obj_t * dd2 = lv_dropdown_create(sub_page);
    lv_dropdown_set_options(dd2, "NORMAL   \n"
                                "INVERSED  \n");
	
	lv_obj_align(dd2, LV_ALIGN_CENTER, -150, -70);	
    lv_obj_add_event_cb(dd2, pwm_polarity_event_handler, LV_EVENT_ALL, NULL);
	
	lv_obj_t *slider = lv_slider_create(sub_page);
    lv_slider_set_range(slider, 0, LV_IMG_SLIDER_RANGE * 10);
    lv_obj_add_event_cb(slider, pwm_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_size(slider, 10, 150);
	lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
	lv_obj_align_to(slider, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 50);
	
	sliderVal = lv_label_create(sub_page);
	lv_label_set_text(sliderVal, "\n0\n");
	lv_obj_align_to(sliderVal, slider, LV_ALIGN_BOTTOM_MID, 0, 60);

	
	lv_obj_t * btn_sub1 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub1, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub1, lv_palette_main(palettecolor[3]), 0);
	lv_obj_set_style_radius(btn_sub1, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub1, pwm_start_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub1, 0, 0);
	lv_obj_set_size(btn_sub1, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub1, LV_ALIGN_CENTER, -150, 100);
	
	label = lv_label_create(btn_sub1);
	lv_label_set_text(label, "START");
	lv_obj_align_to(label, btn_sub1, LV_ALIGN_CENTER, 0, 0);


	lv_obj_t * btn_sub2 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub2, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub2, lv_palette_main(palettecolor[4]), 0);
	lv_obj_set_style_radius(btn_sub2, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub2, pwm_stop_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub2, 0, 0);
	lv_obj_set_size(btn_sub2, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub2, LV_ALIGN_CENTER, 150, 100);
	
	label = lv_label_create(btn_sub2);
	lv_label_set_text(label, "STOP");
	lv_obj_align_to(label, btn_sub2, LV_ALIGN_CENTER, 0, 0);

	
	lv_obj_t * btn = lv_btn_create(main_page);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_add_event_cb(btn, pwm_event_handler, LV_EVENT_CLICKED, main_page);
	lv_obj_set_size(btn, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -100, 120);
	
	label = lv_label_create(btn);
	lv_label_set_text(label, "PWM");
	lv_obj_align_to(label, btn, LV_ALIGN_CENTER, 0, 0);
	
	lv_menu_set_load_page_event(menu, btn, sub_page);
}

void lv_demo_test_audio(lv_obj_t * menu, lv_obj_t * main_page)
{
	lv_obj_t * label;
	lv_obj_t * sub_page = lv_menu_page_create(menu, NULL);

	audiotestresult = lv_label_create(sub_page);
	lv_obj_add_style(audiotestresult, &style_word, 0);
	lv_label_set_text(audiotestresult, "\n\n\n");

	set_volume(0);
	lv_obj_t *slider = lv_slider_create(sub_page);
    lv_slider_set_range(slider, 0, LV_IMG_SLIDER_RANGE * 10);
    lv_obj_add_event_cb(slider, audio_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_size(slider, 10, 150);
	lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
	lv_obj_align_to(slider, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 50);
	
	sliderVal_audio = lv_label_create(sub_page);
	lv_label_set_text(sliderVal_audio, "\n0\n");
	lv_obj_align_to(sliderVal_audio, slider, LV_ALIGN_BOTTOM_MID, 0, 60);
	
	lv_obj_t * btn_sub1 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub1, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub1, lv_palette_main(palettecolor[3]), 0);
	lv_obj_set_style_radius(btn_sub1, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub1, mp3_start_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub1, 0, 0);
	lv_obj_set_size(btn_sub1, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub1, LV_ALIGN_CENTER, -150, 0);
	
	label = lv_label_create(btn_sub1);
	lv_label_set_text(label, "PLAY");
	lv_obj_align_to(label, btn_sub1, LV_ALIGN_CENTER, 0, 0);


	lv_obj_t * btn_sub2 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub2, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub2, lv_palette_main(palettecolor[4]), 0);
	lv_obj_set_style_radius(btn_sub2, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub2, mp3_stop_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub2, 0, 0);
	lv_obj_set_size(btn_sub2, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub2, LV_ALIGN_CENTER, 150, 0);
	
	label = lv_label_create(btn_sub2);
	lv_label_set_text(label, "STOP");
	lv_obj_align_to(label, btn_sub2, LV_ALIGN_CENTER, 0, 0);

	
	lv_obj_t * btn = lv_btn_create(main_page);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_add_event_cb(btn, mp3_event_handler, LV_EVENT_CLICKED, main_page);
	lv_obj_set_size(btn, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -325, 120);

	label = lv_label_create(btn);
	lv_label_set_text(label, "AUDIO");
	lv_obj_align_to(label, btn, LV_ALIGN_CENTER, 0, 0);
	
	#if 0
	lv_obj_t * logo = lv_img_create(btn);
    LV_IMG_DECLARE(audio);
    lv_img_set_src(logo, &audio);
	lv_obj_align_to(logo, btn, LV_ALIGN_CENTER, 0, 0);
	#endif
	
	lv_menu_set_load_page_event(menu, btn, sub_page);
}

void lv_demo_test_vdec(lv_obj_t * menu, lv_obj_t * main_page)
{
	lv_obj_t * label;
	lv_obj_t * sub_page = lv_menu_page_create(menu, NULL);
	
	lv_obj_t * btn = lv_btn_create(main_page);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_add_event_cb(btn, vdec_event_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_size(btn, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 100, 120);
	
	label = lv_label_create(btn);
	lv_label_set_text(label, "VDEC");
	lv_obj_align_to(label, btn, LV_ALIGN_CENTER, 0, 0);

	lv_menu_set_load_page_event(menu, btn, sub_page);
}

void lv_demo_test_spi(lv_obj_t * menu, lv_obj_t * main_page)
{
	lv_obj_t * label;
	lv_obj_t * sub_page = lv_menu_page_create(menu, NULL);

	spitext = lv_label_create(sub_page);
	lv_obj_add_style(spitext, &style_word, 0);
	lv_label_set_text(spitext, "\n\n\n");
	
	lv_obj_t * btn_sub1 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub1, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub1, lv_palette_main(palettecolor[3]), 0);
	lv_obj_set_style_radius(btn_sub1, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub1, spi_mode1_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub1, 0, 0);
	lv_obj_set_size(btn_sub1, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub1, LV_ALIGN_CENTER, -400, 0);
	
	label = lv_label_create(btn_sub1);
	lv_label_set_text(label, "START1");
	lv_obj_align_to(label, btn_sub1, LV_ALIGN_CENTER, 0, 0);
	
	lv_obj_t * btn_sub2 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub2, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub2, lv_palette_main(palettecolor[3]), 0);
	lv_obj_set_style_radius(btn_sub2, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub2, spi_mode2_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub2, 0, 0);
	lv_obj_set_size(btn_sub2, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub2, LV_ALIGN_CENTER, 0, 0);
	
	label = lv_label_create(btn_sub2);
	lv_label_set_text(label, "START2");
	lv_obj_align_to(label, btn_sub2, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t * btn_sub3 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub3, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub3, lv_palette_main(palettecolor[3]), 0);
	lv_obj_set_style_radius(btn_sub3, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub3, spi_mode3_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub3, 0, 0);
	lv_obj_set_size(btn_sub3, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub3, LV_ALIGN_CENTER, 400, 0);
	
	label = lv_label_create(btn_sub3);
	lv_label_set_text(label, "START3");
	lv_obj_align_to(label, btn_sub3, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t * btn = lv_btn_create(main_page);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_set_size(btn, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -120);
	
	label = lv_label_create(btn);
	lv_label_set_text(label, "SPI");
	lv_obj_align_to(label, btn, LV_ALIGN_CENTER, 0, 0);
	
	lv_menu_set_load_page_event(menu, btn, sub_page);
}

void lv_demo_test_ir(lv_obj_t * menu, lv_obj_t * main_page)
{
	lv_obj_t * label;
	lv_obj_t * sub_page = lv_menu_page_create(menu, NULL);

	irtext = lv_label_create(sub_page);
	lv_obj_add_style(irtext, &style_word, 0);
	lv_label_set_text(irtext, "\n\n\n");
	
	lv_obj_t * btn_sub1 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub1, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub1, lv_palette_main(palettecolor[3]), 0);
	lv_obj_set_style_radius(btn_sub1, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub1, ir_start_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub1, 0, 0);
	lv_obj_set_size(btn_sub1, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub1, LV_ALIGN_CENTER, 0, 0);
	
	label = lv_label_create(btn_sub1);
	lv_label_set_text(label, "START");
	lv_obj_align_to(label, btn_sub1, LV_ALIGN_CENTER, 0, 0);

	#if 0
	lv_obj_t * btn_sub2 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub2, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub2, lv_palette_main(palettecolor[4]), 0);
	lv_obj_set_style_radius(btn_sub2, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub2, ir_stop_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub2, 0, 0);
	lv_obj_set_size(btn_sub2, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub2, LV_ALIGN_CENTER, 150, 0);
	
	label = lv_label_create(btn_sub2);
	lv_label_set_text(label, "STOP");
	lv_obj_align_to(label, btn_sub2, LV_ALIGN_CENTER, 0, 0);
	#endif 
	
	lv_obj_t * btn = lv_btn_create(main_page);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	//lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, main_page);
	lv_obj_set_size(btn, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -100, -120);

	label = lv_label_create(btn);
	lv_label_set_text(label, "IR");
	lv_obj_align_to(label, btn, LV_ALIGN_CENTER, 0, 0);
	
	lv_menu_set_load_page_event(menu, btn, sub_page);
}

void lv_demo_test_rtc(lv_obj_t * menu, lv_obj_t * main_page)
{
	lv_obj_t * label;
	lv_obj_t * sub_page = lv_menu_page_create(menu, NULL);
	
	rtctiletext = lv_label_create(sub_page);
	lv_obj_add_style(rtctiletext, &style_word, 0);
	lv_label_set_text(rtctiletext, "\n");
	
	rtctext = lv_label_create(sub_page);
	lv_obj_add_style(rtctext, &style_word, 0);
	lv_label_set_text(rtctext, "\n");

	static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(800, 100)];

    lv_obj_t * canvas = lv_canvas_create(sub_page);
    lv_canvas_set_buffer(canvas, cbuf, 612, 400, LV_IMG_CF_TRUE_COLOR);
	lv_canvas_fill_bg(canvas, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_OPA_COVER);
	
	lv_example_keyboard(canvas);

	#if 1
	rtcbtn_sub1 = lv_btn_create(sub_page);
	lv_obj_add_flag(rtcbtn_sub1, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE );
	lv_obj_set_style_bg_color(rtcbtn_sub1, lv_palette_main(palettecolor[3]), 0);
	lv_obj_set_style_radius(rtcbtn_sub1, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(rtcbtn_sub1, rtc_setsystime_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(rtcbtn_sub1, 0, 0);
	lv_obj_set_size(rtcbtn_sub1, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(rtcbtn_sub1, LV_ALIGN_CENTER, -100, 100);
	
	label = lv_label_create(rtcbtn_sub1);
	lv_label_set_text(label, "SETTIME");
	lv_obj_align_to(label, rtcbtn_sub1, LV_ALIGN_CENTER, 0, 0);
	
	
	rtcbtn_sub2 = lv_btn_create(sub_page);
	lv_obj_add_flag(rtcbtn_sub2, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(rtcbtn_sub2, lv_palette_main(palettecolor[3]), 0);
	lv_obj_set_style_radius(rtcbtn_sub2, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(rtcbtn_sub2, rtc_getsystime_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(rtcbtn_sub2, 0, 0);
	lv_obj_set_size(rtcbtn_sub2, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(rtcbtn_sub2, LV_ALIGN_CENTER, 100, 100);
	
	label = lv_label_create(rtcbtn_sub2);
	lv_label_set_text(label, "GETTIME");
	lv_obj_align_to(label, rtcbtn_sub2, LV_ALIGN_CENTER, 0, 0);
	#endif
	
	lv_obj_t * btn = lv_btn_create(main_page);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	//lv_obj_add_event_cb(btn, sar_event_handler, LV_EVENT_CLICKED, main_page);
	lv_obj_set_size(btn, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 325, -120);

	label = lv_label_create(btn);
	lv_label_set_text(label, "RTC");
	lv_obj_align_to(label, btn, LV_ALIGN_CENTER, 0, 0);
	
	lv_menu_set_load_page_event(menu, btn, sub_page);
}

void lv_demo_test_uart(lv_obj_t * menu, lv_obj_t * main_page)
{
	lv_obj_t * label;
	lv_obj_t * sub_page = lv_menu_page_create(menu, NULL);

	uarttext = lv_label_create(sub_page);
	lv_obj_add_style(uarttext, &style_word, 0);
	lv_label_set_text(uarttext, "\n");

	/*Create a normal drop down list*/
    lv_obj_t * dd1 = lv_dropdown_create(sub_page);
    lv_dropdown_set_options(dd1, "UATR1\n"
                                "UATR2\n"
                                "UATR3\n"
                                "UATR4");

    lv_obj_align(dd1, LV_ALIGN_CENTER, -150, -20);	
    lv_obj_add_event_cb(dd1, dropdown1_event_handler, LV_EVENT_ALL, NULL);

	lv_obj_t * dd2 = lv_dropdown_create(sub_page);
    lv_dropdown_set_options(dd2, "115200\n"
                                "921600");

    lv_obj_align(dd2, LV_ALIGN_CENTER, 150, -20);
    lv_obj_add_event_cb(dd2, dropdown2_event_handler, LV_EVENT_ALL, NULL);
	
	lv_obj_t * btn_sub1 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub1, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub1, lv_palette_main(palettecolor[3]), 0);
	lv_obj_set_style_radius(btn_sub1, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub1, uart_start_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub1, 0, 0);
	lv_obj_set_size(btn_sub1, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub1, LV_ALIGN_CENTER, -150, 100);
	
	label = lv_label_create(btn_sub1);
	lv_label_set_text(label, "START");
	lv_obj_align_to(label, btn_sub1, LV_ALIGN_CENTER, 0, 0);


	lv_obj_t * btn_sub2 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub2, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub2, lv_palette_main(palettecolor[4]), 0);
	lv_obj_set_style_radius(btn_sub2, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub2, uart_stop_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub2, 0, 0);
	lv_obj_set_size(btn_sub2, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub2, LV_ALIGN_CENTER, 150, 100);
	
	label = lv_label_create(btn_sub2);
	lv_label_set_text(label, "STOP");
	lv_obj_align_to(label, btn_sub2, LV_ALIGN_CENTER, 0, 0);
	
	lv_obj_t * btn = lv_btn_create(main_page);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_add_event_cb(btn, uart_event_handler, LV_EVENT_CLICKED, main_page);
	lv_obj_set_size(btn, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 100, -120);

	label = lv_label_create(btn);
	lv_label_set_text(label, "UART");
	lv_obj_align_to(label, btn, LV_ALIGN_CENTER, 0, 0);
	
	lv_menu_set_load_page_event(menu, btn, sub_page);
}

void lv_demo_test_str(lv_obj_t * menu, lv_obj_t * main_page)
{
	lv_obj_t * label;
	
	lv_obj_t * btn = lv_btn_create(main_page);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_add_event_cb(btn, str_event_handler, LV_EVENT_CLICKED, main_page);
	lv_obj_set_size(btn, LV_DPX(150), LV_DPX(75)); /*设置按键大小*/
	lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 100, -30);

	label = lv_label_create(btn);
	lv_label_set_text(label, "STR");
	lv_obj_align_to(label, btn, LV_ALIGN_CENTER, 0, 0);
}


void lv_demo_test_watchdog(lv_obj_t * menu, lv_obj_t * main_page)
{
	lv_obj_t * label;
	lv_obj_t * sub_page = lv_menu_page_create(menu, NULL);

	wdttext = lv_label_create(sub_page);
	lv_obj_add_style(wdttext, &style_word, 0);
	lv_label_set_text(wdttext, "\n");
	
	lv_obj_t * btn_sub1 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub1, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub1, lv_palette_main(palettecolor[3]), 0);
	lv_obj_set_style_radius(btn_sub1, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub1, wdt_start_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub1, 0, 0);
	lv_obj_set_size(btn_sub1, LV_DPX(200), LV_DPX(200)); /*设置按键大小*/
	lv_obj_align(btn_sub1, LV_ALIGN_CENTER, -150, 0);
	
	label = lv_label_create(btn_sub1);
	lv_label_set_text(label, "STARTWTD");
	lv_obj_align_to(label, btn_sub1, LV_ALIGN_CENTER, 0, 0);


	lv_obj_t * btn_sub2 = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub2, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub2, lv_palette_main(palettecolor[4]), 0);
	lv_obj_set_style_radius(btn_sub2, LV_RADIUS_CIRCLE, 0);
	lv_obj_add_event_cb(btn_sub2, wdt_stop_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_style_shadow_width(btn_sub2, 0, 0);
	lv_obj_set_size(btn_sub2, LV_DPX(200), LV_DPX(200)); /*设置按键大小*/
	lv_obj_align(btn_sub2, LV_ALIGN_CENTER, 150, 0);
	
	label = lv_label_create(btn_sub2);
	lv_label_set_text(label, "STOPFEED");
	lv_obj_align_to(label, btn_sub2, LV_ALIGN_CENTER, 0, 0);

	
	lv_obj_t * btn = lv_btn_create(main_page);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_add_event_cb(btn, wdt_event_handler, LV_EVENT_CLICKED, main_page);
	lv_obj_set_size(btn, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -325, -120);

	label = lv_label_create(btn);
	lv_label_set_text(label, "WDT");
	lv_obj_align_to(label, btn, LV_ALIGN_CENTER, 0, 0);
	
	lv_menu_set_load_page_event(menu, btn, sub_page);
}

void lv_demo_test_ipu(lv_obj_t * menu, lv_obj_t * main_page)
{
	lv_obj_t * label;
	lv_obj_t * sub_page = lv_menu_page_create(menu, NULL);

	
	lv_obj_t * btn = lv_btn_create(main_page);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);

	lv_obj_add_event_cb(btn, ipu_event_handler, LV_EVENT_CLICKED, sub_page);
	lv_obj_set_size(btn, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 325, 120);
	
	label = lv_label_create(btn);
	lv_label_set_text(label, "FaceIpu");
	lv_obj_align_to(label, btn, LV_ALIGN_CENTER, 0, 0);

	lv_menu_set_load_page_event(menu, btn, sub_page);
}

void lv_demo_test_wifi(lv_obj_t * menu, lv_obj_t * main_page)
{
	lv_obj_t * label;
	lv_obj_t * sub_page = lv_menu_page_create(menu, NULL);
	
	lv_obj_t * btn_sub = lv_btn_create(sub_page);
	lv_obj_add_flag(btn_sub, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_bg_color(btn_sub, lv_palette_main(palettecolor[3]), 0);
	lv_obj_set_style_radius(btn_sub, LV_RADIUS_CIRCLE, 0);
	//lv_obj_add_event_cb(btn_sub, event_handler, LV_EVENT_CLICKED, sub_page1);
	lv_obj_set_style_shadow_width(btn_sub, 0, 0);
	lv_obj_set_size(btn_sub, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn_sub, LV_ALIGN_CENTER, 0, 0);
	
	label = lv_label_create(btn_sub);
	lv_label_set_text(label, "START");
	lv_obj_align_to(label, btn_sub, LV_ALIGN_CENTER, 0, 0);
	
	lv_obj_t * btn = lv_btn_create(main_page);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_set_style_bg_color(btn, lv_palette_main(palettecolor[1]), 0);
	lv_obj_add_event_cb(btn, sar_event_handler, LV_EVENT_CLICKED, main_page);
	lv_obj_set_size(btn, LV_DPX(150), LV_DPX(150)); /*设置按键大小*/
	lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 50, -50);

	label = lv_label_create(btn);
	lv_label_set_text(label, "WIFI");
	lv_obj_align_to(label, btn, LV_ALIGN_CENTER, 0, 0);
	
	lv_menu_set_load_page_event(menu, btn, sub_page);
}

void lv_demo_menu(void)
{
    /*Create a menu object*/
    lv_obj_t * menu = lv_menu_create(lv_scr_act());
    lv_obj_set_size(menu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_center(menu);
	
	lv_obj_set_style_text_font(menu, &lv_font_montserrat_26, 0);
   	//lv_obj_set_style_bg_color(menu, lv_color_black(), 0);

	lv_style_init(&style_word);
    lv_style_set_text_color(&style_word, lv_palette_main(palettecolor[1]));
	
    /*Create a main page*/
    lv_obj_t * main_page = lv_menu_page_create(menu, NULL);
	
	lv_demo_test_pwm(menu, main_page);
	lv_demo_test_vdec(menu, main_page);
	lv_demo_test_ipu(menu, main_page);
	lv_demo_test_audio(menu, main_page);
	//lv_demo_test_spi(menu, main_page);
	lv_demo_test_ir(menu, main_page);
	lv_demo_test_rtc(menu, main_page);
	lv_demo_test_uart(menu, main_page);
	lv_demo_test_watchdog(menu, main_page);
	lv_demo_test_str(menu, main_page);
	//lv_demo_test_wifi(menu, main_page);
	lv_obj_add_event_cb(((lv_menu_t *)menu)->main_header_back_btn, back_event_handler, LV_EVENT_CLICKED, NULL);

	#if 0
	lv_obj_t * label = lv_label_create(main_page);
	lv_label_set_text(label, "\n\nP5 TEST\n");
	lv_obj_add_style(label, &style_word, 0);	
	lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
	#endif
    lv_menu_set_page(menu, main_page);

}

