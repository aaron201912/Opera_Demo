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

#ifndef _SSTAR_SENSOR_DEMO_H_
#define _SSTAR_SENSOR_DEMO_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "mi_common_datatype.h"
#include "mi_isp.h"
#include "mi_sensor.h"
#include "mi_sensor_datatype.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "mi_vif.h"
#include "mi_iqserver.h"


#define MAX_SENSOR_NUM (1)
#define MI_SYS_DEVICE_ID (0)


/***************************************************************
SENSOR PARAM
***************************************************************/
typedef struct Sensor_Attr_s{
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


    MI_BOOL bUsed;
    MI_BOOL bPlaneMode;
    MI_U8 u8ResIndex;
    MI_U16 u16Width;
    MI_U16 u16Height;
} Sensor_Attr_t;

/***************************************************************
SYSBIND PARAM
***************************************************************/
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
int sstar_SnrPipeLine_Creat(MI_SNR_PADID eSnrPadId, char * iq_path);

/**************************************************
* destroy sensor -> vif -> scl -> venc pipeline, venc select by venc_flag
* @param            \b IN: buffer object
* @return           \b OUT: 0:   success
                           -1:   fail
**************************************************/
int sstar_SnrPipeLine_Destory(MI_SNR_PADID eSnrPadId);

int sstar_get_sensor_attr(int eSnrPadId, Sensor_Attr_t* sensorAttr);

int sstar_get_snr_frame(MI_SYS_FrameData_t *stFrameData, MI_SYS_BUF_HANDLE *stBufHandle);

int sstar_put_snr_frame(MI_SYS_BUF_HANDLE stBufHandle);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _SSTAR_SENSOR_DEMO_H_ */



