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

#include "mi_common_datatype.h"
#include "mi_isp.h"
#include "mi_scl.h"
#include "mi_sensor.h"
#include "mi_sensor_datatype.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "mi_vif.h"
#include "mi_iqserver.h"
#include "common.h"

#define MAX_SENSOR_NUM (2)
#define MI_SYS_DEVICE_ID (0)
#define MAX_NUM_OF_DMABUFF 3


typedef struct {
    MI_VIF_SNRPad_e eSensorPadID;

    MI_VIF_GROUP u32VifGroupID;
    MI_U32 u32VifDev;
    MI_VIF_PORT u32vifOutPortId;
    MI_SYS_BindType_e eVif2IspType;


    MI_ISP_CHANNEL u32IspChnId;
    MI_ISP_DEV u32IspDevId;
    MI_ISP_PORT u32IspOutPortId;
    MI_SYS_Rotate_e   isp_eRot;
    MI_SYS_BindType_e eIsp2SclType;

    MI_SCL_DEV u32SclDevId ;
    MI_SCL_CHANNEL u32SclChnId;
    MI_SCL_PORT u32SclOutPortId ;
    MI_U32 u32HWOutPortMask;

    MI_BOOL bUsed;
    MI_BOOL bPlaneMode;
    MI_U8 u8ResIndex;
    MI_U16 u16Width;
    MI_U16 u16Height;
} Sensor_Attr_t;

typedef struct {
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
    MI_SYS_BindType_e eBindType;
    MI_U32 u32BindParam;
} Sys_BindInfo_T;

int create_snr_pipeline(buffer_object_t *buf_obj);
int destroy_snr_pipeline(buffer_object_t *buf_obj);
int sstar_scl_enqueueOneBuffer(int dev, int chn, MI_SYS_DmaBufInfo_t* mi_dma_buf_info);
int get_sensor_attr(int eSnrPad, Sensor_Attr_t* sensorAttr);
void init_sensor_attr (buffer_object_t *buf_obj, int snr_num);

#endif /* _SSTAR_SENSOR_H_ */

