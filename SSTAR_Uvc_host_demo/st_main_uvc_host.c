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

#include "st_uvc_host.h"
#ifdef AUDIO_ENABLE
#include "st_uac_host.h"
#endif
#include "common.h"
#include "uvc_host_common.h"
#include "mi_jpd.h"
#include "mi_vdec.h"
#include "mi_scl.h"
#include "mi_disp.h"
#include "mi_hdmi.h"
#include "mi_sys.h"

#include "sstar_drm.h"
#include "sstar_scl.h"
#include "sstar_vdec.h"

#define JPD_DEV_ID 0
#define JPD_CHN_ID 0
#define JPD_PORT_ID 0

#define VDEC_DEV_ID 0
#define VDEC_CHN_ID 0
#define VDEC_PORT_ID 1

#define SCL_DEV_ID 1
#define SCL_CHN_ID 0
#define SCL_PORT_ID 0

#define DISP_DEV_ID 0
#define DISP_CHN_ID 0
#define DISP_PORT_ID 0
#define DISP_LAYER_ID 0

#define AO_DEV_ID 0
#define AI_DEV_ID 0

#define HDMI_DEV_ID 0

static unsigned int g_audio_index = 0;
static unsigned int g_video_index = 0;
static unsigned int g_buf_cnt = 3;
static int g_run_mode = VS_MODE;
static int g_buf_handle_mode = BUF_HANDLE_MODE_NONE;
static char g_file_path[16] = DEFAULT_DIRECTORY;
static bool g_exit = false;
static pthread_t vs_tid;
static pthread_t vc_tid;
static pthread_t as_in_tid;
static pthread_t as_out_tid;
static pthread_t ac_tid;

DEMO_DBG_LEVEL_e demo_debug_level;

#ifdef AUDIO_ENABLE

#include "st_uac_host.h"
#include "mi_ao.h"
#include "mi_ai.h"

static MI_S32 St_AoModuleInit(Audio_Handle_t *audio_handle)
{
    MI_AO_Attr_t stAoSetAttr;
    MI_S8 s8LeftVolume = 0;
    MI_S8 s8RightVolume = 0;
    MI_AO_GainFading_e eGainFading = E_MI_AO_GAIN_FADING_OFF;

    memset(&stAoSetAttr, 0, sizeof(MI_AO_Attr_t));
    stAoSetAttr.enChannelMode = E_MI_AO_CHANNEL_MODE_STEREO;
    stAoSetAttr.enFormat = E_MI_AUDIO_FORMAT_PCM_S16_LE;
    stAoSetAttr.enSampleRate = E_MI_AUDIO_SAMPLE_RATE_48000;
    stAoSetAttr.enSoundMode = E_MI_AUDIO_SOUND_MODE_STEREO;
    stAoSetAttr.u32PeriodSize = 1024;

    ExecFunc(MI_AO_Open(AO_DEV_ID, &stAoSetAttr), MI_SUCCESS);
    ExecFunc(MI_AO_AttachIf(AO_DEV_ID, E_MI_AO_IF_HDMI_A, 0), MI_SUCCESS);

    ExecFunc(MI_AO_SetVolume(AO_DEV_ID, s8LeftVolume, s8RightVolume, eGainFading), MI_SUCCESS);

    return MI_SUCCESS;
}

static MI_S32 St_AiModuleInit(Audio_Handle_t *audio_handle)
{
    MI_AI_Attr_t stAiSetAttr;
    MI_AI_If_e enAiIf[] = {E_MI_AI_IF_ADC_AB};
    MI_SYS_ChnPort_t stAiChnOutputPort;

    memset(&stAiSetAttr, 0, sizeof(MI_AI_Attr_t));
    stAiSetAttr.bInterleaved = TRUE;
    stAiSetAttr.enFormat = E_MI_AUDIO_FORMAT_PCM_S16_LE;
    stAiSetAttr.enSampleRate = E_MI_AUDIO_SAMPLE_RATE_48000;
    stAiSetAttr.enSoundMode = E_MI_AUDIO_SOUND_MODE_STEREO;
    stAiSetAttr.u32PeriodSize = 1024;

    ExecFunc(MI_AI_Open(AI_DEV_ID, &stAiSetAttr), MI_SUCCESS);
    ExecFunc(MI_AI_AttachIf(AI_DEV_ID, enAiIf, sizeof(enAiIf) / sizeof(enAiIf[0])), MI_SUCCESS);

    memset(&stAiChnOutputPort, 0, sizeof(stAiChnOutputPort));
    stAiChnOutputPort.eModId = E_MI_MODULE_ID_AI;
    stAiChnOutputPort.u32DevId = AI_DEV_ID;
    stAiChnOutputPort.u32ChnId = 0;
    stAiChnOutputPort.u32PortId = 0;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(0, &stAiChnOutputPort, 4, 8), MI_SUCCESS);
    ExecFunc(MI_AI_EnableChnGroup(AI_DEV_ID, 0), MI_SUCCESS);

    return MI_SUCCESS;
}

static MI_S32 St_AoModuleDeinit(Audio_Handle_t *audio_handle)
{
    ExecFunc(MI_AO_DetachIf(AO_DEV_ID, E_MI_AO_IF_HDMI_A), MI_SUCCESS);
    ExecFunc(MI_AO_Close(AO_DEV_ID), MI_SUCCESS);
    return MI_SUCCESS;
}

static MI_S32 St_AiModuleDeinit(Audio_Handle_t *audio_handle)
{
    ExecFunc(MI_AI_DisableChnGroup(AI_DEV_ID, 0), MI_SUCCESS);
    ExecFunc(MI_AI_Close(AI_DEV_ID), MI_SUCCESS);
    return MI_SUCCESS;
}


static MI_S32 St_AudioModuleInit(Audio_Handle_t *audio_handle, int mode)
{
    if(mode & AS_IN_MODE)
        ExecFunc(St_AoModuleInit(audio_handle), MI_SUCCESS);

    if(mode & AS_OUT_MODE)
        ExecFunc(St_AiModuleInit(audio_handle), MI_SUCCESS);

    return MI_SUCCESS;
}

static MI_S32 St_AudioModuleDeinit(Audio_Handle_t *audio_handle, int mode)
{
    if(mode & AS_IN_MODE)
        ExecFunc(St_AoModuleDeinit(audio_handle), MI_SUCCESS);

    if(mode & AS_OUT_MODE)
        ExecFunc(St_AiModuleDeinit(audio_handle), MI_SUCCESS);

    return MI_SUCCESS;
}

static MI_S32 St_Audio_Preview(Audio_Handle_t *audio_handle, Audio_Buffer_t *audio_buf)
{
    MI_S32 s32Ret;

    do {
        s32Ret = MI_AO_Write(AO_DEV_ID, audio_buf->buf, audio_buf->length, 0, -1);
    } while(MI_AO_ERR_NOBUF == s32Ret);

    if(MI_SUCCESS != s32Ret)
    {
        DEMO_ERR(audio_handle, "MI_AO_SendFrame: 0x%x.\n", s32Ret);
        return -1;
    }

    return MI_SUCCESS;
}

