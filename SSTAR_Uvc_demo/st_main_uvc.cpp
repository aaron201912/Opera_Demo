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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <poll.h>
#include <cerrno>

#include "common.h"
//#include "st_rgn.h"
#include "st_uvc.h"

#include "mi_sensor.h"
#include "mi_vif.h"
#include "mi_isp.h"
#ifdef ISP_ALGO_ENABLE
#include "mi_isp_iq.h"
#include "mi_isp_ae.h"
#include "mi_isp_cus3a_api.h"
#include "mi_iqserver.h"
#endif
#include "mi_scl.h"
#include "mi_venc.h"
#ifdef HDMI_ENABLE
#include "st_hdmi.h"
#include "mi_disp.h"
#define SCL_HDMI_DEV_ID 3
typedef enum
{
    ST_DISP_LAYER0 = 0,
    ST_DISP_LAYER1,         // for PIP
    ST_DISP_LAYER2,         // for CVBS
    ST_DISP_MAX
} ST_DispLayerId_e;
typedef struct ST_DispInfo_s
{
    /* Vo Layer attr*/
    MI_DISP_DEV DispDev;
    MI_DISP_Interface_e eIntfType;
    MI_DISP_LAYER DispLayer;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
    MI_DISP_OutputTiming_e eTiming;

    /* Others */
    MI_U32 u32Toleration;
} ST_DispDevInfo_t;

typedef struct ST_DispChnAttr_s
{
    MI_U32 u32Port;
    MI_DISP_InputPortAttr_t stAttr;
} ST_DispChnAttr_t;

typedef struct ST_DispChnInfo_s
{
    /* Vo Chn attr */
    MI_DISP_INPUTPORT InputPortNum;
    ST_DispChnAttr_t stInputPortAttr[16];
} ST_DispChnInfo_t;

typedef struct ST_DataInfo_s
{
    MI_U16 u16PicWidth;
    MI_U16 u16PicHeight;
    MI_U16 u16Times;
    MI_U8 au8FileName[128];
} ST_DataInfo_t;
MI_U32 g_hdmiDispDev = 1;
MI_U32 g_hdmiDispLayer = ST_DISP_LAYER2;
#endif
//#include "linux_list.h"

#define ISP_DEV_ID                0
#define ISP_CHN_ID                0
#define ISP_PORT_ID_RT            0
#define ISP_PORT_ID               1
#define SCL_DEV_ID_RT             0
#define SCL_DEV_ID                1
#define SCL_PORT_ID               0

#define USB_CAMERA0_INDEX          0
#define USB_CAMERA1_INDEX          1
#define USB_CAMERA2_INDEX          2
#define USB_CAMERA3_INDEX          3
#define USB_CAMERA4_INDEX          4
#define USB_CAMERA5_INDEX          5
#define UVC_STREAM0                "uvc_stream0"
#define UVC_STREAM1                "uvc_stream1"
#define UVC_STREAM2                "uvc_stream2"
#define UVC_STREAM3                "uvc_stream3"
#define UVC_STREAM4                "uvc_stream4"
#define UVC_STREAM5                "uvc_stream5"
#define MAX_UVC_DEV_NUM             6
#define PATH_PREFIX                "/mnt"
#define DEBUG_ES_FILE            0

#define MAX_CAPTURE_NUM            4
#define CAPTURE_PATH            "/mnt/capture"

#define USE_TEST    0
#define USE_STREAM_FILE 0

#define IQ_FILE_PATH    "/customer/iq_api.bin"
#define IQ_HDR_FILE_PATH    "/customer/hdr.bin"

#ifdef ALIGN_UP
#undef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#else
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(val, alignment) (((val)/(alignment))*(alignment))
#endif

struct ST_Stream_Attr_T {
    MI_U32     u32IspDev;
    MI_U32     u32IspChn;
    MI_U32     u32IspPort;
    MI_U32     u32SclDev;
    MI_U32     u32SclChn;
    MI_U32     u32SclPort;
    MI_U32     u32VencChn;

    const char    *pszStreamName;
    MI_BOOL bForceIdr;
};

typedef struct {
    MI_S32 s32HDRtype;
    MI_SYS_Rotate_e enRotation;
    MI_ISP_3DNR_Level_e en3dNrLevel;
    MI_U8 u8SnrPad;
    MI_S8 s8SnrResIndex;
    MI_U32 u32MaxFps;
} ST_Config_S;

typedef struct {
    MI_U32  fcc;
    MI_U32  u32Width;
    MI_U32  u32Height;
    MI_U32  u32FrameRate;
    MI_SYS_ChnPort_t dstChnPort;
} ST_UvcSetting_Attr_T;

typedef struct VENC_STREAMS_s {
    bool used;
    MI_VENC_Stream_t stStream;
} VENC_STREAMS_t;

typedef struct {
    VENC_STREAMS_t *pstuserptr_stream;
} ST_Uvc_Resource_t;

typedef struct {
    char name[20];
    int dev_index;
    ST_UVC_Handle_h handle;
    ST_UvcSetting_Attr_T setting;
    ST_Uvc_Resource_t res;
} ST_UvcDev_t;

typedef struct {
    int devnum;
    ST_UvcDev_t dev[];
} ST_UvcSrc_t;

#define MAX_FRAME 10000
typedef struct
{
    MI_S32  imgFd;
    char    filePath[64];
    MI_U32 *frameArr;
    MI_U32  arrLen;
    MI_U32  cur_offset;
    MI_U32  cur_frame;
} ST_UvcFile_t;
typedef struct
{
    MI_S32  wavFd;
    char    filePath[64];
} ST_UacFile_t;

static MI_BOOL g_bEnableVideo = TRUE;
static MI_BOOL g_bExit = FALSE;
static ST_Config_S g_stConfig;
static ST_UvcSrc_t * g_UvcSrc;
static MI_U8 g_bitrate[MAX_UVC_DEV_NUM] = {0, 0, 0, 0, 0, 0};
static MI_U8 g_qfactor[MAX_UVC_DEV_NUM] = {0, 0, 0, 0, 0, 0};
static MI_U8 g_maxbuf_cnt = 3;
static MI_U8 g_enable_iqserver = 0;
static MI_U8 g_load_iq_bin = 0;
static MI_U32 g_device_num = 1;
static char g_IspBinPath[64] = {0};
static MI_BOOL g_bStartCapture = FALSE;
static MI_BOOL g_bEnableHDMI = FALSE;
static MI_BOOL g_bEnableFile = FALSE;
static ST_UacFile_t g_stUacFile[2] =
{
    [0] = {-1, {0}},
    [1] = {-1, {0}},
};
static ST_UvcFile_t g_stUvcFile[MAX_UVC_DEV_NUM] =
{
    [USB_CAMERA0_INDEX] = {-1, {0}, NULL, MAX_FRAME, 0, 1},
    [USB_CAMERA1_INDEX] = {-1, {0}, NULL, MAX_FRAME, 0, 1},
    [USB_CAMERA2_INDEX] = {-1, {0}, NULL, MAX_FRAME, 0, 1},
    [USB_CAMERA3_INDEX] = {-1, {0}, NULL, MAX_FRAME, 0, 1},
    [USB_CAMERA4_INDEX] = {-1, {0}, NULL, MAX_FRAME, 0, 1},
    [USB_CAMERA5_INDEX] = {-1, {0}, NULL, MAX_FRAME, 0, 1},
};
static struct ST_Stream_Attr_T g_stStreamAttr[] =
{
    [USB_CAMERA0_INDEX] =
    {
        .u32IspDev = ISP_DEV_ID,
        .u32IspChn = ISP_CHN_ID,
        .u32IspPort = ISP_PORT_ID,
        .u32SclDev = SCL_DEV_ID,
        .u32SclChn = 0,
        .u32SclPort = SCL_PORT_ID,
        .u32VencChn = 0,

        .pszStreamName = UVC_STREAM0,
        .bForceIdr = FALSE,
    },

    [USB_CAMERA1_INDEX] =
    {
        .u32IspDev = ISP_DEV_ID,
        .u32IspChn = ISP_CHN_ID,
        .u32IspPort = ISP_PORT_ID,
        .u32SclDev = SCL_DEV_ID,
        .u32SclChn = 1,
        .u32SclPort = SCL_PORT_ID,
        .u32VencChn = 1,

        .pszStreamName = UVC_STREAM1,
        .bForceIdr = FALSE,
    },

    [USB_CAMERA2_INDEX] =
    {
        .u32IspDev = ISP_DEV_ID,
        .u32IspChn = ISP_CHN_ID,
        .u32IspPort = ISP_PORT_ID,
        .u32SclDev = SCL_DEV_ID,
        .u32SclChn = 2,
        .u32SclPort = SCL_PORT_ID,
        .u32VencChn = 2,

        .pszStreamName = UVC_STREAM2,
        .bForceIdr = FALSE,
    },

    [USB_CAMERA3_INDEX] =
    {
        .u32IspDev = ISP_DEV_ID,
        .u32IspChn = ISP_CHN_ID,
        .u32IspPort = ISP_PORT_ID,
        .u32SclDev = SCL_DEV_ID,
        .u32SclChn = 3,
        .u32SclPort = SCL_PORT_ID,
        .u32VencChn = 3,

        .pszStreamName = UVC_STREAM3,
        .bForceIdr = FALSE,
    },

    [USB_CAMERA4_INDEX] =
    {
        .u32IspDev = ISP_DEV_ID,
        .u32IspChn = ISP_CHN_ID,
        .u32IspPort = ISP_PORT_ID,
        .u32SclDev = SCL_DEV_ID,
        .u32SclChn = 4,
        .u32SclPort = SCL_PORT_ID,
        .u32VencChn = 4,

        .pszStreamName = UVC_STREAM3,
        .bForceIdr = FALSE,
    },

    [USB_CAMERA5_INDEX] =
    {
        .u32IspDev = ISP_DEV_ID,
        .u32IspChn = ISP_CHN_ID,
        .u32IspPort = ISP_PORT_ID,
        .u32SclDev = SCL_DEV_ID,
        .u32SclChn = 5,
        .u32SclPort = SCL_PORT_ID,
        .u32VencChn = 5,

        .pszStreamName = UVC_STREAM3,
        .bForceIdr = FALSE,
    },
};

#ifdef HDMI_ENABLE
MI_S32 Hdmi_Start(MI_HDMI_DeviceId_e eHdmi, MI_HDMI_TimingType_e eTimingType)
{
    MI_HDMI_Attr_t stAttr;

    STDBG_ENTER();
    memset(&stAttr, 0, sizeof(MI_HDMI_Attr_t));
    stAttr.stEnInfoFrame.bEnableAudInfoFrame  = FALSE;
    stAttr.stEnInfoFrame.bEnableAviInfoFrame  = FALSE;
    stAttr.stEnInfoFrame.bEnableSpdInfoFrame  = FALSE;
    stAttr.stAudioAttr.bEnableAudio = TRUE;
    stAttr.stAudioAttr.bIsMultiChannel = 0;
    stAttr.stAudioAttr.eBitDepth = E_MI_HDMI_BIT_DEPTH_16;
    stAttr.stAudioAttr.eCodeType = E_MI_HDMI_ACODE_PCM;
    stAttr.stAudioAttr.eSampleRate = E_MI_HDMI_AUDIO_SAMPLERATE_48K;
    stAttr.stVideoAttr.bEnableVideo = TRUE;
    stAttr.stVideoAttr.eColorType = E_MI_HDMI_COLOR_TYPE_RGB444;//default color type
    stAttr.stVideoAttr.eInColorType = E_MI_HDMI_COLOR_TYPE_YCBCR444;//default color type
    stAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_24BIT;//E_MI_HDMI_DEEP_COLOR_MAX;
    stAttr.stVideoAttr.eTimingType = eTimingType;
    stAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
    ExecFunc(MI_HDMI_SetAttr(eHdmi, &stAttr), MI_SUCCESS);

    ExecFunc(MI_HDMI_Start(eHdmi), MI_SUCCESS);
    STDBG_LEAVE();

    return MI_SUCCESS;
}

static MI_S32 Disp_DevInit(ST_DispDevInfo_t *pstDispDevInfo)
{
    MI_DISP_DEV DispDev = pstDispDevInfo->DispDev;
    MI_DISP_PubAttr_t stPubAttr;

    MI_DISP_LAYER DispLayer = pstDispDevInfo->DispLayer;
    MI_U32 u32Toleration = pstDispDevInfo->u32Toleration;
    MI_DISP_VideoLayerAttr_t stLayerAttr;

    STDBG_ENTER();
    memset(&stPubAttr, 0, sizeof(stPubAttr));
    stPubAttr.u32BgColor = YUYV_BLACK;
    stPubAttr.eIntfSync = pstDispDevInfo->eTiming;
    if(0 == DispDev)
    {
        stPubAttr.eIntfType = E_MI_DISP_INTF_BT1120;
        stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_USER;
        ExecFunc(MI_DISP_SetPubAttr(DispDev, &stPubAttr), MI_SUCCESS);
    }
    else if(1 == DispDev)
    {
        stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
        ExecFunc(MI_DISP_SetPubAttr(DispDev, &stPubAttr), MI_SUCCESS);
    }

    ExecFunc(MI_DISP_Enable(DispDev), MI_SUCCESS);
    if(stPubAttr.eIntfType == E_MI_DISP_INTF_HDMI)
    {
        STCHECKRESULT(ST_Hdmi_Init());
    }

    memset(&stLayerAttr, 0, sizeof(stLayerAttr));

    stLayerAttr.stVidLayerSize.u16Width  = pstDispDevInfo->stLayerAttr.stVidLayerSize.u16Width;
    stLayerAttr.stVidLayerSize.u16Height = pstDispDevInfo->stLayerAttr.stVidLayerSize.u16Height;

    stLayerAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stLayerAttr.stVidLayerDispWin.u16X      = pstDispDevInfo->stLayerAttr.stVidLayerDispWin.u16X;
    stLayerAttr.stVidLayerDispWin.u16Y      = pstDispDevInfo->stLayerAttr.stVidLayerDispWin.u16Y;
    stLayerAttr.stVidLayerDispWin.u16Width  = pstDispDevInfo->stLayerAttr.stVidLayerDispWin.u16Width;
    stLayerAttr.stVidLayerDispWin.u16Height = pstDispDevInfo->stLayerAttr.stVidLayerDispWin.u16Height;
    // must before set layer attribute

    ExecFunc(MI_DISP_BindVideoLayer(DispLayer, DispDev), MI_SUCCESS);
    ExecFunc(MI_DISP_SetVideoLayerAttr(DispLayer, &stLayerAttr), MI_SUCCESS);
    ExecFunc(MI_DISP_GetVideoLayerAttr(DispLayer, &stLayerAttr), MI_SUCCESS);
    printf("\n");
    printf("[%s %d]Get Video Layer Size [%d, %d] !!!\n", __FUNCTION__, __LINE__, stLayerAttr.stVidLayerSize.u16Width,
        stLayerAttr.stVidLayerSize.u16Height);
    printf("[%s %d]Get Video Layer DispWin [%d, %d, %d, %d] !!!\n", __FUNCTION__, __LINE__,\
        stLayerAttr.stVidLayerDispWin.u16X, stLayerAttr.stVidLayerDispWin.u16Y,
        stLayerAttr.stVidLayerDispWin.u16Width, stLayerAttr.stVidLayerDispWin.u16Height);
    printf("\n");
    ExecFunc(MI_DISP_EnableVideoLayer(DispLayer), MI_SUCCESS);
    STDBG_LEAVE();

    return MI_SUCCESS;
}

