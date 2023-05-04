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

#include <string.h>

#include "sstar_sensor.h"
#include "common.h"
#include "mi_isp_cus3a_api.h"
#include "mi_rgn.h"
#include "mi_ipu.h"
#include "mi_sys.h"

#include "common.h"
#include "sstar_drm.h"
#include "libsync.h"

extern buffer_object_t _g_buf_obj[];

pthread_t tid_drm_buf_thread[BUF_NUM];
pthread_t tid_enqueue_buf_thread[BUF_NUM];
int bSensor_inited = 0;


Sensor_Attr_t gstSensorAttr[MAX_SENSOR_NUM];
int _g_isp_dev = 0;

extern void get_output_size(int snr_index, int *out_width, int *out_height);


/*****************************************************************************/
/* MI related Init */

void select_mi_stream(int sensor_id, int stream_id) {
    gstSensorAttr[sensor_id].u8ResIndex = stream_id;
    printf("set resolution to stream %d \n", gstSensorAttr[sensor_id].u8ResIndex);
}

int get_mi_pad_id(int sensor_id) {
    return gstSensorAttr[sensor_id].eSensorPadID;
}

static MI_S32 bind_port(Sys_BindInfo_T* pstBindInfo) {
    MI_S32 ret;

    ret = MI_SYS_BindChnPort2(0, &pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort,
                              pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate,
                              pstBindInfo->eBindType, pstBindInfo->u32BindParam);
    printf("src(%d-%d-%d-%d)  dst(%d-%d-%d-%d)  %d...\n", pstBindInfo->stSrcChnPort.eModId,
           pstBindInfo->stSrcChnPort.u32DevId, pstBindInfo->stSrcChnPort.u32ChnId,
           pstBindInfo->stSrcChnPort.u32PortId, pstBindInfo->stDstChnPort.eModId,
           pstBindInfo->stDstChnPort.u32DevId, pstBindInfo->stDstChnPort.u32ChnId,
           pstBindInfo->stDstChnPort.u32PortId, pstBindInfo->eBindType);

    return ret;
}

int get_vif_from_snrpad(MI_SNR_PADID eSnrPad, MI_VIF_GROUP *vifGroupId, MI_VIF_DEV *u32VifDev)
{
    switch(eSnrPad)
    {
        case 0:
            {
                *vifGroupId = 0;
                *u32VifDev = 0;
                break;
            }
        case 1:
            {
                *vifGroupId = 2;
                *u32VifDev = 8;
                break;
            }
        case 2:
            {
                *vifGroupId = 1;
                *u32VifDev = 4;
                break;
            }
        case 3:
            {
                *vifGroupId = 3;
                *u32VifDev = 12;
                break;
            }
        default:
            {
                printf("Invalid SnrPadid \n");
                return -1;
            }
    }
    return 0;

}


static MI_S32 unbind_port(Sys_BindInfo_T* pstBindInfo) {
    MI_S32 ret;

    ret = MI_SYS_UnBindChnPort(0, &pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort);

    return ret;
}

int get_free_sensorgst()
{
    int i;

    for(i=0; i< MAX_SENSOR_NUM; i++)
    {
        if(gstSensorAttr[i].bUsed == FALSE)
        {
            return i;
        }
    }
    if(i >= MAX_SENSOR_NUM)
    {
        printf("init_sensor_attr fail\n");
    }
    return -1;
}

int get_sensorgst_with_id(MI_SNR_PADID eSnrPadId)
{
    int i;

    for(i=0; i< MAX_SENSOR_NUM; i++)
    {
        if(gstSensorAttr[i].eSensorPadID == eSnrPadId)
        {
            return i;
        }
    }
    if(i >= MAX_SENSOR_NUM)
    {
        printf("get_sensorgst_with_id fail\n");
    }
    return -1;
}

int get_sensor_attr(int eSnrPadId, Sensor_Attr_t* sensorAttr)
{
    if(eSnrPadId > MAX_SENSOR_NUM)
    {
        printf("get_sensor_attr fail,snr_index=%d \n",eSnrPadId);
        return -1;
    }
    int snr_index;
    snr_index = get_sensorgst_with_id(eSnrPadId);
    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error \n");
        return -1;
    }

    memcpy(sensorAttr, &gstSensorAttr[snr_index], sizeof(Sensor_Attr_t));
    return 0;
}