static MI_S32 St_Audio_Grab(Audio_Handle_t *audio_handle, Audio_Buffer_t *audio_buf)
{
    MI_S32 s32Ret;

    MI_AI_Data_t stAudioFrame;

    memset(&stAudioFrame, 0x0, sizeof(MI_AI_Data_t));

    s32Ret = MI_AI_Read(AI_DEV_ID, 0, &stAudioFrame, NULL, 1000);
    if(MI_SUCCESS == s32Ret)
    {
        audio_buf->length = stAudioFrame.u32Byte[0];
        memcpy(audio_buf->buf, stAudioFrame.apvBuffer[0], audio_buf->length);
        MI_AI_ReleaseData(AI_DEV_ID, 0, &stAudioFrame, NULL);
    }
    else
    {
        DEMO_ERR(audio_handle, "MI_AI_GetFrame: 0x%x.\n", s32Ret);
        return -1;
    }

    return MI_SUCCESS;
}

#endif

static MI_S32 sstar_scl_init_for_uvc_host(buffer_object_t *buf_obj)
{
    // MI_S32 ret;
    // sstar_scl_info_t scl_info;

    // scl_info.scl_dev_id = 1;
    // scl_info.scl_chn_id = 0;
    // scl_info.scl_outport = 0;
    // scl_info.scl_hw_outport_mask = E_MI_SCL_HWSCL0;
    // scl_info.scl_rotate = E_MI_SYS_ROTATE_NONE;
    // scl_info.scl_out_width = ALIGN_BACK(buf_obj->vdec_info.v_out_width, ALIGN_NUM);
    // scl_info.scl_out_height = ALIGN_BACK(buf_obj->vdec_info.v_out_height, ALIGN_NUM);
    // scl_info.scl_src_chn_port.eModId = E_MI_MODULE_ID_HVP;
    // scl_info.scl_src_chn_port.u32DevId = 0;
    // scl_info.scl_src_chn_port.u32ChnId = 0;
    // scl_info.scl_src_chn_port.u32PortId = 0;
    // scl_info.scl_src_module_inited = 1;
    // scl_info.scl_src_fps = 60;
    // scl_info.scl_dst_fps = 60;
    // scl_info.scl_bind_type = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    // ExecFunc(sstar_scl_init(&scl_info), MI_SUCCESS);
    // return 0;
    // MI_SYS_ChnPort_t stSysChnPort;
    // MI_SCL_DevAttr_t stSclDevAttr;
    // MI_SCL_ChannelAttr_t stSclChnAttr;
    // MI_SCL_ChnParam_t stSclChnParam;
    // MI_SCL_OutPortParam_t stSclOutputParam;

    // memset(&stSysChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    // memset(&stSclDevAttr, 0x0, sizeof(MI_SCL_DevAttr_t));
    // memset(&stSclChnAttr, 0x0, sizeof(MI_SCL_ChannelAttr_t));
    // memset(&stSclChnParam, 0x0, sizeof(MI_SCL_ChnParam_t));
    // memset(&stSclOutputParam, 0x0, sizeof(MI_SCL_OutPortParam_t));

    // stSysChnPort.eModId = E_MI_MODULE_ID_SCL;
    // stSysChnPort.u32DevId = SCL_DEV_ID;
    // stSysChnPort.u32ChnId = SCL_CHN_ID;
    // stSysChnPort.u32PortId = SCL_PORT_ID;
    // stSclDevAttr.u32NeedUseHWOutPortMask = E_MI_SCL_HWSCL0;
    // stSclOutputParam.stSCLOutCropRect.u16X = 0;
    // stSclOutputParam.stSCLOutCropRect.u16Y = 0;
    // stSclOutputParam.stSCLOutCropRect.u16Width = video_handle->video_info.width;
    // stSclOutputParam.stSCLOutCropRect.u16Height = video_handle->video_info.height;
    // stSclOutputParam.stSCLOutputSize.u16Width = video_handle->video_info.width;
    // stSclOutputParam.stSCLOutputSize.u16Height = video_handle->video_info.height;
    // stSclOutputParam.bMirror = false;
    // stSclOutputParam.bFlip = false;
    // stSclOutputParam.eCompressMode= E_MI_SYS_COMPRESS_MODE_NONE;
    // stSclOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

    // ExecFunc(MI_SCL_CreateDevice(SCL_DEV_ID, &stSclDevAttr), MI_SUCCESS);
    // ExecFunc(MI_SCL_CreateChannel(SCL_DEV_ID, SCL_CHN_ID, &stSclChnAttr), MI_SUCCESS);
    // ExecFunc(MI_SCL_SetChnParam(SCL_DEV_ID, SCL_CHN_ID, &stSclChnParam), MI_SUCCESS);
    // ExecFunc(MI_SCL_StartChannel(SCL_DEV_ID, SCL_CHN_ID), MI_SUCCESS);
    // ExecFunc(MI_SCL_SetOutputPortParam(SCL_DEV_ID, SCL_CHN_ID, SCL_PORT_ID, &stSclOutputParam), MI_SUCCESS);
    // ExecFunc(MI_SYS_SetChnOutputPortDepth(0, &stSysChnPort, 0, 4), MI_SUCCESS);
    // ExecFunc(MI_SCL_EnableOutputPort(SCL_DEV_ID, SCL_CHN_ID, SCL_PORT_ID), MI_SUCCESS);

    // return MI_SUCCESS;
}

static MI_S32 sstar_scl_deinit_for_uvc_host(buffer_object_t *buf_obj)
{
    // scl_info.scl_dev_id = 1;
    // scl_info.scl_chn_id = 0;
    // scl_info.scl_outport = 0;
    // scl_info.scl_src_chn_port.eModId = E_MI_MODULE_ID_SCL;
    // scl_info.scl_src_chn_port.u32DevId = ;
    // scl_info.scl_src_chn_port.u32ChnId = ;
    // scl_info.scl_src_chn_port.u32PortId = ;
    // scl_info.scl_src_fps = 60;
    // scl_info.scl_dst_fps = 60;
    // scl_info.scl_bind_type = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    // ExecFunc(sstar_scl_deinit(&scl_info), MI_SUCCESS);

    // ExecFunc(MI_SCL_DisableOutputPort(SCL_DEV_ID, SCL_CHN_ID, SCL_CHN_ID), MI_SUCCESS);
    // ExecFunc(MI_SCL_StopChannel(SCL_DEV_ID, SCL_CHN_ID), MI_SUCCESS);
    // ExecFunc(MI_SCL_DestroyChannel(SCL_DEV_ID, SCL_CHN_ID), MI_SUCCESS);

    return MI_SUCCESS;
}

