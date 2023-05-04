#ifndef _UARTDEV_H_
#define _UARTDEV_H_
#if defined (__cplusplus)
extern "C" {
#endif

int uart_init(char *device, int baudrate);
int uart_deinit(void);
int uart_Get_Successnum();
int uart_Get_Errnum();
#if defined (__cplusplus)
}
#endif

#endif