void init_sensor_attr (buffer_object_t *buf_obj, int snr_num)//MI_SNR_PADID eSnrPadId, int bRotate)
{
    MI_VIF_DEV vifDev;
    MI_VIF_GROUP vifGroupId;
    int snr_index = 0;
    MI_SNR_PADID eSnrPadId;
    int bRotate;
    eSnrPadId = buf_obj->sensorIdx;
    bRotate = buf_obj->rotate;

    if(eSnrPadId > MAX_SENSOR_NUM)
    {
        printf("init_sensor_attr fail,snr_index=%d \n",snr_index);
        return ;
    }
    snr_index = get_free_sensorgst();
    if(snr_index == -1)
    {
        return ;
    }
    gstSensorAttr[snr_index].bUsed = TRUE;
    gstSensorAttr[snr_index].eSensorPadID = eSnrPadId;
    printf("init_sensor_attr,gstSensorAttr[%d].eSensorPadID=%d \n", snr_index, eSnrPadId);
    gstSensorAttr[snr_index].isp_eRot = (MI_SYS_Rotate_e) bRotate;

    get_vif_from_snrpad(gstSensorAttr[snr_index].eSensorPadID, &vifGroupId, &vifDev);
    gstSensorAttr[snr_index].u32VifDev = vifDev;
    gstSensorAttr[snr_index].u32VifGroupID = vifGroupId;
    gstSensorAttr[snr_index].u32vifOutPortId = 0; //outport0 can realtime to isp

    gstSensorAttr[snr_index].u32IspDevId = 0;

    if(gstSensorAttr[snr_index].u32VifDev == 0 && gstSensorAttr[snr_index].u32vifOutPortId == 0
        && gstSensorAttr[snr_index].u32IspDevId == 0)
    {
        if(snr_num > 1)
        {
            gstSensorAttr[snr_index].eVif2IspType = E_MI_SYS_BIND_TYPE_FRAME_BASE;//Dual sensor,vif can not realtime to isp,cause only one isp
        }
        else
        {
            gstSensorAttr[snr_index].eVif2IspType = E_MI_SYS_BIND_TYPE_REALTIME; //sigle sensor,outport0 can realtime to isp
        }
    }
    else
    {
        gstSensorAttr[snr_index].eVif2IspType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    }


    if(eSnrPadId == 0)
    {
        gstSensorAttr[snr_index].u32IspChnId = 0;
        if(gstSensorAttr[snr_index].eVif2IspType == E_MI_SYS_BIND_TYPE_REALTIME)
        {
            gstSensorAttr[snr_index].u32IspOutPortId = 0;//outport0 can realtime only
        }
        else
        {
            gstSensorAttr[snr_index].u32IspOutPortId = 1;//outport1 can framemode
        }
    }
    else
    {
        gstSensorAttr[snr_index].u32IspChnId = 1;
        gstSensorAttr[snr_index].u32IspOutPortId = 1;
    }


    if(eSnrPadId == 0)
    {
        if(gstSensorAttr[snr_index].eVif2IspType == E_MI_SYS_BIND_TYPE_REALTIME)
        {
            gstSensorAttr[snr_index].u32SclDevId = 0;//scldev can bind to isp with realtime only
            if(buf_obj->face_detect)
            {
                gstSensorAttr[snr_index].u32HWOutPortMask = E_MI_SCL_HWSCL0|E_MI_SCL_HWSCL2;
                buf_obj->face_sclid = gstSensorAttr[snr_index].u32SclDevId;
            }
            else
            {
                gstSensorAttr[snr_index].u32HWOutPortMask = E_MI_SCL_HWSCL0;
            }
        }
        else if(gstSensorAttr[snr_index].eVif2IspType == E_MI_SYS_BIND_TYPE_FRAME_BASE)
        {
            gstSensorAttr[snr_index].u32SclDevId = 1;
            if(buf_obj->face_detect)
            {
                gstSensorAttr[snr_index].u32HWOutPortMask = E_MI_SCL_HWSCL0|E_MI_SCL_HWSCL1;
                buf_obj->face_sclid = gstSensorAttr[snr_index].u32SclDevId;
            }
            else
            {
                gstSensorAttr[snr_index].u32HWOutPortMask = E_MI_SCL_HWSCL0;
            }
        }
    }
    else
    {
        gstSensorAttr[snr_index].u32SclDevId = 3;
        gstSensorAttr[snr_index].u32HWOutPortMask = E_MI_SCL_HWSCL3;
    }
    gstSensorAttr[snr_index].u32SclChnId = 0;
    gstSensorAttr[snr_index].u32SclOutPortId = 0;
    if(gstSensorAttr[snr_index].u32SclDevId == 0 && gstSensorAttr[snr_index].u32IspDevId == 0 && gstSensorAttr[snr_index].u32IspOutPortId == 0)
    {
        gstSensorAttr[snr_index].eIsp2SclType = E_MI_SYS_BIND_TYPE_REALTIME; //outport0 can realtime to scl
    }
    else
    {
        gstSensorAttr[snr_index].eIsp2SclType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    }


}

static MI_S32 convert_snrid_to_ispid(MI_SNR_PADID eMiSnrPadId,
                                     MI_ISP_BindSnrId_e* peMiIspSnrBindId) {
    switch (eMiSnrPadId) {
        case 0:
            *peMiIspSnrBindId = E_MI_ISP_SENSOR0;
            break;
        case 1:
            *peMiIspSnrBindId = E_MI_ISP_SENSOR1;
            break;
        case 2:
            *peMiIspSnrBindId = E_MI_ISP_SENSOR2;
            break;
        case 3:
            *peMiIspSnrBindId = E_MI_ISP_SENSOR3;
            break;
        default:
            *peMiIspSnrBindId = E_MI_ISP_SENSOR0;
            printf("snrPad%d fail", eMiSnrPadId);
            break;
    }

    return MI_SUCCESS;
}

static MI_S32 sstar_init_snr(MI_U32 eSnrPadId) {
    MI_S32 ret = 0;
    MI_U32 u32ResCount =0;
    MI_U8 u8ResIndex =0;
    MI_U8 u8ChocieRes = 0;
    MI_SNR_Res_t stRes;
    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));
    ret = MI_SNR_SetPlaneMode(eSnrPadId, FALSE);
    if (ret != 0) {
        printf("MI_SNR_SetPlaneMode error: %X \n", ret);
        return ret;
    }

    ret = MI_SNR_QueryResCount(eSnrPadId, &u32ResCount);
    for(u8ResIndex=0; u8ResIndex < u32ResCount; u8ResIndex++)
    {
        MI_SNR_GetRes(eSnrPadId, u8ResIndex, &stRes);
        printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
        u8ResIndex,
        stRes.stCropRect.u16X, stRes.stCropRect.u16Y, stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,
        stRes.stOutputSize.u16Width, stRes.stOutputSize.u16Height,
        stRes.u32MaxFps,stRes.u32MinFps,
        stRes.strResDesc);
    }

	if(eSnrPadId == 0)
	{
		#ifdef CHIP_IS_SSU9383
		u8ChocieRes = 0;  
		#endif
		#ifdef CHIP_IS_SSD2386
		u8ChocieRes = 6; 
		#endif
	}
	
    ret = MI_SNR_SetRes(eSnrPadId, u8ChocieRes);
    if (ret != 0) {
        printf("MI_SNR_SetRes error: %X \n", ret);
        return ret;
    }
    ret = MI_SNR_Enable(eSnrPadId);
    if (ret != 0) {
        printf("MI_SNR_Enable error: %X \n", ret);
        return ret;
    }
    printf("sstar_init_snr ok\n");
    return MI_SUCCESS;
}

static MI_S32 sstar_deinit_snr(MI_SNR_PADID eSnrPadId) {
    if (MI_SNR_Disable(eSnrPadId)) {
        printf("Destroy sensor failed \n");
        return -1;
    }
    printf("sstar_deinit_snr ok\n");
    return 0;
}

