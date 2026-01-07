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

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>

#include "mi_common.h"
#include "mi_sensor.h"
#include "mi_sys.h"
#include "mi_vif.h"
#include "mi_isp.h"
#include "mi_isp_cus3a_api.h"
#include "mi_iqserver.h"
MI_U32 mIspDev = 0;
MI_U32 mIspChn = 0;
MI_ISP_PORT ispOutPortId = 1;

//snr pad 0 parm
// MI_U32 mVifDev = 0;
// MI_U32 mVifChn = 0;
// MI_VIF_GROUP mVifGroup = 0;
// MI_VIF_PORT vifPortId = 0;
// MI_U32 snrPadId = 0;

//snr pad 1 parm
MI_U32 mVifDev = 8;
MI_U32 mVifChn = 0;
MI_VIF_GROUP mVifGroup = 2;
MI_VIF_PORT vifPortId = 0;
MI_U32 snrPadId = 1;

char *iqpath = NULL;

//snr pad 2 parm
// MI_U32 mVifDev = 4;
// MI_U32 mVifChn = 0;
// MI_VIF_GROUP mVifGroup = 1;
// MI_VIF_PORT vifPortId = 0;
// MI_U32 snrPadId = 2;

static bool bExit = FALSE;

void handleSignal(MI_S32 signo) {
    if (signo == SIGINT) {
        printf("catch Ctrl + C, exit normally\n");
        bExit = TRUE;
    }
}

typedef struct {
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
    MI_SYS_BindType_e eBindType;
    MI_U32 u32BindParam;
} Sys_BindInfo_T;

