/*
* XXX.c - Sigmastar
*
* Copyright (c) [2019~2020] SigmaStar Technology.
*
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License version 2 for more details.
*
*/

#ifndef _ST_UVC_H_
#define _ST_UVC_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C"{
#endif

#include "st_uvc_datatype.h"

extern int pthread_setname_np(pthread_t __target_thread,
        const char *__name);

int32_t ST_UVC_Init(char *uvc_name, ST_UVC_Handle_h*);
int32_t ST_UVC_Uninit(ST_UVC_Handle_h);
int32_t ST_UVC_CreateDev(ST_UVC_Handle_h, const ST_UVC_ChnAttr_t*);
int32_t ST_UVC_DestroyDev(ST_UVC_Handle_h);
int32_t ST_UVC_StartDev(ST_UVC_Handle_h);
int32_t ST_UVC_StopDev(ST_UVC_Handle_h);
int ST_UVC_SetTraceLevel(int level);
void save_file(void *buf,uint32_t length,char type);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //_ST_UVC_H_