static MI_S32 sstar_init_vif(MI_U32 eSnrPadId) {

    MI_S32 ret;
    MI_VIF_DEV vifDev;
    MI_VIF_GROUP GroupId;
    printf("sstar_init_vif \n");
    MI_VIF_GroupAttr_t stGroupAttr;
    MI_SNR_PADInfo_t stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_VIF_DevAttr_t stVifDevAttr;
    MI_VIF_OutputPortAttr_t stVifPortInfo;
    int snr_index;

    MI_U32 u32PlaneId = 0;
    MI_VIF_PORT vifPort;

    snr_index = get_sensorgst_with_id(eSnrPadId);
    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error \n");
        return -1;
    }
    vifDev = gstSensorAttr[snr_index].u32VifDev;
    GroupId = gstSensorAttr[snr_index].u32VifGroupID;
    vifPort = gstSensorAttr[snr_index].u32vifOutPortId;

    memset(&stGroupAttr, 0, sizeof(MI_VIF_GroupAttr_t));
    memset(&stPad0Info, 0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlane0Info, 0, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stVifDevAttr, 0, sizeof(MI_VIF_DevAttr_t));
    memset(&stVifPortInfo, 0, sizeof(MI_VIF_OutputPortAttr_t));

    if (MI_SNR_GetPadInfo(eSnrPadId, &stPad0Info)) {
        printf("MI_SNR_GetPadInfo error \n");
        return -1;
    }

    if (MI_SNR_GetPlaneInfo(eSnrPadId, u32PlaneId, &stSnrPlane0Info)) {
        printf("MI_SNR_GetPlaneInfo error \n");
        return -1;
    }
    gstSensorAttr[snr_index].u16Height = stSnrPlane0Info.stCapRect.u16Height;
    gstSensorAttr[snr_index].u16Width = stSnrPlane0Info.stCapRect.u16Width;


    stGroupAttr.eIntfMode = E_MI_VIF_MODE_MIPI;  //(MI_VIF_IntfMode_e)stPad0Info.eIntfMode;
    stGroupAttr.eWorkMode = E_MI_VIF_WORK_MODE_1MULTIPLEX;
    stGroupAttr.eHDRType = E_MI_VIF_HDR_TYPE_OFF;
    if (stGroupAttr.eIntfMode == E_MI_VIF_MODE_BT656) {
        stGroupAttr.eClkEdge = (MI_VIF_ClkEdge_e)stPad0Info.unIntfAttr.stBt656Attr.eClkEdge;
    } else {
        stGroupAttr.eClkEdge = E_MI_VIF_CLK_EDGE_DOUBLE;
    }

    if (MI_VIF_CreateDevGroup(GroupId, &stGroupAttr)) {
        printf("MI_VIF_CreateDevGroup error \n");
        return -1;
    }

    stVifDevAttr.stInputRect.u16X = stSnrPlane0Info.stCapRect.u16X;
    stVifDevAttr.stInputRect.u16Y = stSnrPlane0Info.stCapRect.u16Y;
    stVifDevAttr.stInputRect.u16Width = stSnrPlane0Info.stCapRect.u16Width;
    stVifDevAttr.stInputRect.u16Height = stSnrPlane0Info.stCapRect.u16Height;
    if (stSnrPlane0Info.eBayerId >= E_MI_SYS_PIXEL_BAYERID_MAX) {
        stVifDevAttr.eInputPixel = stSnrPlane0Info.ePixel;
    } else {
        stVifDevAttr.eInputPixel = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(
                stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
    }

    if (MI_VIF_SetDevAttr(vifDev, &stVifDevAttr)) {
        printf("MI_VIF_SetDevAttr error \n");
        return -1;
    }

    if (MI_VIF_EnableDev(vifDev)) {
        printf("MI_VIF_EnableDev error \n");
        return -1;
    }

    stVifPortInfo.stCapRect.u16X = stSnrPlane0Info.stCapRect.u16X;
    stVifPortInfo.stCapRect.u16Y = stSnrPlane0Info.stCapRect.u16Y;
    stVifPortInfo.stCapRect.u16Width = stSnrPlane0Info.stCapRect.u16Width;
    stVifPortInfo.stCapRect.u16Height = stSnrPlane0Info.stCapRect.u16Height;
    stVifPortInfo.stDestSize.u16Width = stSnrPlane0Info.stCapRect.u16Width;
    stVifPortInfo.stDestSize.u16Height = stSnrPlane0Info.stCapRect.u16Height;
    if (stSnrPlane0Info.eBayerId >= E_MI_SYS_PIXEL_BAYERID_MAX) {
        stVifPortInfo.ePixFormat = stSnrPlane0Info.ePixel;
    } else {
        stVifPortInfo.ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(
                stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
    }
    stVifPortInfo.eFrameRate = E_MI_VIF_FRAMERATE_FULL;

    if (MI_VIF_SetOutputPortAttr(vifDev, vifPort, &stVifPortInfo)) {
        printf("MI_VIF_SetOutputPortAttr error \n");
        return -1;
    }

    MI_SYS_ChnPort_t stChnPort;
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = vifDev;
    stChnPort.u32PortId = vifPort;
    stChnPort.u32ChnId = 0;
    if (MI_SYS_SetChnOutputPortDepth(0, &stChnPort, 0, 6)) {
        printf("MI_SYS_SetChnOutputPortDepth sstar_init_vif error \n");
        return -1;
    }

    ret = MI_VIF_EnableOutputPort(vifDev, vifPort);
    if (ret != 0) {
        printf("MI_VIF_EnableOutputPort error \n");
        return ret;
    }

    printf("MI Vif Init ok \n");

    return 0;
}

static MI_S32 sstar_deinit_vif(MI_U32 eSnrPadId) {


    MI_VIF_DEV vifDev = 0;
    MI_VIF_PORT vifPort = 0;
    MI_VIF_GROUP GroupId;
    int snr_index;
    snr_index = get_sensorgst_with_id(eSnrPadId);
    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error \n");
        return -1;
    }
    vifDev = gstSensorAttr[snr_index].u32VifDev;
    GroupId = gstSensorAttr[snr_index].u32VifGroupID;
    if (MI_VIF_DisableOutputPort(vifDev, vifPort) || MI_VIF_DisableDev(vifDev) ||
        MI_VIF_DestroyDevGroup(GroupId)) {
        printf("Destroy vif failed \n");
        return -1;
    }
    printf("sstar_deinit_vif ok\n");
    return 0;
}