MI_S32 Disp_DevInit(MI_DISP_DEV dispDev, MI_DISP_LAYER DispLayer, MI_DISP_OutputTiming_e eTiming)
{
    ST_DispDevInfo_t stDispDevInfo = {};

    STDBG_ENTER();

    switch (eTiming)
    {
        case E_MI_DISP_OUTPUT_PAL:
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Width = 720;
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Height = 576;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16X = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Y = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Width = 720;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Height = 576;
            break;
        case E_MI_DISP_OUTPUT_NTSC:
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Width = 720;
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Height = 480;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16X = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Y = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Width = 720;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Height = 480;
            break;
        case E_MI_DISP_OUTPUT_720P60:
        case E_MI_DISP_OUTPUT_720P50:
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Width = 1280;
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Height = 720;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16X = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Y = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Width = 1280;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Height = 720;
            break;
        case E_MI_DISP_OUTPUT_1080P60:
        case E_MI_DISP_OUTPUT_1080P50:
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Width = 1920;
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Height = 1080;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16X = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Y = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Width = 1920;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Height = 1080;
            break;
        case E_MI_DISP_OUTPUT_1024x768_60:
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Width = 1024;
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Height = 768;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16X = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Y = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Width = 1024;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Height = 768;
            break;
        case E_MI_DISP_OUTPUT_1280x1024_60:
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Width = 1280;
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Height = 1024;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16X = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Y = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Width = 1280;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Height = 1024;
            break;
        case E_MI_DISP_OUTPUT_1440x900_60:
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Width = 1440;
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Height = 900;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16X = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Y = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Width = 1440;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Height = 900;
            break;
        case E_MI_DISP_OUTPUT_1600x1200_60:
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Width = 1600;
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Height = 1200;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16X = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Y = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Width = 1600;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Height = 1200;
            break;
        case E_MI_DISP_OUTPUT_1280x800_60:
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Width = 1280;
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Height = 800;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16X = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Y = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Width = 1280;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Height = 800;
            break;
        case E_MI_DISP_OUTPUT_1366x768_60:
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Width = 1366;
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Height = 768;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16X = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Y = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Width = 1366;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Height = 768;
            break;
        case E_MI_DISP_OUTPUT_1680x1050_60:
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Width = 1680;
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Height = 1050;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16X = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Y = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Width = 1680;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Height = 1050;
            break;
        case E_MI_DISP_OUTPUT_3840x2160_30:
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Width = 3840;
            stDispDevInfo.stLayerAttr.stVidLayerSize.u16Height = 2160;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16X = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Y = 0;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Width = 3840;
            stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Height = 2160;
            break;
        default:
            printf("Unsupported timing!\n");
            return -1;
    }

    stDispDevInfo.eTiming   = eTiming;
    stDispDevInfo.DispDev   = dispDev;
    stDispDevInfo.DispLayer = DispLayer;
    printf("layer size(%d-%d) window(%d-%d-%d-%d)...\n",
        stDispDevInfo.stLayerAttr.stVidLayerSize.u16Width,
        stDispDevInfo.stLayerAttr.stVidLayerSize.u16Height,
        stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16X,
        stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Y,
        stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Width,
        stDispDevInfo.stLayerAttr.stVidLayerDispWin.u16Height);
    STCHECKRESULT(Disp_DevInit(&stDispDevInfo));

    STDBG_LEAVE();

    return MI_SUCCESS;
}

MI_S32 Disp_ChnInit(MI_DISP_LAYER DispLayer, const ST_DispChnInfo_t *pstDispChnInfo)
{
    MI_U32 i = 0;
    MI_U32 u32InputPort = 0;
    MI_S32 InputPortNum = pstDispChnInfo->InputPortNum; //test use 0

    MI_DISP_InputPortAttr_t stInputPortAttr;

    STDBG_ENTER();
    for (i = 0; i < InputPortNum; i++)
    {
        memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));

        u32InputPort = pstDispChnInfo->stInputPortAttr[i].u32Port;
        ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, u32InputPort, &stInputPortAttr), MI_SUCCESS);
        stInputPortAttr.stDispWin.u16X      = pstDispChnInfo->stInputPortAttr[i].stAttr.stDispWin.u16X;
        stInputPortAttr.stDispWin.u16Y      = pstDispChnInfo->stInputPortAttr[i].stAttr.stDispWin.u16Y;
        stInputPortAttr.stDispWin.u16Width  = pstDispChnInfo->stInputPortAttr[i].stAttr.stDispWin.u16Width;
        stInputPortAttr.stDispWin.u16Height = pstDispChnInfo->stInputPortAttr[i].stAttr.stDispWin.u16Height;
        stInputPortAttr.u16SrcWidth = pstDispChnInfo->stInputPortAttr[i].stAttr.u16SrcWidth;
        stInputPortAttr.u16SrcHeight = pstDispChnInfo->stInputPortAttr[i].stAttr.u16SrcHeight;

        ExecFunc(MI_DISP_SetInputPortAttr(DispLayer, u32InputPort, &stInputPortAttr), MI_SUCCESS);
        ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, u32InputPort, &stInputPortAttr), MI_SUCCESS);
        ExecFunc(MI_DISP_EnableInputPort(DispLayer, u32InputPort), MI_SUCCESS);
        ExecFunc(MI_DISP_SetInputPortSyncMode(DispLayer, u32InputPort, E_MI_DISP_SYNC_MODE_FREE_RUN), MI_SUCCESS);
    }
    STDBG_LEAVE();

    return MI_SUCCESS;
}

