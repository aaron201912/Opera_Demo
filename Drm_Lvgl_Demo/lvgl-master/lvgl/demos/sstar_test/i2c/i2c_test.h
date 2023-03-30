#ifndef _I2CDEV_H_
#define _I2CDEV_H_
#if defined (__cplusplus)
extern "C" {
#endif

int i2c_init(char *device, char * inputtx, char *inputfile, int uartspeed, int bVerbose);
int i2c_deinit();

#if defined (__cplusplus)
}
#endif

#endif