#ifndef STCHECKRESULT
#define STCHECKRESULT(_func_)                                               \
    do {                                                                    \
        MI_S32 s32Ret = MI_SUCCESS;                                         \
        s32Ret = _func_;                                                    \
        if (s32Ret != MI_SUCCESS) {                                         \
            printf("%s exec function failed, error:%x\n", #_func_, s32Ret); \
            return -1;                                                      \
        } else {                                                            \
            printf("exec function pass\n");                                 \
        }                                                                   \
    } while (0)
#endif

MI_S32 miSysInit() {
    MI_SYS_Version_t stVersion;
    MI_U64 u64Pts = 0;
    STCHECKRESULT(MI_SYS_Init(0));
    memset(&stVersion, 0x0, sizeof(MI_SYS_Version_t));
    STCHECKRESULT(MI_SYS_GetVersion(0, &stVersion));
    printf("u8Version:%s\n", stVersion.u8Version);
    STCHECKRESULT(MI_SYS_GetCurPts(0, &u64Pts));
    printf("u64Pts:0x%llx\n", u64Pts);
    u64Pts = 0xF1237890F1237890;
    STCHECKRESULT(MI_SYS_InitPtsBase(0, u64Pts));
    u64Pts = 0xE1237890E1237890;
    STCHECKRESULT(MI_SYS_SyncPts(0, u64Pts));
    return MI_SUCCESS;
}

MI_S32 bind_port(Sys_BindInfo_T* pstBindInfo)
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


void* get_buffer_test(MI_SYS_ChnPort_t stChnPort) {
    fd_set read_fds;
    MI_S32 fd = -1;
    MI_S32 ret = -1;

    printf("start get buff\n");
    if (MI_SYS_GetFd(&stChnPort, &fd) < 0) {
        printf("get fd failed\n");
    }
    printf("fd = %d\n", fd);
    MI_SYS_BufInfo_t bufInfo;
    MI_SYS_BUF_HANDLE handle;
    int file_id = open("./test.yuv", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    while (!bExit) {
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        ret = select(fd + 1, &read_fds, NULL, NULL, NULL);
        if (ret < 0) {
            printf("select failed\n");
        } else if (0 == ret) {
            printf("select timeout\n");
        } else {
            if (FD_ISSET(fd, &read_fds)) {
                memset(&bufInfo, 0, sizeof(MI_SYS_BufInfo_t));
                memset(&handle, 0, sizeof(MI_SYS_BUF_HANDLE));
                ret = MI_SYS_ChnOutputPortGetBuf(&stChnPort, &bufInfo, &handle);
                if (ret == MI_SUCCESS) {
                    printf("MI_SYS_ChnOutputPortGetBuf success, mod:%d, dev:%d,chn:%d,port:%d\n",
                           stChnPort.eModId, stChnPort.u32DevId, stChnPort.u32ChnId,
                           stChnPort.u32PortId);
                    write(file_id, bufInfo.stFrameData.pVirAddr[0], bufInfo.stFrameData.u16Height * bufInfo.stFrameData.u32Stride[0] * 3 /2);
                    // write(file_id, bufInfo.stFrameData.pVirAddr[1], bufInfo.stFrameData.u16Height * bufInfo.stFrameData.u32Stride[0]/4);
                    printf("bufInfo.stFrameData.u16Height == %d, bufInfo.stFrameData.u32Stride == %d\n", bufInfo.stFrameData.u16Height, bufInfo.stFrameData.u32Stride[0]);
                    MI_SYS_ChnOutputPortPutBuf(handle);
                }
            }
        }
    }
    return NULL;
}

MI_S32 deinitPipeline() {
    /************************************************
    Step1:  Deinit Vif
    *************************************************/
    STCHECKRESULT(MI_VIF_DisableOutputPort(mVifDev, vifPortId));
    STCHECKRESULT(MI_VIF_DisableDev(mVifDev));
    STCHECKRESULT(MI_VIF_DestroyDevGroup(mVifGroup));

    /************************************************
    Step2:  Deinit Sensor
    *************************************************/
    STCHECKRESULT(MI_SNR_Disable(snrPadId));

    return 0;
}

MI_S32 initPipeline() {
    MI_SNR_PADInfo_t stSnrPadInfo;
    MI_SNR_PlaneInfo_t stSnrPlaneInfo;
    MI_SNR_Res_t stSnrRes;
    MI_U32 u32ResCount = 0;
    MI_U32 choiceRes = 0;

    STCHECKRESULT(miSysInit());
    memset(&stSnrPadInfo, 0x0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlaneInfo, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stSnrRes, 0x0, sizeof(MI_SNR_Res_t));
    STCHECKRESULT(MI_SNR_SetPlaneMode(snrPadId, FALSE));
    STCHECKRESULT(MI_SNR_QueryResCount(snrPadId, &u32ResCount));
    for (MI_U32 u8ResIndex = 0; u8ResIndex < u32ResCount; u8ResIndex++) {
        STCHECKRESULT(MI_SNR_GetRes(snrPadId, u8ResIndex, &stSnrRes));
        printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc "
               "%s\n",
               u8ResIndex, stSnrRes.stCropRect.u16X, stSnrRes.stCropRect.u16Y,
               stSnrRes.stCropRect.u16Width, stSnrRes.stCropRect.u16Height,
               stSnrRes.stOutputSize.u16Width, stSnrRes.stOutputSize.u16Height, stSnrRes.u32MaxFps,
               stSnrRes.u32MinFps, stSnrRes.strResDesc);
    }
    STCHECKRESULT(MI_SNR_GetRes(snrPadId, choiceRes, &stSnrRes));
    STCHECKRESULT(MI_SNR_SetRes(snrPadId, choiceRes));
    STCHECKRESULT(MI_SNR_Enable(snrPadId));
    printf("Snr pad id:%d, res count:%d, You select %d res\n", (int)snrPadId, u32ResCount,
           choiceRes);
    /************************************************
    Step1:  Init Vif
    *************************************************/
    MI_SYS_ChnPort_t stChnPort;
    MI_VIF_GroupAttr_t stVifGroupAttr;
    MI_VIF_DevAttr_t stVifDevAttr;
    MI_VIF_OutputPortAttr_t stVifPortAttr;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stVifGroupAttr, 0x0, sizeof(MI_VIF_GroupAttr_t));
    memset(&stVifDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));
    memset(&stVifPortAttr, 0x0, sizeof(MI_VIF_OutputPortAttr_t));
    STCHECKRESULT(MI_SNR_GetPadInfo(snrPadId, &stSnrPadInfo));
    STCHECKRESULT(MI_SNR_GetPlaneInfo(snrPadId, 0, &stSnrPlaneInfo));
    stVifGroupAttr.eIntfMode = E_MI_VIF_MODE_MIPI;
    stVifGroupAttr.eWorkMode = E_MI_VIF_WORK_MODE_1MULTIPLEX;
    stVifGroupAttr.eHDRType = E_MI_VIF_HDR_TYPE_OFF;
    if (stVifGroupAttr.eIntfMode == E_MI_VIF_MODE_BT656)
        stVifGroupAttr.eClkEdge = (MI_VIF_ClkEdge_e)stSnrPadInfo.unIntfAttr.stBt656Attr.eClkEdge;
    else
        stVifGroupAttr.eClkEdge = E_MI_VIF_CLK_EDGE_DOUBLE;
    STCHECKRESULT(MI_VIF_CreateDevGroup(mVifGroup, &stVifGroupAttr));

    stVifDevAttr.stInputRect.u16X = stSnrPlaneInfo.stCapRect.u16X;
    stVifDevAttr.stInputRect.u16Y = stSnrPlaneInfo.stCapRect.u16Y;
    stVifDevAttr.stInputRect.u16Width = stSnrPlaneInfo.stCapRect.u16Width;
    stVifDevAttr.stInputRect.u16Height = stSnrPlaneInfo.stCapRect.u16Height;
    if (stSnrPlaneInfo.eBayerId >= E_MI_SYS_PIXEL_BAYERID_MAX)
        stVifDevAttr.eInputPixel = stSnrPlaneInfo.ePixel;
    else
        stVifDevAttr.eInputPixel = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(
                stSnrPlaneInfo.ePixPrecision, stSnrPlaneInfo.eBayerId);
    STCHECKRESULT(MI_VIF_SetDevAttr(mVifDev, &stVifDevAttr));
    STCHECKRESULT(MI_VIF_EnableDev(mVifDev));
    stVifPortAttr.stCapRect.u16X = stSnrPlaneInfo.stCapRect.u16X;
    stVifPortAttr.stCapRect.u16Y = stSnrPlaneInfo.stCapRect.u16Y;
    stVifPortAttr.stCapRect.u16Width = stSnrPlaneInfo.stCapRect.u16Width;
    stVifPortAttr.stCapRect.u16Height = stSnrPlaneInfo.stCapRect.u16Height;
    stVifPortAttr.stDestSize.u16Width = stSnrPlaneInfo.stCapRect.u16Width;
    stVifPortAttr.stDestSize.u16Height = stSnrPlaneInfo.stCapRect.u16Height;
    stVifPortAttr.eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    if (stSnrPlaneInfo.eBayerId >= E_MI_SYS_PIXEL_BAYERID_MAX)
        stVifPortAttr.ePixFormat = stSnrPlaneInfo.ePixel;
    else
        stVifPortAttr.ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(
                stSnrPlaneInfo.ePixPrecision, stSnrPlaneInfo.eBayerId);
    STCHECKRESULT(MI_VIF_SetOutputPortAttr(mVifDev, vifPortId, &stVifPortAttr));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = mVifDev;
    stChnPort.u32ChnId = mVifChn;
    stChnPort.u32PortId = vifPortId;
    STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(0, &stChnPort, 2, 4));
    STCHECKRESULT(MI_VIF_EnableOutputPort(mVifDev, vifPortId));
    printf("vif enabled, dev: %d, chn:%d, port:%d\n", (int)mVifDev, (int)mVifChn, vifPortId);
    printf("crop w = %d, crop h =%d\nsnr w = %d, snr h = %d\n", stVifPortAttr.stCapRect.u16Width, stVifPortAttr.stCapRect.u16Height,
    stSnrRes.stOutputSize.u16Width, stSnrRes.stOutputSize.u16Height);
    // get_buffer_test(stChnPort);

    /************************************************
    Step2:  Init Isp
    *************************************************/

    MI_ISP_DevAttr_t stIspDevAttr;
    MI_ISP_ChannelAttr_t stIspChnAttr;
    MI_ISP_ChnParam_t stIspChnParam;
    MI_ISP_OutPortParam_t stIspOutputParam;
    memset(&stIspDevAttr, 0x0, sizeof(MI_ISP_DevAttr_t));
    memset(&stIspChnAttr, 0x0, sizeof(MI_ISP_ChannelAttr_t));
    memset(&stIspChnParam, 0x0, sizeof(MI_ISP_ChnParam_t));
    memset(&stIspOutputParam, 0x0, sizeof(MI_ISP_OutPortParam_t));
    stIspDevAttr.u32DevStitchMask = E_MI_ISP_DEVICEMASK_ID0;
    STCHECKRESULT(MI_ISP_CreateDevice(mIspDev, &stIspDevAttr));

    switch (snrPadId) {
        case 0:
            stIspChnAttr.u32SensorBindId = E_MI_ISP_SENSOR0;
            break;
        case 1:
            stIspChnAttr.u32SensorBindId = E_MI_ISP_SENSOR1;
            break;
        case 2:
            stIspChnAttr.u32SensorBindId = E_MI_ISP_SENSOR2;
            break;
        case 3:
            stIspChnAttr.u32SensorBindId = E_MI_ISP_SENSOR3;
            break;
        case 4:
            stIspChnAttr.u32SensorBindId = E_MI_ISP_SENSOR4;
            break;
        default:
            printf("Invalid Snr pad id:%d\n", (int)snrPadId);
            return -1;
    }
    stIspChnParam.eHDRType = E_MI_ISP_HDR_TYPE_OFF;
    stIspChnParam.e3DNRLevel = E_MI_ISP_3DNR_LEVEL1;
    stIspChnParam.bMirror = FALSE;
    stIspChnParam.bFlip = FALSE;
    stIspChnParam.eRot = 0;
    STCHECKRESULT(MI_ISP_CreateChannel(mIspDev, mIspChn, &stIspChnAttr));
    STCHECKRESULT(MI_ISP_SetChnParam(mIspDev, mIspChn, &stIspChnParam));
    STCHECKRESULT(MI_ISP_StartChannel(mIspDev, mIspChn));
    printf("isp chn enabled, dev: %d, chn:%d\n", (int)mIspDev, (int)mIspChn);
    stIspOutputParam.stCropRect.u16X =      stSnrPlaneInfo.stCapRect.u16X;
    stIspOutputParam.stCropRect.u16Y =      stSnrPlaneInfo.stCapRect.u16Y;
    stIspOutputParam.stCropRect.u16Width =  stSnrPlaneInfo.stCapRect.u16Width;
    stIspOutputParam.stCropRect.u16Height = stSnrPlaneInfo.stCapRect.u16Height;
    stIspOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    MI_ISP_SetOutputPortParam(mIspDev, mIspChn, ispOutPortId, &stIspOutputParam);
    if (MI_ISP_EnableOutputPort(mIspDev, mIspChn, ispOutPortId)) {
        printf("MI_ISP_EnableOutputPort error \n");
        return -1;
    }
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_ISP;
    stChnPort.u32DevId = mIspDev;
    stChnPort.u32ChnId = mIspChn;
    stChnPort.u32PortId = ispOutPortId;
    if (MI_SYS_SetChnOutputPortDepth(0, &stChnPort, 2, 4)) {
        printf("MI_SYS_SetChnOutputPortDepth error \n");
        return -1;
    }

    /************************************************
    Step3:  Bind Vif and Isp
    *************************************************/
    Sys_BindInfo_T  stBindInfo;
    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = mVifDev;
    stBindInfo.stSrcChnPort.u32ChnId = mVifChn;
    stBindInfo.stSrcChnPort.u32PortId = vifPortId;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_ISP;
    stBindInfo.stDstChnPort.u32DevId = mIspDev;
    stBindInfo.stDstChnPort.u32ChnId = mIspChn;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    STCHECKRESULT(bind_port(&stBindInfo));
    printf("bind VIF(%d %d %d) -> ISP(%d %d), bind type:%d\n", (int)mVifDev, (int)mVifChn,
           (int)mIspDev, (int)mIspDev, (int)mIspChn, (int)stBindInfo.eBindType);

    MI_IQSERVER_Open();
    if(iqpath != NULL)
    {
        MI_ISP_ApiCmdLoadBinFile(mIspDev, mIspChn, iqpath, 0);
        printf("MI_ISP_ApiCmdLoadBinFile Isp:%d Chn:%d path:%s \n", mIspDev, mIspChn, iqpath);
    }
    get_buffer_test(stChnPort);
    return 0;
}

