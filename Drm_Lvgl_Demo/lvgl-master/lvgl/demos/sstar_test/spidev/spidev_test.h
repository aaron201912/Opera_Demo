#ifndef _SPIDEV_H_
#define _SPIDEV_H_
#if defined (__cplusplus)
extern "C" {
#endif

int spidev_init(char *device, char * inputtx, char *inputfile, int uartspeed, int bVerbose);
int spidev_deinit();

#if defined (__cplusplus)
}
#endif

#endif