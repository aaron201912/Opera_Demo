#ifndef _SSTAR_ALGO_H_
#define _SSTAR_ALGO_H_

#include "common.h"

#if defined (__cplusplus)
extern "C" {
#endif

/**************************************************
* Init face recognition algo param , start algo and create thread to get input buff
* @param            \b IN: buffer object
* @param            \b IN: dma file descriptor
* @return           \b OUT: 0:   success
**************************************************/
int sstar_algo_init(buffer_object_t * buf_obj);

/**************************************************
* Stop face recognition algo and release resource
* @return           \b OUT: 0:   success
**************************************************/
int sstar_algo_deinit();

#if defined (__cplusplus)
}
#endif
#endif