static MI_S32 sstar_init_isp(MI_U32 eSnrPadId, char * iq_path) {

    MI_S32 ret;
    MI_ISP_CHANNEL ispChnId;
    MI_ISP_DEV ispDevId = 0;
    MI_ISP_PORT ispOutPortId = 0;
    MI_ISP_DevAttr_t stCreateDevAttr;
    MI_ISP_ChannelAttr_t stIspChnAttr;
    MI_ISP_ChnParam_t stIspChnParam;
    MI_ISP_OutPortParam_t stIspOutputParam;
    MI_SYS_ChnPort_t stChnPort;
    Sys_BindInfo_T stBindInfo;

    int snr_index;
    snr_index = get_sensorgst_with_id(eSnrPadId);
    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error \n");
        return -1;
    }

    memset(&stCreateDevAttr, 0x0, sizeof(MI_ISP_DevAttr_t));
    stCreateDevAttr.u32DevStitchMask = E_MI_ISP_DEVICEMASK_ID0;
    if(!_g_isp_dev)
    {
        if (MI_ISP_CreateDevice(ispDevId, &stCreateDevAttr)) {
            printf("MI_ISP_CreateDevice error \n");
            return -1;
        }
        _g_isp_dev = 1;
    }
    else
    {
        _g_isp_dev ++;
    }

    ispChnId = gstSensorAttr[snr_index].u32IspChnId;
    ispDevId = gstSensorAttr[snr_index].u32IspDevId;
    ispOutPortId = gstSensorAttr[snr_index].u32IspOutPortId;
    printf("init_sensor_attr,gstSensorAttr[%d].eSensorPadID=%d u32IspChnId=%d\n", snr_index, eSnrPadId,gstSensorAttr[snr_index].u32IspChnId);
    memset(&stIspChnAttr, 0x0, sizeof(MI_ISP_ChannelAttr_t));
    convert_snrid_to_ispid(gstSensorAttr[snr_index].eSensorPadID,
                           (MI_ISP_BindSnrId_e*)&stIspChnAttr.u32SensorBindId);


    if (MI_ISP_CreateChannel(ispDevId, ispChnId, &stIspChnAttr)) {
        printf("MI_ISP_CreateChannel error \n");
        return -1;
    }
    memset(&stIspChnParam, 0x0, sizeof(MI_ISP_ChnParam_t));
    stIspChnParam.eHDRType = E_MI_ISP_HDR_TYPE_OFF;
    stIspChnParam.e3DNRLevel = E_MI_ISP_3DNR_LEVEL1;
    stIspChnParam.eRot = gstSensorAttr[snr_index].isp_eRot;
    stIspChnParam.bMirror = 0;
    stIspChnParam.bFlip = 0;
    if (MI_ISP_SetChnParam(ispDevId, ispChnId, &stIspChnParam)) {
        printf("MI_ISP_SetChnParam error \n");
        return -1;
    }

    if (MI_ISP_StartChannel(ispDevId, ispChnId)) {
        printf("MI_ISP_StartChannel error \n");
        return -1;
    }

    memset(&stIspOutputParam, 0x0, sizeof(MI_ISP_OutPortParam_t));
    stIspOutputParam.stCropRect.u16X = 0;
    stIspOutputParam.stCropRect.u16Y = 0;
    stIspOutputParam.stCropRect.u16Width = gstSensorAttr[snr_index].u16Width;
    stIspOutputParam.stCropRect.u16Height = gstSensorAttr[snr_index].u16Height;
    stIspOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    MI_ISP_SetOutputPortParam(ispDevId, ispChnId, ispOutPortId, &stIspOutputParam);
    if (MI_ISP_EnableOutputPort(ispDevId, ispChnId, ispOutPortId)) {
        printf("MI_ISP_EnableOutputPort error \n");
        return -1;
    }
    stChnPort.eModId = E_MI_MODULE_ID_ISP;
    stChnPort.u32DevId = ispDevId;
    stChnPort.u32ChnId = ispChnId;
    stChnPort.u32PortId = ispOutPortId;
    if (MI_SYS_SetChnOutputPortDepth(0, &stChnPort, 0, 3)) {
        printf("MI_SYS_SetChnOutputPortDepth error \n");
        return -1;
    }

    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = gstSensorAttr[snr_index].u32VifDev;
    stBindInfo.stSrcChnPort.u32PortId = gstSensorAttr[snr_index].u32vifOutPortId;
    stBindInfo.stSrcChnPort.u32ChnId = 0;

    printf("vif u32VifDev=%d u32PortId=%d\n",stBindInfo.stSrcChnPort.u32DevId, stBindInfo.stSrcChnPort.u32PortId);

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_ISP;
    stBindInfo.stDstChnPort.u32DevId = ispDevId;
    stBindInfo.stDstChnPort.u32ChnId = ispChnId;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 60;
    stBindInfo.u32DstFrmrate = 60;
    stBindInfo.eBindType = gstSensorAttr[snr_index].eVif2IspType;
    //if(gstSensorAttr[snr_index].u32VifDev == 4)
    //    return 0;
    printf("dst isp ispDevId=%d ispChnId=%d ispOutPortId=%d eBindType=%d\n",ispDevId, ispChnId, ispOutPortId, stBindInfo.eBindType);
    ret = bind_port(&stBindInfo);
    if (ret != 0) {
        printf("bind vif and isp error \n");
        return ret;
    }
    if(_g_isp_dev == 1)
    {
        MI_IQSERVER_Open();
    }
    if(iq_path != NULL)
    {
        MI_ISP_ApiCmdLoadBinFile(ispDevId, ispChnId, iq_path, 0);
    }
    printf("MI Isp Init done \n");

    return 0;
}

static MI_S32 sstar_deinit_isp(MI_U32 eSnrPadId) {


    MI_ISP_DEV ispDevId;
    MI_ISP_CHANNEL ispChnId;
    MI_ISP_PORT ispOutPortId;
    Sys_BindInfo_T stBindInfo;
    int snr_index;
    snr_index = get_sensorgst_with_id(eSnrPadId);
    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error \n");
        return -1;
    }

    ispDevId = gstSensorAttr[snr_index].u32IspDevId;
    ispChnId = gstSensorAttr[snr_index].u32IspChnId;
    ispOutPortId = gstSensorAttr[snr_index].u32IspOutPortId;


    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = gstSensorAttr[snr_index].u32VifDev;
    stBindInfo.stSrcChnPort.u32PortId = gstSensorAttr[snr_index].u32vifOutPortId;
    stBindInfo.stSrcChnPort.u32ChnId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_ISP;
    stBindInfo.stDstChnPort.u32DevId = ispDevId;
    stBindInfo.stDstChnPort.u32ChnId = ispChnId;
    stBindInfo.stDstChnPort.u32PortId = 0;
    unbind_port(&stBindInfo);


    MI_ISP_DisableOutputPort(ispDevId, ispChnId, ispOutPortId);
    MI_ISP_StopChannel(ispDevId, ispChnId);
    MI_ISP_DestroyChannel(ispDevId, ispChnId);
    if(_g_isp_dev == 1)
    {
        MI_IQSERVER_Close();
        MI_ISP_DestoryDevice(ispDevId);
		_g_isp_dev = 0;
    }
    else
    {
        _g_isp_dev --;
    }
    printf("MI_SCL_DestroyDevice ok \n");
    return 0;
}

