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
#include <stdio.h>

#include <string.h>

#include "sstar_sensor_demo.h"
#include "mi_isp_cus3a_api.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

static Sensor_Attr_t gstSensorAttr[MAX_SENSOR_NUM];
static MI_S32 g_IspFd = -1;
static fd_set g_read_fds;
static MI_SYS_ChnPort_t g_stIspOutputPort;

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

#if 0
static void ST_Flush(void)
{
	char c;
	while((c = getchar()) != '\n' && c != EOF);
}
#endif

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

static MI_S32 bind_port(Sys_BindInfo_T* pstBindInfo)
{
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

static MI_S32 unbind_port(Sys_BindInfo_T* pstBindInfo)
{
    MI_S32 ret;

    ret = MI_SYS_UnBindChnPort(0, &pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort);

    return ret;
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
	//MI_S32 s32Input =0;
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
#if 0
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
#endif
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
		stGroupAttr.eHDRType =	(MI_VIF_HDRType_e)stPad0Info.eHDRMode;
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

    if (MI_ISP_CreateDevice(ispDevId, &stCreateDevAttr)) {
        printf("MI_ISP_CreateDevice error \n");
        return -1;
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
		stIspChnParam.eHDRType = (MI_ISP_HDRType_e)stPad0Info.eHDRMode;
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

    MI_IQSERVER_Open();

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

    MI_IQSERVER_Close();
    MI_ISP_DestoryDevice(ispDevId);
    printf("MI_SCL_DestroyDevice ok \n");

    return 0;
}

static void init_sensor_attr(MI_SNR_PADID eSnrPadId)//MI_SNR_PADID eSnrPadId, int bRotate)
{
    MI_VIF_DEV vifDev;
    MI_VIF_GROUP vifGroupId;
    int snr_index = 0;

    if(eSnrPadId > MAX_SENSOR_NUM)
    {
        printf("init_sensor_attr fail,snr_index=%d \n",snr_index);
        return ;
    }

	gstSensorAttr[snr_index].bUsed = TRUE;
    gstSensorAttr[snr_index].eSensorPadID = (MI_VIF_SNRPad_e)eSnrPadId;
    printf("init_sensor_attr,gstSensorAttr[%d].eSensorPadID=%d \n", snr_index, eSnrPadId);
    gstSensorAttr[snr_index].isp_eRot = E_MI_SYS_ROTATE_NONE;

	get_vif_from_snrpad(gstSensorAttr[snr_index].eSensorPadID, &vifGroupId, &vifDev);
	gstSensorAttr[snr_index].u32VifDev = vifDev;
	gstSensorAttr[snr_index].u32VifGroupID = vifGroupId;
	gstSensorAttr[snr_index].u32vifOutPortId = 0; //outport0 can realtime to isp
	gstSensorAttr[snr_index].eVif2IspType = E_MI_SYS_BIND_TYPE_REALTIME;//sigle sensor,outport0 can realtime to isp

	gstSensorAttr[snr_index].u32IspDevId = 0;
    gstSensorAttr[snr_index].u32IspChnId = 0;
    gstSensorAttr[snr_index].u32IspOutPortId = 1;//outport0 can realtime only

}

int sstar_get_sensor_attr(int eSnrPadId, Sensor_Attr_t* sensorAttr)
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


static int sensor_select_init()
{
    Sensor_Attr_t sensorAttr;

    sstar_get_sensor_attr(0, &sensorAttr);

	memset(&g_stIspOutputPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    g_stIspOutputPort.eModId = E_MI_MODULE_ID_ISP;
    g_stIspOutputPort.u32DevId = sensorAttr.u32IspDevId;
    g_stIspOutputPort.u32ChnId = sensorAttr.u32IspChnId;
    g_stIspOutputPort.u32PortId = sensorAttr.u32IspOutPortId;
	MI_SYS_SetChnOutputPortDepth(0, &g_stIspOutputPort , 2, 4);
    if(MI_SYS_GetFd(&g_stIspOutputPort, &g_IspFd) < 0)
    {
        printf("MI_SYS GET Scl ch: %d fd err.\n", g_stIspOutputPort.u32ChnId);
        return -1;
    }
    FD_ZERO(&g_read_fds);
    FD_SET(g_IspFd, &g_read_fds);
    return 0;
}


int sstar_get_snr_frame(MI_SYS_FrameData_t *stFrameData, MI_SYS_BUF_HANDLE *stBufHandle)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE tmpBufHandle;
    struct timeval tv;

    MI_S32 s32Ret = 0;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    s32Ret = select(g_IspFd + 1, &g_read_fds, NULL, NULL, &tv);
    if(s32Ret <= 0)
    {
        printf("select fail,ret:%x\n",s32Ret);
        return -1;
    }
    else
    {
        if(FD_ISSET(g_IspFd, &g_read_fds))
        {
            memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
            s32Ret = MI_SYS_ChnOutputPortGetBuf(&g_stIspOutputPort, &stBufInfo, &tmpBufHandle);
            if(MI_SUCCESS != s32Ret || E_MI_SYS_BUFDATA_FRAME != stBufInfo.eBufType)
            {
                printf("get isp buffer fail,ret:%x\n",s32Ret);
                MI_SYS_ChnOutputPortPutBuf(tmpBufHandle);
                return -1;
            }
            *stBufHandle = tmpBufHandle;
            memcpy(stFrameData, &stBufInfo.stFrameData, sizeof(MI_SYS_FrameData_t));
        }
    }
    return 0;
}

int sstar_put_snr_frame(MI_SYS_BUF_HANDLE stBufHandle)
{
    MI_SYS_ChnOutputPortPutBuf(stBufHandle);
    return 0;
}

/************************************************************
Init sensor module.for example,sensor, vif, isp, etc.
************************************************************/
int sstar_SnrPipeLine_Creat(MI_SNR_PADID eSnrPadId, char * iq_path) {
    int ret = 0;
	int Hdr = 0;
    printf("create sensor pipeline ,iq_path=%s \n",iq_path);
    init_sensor_attr(eSnrPadId);
    ret = sstar_init_snr(eSnrPadId, Hdr) || sstar_init_vif(eSnrPadId, Hdr) ||
          sstar_init_isp(eSnrPadId, iq_path, Hdr);
    if (ret != 0) {
        printf("create sensor pipeline fail,sensorIdx=%d \n",eSnrPadId);
        return ret;
    }

    sensor_select_init();


    return 0;
}

int sstar_SnrPipeLine_Destory(MI_SNR_PADID eSnrPadId)
{
    int ret = 0;

    ret = sstar_deinit_isp(eSnrPadId) || sstar_deinit_vif(eSnrPadId)
          || sstar_deinit_snr(eSnrPadId);
    if (ret != 0) {
        printf("destroy sensor pipeline fail \n");
        return ret;
    }

    return 0;
}

