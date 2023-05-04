#ifndef _PWMDEV_H_
#define _PWMDEV_H_
#if defined (__cplusplus)
extern "C" {
#endif

int pwm_init(int pwmId, int period, int duty_cycle, int polarity);
int pwm_deinit(int pwmId);
int SetPwmDutyCycle(int nPwm, int dutyCycle);
int SetPwmAttribute(int nPwm, char *pAttr, int value);
#if defined (__cplusplus)
}
#endif
#endif