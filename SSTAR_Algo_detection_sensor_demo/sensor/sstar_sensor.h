/* SigmaStar trade secret */
/* Copyright (c) [2019~2020] SigmaStar Technology.
All rights reserved.

Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
SigmaStar and be kept in strict confidence
(SigmaStar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of SigmaStar Confidential
Information is unlawful and strictly prohibited. SigmaStar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#ifndef _SSTAR_SENSOR_H_
#define _SSTAR_SENSOR_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "mi_common_datatype.h"
#include "mi_isp.h"
#include "mi_scl.h"
#include "mi_sensor.h"
#include "mi_sensor_datatype.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "mi_vif.h"
#include "common.h"

#define MAX_SENSOR_NUM (2)
#define MI_SYS_DEVICE_ID (0)

typedef struct {
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
    MI_SYS_BindType_e eBindType;
    MI_U32 u32BindParam;
} Sys_BindInfo_T;

/**************************************************
* Create sensor -> vif -> scl -> venc pipeline, venc select by venc_flag
* @param            \b IN: buffer object
* @return           \b OUT: 0:   success
                           -1:   fail
**************************************************/
int create_snr_pipeline(buffer_object_t *buf_obj);

/**************************************************
* destroy sensor -> vif -> scl -> venc pipeline, venc select by venc_flag
* @param            \b IN: buffer object
* @return           \b OUT: 0:   success
                           -1:   fail
**************************************************/
int destroy_snr_pipeline(buffer_object_t *buf_obj);

/**************************************************
* Get sensor type struct according to sensor pad id
* @param            \b IN: sensor pad id
* @param            \b OUT: sensor type struct
* @return           \b OUT: 0:   success
                           -1:   fail
**************************************************/
int get_sensor_attr(int eSnrPad, Sensor_Attr_t* sensorAttr);

/**************************************************
* Init sensor pipeline param
* @param            \b IN: buffer object
* @param            \b IN: sensor number
**************************************************/
void init_sensor_attr(buffer_object_t *buf_obj, int snr_num);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _SSTAR_SENSOR_H_ */