static MI_S32 sstar_init_scl(buffer_object_t *buf_obj) {
    MI_S32 ret;
    MI_SCL_DEV SclDevId;
    MI_SCL_CHANNEL SclChnId;
    MI_SCL_PORT SclOutPortId;
    MI_SCL_DevAttr_t stCreateDevAttr;
    MI_SCL_ChannelAttr_t stSclChnAttr;
    MI_SCL_ChnParam_t stSclChnParam;
    MI_ISP_OutPortParam_t stIspOutputParam;
    MI_SCL_OutPortParam_t stSclOutputParam;
    MI_SYS_ChnPort_t stChnPort;
    Sys_BindInfo_T stBindInfo;
    int scl_out_width;
    int scl_out_height;
    int snr_index;
    snr_index = get_sensorgst_with_id(buf_obj->sensorIdx);
    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error \n");
        return -1;
    }

    SclChnId = gstSensorAttr[snr_index].u32SclChnId;
    SclDevId = gstSensorAttr[snr_index].u32SclDevId;
    SclOutPortId = gstSensorAttr[snr_index].u32SclOutPortId;

    memset(&stCreateDevAttr, 0x0, sizeof(MI_SCL_DevAttr_t));
    stCreateDevAttr.u32NeedUseHWOutPortMask = gstSensorAttr[snr_index].u32HWOutPortMask;//E_MI_SCL_HWSCL2|E_MI_SCL_HWSCL3| E_MI_SCL_HWSCL4;//0x7f;
    if (MI_SCL_CreateDevice(SclDevId, &stCreateDevAttr)) {
        printf("MI_SCL_CreateDevice error, SclDevId=%d\n",SclDevId);
        return -1;
    }

    memset(&stSclChnAttr, 0x0, sizeof(MI_SCL_ChannelAttr_t));
    if (MI_SCL_CreateChannel(SclDevId, SclChnId, &stSclChnAttr)) {
        printf("MI_SCL_CreateChannel error \n");
        return -1;
    }

    memset(&stSclChnParam, 0x0, sizeof(MI_SCL_ChnParam_t));
    stSclChnParam.eRot = E_MI_SYS_ROTATE_NONE;
    if (MI_SCL_SetChnParam(SclDevId, SclChnId, &stSclChnParam)) {
        printf("MI_SCL_SetChnParam error \n");
        return -1;
    }

    if (MI_SCL_StartChannel(SclDevId, SclChnId)) {
        printf("MI_SCL_StartChannel error \n");
        return -1;
    }

    memset(&stIspOutputParam, 0x0, sizeof(MI_ISP_OutPortParam_t));
    MI_ISP_GetInputPortCrop(0, gstSensorAttr[snr_index].u32IspChnId, &stIspOutputParam.stCropRect);


    get_output_size(buf_obj->sensorIdx, &scl_out_width ,&scl_out_height);

    //printf("snr_index=%d scl_out_width=%d scl_out_height=%d \n", buf_obj->sensorIdx, scl_out_width, scl_out_height);
    memset(&stSclOutputParam, 0x0, sizeof(MI_SCL_OutPortParam_t));

    stSclOutputParam.stSCLOutCropRect.u16X = stIspOutputParam.stCropRect.u16X;
    stSclOutputParam.stSCLOutCropRect.u16Y = stIspOutputParam.stCropRect.u16Y;
    stSclOutputParam.stSCLOutCropRect.u16Width = stIspOutputParam.stCropRect.u16Width;
    stSclOutputParam.stSCLOutCropRect.u16Height = stIspOutputParam.stCropRect.u16Height;
    stSclOutputParam.stSCLOutputSize.u16Width = ALIGN_BACK(scl_out_width, ALIGN_NUM);
    stSclOutputParam.stSCLOutputSize.u16Height = ALIGN_BACK(scl_out_height, ALIGN_NUM);
    stSclOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stSclOutputParam.bMirror = 0;
    stSclOutputParam.bFlip = 0;
    stSclOutputParam.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    if (MI_SCL_SetOutputPortParam(SclDevId, SclChnId, SclOutPortId, &stSclOutputParam)) {
        printf("MI_SCL_SetOutputPortParam error \n");
        return -1;
    }

    stChnPort.eModId = E_MI_MODULE_ID_SCL;
    stChnPort.u32DevId = SclDevId;
    stChnPort.u32ChnId = SclChnId;
    stChnPort.u32PortId = SclOutPortId;
    if (MI_SYS_SetChnOutputPortDepth(0, &stChnPort, 0, 4)) {
        printf("MI_SYS_SetChnOutputPortDepth error \n");
        return -1;
    }
    printf("MI_SCL_EnableOutputPort,SclDevId=%d SclChnId=%d SclOutPortId=%d eIsp2SclType=%d\n",SclDevId,SclChnId,SclOutPortId,gstSensorAttr[snr_index].eIsp2SclType);
    if (MI_SCL_EnableOutputPort(SclDevId, SclChnId, SclOutPortId)) {
        printf("MI_SCL_EnableOutputPort error \n");
        return -1;
    }

    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_ISP;
    stBindInfo.stSrcChnPort.u32DevId = gstSensorAttr[snr_index].u32IspDevId;
    stBindInfo.stSrcChnPort.u32ChnId = gstSensorAttr[snr_index].u32IspChnId;
    stBindInfo.stSrcChnPort.u32PortId = gstSensorAttr[snr_index].u32IspOutPortId;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stDstChnPort.u32DevId = SclDevId;
    stBindInfo.stDstChnPort.u32ChnId = SclChnId;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 60;
    stBindInfo.u32DstFrmrate = 60;
    stBindInfo.eBindType = gstSensorAttr[snr_index].eIsp2SclType;
    ret = bind_port(&stBindInfo);
    if (ret != 0) {
        printf("bind isp and scl error\n");
        return ret;
    }


    if(buf_obj->face_detect)
    {
    	/************************************************
        Step5:  Init Scl for Algo with face detect
        *************************************************/
    	SclOutPortId = 1;
    	memset(&stSclOutputParam, 0x0, sizeof(MI_SCL_OutPortParam_t));
    	stSclOutputParam.stSCLOutputSize.u16Width = 800;//g_stAlgoRes.width;
    	stSclOutputParam.stSCLOutputSize.u16Height = 480;//g_stAlgoRes.height;
    	stSclOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_ARGB8888;//g_eAlgoFormat;
    	stSclOutputParam.bMirror = 0;
    	stSclOutputParam.bFlip = 0;
    	ExecFunc(MI_SCL_SetOutputPortParam(SclDevId, SclChnId, SclOutPortId, &stSclOutputParam), MI_SUCCESS);
        printf("MI_SCL_EnableOutputPort,SclDevId=%d SclChnId=%d SclOutPortId=%d \n",SclDevId,SclChnId,SclOutPortId);
    	ExecFunc(MI_SCL_EnableOutputPort(SclDevId, SclChnId, SclOutPortId), MI_SUCCESS);

    }

    printf("MI Scl Init done \n");

    return 0;
}

