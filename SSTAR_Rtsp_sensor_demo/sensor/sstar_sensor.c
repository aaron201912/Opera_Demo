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
// #include "mi_venc_datatype.h"
// #include "mi_venc.h"

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

// static MI_S32 bind_port(Sys_BindInfo_T* pstBindInfo) {
//     MI_S32 ret;

//     ret = MI_SYS_BindChnPort2(0, &pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort,
//                               pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate,
//                               pstBindInfo->eBindType, pstBindInfo->u32BindParam);
//     printf("src(%d-%d-%d-%d)  dst(%d-%d-%d-%d)  %d...\n", pstBindInfo->stSrcChnPort.eModId,
//            pstBindInfo->stSrcChnPort.u32DevId, pstBindInfo->stSrcChnPort.u32ChnId,
//            pstBindInfo->stSrcChnPort.u32PortId, pstBindInfo->stDstChnPort.eModId,
//            pstBindInfo->stDstChnPort.u32DevId, pstBindInfo->stDstChnPort.u32ChnId,
//            pstBindInfo->stDstChnPort.u32PortId, pstBindInfo->eBindType);

//     return ret;
// }

// static MI_S32 unbind_port(Sys_BindInfo_T* pstBindInfo) {
//     MI_S32 ret;

//     ret = MI_SYS_UnBindChnPort(0, &pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort);

//     return ret;
// }

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
VENC INIT AND DEINT FOR SENSOR
***************************************************************/
static int sstar_venc_init_for_sensor(buffer_object_t *buf_obj)
{
    /************************************************
    Step8:  venc init
    *************************************************/
    sstar_venc_info_t venc_info;

    venc_info.venc_dev_id = 0;
    venc_info.venc_chn_id = buf_obj->vencChn;
    venc_info.venc_outport = 0;
    venc_info.venc_out_width = ALIGN_BACK(buf_obj->vdec_info.v_out_width, ALIGN_NUM);
    venc_info.venc_out_height = ALIGN_BACK(buf_obj->vdec_info.v_out_height, ALIGN_NUM);
    venc_info.venc_src_chn_port.eModId = buf_obj->chn_port_info.eModId;
    venc_info.venc_src_chn_port.u32DevId = buf_obj->chn_port_info.u32DevId;
    venc_info.venc_src_chn_port.u32ChnId = buf_obj->chn_port_info.u32ChnId;
    venc_info.venc_src_chn_port.u32PortId = buf_obj->chn_port_info.u32PortId;
    venc_info.venc_src_fps = 60;
    venc_info.venc_dst_fps = 30;
    venc_info.venc_bind_type = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    ExecFunc(sstar_venc_init(&venc_info), MI_SUCCESS);

    return 0;

}