int parse_args(int argc, char **argv)
{
    int option_index=0;
    MI_S32 s32Opt = 0;
    struct option long_options[] = {
            {"iqbin0", required_argument, NULL, 'i'},
            {"sensorId", required_argument, NULL, 's'},
            {"help", no_argument, NULL, 'h'},
            {0, 0, 0, 0}
    };

    while ((s32Opt = getopt_long(argc, argv, "i:s:h::",long_options, &option_index))!= -1 )
    {
        switch(s32Opt)
        {
            case 'i':
            {
                iqpath = (char*)malloc(128);
                strcpy(iqpath, optarg);
                break;
            }
            case 's'://single sensor chose eSnrPadId
            {
                switch(atoi(optarg))
                {
                    case 0:
                        mVifDev = 0;
                        mVifChn = 0;
                        mVifGroup = 0;
                        vifPortId = 0;
                        snrPadId = 0;
                    break;
                    case 1:
                        mVifDev = 8;
                        mVifChn = 0;
                        mVifGroup = 2;
                        vifPortId = 0;
                        snrPadId = 1;
                    break;
                    case 2:
                        mVifDev = 4;
                        mVifChn = 0;
                        mVifGroup = 1;
                        vifPortId = 0;
                        snrPadId = 2;
                    break;
                    case 3:
                        mVifDev = 12;
                        mVifChn = 0;
                        mVifGroup = 3;
                        vifPortId = 0;
                        snrPadId = 3;
                    break;
                    default:
                        mVifDev = 0;
                        mVifChn = 0;
                        mVifGroup = 0;
                        vifPortId = 0;
                        snrPadId = 0;
                    break;
                }
				break;
            }
            case 'h':
            default:
            {
                return -1;
                break;
            }
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    if(parse_args(argc, argv) != 0)
    {
        return 0;
    }
    struct sigaction sigAction;
    sigAction.sa_handler = handleSignal;
    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_flags = 0;
    sigaction(SIGINT, &sigAction, NULL);


    initPipeline();
    while (!bExit) {
        usleep(5000);
    }
    deinitPipeline();
    return 0;
}