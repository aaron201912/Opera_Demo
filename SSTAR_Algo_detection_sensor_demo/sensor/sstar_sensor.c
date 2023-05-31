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

#include "common.h"
//#include "st_rgn.h"
#include "mi_venc_datatype.h"
#include "mi_venc.h"

static int snr_num = 1;

Sensor_Attr_t gstSensorAttr[MAX_SENSOR_NUM];
int _g_isp_dev = 0;


/*****************************************************************************/
/* MI related Init */

const char* Get_HDRType(int HDRType)
{
	static char HDRMode[20];
	memset(HDRMode, 0, sizeof(HDRMode));
	switch(HDRType)
	{
		case 0:
			strncpy(HDRMode, "HDR_OFF", 7);
			break;
		case 1:
			strncpy(HDRMode, "HDR_VC", 6);
			break;			
		case 2:
			strncpy(HDRMode, "HDR_DOL", 7);
			break;
		case 3:
			strncpy(HDRMode, "HDR_COMP", 8);
			break;
		case 4:
			strncpy(HDRMode, "HDR_LI", 6);
			break;
		case 5:
			strncpy(HDRMode, "HDR_COMPVS", 10);
			break;
		case 6:
			strncpy(HDRMode, "HDR_DCG", 7);
			break;
		case 7:
			strncpy(HDRMode, "HDR_MAX", 7);
			printf("HDRType err! \n");
			return NULL;
			break;
	}
	return HDRMode;
}

static void ST_Flush(void)
{
	char c;
	while((c = getchar()) != '\n' && c != EOF);
}

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

/***************************************************************
SNR INIT AND DEINT
***************************************************************/
static MI_S32 sstar_init_snr(MI_U32 eSnrPadId, int eHdr) {
    MI_S32 ret = 0;
    MI_U32 u32ResCount =0;
    MI_U8 u8ResIndex =0;
    MI_U8 u8ChocieRes = 0;
    MI_SNR_Res_t stRes;
	MI_S32 s32Input =0;
    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));
	if(1 == eHdr)
		ret = MI_SNR_SetPlaneMode(eSnrPadId, TRUE);
	else
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
    printf("choice which resolution use, cnt %d\n", u32ResCount);
    do
    {
    	scanf("%d", &s32Input);
        u8ChocieRes = (MI_U8)s32Input;
        ST_Flush();
        MI_SNR_QueryResCount(eSnrPadId, &u32ResCount);
        if(u8ChocieRes >= u32ResCount)
        {
        	printf("choice err res %d > =cnt %d\n", u8ChocieRes, u32ResCount);
        }
    }while(u8ChocieRes >= u32ResCount);
    printf("You select %d res\n", u8ChocieRes);	
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
	
	//if(eSnrPadId == 0)
	//	MI_SNR_SetOrien(eSnrPadId, 0, 1);
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
/***************************************************************
VIF INIT AND DEINT
***************************************************************/
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