static MI_S32 sstar_deinit_scl(buffer_object_t *buf_obj) {

    MI_SCL_DEV SclDevId;
    MI_SCL_CHANNEL SclChnId;
    MI_SCL_PORT SclOutPortId;
    Sys_BindInfo_T stBindInfo;

    int snr_index;
    snr_index = get_sensorgst_with_id(buf_obj->sensorIdx);
    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error \n");
        return -1;
    }
    SclDevId = gstSensorAttr[snr_index].u32SclDevId;
    SclChnId = gstSensorAttr[snr_index].u32SclChnId;
    SclOutPortId = gstSensorAttr[snr_index].u32SclOutPortId;

    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_ISP;
    stBindInfo.stSrcChnPort.u32DevId = gstSensorAttr[snr_index].u32IspDevId;
    stBindInfo.stSrcChnPort.u32ChnId = gstSensorAttr[snr_index].u32IspChnId;
    stBindInfo.stSrcChnPort.u32PortId = gstSensorAttr[snr_index].u32IspOutPortId;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stDstChnPort.u32DevId = gstSensorAttr[snr_index].u32SclDevId;
    stBindInfo.stDstChnPort.u32ChnId = gstSensorAttr[snr_index].u32SclChnId;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 60;
    stBindInfo.u32DstFrmrate = 60;
    unbind_port(&stBindInfo);


    if (MI_SCL_DisableOutputPort(SclDevId, SclChnId, SclOutPortId)) {
        printf("MI_SCL_DisableOutputPort error,SclChnId=%d SclOutPortId=%d \n",SclChnId,SclOutPortId);
        return -1;
    }

    if(buf_obj->face_detect)
    {
        SclOutPortId = 1;
        if (MI_SCL_DisableOutputPort(SclDevId, SclChnId, SclOutPortId)) {
            printf("MI_SCL_DisableOutputPort error,SclChnId=%d SclOutPortId=%d \n",SclChnId,SclOutPortId);
            return -1;
        }
    }

    if (MI_SCL_StopChannel(SclDevId, SclChnId)) {
        printf("MI_SCL_StopChannel error \n \n");
        return -1;
    }

    if (MI_SCL_DestroyChannel(SclDevId, SclChnId)) {
        printf("MI_SCL_DestroyChannel error \n");
        return -1;
    }

    if (MI_SCL_DestroyDevice(SclDevId)) {
        printf("MI_SCL_DestroyDevice error \n");
        return -1;
    }
    printf("MI_SCL_DestroyDevice ok \n");
    return 0;
}
int _g_stRgnHandle = 0;
static MI_S32 sstar_init_rgn(buffer_object_t *buf_obj)
{
    /************************************************
    Step5:  Init Rgn
    *************************************************/
    MI_RGN_Attr_t stRegion;
    MI_RGN_ChnPort_t stRgnChnPort;
    MI_RGN_ChnPortParam_t stRgnChnPortParam;
    int scl_out_width;
    int scl_out_height;

    int snr_index;
    snr_index = get_sensorgst_with_id(buf_obj->sensorIdx);
    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error \n");
        return -1;
    }

    get_output_size(buf_obj->sensorIdx, &scl_out_width ,&scl_out_height);

    ST_OSD_Init(0);
    stRegion.eType = E_MI_RGN_TYPE_OSD;
    stRegion.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_I4;
    stRegion.stOsdInitParam.stSize.u32Width = scl_out_width;
    stRegion.stOsdInitParam.stSize.u32Height = scl_out_height;
    ST_OSD_Create(0, _g_stRgnHandle, &stRegion);
    stRgnChnPort.eModId = E_MI_MODULE_ID_SCL;
    stRgnChnPort.s32DevId = gstSensorAttr[snr_index].u32SclDevId;
    stRgnChnPort.s32ChnId = gstSensorAttr[snr_index].u32SclChnId;
    stRgnChnPort.s32PortId = gstSensorAttr[snr_index].u32SclOutPortId;
    memset(&stRgnChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stRgnChnPortParam.bShow = TRUE;
    stRgnChnPortParam.stPoint.u32X = 0;
    stRgnChnPortParam.stPoint.u32Y = 0;
    stRgnChnPortParam.unPara.stOsdChnPort.u32Layer = 99;
    ExecFunc(MI_RGN_AttachToChn(0, _g_stRgnHandle, &stRgnChnPort, &stRgnChnPortParam), MI_SUCCESS);
    return 0;
}

static MI_S32 sstar_deinit_rgn(MI_U32 eSnrPadId)
{
    MI_RGN_ChnPort_t stRgnChnPort;
    int snr_index;
    snr_index = get_sensorgst_with_id(eSnrPadId);
    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error,snr_index=%d \n",snr_index);
        return -1;
    }

    stRgnChnPort.eModId = E_MI_MODULE_ID_SCL;
    stRgnChnPort.s32DevId = gstSensorAttr[snr_index].u32SclDevId;
    stRgnChnPort.s32ChnId = gstSensorAttr[snr_index].u32SclChnId;
    stRgnChnPort.s32PortId = gstSensorAttr[snr_index].u32SclOutPortId;
    ExecFunc(MI_RGN_DetachFromChn(0, _g_stRgnHandle, &stRgnChnPort), MI_SUCCESS);
    ST_OSD_Destroy(0, _g_stRgnHandle);
    ST_OSD_Deinit(0);
    printf("sstar_deinit_rgn ok \n");
    return 0;
}