MI_S32 Disp_DeInit(MI_DISP_DEV DispDev, MI_DISP_LAYER DispLayer, MI_S32 s32InputPortNum)
{
    MI_S32 s32InputPort = 0;
    for (s32InputPort = 0; s32InputPort < s32InputPortNum; s32InputPort++)
    {
        ExecFunc(MI_DISP_DisableInputPort(DispLayer, s32InputPort), MI_SUCCESS);
    }
    ExecFunc(MI_DISP_DisableVideoLayer(DispLayer), MI_SUCCESS);
    ExecFunc(MI_DISP_UnBindVideoLayer(DispLayer, DispDev), MI_SUCCESS);
    ExecFunc(MI_DISP_Disable(DispDev), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 Disp_ShowStatus(MI_DISP_LAYER DispLayer, MI_S32 s32InputPortNum, MI_BOOL bIsShow)
{
    if (bIsShow)
    {
        ExecFunc(MI_DISP_ShowInputPort(DispLayer, s32InputPortNum), MI_SUCCESS);
    }
    else
    {
        ExecFunc(MI_DISP_HideInputPort(DispLayer, s32InputPortNum), MI_SUCCESS);

    }

    return MI_SUCCESS;
}
#endif

#ifdef AUDIO_ENABLE

#if defined(__cplusplus) || defined(c_plusplus)
extern "C"{
#endif
#include "mi_ao.h"
#include "mi_ai.h"
#include "st_uac.h"
#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#define AI_VOLUME_AMIC_MIN      (0)
#define AI_VOLUME_AMIC_MAX      (19)
#define AI_VOLUME_DMIC_MIN      (0)
#define AI_VOLUME_DMIC_MAX      (6)

MI_BOOL bEnableAI = FALSE;
MI_BOOL bEnableAO= FALSE;
MI_U32 AiIfId= 0;
MI_U32 u32AiSampleRate = 0;
MI_U32 AoIfId= 0;
MI_U32 u32AoSampleRate = E_MI_AUDIO_SAMPLE_RATE_48000;

static MI_S32 ST_AO_FILE_Init(MI_U32 u32AoSampleRate, MI_U8 chn)
{
    char         wav_path[256], wav_name[256];

    memset(wav_name, 0x00, sizeof(wav_name));
    sprintf(wav_name, "%dK_16bit_STERO_30s.wav", u32AoSampleRate / 1000);
    memset(wav_path, 0x00, sizeof(wav_path));
    strcat(wav_path, g_stUacFile[1].filePath);
    strcat(wav_path, "/wav/out/");
    strcat(wav_path, wav_name);

    g_stUacFile[1].wavFd = open(wav_path, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (g_stUacFile[1].wavFd < 0)
    {
        printf("open %s error\n", wav_path);
        return -1;
    }

    return MI_SUCCESS;
}

static MI_S32 ST_AO_FILE_Deinit(void)
{
    close(g_stUacFile[1].wavFd);
    return MI_SUCCESS;
}

static MI_S32 ST_AI_FILE_Init(MI_U32 u32AiSampleRate, MI_U8 chn)
{
    char         wav_path[256], wav_name[256];

    memset(wav_name, 0x00, sizeof(wav_name));
    sprintf(wav_name, "%dK_16bit_STERO_30s.wav", u32AiSampleRate / 1000);
    memset(wav_path, 0x00, sizeof(wav_path));
    strcat(wav_path, g_stUacFile[0].filePath);
    strcat(wav_path, "/wav_in/");
    strcat(wav_path, wav_name);

    g_stUacFile[0].wavFd = open(wav_path, O_RDONLY);
    if (g_stUacFile[0].wavFd < 0)
    {
        printf("open %s error\n", wav_path);
        return -1;
    }

    return MI_SUCCESS;
}


static MI_S32 ST_AI_FILE_Deinit(void)
{
    close(g_stUacFile[0].wavFd);
    return MI_SUCCESS;
}

static MI_S32 ST_AI_FILE_SetVqeVolume(ST_UAC_Volume_t stVolume)
{
    return MI_SUCCESS;
}

static MI_S32 AO_FILE_TakeBuffer(ST_UAC_Frame_t *stUacFrame)
{
    MI_S32 ret = MI_SUCCESS;
    MI_S32 len;

    len = write(g_stUacFile[1].wavFd, stUacFrame->data, stUacFrame->length);
    if (len <= 0)
    {
        ret = -1;
    }

    return ret;
}

static MI_S32 AI_FILE_FillBuffer(ST_UAC_Frame_t *stUacFrame)
{
    MI_S32 ret = MI_SUCCESS;
    MI_S32 len, s32NeedSize = stUacFrame->length;

    len = read(g_stUacFile[0].wavFd, stUacFrame->data, s32NeedSize);
    if (len > 0)
    {
        stUacFrame->length = len;
    }
    else if (0 == len)
    {
        lseek(g_stUacFile[0].wavFd, 0, SEEK_SET);
    }
    else
    {
        ret = -1;
    }

    return ret;
}

static MI_S32 ST_AO_Init(MI_U32 u32AoSampleRate, MI_U8 chn)
{
    MI_AO_Attr_t stAoSetAttr;
    MI_S8 s8LeftVolume = 0;
    MI_S8 s8RightVolume = 0;
    MI_AO_GainFading_e eGainFading = E_MI_AO_GAIN_FADING_OFF;

    memset(&stAoSetAttr, 0, sizeof(MI_AO_Attr_t));
    stAoSetAttr.enChannelMode = E_MI_AO_CHANNEL_MODE_STEREO;
    stAoSetAttr.enFormat = E_MI_AUDIO_FORMAT_PCM_S16_LE;
    stAoSetAttr.enSampleRate = (MI_AUDIO_SampleRate_e)u32AoSampleRate;
    stAoSetAttr.enSoundMode = E_MI_AUDIO_SOUND_MODE_STEREO;
    stAoSetAttr.u32PeriodSize = 1024;

    ExecFunc(MI_AO_Open(0, &stAoSetAttr), MI_SUCCESS);
    ExecFunc(MI_AO_AttachIf(0, (MI_AO_If_e)AoIfId, 0), MI_SUCCESS);

    ExecFunc(MI_AO_SetVolume(0, s8LeftVolume, s8RightVolume, eGainFading), MI_SUCCESS);

    return MI_SUCCESS;
}

static MI_S32 ST_AO_Deinit(void)
{
    ExecFunc(MI_AO_DetachIf(0, (MI_AO_If_e)AoIfId), MI_SUCCESS);
    ExecFunc(MI_AO_Close(0), MI_SUCCESS);
    return MI_SUCCESS;
}

static MI_S32 ST_AI_Init(MI_U32 u32AiSampleRate, MI_U8 chn)
{
    MI_AI_Attr_t stAiSetAttr;
    MI_AI_If_e enAiIf[] = {(MI_AI_If_e)AiIfId};
    MI_SYS_ChnPort_t stAiChnOutputPort;

    memset(&stAiSetAttr, 0, sizeof(MI_AI_Attr_t));
    stAiSetAttr.bInterleaved = TRUE;
    stAiSetAttr.enFormat = E_MI_AUDIO_FORMAT_PCM_S16_LE;
    stAiSetAttr.enSampleRate = (MI_AUDIO_SampleRate_e)u32AiSampleRate;
    stAiSetAttr.enSoundMode = E_MI_AUDIO_SOUND_MODE_STEREO;
    stAiSetAttr.u32PeriodSize = 1024;

    ExecFunc(MI_AI_Open(0, &stAiSetAttr), MI_SUCCESS);
    ExecFunc(MI_AI_AttachIf(0, enAiIf, sizeof(enAiIf) / sizeof(enAiIf[0])), MI_SUCCESS);

    memset(&stAiChnOutputPort, 0, sizeof(stAiChnOutputPort));
    stAiChnOutputPort.eModId = E_MI_MODULE_ID_AI;
    stAiChnOutputPort.u32DevId = 0;
    stAiChnOutputPort.u32ChnId = 0;
    stAiChnOutputPort.u32PortId = 0;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(0, &stAiChnOutputPort, 4, 8), MI_SUCCESS);
    ExecFunc(MI_AI_SetIfGain(enAiIf[0], 18, 18), MI_SUCCESS);
    ExecFunc(MI_AI_EnableChnGroup(0, 0), MI_SUCCESS);

    return MI_SUCCESS;
}

static MI_S32 ST_AI_Deinit(void)
{
    ExecFunc(MI_AI_DisableChnGroup(0, 0), MI_SUCCESS);
    ExecFunc(MI_AI_Close(0), MI_SUCCESS);
    return MI_SUCCESS;
}

static inline MI_S32 Vol_to_Db(ST_UAC_Volume_t stVolume)
{
    switch (AiIfId)
    {
        case E_MI_AI_IF_ADC_AB:
        case E_MI_AI_IF_ADC_CD:
            return ((stVolume.s32Volume - stVolume.s32Min) * (AI_VOLUME_AMIC_MAX - AI_VOLUME_AMIC_MIN) / (stVolume.s32Max - stVolume.s32Min)) + AI_VOLUME_AMIC_MIN;
            break;
        case E_MI_AI_IF_DMIC_A_01:
        case E_MI_AI_IF_DMIC_A_23:
            return ((stVolume.s32Volume - stVolume.s32Min) * (AI_VOLUME_DMIC_MAX - AI_VOLUME_DMIC_MIN) / (stVolume.s32Max - stVolume.s32Min)) + AI_VOLUME_DMIC_MIN;
            break;
        default:
            break;

    }
    return -EINVAL;
}

static MI_S32 ST_AI_SetVqeVolume(ST_UAC_Volume_t stVolume)
{
    MI_S32 ret = MI_SUCCESS, s32AiVolume;

    s32AiVolume = Vol_to_Db(stVolume);
    ret = MI_AI_SetIfGain((MI_AI_If_e)AiIfId, s32AiVolume, s32AiVolume);
    if(MI_SUCCESS != ret)
    {
        printf("MI_AI_SetIfGain failed: %x.\n", ret);
    }

    return ret;
}

static MI_S32 AO_TakeBuffer(ST_UAC_Frame_t *stUacFrame)
{
    MI_S32 ret = MI_SUCCESS;

    do {
        ret = MI_AO_Write(0, stUacFrame->data, stUacFrame->length, 0, -1);
    } while ((ret == MI_AO_ERR_NOBUF));

    if(MI_SUCCESS != ret)
    {
        printf("MI_AO_Write failed: %x.\n", ret);
    }

    return ret;
}

static MI_S32 AI_FillBuffer(ST_UAC_Frame_t *stUacFrame)
{
    MI_S32 ret = MI_SUCCESS;
    MI_AI_Data_t stAiFrame, stAiFrame2;

    ret = MI_AI_Read(0, 0, &stAiFrame, &stAiFrame2, -1);
    if (MI_SUCCESS != ret)
    {
        printf("MI_AI_Read failed: %x.\n", ret);
        goto exit;
    }

    memcpy(stUacFrame->data, stAiFrame.apvBuffer[0], stAiFrame.u32Byte[0]);
    stUacFrame->length = stAiFrame.u32Byte[0];

    ret = MI_AI_ReleaseData(0, 0, &stAiFrame, &stAiFrame2);
    if(MI_SUCCESS != ret)
    {
        printf("MI_AI_ReleaseData failed: %x.\n", ret);
    }

exit:
    return ret;
}

MI_S32 ST_UacInit(ST_UAC_Handle_h *pstHandle)
{
    MI_S32 ret = MI_SUCCESS;
    static ST_UAC_Device_t *pstDevice = NULL;
    ST_UAC_OPS_t opsAo = {ST_AO_Init, AO_TakeBuffer, ST_AO_Deinit, NULL};
    ST_UAC_OPS_t opsAi = {ST_AI_Init, AI_FillBuffer, ST_AI_Deinit, ST_AI_SetVqeVolume};
    MI_S32 eMode = 0;

    if (!bEnableAO && !bEnableAI)
        return ret;

    if (bEnableAO)
        eMode |= AS_OUT_MODE;
    if (bEnableAI)
        eMode |= AS_IN_MODE;

    if (g_bEnableFile)
    {
        opsAi.UAC_AUDIO_Init = ST_AI_FILE_Init;
        opsAi.UAC_AUDIO_BufTask = AI_FILE_FillBuffer;
        opsAi.UAC_AUDIO_Deinit = ST_AI_FILE_Deinit;
        opsAi.UAC_AUDIO_SetVol  = ST_AI_FILE_SetVqeVolume;

        opsAo.UAC_AUDIO_Init = ST_AO_FILE_Init;
        opsAo.UAC_AUDIO_BufTask = AO_FILE_TakeBuffer;
        opsAo.UAC_AUDIO_Deinit  = ST_AO_FILE_Deinit;
        opsAo.UAC_AUDIO_SetVol = NULL;
    }

    ret = ST_UAC_AllocStream(pstHandle);
    if(MI_SUCCESS != ret)
    {
        printf("ST_UAC_AllocStream failed!\n");
        goto exit;
    }

    pstDevice = (ST_UAC_Device_t *)*pstHandle;
    pstDevice->mode = eMode;
    pstDevice->opsAo = opsAo;
    pstDevice->opsAi = opsAi;
    pstDevice->config[1]->pcm_config.rate = u32AoSampleRate;
    pstDevice->config[1]->pcm_config.channels = 2;
    pstDevice->config[0]->pcm_config.rate = 0;
    pstDevice->config[0]->pcm_config.channels = 2;
    pstDevice->config[0]->volume.s32Volume = -EINVAL;

    ST_UAC_StartDev(*pstHandle);

exit:
    return ret;
}

MI_S32 ST_UacDeinit(ST_UAC_Handle_h stHandle)
{
    MI_S32 ret = MI_SUCCESS;

    if(!bEnableAO && !bEnableAI)
        return ret;

    ST_UAC_StoptDev(stHandle);
    ret = ST_UAC_FreeStream(stHandle);
    if(MI_SUCCESS != ret)
    {
        printf("ST_UAC_FreeStream failed.\n");
    }

    return ret;
}

#endif

void ST_Flush(void)
{
    char c;

    while((c = getchar()) != '\n' && c != EOF);
}

MI_BOOL ST_DoSetIqBin(MI_ISP_DEV IspDevId, MI_ISP_CHANNEL IspChnId, char *pConfigPath)
{
#ifdef ISP_ALGO_ENABLE
    MI_ISP_IQ_ParamInitInfoType_t status;
    MI_U8  u8ispreadycnt = 0;

    if (strlen(pConfigPath) == 0)
    {
        printf("IQ Bin File path NULL!\n");
        return FALSE;
    }

    do
    {
        if(u8ispreadycnt > 100)
        {
            printf("%s:%d, isp ready time out \n", __FUNCTION__, __LINE__);
            u8ispreadycnt = 0;
            break;
        }

        STCHECKRESULT(MI_ISP_IQ_GetParaInitStatus(IspDevId, IspChnId, &status));
        if(status.stParaAPI.bFlag != 1)
        {
            usleep(300*1000);
            u8ispreadycnt++;
            continue;
        }

        u8ispreadycnt = 0;
        printf("loading api bin...path:%s\n",pConfigPath);
        STCHECKRESULT(MI_ISP_ApiCmdLoadBinFile(IspDevId, IspChnId, (char *)pConfigPath, 1234));

        usleep(10*1000);
    }while(!status.stParaAPI.bFlag);

    MI_ISP_AE_FlickerType_e Flicker = E_SS_AE_FLICKER_TYPE_50HZ;
    STCHECKRESULT(MI_ISP_AE_SetFlicker(IspDevId, IspChnId, &Flicker));
    printf("MI_ISP_AE_SetFlicker!\n");
#endif

    return MI_SUCCESS;
}

MI_BOOL ST_DoChangeHdrRes(MI_BOOL bEnableHdr, MI_U8 _u8ResIndex)
{
    //todo
    return MI_SUCCESS;
}

MI_S32 ST_Test()
{
    //todo
    return MI_SUCCESS;
}

MI_S32 ST_VideoModuleInit(ST_Config_S* pstConfig)
{
    /************************************************
    Step0:  Init Sensor
    *************************************************/
    MI_SNR_PADID u8SnrPad = (MI_SNR_PADID)pstConfig->u8SnrPad;
    MI_S8 s8SnrResIndex = pstConfig->s8SnrResIndex;

    MI_U32 u32ResCount =0;
    MI_U8 u8ResIndex =0;
    MI_U32 u32ChocieRes =0;

    MI_SNR_PADInfo_t  stSnrPadInfo;
    MI_SNR_PlaneInfo_t stSnrPlaneInfo;
    MI_SNR_Res_t stSnrRes;
    memset(&stSnrPadInfo, 0x0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlaneInfo, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stSnrRes, 0x0, sizeof(MI_SNR_Res_t));

    ST_DBG("Snr pad id:%d\n", (int)u8SnrPad);

    if(pstConfig->s32HDRtype > 0)
        STCHECKRESULT(MI_SNR_SetPlaneMode(u8SnrPad, TRUE));
    else
        STCHECKRESULT(MI_SNR_SetPlaneMode(u8SnrPad, FALSE));

    STCHECKRESULT(MI_SNR_QueryResCount(u8SnrPad, &u32ResCount));

    for(u8ResIndex = 0; u8ResIndex < u32ResCount; u8ResIndex++)
    {
        STCHECKRESULT(MI_SNR_GetRes(u8SnrPad, u8ResIndex, &stSnrRes));
        printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
        u8ResIndex,
        stSnrRes.stCropRect.u16X, stSnrRes.stCropRect.u16Y, stSnrRes.stCropRect.u16Width,stSnrRes.stCropRect.u16Height,
        stSnrRes.stOutputSize.u16Width, stSnrRes.stOutputSize.u16Height,
        stSnrRes.u32MaxFps,stSnrRes.u32MinFps,
        stSnrRes.strResDesc);
    }

    if(s8SnrResIndex < 0)
    {
        printf("choice which resolution use, cnt %d\n", u32ResCount);
        do
        {
            scanf("%d", &u32ChocieRes);
            ST_Flush();
            STCHECKRESULT(MI_SNR_QueryResCount(u8SnrPad, &u32ResCount));
            if(u32ChocieRes >= u32ResCount)
            {
                printf("choice err res %d > =cnt %d\n", u32ChocieRes, u32ResCount);
            }
        }while(u32ChocieRes >= u32ResCount);
    }
    else
    {
        if((MI_U32)s8SnrResIndex >= u32ResCount)
        {
            ST_ERR("snr index:%d exceed u32ResCount:%d, set default index 0\n", s8SnrResIndex, u32ResCount);
            s8SnrResIndex = 0;
        }

        u32ChocieRes = s8SnrResIndex;
    }
    printf("You select %d res\n", u32ChocieRes);

    STCHECKRESULT(MI_SNR_GetRes(u8SnrPad, u32ChocieRes, &stSnrRes));
    pstConfig->u32MaxFps = stSnrRes.u32MaxFps;

    STCHECKRESULT(MI_SNR_SetRes(u8SnrPad,u32ChocieRes));
    STCHECKRESULT(MI_SNR_Enable(u8SnrPad));

    /************************************************
    Step1:  Init Vif
    *************************************************/
    MI_VIF_GROUP VifGroupId;
    MI_VIF_DEV VifDevId;
    MI_VIF_DEV VifChnId = 0;
    MI_VIF_PORT VifPortId = 0;

    switch(u8SnrPad)
    {
        case 0:
            VifGroupId = 0;
            VifDevId = 0;
            break;
        case 1:
            VifGroupId = 2;
            VifDevId = 8;
            break;
        case 2:
            VifGroupId = 1;
            VifDevId = 4;
            break;
        case 3:
            VifGroupId = 3;
            VifDevId = 12;
            break;
        default:
            ST_ERR("Invalid Snr pad id:%d\n", (int)u8SnrPad);
            return -1;
    }

    MI_SYS_ChnPort_t stChnPort;
    MI_VIF_GroupAttr_t stVifGroupAttr;
    MI_VIF_DevAttr_t stVifDevAttr;
    MI_VIF_OutputPortAttr_t stVifPortAttr;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stVifGroupAttr, 0x0, sizeof(MI_VIF_GroupAttr_t));
    memset(&stVifDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));
    memset(&stVifPortAttr, 0x0, sizeof(MI_VIF_OutputPortAttr_t));

    STCHECKRESULT(MI_SNR_GetPadInfo(u8SnrPad, &stSnrPadInfo));
    STCHECKRESULT(MI_SNR_GetPlaneInfo(u8SnrPad, 0, &stSnrPlaneInfo));

    stVifGroupAttr.eIntfMode = E_MI_VIF_MODE_MIPI;
    stVifGroupAttr.eWorkMode = E_MI_VIF_WORK_MODE_1MULTIPLEX;
    stVifGroupAttr.eHDRType = E_MI_VIF_HDR_TYPE_OFF;
    if(stVifGroupAttr.eIntfMode == E_MI_VIF_MODE_BT656)
        stVifGroupAttr.eClkEdge = (MI_VIF_ClkEdge_e)stSnrPadInfo.unIntfAttr.stBt656Attr.eClkEdge;
    else
        stVifGroupAttr.eClkEdge = E_MI_VIF_CLK_EDGE_DOUBLE;
    STCHECKRESULT(MI_VIF_CreateDevGroup(VifGroupId, &stVifGroupAttr));

    stVifDevAttr.stInputRect.u16X = stSnrPlaneInfo.stCapRect.u16X;
    stVifDevAttr.stInputRect.u16Y = stSnrPlaneInfo.stCapRect.u16Y;
    stVifDevAttr.stInputRect.u16Width = stSnrPlaneInfo.stCapRect.u16Width;
    stVifDevAttr.stInputRect.u16Height = stSnrPlaneInfo.stCapRect.u16Height;
    if(stSnrPlaneInfo.eBayerId >= E_MI_SYS_PIXEL_BAYERID_MAX)
        stVifDevAttr.eInputPixel = stSnrPlaneInfo.ePixel;
    else
        stVifDevAttr.eInputPixel = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlaneInfo.ePixPrecision, stSnrPlaneInfo.eBayerId);
    STCHECKRESULT(MI_VIF_SetDevAttr(VifDevId, &stVifDevAttr));
    STCHECKRESULT(MI_VIF_EnableDev(VifDevId));

    stVifPortAttr.stCapRect.u16X = stSnrPlaneInfo.stCapRect.u16X;
    stVifPortAttr.stCapRect.u16Y = stSnrPlaneInfo.stCapRect.u16Y;
    stVifPortAttr.stCapRect.u16Width =  stSnrPlaneInfo.stCapRect.u16Width;
    stVifPortAttr.stCapRect.u16Height = stSnrPlaneInfo.stCapRect.u16Height;
    stVifPortAttr.stDestSize.u16Width = stSnrPlaneInfo.stCapRect.u16Width;
    stVifPortAttr.stDestSize.u16Height = stSnrPlaneInfo.stCapRect.u16Height;
    stVifPortAttr.eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    if(stSnrPlaneInfo.eBayerId >= E_MI_SYS_PIXEL_BAYERID_MAX)
        stVifPortAttr.ePixFormat = stSnrPlaneInfo.ePixel;
    else
        stVifPortAttr.ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlaneInfo.ePixPrecision, stSnrPlaneInfo.eBayerId);
    STCHECKRESULT(MI_VIF_SetOutputPortAttr(VifDevId, VifPortId, &stVifPortAttr));

    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = VifDevId;
    stChnPort.u32ChnId = VifChnId;
    stChnPort.u32PortId = VifPortId;
    STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(0, &stChnPort, 0, g_maxbuf_cnt+3));
    STCHECKRESULT(MI_VIF_EnableOutputPort(VifDevId, VifPortId));

    /************************************************
    Step2:  Init Isp
    *************************************************/
    MI_ISP_DEV IspDevId = ISP_DEV_ID;
    MI_ISP_CHANNEL IspChnId = ISP_CHN_ID;

    MI_ISP_DevAttr_t stIspDevAttr;
    MI_ISP_ChannelAttr_t  stIspChnAttr;
    MI_ISP_ChnParam_t stIspChnParam;
    memset(&stIspDevAttr, 0x0, sizeof(MI_ISP_DevAttr_t));
    memset(&stIspChnAttr, 0x0, sizeof(MI_ISP_ChannelAttr_t));
    memset(&stIspChnParam, 0x0, sizeof(MI_ISP_ChnParam_t));

    stIspDevAttr.u32DevStitchMask = E_MI_ISP_DEVICEMASK_ID0;
    STCHECKRESULT(MI_ISP_CreateDevice(IspDevId, &stIspDevAttr));

    switch(u8SnrPad)
    {
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
        default:
            ST_ERR("Invalid Snr pad id:%d\n", (int)u8SnrPad);
            return -1;
    }

    stIspChnParam.eHDRType = E_MI_ISP_HDR_TYPE_OFF;
    stIspChnParam.e3DNRLevel = pstConfig->en3dNrLevel;
    stIspChnParam.bMirror = FALSE;
    stIspChnParam.bFlip = FALSE;
    stIspChnParam.eRot = E_MI_SYS_ROTATE_NONE;
    STCHECKRESULT(MI_ISP_CreateChannel(IspDevId, IspChnId, &stIspChnAttr));
    STCHECKRESULT(MI_ISP_SetChnParam(IspDevId, IspChnId, &stIspChnParam));
    STCHECKRESULT(MI_ISP_StartChannel(IspDevId, IspChnId));

    /************************************************
    Step3:  Bind Vif and Isp
    *************************************************/
    Sys_BindInfo_T stBindInfo;
    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = VifDevId;
    stBindInfo.stSrcChnPort.u32ChnId = VifChnId;
    stBindInfo.stSrcChnPort.u32PortId = VifPortId;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_ISP;
    stBindInfo.stDstChnPort.u32DevId = IspDevId;
    stBindInfo.stDstChnPort.u32ChnId = IspChnId;
    stBindInfo.u32SrcFrmrate = pstConfig->u32MaxFps;
    stBindInfo.u32DstFrmrate = pstConfig->u32MaxFps;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    STCHECKRESULT(bind_port(&stBindInfo));

    /************************************************
    Step4:  Init Scl
    *************************************************/
    MI_SCL_DEV SclDevId = (!g_bEnableHDMI && g_device_num == 1) ? SCL_DEV_ID_RT : SCL_DEV_ID;

    MI_SCL_DevAttr_t stSclDevAttr;
    MI_SCL_ChannelAttr_t  stSclChnAttr;
    MI_SCL_ChnParam_t  stSclChnParam;
    memset(&stSclDevAttr, 0x0, sizeof(MI_SCL_DevAttr_t));
    memset(&stSclChnAttr, 0x0, sizeof(MI_SCL_ChannelAttr_t));
    memset(&stSclChnParam, 0x0, sizeof(MI_SCL_ChnParam_t));

    switch(SCL_PORT_ID)
    {
        case 0:
            stSclDevAttr.u32NeedUseHWOutPortMask = E_MI_SCL_HWSCL0; //Port0->HWSCL2
            break;
        case 1:
            stSclDevAttr.u32NeedUseHWOutPortMask = E_MI_SCL_HWSCL1; //Port1->HWSCL3
            break;
        case 2:
            stSclDevAttr.u32NeedUseHWOutPortMask = E_MI_SCL_HWSCL2; //Port2->HWSCL4
            break;
        default:
            ST_ERR("Invalid Scl Port Id:%d\n", SCL_PORT_ID);
            return -1;
    }
    STCHECKRESULT(MI_SCL_CreateDevice(SclDevId, &stSclDevAttr));

    for(int i = 0; i < g_device_num; i++)
    {
        STCHECKRESULT(MI_SCL_CreateChannel(SclDevId, i, &stSclChnAttr));
        STCHECKRESULT(MI_SCL_SetChnParam(SclDevId, i, &stSclChnParam));
        STCHECKRESULT(MI_SCL_StartChannel(SclDevId, i));
    }