static int sstar_venc_deinit_for_sensor(buffer_object_t *buf_obj)
{
    sstar_venc_info_t venc_info;

    venc_info.venc_dev_id = 0;
    venc_info.venc_chn_id = buf_obj->vencChn;
    venc_info.venc_outport = 0;
    venc_info.venc_src_chn_port.eModId = buf_obj->chn_port_info.eModId;
    venc_info.venc_src_chn_port.u32DevId = buf_obj->chn_port_info.u32DevId;
    venc_info.venc_src_chn_port.u32ChnId = buf_obj->chn_port_info.u32ChnId;
    venc_info.venc_src_chn_port.u32PortId = buf_obj->chn_port_info.u32PortId;
    venc_info.venc_src_fps = 60;
    venc_info.venc_dst_fps = 30;
    venc_info.venc_bind_type = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    ExecFunc(sstar_venc_deinit(&venc_info), MI_SUCCESS);

    return 0;
}
/***************************************************************
SCL INIT AND DEINT FOR SENSOR
***************************************************************/
static MI_S32 sstar_scl_init_for_sensor(buffer_object_t *buf_obj) 
{
    MI_S32 ret;
    sstar_scl_info_t scl_info;
    MI_SCL_OutPortParam_t stSclOutputParam;
    int snr_index;
    snr_index = get_sensorgst_with_id(buf_obj->sensorIdx);
    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error \n");
        return -1;
    }
    scl_info.scl_dev_id = gstSensorAttr[snr_index].u32SclDevId;
    scl_info.scl_chn_id = gstSensorAttr[snr_index].u32SclChnId;
    scl_info.scl_outport = gstSensorAttr[snr_index].u32SclOutPortId;
    scl_info.scl_hw_outport_mask = gstSensorAttr[snr_index].u32HWOutPortMask;
    scl_info.scl_rotate = E_MI_SYS_ROTATE_NONE;
	if ((buf_obj->scl_rotate == E_MI_SYS_ROTATE_90) || (buf_obj->scl_rotate == E_MI_SYS_ROTATE_270))
    {    
        scl_info.scl_out_width = buf_obj->vdec_info.v_out_height;
        scl_info.scl_out_height = buf_obj->vdec_info.v_out_width;
    }else
    {
        scl_info.scl_out_width = buf_obj->vdec_info.v_out_width;
        scl_info.scl_out_height = buf_obj->vdec_info.v_out_height;
    }
    scl_info.scl_src_chn_port.eModId = E_MI_MODULE_ID_ISP;
    scl_info.scl_src_chn_port.u32DevId = gstSensorAttr[snr_index].u32IspDevId;
    scl_info.scl_src_chn_port.u32ChnId = gstSensorAttr[snr_index].u32IspChnId;
    scl_info.scl_src_chn_port.u32PortId = gstSensorAttr[snr_index].u32IspOutPortId;
    scl_info.scl_src_module_inited = 1;
    scl_info.scl_src_fps = 60;
    scl_info.scl_dst_fps = 60;
    scl_info.scl_bind_type = gstSensorAttr[snr_index].eIsp2SclType;
    ExecFunc(sstar_scl_init(&scl_info), MI_SUCCESS);

    if(buf_obj->face_detect)
    {
    	/************************************************
        Step5:  Init Scl for Algo with face detect
        *************************************************/
    	scl_info.scl_outport = 1;
    	memset(&stSclOutputParam, 0x0, sizeof(MI_SCL_OutPortParam_t));
    	stSclOutputParam.stSCLOutputSize.u16Width = 800;//g_stAlgoRes.width;
    	stSclOutputParam.stSCLOutputSize.u16Height = 480;//g_stAlgoRes.height;
    	stSclOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;//g_eAlgoFormat;
    	stSclOutputParam.bMirror = 0;
    	stSclOutputParam.bFlip = 0;
    	ExecFunc(MI_SCL_SetOutputPortParam(scl_info.scl_dev_id, scl_info.scl_chn_id, scl_info.scl_outport, &stSclOutputParam), MI_SUCCESS);
        printf("MI_SCL_EnableOutputPort,SclDevId=%d SclChnId=%d SclOutPortId=%d \n", scl_info.scl_dev_id, scl_info.scl_chn_id, scl_info.scl_outport);
    	ExecFunc(MI_SCL_EnableOutputPort(scl_info.scl_dev_id, scl_info.scl_chn_id, scl_info.scl_outport), MI_SUCCESS);

    }
    #if 1
    if (buf_obj->scl_rotate > E_MI_SYS_ROTATE_NONE && snr_index == 0)
    {
        /************************************************
        Step6:  scl rotate
        *************************************************/
        scl_info.scl_dev_id = 7;
        scl_info.scl_chn_id = 0;
        scl_info.scl_outport = 0;
        scl_info.scl_hw_outport_mask = E_MI_SCL_HWSCL1;
        scl_info.scl_rotate = gstSensorAttr[snr_index].scl_eRot;
        scl_info.scl_out_width = buf_obj->vdec_info.v_out_width;
        scl_info.scl_out_height = buf_obj->vdec_info.v_out_height;
        scl_info.scl_src_chn_port.eModId = E_MI_MODULE_ID_SCL;
        scl_info.scl_src_chn_port.u32DevId = gstSensorAttr[snr_index].u32SclDevId;
        scl_info.scl_src_chn_port.u32ChnId = gstSensorAttr[snr_index].u32SclChnId;
        scl_info.scl_src_chn_port.u32PortId = gstSensorAttr[snr_index].u32SclOutPortId;
        scl_info.scl_src_module_inited = 1;
        scl_info.scl_src_fps = 60;
        scl_info.scl_dst_fps = 60;
        scl_info.scl_bind_type = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        ExecFunc(sstar_scl_init(&scl_info), MI_SUCCESS);        
    }
    #endif
    printf("MI Scl Init done \n");
    return 0;
}