/* Init sensor module.for example,sensor, vif, isp, scl, etc. */
int create_snr_pipeline(buffer_object_t *buf_obj) {
    int ret = 0;
    int sensorIdx;
    sensorIdx = buf_obj->sensorIdx;
    //printf("create_snr_pipeline eSensorPadID=%d\n",sensorIdx);
    ret = sstar_init_snr(sensorIdx) || sstar_init_vif(sensorIdx) ||
          sstar_init_isp(sensorIdx, buf_obj->iq_file) || sstar_init_scl(buf_obj);
    if (ret != 0) {
        printf("create sensor pipeline fail,sensorIdx=%d \n",sensorIdx);
        return ret;
    }

    if(buf_obj->face_detect != 0)
    {
        sstar_init_rgn(buf_obj);
    }

    return 0;
}

int destroy_snr_pipeline(buffer_object_t *buf_obj)
{
    int ret = 0;
    int sensorIdx = buf_obj->sensorIdx;

    if(buf_obj->face_detect)
    {
        sstar_deinit_rgn(sensorIdx);
    }
    ret = sstar_deinit_scl(buf_obj) || sstar_deinit_isp(sensorIdx) ||
          sstar_deinit_vif(sensorIdx) || sstar_deinit_snr(sensorIdx);
    if (ret != 0) {
        printf("destroy sensor pipeline fail \n");
        return ret;
    }

    return 0;
}

int sstar_scl_enqueueOneBuffer(int dev, int chn, MI_SYS_DmaBufInfo_t* mi_dma_buf_info)
{
    MI_SYS_ChnPort_t chnPort;
    // set chn port info
    memset(&chnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    chnPort.eModId = E_MI_MODULE_ID_SCL;
    chnPort.u32DevId = dev;
    chnPort.u32ChnId = chn;
    chnPort.u32PortId = 0;

    // enqueue one dma buffer
    if (MI_SUCCESS != MI_SYS_ChnOutputPortEnqueueDmabuf(&chnPort, mi_dma_buf_info)) {
        printf("call MI_SYS_ChnOutputPortEnqueueDmabuf faild\n");
        return -1;
    }
    return 0;
}

void creat_dmabuf_queue(buffer_object_t *buf_obj)
{
    int i;

    frame_queue_init(&buf_obj->_EnQueue_t, MAX_NUM_OF_DMABUFF, 0);

    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        sstar_creat_dma_obj(&buf_obj->dma_info[i], buf_obj->vdec_info.v_src_size);
        if(buf_obj->dma_info[i].dma_buf_fd > 0)
        {
            buf_obj->dma_info[i].buf_in_use = IDLEQUEUE_STATE;
            //printf("int_buf_obj dma_buf_fd=%d \n",buf_obj->dma_info[i].dma_buf_fd);
        }
    }
}

void destory_dmabuf_queue(buffer_object_t *buf_obj)
{
    int i;
    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        sstar_release_dma_obj(buf_obj->fd, &buf_obj->dma_info[i]);
    }
    frame_queue_destory(&buf_obj->_EnQueue_t);
}

int creat_outport_dmabufallocator(buffer_object_t * buf_obj)
{
    Sensor_Attr_t sensorAttr;
    if(get_sensor_attr(buf_obj->sensorIdx, &sensorAttr) != 0)
    {
        printf("get_sensor_attr fail \n");
        return -1;
    }
    buf_obj->chn_port_info.eModId = E_MI_MODULE_ID_SCL;
    buf_obj->chn_port_info.u32DevId = sensorAttr.u32SclDevId;
    buf_obj->chn_port_info.u32ChnId = sensorAttr.u32SclChnId;
    buf_obj->chn_port_info.u32PortId = sensorAttr.u32SclOutPortId;

    if (MI_SUCCESS != MI_SYS_CreateChnOutputPortDmabufCusAllocator(&buf_obj->chn_port_info))
    {
        printf("MI_SYS_CreateChnOutputPortDmabufCusAllocator failed \n");
        return -1;
    }
    return 0;
}

void* enqueue_buffer_loop(void* param)
{
    int dma_buf_handle = -1;
    MI_SYS_DmaBufInfo_t mi_dma_buf_info;
    int i;
    dma_info_t *dma_info;
    int ret;
    buffer_object_t * buf_obj = (buffer_object_t *)param;
    memset(&mi_dma_buf_info, 0x0, sizeof(MI_SYS_DmaBufInfo_t));

    i = 0;
    while (!buf_obj->bExit_second)
    {
        sem_wait(&buf_obj->sem_avail);
        dma_info = &buf_obj->dma_info[i];
        dma_buf_handle = dma_info->dma_buf_fd;

        mi_dma_buf_info.u16Width = buf_obj->vdec_info.v_src_width;
        mi_dma_buf_info.u16Height = buf_obj->vdec_info.v_src_height;
        mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
        mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
        mi_dma_buf_info.u32Stride[0] = buf_obj->vdec_info.v_out_stride;
        mi_dma_buf_info.u32Stride[1] = buf_obj->vdec_info.v_out_stride;
        mi_dma_buf_info.u32DataOffset[0] = 0;
        mi_dma_buf_info.u32DataOffset[1] = buf_obj->vdec_info.v_src_width * buf_obj->vdec_info.v_src_height;
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        #if 1
        if((dma_info->out_fence != 0) && (dma_info->out_fence != -1))
        {
            ret = sync_wait_sys(dma_info->out_fence, 50);
            if(ret != 0)
            {
                printf("Waring:maybe drop one drm frame, ret=%d out_fence=%d\n", ret, dma_info->out_fence);
            }
            close(dma_info->out_fence);
            dma_info->out_fence = 0;
        }
        #endif
        if(sstar_scl_enqueueOneBuffer(buf_obj->chn_port_info.u32DevId, buf_obj->chn_port_info.u32ChnId, &mi_dma_buf_info) == 0)
        {
            dma_info->buf_in_use = DEQUEUE_STATE;
            frame_queue_putbuf(&buf_obj->_EnQueue_t, (char*)&buf_obj->dma_info[i], sizeof(dma_info_t), NULL);
        }
        else
        {
            printf("sstar_vdec_enqueueOneBuffer FAIL \n");
        }

        if(++i == MAX_NUM_OF_DMABUFF)
        {
            i = 0;
        }
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    }

    printf("Thread enqueue_buffer_loop exit %d\n", buf_obj->sensorIdx);
    
    return (void*)0;
}