#ifdef HDMI_ENABLE
    if(g_bEnableHDMI)
    {
        stSclDevAttr.u32NeedUseHWOutPortMask = E_MI_SCL_HWSCL5;
        STCHECKRESULT(MI_SCL_CreateDevice(SCL_HDMI_DEV_ID, &stSclDevAttr));
        STCHECKRESULT(MI_SCL_CreateChannel(SCL_HDMI_DEV_ID, g_device_num, &stSclChnAttr));
        STCHECKRESULT(MI_SCL_SetChnParam(SCL_HDMI_DEV_ID, g_device_num, &stSclChnParam));
        STCHECKRESULT(MI_SCL_StartChannel(SCL_HDMI_DEV_ID, g_device_num));
        MI_ISP_OutPortParam_t  stIspOutputParam;
        memset(&stIspOutputParam, 0x0, sizeof(MI_ISP_OutPortParam_t));
        STCHECKRESULT(MI_ISP_GetInputPortCrop(IspDevId, IspChnId, &stIspOutputParam.stCropRect));
        MI_SCL_OutPortParam_t  stSclOutputParam;
        memset(&stSclOutputParam, 0x0, sizeof(MI_SCL_OutPortParam_t));
        stSclOutputParam.stSCLOutCropRect.u16X = stIspOutputParam.stCropRect.u16X;
        stSclOutputParam.stSCLOutCropRect.u16Y = stIspOutputParam.stCropRect.u16Y;
        stSclOutputParam.stSCLOutCropRect.u16Width = stIspOutputParam.stCropRect.u16Width;
        stSclOutputParam.stSCLOutCropRect.u16Height = stIspOutputParam.stCropRect.u16Height;
        stSclOutputParam.stSCLOutputSize.u16Width = 1920;
        stSclOutputParam.stSCLOutputSize.u16Height = 1080;
        stSclOutputParam.bMirror = FALSE;
        stSclOutputParam.bFlip = FALSE;
        stSclOutputParam.eCompressMode= E_MI_SYS_COMPRESS_MODE_NONE;
        stSclOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        STCHECKRESULT(MI_ISP_SetOutputPortParam(IspDevId, IspChnId, ISP_PORT_ID, &stIspOutputParam));
        STCHECKRESULT(MI_SCL_SetOutputPortParam(SCL_HDMI_DEV_ID, g_device_num, SCL_PORT_ID, &stSclOutputParam));
        MI_SYS_ChnPort_t stChnPort[2];
        memset(&stChnPort[0], 0x0, sizeof(MI_SYS_ChnPort_t));
        stChnPort[0].eModId = E_MI_MODULE_ID_ISP;
        stChnPort[0].u32DevId = IspDevId;
        stChnPort[0].u32ChnId = IspChnId;
        stChnPort[0].u32PortId = ISP_PORT_ID;

        memset(&stChnPort[1], 0x0, sizeof(MI_SYS_ChnPort_t));
        stChnPort[1].eModId = E_MI_MODULE_ID_SCL;
        stChnPort[1].u32DevId = SCL_HDMI_DEV_ID;
        stChnPort[1].u32ChnId = g_device_num;
        stChnPort[1].u32PortId = SCL_PORT_ID;
        STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(0, &stChnPort[0] , 0, g_maxbuf_cnt+2));
        STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(0, &stChnPort[1] , g_maxbuf_cnt+1, g_maxbuf_cnt+2));
        STCHECKRESULT(MI_ISP_EnableOutputPort(IspDevId, IspChnId, ISP_PORT_ID));
        STCHECKRESULT(MI_SCL_EnableOutputPort(SCL_HDMI_DEV_ID, g_device_num, SCL_PORT_ID));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_ISP;
        stBindInfo.stSrcChnPort.u32DevId = IspDevId;
        stBindInfo.stSrcChnPort.u32ChnId = IspChnId;
        stBindInfo.stSrcChnPort.u32PortId = ISP_PORT_ID;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
        stBindInfo.stDstChnPort.u32DevId = SCL_HDMI_DEV_ID;
        stBindInfo.stDstChnPort.u32ChnId = g_device_num;
        stBindInfo.stDstChnPort.u32PortId = SCL_PORT_ID;
        stBindInfo.u32SrcFrmrate = pstConfig->u32MaxFps;
        stBindInfo.u32DstFrmrate = pstConfig->u32MaxFps;
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }
#endif
    /************************************************
    Step5:  Optional
    *************************************************/
#ifdef ISP_ALGO_ENABLE
    if(g_enable_iqserver)
    {
        ST_DBG("MI_IQSERVER_Open...\n");
        STCHECKRESULT(MI_IQSERVER_Open());
    }
#endif

    return MI_SUCCESS;
}

MI_S32 ST_VideoModuleUnInit(ST_Config_S* pstConfig)
{
    MI_SNR_PADID u8SnrPad = (MI_SNR_PADID)pstConfig->u8SnrPad;
    MI_VIF_GROUP VifGroupId;
    MI_VIF_DEV VifDevId;
    MI_VIF_DEV VifChnId =0;
    MI_VIF_PORT VifPortId = 0;
    switch(u8SnrPad)
    {
        case 0:
            VifGroupId = 0;
            VifDevId = 0;
            break;
        case 1:
            VifGroupId = 2;
            VifDevId = 8;
            break;
        case 2:
            VifGroupId = 1;
            VifDevId = 4;
            break;
        case 3:
            VifGroupId = 3;
            VifDevId = 12;
            break;
        default:
            ST_ERR("Invalid Snr pad id:%d\n", (int)u8SnrPad);
            return -1;
    }
    MI_ISP_DEV IspDevId = ISP_DEV_ID;
    MI_ISP_CHANNEL IspChnId = ISP_CHN_ID;
    MI_SCL_DEV SclDevId = (!g_bEnableHDMI && g_device_num == 1) ? SCL_DEV_ID_RT : SCL_DEV_ID;

    /************************************************
    Step0:  Deinit Scl
    *************************************************/
    for(int i = 0; i < g_device_num; i++)
    {
        STCHECKRESULT(MI_SCL_StopChannel(SclDevId, i));
        STCHECKRESULT(MI_SCL_DestroyChannel(SclDevId, i));
    }
#ifdef HDMI_ENABLE
    if(g_bEnableHDMI)
    {
        ST_Sys_BindInfo_T stBindInfo;
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_ISP;
        stBindInfo.stSrcChnPort.u32DevId = IspDevId;
        stBindInfo.stSrcChnPort.u32ChnId = IspChnId;
        stBindInfo.stSrcChnPort.u32PortId = ISP_PORT_ID;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
        stBindInfo.stDstChnPort.u32DevId = SCL_HDMI_DEV_ID;
        stBindInfo.stDstChnPort.u32ChnId = g_device_num;
        stBindInfo.stDstChnPort.u32PortId = SCL_PORT_ID;
        stBindInfo.u32SrcFrmrate = pstConfig->u32MaxFps;
        stBindInfo.u32DstFrmrate = pstConfig->u32MaxFps;
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
        STCHECKRESULT(MI_SCL_StopChannel(SCL_HDMI_DEV_ID, g_device_num));
        STCHECKRESULT(MI_SCL_DestroyChannel(SCL_HDMI_DEV_ID, g_device_num));
        STCHECKRESULT(MI_SCL_DestroyDevice(SCL_HDMI_DEV_ID));
    }
#endif
    STCHECKRESULT(MI_SCL_DestroyDevice(SclDevId));

    /************************************************
    Step1:  Unbind Vif and Isp
    *************************************************/
    Sys_BindInfo_T stBindInfo;
    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = VifDevId;
    stBindInfo.stSrcChnPort.u32ChnId = VifChnId;
    stBindInfo.stSrcChnPort.u32PortId = VifPortId;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_ISP;
    stBindInfo.stDstChnPort.u32DevId = IspDevId;
    stBindInfo.stDstChnPort.u32ChnId = IspChnId;
    stBindInfo.u32SrcFrmrate = pstConfig->u32MaxFps;
    stBindInfo.u32DstFrmrate = pstConfig->u32MaxFps;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    STCHECKRESULT(unbind_port(&stBindInfo));
    /************************************************
    Step2:  Deinit lsp
    *************************************************/
    STCHECKRESULT(MI_ISP_StopChannel(IspDevId, IspChnId));
    STCHECKRESULT(MI_ISP_DestroyChannel(IspDevId, IspChnId));
    STCHECKRESULT(MI_ISP_DestoryDevice(IspDevId));

    /************************************************
    Step3:  Deinit Vif
    *************************************************/
    STCHECKRESULT(MI_VIF_DisableOutputPort(VifDevId, VifPortId));
    STCHECKRESULT(MI_VIF_DisableDev(VifDevId));
    STCHECKRESULT(MI_VIF_DestroyDevGroup(VifGroupId));

    /************************************************
    Step4:  Deinit Sensor
    *************************************************/
    STCHECKRESULT(MI_SNR_Disable(u8SnrPad));

    /************************************************
    Step5:  Optional
    *************************************************/
#ifdef ISP_ALGO_ENABLE
    if(g_enable_iqserver)
        STCHECKRESULT(MI_IQSERVER_Close());
#endif

    return MI_SUCCESS;
}

void ST_DefaultArgs(ST_Config_S *pstConfig)
{
    memset(pstConfig, 0, sizeof(ST_Config_S));

    pstConfig->s32HDRtype    = 0;
    pstConfig->enRotation = E_MI_SYS_ROTATE_NONE;
    pstConfig->en3dNrLevel = E_MI_ISP_3DNR_LEVEL_OFF;
    pstConfig->u8SnrPad = 0;
    pstConfig->s8SnrResIndex = -1;
}

void ST_HandleSig(MI_S32 signo)
{
    if(signo == SIGINT)
    {
        ST_INFO("catch Ctrl + C, exit normally\n");

        g_bExit = TRUE;
    }
}

static MI_S32 UVC_Init(void *uvc)
{
    return MI_SUCCESS;
}

static MI_S32 UVC_Deinit(void *uvc)
{
    return MI_SUCCESS;
}

static ST_UvcDev_t * Get_UVC_Device(void *uvc)
{
    ST_UVC_Device_t *pdev = (ST_UVC_Device_t*)uvc;

    for (int i = 0; i < g_UvcSrc->devnum;i++)
    {
        ST_UvcDev_t *dev = &g_UvcSrc->dev[i];
        if (!strcmp(dev->name, pdev->name))
        {
            return dev;
        }
    }
    return NULL;
}

static void UVC_ForceIdr(void *uvc)
{
    ST_UvcDev_t *dev = Get_UVC_Device(uvc);
    if ((dev->setting.fcc == V4L2_PIX_FMT_H264) || (dev->setting.fcc == V4L2_PIX_FMT_H265)) {
        g_stStreamAttr[dev->dev_index].bForceIdr = TRUE;
    }
}

static MI_S32 getFramePos(int fd, char *str, int strLen, MI_U32 *arr, MI_U32 *len)
{
    MI_U32 arrLen = *len;
    int    res = 0;
    long   pos    = 0;
    long   posEnd = 0;
    char * buf    = (char *)malloc(strLen * sizeof(char));

    if (!buf)
        return -1;

    posEnd = lseek(fd, 0L, SEEK_END) - strLen;
    *len    = 0;

    while (pos <= posEnd && *len < arrLen)
    {
        lseek(fd, pos, SEEK_SET);
        res = read(fd, buf, strLen);
        if (res != strLen)
            break;
        if (memcmp(str, buf, strLen * sizeof(char)) == 0)
        {
            arr[*len] = pos;
            (*len)++;
        }
        pos++;
    }

    lseek(fd, 0L, SEEK_SET);
    free(buf);

    return MI_SUCCESS;
}

static MI_S32 UVC_UP_FILE_FinishBuffer(void *uvc,ST_UVC_BufInfo_t *bufInfo)
{
    ST_ERR("%s not support\n", __FUNCTION__);
    return -1;
}

static MI_S32 UVC_UP_FILE_FillBuffer(void *uvc,ST_UVC_BufInfo_t *bufInfo)
{
    ST_ERR("%s not support\n", __FUNCTION__);
    return -1;
}