static MI_S32 sstar_scl_deinit_for_sensor(buffer_object_t *buf_obj) {

    sstar_scl_info_t scl_info;
    Sys_BindInfo_T stBindInfo;

    int snr_index;
    snr_index = get_sensorgst_with_id(buf_obj->sensorIdx);
    if(snr_index == -1)
    {
        printf("get_sensorgst_with_id error \n");
        return -1;
    }
    #if 1
    if(buf_obj->scl_rotate > E_MI_SYS_ROTATE_NONE && snr_index == 0)
    {
        scl_info.scl_dev_id = 7;
        scl_info.scl_chn_id = 0;
        scl_info.scl_outport = 0;
        scl_info.scl_src_chn_port.eModId = E_MI_MODULE_ID_SCL;
        scl_info.scl_src_module_inited = 1;
        scl_info.scl_src_chn_port.u32DevId = gstSensorAttr[snr_index].u32SclDevId;
        scl_info.scl_src_chn_port.u32ChnId = gstSensorAttr[snr_index].u32SclChnId;
        scl_info.scl_src_chn_port.u32PortId = gstSensorAttr[snr_index].u32SclOutPortId;
        scl_info.scl_src_fps = 60;
        scl_info.scl_dst_fps = 60;
        scl_info.scl_bind_type = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        ExecFunc(sstar_scl_deinit(&scl_info), MI_SUCCESS);
    }
    #endif

    scl_info.scl_dev_id = gstSensorAttr[snr_index].u32SclDevId;
    scl_info.scl_chn_id = gstSensorAttr[snr_index].u32SclChnId;
    scl_info.scl_src_chn_port.eModId = E_MI_MODULE_ID_ISP;
    scl_info.scl_src_module_inited = 1; 
    scl_info.scl_src_chn_port.u32DevId = gstSensorAttr[snr_index].u32IspDevId;
    scl_info.scl_src_chn_port.u32ChnId = gstSensorAttr[snr_index].u32IspChnId;
    scl_info.scl_src_chn_port.u32PortId = gstSensorAttr[snr_index].u32IspOutPortId;
    scl_info.scl_src_fps = 60;
    scl_info.scl_dst_fps = 60;
    scl_info.scl_bind_type =  gstSensorAttr[snr_index].eIsp2SclType;

    if(buf_obj->face_detect)
    {
       scl_info.scl_outport = 1;
        if (MI_SCL_DisableOutputPort(scl_info.scl_dev_id, scl_info.scl_chn_id, scl_info.scl_outport)) {
            printf("MI_SCL_DisableOutputPort error,SclChnId=%d SclOutPortId=%d \n", scl_info.scl_chn_id, scl_info.scl_outport);
            return -1;
        }
    }
    scl_info.scl_outport = gstSensorAttr[snr_index].u32SclOutPortId;
    ExecFunc(sstar_scl_deinit(&scl_info), MI_SUCCESS);
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
          sstar_init_isp(sensorIdx, buf_obj->iq_file, Hdr) || sstar_scl_init_for_sensor(buf_obj);
    if (ret != 0) {
        printf("create sensor pipeline fail,sensorIdx=%d \n",sensorIdx);
        return ret;
    }
    if(buf_obj->venc_flag != 0)
    {
        sstar_venc_init_for_sensor(buf_obj);
    }
    return 0;
}

int destroy_snr_pipeline(buffer_object_t *buf_obj)
{
    int ret = 0;
    int sensorIdx = buf_obj->sensorIdx;

    if(buf_obj->venc_flag != 0)
    {
        sstar_venc_deinit_for_sensor(buf_obj);
    }
    ret = sstar_scl_deinit_for_sensor(buf_obj) || sstar_deinit_isp(sensorIdx) ||
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