static MI_S32 sstar_init_vif(MI_U32 eSnrPadId, int eHdr) {

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
	if(1 == eHdr){
		stGroupAttr.eHDRType =	stPad0Info.eHDRMode;
		printf("VIF Set HDRType [%s] \n", Get_HDRType(stGroupAttr.eHDRType));
	}
	else
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
/***************************************************************
ISP INIT AND DEINT
***************************************************************/

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

static MI_S32 sstar_init_isp(MI_U32 eSnrPadId, char * iq_path, int eHdr) {

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
    MI_SNR_PADInfo_t stPad0Info;

    int snr_index;
    snr_index = get_sensorgst_with_id(eSnrPadId);
    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error \n");
        return -1;
    }
	
	if(1 == eHdr){	
		memset(&stPad0Info, 0, sizeof(MI_SNR_PADInfo_t));
    	if (MI_SNR_GetPadInfo(eSnrPadId, &stPad0Info)) {
        	printf("MI_SNR_GetPadInfo error \n");
        	return -1;
    	}
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
	if(1 == eHdr){
		stIspChnParam.eHDRType = stPad0Info.eHDRMode;
		printf("ISP Set HDRType [%s] \n", Get_HDRType(stIspChnParam.eHDRType));
	}
	else
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
    printf("ISP output width=%d, height=%d\n", gstSensorAttr[snr_index].u16Width, gstSensorAttr[snr_index].u16Height);
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
        printf("MI_ISP_ApiCmdLoadBinFile Isp:%d Chn:%d path:%s \n", ispDevId, ispChnId, iq_path);
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
/***************************************************************
VENC INIT AND DEINT
***************************************************************/
static int sstar_init_venc(buffer_object_t *buf_obj)
{
    /************************************************
    Step8:  venc init
    *************************************************/
    MI_VENC_DEV VencDevId = 0;
    MI_VENC_CHN VencChnId = 0;
    MI_S32 VencPortId = 0;
    MI_VENC_ChnAttr_t           stVencChnAttr;
    MI_VENC_InputSourceConfig_t stVencInputSourceConfig;
	MI_VENC_InitParam_t stInitParam;
    int snr_index;

    Sys_BindInfo_T stBindInfo;
	VencChnId = buf_obj->vencChn;

    snr_index = get_sensorgst_with_id(buf_obj->sensorIdx);
    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error \n");
        return -1;
    }

	memset(&stInitParam, 0, sizeof(MI_VENC_InitParam_t));
	stInitParam.u32MaxWidth = 1920;
    stInitParam.u32MaxHeight = 1080;
	ExecFunc(MI_VENC_CreateDev(VencDevId, &stInitParam), MI_SUCCESS);

    /* 设置编码器属性与码率控制属性 */
    memset(&stVencChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    stVencChnAttr.stVeAttr.stAttrH264e.u32PicWidth         = ALIGN_BACK(buf_obj->vdec_info.v_out_width, ALIGN_NUM);
    stVencChnAttr.stVeAttr.stAttrH264e.u32PicHeight        = ALIGN_BACK(buf_obj->vdec_info.v_out_height, ALIGN_NUM);
    stVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth      = ALIGN_BACK(buf_obj->vdec_info.v_out_width, ALIGN_NUM);
    stVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight     = ALIGN_BACK(buf_obj->vdec_info.v_out_height, ALIGN_NUM);
    stVencChnAttr.stVeAttr.stAttrH264e.bByFrame            = TRUE;
    stVencChnAttr.stVeAttr.stAttrH264e.u32BFrameNum        = 2;
    stVencChnAttr.stVeAttr.stAttrH264e.u32Profile          = 1;
    stVencChnAttr.stVeAttr.eType                           = E_MI_VENC_MODTYPE_H264E;
    stVencChnAttr.stRcAttr.eRcMode                         = E_MI_VENC_RC_MODE_H264CBR;
    stVencChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate        = 1024 * 1024 * 4;
    stVencChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum  = 30;
    stVencChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen  = 1;
    stVencChnAttr.stRcAttr.stAttrH264Cbr.u32Gop            = 30;
    stVencChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
    stVencChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime       = 0;

    printf("venc start MI_VENC_CreateChn\n");
    ExecFunc(MI_VENC_CreateChn(VencDevId, VencChnId, &stVencChnAttr), MI_SUCCESS);

    /*! 设置输入源的属性
     *  设置了E_MI_VENC_INPUT_MODE_RING_ONE_FRM那APP 在调用 MI_SYS_BindChnPort2 需要设置
     *  E_MI_SYS_BIND_TYPE_HW_RING/和相应 ring buffer 高度
     *  */
    memset(&stVencInputSourceConfig, 0, sizeof(MI_VENC_InputSourceConfig_t));
    stVencInputSourceConfig.eInputSrcBufferMode = E_MI_VENC_INPUT_MODE_NORMAL_FRMBASE;
    ExecFunc(MI_VENC_SetInputSourceConfig(VencDevId, VencChnId, &stVencInputSourceConfig), MI_SUCCESS);

    ExecFunc(MI_VENC_SetMaxStreamCnt(VencDevId, VencChnId, 4), MI_SUCCESS);

	memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stSrcChnPort.u32DevId = gstSensorAttr[snr_index].u32SclDevId;
    stBindInfo.stSrcChnPort.u32ChnId = gstSensorAttr[snr_index].u32SclChnId;
    stBindInfo.stSrcChnPort.u32PortId = gstSensorAttr[snr_index].u32SclOutPortId;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = VencDevId;
    stBindInfo.stDstChnPort.u32ChnId = VencChnId;
	stBindInfo.stDstChnPort.u32PortId = VencPortId;
    stBindInfo.u32SrcFrmrate = 60;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
	//stBindInfo.u32BindParam = stSnrPlaneInfo.stCapRect.u16Height;
    ExecFunc(bind_port(&stBindInfo), MI_SUCCESS);
    printf("bind SCL(%d %d %d)->VENC(%d %d 0),bind type:%d ", (int)gstSensorAttr[snr_index].u32SclDevId, (int)gstSensorAttr[snr_index].u32SclChnId,
               (int)gstSensorAttr[snr_index].u32SclOutPortId, (int)VencDevId, (int)VencChnId, stBindInfo.eBindType);
	ExecFunc(MI_VENC_StartRecvPic(VencDevId, VencChnId), MI_SUCCESS);
    return 0;

}

static int sstar_deinit_venc(buffer_object_t *buf_obj)
{
    Sys_BindInfo_T stBindInfo;
    int snr_index;
    snr_index = get_sensorgst_with_id(buf_obj->sensorIdx);

    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error \n");
        return -1;
    }

    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stSrcChnPort.u32DevId = gstSensorAttr[snr_index].u32SclDevId;
    stBindInfo.stSrcChnPort.u32ChnId = gstSensorAttr[snr_index].u32SclChnId;
    stBindInfo.stSrcChnPort.u32PortId = gstSensorAttr[snr_index].u32SclOutPortId;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = buf_obj->vencChn;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 60;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    ExecFunc(unbind_port(&stBindInfo), MI_SUCCESS);
	ExecFunc(MI_VENC_StopRecvPic(0, buf_obj->vencChn), MI_SUCCESS);
    ExecFunc(MI_VENC_DestroyChn(0, buf_obj->vencChn), MI_SUCCESS);
	ExecFunc(MI_VENC_DestroyDev(0), MI_SUCCESS);
    return 0;
}
/***************************************************************
SCL INIT AND DEINT
***************************************************************/
static MI_S32 sstar_init_scl(buffer_object_t *buf_obj) 
{
    MI_S32 ret;
    MI_SCL_DEV SclDevId;
    MI_SCL_CHANNEL SclChnId;
    MI_SCL_PORT SclOutPortId;
    MI_SCL_DevAttr_t stCreateDevAttr;
    MI_SCL_ChannelAttr_t stSclChnAttr;
    MI_SCL_ChnParam_t stSclChnParam;
    MI_SCL_OutPortParam_t stSclOutputParam;
    MI_SYS_ChnPort_t stChnPort;
    Sys_BindInfo_T stBindInfo;
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
    stCreateDevAttr.u32NeedUseHWOutPortMask = gstSensorAttr[snr_index].u32HWOutPortMask;
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


    memset(&stSclOutputParam, 0x0, sizeof(MI_SCL_OutPortParam_t));
    MI_SCL_GetInputPortCrop(SclDevId, SclChnId, &stSclOutputParam.stSCLOutCropRect);
    stSclOutputParam.stSCLOutCropRect.u16X = stSclOutputParam.stSCLOutCropRect.u16X;
    stSclOutputParam.stSCLOutCropRect.u16Y = stSclOutputParam.stSCLOutCropRect.u16Y;
    stSclOutputParam.stSCLOutCropRect.u16Width = stSclOutputParam.stSCLOutCropRect.u16Width;
    stSclOutputParam.stSCLOutCropRect.u16Height = stSclOutputParam.stSCLOutCropRect.u16Height;
	if ((buf_obj->scl_rotate == E_MI_SYS_ROTATE_90) || (buf_obj->scl_rotate == E_MI_SYS_ROTATE_270))
    {    
        stSclOutputParam.stSCLOutputSize.u16Width = ALIGN_BACK(buf_obj->vdec_info.v_out_height, ALIGN_NUM);
        stSclOutputParam.stSCLOutputSize.u16Height = ALIGN_BACK(buf_obj->vdec_info.v_out_width, ALIGN_NUM);
    }else
    {
        stSclOutputParam.stSCLOutputSize.u16Width = ALIGN_BACK(buf_obj->vdec_info.v_out_width, ALIGN_NUM);
        stSclOutputParam.stSCLOutputSize.u16Height = ALIGN_BACK(buf_obj->vdec_info.v_out_height, ALIGN_NUM);
    }
    printf("SCL%d Crop.Width=%d CropRect.Height=%d Out.Width=%d Out.Height=%d\n",SclDevId, stSclOutputParam.stSCLOutCropRect.u16Width, stSclOutputParam.stSCLOutCropRect.u16Height,
        stSclOutputParam.stSCLOutputSize.u16Width, stSclOutputParam.stSCLOutputSize.u16Height);
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
    #if 1
    if (buf_obj->scl_rotate > E_MI_SYS_ROTATE_NONE)
    {
        /************************************************
        Step6:  scl rotate
        *************************************************/
        SclDevId = 7;
        SclChnId = 0;
        SclOutPortId = 0;
        memset(&stCreateDevAttr, 0x0, sizeof(MI_SCL_DevAttr_t));
        memset(&stSclChnAttr, 0x0, sizeof(MI_SCL_ChannelAttr_t));
        memset(&stSclChnParam, 0x0, sizeof(MI_SCL_ChnParam_t));
        stCreateDevAttr.u32NeedUseHWOutPortMask = E_MI_SCL_HWSCL1;
        stSclChnParam.eRot = gstSensorAttr[snr_index].scl_eRot;
        
        ExecFunc(MI_SCL_CreateDevice(SclDevId, &stCreateDevAttr), MI_SUCCESS);
        ExecFunc(MI_SCL_CreateChannel(SclDevId, SclChnId, &stSclChnAttr), MI_SUCCESS);
        ExecFunc(MI_SCL_SetChnParam(SclDevId, SclChnId, &stSclChnParam), MI_SUCCESS);
        ExecFunc(MI_SCL_StartChannel(SclDevId, SclChnId), MI_SUCCESS);
        printf("scl chn start, (%d %d)", (int)SclDevId, (int)SclChnId);
        memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
        stBindInfo.stSrcChnPort.u32DevId = gstSensorAttr[snr_index].u32SclDevId;
        stBindInfo.stSrcChnPort.u32ChnId = gstSensorAttr[snr_index].u32SclChnId;
        stBindInfo.stSrcChnPort.u32PortId = gstSensorAttr[snr_index].u32SclOutPortId;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
        stBindInfo.stDstChnPort.u32DevId = SclDevId;
        stBindInfo.stDstChnPort.u32ChnId = SclChnId;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 60;
        stBindInfo.u32DstFrmrate = 60;
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        ExecFunc(bind_port(&stBindInfo), MI_SUCCESS);
        printf("bind SCL(%d %d %d)->SCL(%d %d),bind type:%d ", (int)gstSensorAttr[snr_index].u32SclDevId, (int)gstSensorAttr[snr_index].u32SclChnId,
                (int)gstSensorAttr[snr_index].u32SclOutPortId, (int)SclDevId, (int)SclChnId, (int)stBindInfo.eBindType);
        memset(&stSclOutputParam, 0x0, sizeof(MI_SCL_OutPortParam_t));     
        ExecFunc(MI_SCL_GetInputPortCrop(SclDevId, SclChnId, &stSclOutputParam.stSCLOutCropRect), MI_SUCCESS);
        printf("MI_SCL_GetInputPortCrop (%d, %d, %d, %d) \n",stSclOutputParam.stSCLOutCropRect.u16X,stSclOutputParam.stSCLOutCropRect.u16Y,
            stSclOutputParam.stSCLOutCropRect.u16Width, stSclOutputParam.stSCLOutCropRect.u16Height);
        stSclOutputParam.stSCLOutCropRect.u16X = 0;
        stSclOutputParam.stSCLOutCropRect.u16Y = 0;
        stSclOutputParam.stSCLOutCropRect.u16Width = 0;
        stSclOutputParam.stSCLOutCropRect.u16Height = 0;
        stSclOutputParam.stSCLOutputSize.u16Width = ALIGN_BACK(buf_obj->vdec_info.v_out_width, ALIGN_NUM);
        stSclOutputParam.stSCLOutputSize.u16Height = ALIGN_BACK(buf_obj->vdec_info.v_out_height, ALIGN_NUM);
        printf("SCL7 Crop.Width=%d CropRect.Height=%d Out.Width=%d Out.Height=%d\n",stSclOutputParam.stSCLOutCropRect.u16Width, stSclOutputParam.stSCLOutCropRect.u16Height,
            stSclOutputParam.stSCLOutputSize.u16Width, stSclOutputParam.stSCLOutputSize.u16Height);  
        stSclOutputParam.bFlip = FALSE;
        stSclOutputParam.bMirror = FALSE;
        stSclOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        ExecFunc(MI_SCL_SetOutputPortParam(SclDevId, SclChnId, SclOutPortId, &stSclOutputParam), MI_SUCCESS);
        memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        stChnPort.eModId = E_MI_MODULE_ID_SCL;
        stChnPort.u32DevId = SclDevId;
        stChnPort.u32ChnId = SclChnId;
        stChnPort.u32PortId = SclOutPortId;
        ExecFunc(MI_SYS_SetChnOutputPortDepth(0, &stChnPort, 0, 3), MI_SUCCESS);
        ExecFunc(MI_SCL_EnableOutputPort(SclDevId, SclChnId, SclOutPortId), MI_SUCCESS);
    }
    #endif
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
    #if 1
    if(buf_obj->scl_rotate > E_MI_SYS_ROTATE_NONE)
    {
        memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
        stBindInfo.stSrcChnPort.u32DevId = SclDevId;
        stBindInfo.stSrcChnPort.u32ChnId = SclChnId;
        stBindInfo.stSrcChnPort.u32PortId = SclOutPortId;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
        stBindInfo.stDstChnPort.u32DevId = 7;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 60;
        stBindInfo.u32DstFrmrate = 60;
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        ExecFunc(unbind_port(&stBindInfo), MI_SUCCESS);    

        ExecFunc(MI_SCL_DisableOutputPort(7, 0, 0), MI_SUCCESS);
        ExecFunc(MI_SCL_StopChannel(7, 0), MI_SUCCESS);
        ExecFunc(MI_SCL_DestroyChannel(7, 0), MI_SUCCESS);
        ExecFunc(MI_SCL_DestroyDevice(7), MI_SUCCESS);

    }
    #endif
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
/************************************************************
Init sensor module.for example,sensor, vif, isp, scl, etc. 
************************************************************/
int create_snr_pipeline(buffer_object_t *buf_obj) {
    int ret = 0;
    int sensorIdx;
	int Hdr;
    sensorIdx = buf_obj->sensorIdx;
	Hdr = buf_obj->Hdr_Used;
    //printf("create_snr_pipeline eSensorPadID=%d\n",sensorIdx);
    ret = sstar_init_snr(sensorIdx, Hdr) || sstar_init_vif(sensorIdx, Hdr) ||
          sstar_init_isp(sensorIdx, buf_obj->iq_file, Hdr) || sstar_init_scl(buf_obj);
    if (ret != 0) {
        printf("create sensor pipeline fail,sensorIdx=%d \n",sensorIdx);
        return ret;
    }
    if(buf_obj->venc_flag != 0)
    {
        sstar_init_venc(buf_obj);
    }
    return 0;
}

int destroy_snr_pipeline(buffer_object_t *buf_obj)
{
    int ret = 0;
    int sensorIdx = buf_obj->sensorIdx;

    if(buf_obj->venc_flag != 0)
    {
        sstar_deinit_venc(buf_obj);
    }
    ret = sstar_deinit_scl(buf_obj) || sstar_deinit_isp(sensorIdx) ||
          sstar_deinit_vif(sensorIdx) || sstar_deinit_snr(sensorIdx);
    if (ret != 0) {
        printf("destroy sensor pipeline fail \n");
        return ret;
    }

    return 0;
}

void init_sensor_attr(buffer_object_t *buf_obj, int eSnr_Num)//MI_SNR_PADID eSnrPadId, int bRotate)
{
    MI_VIF_DEV vifDev;
    MI_VIF_GROUP vifGroupId;
    int snr_index = 0;
    MI_SNR_PADID eSnrPadId;
    int bRotate;
    eSnrPadId = buf_obj->sensorIdx;
    snr_num = eSnr_Num;

    if(eSnrPadId > MAX_SENSOR_NUM)
    {
        printf("init_sensor_attr fail,snr_index=%d \n",snr_index);
        return ;
    }
    snr_index = get_free_sensorgst();
    if(snr_index == -1)
    {
        return;
    }
	gstSensorAttr[snr_index].bUsed = TRUE;
    gstSensorAttr[snr_index].eSensorPadID = (MI_VIF_SNRPad_e)eSnrPadId;
    printf("init_sensor_attr,gstSensorAttr[%d].eSensorPadID=%d \n", snr_index, eSnrPadId);
    gstSensorAttr[snr_index].isp_eRot = (MI_SYS_Rotate_e) buf_obj->isp_rotate;
    gstSensorAttr[snr_index].scl_eRot = (MI_SYS_Rotate_e) buf_obj->scl_rotate;
	if(snr_num == 1)
	{
    	get_vif_from_snrpad(gstSensorAttr[snr_index].eSensorPadID, &vifGroupId, &vifDev);
    	gstSensorAttr[snr_index].u32VifDev = vifDev;
    	gstSensorAttr[snr_index].u32VifGroupID = vifGroupId;
    	gstSensorAttr[snr_index].u32vifOutPortId = 0; //outport0 can realtime to isp
		gstSensorAttr[snr_index].eVif2IspType = E_MI_SYS_BIND_TYPE_REALTIME;//sigle sensor,outport0 can realtime to isp

		gstSensorAttr[snr_index].u32IspDevId = 0;
        gstSensorAttr[snr_index].u32IspChnId = 0;
        gstSensorAttr[snr_index].u32IspOutPortId = 0;//outport0 can realtime only
		gstSensorAttr[snr_index].eIsp2SclType = E_MI_SYS_BIND_TYPE_REALTIME; //outport0 can realtime to scl 

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
    	gstSensorAttr[snr_index].u32SclChnId = 0;
		gstSensorAttr[snr_index].u32SclOutPortId = 0;
	}
	else if(snr_num == 2)
	{
    	get_vif_from_snrpad(gstSensorAttr[snr_index].eSensorPadID, &vifGroupId, &vifDev);
    	gstSensorAttr[snr_index].u32VifDev = vifDev;
    	gstSensorAttr[snr_index].u32VifGroupID = vifGroupId;
    	gstSensorAttr[snr_index].u32vifOutPortId = 0; //outport0 can realtime to isp
		gstSensorAttr[snr_index].eVif2IspType = E_MI_SYS_BIND_TYPE_FRAME_BASE;//Dual sensor,vif can not realtime to isp,cause only one isp

    	gstSensorAttr[snr_index].u32IspDevId = 0;
    	if(snr_index == 0)
    	{
        	gstSensorAttr[snr_index].u32IspChnId = 0;
            gstSensorAttr[snr_index].u32IspOutPortId = 1;//outport1 can framemode
    	}
		else
		{
            gstSensorAttr[snr_index].u32IspChnId = 1;
            gstSensorAttr[snr_index].u32IspOutPortId = 1;
		}
		gstSensorAttr[snr_index].eIsp2SclType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        if(snr_index == 0)
		    gstSensorAttr[snr_index].u32SclDevId = 1;
        else
		    gstSensorAttr[snr_index].u32SclDevId = 3; 
		gstSensorAttr[snr_index].u32SclChnId = 0;
    	gstSensorAttr[snr_index].u32SclOutPortId = 0;
    	if(snr_index == 0)
    	{
            if(buf_obj->face_detect)
            {
                gstSensorAttr[snr_index].u32HWOutPortMask = E_MI_SCL_HWSCL0|E_MI_SCL_HWSCL2;//E_MI_SCL_HWSCL0|E_MI_SCL_HWSCL1|E_MI_SCL_HWSCL2;
                buf_obj->face_sclid = gstSensorAttr[snr_index].u32SclDevId;
            }
            else
            {
                gstSensorAttr[snr_index].u32HWOutPortMask = E_MI_SCL_HWSCL0;//E_MI_SCL_HWSCL0|E_MI_SCL_HWSCL1;
            }
        }
        else
            gstSensorAttr[snr_index].u32HWOutPortMask = E_MI_SCL_HWSCL3;
	}
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

// int sstar_scl_enqueueOneBuffer(int dev, int chn, MI_SYS_DmaBufInfo_t* mi_dma_buf_info)
// {
//     MI_SYS_ChnPort_t chnPort;
//     // set chn port info
//     memset(&chnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
//     chnPort.eModId = E_MI_MODULE_ID_SCL;
//     chnPort.u32DevId = dev;
//     chnPort.u32ChnId = chn;
//     chnPort.u32PortId = 0;

//     // enqueue one dma buffer
//     if (MI_SUCCESS != MI_SYS_ChnOutputPortEnqueueDmabuf(&chnPort, mi_dma_buf_info)) {
//         printf("call MI_SYS_ChnOutputPortEnqueueDmabuf faild\n");
//         return -1;
//     }
//     return 0;
// }