static MI_S32 UVC_MM_FILE_FillBuffer(void *uvc, ST_UVC_BufInfo_t *bufInfo)
{
    ST_UvcDev_t *dev = Get_UVC_Device(uvc);

    MI_U8 *       u8CopyData = (MI_U8 *)bufInfo->b.buf;
    MI_U32 *      pu32length = (MI_U32 *)&bufInfo->length;
    MI_U32        ret        = 0;
    MI_U32        width      = dev->setting.u32Width;
    MI_U32        height     = dev->setting.u32Height;

    long int      tm;
    static struct timeval tv_before = {0};
    struct timeval        tv_after;

    if (!dev)
        goto err;

    if (!g_stUvcFile[dev->dev_index].imgFd)
        goto err;


    switch (dev->setting.fcc)
    {
        case V4L2_PIX_FMT_YUYV:
            *pu32length = width * height * 2;
            ret = 0;
            ret         = read(g_stUvcFile[dev->dev_index].imgFd, u8CopyData, *pu32length);
            if (ret == *pu32length)
            {
                g_stUvcFile[dev->dev_index].cur_offset += ret;
            }
            else if (0 == ret)
            {
                lseek(g_stUvcFile[dev->dev_index].imgFd, 0, SEEK_SET);
            }
            else
            {
                lseek(g_stUvcFile[dev->dev_index].imgFd, g_stUvcFile[dev->dev_index].cur_offset, SEEK_SET);
            }
            break;
        case V4L2_PIX_FMT_NV12:
            *pu32length = width * height * 3 / 2;
            ret         = read(g_stUvcFile[dev->dev_index].imgFd, u8CopyData, *pu32length);
            if (ret == *pu32length)
            {
                g_stUvcFile[dev->dev_index].cur_offset += ret;
            }
            else if (0 == ret)
            {
                lseek(g_stUvcFile[dev->dev_index].imgFd, 0, SEEK_SET);
            }
            else
            {
                lseek(g_stUvcFile[dev->dev_index].imgFd, g_stUvcFile[dev->dev_index].cur_offset, SEEK_SET);
            }
            break;
        case V4L2_PIX_FMT_H264:
        case V4L2_PIX_FMT_H265:
        case V4L2_PIX_FMT_MJPEG:
            if (g_stUvcFile[dev->dev_index].cur_frame < g_stUvcFile[dev->dev_index].arrLen)
            {
                *pu32length = g_stUvcFile[dev->dev_index].frameArr[g_stUvcFile[dev->dev_index].cur_frame] - g_stUvcFile[dev->dev_index].frameArr[g_stUvcFile[dev->dev_index].cur_frame - 1];
                ret         = read(g_stUvcFile[dev->dev_index].imgFd, u8CopyData, *pu32length);
                if (ret == *pu32length)
                {
                    g_stUvcFile[dev->dev_index].cur_frame++;
                    g_stUvcFile[dev->dev_index].cur_offset += ret;
                }
                else
                {
                    lseek(g_stUvcFile[dev->dev_index].imgFd, g_stUvcFile[dev->dev_index].cur_offset, SEEK_SET);
                }
            }
            else
            {
                g_stUvcFile[dev->dev_index].cur_frame = 1;
                lseek(g_stUvcFile[dev->dev_index].imgFd, 0, SEEK_SET);
            }
            break;
        default:
            printf("unknown format %d\n", dev->setting.fcc);
            goto err;
    }

    gettimeofday(&tv_after, NULL);
    tm = ((tv_after.tv_sec - tv_before.tv_sec) * 1000 * 1000 + (tv_after.tv_usec - tv_before.tv_usec));
    if (tm > 0 && tm < 33 * 1000)
    {
        usleep(33 * 1000 - tm);
    }
    tv_before = tv_after;

    return MI_SUCCESS;
err:
    return -EINVAL;
}

static MI_S32 UVC_StartReadFile(void *uvc, Stream_Params_t format)
{
    ST_UvcDev_t *dev = Get_UVC_Device(uvc);
    char         img_path[256], EsFileEnd[20], str4num[10];
    MI_U32       fcc          = format.fcc;
    MI_U32       u32Width     = format.width;
    MI_U32       u32Height    = format.height;
    MI_U32       u32FrameRate = format.frameRate;
    char         nal[4]       = {0x00, 0x00, 0x00, 0x01};
    char         jiff[4]      = {0xff, 0xd8, 0xff, 0xe0};

    if (!dev)
        return -1;

    memset(&dev->setting, 0x00, sizeof(dev->setting));
    dev->setting.fcc          = format.fcc;
    dev->setting.u32Width     = format.width;
    dev->setting.u32Height    = format.height;
    dev->setting.u32FrameRate = format.frameRate;
    printf("%s, width:%d, height:%d FrameRate:%d\n", __func__, u32Width, u32Height, u32FrameRate);

    memset(img_path, 0x00, sizeof(img_path));
    strcat(img_path, g_stUvcFile[dev->dev_index].filePath);
    strcat(img_path, "/img/");
    memset(str4num, 0x00, sizeof(str4num));
    sprintf(str4num, "%d", u32Width);
    strcat(img_path, str4num);
    strcat(img_path, "_");
    memset(str4num, 0x00, sizeof(str4num));
    sprintf(str4num, "%d", u32Height);
    strcat(img_path, str4num);

    memset(EsFileEnd, 0x00, sizeof(EsFileEnd));
    switch (fcc)
    {
        case V4L2_PIX_FMT_H264:
            strcat(EsFileEnd, "_h264.es");
            break;
        case V4L2_PIX_FMT_H265:
            strcat(EsFileEnd, "_h265.es");
            break;
        case V4L2_PIX_FMT_MJPEG:
            strcat(EsFileEnd, "_jpeg.es");
            break;
        case V4L2_PIX_FMT_NV12:
            printf("V4L2_PIX_FMT_NV12\n");
            strcat(EsFileEnd, "_NV12.yuv");
            break;
        case V4L2_PIX_FMT_YUYV:
            printf("V4L2_PIX_FMT_YUYV\n");
            strcat(EsFileEnd, "_YUYV.yuv");
            break;
        default:
            return -EINVAL;
    }

    strcat(img_path, EsFileEnd);
    printf("img_path: %s \n", img_path);

    g_stUvcFile[dev->dev_index].arrLen = MAX_FRAME;
    g_stUvcFile[dev->dev_index].cur_frame = 1;
    g_stUvcFile[dev->dev_index].cur_offset = 0;

    g_stUvcFile[dev->dev_index].imgFd = open(img_path, O_RDONLY);
    if (!g_stUvcFile[dev->dev_index].imgFd)
    {
        printf("open %s error\n", img_path);
        goto err;
    }

    if (fcc == V4L2_PIX_FMT_H265 || fcc == V4L2_PIX_FMT_H264 || fcc == V4L2_PIX_FMT_MJPEG)
    {
        g_stUvcFile[dev->dev_index].frameArr = (MI_U32 *)malloc(MAX_FRAME * sizeof(unsigned int));
        if (NULL == g_stUvcFile[dev->dev_index].frameArr)
        {
            printf("no mem for frameArr\n");
            goto err;
        }

        if (getFramePos(g_stUvcFile[dev->dev_index].imgFd, fcc == V4L2_PIX_FMT_MJPEG ? jiff : nal, 4, g_stUvcFile[dev->dev_index].frameArr, &g_stUvcFile[dev->dev_index].arrLen))
        {
            printf("pares %s error\n", img_path);
            goto err;
        }
    }

    return MI_SUCCESS;

err:
    if (g_stUvcFile[dev->dev_index].frameArr)
    {
        free(g_stUvcFile[dev->dev_index].frameArr);
        g_stUvcFile[dev->dev_index].frameArr = NULL;
    }
    return -1;
}

static MI_S32 UVC_StopReadFile(void *uvc)
{
    ST_UvcDev_t *dev = Get_UVC_Device(uvc);
    if (!dev)
        return -1;

    if (g_stUvcFile[dev->dev_index].imgFd > 0)
    {
        close(g_stUvcFile[dev->dev_index].imgFd);
        g_stUvcFile[dev->dev_index].imgFd = -1;
    }

    if (g_stUvcFile[dev->dev_index].frameArr)
    {
        free(g_stUvcFile[dev->dev_index].frameArr);
        g_stUvcFile[dev->dev_index].frameArr = NULL;
    }

    g_stUvcFile[dev->dev_index].arrLen = MAX_FRAME;
    g_stUvcFile[dev->dev_index].cur_frame = 1;
    g_stUvcFile[dev->dev_index].cur_offset = 0;

    return MI_SUCCESS;
}

static MI_S32 UVC_UP_FinishBuffer(void *uvc,ST_UVC_BufInfo_t *bufInfo)
{
    ST_UvcDev_t *dev = Get_UVC_Device(uvc);
    MI_S32 s32Ret = MI_SUCCESS;
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_SYS_BUF_HANDLE stBufHandle = NULL;
    VENC_STREAMS_t * pUserptrStream = NULL;
    MI_U32 VencChnId = pstStreamAttr[dev->dev_index].u32VencChn;
    MI_U32 VencDevId;

    if (!dev)
        return -1;

    switch(dev->setting.fcc)
    {
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_NV12:
            stBufHandle = bufInfo->b.handle;

            s32Ret = MI_SYS_ChnOutputPortPutBuf(stBufHandle);
            if(MI_SUCCESS!=s32Ret)
            {
                printf("%s Release Frame Failed\n", __func__);
                return s32Ret;
            }
            break;

        case V4L2_PIX_FMT_MJPEG:
        case V4L2_PIX_FMT_H264:
        case V4L2_PIX_FMT_H265:
            VencDevId = (dev->setting.fcc == V4L2_PIX_FMT_MJPEG) ? MI_VENC_DEV_ID_JPEG_0 : MI_VENC_DEV_ID_H264_H265_0;
            pUserptrStream = (VENC_STREAMS_t*)bufInfo->b.handle;

            s32Ret = MI_VENC_ReleaseStream(VencDevId, VencChnId, &pUserptrStream->stStream);
            if (MI_SUCCESS != s32Ret)
            {
                printf("%s Release Frame Failed\n", __func__);
                return s32Ret;
            }

            pUserptrStream->used = FALSE;
            break;
    }

    return MI_SUCCESS;
}

static MI_S32 UVC_UP_FillBuffer(void *uvc,ST_UVC_BufInfo_t *bufInfo)
{
    ST_UvcDev_t * dev = Get_UVC_Device(uvc);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    MI_SYS_ChnPort_t dstChnPort;
    VENC_STREAMS_t * pUserptrStream = NULL;
    MI_VENC_Stream_t * pstStream = NULL;
    MI_VENC_ChnStat_t stStat;
    MI_U32 VencChnId = pstStreamAttr[dev->dev_index].u32VencChn;
    MI_U32 VencDevId;

    if (!dev)
        return -1;

    dstChnPort = dev->setting.dstChnPort;
    switch(dev->setting.fcc)
    {
        case V4L2_PIX_FMT_YUYV:
            memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
            memset(&stBufHandle, 0, sizeof(MI_SYS_BUF_HANDLE));

            s32Ret = MI_SYS_ChnOutputPortGetBuf(&dstChnPort, &stBufInfo, &stBufHandle);
            if(MI_SUCCESS!=s32Ret)
                return -EINVAL;

            bufInfo->b.start = (long unsigned int)stBufInfo.stFrameData.pVirAddr[0];
            bufInfo->length = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            bufInfo->b.handle = (long unsigned int)stBufHandle;
            break;

        case V4L2_PIX_FMT_NV12:
            memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
            memset(&stBufHandle, 0, sizeof(MI_SYS_BUF_HANDLE));

            s32Ret = MI_SYS_ChnOutputPortGetBuf(&dstChnPort, &stBufInfo, &stBufHandle);
            if(MI_SUCCESS!=s32Ret)
                return -EINVAL;

            bufInfo->b.start = (long unsigned int)stBufInfo.stFrameData.pVirAddr[0];
            bufInfo->length = stBufInfo.stFrameData.u16Height
                     * (stBufInfo.stFrameData.u32Stride[0] + stBufInfo.stFrameData.u32Stride[1] / 2);
            bufInfo->b.handle = (long unsigned int)stBufHandle;
            break;

        case V4L2_PIX_FMT_MJPEG:
        case V4L2_PIX_FMT_H264:
        case V4L2_PIX_FMT_H265:
            VencDevId = (dev->setting.fcc == V4L2_PIX_FMT_MJPEG) ? MI_VENC_DEV_ID_JPEG_0 : MI_VENC_DEV_ID_H264_H265_0;
            for (int i = 0;i < g_maxbuf_cnt; i++)
            {
                if (!dev->res.pstuserptr_stream[i].used)
                {
                    pUserptrStream = &dev->res.pstuserptr_stream[i];
                    break;
                }
            }
            if (!pUserptrStream)
            {
                return -EINVAL;
            }
            pstStream = &pUserptrStream->stStream;
            memset(pstStream->pstPack, 0, sizeof(MI_VENC_Pack_t) * 4);

            s32Ret = MI_VENC_Query(VencDevId, VencChnId, &stStat);
            if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
                return -EINVAL;

            pstStream->u32PackCount = 1; //only need 1 packet

            s32Ret = MI_VENC_GetStream(VencDevId, VencChnId, pstStream, 40);
            if (MI_SUCCESS != s32Ret)
                return -EINVAL;
            if (((dev->setting.fcc == V4L2_PIX_FMT_H264) && (pstStream->stH264Info.eRefType == E_MI_VENC_BASE_IDR)) ||
                ((dev->setting.fcc == V4L2_PIX_FMT_H265) && (pstStream->stH265Info.eRefType == E_MI_VENC_BASE_IDR))) {
                bufInfo->is_keyframe = TRUE;
            }
            else {
                bufInfo->is_keyframe = FALSE;
            }

            bufInfo->b.start = (long unsigned int)pstStream->pstPack[0].pu8Addr;
            bufInfo->length = pstStream->pstPack[0].u32Len;
            bufInfo->b.handle = (long unsigned int)pUserptrStream;
            pUserptrStream->used = TRUE;

            if (pstStreamAttr[dev->dev_index].bForceIdr && dev->setting.fcc!=V4L2_PIX_FMT_MJPEG)
            {
                pstStreamAttr[dev->dev_index].bForceIdr = FALSE;
                s32Ret = MI_VENC_RequestIdr(VencDevId, VencChnId, TRUE);
                if (MI_SUCCESS != s32Ret)
                    printf("MI_VENC_RequestIdr failed:%d\n", s32Ret);
            }
            break;

        default:
            return -EINVAL;
    }

    return MI_SUCCESS;
}