static MI_S32 St_JpdModuleInit()
{
    MI_SYS_ChnPort_t stSysChnPort;
    MI_JPD_InitParam_t stJpdInitParam;
    MI_JPD_ChnCreatConf_t stJpdChnCreatConf;

    memset(&stSysChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    memset(&stJpdInitParam, 0, sizeof(MI_JPD_InitParam_t));
    memset(&stJpdChnCreatConf, 0, sizeof(MI_JPD_ChnCreatConf_t));

    stSysChnPort.eModId = E_MI_MODULE_ID_JPD;
    stSysChnPort.u32DevId = JPD_DEV_ID;
    stSysChnPort.u32ChnId = JPD_CHN_ID;
    stSysChnPort.u32PortId = JPD_PORT_ID;
    stJpdChnCreatConf.u32MaxPicWidth   = 8192;
    stJpdChnCreatConf.u32MaxPicHeight  = 8192;
    stJpdChnCreatConf.u32StreamBufSize = 2*1024*1024;

#if 0
    ExecFunc(MI_JPD_CreateDev(JPD_DEV_ID, &stJpdInitParam), MI_SUCCESS);
#endif
    ExecFunc(MI_JPD_CreateChn(JPD_DEV_ID, JPD_CHN_ID, &stJpdChnCreatConf), MI_SUCCESS);
    ExecFunc(MI_JPD_StartChn(JPD_DEV_ID, JPD_CHN_ID), MI_SUCCESS);
    ExecFunc(MI_SYS_SetChnOutputPortDepth(0, &stSysChnPort, 0, 4), MI_SUCCESS);

    return MI_SUCCESS;
}

static MI_S32 St_JpdModuleDeinit()
{
    ExecFunc(MI_JPD_StopChn(JPD_DEV_ID, JPD_CHN_ID), MI_SUCCESS);
    ExecFunc(MI_JPD_DestroyChn(JPD_DEV_ID, JPD_CHN_ID), MI_SUCCESS);
#if 0
    ExecFunc(MI_JPD_DestroyDev(JPD_DEV_ID), MI_SUCCESS);
#endif

    return MI_SUCCESS;
}
#if 0
static MI_S32 St_VdecModuleInit(Video_Handle_t *video_handle)
{
    MI_SYS_ChnPort_t stSysChnPort;
    MI_VDEC_InitParam_t stVdecInitParam;
    MI_VDEC_ChnAttr_t stVdecChnAttr;
    MI_VDEC_OutputPortAttr_t stVdecOutputPortAttr;

    memset(&stSysChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    memset(&stVdecInitParam, 0, sizeof(MI_VDEC_InitParam_t));
    memset(&stVdecChnAttr, 0, sizeof(MI_VDEC_ChnAttr_t));
    memset(&stVdecOutputPortAttr, 0, sizeof(MI_VDEC_OutputPortAttr_t));

    stSysChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stSysChnPort.u32DevId = VDEC_DEV_ID;
    stSysChnPort.u32ChnId = VDEC_CHN_ID;
    stSysChnPort.u32PortId = VDEC_PORT_ID;
    //stVdecInitParam.bDisableLowLatency = false;
    stVdecChnAttr.eCodecType = (video_handle->video_info.pixelformat == V4L2_PIX_FMT_H264) ? E_MI_VDEC_CODEC_TYPE_H264 : E_MI_VDEC_CODEC_TYPE_H265;
    stVdecChnAttr.eDpbBufMode = E_MI_VDEC_DPB_MODE_INPLACE_ONE_BUF;
    stVdecChnAttr.eVideoMode = E_MI_VDEC_VIDEO_MODE_FRAME;
    stVdecChnAttr.stVdecVideoAttr.stErrHandlePolicy.bUseCusPolicy = false;
    stVdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 1;
    stVdecChnAttr.u32BufSize = 2*1024*1024;
    stVdecChnAttr.u32PicHeight = video_handle->video_info.height;
    stVdecChnAttr.u32PicWidth = video_handle->video_info.width;
    stVdecChnAttr.u32Priority = 0;
    stVdecOutputPortAttr.u16Height = video_handle->video_info.height;
    stVdecOutputPortAttr.u16Width = video_handle->video_info.width;

    ExecFunc(MI_VDEC_CreateDev(VDEC_DEV_ID, &stVdecInitParam), MI_SUCCESS);
    ExecFunc(MI_VDEC_CreateChn(VDEC_DEV_ID, VDEC_CHN_ID, &stVdecChnAttr), MI_SUCCESS);
    ExecFunc(MI_VDEC_SetDisplayMode(VDEC_DEV_ID, VDEC_CHN_ID, E_MI_VDEC_DISPLAY_MODE_PLAYBACK), MI_SUCCESS);
    ExecFunc(MI_VDEC_StartChn(VDEC_DEV_ID, VDEC_CHN_ID), MI_SUCCESS);
    ExecFunc(MI_VDEC_SetOutputPortAttr(VDEC_DEV_ID, VDEC_CHN_ID, &stVdecOutputPortAttr), MI_SUCCESS);
    ExecFunc(MI_SYS_SetChnOutputPortDepth(0, &stSysChnPort, 0, 4), MI_SUCCESS);

    return MI_SUCCESS;
}

static MI_S32 St_VdecModuleDeinit(Video_Handle_t *video_handle)
{
    ExecFunc(MI_VDEC_StopChn(VDEC_DEV_ID, VDEC_CHN_ID), MI_SUCCESS);
    ExecFunc(MI_VDEC_DestroyChn(VDEC_DEV_ID, VDEC_CHN_ID), MI_SUCCESS);
#if 0
    ExecFunc(MI_VDEC_DestroyDev(VDEC_DEV_ID), MI_SUCCESS);
#endif

    return MI_SUCCESS;
}

static MI_S32 St_DispModuleInit(Video_Handle_t *video_handle)
{
    MI_DISP_PubAttr_t stDispPubAttr;
    MI_DISP_VideoLayerAttr_t stDispLayerAttr;
    MI_DISP_InputPortAttr_t stDispInputPortAttr;

    memset(&stDispPubAttr, 0, sizeof(MI_DISP_PubAttr_t));
    memset(&stDispLayerAttr, 0, sizeof(MI_DISP_VideoLayerAttr_t));
    memset(&stDispInputPortAttr, 0, sizeof(MI_DISP_InputPortAttr_t));

    stDispPubAttr.eIntfSync = E_MI_DISP_OUTPUT_1920x1080_5994;
    stDispPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
    stDispPubAttr.u32BgColor = YUYV_BLACK;
    stDispLayerAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stDispLayerAttr.stVidLayerDispWin.u16Height = 1080;
    stDispLayerAttr.stVidLayerDispWin.u16Width = 1920;
    stDispLayerAttr.stVidLayerDispWin.u16X = 0;
    stDispLayerAttr.stVidLayerDispWin.u16Y = 0;
    stDispLayerAttr.stVidLayerSize.u16Height = 1080;
    stDispLayerAttr.stVidLayerSize.u16Width = 1920;
    stDispInputPortAttr.stDispWin.u16Height = 1080;
    stDispInputPortAttr.stDispWin.u16Width = 1920;
    stDispInputPortAttr.stDispWin.u16X = 0;
    stDispInputPortAttr.stDispWin.u16Y = 0;
    stDispInputPortAttr.u16SrcHeight = video_handle->video_info.height;
    stDispInputPortAttr.u16SrcWidth = video_handle->video_info.width;

    ExecFunc(MI_DISP_SetPubAttr(DISP_DEV_ID, &stDispPubAttr), MI_SUCCESS);
    ExecFunc(MI_DISP_Enable(DISP_DEV_ID), MI_SUCCESS);
    ExecFunc(MI_DISP_BindVideoLayer(DISP_LAYER_ID,DISP_DEV_ID), MI_SUCCESS);
    ExecFunc(MI_DISP_SetVideoLayerAttr(DISP_LAYER_ID, &stDispLayerAttr), MI_SUCCESS);
    ExecFunc(MI_DISP_EnableVideoLayer(DISP_LAYER_ID), MI_SUCCESS);
    ExecFunc(MI_DISP_SetInputPortAttr(DISP_LAYER_ID, 0, &stDispInputPortAttr), MI_SUCCESS);
    ExecFunc(MI_DISP_EnableInputPort(DISP_LAYER_ID, 0), MI_SUCCESS);
    ExecFunc(MI_DISP_SetInputPortSyncMode(DISP_LAYER_ID, 0, E_MI_DISP_SYNC_MODE_FREE_RUN), MI_SUCCESS);

    return MI_SUCCESS;
}

static MI_S32 St_DispModuleDeinit(Video_Handle_t *video_handle)
{
    ExecFunc(MI_DISP_DisableInputPort(DISP_LAYER_ID, 0), MI_SUCCESS);
    ExecFunc(MI_DISP_DisableVideoLayer(DISP_LAYER_ID), MI_SUCCESS);
    ExecFunc(MI_DISP_UnBindVideoLayer(DISP_LAYER_ID, DISP_DEV_ID), MI_SUCCESS);
    ExecFunc(MI_DISP_Disable(DISP_DEV_ID), MI_SUCCESS);

    return MI_SUCCESS;
}

static MI_S32 St_HdmiModuleInit(Video_Handle_t *video_handle)
{
    MI_HDMI_InitParam_t stHdmiInitParam;
    MI_HDMI_Attr_t stHdmiAttr;

    memset(&stHdmiInitParam, 0, sizeof(MI_HDMI_InitParam_t));
    memset(&stHdmiAttr, 0, sizeof(MI_HDMI_Attr_t));

    stHdmiInitParam.pCallBackArgs = NULL;
    stHdmiInitParam.pfnHdmiEventCallback = NULL;
    stHdmiAttr.stVideoAttr.bEnableVideo = true;
    stHdmiAttr.stVideoAttr.eColorType = E_MI_HDMI_COLOR_TYPE_YCBCR444;
    stHdmiAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_24BIT;
    stHdmiAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
    stHdmiAttr.stVideoAttr.eTimingType = E_MI_HDMI_TIMING_1080_60P;

#ifdef AUDIO_ENABLE
    if (g_run_mode & AS_IN_MODE)
    {
        stHdmiAttr.stAudioAttr.bEnableAudio = true;
        stHdmiAttr.stAudioAttr.bIsMultiChannel = 0;
        stHdmiAttr.stAudioAttr.eSampleRate = E_MI_HDMI_AUDIO_SAMPLERATE_48K;
        stHdmiAttr.stAudioAttr.eCodeType = E_MI_HDMI_ACODE_PCM;
        stHdmiAttr.stAudioAttr.eBitDepth = E_MI_HDMI_BIT_DEPTH_16;
    }
    else
        stHdmiAttr.stAudioAttr.bEnableAudio = false;
#else
    stHdmiAttr.stAudioAttr.bEnableAudio = false;
#endif

    stHdmiAttr.stEnInfoFrame.bEnableAudInfoFrame = true;
    stHdmiAttr.stEnInfoFrame.bEnableAviInfoFrame = true;
    stHdmiAttr.stEnInfoFrame.bEnableSpdInfoFrame = true;

    ExecFunc(MI_HDMI_Init(&stHdmiInitParam), MI_SUCCESS);
    ExecFunc(MI_HDMI_Open(HDMI_DEV_ID), MI_SUCCESS);
    ExecFunc(MI_HDMI_SetAttr(HDMI_DEV_ID, &stHdmiAttr), MI_SUCCESS);
    ExecFunc(MI_HDMI_Start(HDMI_DEV_ID), MI_SUCCESS);

#ifdef AUDIO_ENABLE
    if (g_run_mode & AS_IN_MODE)
    {
        ExecFunc(MI_HDMI_SetAvMute(HDMI_DEV_ID, false), MI_SUCCESS);
    }
#endif

    return MI_SUCCESS;
}

static MI_S32 St_HdmiModuleDeinit(Video_Handle_t *video_handle)
{
    ExecFunc(MI_HDMI_Stop(HDMI_DEV_ID), MI_SUCCESS);

    return MI_SUCCESS;
}

#endif

static MI_S32 St_VideoModuleInit(buffer_object_t *buf_obj)
{
    sstar_scl_info_t scl_info;
    switch(buf_obj->vdec_info.pixelformat)
    {
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_NV12:
            /* scl start */
            memset(&scl_info, 0x0, sizeof(sstar_scl_info_t));
            scl_info.scl_dev_id = 1;
            scl_info.scl_chn_id = 0;
            scl_info.scl_outport = 0;
            scl_info.scl_hw_outport_mask = E_MI_SCL_HWSCL0;
            scl_info.scl_rotate = E_MI_SYS_ROTATE_NONE;
            scl_info.scl_out_width = ALIGN_BACK(buf_obj->vdec_info.v_out_width, ALIGN_NUM);
            scl_info.scl_out_height = ALIGN_BACK(buf_obj->vdec_info.v_out_height, ALIGN_NUM);
            scl_info.scl_src_module_inited = 0;
            ExecFunc(sstar_scl_init(&scl_info), MI_SUCCESS);
            buf_obj->chn_port_info.eModId = E_MI_MODULE_ID_SCL;
            buf_obj->chn_port_info.u32DevId = 1;
            buf_obj->chn_port_info.u32ChnId = 0;
            buf_obj->chn_port_info.u32PortId = 0;
            break;

        case V4L2_PIX_FMT_MJPEG:
            /* jpd start */
            ExecFunc(St_JpdModuleInit(), MI_SUCCESS);
            /* scl start */
            memset(&scl_info, 0x0, sizeof(sstar_scl_info_t));
            scl_info.scl_dev_id = 1;
            scl_info.scl_chn_id = 0;
            scl_info.scl_outport = 0;
            scl_info.scl_hw_outport_mask = E_MI_SCL_HWSCL0;
            scl_info.scl_rotate = E_MI_SYS_ROTATE_NONE;
            scl_info.scl_out_width = ALIGN_BACK(buf_obj->vdec_info.v_out_width, ALIGN_NUM);
            scl_info.scl_out_height = ALIGN_BACK(buf_obj->vdec_info.v_out_height, ALIGN_NUM);
            scl_info.scl_src_module_inited = 1;
            scl_info.scl_src_chn_port.eModId = E_MI_MODULE_ID_JPD;
            scl_info.scl_src_chn_port.u32DevId = 0;
            scl_info.scl_src_chn_port.u32ChnId = 0;
            scl_info.scl_src_chn_port.u32PortId = 0;
            scl_info.scl_src_fps = 30;
            scl_info.scl_dst_fps = 30;
            scl_info.scl_bind_type = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            ExecFunc(sstar_scl_init(&scl_info), MI_SUCCESS);
            buf_obj->chn_port_info.eModId = E_MI_MODULE_ID_SCL;
            buf_obj->chn_port_info.u32DevId = 1;
            buf_obj->chn_port_info.u32ChnId = 0;
            buf_obj->chn_port_info.u32PortId = 0;
            break;

        case V4L2_PIX_FMT_H264:
        case V4L2_PIX_FMT_H265:
            /* vdec start */

            buf_obj->vdec_info.pixelformat = ( buf_obj->vdec_info.pixelformat == V4L2_PIX_FMT_H264) ? E_MI_VDEC_CODEC_TYPE_H264 : E_MI_VDEC_CODEC_TYPE_H265;
            ExecFunc(sstar_vdec_init(buf_obj), MI_SUCCESS);
            buf_obj->chn_port_info.eModId = E_MI_MODULE_ID_VDEC;
            buf_obj->chn_port_info.u32DevId = 0;
            buf_obj->chn_port_info.u32ChnId = 0;
            buf_obj->chn_port_info.u32PortId = 0;
            break;

        default:
            break;
    }



    return MI_SUCCESS;
}

static MI_S32 St_VideoModuleDeinit(buffer_object_t *buf_obj)
{
    sstar_scl_info_t scl_info;

    switch(buf_obj->vdec_info.pixelformat)
    {
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_YUYV:
            memset(&scl_info, 0x0, sizeof(sstar_scl_info_t));
            scl_info.scl_dev_id = 1;
            scl_info.scl_chn_id = 0;
            scl_info.scl_outport = 0;
            scl_info.scl_src_chn_port.eModId = E_MI_MODULE_ID_SCL;
            scl_info.scl_src_module_inited = 0;
            scl_info.scl_src_chn_port.u32DevId = 0;
            scl_info.scl_src_chn_port.u32ChnId = 0;
            scl_info.scl_src_chn_port.u32PortId = 0;
            scl_info.scl_src_fps = 0;
            scl_info.scl_dst_fps = 0;
            scl_info.scl_bind_type = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            ExecFunc(sstar_scl_deinit(&scl_info), MI_SUCCESS);
            break;

        case V4L2_PIX_FMT_MJPEG:
            memset(&scl_info, 0x0, sizeof(sstar_scl_info_t));
            scl_info.scl_dev_id = 1;
            scl_info.scl_chn_id = 0;
            scl_info.scl_outport = 0;
            scl_info.scl_src_module_inited = 1;
            scl_info.scl_src_chn_port.eModId = E_MI_MODULE_ID_JPD;
            scl_info.scl_src_chn_port.u32DevId = 0;
            scl_info.scl_src_chn_port.u32ChnId = 0;
            scl_info.scl_src_chn_port.u32PortId = 0;
            scl_info.scl_src_fps = 30;
            scl_info.scl_dst_fps = 30;
            scl_info.scl_bind_type = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            ExecFunc(sstar_scl_deinit(&scl_info), MI_SUCCESS);
            ExecFunc(St_JpdModuleDeinit(), MI_SUCCESS);
            break;

        case V4L2_PIX_FMT_H264:
        case V4L2_PIX_FMT_H265:
            ExecFunc(sstar_vdec_deinit(buf_obj), MI_SUCCESS);
            break;

        default:
            break;
    }

    return MI_SUCCESS;
}

static MI_S32 St_Video_Preview(Video_Handle_t *video_handle, Video_Buffer_t *video_buf)
{
    MI_S32 s32Ret;

    MI_SYS_ChnPort_t stSysChnPort;
    MI_SYS_BufConf_t stSysBufConf;
    MI_SYS_BufInfo_t stSysBufInfo;
    MI_SYS_BUF_HANDLE stSysBufHandle;
    MI_JPD_StreamBuf_t stJpdStreamBuf;
    MI_VDEC_VideoStream_t stVdecVideoStream;

    memset(&stSysChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    memset(&stSysBufConf, 0, sizeof(MI_SYS_BufConf_t));
    memset(&stSysBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
    memset(&stSysBufHandle, 0, sizeof(MI_SYS_BUF_HANDLE));
    memset(&stJpdStreamBuf, 0, sizeof(MI_JPD_StreamBuf_t));
    memset(&stVdecVideoStream, 0, sizeof(MI_VDEC_VideoStream_t));

    switch(video_handle->video_info.pixelformat)
    {
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_YUYV:
            stSysChnPort.eModId = E_MI_MODULE_ID_SCL;
            stSysChnPort.u32DevId = SCL_DEV_ID;
            stSysChnPort.u32ChnId = SCL_CHN_ID;
            stSysChnPort.u32PortId = SCL_PORT_ID;

            stSysBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
            if(video_handle->video_info.pixelformat == V4L2_PIX_FMT_YUYV)
                stSysBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            else if(video_handle->video_info.pixelformat == V4L2_PIX_FMT_NV12)
                stSysBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            stSysBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
            stSysBufConf.stFrameCfg.u16Width = video_handle->video_info.width;
            stSysBufConf.stFrameCfg.u16Height = video_handle->video_info.height;

            s32Ret = MI_SYS_ChnInputPortGetBuf(&stSysChnPort, &stSysBufConf, &stSysBufInfo, &stSysBufHandle, -1);
            if(MI_SUCCESS != s32Ret)
            {
                DEMO_ERR(video_handle, "MI_SYS_ChnInputPortGetBuf: 0x%x.\n", s32Ret);
                return -1;
            }

            stSysBufInfo.bEndOfStream = true;
            memcpy(stSysBufInfo.stFrameData.pVirAddr[0], video_buf->buf, video_buf->length);

            s32Ret = MI_SYS_ChnInputPortPutBuf(stSysBufHandle ,&stSysBufInfo , false);
            if(MI_SUCCESS != s32Ret)
            {
                DEMO_ERR(video_handle, "MI_SYS_ChnInputPortPutBuf: 0x%x.\n", s32Ret);
                return -1;
            }
            break;

        case V4L2_PIX_FMT_MJPEG:
            s32Ret = MI_JPD_GetStreamBuf(JPD_DEV_ID, JPD_CHN_ID, video_buf->length, &stJpdStreamBuf, -1);
            if(MI_SUCCESS != s32Ret)
            {
                DEMO_ERR(video_handle, "MI_JPD_GetStreamBuf: 0x%x.\n", s32Ret);
                return -1;
            }

            memcpy(stJpdStreamBuf.pu8HeadVirtAddr, video_buf->buf, MIN(stJpdStreamBuf.u32HeadLength, video_buf->length));

            if(stJpdStreamBuf.u32HeadLength + stJpdStreamBuf.u32TailLength < video_buf->length)
            {
                DEMO_ERR(video_handle, "MI_JPD_GetStreamBuf return wrong value: HeadLen%u TailLen%u RequiredLength%u.\n",
                        stJpdStreamBuf.u32HeadLength, stJpdStreamBuf.u32TailLength, video_buf->length);

                s32Ret = MI_JPD_DropStreamBuf(JPD_DEV_ID, JPD_CHN_ID, &stJpdStreamBuf);
                if(MI_SUCCESS != s32Ret)
                {
                    DEMO_ERR(video_handle, "MI_JPD_DropStreamBuf: 0x%x.", s32Ret);
                    return -1;
                }

                return -1;
            }
            else if(stJpdStreamBuf.u32TailLength > 0)
            {
                memcpy(stJpdStreamBuf.pu8TailVirtAddr, video_buf->buf + stJpdStreamBuf.u32HeadLength,
                       MIN(stJpdStreamBuf.u32TailLength, video_buf->length - stJpdStreamBuf.u32HeadLength));
            }

            stJpdStreamBuf.u32ContentLength = video_buf->length;

            s32Ret = MI_JPD_PutStreamBuf(JPD_DEV_ID, JPD_CHN_ID, &stJpdStreamBuf);
            if(MI_SUCCESS != s32Ret)
            {
                DEMO_ERR(video_handle, "MI_JPD_PutStreamBuf: 0x%x.\n", s32Ret);
                return -1;
            }
            break;

        case V4L2_PIX_FMT_H264:
        case V4L2_PIX_FMT_H265:
            stVdecVideoStream.bEndOfFrame = true;
            stVdecVideoStream.bEndOfStream = false;
            stVdecVideoStream.pu8Addr = video_buf->buf;
            stVdecVideoStream.u32Len = video_buf->length;
            stVdecVideoStream.u64PTS = -1;

            s32Ret = MI_VDEC_SendStream(VDEC_DEV_ID, VDEC_CHN_ID, &stVdecVideoStream, -1);
            if(MI_SUCCESS != s32Ret)
            {
                DEMO_ERR(video_handle, "MI_VDEC_SendStream: 0x%x.\n", s32Ret);
                return -1;
            }
            break;

        default:
            break;
    }

    return MI_SUCCESS;
}

static void Sig_Handler(int signum)
{
    if(signum == SIGINT)
    {
        printf("catch Ctrl + C, exit normally.\n");
        g_exit = true;
    }
}

static void help_message(char **arg)
{
    printf("\n");
    printf("usage: %s\n", arg[0]);
    printf(" -a: /dev/snd/pcmC[X]D0C (default 0)\n");
    printf(" -b: v4l2 buffer count (default 3)\n");
    printf(" -m: run mode [1]:Video Stream [2]:Video Control [4]:Audio Stream In [8]:Audio Stream Out [16]:Audio Control (default 1)\n");
    printf(" -v: /dev/video[X] (default 0)\n");
    printf(" -d: buf handle mode [0]:No Handle Mode [1]:File Handle Mode [2]:MI Handle Mode (default 0)\n");
    printf(" -p: save directory (default /mnt)\n");
    printf(" -t: debug level (default 1)\n");
    printf(" -h: help message\n");
    printf("\nExample: %s -a1 -v1 -b5 -m1\n", arg[0]);
    printf("\n");
}

#define BUF_NUM 1
buffer_object_t _g_buf_obj[BUF_NUM];
int _g_drm_flag = 0;
void  int_buf_obj(Video_Handle_t *video_handle)
{
    int dev_fd = 0;
    _g_buf_obj[0].format = 0;
    _g_buf_obj[0].vdec_info.format = DRM_FORMAT_NV12;
    _g_buf_obj[0].vdec_info.pixelformat = video_handle->video_info.pixelformat;
    if(_g_drm_flag == 0)
    {
        dev_fd = sstar_drm_open();//Only open once
        if(dev_fd < 0)
        {
            printf("sstar_drm_open fail\n");
            return;
        }
        _g_buf_obj[0].fd = dev_fd;
        _g_drm_flag = 1;
        printf("sstar_drm_open success!\n");
    }
    _g_buf_obj[0].vdec_info.plane_type = MOPG;
    _g_buf_obj[0].connector_type = DRM_MODE_CONNECTOR_DPI;
    sstar_drm_getattr(&_g_buf_obj[0]);

    _g_buf_obj[0].vdec_info.v_src_width = ALIGN_BACK(video_handle->video_info.width, ALIGN_NUM);
    _g_buf_obj[0].vdec_info.v_src_height = ALIGN_BACK(video_handle->video_info.height, ALIGN_NUM);
    _g_buf_obj[0].vdec_info.v_src_stride = _g_buf_obj[0].vdec_info.v_src_width;
    _g_buf_obj[0].vdec_info.v_src_size = (_g_buf_obj[0].vdec_info.v_src_height * _g_buf_obj[0].vdec_info.v_src_stride * 3)/2;

    _g_buf_obj[0].vdec_info.v_out_x = 0;
    _g_buf_obj[0].vdec_info.v_out_y = 0;
    _g_buf_obj[0].vdec_info.v_out_width = ALIGN_BACK(video_handle->video_info.width, ALIGN_NUM);
    _g_buf_obj[0].vdec_info.v_out_height = ALIGN_BACK(video_handle->video_info.height, ALIGN_NUM);
    _g_buf_obj[0].vdec_info.v_out_stride = _g_buf_obj[0].vdec_info.v_out_width;
    _g_buf_obj[0].vdec_info.v_out_size = (_g_buf_obj[0].vdec_info.v_out_height * _g_buf_obj[0].vdec_info.v_out_stride * 3)/2;
    _g_buf_obj[0].vdec_info.v_bframe = 0;
    sem_init(&_g_buf_obj[0].sem_avail, 0, MAX_NUM_OF_DMABUFF);
	printf("buf->fb=%d buf->width=%d buf->height=%d v_src_width=%d v_src_height=%d v_out_width=%d v_out_height=%d\n",_g_buf_obj[0].fd, _g_buf_obj[0].width,
		_g_buf_obj[0].height,_g_buf_obj[0].vdec_info.v_src_width,_g_buf_obj[0].vdec_info.v_src_height,_g_buf_obj[0].vdec_info.v_out_width,_g_buf_obj[0].vdec_info.v_out_height);
}

static void *uvc_stream(void *arg)
{
    int ret = 0, frame_cnt = 0;
    struct timeval tv_before, tv_after;
    pthread_t tid_drm_buf_thread;
    pthread_t tid_enqueue_buf_thread;
    Video_Handle_t *video_handle = (Video_Handle_t *)arg;

    ret = video_enum_format(video_handle);
    if(ret != 0)
        pthread_exit(&ret);

    ret = video_set_format(video_handle);
    if(ret != 0)
        pthread_exit(&ret);

    if(g_buf_handle_mode == BUF_HANDLE_MODE_MI)
    {
        _g_buf_obj[0].bExit = 0;
        _g_buf_obj[0].bExit_second = 0;
        int_buf_obj(video_handle);
        sstar_drm_init(&_g_buf_obj[0]);
        creat_dmabuf_queue(&_g_buf_obj[0]);
        ret = St_VideoModuleInit(&_g_buf_obj[0]);
        if(ret != 0)
            pthread_exit(&ret);
        creat_outport_dmabufallocator(&_g_buf_obj[0]);
        pthread_create(&tid_enqueue_buf_thread, NULL, enqueue_buffer_loop, (void*)&_g_buf_obj[0]);
        pthread_create(&tid_drm_buf_thread, NULL, drm_buffer_loop, (void*)&_g_buf_obj[0]);
    }

    ret = video_streamon(video_handle, g_buf_cnt);
    if(ret != 0)
        pthread_exit(&ret);

    Video_Buffer_t video_buf;
    memset(&video_buf, 0, sizeof(Video_Buffer_t));

    if(gettimeofday(&tv_before, NULL))
        DEMO_WRN(video_handle, "gettimeofday begin: %s.\n", strerror(errno));

    while(!g_exit)
    {
        ret = video_get_buf(video_handle, &video_buf);
        if(ret == 0)
        {
            if(g_buf_handle_mode == BUF_HANDLE_MODE_FILE)
            {
                char path[64] = {0};
                snprintf(path, 64, "%s/%dx%d.%s", g_file_path, video_handle->video_info.width,
                            video_handle->video_info.height, format_fcc_to_str(video_handle->video_info.pixelformat));
                ret = video_dump_buf(video_handle, &video_buf, path, DUMP_STREAM);
            }
            else if(g_buf_handle_mode == BUF_HANDLE_MODE_MI)
                ret = St_Video_Preview(video_handle, &video_buf);
            else if(g_buf_handle_mode == BUF_HANDLE_MODE_NONE)
                DEMO_INFO(video_handle, "Buffer Length: %d bytes.\n", video_buf.length);

            video_put_buf(video_handle, &video_buf);

            if(ret != 0)
                break;
        }
        else if(ret == -EAGAIN)
            continue;
        else
            break;

        if(gettimeofday(&tv_after, NULL))
            DEMO_WRN(video_handle, "gettimeofday end: %s.\n", strerror(errno));

        if((tv_after.tv_sec - tv_before.tv_sec)*1000*1000 + (tv_after.tv_usec - tv_before.tv_usec) >= 1*1000*1000)
        {
            DEMO_INFO(video_handle, "Current Frame Rate: %d fps.\n", frame_cnt);
            frame_cnt = 0;

            if(gettimeofday(&tv_before, NULL))
                DEMO_WRN(video_handle, "gettimeofday update: %s.\n", strerror(errno));
        }
        else
            frame_cnt++;
    }

    ret = video_streamoff(video_handle);
    if(ret != 0)
        pthread_exit(&ret);

    if(g_buf_handle_mode == BUF_HANDLE_MODE_MI)
    {
        _g_buf_obj[0].bExit = 1;
        if(tid_drm_buf_thread)
        {
            pthread_join(tid_drm_buf_thread, NULL);
        }
        if(tid_enqueue_buf_thread)
        {
            pthread_join(tid_enqueue_buf_thread, NULL);
        }
        destory_dmabuf_queue(&_g_buf_obj[0]);
		sstar_drm_deinit(&_g_buf_obj[0]);
        ret = St_VideoModuleDeinit(&_g_buf_obj[0]);
        if(ret != 0)
            pthread_exit(&ret);
        if(_g_drm_flag == 1)
    	    sstar_drm_close(_g_buf_obj[0].fd);
    }

    pthread_exit(&ret);
}

static void *uvc_control(void *arg)
{
    int ret = 0, xu_data = REQUEST_IFRAME, pu_id = V4L2_CID_BASE, pu_value = 0;

    Video_Handle_t *video_handle = (Video_Handle_t *)arg;

    video_enum_standard_control(video_handle);

    while(!g_exit)
    {
        ret = video_send_extension_control(video_handle,
                        UVC_VC_EXTENSION2_UNIT_ID,
                        CUS_XU_SET_ISP,
                        UVC_SET_CUR,
                        sizeof(xu_data),
                        (void *)&xu_data);
        if(ret != 0)
            break;

        DEMO_INFO(video_handle, "Send XU Success.\n");

        ret = video_send_standard_control(video_handle, pu_id, &pu_value, CONTROL_SET);
        if(ret != 0)
            break;

        ret = video_send_standard_control(video_handle, pu_id, &pu_value, CONTROL_GET);
        if(ret != 0)
            break;

        DEMO_INFO(video_handle, "Send PU Success.\n");

        sleep(3);
    }

    pthread_exit(&ret);
}

#ifdef AUDIO_ENABLE

static void *uac_recv_stream(void *arg)
{
    int ret = 0, frame_cnt = 0;
    struct timeval tv_before, tv_after;

    Audio_Handle_t *audio_handle = (Audio_Handle_t *)arg;

    if(g_buf_handle_mode == BUF_HANDLE_MODE_MI)
    {
        ret = St_AudioModuleInit(audio_handle, g_run_mode);
        if(ret != 0)
            pthread_exit(&ret);
    }

    Audio_Buffer_t audio_buf;
    memset(&audio_buf, 0, sizeof(Audio_Buffer_t));

    if(gettimeofday(&tv_before, NULL))
        DEMO_WRN(audio_handle, "gettimeofday begin: %s.\n", strerror(errno));

    while(!g_exit)
    {
        ret = audio_get_buf(audio_handle, &audio_buf, AS_IN_MODE);
        if(ret == 0)
        {
            if(g_buf_handle_mode == BUF_HANDLE_MODE_FILE)
            {
                char path[64] = {0};
                snprintf(path, 64, "%s/%dK_%dbit_%dch.pcm", g_file_path, audio_handle->audio_info[0].pcm_config.rate,
                            pcm_format_to_bits(audio_handle->audio_info[0].pcm_config.format), audio_handle->audio_info[0].pcm_config.channels);
                ret = audio_dump_buf(audio_handle, &audio_buf, path, AS_IN_MODE);
            }
            else if(g_buf_handle_mode == BUF_HANDLE_MODE_MI)
                ret = St_Audio_Preview(audio_handle, &audio_buf);
            else if(g_buf_handle_mode == BUF_HANDLE_MODE_NONE)
                DEMO_INFO(audio_handle, "Buffer Length: %d bytes.\n", audio_buf.length);

            ret = audio_put_buf(audio_handle, &audio_buf, AS_IN_MODE);

            if(ret != 0)
                break;
        }
        else
            break;

        if(gettimeofday(&tv_after, NULL))
            DEMO_WRN(audio_handle, "gettimeofday end: %s.\n", strerror(errno));

        if((tv_after.tv_sec - tv_before.tv_sec)*1000*1000 + (tv_after.tv_usec - tv_before.tv_usec) >= 1*1000*1000)
        {
            DEMO_INFO(audio_handle, "Current Sample Rate: %d Hz.\n", frame_cnt * audio_handle->audio_info[0].pcm_config.period_size);
            frame_cnt = 0;

            if(gettimeofday(&tv_before, NULL))
                DEMO_WRN(audio_handle, "gettimeofday update: %s.\n", strerror(errno));
        }
        else
            frame_cnt++;
    }

    if(g_buf_handle_mode == BUF_HANDLE_MODE_MI)
    {
        ret = St_AudioModuleDeinit(audio_handle, g_run_mode);
        if(ret != 0)
            pthread_exit(&ret);
    }

    pthread_exit(&ret);
}

static void *uac_send_stream(void *arg)
{
    int ret = 0, frame_cnt = 0;
    struct timeval tv_before, tv_after;

    Audio_Handle_t *audio_handle = (Audio_Handle_t *)arg;

    if(g_buf_handle_mode == BUF_HANDLE_MODE_MI)
    {
        ret = St_AudioModuleInit(audio_handle, g_run_mode);
        if(ret != 0)
            pthread_exit(&ret);
    }

    Audio_Buffer_t audio_buf;
    memset(&audio_buf, 0, sizeof(Audio_Buffer_t));

    if(gettimeofday(&tv_before, NULL))
        DEMO_WRN(audio_handle, "gettimeofday begin: %s.\n", strerror(errno));

    while(!g_exit)
    {
        ret = audio_get_buf(audio_handle, &audio_buf, AS_OUT_MODE);
        if(ret == 0)
        {
            if(g_buf_handle_mode == BUF_HANDLE_MODE_FILE)
            {
                char path[64] = {0};
                snprintf(path, 64, "%s/%dK_%dbit_%dch.pcm", g_file_path, audio_handle->audio_info[1].pcm_config.rate,
                            pcm_format_to_bits(audio_handle->audio_info[1].pcm_config.format), audio_handle->audio_info[1].pcm_config.channels);
                ret = audio_dump_buf(audio_handle, &audio_buf, path, AS_OUT_MODE);
            }
            else if(g_buf_handle_mode == BUF_HANDLE_MODE_MI)
                ret = St_Audio_Grab(audio_handle, &audio_buf);
            else if(g_buf_handle_mode == BUF_HANDLE_MODE_NONE)
                DEMO_INFO(audio_handle, "Buffer Length: %d bytes.\n", audio_buf.length);

            ret = audio_put_buf(audio_handle, &audio_buf, AS_OUT_MODE);

            if(ret != 0)
                break;
        }
        else
            break;

        if(gettimeofday(&tv_after, NULL))
            DEMO_WRN(audio_handle, "gettimeofday end: %s.\n", strerror(errno));

        if((tv_after.tv_sec - tv_before.tv_sec)*1000*1000 + (tv_after.tv_usec - tv_before.tv_usec) >= 1*1000*1000)
        {
            DEMO_INFO(audio_handle, "Current Sample Rate: %d Hz.\n", frame_cnt * audio_handle->audio_info[1].pcm_config.period_size);
            frame_cnt = 0;

            if(gettimeofday(&tv_before, NULL))
                DEMO_WRN(audio_handle, "gettimeofday update: %s.\n", strerror(errno));
        }
        else
            frame_cnt++;
    }

    if(g_buf_handle_mode == BUF_HANDLE_MODE_MI)
    {
        ret = St_AudioModuleDeinit(audio_handle, g_run_mode);
        if(ret != 0)
            pthread_exit(&ret);
    }

    pthread_exit(&ret);
}

static void *uac_control(void *arg)
{
    int ret = 0, fu_id = 0, fu_value = 0;

    Audio_Handle_t *audio_handle = (Audio_Handle_t *)arg;

    audio_enum_standard_control(audio_handle);

    while(!g_exit)
    {
        ret = audio_send_standard_control(audio_handle, fu_id, &fu_value, CONTROL_SET);
        if(ret != 0)
            break;

        ret = audio_send_standard_control(audio_handle, fu_id, &fu_value, CONTROL_GET);
        if(ret != 0)
            break;

        DEMO_INFO(audio_handle, "Send FU Success.\n");

        sleep(3);
    }

    pthread_exit(&ret);
}

#endif

int main(int argc, char **argv)
{
    int result = 0;
    char path[20] = {};

    signal(SIGINT, Sig_Handler);

    demo_debug_level = DEMO_DBG_ERR;

    while((result = getopt(argc, argv, "a:b:m:v:d:p:t:h")) != -1)
    {
        switch(result)
        {
            case 'a':
                g_audio_index = strtol(optarg, NULL, 10);
                break;

            case 'b':
                g_buf_cnt = strtol(optarg, NULL, 10);
                break;

            case 'm':
                g_run_mode = strtol(optarg, NULL, 10);
                break;

            case 'v':
                g_video_index = strtol(optarg, NULL, 10);
                break;

            case 'd':
                g_buf_handle_mode = strtol(optarg, NULL, 10);
                break;

            case 'p':
                strcpy(g_file_path, optarg);
                break;

            case 't':
                demo_debug_level = strtol(optarg, NULL, 10);
                break;

            case 'h':
                help_message(argv);
                return 0;

            default:
                break;
        }
    }

    if(g_buf_handle_mode == BUF_HANDLE_MODE_MI)
        ExecFunc(MI_SYS_Init(0), MI_SUCCESS);

    Video_Handle_t video_handle;
#ifdef AUDIO_ENABLE
    Audio_Handle_t audio_handle;
#endif
    if(g_run_mode & VS_MODE || g_run_mode & VC_MODE)
    {
        sprintf(path, "/dev/video%d", g_video_index);
        memcpy(video_handle.path, path, 20);

        if(video_init(&video_handle) != 0)
            return -1;

        if(g_run_mode & VS_MODE)
        {
            pthread_create(&vs_tid, NULL, uvc_stream, &video_handle);
        }
        if(g_run_mode & VC_MODE)
            pthread_create(&vc_tid, NULL, uvc_control, &video_handle);
    }

#ifdef AUDIO_ENABLE
    if(g_run_mode & AS_IN_MODE || g_run_mode & AS_OUT_MODE || g_run_mode & AC_MODE)
    {
        sprintf(path, "/dev/snd/pcmC%dD0", g_audio_index);
        memcpy(audio_handle.path, path, 20);
        audio_handle.card = g_audio_index;
        audio_handle.device = 0;

        if(audio_init(&audio_handle, g_run_mode) != 0)
            return -1;

        if(g_run_mode & AS_IN_MODE)
            pthread_create(&as_in_tid, NULL, uac_recv_stream, &audio_handle);

        if(g_run_mode & AS_OUT_MODE)
            pthread_create(&as_out_tid, NULL, uac_send_stream, &audio_handle);

        if(g_run_mode & AC_MODE)
            pthread_create(&ac_tid, NULL, uac_control, &audio_handle);

        if(g_run_mode & AS_IN_MODE)
            pthread_join(as_in_tid, NULL);

        if(g_run_mode & AS_OUT_MODE)
            pthread_join(as_out_tid, NULL);

        if(g_run_mode & AC_MODE)
            pthread_join(ac_tid, NULL);

        if(audio_deinit(&audio_handle, g_run_mode) != 0)
        return -1;
    }
#endif
    if(g_run_mode & VS_MODE || g_run_mode & VC_MODE)
    {
        if(g_run_mode & VS_MODE)
            pthread_join(vs_tid, NULL);

        if(g_run_mode & VC_MODE)
            pthread_join(vc_tid, NULL);

        if(video_deinit(&video_handle) != 0)
            return -1;
    }

    if(g_buf_handle_mode == BUF_HANDLE_MODE_MI)
        ExecFunc(MI_SYS_Exit(0), MI_SUCCESS);

    return 0;
}