void* drm_buffer_loop(void* param)
{
    frame_t *pQueue = NULL;
    int dma_buf_handle = -1;
    dma_info_t *dma_info;
    MI_SYS_DmaBufInfo_t mi_dma_buf_info;
    int timeout = 5000;
    int dequeue_try_cnt = 0;

    buffer_object_t * buf_obj = (buffer_object_t *)param;
    memset(&mi_dma_buf_info, 0x0, sizeof(MI_SYS_DmaBufInfo_t));

    MI_SYS_GetFd(&buf_obj->chn_port_info, &buf_obj->_g_mi_sys_fd);
    //usleep(500*1000);
    while (!buf_obj->bExit)
    {
        pQueue = get_last_queue(&buf_obj->_EnQueue_t, DEQUEUE_STATE);
        if( pQueue !=NULL && pQueue->frame != NULL )
        {
            dma_info = (dma_info_t *)pQueue->frame;
        }
        else
        {
            printf("get_last_queue ENQUEUE_STATE fail,sensorIdx=%d \n",buf_obj->sensorIdx);
            continue;
        }
        dma_buf_handle = dma_info->dma_buf_fd;
        mi_dma_buf_info.u16Width = buf_obj->vdec_info.v_src_width;
        mi_dma_buf_info.u16Height = buf_obj->vdec_info.v_src_height;
        mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
        mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
        mi_dma_buf_info.u32Stride[0] = buf_obj->vdec_info.v_src_stride;
        mi_dma_buf_info.u32Stride[1] = buf_obj->vdec_info.v_src_stride;
        mi_dma_buf_info.u32DataOffset[0] = 0;
        mi_dma_buf_info.u32DataOffset[1] = buf_obj->vdec_info.v_src_width * buf_obj->vdec_info.v_src_height;

        sync_wait_sys(buf_obj->_g_mi_sys_fd, timeout);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        if (dma_buf_handle > 0)
        {
            while(0 != MI_SYS_ChnOutputPortDequeueDmabuf(&buf_obj->chn_port_info, &mi_dma_buf_info) && dequeue_try_cnt < 10)
            {
                dequeue_try_cnt++;
                sync_wait_sys(buf_obj->_g_mi_sys_fd, 10);
            }

            #if 1
            if((mi_dma_buf_info.u32Status != MI_SYS_DMABUF_STATUS_INVALID) && (mi_dma_buf_info.u32Status != MI_SYS_DMABUF_STATUS_DROP))
            {
                if(0 != sstar_drm_update(buf_obj, dma_buf_handle))
                {
                    printf("Waring: drm update frame buffer failed \n");
                }
            }
            else
            {
                printf("Error frame,u32Status=0x%x  0x%x 0x%x\n",mi_dma_buf_info.u32Status,MI_SYS_DMABUF_STATUS_DONE,MI_SYS_DMABUF_STATUS_DROP);
            }
            #endif

            dma_info->buf_in_use = IDLEQUEUE_STATE;
            frame_queue_next(&buf_obj->_EnQueue_t, pQueue);
            timeout = 100;
            sem_post(&buf_obj->sem_avail);
        }
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    }

    printf("Thread drm_buffer_loop exit %d\n", buf_obj->sensorIdx);
	
	buf_obj->bExit_second = 1;
	
    return (void*)0;
}

int lv_demo_init_ipu_sensor()
{
	int i, ret;
	
	//sstar_drm_init(&_g_buf_obj[1]);  
	
	for(i=0; i < BUF_NUM; i++)
    {	 
    	_g_buf_obj[i].bExit = 0;
		_g_buf_obj[i].bExit_second = 0;
    	int_buf_obj(i);
			
    	if(bSensor_inited == 0)
		{
			init_sensor_attr(&_g_buf_obj[i], BUF_NUM);
		}

		sem_init(&_g_buf_obj[i].sem_avail, 0, MAX_NUM_OF_DMABUFF);
		memset(_g_buf_obj[i].dma_info, 0, sizeof(dma_info_t) * MAX_NUM_OF_DMABUFF);
		
		creat_dmabuf_queue(&_g_buf_obj[i]);
        create_snr_pipeline(&_g_buf_obj[i]);
        creat_outport_dmabufallocator(&_g_buf_obj[i]);
        if(_g_buf_obj[i].face_detect != 0)
        {
            sstar_algo_init(&_g_buf_obj[i]);
        }

		printf("_g_buf_obj[%d].drm_commited = %d\n", i, _g_buf_obj[i].drm_commited);
		
        ret = pthread_create(&tid_enqueue_buf_thread[i], NULL, enqueue_buffer_loop, (void*)&_g_buf_obj[i]);
		if(ret != 0)
		{
			printf("pthread_create error = %d , tid_enqueue_buf_thread[%d]\n", ret, i);
		}
        ret = pthread_create(&tid_drm_buf_thread[i], NULL, drm_buffer_loop, (void*)&_g_buf_obj[i]);
		if(ret != 0)
		{
			printf("pthread_create error = %d , tid_enqueue_buf_thread[%d]\n", ret, i);
		}
	}
	bSensor_inited = 1;
	
	return 0;
}

int lv_demo_deinit_ipu_sensor()
{
	int i;
	for(i=0; i < BUF_NUM; i++)
	{
		_g_buf_obj[i].bExit = 1;
	}

	for(i=0; i < BUF_NUM; i++)
	{	
		if(tid_drm_buf_thread[i])
		{
			printf("pthread_join tid_drm_buf_thread %d\n", i);
		   pthread_join(tid_drm_buf_thread[i], NULL);
		   sem_post(&_g_buf_obj[i].sem_avail);
		}

		if(tid_enqueue_buf_thread[i])
		{
			printf("pthread_join tid_enqueue_buf_thread %d\n", i);
		   pthread_join(tid_enqueue_buf_thread[i], NULL);
		}
	}

	for(i=0; i < BUF_NUM; i++)
	{	   
	   if(_g_buf_obj[i].face_detect)
	   {
		   sstar_algo_deinit();
	   }
	   destroy_snr_pipeline(&_g_buf_obj[i]);
	   //deint_buf_obj(&_g_buf_obj[i]);
	   destory_dmabuf_queue(&_g_buf_obj[i]);
	   _g_buf_obj[i].drm_commited = 0;
	}

	printf("lv_demo_deinit_ipu_sensor \n");
	return 0;
}