static MI_S32 UVC_MM_FillBuffer(void *uvc,ST_UVC_BufInfo_t *bufInfo)
{
    ST_UvcDev_t * dev = Get_UVC_Device(uvc);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 u32Size, i;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    MI_SYS_ChnPort_t dstChnPort;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack[4];
    MI_VENC_ChnStat_t stStat;

    MI_U8 *u8CopyData = (MI_U8 *)bufInfo->b.buf;
    MI_U32 *pu32length = (MI_U32 *)&bufInfo->length;
    MI_U32 VencChnId = pstStreamAttr[dev->dev_index].u32VencChn;
    MI_U32 VencDevId;

    if (!dev)
        return -1;

    dstChnPort = dev->setting.dstChnPort;

    switch(dev->setting.fcc)
    {
        case V4L2_PIX_FMT_YUYV:
            memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
            memset(&stBufHandle, 0, sizeof(MI_SYS_BUF_HANDLE));

            s32Ret = MI_SYS_ChnOutputPortGetBuf(&dstChnPort, &stBufInfo, &stBufHandle);
            if(MI_SUCCESS!=s32Ret)
            {
                return -EINVAL;
            }

            *pu32length = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[0], *pu32length);

            s32Ret = MI_SYS_ChnOutputPortPutBuf(stBufHandle);
            if(MI_SUCCESS!=s32Ret)
                printf("%s Release Frame Failed\n", __func__);
            break;

        case V4L2_PIX_FMT_NV12:
            memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
            memset(&stBufHandle, 0, sizeof(MI_SYS_BUF_HANDLE));

            s32Ret = MI_SYS_ChnOutputPortGetBuf(&dstChnPort, &stBufInfo, &stBufHandle);
            if(MI_SUCCESS!=s32Ret)
                return -EINVAL;

            *pu32length = stBufInfo.stFrameData.u16Height
                    * (stBufInfo.stFrameData.u32Stride[0] + stBufInfo.stFrameData.u32Stride[1] / 2);
            memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[0],
                            stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0]);
            u8CopyData += stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[1],
                            stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[1]/2);

            s32Ret = MI_SYS_ChnOutputPortPutBuf(stBufHandle);
            if(MI_SUCCESS!=s32Ret)
                printf("%s Release Frame Failed\n", __func__);
            break;

        case V4L2_PIX_FMT_MJPEG:
        case V4L2_PIX_FMT_H264:
        case V4L2_PIX_FMT_H265:
            VencDevId = (dev->setting.fcc == V4L2_PIX_FMT_MJPEG) ? MI_VENC_DEV_ID_JPEG_0 : MI_VENC_DEV_ID_H264_H265_0;
            memset(&stStream, 0, sizeof(MI_VENC_Stream_t));
            memset(&stPack, 0, sizeof(MI_VENC_Pack_t) * 4);
            stStream.pstPack = stPack;

            s32Ret = MI_VENC_Query(VencDevId, VencChnId, &stStat);
            if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
                return -EINVAL;

            stStream.u32PackCount = stStat.u32CurPacks;

            s32Ret = MI_VENC_GetStream(VencDevId, VencChnId, &stStream, 40);
            if (MI_SUCCESS != s32Ret)
                return -EINVAL;
            if (((dev->setting.fcc == V4L2_PIX_FMT_H264) && (stStream.stH264Info.eRefType == E_MI_VENC_BASE_IDR)) ||
                ((dev->setting.fcc == V4L2_PIX_FMT_H265) && (stStream.stH265Info.eRefType == E_MI_VENC_BASE_IDR))) {
                bufInfo->is_keyframe = TRUE;
            }
            else {
                bufInfo->is_keyframe = FALSE;
            }

            for(i = 0; i < stStat.u32CurPacks; i++)
            {
                u32Size = stStream.pstPack[i].u32Len;
                memcpy(u8CopyData,stStream.pstPack[i].pu8Addr, u32Size);
                u8CopyData += u32Size;
            }
            *pu32length = u8CopyData - (MI_U8 *)bufInfo->b.buf;
            bufInfo->is_tail = TRUE; //default is frameEnd

            s32Ret = MI_VENC_ReleaseStream(VencDevId, VencChnId, &stStream);
            if (MI_SUCCESS != s32Ret)
                printf("%s Release Frame Failed\n", __func__);

            if (pstStreamAttr[dev->dev_index].bForceIdr && dev->setting.fcc!=V4L2_PIX_FMT_MJPEG)
            {
                pstStreamAttr[dev->dev_index].bForceIdr = FALSE;
                s32Ret = MI_VENC_RequestIdr(VencDevId, VencChnId, TRUE);
                if (MI_SUCCESS != s32Ret)
                    printf("MI_VENC_RequestIdr failed:0x%x\n", s32Ret);
            }
            break;

        default:
            printf("unknown format %d\n", dev->setting.fcc);
            return -EINVAL;
    }

    return MI_SUCCESS;
}

static MI_S32 UVC_StartCapture(void *uvc,Stream_Params_t format)
{
    /************************************************
    Step0:  Init General Param
    *************************************************/
    ST_UvcDev_t *dev = Get_UVC_Device(uvc);
    if (!dev)
        return -1;

    memset(&dev->setting, 0x00, sizeof(dev->setting));
    dev->setting.fcc = format.fcc;
    dev->setting.u32Width = format.width;
    dev->setting.u32Height = format.height;
    dev->setting.u32FrameRate = format.frameRate;

    MI_U32 fcc = dev->setting.fcc;
    MI_U32 u32Width = dev->setting.u32Width;
    MI_U32 u32Height = dev->setting.u32Height;
    MI_U32 u32FrameRate = dev->setting.u32FrameRate;

    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_SYS_ChnPort_t *dstChnPort = &dev->setting.dstChnPort;

    /************************************************
    Step1:  Init Isp Output Param
    *************************************************/
    MI_ISP_DEV IspDevId = (MI_ISP_DEV)pstStreamAttr[dev->dev_index].u32IspDev;
    MI_ISP_CHANNEL IspChnId = (MI_ISP_CHANNEL)pstStreamAttr[dev->dev_index].u32IspChn;
    MI_ISP_PORT  IspPortId = (MI_ISP_PORT)pstStreamAttr[dev->dev_index].u32IspPort;

    MI_ISP_OutPortParam_t  stIspOutputParam;
    memset(&stIspOutputParam, 0x0, sizeof(MI_ISP_OutPortParam_t));
    STCHECKRESULT(MI_ISP_GetInputPortCrop(IspDevId, IspChnId, &stIspOutputParam.stCropRect));

    /************************************************
    Step2:  Init Scl Output Param
    *************************************************/
    MI_SCL_DEV SclDevId = (MI_SCL_DEV)pstStreamAttr[dev->dev_index].u32SclDev;
    MI_SCL_CHANNEL SclChnId = (MI_SCL_CHANNEL)pstStreamAttr[dev->dev_index].u32SclChn;
    MI_SCL_PORT SclPortId = (MI_SCL_PORT)pstStreamAttr[dev->dev_index].u32SclPort;

    MI_SCL_OutPortParam_t  stSclOutputParam;
    memset(&stSclOutputParam, 0x0, sizeof(MI_SCL_OutPortParam_t));
    stSclOutputParam.stSCLOutCropRect.u16X = stIspOutputParam.stCropRect.u16X;
    stSclOutputParam.stSCLOutCropRect.u16Y = stIspOutputParam.stCropRect.u16Y;
    stSclOutputParam.stSCLOutCropRect.u16Width = stIspOutputParam.stCropRect.u16Width;
    stSclOutputParam.stSCLOutCropRect.u16Height = stIspOutputParam.stCropRect.u16Height;
    stSclOutputParam.stSCLOutputSize.u16Width = u32Width;
    stSclOutputParam.stSCLOutputSize.u16Height = u32Height;
    stSclOutputParam.bMirror = FALSE;
    stSclOutputParam.bFlip = FALSE;
    stSclOutputParam.eCompressMode= E_MI_SYS_COMPRESS_MODE_NONE;

    /************************************************
    Step3:  Init Venc Param
    *************************************************/
    MI_VENC_DEV VencDevId;
    MI_VENC_CHN VencChnId = (MI_VENC_CHN)pstStreamAttr[dev->dev_index].u32VencChn;

    MI_VENC_InitParam_t stInitParam;
    MI_VENC_ChnAttr_t stVencChnAttr;
    MI_VENC_InputSourceConfig_t stVencInputSourceConfig;
    memset(&stVencChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    memset(&stVencInputSourceConfig, 0, sizeof(MI_VENC_InputSourceConfig_t));

    MI_U32 u32VenBitRate;
    MI_U32 u32VenQfactor;
    bool bByFrame = TRUE;

    /************************************************
    Step4:  Init Bind Param
    *************************************************/
    Sys_BindInfo_T stBindInfo[2];
    MI_SYS_ChnPort_t stChnPort[2];

    memset(&stBindInfo[0], 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo[0].stSrcChnPort.eModId = E_MI_MODULE_ID_ISP;
    stBindInfo[0].stSrcChnPort.u32DevId = IspDevId;
    stBindInfo[0].stSrcChnPort.u32ChnId = IspChnId;
    stBindInfo[0].stSrcChnPort.u32PortId = IspPortId;
    stBindInfo[0].stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo[0].stDstChnPort.u32DevId = SclDevId;
    stBindInfo[0].stDstChnPort.u32ChnId = SclChnId;
    stBindInfo[0].u32SrcFrmrate = g_stConfig.u32MaxFps;
    stBindInfo[0].u32DstFrmrate = u32FrameRate;
    stBindInfo[0].eBindType = (!g_bEnableHDMI && g_device_num == 1) ? E_MI_SYS_BIND_TYPE_REALTIME : E_MI_SYS_BIND_TYPE_FRAME_BASE;

    memset(&stBindInfo[1], 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo[1].stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo[1].stSrcChnPort.u32DevId = SclDevId;
    stBindInfo[1].stSrcChnPort.u32ChnId = SclChnId;
    stBindInfo[1].stSrcChnPort.u32PortId = SclPortId;
    stBindInfo[1].stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo[1].stDstChnPort.u32ChnId = VencChnId;
    stBindInfo[1].u32SrcFrmrate = u32FrameRate;
    stBindInfo[1].u32DstFrmrate = u32FrameRate;
    if(!g_bEnableHDMI && g_device_num == 1)
    {
        if(fcc == V4L2_PIX_FMT_MJPEG)
            stBindInfo[1].eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
        else if(fcc == V4L2_PIX_FMT_H264 || fcc == V4L2_PIX_FMT_H265)
            stBindInfo[1].eBindType = E_MI_SYS_BIND_TYPE_HW_RING;
    }
    else
    {
        stBindInfo[1].eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    }

    memset(&stChnPort[0], 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort[0].eModId = E_MI_MODULE_ID_ISP;
    stChnPort[0].u32DevId = IspDevId;
    stChnPort[0].u32ChnId = IspChnId;
    stChnPort[0].u32PortId = IspPortId;

    memset(&stChnPort[1], 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort[1].eModId = E_MI_MODULE_ID_SCL;
    stChnPort[1].u32DevId = SclDevId;
    stChnPort[1].u32ChnId = SclChnId;
    stChnPort[1].u32PortId = SclPortId;

    /************************************************
    Step5:  Init User Param
    *************************************************/
    if (u32Width * u32Height > 2560 *1440)
    {
        u32VenBitRate = 1024 * 1024 * 8;
    }
    else if (u32Width * u32Height >= 1920 *1080)
    {
        u32VenBitRate = 1024 * 1024 * 4;
    }
    else if(u32Width * u32Height < 640*480)
    {
        u32VenBitRate = 1024 * 500;
    }
    else
    {
        u32VenBitRate = 1024 * 1024 * 2;
    }

    if (g_bitrate[dev->dev_index])
    {
        u32VenBitRate = g_bitrate[dev->dev_index] * 1024 * 1024;
    }

    if (g_qfactor[dev->dev_index])
    {
        u32VenQfactor = g_qfactor[dev->dev_index];
    }
    else
    {
        u32VenQfactor = 80;
    }

    if (!dev->res.pstuserptr_stream)
    {
        dev->res.pstuserptr_stream = (VENC_STREAMS_t*)calloc(g_maxbuf_cnt, sizeof(VENC_STREAMS_t));
        for(int i = 0; i < g_maxbuf_cnt; i++)
        {
            dev->res.pstuserptr_stream[i].stStream.pstPack = (MI_VENC_Pack_t*)calloc(4, sizeof(MI_VENC_Pack_t));
        }
    }

    /************************************************
    Step5:  Start
    *************************************************/
    switch(fcc)
    {
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_NV12:
            stIspOutputParam.ePixelFormat = (fcc == V4L2_PIX_FMT_YUYV) ? E_MI_SYS_PIXEL_FRAME_YUV422_YUYV : E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            stSclOutputParam.ePixelFormat = (fcc == V4L2_PIX_FMT_YUYV) ? E_MI_SYS_PIXEL_FRAME_YUV422_YUYV : E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

            STCHECKRESULT(MI_ISP_SetOutputPortParam(IspDevId, IspChnId, IspPortId, &stIspOutputParam));
            STCHECKRESULT(MI_SCL_SetOutputPortParam(SclDevId, SclChnId, SclPortId, &stSclOutputParam));

            STCHECKRESULT(bind_port(&stBindInfo[0]));
            STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(0, &stChnPort[0] , 0, g_maxbuf_cnt+2));
            STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(0, &stChnPort[1] , g_maxbuf_cnt+1, g_maxbuf_cnt+2));
            STCHECKRESULT(MI_ISP_EnableOutputPort(IspDevId, IspChnId, IspPortId));
            STCHECKRESULT(MI_SCL_EnableOutputPort(SclDevId, SclChnId, SclPortId));
            *dstChnPort = stBindInfo[0].stDstChnPort;
            break;

        case V4L2_PIX_FMT_MJPEG:
        case V4L2_PIX_FMT_H264:
        case V4L2_PIX_FMT_H265:
            if(!g_bEnableHDMI && g_device_num == 1 && fcc == V4L2_PIX_FMT_MJPEG)
            {
                stIspOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                stSclOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            }
            else
            {
                stIspOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
                stSclOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            }

            STCHECKRESULT(MI_SCL_SetOutputPortParam(SclDevId, SclChnId, SclPortId, &stSclOutputParam));
            STCHECKRESULT(MI_ISP_SetOutputPortParam(IspDevId, IspChnId, IspPortId, &stIspOutputParam));

            VencDevId = (fcc == V4L2_PIX_FMT_MJPEG) ? MI_VENC_DEV_ID_JPEG_0 : MI_VENC_DEV_ID_H264_H265_0;
            stBindInfo[1].stDstChnPort.u32DevId = VencDevId;

            memset(&stInitParam, 0, sizeof(MI_VENC_InitParam_t));
            stInitParam.u32MaxWidth = ALIGN_UP(1920, ALIGN_NUM);
            stInitParam.u32MaxHeight = ALIGN_UP(1080, ALIGN_NUM);
            ExecFunc(MI_VENC_CreateDev(VencDevId, &stInitParam), MI_SUCCESS);

            if(fcc == V4L2_PIX_FMT_MJPEG)
            {
                stVencChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = u32Width;
                stVencChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = u32Height;
                stVencChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth =  u32Width;
                stVencChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = ALIGN_UP(u32Height, 16);
                stVencChnAttr.stVeAttr.stAttrJpeg.bByFrame = bByFrame;
                stVencChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
                stVencChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
                stVencChnAttr.stRcAttr.stAttrMjpegFixQp.u32Qfactor = u32VenQfactor;
                stVencChnAttr.stRcAttr.stAttrMjpegFixQp.u32SrcFrmRateNum = u32FrameRate;
                stVencChnAttr.stRcAttr.stAttrMjpegFixQp.u32SrcFrmRateDen = 1;
            }
            else if(fcc == V4L2_PIX_FMT_H264)
            {
                stVencChnAttr.stVeAttr.stAttrH264e.u32PicWidth = u32Width;
                stVencChnAttr.stVeAttr.stAttrH264e.u32PicHeight = u32Height;
                stVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = u32Width;
                stVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = u32Height;
                stVencChnAttr.stVeAttr.stAttrH264e.bByFrame = bByFrame;
                stVencChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2;
                stVencChnAttr.stVeAttr.stAttrH264e.u32Profile = 1;
                stVencChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
                stVencChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
                stVencChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = u32VenBitRate;
                stVencChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = u32FrameRate;
                stVencChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
                stVencChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
                stVencChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
                stVencChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
            }
            else if(fcc == V4L2_PIX_FMT_H265)
            {
                stVencChnAttr.stVeAttr.stAttrH265e.u32PicWidth = u32Width;
                stVencChnAttr.stVeAttr.stAttrH265e.u32PicHeight = u32Height;
                stVencChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = u32Width;
                stVencChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = u32Height;
                stVencChnAttr.stVeAttr.stAttrH265e.bByFrame = bByFrame;
                stVencChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
                stVencChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265CBR;
                stVencChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = u32VenBitRate;
                stVencChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = u32FrameRate;
                stVencChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
                stVencChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = 30;
                stVencChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
                stVencChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
            }

            STCHECKRESULT(MI_VENC_CreateChn(VencDevId, VencChnId, &stVencChnAttr));
            if(fcc == V4L2_PIX_FMT_H264)
            {
                /* Ring Mode don't support SliceSplict */
                if(!g_bEnableHDMI && g_device_num == 1)
                {
                    stVencInputSourceConfig.eInputSrcBufferMode = E_MI_VENC_INPUT_MODE_RING_UNIFIED_DMA;//E_MI_VENC_INPUT_MODE_RING_ONE_FRM;
                    stBindInfo[1].u32BindParam = u32Height;
                }
                else
                {
                    stVencInputSourceConfig.eInputSrcBufferMode = E_MI_VENC_INPUT_MODE_NORMAL_FRMBASE;
                    MI_VENC_ParamH264SliceSplit_t stSliceSplit = {TRUE, 0xF};
                    STCHECKRESULT(MI_VENC_SetH264SliceSplit(VencDevId, VencChnId, &stSliceSplit));
                }
                STCHECKRESULT(MI_VENC_SetInputSourceConfig(VencDevId, VencChnId, &stVencInputSourceConfig));
                if(!g_bEnableHDMI && g_device_num == 1)
                {                
                    MI_SYS_GlobalPrivPoolConfig_t stConfig;
                    memset(&stConfig, 0 , sizeof(MI_SYS_GlobalPrivPoolConfig_t));
                    stConfig.eConfigType = E_MI_SYS_PER_DEV_PRIVATE_RING_POOL;
                    stConfig.bCreate = TRUE;
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.eModule = E_MI_MODULE_ID_SCL;
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u32Devid =  SclDevId;
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u16MaxWidth = ALIGN_BACK(1920, ALIGN_NUM);
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u16MaxHeight = ALIGN_BACK(1080, ALIGN_NUM);
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u16RingLine = ALIGN_BACK(1080 / 2, ALIGN_NUM);
                    strcpy((char*)stConfig.uConfig.stpreDevPrivRingPoolConfig.u8MMAHeapName,"mma_heap_name0");
                    ExecFunc(MI_SYS_ConfigPrivateMMAPool(0, &stConfig), MI_SUCCESS); //设置前端SCL的private ring pool
                    //printf("create scl ring pool\n");

                    memset(&stConfig, 0 , sizeof(MI_SYS_GlobalPrivPoolConfig_t));
                    stConfig.eConfigType = E_MI_SYS_PER_DEV_PRIVATE_RING_POOL;
                    stConfig.bCreate = TRUE;
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.eModule = E_MI_MODULE_ID_VENC;
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u32Devid =  VencDevId;
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u16MaxWidth = ALIGN_BACK(1920, ALIGN_NUM);
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u16MaxHeight = ALIGN_BACK(1080, ALIGN_NUM);
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u16RingLine = ALIGN_BACK(1080 / 2, ALIGN_NUM);
                    strcpy((char*)stConfig.uConfig.stpreDevPrivRingPoolConfig.u8MMAHeapName,"mma_heap_name0");
                    ExecFunc(MI_SYS_ConfigPrivateMMAPool(0, &stConfig), MI_SUCCESS); //设置前端SCL的private ring pool
                    //printf("create venc ring pool\n");
                }
            }
            else if(fcc == V4L2_PIX_FMT_H265)
            {
                /* Ring Mode don't support SliceSplict */
                if(!g_bEnableHDMI && g_device_num == 1)
                {
                    stVencInputSourceConfig.eInputSrcBufferMode = E_MI_VENC_INPUT_MODE_RING_UNIFIED_DMA;//E_MI_VENC_INPUT_MODE_RING_ONE_FRM;
                    stBindInfo[1].u32BindParam = u32Height;
                }
                else
                {
                    stVencInputSourceConfig.eInputSrcBufferMode = E_MI_VENC_INPUT_MODE_NORMAL_FRMBASE;
                    MI_VENC_ParamH265SliceSplit_t stSliceSplit = {TRUE, (u32Height+31)/32};
                    STCHECKRESULT(MI_VENC_SetH265SliceSplit(VencDevId, VencChnId, &stSliceSplit));
                }
                STCHECKRESULT(MI_VENC_SetInputSourceConfig(VencDevId, VencChnId, &stVencInputSourceConfig));
                if(!g_bEnableHDMI && g_device_num == 1)
                {                
                    MI_SYS_GlobalPrivPoolConfig_t stConfig;
                    memset(&stConfig, 0 , sizeof(MI_SYS_GlobalPrivPoolConfig_t));
                    stConfig.eConfigType = E_MI_SYS_PER_DEV_PRIVATE_RING_POOL;
                    stConfig.bCreate = TRUE;
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.eModule = E_MI_MODULE_ID_SCL;
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u32Devid =  SclDevId;
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u16MaxWidth = ALIGN_BACK(1920, ALIGN_NUM);
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u16MaxHeight = ALIGN_BACK(1080, ALIGN_NUM);
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u16RingLine = ALIGN_BACK(1080 / 2, ALIGN_NUM);
                    strcpy((char*)stConfig.uConfig.stpreDevPrivRingPoolConfig.u8MMAHeapName,"mma_heap_name0");
                    ExecFunc(MI_SYS_ConfigPrivateMMAPool(0, &stConfig), MI_SUCCESS); //设置前端SCL的private ring pool
                    //printf("create scl ring pool\n");

                    memset(&stConfig, 0 , sizeof(MI_SYS_GlobalPrivPoolConfig_t));
                    stConfig.eConfigType = E_MI_SYS_PER_DEV_PRIVATE_RING_POOL;
                    stConfig.bCreate = TRUE;
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.eModule = E_MI_MODULE_ID_VENC;
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u32Devid =  VencDevId;
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u16MaxWidth = ALIGN_BACK(1920, ALIGN_NUM);
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u16MaxHeight = ALIGN_BACK(1080, ALIGN_NUM);
                    stConfig.uConfig.stpreDevPrivRingPoolConfig.u16RingLine = ALIGN_BACK(1080 / 2, ALIGN_NUM);
                    strcpy((char*)stConfig.uConfig.stpreDevPrivRingPoolConfig.u8MMAHeapName,"mma_heap_name0");
                    ExecFunc(MI_SYS_ConfigPrivateMMAPool(0, &stConfig), MI_SUCCESS); //设置前端SCL的private ring pool
                    //printf("create venc ring pool\n");
                }

            }
            STCHECKRESULT(MI_VENC_SetMaxStreamCnt(VencDevId, VencChnId, g_maxbuf_cnt+1));
            STCHECKRESULT(bind_port(&stBindInfo[0]));
            STCHECKRESULT(bind_port(&stBindInfo[1]));
            STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(0, &stChnPort[0] , 0, g_maxbuf_cnt+2));
            STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(0, &stChnPort[1] , 0, g_maxbuf_cnt+2));
            STCHECKRESULT(MI_ISP_EnableOutputPort(IspDevId, IspChnId, IspPortId));
            STCHECKRESULT(MI_SCL_EnableOutputPort(SclDevId, SclChnId, SclPortId));
            STCHECKRESULT(MI_VENC_StartRecvPic(VencDevId, VencChnId));
            *dstChnPort = stBindInfo[1].stDstChnPort;
            break;

        default:
            return -EINVAL;
    }

    if(g_load_iq_bin)
    {
        ST_DoSetIqBin(IspDevId, IspChnId, g_IspBinPath);
        g_load_iq_bin = 0;
    }

    g_bStartCapture = TRUE;

    printf("Capture u32Width: %d, u32height: %d, format: %s\n",u32Width,u32Height,
        fcc==V4L2_PIX_FMT_YUYV ? "YUYV":(fcc==V4L2_PIX_FMT_NV12 ? "NV12":
        (fcc==V4L2_PIX_FMT_MJPEG ?"MJPEG":(fcc==V4L2_PIX_FMT_H264 ? "H264":"H265"))));

    return MI_SUCCESS;
}

static MI_S32 UVC_StopCapture(void *uvc)
{
    ST_UvcDev_t *dev = Get_UVC_Device(uvc);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;

    if (!dev)
        return -1;

    MI_ISP_DEV IspDevId = (MI_ISP_DEV)pstStreamAttr[dev->dev_index].u32IspDev;
    MI_ISP_CHANNEL IspChnId = (MI_ISP_CHANNEL)pstStreamAttr[dev->dev_index].u32IspChn;
    MI_ISP_PORT  IspPortId = (MI_ISP_PORT)pstStreamAttr[dev->dev_index].u32IspPort;

    MI_SCL_DEV SclDevId = (MI_SCL_DEV)pstStreamAttr[dev->dev_index].u32SclDev;
    MI_SCL_CHANNEL SclChnId = (MI_SCL_CHANNEL)pstStreamAttr[dev->dev_index].u32SclChn;
    MI_SCL_PORT SclPortId = (MI_SCL_PORT)pstStreamAttr[dev->dev_index].u32SclPort;

    MI_VENC_DEV VencDevId;
    MI_VENC_CHN VencChnId = (MI_VENC_CHN)pstStreamAttr[dev->dev_index].u32VencChn;

    /************************************************
    Step0:  General Param Set
    *************************************************/
    Sys_BindInfo_T stBindInfo[2];

    memset(&stBindInfo[0], 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo[0].stSrcChnPort.eModId = E_MI_MODULE_ID_ISP;
    stBindInfo[0].stSrcChnPort.u32DevId = IspDevId;
    stBindInfo[0].stSrcChnPort.u32ChnId = IspChnId;
    stBindInfo[0].stSrcChnPort.u32PortId = IspPortId;
    stBindInfo[0].stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo[0].stDstChnPort.u32DevId = SclDevId;
    stBindInfo[0].stDstChnPort.u32ChnId = SclChnId;
    stBindInfo[0].u32SrcFrmrate = g_stConfig.u32MaxFps;
    stBindInfo[0].u32DstFrmrate = dev->setting.u32FrameRate;
    stBindInfo[0].eBindType = (!g_bEnableHDMI && g_device_num == 1) ? E_MI_SYS_BIND_TYPE_REALTIME : E_MI_SYS_BIND_TYPE_FRAME_BASE;

    memset(&stBindInfo[1], 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo[1].stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo[1].stSrcChnPort.u32DevId = SclDevId;
    stBindInfo[1].stSrcChnPort.u32ChnId = SclChnId;
    stBindInfo[1].stSrcChnPort.u32PortId = SclPortId;
    stBindInfo[1].stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo[1].stDstChnPort.u32ChnId = VencChnId;
    stBindInfo[1].u32SrcFrmrate = dev->setting.u32FrameRate;
    stBindInfo[1].u32DstFrmrate = dev->setting.u32FrameRate;
    if(!g_bEnableHDMI && g_device_num == 1)
    {
        if(dev->setting.fcc == V4L2_PIX_FMT_MJPEG)
            stBindInfo[1].eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
        else if(dev->setting.fcc == V4L2_PIX_FMT_H264 || dev->setting.fcc == V4L2_PIX_FMT_H265)
            stBindInfo[1].eBindType = E_MI_SYS_BIND_TYPE_HW_RING;
    }
    else
    {
        stBindInfo[1].eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    }

    if (dev->res.pstuserptr_stream)
    {
        for(int i = 0; i < g_maxbuf_cnt; i++)
        {
            free(dev->res.pstuserptr_stream[i].stStream.pstPack);
        }
        free(dev->res.pstuserptr_stream);
        dev->res.pstuserptr_stream = NULL;
    }

    /************************************************
    Step1:  Stop Port And Unbind
    *************************************************/
    switch(dev->setting.fcc)
    {
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_NV12:
            STCHECKRESULT(MI_SCL_DisableOutputPort(SclDevId, SclChnId, SclPortId));
            STCHECKRESULT(MI_ISP_DisableOutputPort(IspDevId, IspChnId, IspPortId));
            STCHECKRESULT(unbind_port(&stBindInfo[0]));
            break;

        case V4L2_PIX_FMT_MJPEG:
        case V4L2_PIX_FMT_H264:
        case V4L2_PIX_FMT_H265:
            VencDevId = (dev->setting.fcc == V4L2_PIX_FMT_MJPEG) ? MI_VENC_DEV_ID_JPEG_0 : MI_VENC_DEV_ID_H264_H265_0;
            stBindInfo[1].stDstChnPort.u32DevId = VencDevId;
            STCHECKRESULT(MI_VENC_StopRecvPic(VencDevId, VencChnId));
            if(dev->setting.fcc == V4L2_PIX_FMT_H264 || dev->setting.fcc == V4L2_PIX_FMT_H265)
            {
                MI_SYS_GlobalPrivPoolConfig_t stConfig;
                memset(&stConfig, 0 , sizeof(MI_SYS_GlobalPrivPoolConfig_t));
                stConfig.eConfigType = E_MI_SYS_PER_DEV_PRIVATE_RING_POOL;
                stConfig.bCreate = FALSE;
                stConfig.uConfig.stpreDevPrivRingPoolConfig.eModule = E_MI_MODULE_ID_SCL;
                stConfig.uConfig.stpreDevPrivRingPoolConfig.u32Devid =  SclDevId;
                ExecFunc(MI_SYS_ConfigPrivateMMAPool(0, &stConfig), MI_SUCCESS);
                //printf("destroy scl ring pool\n");

                memset(&stConfig, 0 , sizeof(MI_SYS_GlobalPrivPoolConfig_t));
                stConfig.eConfigType = E_MI_SYS_PER_DEV_PRIVATE_RING_POOL;
                stConfig.bCreate = FALSE;
                stConfig.uConfig.stpreDevPrivRingPoolConfig.eModule = E_MI_MODULE_ID_VENC;
                stConfig.uConfig.stpreDevPrivRingPoolConfig.u32Devid =  VencDevId;
                ExecFunc(MI_SYS_ConfigPrivateMMAPool(0, &stConfig), MI_SUCCESS);
                //printf("destroy venc ring pool\n");
            }
            STCHECKRESULT(MI_SCL_DisableOutputPort(SclDevId, SclChnId, SclPortId));
            STCHECKRESULT(MI_ISP_DisableOutputPort(IspDevId, IspChnId, IspPortId));
            STCHECKRESULT(unbind_port(&stBindInfo[1]));
            STCHECKRESULT(unbind_port(&stBindInfo[0]));
            STCHECKRESULT(MI_VENC_DestroyChn(VencDevId, VencChnId));
            break;

        default:
            return -EINVAL;
    }

    return MI_SUCCESS;
}

MI_S32 ST_UvcDeinit()
{
    if (!g_UvcSrc)
        return -1;

    for (int i = 0; i < g_UvcSrc->devnum; i++)
    {
        ST_UvcDev_t *dev = &g_UvcSrc->dev[i];
        STCHECKRESULT(ST_UVC_StopDev((dev->handle)));
        STCHECKRESULT(ST_UVC_DestroyDev(dev->handle));
        STCHECKRESULT(ST_UVC_Uninit(dev->handle));
    }
    return MI_SUCCESS;
}

MI_S32 ST_UvcInitDev(ST_UvcDev_t *dev, MI_U32 maxpacket, MI_U8 mult, MI_U8 burst, MI_U8 c_intf, MI_U8 s_intf, MI_S32 mode, MI_S32 type)
{
    ST_UVC_Setting_t pstSet={g_maxbuf_cnt, maxpacket, mult, burst, c_intf, s_intf, (UVC_IO_MODE_e)mode, (Transfer_Mode_e)type};
    ST_UVC_MMAP_BufOpts_t m = {UVC_MM_FillBuffer};
    ST_UVC_USERPTR_BufOpts_t u = {UVC_UP_FillBuffer, UVC_UP_FinishBuffer};

    ST_UVC_OPS_t fops = { UVC_Init,
                          UVC_Deinit,
                          {{}},
                          UVC_StartCapture,
                          UVC_StopCapture,
                          UVC_ForceIdr};

    if (g_bEnableFile)
    {
        m                     = {UVC_MM_FILE_FillBuffer};
        u                     = {UVC_UP_FILE_FillBuffer, UVC_UP_FILE_FinishBuffer};
        fops.UVC_StartCapture = UVC_StartReadFile;
        fops.UVC_StopCapture  = UVC_StopReadFile;
    }

    if (mode==UVC_MEMORY_MMAP)
        fops.m = m;
    else
        fops.u = u;

    printf(ASCII_COLOR_YELLOW "ST_UvcInitDev: name:%s bufcnt:%d mult:%d burst:%d ci:%d si:%d, Mode:%s, Type:%s" ASCII_COLOR_END "\n",
                    dev->name, g_maxbuf_cnt, mult, burst, c_intf, s_intf, mode==UVC_MEMORY_MMAP?"mmap":"userptr", type==USB_ISOC_MODE?"isoc":"bulk");

    ST_UVC_ChnAttr_t pstAttr ={pstSet,fops};
    STCHECKRESULT(ST_UVC_Init(dev->name, &dev->handle));
    STCHECKRESULT(ST_UVC_CreateDev(dev->handle, &pstAttr));
    STCHECKRESULT(ST_UVC_StartDev(dev->handle));
    return MI_SUCCESS;
}

MI_S32 ST_UvcInit(MI_S32 devnum, MI_U32 *maxpacket, MI_U8 *mult, MI_U8 *burst, MI_U8 *intf, MI_S32 mode, MI_S32 type)
{
    char devnode[20] = "/dev/video0";

    if (devnum > MAX_UVC_DEV_NUM)
    {
        printf(ASCII_COLOR_YELLOW "%s Max Uvc Dev Num %d\n" ASCII_COLOR_END "\n", __func__, MAX_UVC_DEV_NUM);
        devnum = MAX_UVC_DEV_NUM;
    }

    g_UvcSrc = (ST_UvcSrc_t*)malloc(sizeof(g_UvcSrc) + sizeof(ST_UvcDev_t) * devnum);
    memset(g_UvcSrc, 0x0, sizeof(g_UvcSrc) + sizeof(ST_UvcDev_t) * devnum);
    g_UvcSrc->devnum = devnum;

    for (int i = 0; i < devnum; i++)
    {
        ST_UvcDev_t *dev = &g_UvcSrc->dev[i];
        sprintf(devnode, "/dev/video%d", i);
        dev->dev_index = i;
        memcpy(dev->name, devnode, sizeof(devnode));
        ST_UvcInitDev(dev, maxpacket[i], mult[i], burst[i], intf[2*i], intf[2*i+1], mode, type);
    }
    return MI_SUCCESS;
}

void ST_DoExitProc(void *args)
{
    g_bExit = TRUE;
}

static void help_message(char **argv)
{
    printf("\n");
    printf("usage: %s \n", argv[0]);
    printf(" -a set sensor pad\n");
    printf(" -A set sensor resolution index\n");
    printf(" -b bitrate\n");
    printf(" -B burst\n");
    printf(" -m mult\n");
    printf(" -p maxpacket\n");
    printf(" -M 0:mmap, 1:userptr\n");
    printf(" -N num of uvc stream\n");
    printf(" -i set iq api.bin,ex: -i \"/customer/imx415_api.bin\" \n");
    printf(" -I c_intf,s_intf\n");
    printf("    c_intf: control interface\n");
    printf("    s_intf: streaming interface\n");
    printf(" -t Trace level (0-6) \n");
    printf(" -q open iqserver\n");
    printf(" -T 0:Isoc, 1:Bulk\n");
    printf(" -Q qfactor\n");
    printf(" -f : use file instead of MI,ex: -f /customer/resource\n");
    printf(" -V 0:Disable video, 1:Enable video(Default)\n");
#ifdef AUDIO_ENABLE
    printf(" -d : AI Attach Intf Id\n");
    printf(" -D : AO Attach Intf Id\n");
    printf(" -S : AO Samle Rate\n");
#endif
    printf(" -h help message\n");
    printf(" multi stram param, using ',' to seperate\n");
    printf("\nExample: %s -N2 -m0,0 -M1 -I0,1,2,3\n", argv[0]);
    printf("\n");
}

#define PARSE_PARM(instance)\
{\
    int i = 0; \
    do { \
        if (!i) {\
            p = strtok(optarg, ","); \
        } \
        if (p) {\
            instance[i++] = atoi(p); \
        } \
    } while((p = strtok(NULL, ","))!=NULL); \
}

int main(int argc, char **argv)
{
    char *p;
    MI_U32 maxpacket[MAX_UVC_DEV_NUM] = {1024, 1024, 1024, 1024, 1024, 1024};
    MI_U8 mult[MAX_UVC_DEV_NUM] = {2, 2, 2, 2, 2, 2},
          burst[MAX_UVC_DEV_NUM] = {13, 13, 13, 13, 0, 0};
    MI_U8 intf[2 * MAX_UVC_DEV_NUM] = {0};
    MI_S32 result = 0, mode = UVC_MEMORY_MMAP, type = USB_ISOC_MODE;
    MI_S32 trace_level = UVC_DBG_ERR;
#ifdef HDMI_ENABLE
    ST_Rect_T stVdispOutRect;
    MI_HDMI_TimingType_e s32HdmiTiming = E_MI_HDMI_TIMING_1080_60P;
    MI_DISP_OutputTiming_e s32DispTiming = E_MI_DISP_OUTPUT_1080P60;
    ST_DispoutTiming_e enDispTimingCur = E_ST_TIMING_1080P_60;//E_ST_TIMING_1080P_60;
#endif

#ifdef AUDIO_ENABLE
    ST_UAC_Handle_h stHandle = {};
#endif

    struct sigaction sigAction;
    sigAction.sa_handler = ST_HandleSig;
    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_flags = 0;
    sigaction(SIGINT, &sigAction, NULL);

    ST_DefaultArgs(&g_stConfig);

    while((result = getopt(argc, argv, "a:A:b:B:M:N:m:p:I:t:i:qT:Q:V:d:D:S:Hhl:f:")) != -1)
    {
        switch(result)
        {
            case 'a':
                g_stConfig.u8SnrPad  = strtol(optarg, NULL, 10);
                break;

            case 'A':
                g_stConfig.s8SnrResIndex = strtol(optarg, NULL, 10);
                break;

            case 'b':
                PARSE_PARM(g_bitrate);
                break;

            case 'B':
                PARSE_PARM(burst);
                break;

            case 'm':
                PARSE_PARM(mult);
                break;

            case 'M':
                mode = strtol(optarg, NULL, 10);
                break;

            case 'N':
                g_device_num = strtol(optarg, NULL, 10);
                break;

            case 'p':
                PARSE_PARM(maxpacket);
                break;

            case 'I':
                PARSE_PARM(intf);
                break;

            case 't':
                trace_level = strtol(optarg, NULL, 10);
                break;

            case 'q':
                g_enable_iqserver = TRUE;
                break;

            case 'T':
                type = strtol(optarg, NULL, 10);
                break;

            case 'Q':
                PARSE_PARM(g_qfactor);
                break;

            case 'i':
                strcpy(g_IspBinPath, optarg);
                ST_DBG("g_IspBinPath:%s\n", g_IspBinPath);
                g_load_iq_bin = TRUE;
                break;

            case 'V':
                g_bEnableVideo = strtol(optarg, NULL, 10);
                break;

            case 'f':
                for (int i = 0; i < MAX_UVC_DEV_NUM; i++)
                {
                    strcpy(g_stUvcFile[i].filePath, optarg);
                }
                for (int i = 0; i < 2; i++)
                {
                    strcpy(g_stUacFile[i].filePath, optarg);
                }
                ST_DBG("FilePath:%s\n", optarg);
                g_bEnableFile = TRUE;
                break;

#ifdef AUDIO_ENABLE
            case 'd':
                bEnableAI = TRUE;
                AiIfId = strtol(optarg, NULL, 10);
                break;

            case 'D':
                bEnableAO = TRUE;
                AoIfId = strtol(optarg, NULL, 10);
                break;

            case 'S':
                u32AoSampleRate = strtol(optarg, NULL, 10);
                break;
#endif
            case 'h':
                help_message(argv);
                return 0;
            case 'H':
                g_bEnableHDMI = TRUE;
                break;
            case 'l':
                g_stConfig.en3dNrLevel = (MI_ISP_3DNR_Level_e)strtol(optarg, NULL, 10);
                break;

            default:
                break;
        }
    }

    if(!g_bEnableHDMI && g_device_num == 1)
    {
        g_stStreamAttr[USB_CAMERA0_INDEX].u32IspPort = ISP_PORT_ID_RT;
        g_stStreamAttr[USB_CAMERA0_INDEX].u32SclDev = SCL_DEV_ID_RT;
    }

    STCHECKRESULT(MI_SYS_Init(0));
#ifdef AUDIO_ENABLE
    ST_UAC_SetTraceLevel(trace_level);
    STCHECKRESULT(ST_UacInit(&stHandle));
#endif
    if(g_bEnableVideo)
    {
        ST_UVC_SetTraceLevel(trace_level);
        if (!g_bEnableFile)
        {
            STCHECKRESULT(ST_VideoModuleInit(&g_stConfig));
        }
        ST_UvcInit(g_device_num, maxpacket, mult, burst, intf, mode, type);
    }
#ifdef HDMI_ENABLE
    if(g_bEnableHDMI)
    {
        MI_U32 u32Width = 0, u32Height = 0;
        memset(&stVdispOutRect,0,sizeof(stVdispOutRect));
        STCHECKRESULT(ST_GetTimingInfo(enDispTimingCur,
                        &s32HdmiTiming, &s32DispTiming, &u32Width, &u32Height));
        stVdispOutRect.u16PicW = (MI_U16)u32Width;
        stVdispOutRect.u16PicH = (MI_U16)u32Height;
        ST_DBG("enDispTimingCur=%d,s32DispTiming=%d,s32HdmiTiming=%d\n",enDispTimingCur,s32DispTiming,s32HdmiTiming);
        Disp_DevInit(g_hdmiDispDev, g_hdmiDispLayer, s32DispTiming);
        ST_DispChnInfo_t stDispInfo;
        stDispInfo.InputPortNum = 1;
        stDispInfo.stInputPortAttr[0].u32Port = 0;
        stDispInfo.stInputPortAttr[0].stAttr.u16SrcWidth = stVdispOutRect.u16PicW;
        stDispInfo.stInputPortAttr[0].stAttr.u16SrcHeight = stVdispOutRect.u16PicH;
        stDispInfo.stInputPortAttr[0].stAttr.stDispWin.u16X = stVdispOutRect.u32X;
        stDispInfo.stInputPortAttr[0].stAttr.stDispWin.u16Y = stVdispOutRect.u32Y;
        stDispInfo.stInputPortAttr[0].stAttr.stDispWin.u16Width = stVdispOutRect.u16PicW;
        stDispInfo.stInputPortAttr[0].stAttr.stDispWin.u16Height = stVdispOutRect.u16PicH;
        ST_DBG("set num:%d port:%d,disp(%d,%d,%d,%d),src(%d,%d)\n",stDispInfo.InputPortNum,stDispInfo.stInputPortAttr[0].u32Port,
        stDispInfo.stInputPortAttr[0].stAttr.stDispWin.u16X,stDispInfo.stInputPortAttr[0].stAttr.stDispWin.u16Y,
        stDispInfo.stInputPortAttr[0].stAttr.stDispWin.u16Width,stDispInfo.stInputPortAttr[0].stAttr.stDispWin.u16Height,
        stDispInfo.stInputPortAttr[0].stAttr.u16SrcWidth,stDispInfo.stInputPortAttr[0].stAttr.u16SrcHeight);
        Disp_ChnInit(g_hdmiDispLayer,&stDispInfo);
        Disp_ShowStatus(g_hdmiDispLayer,0,TRUE);
        ST_Sys_BindInfo_T stBindInfo;
        memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
        stBindInfo.stSrcChnPort.u32DevId = SCL_HDMI_DEV_ID;
        stBindInfo.stSrcChnPort.u32ChnId = g_device_num;
        stBindInfo.stSrcChnPort.u32PortId = SCL_PORT_ID;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stBindInfo.stDstChnPort.u32DevId = g_hdmiDispDev;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 60;
        stBindInfo.u32DstFrmrate = 60;
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
        // init hdmi
        MI_DISP_HdmiParam_t stHdmiParam;
        MI_DISP_GetHdmiParam(g_hdmiDispDev,&stHdmiParam);
        stHdmiParam.stCsc.eCscMatrix = E_MI_DISP_CSC_MATRIX_BYPASS;
        MI_DISP_SetHdmiParam(g_hdmiDispDev,&stHdmiParam);
        //STCHECKRESULT(ST_Hdmi_Init());
        STCHECKRESULT(Hdmi_Start(E_MI_HDMI_ID_0, s32HdmiTiming));
    }
#endif
    while(!g_bExit)
    {
        usleep(100 * 1000);
#if USE_TEST
        ST_Test();
#endif
    }

    usleep(100 * 1000);
#ifdef HDMI_ENABLE
    if(g_bEnableHDMI)
    {
        ST_Sys_BindInfo_T stBindInfo;
        memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
        stBindInfo.stSrcChnPort.u32DevId = SCL_HDMI_DEV_ID;
        stBindInfo.stSrcChnPort.u32ChnId = g_device_num;
        stBindInfo.stSrcChnPort.u32PortId = SCL_PORT_ID;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stBindInfo.stDstChnPort.u32DevId = g_hdmiDispDev;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.u32SrcFrmrate = 60;
        stBindInfo.u32DstFrmrate = 60;
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
        // stop disp dev and disable input port
        ExecFunc(Disp_DeInit(g_hdmiDispDev, g_hdmiDispLayer, 1), MI_SUCCESS);
        // stop hdmi
        ExecFunc(ST_Hdmi_DeInit(E_MI_HDMI_ID_0), MI_SUCCESS);
    }
#endif
    if(g_bEnableVideo)
    {
        ST_UvcDeinit();
        if (!g_bEnableFile)
        {
            STCHECKRESULT(ST_VideoModuleUnInit(&g_stConfig));
        }
    }
#ifdef AUDIO_ENABLE
    STCHECKRESULT(ST_UacDeinit(stHandle));
#endif
    STCHECKRESULT(MI_SYS_Exit(0));

    return 0;
}

