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

#include "sstar_hdmi_rx.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

static MI_U8 defaultEdid[256] = {
        0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x4D, 0x27, 0x72, 0x09, 0x01, 0x00, 0x00,
        0x00, 0x07, 0x1E, 0x01, 0x03, 0x80, 0x73, 0x41, 0x78, 0x0A, 0xCF, 0x74, 0xA3, 0x57, 0x4C,
        0xB0, 0x23, 0x09, 0x48, 0x4C, 0x21, 0x08, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38,
        0x2D, 0x40, 0x58, 0x2C, 0x45, 0x00, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E, 0x01, 0x1D, 0x00,
        0xBC, 0x52, 0xD0, 0x1E, 0x20, 0xB8, 0x28, 0x55, 0x40, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E,
        0x00, 0x00, 0x00, 0xFC, 0x00, 0x53, 0x69, 0x67, 0x6D, 0x61, 0x73, 0x74, 0x61, 0x72, 0x0A,
        0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFD, 0x00, 0x3B, 0x46, 0x1F, 0x8C, 0x3C, 0x00, 0x0A,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x00, 0x02, 0x03, 0x3E, 0xF0, 0x53, 0x10, 0x1F,
        0x14, 0x05, 0x13, 0x04, 0x20, 0x22, 0x3C, 0x3E, 0x12, 0x16, 0x03, 0x07, 0x11, 0x15, 0x02,
        0x06, 0x01, 0x2F, 0x09, 0x7F, 0x05, 0x15, 0x07, 0x50, 0x57, 0x07, 0x00, 0x3D, 0x07, 0xC0,
        0x5F, 0x7E, 0x01, 0x83, 0x01, 0x00, 0x00, 0x6E, 0x03, 0x0C, 0x00, 0x10, 0x00, 0xB8, 0x3C,
        0x2F, 0x00, 0x80, 0x01, 0x02, 0x03, 0x04, 0xE2, 0x00, 0xFB, 0x01, 0x1D, 0x00, 0x72, 0x51,
        0xD0, 0x1E, 0x20, 0x6E, 0x28, 0x55, 0x00, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E, 0x8C, 0x0A,
        0xD0, 0x8A, 0x20, 0xE0, 0x2D, 0x10, 0x10, 0x3E, 0x96, 0x00, 0x13, 0x8E, 0x21, 0x00, 0x00,
        0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x05,
};

pthread_t tid_hdmi_rx_plug_detection_thread;
int bHdmi_Rx_plug_Exit = 0;

#define SIGNAL_MONITOR_COLOR_FORMAT_HDMI_2_HVP(__hvp, __hdmirx) \
    do                                                          \
    {                                                           \
        switch (__hdmirx)                                       \
        {                                                       \
            case E_MI_HDMIRX_PIXEL_FORMAT_RGB:                  \
                __hvp = E_MI_HVP_COLOR_FORMAT_RGB444;           \
                break;                                          \
            case E_MI_HDMIRX_PIXEL_FORMAT_YUV444:               \
                __hvp = E_MI_HVP_COLOR_FORMAT_YUV444;           \
                break;                                          \
            case E_MI_HDMIRX_PIXEL_FORMAT_YUV422:               \
                __hvp = E_MI_HVP_COLOR_FORMAT_YUV422;           \
                break;                                          \
            case E_MI_HDMIRX_PIXEL_FORMAT_YUV420:               \
                __hvp = E_MI_HVP_COLOR_FORMAT_YUV420;           \
                break;                                          \
            default:                                            \
                __hvp = E_MI_HVP_COLOR_FORMAT_MAX;              \
                break;                                          \
        }                                                       \
    } while(0)
#define SIGNAL_MONITOR_COLOR_DEPTH_HDMI_2_HVP(__hvp, __hdmirx) \
do                                                             \
    {                                                          \
        switch (__hdmirx)                                      \
        {                                                      \
            case E_MI_HDMIRX_PIXEL_BITWIDTH_8BIT:              \
                __hvp = E_MI_HVP_COLOR_DEPTH_8;                \
                break;                                         \
            case E_MI_HDMIRX_PIXEL_BITWIDTH_10BIT:             \
                __hvp = E_MI_HVP_COLOR_DEPTH_10;               \
                break;                                         \
            case E_MI_HDMIRX_PIXEL_BITWIDTH_12BIT:             \
                __hvp = E_MI_HVP_COLOR_DEPTH_12;               \
                break;                                         \
            default:                                           \
                __hvp = E_MI_HVP_COLOR_DEPTH_MAX;              \
                break;                                         \
        }                                                      \
    } while(0)
#define SIGNAL_MONITOR_PIXEX_REPETITIVE_HDMI_2_HVP(__hvp, __hdmirx) \
do                                                                  \
    {                                                               \
        switch (__hdmirx)                                           \
        {                                                           \
            case E_MI_HDMIRX_OVERSAMPLE_1X:                         \
                __hvp = FALSE;                                      \
                break;                                              \
            case E_MI_HDMIRX_OVERSAMPLE_2X:                         \
            case E_MI_HDMIRX_OVERSAMPLE_3X:                         \
            case E_MI_HDMIRX_OVERSAMPLE_4X:                         \
                __hvp = TRUE;                                       \
                break;                                              \
            default:                                                \
                __hvp = FALSE;                                      \
                break;                                              \
        }                                                           \
    } while(0)
    
//static int signal_monitor_sync_hdmi_paramter_2_hvp(const MI_HDMIRX_TimingInfo_t *pstTimingInfo)
//{
//    MI_S32 ret = 0;
//    MI_HVP_ChannelParam_t stChnParam;
//    MI_HVP_DEV dev = 0;
//    MI_HVP_CHN chn = 0;
//
//    memset(&stChnParam, 0, sizeof(MI_HVP_ChannelParam_t));
//    ret = MI_HVP_GetChannelParam(dev, chn, &stChnParam);
//    if (ret != MI_SUCCESS)
//    {
//        printf("Get HVP Param error! Dev %d Chn %d\n", dev, chn);
//        return -1;
//    }
//    SIGNAL_MONITOR_PIXEX_REPETITIVE_HDMI_2_HVP(stChnParam.stChnSrcParam.bPixelRepetitive, pstTimingInfo->eOverSample);
//    SIGNAL_MONITOR_COLOR_FORMAT_HDMI_2_HVP(stChnParam.stChnSrcParam.enInputColor, pstTimingInfo->ePixelFmt);
//    SIGNAL_MONITOR_COLOR_DEPTH_HDMI_2_HVP(stChnParam.stChnSrcParam.enColorDepth, pstTimingInfo->eBitWidth);
//    ret = MI_HVP_SetChannelParam(dev, chn, &stChnParam);
//    if (ret != MI_SUCCESS)
//    {
//        printf("Set HVP param error! Dev %d Chn %d\n", dev, chn);
//        return -1;
//    }
//    return 0;
//}

//static void signal_monitor_hdmi(void)
//{
//    int select_ret = 0;
//    MI_HVP_DEV dev = 0;
//    MI_S32 ret = 0;
//    MI_S32 s32Fd = -1;
//    MI_BOOL bTrigger;
//    fd_set read_fds;
//    struct timeval tv;
//    MI_HDMIRX_PortId_e port_id = E_MI_HDMIRX_PORT0;
//    MI_HVP_SignalStatus_e cur_signal_status = E_MI_HVP_SIGNAL_UNSTABLE;
//    MI_HVP_SignalTiming_t cur_signal_timing = {0, 0, 0, 0};
//    MI_HDMIRX_TimingInfo_t cur_hdmirx_timing_info;
//    static MI_HVP_SignalTiming_t last_signal_timing = {0, 0, 0, 0};
//    static MI_HDMIRX_TimingInfo_t last_hdmirx_timing_info;
//
//    ret = MI_HVP_GetResetEventFd(dev, &s32Fd);
//    if (ret != MI_SUCCESS)
//    {
//        printf("Get Reset fd Errr, Hvp Dev%d!\n", dev);
//        return;
//    }
//    FD_ZERO(&read_fds);
//    FD_SET(s32Fd, &read_fds);
//    tv.tv_sec = 0;
//    tv.tv_usec = 100 * 1000;
//    select_ret = select(s32Fd + 1, &read_fds, NULL, NULL, &tv);
//    if (select_ret < 0)
//    {
//        printf("Reset fd select error!!\n");
//        goto EXIT_FD;
//    }
//    ret = MI_HVP_GetResetEvent(dev, &bTrigger);
//    if (ret != MI_SUCCESS)
//    {
//        printf("Get Reset cnt Errr, Hvp Dev%d!\n", dev);
//        goto EXIT_FD;
//    }
//    ret = MI_HVP_GetSourceSignalStatus(dev, &cur_signal_status);
//    if (ret != MI_SUCCESS)
//    {
//        printf("Get Signal Status Errr, Hvp Dev%d!\n", dev);
//        goto EXIT_RST_CNT;
//    }
//    ret = MI_HVP_GetSourceSignalTiming(dev, &cur_signal_timing);
//    if (ret != MI_SUCCESS)
//    {
//        printf("Get Signal Timing Errr, Hvp Dev%d!\n", dev);
//        goto EXIT_RST_CNT;
//    }
//    ret = MI_HDMIRX_GetTimingInfo(port_id, &cur_hdmirx_timing_info);
//    if (ret != MI_SUCCESS)
//    {
//        printf("Get Hdmirx Timing error!\n");
//        goto EXIT_RST_CNT;
//    }
//    if (cur_signal_timing.u16Width != last_signal_timing.u16Width
//        || cur_signal_timing.u16Height!= last_signal_timing.u16Height
//        || cur_signal_timing.bInterlace!= last_signal_timing.bInterlace
//        || cur_hdmirx_timing_info.eOverSample != last_hdmirx_timing_info.eOverSample
//        || cur_hdmirx_timing_info.ePixelFmt != last_hdmirx_timing_info.ePixelFmt
//        || cur_hdmirx_timing_info.eBitWidth != last_hdmirx_timing_info.eBitWidth)
//    {
//        printf("Get Signal St: %s W: %d H: %d, Fps: %d bInterlace: %d OverSample: %d HdmirxFmt: %d BitWidth: %d\n",
//               (cur_signal_status == E_MI_HVP_SIGNAL_STABLE ? " stable" : " unstable"),
//               cur_signal_timing.u16Width, cur_signal_timing.u16Height,
//               cur_signal_timing.u16Fpsx100, (int)cur_signal_timing.bInterlace,
//               cur_hdmirx_timing_info.eOverSample, cur_hdmirx_timing_info.ePixelFmt,
//               cur_hdmirx_timing_info.eBitWidth);
//        if (last_signal_timing.u16Fpsx100 && !cur_signal_timing.u16Fpsx100)
//        {
//            printf("Signal unstable.\n");
//        }
//        else if (!last_signal_timing.u16Fpsx100  && cur_signal_timing.u16Fpsx100)
//        {
//            printf("Signal stable.\n");
//        }
//        signal_monitor_sync_hdmi_paramter_2_hvp(&cur_hdmirx_timing_info);
//        last_signal_timing = cur_signal_timing;
//        last_hdmirx_timing_info = cur_hdmirx_timing_info;
//    }
//EXIT_RST_CNT:
//    if (bTrigger)
//    {
//        printf("Get reset event!!\n");
//        MI_HVP_ClearResetEvent(dev);
//    }
//EXIT_FD:
//    MI_HVP_CloseResetEventFd(dev, s32Fd);
//}



void* hdmi_rx_plug_detection(void* param)
{
	int i, ret;
	MI_S32 s32HDMIRxFd;
	fd_set set; 
	struct timeval tv;
    //char TxtimingCmd[MAX_SYSCMD_LEN];
    MI_HVP_DEV HvpDevId = 0;
    MI_HVP_CHN HvpChnId = 0;
    MI_HVP_DeviceAttr_t HvpDevAttr;
    MI_HVP_ChannelAttr_t HvpChnAttr;
    MI_HVP_ChannelParam_t HvpChnParam;
    //MI_SYS_WindowRect_t CropInfo;
    MI_SYS_ChnPort_t OutputPort;
    MI_HDMIRX_TimingInfo_t TimingInfo;
	MI_HDMIRX_SigStatus_e eSignalStatus = E_MI_HDMIRX_SIG_UNSTABLE;
	MI_BOOL bTrigger = 0;
	buffer_object_t * buf_obj = (buffer_object_t *)param;
    vdec_info_t vdec_info = buf_obj->vdec_info;
	MI_HVP_ColorFormat_e eColor_format;
	MI_HVP_ColorDepth_e eColor_depth;
	MI_BOOL bPixel_Repetitive;
	int hvp_already_init = 0;

	s32HDMIRxFd = MI_HDMIRX_GetFd(E_MI_HDMIRX_PORT0);
	
    while (!bHdmi_Rx_plug_Exit)
    {
    	FD_ZERO(&set);       /* 将set清零使集合中不含任何s32HDMIRxFd */
		FD_SET(s32HDMIRxFd , &set);    /* 将s32HDMIRxFd 加入set集合 */
		tv.tv_sec = 0;
        tv.tv_usec = 500 * 1000;
		ret = select(s32HDMIRxFd+1, &set, NULL, NULL, &tv);
		if(ret <0 )
		{
			printf("selcet err \n");
			return -1;
		}
		else if (ret == 0)
		{
			//printf("selcet timeout \n");
			//return -1;
		}
		else
		{
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
			if(FD_ISSET(s32HDMIRxFd, &set)) /* 测试fd是否在set集合中*/
			{
			    ret = MI_HDMIRX_GetSignalStatus(E_MI_HDMIRX_PORT0, &eSignalStatus);
				if(ret != MI_SUCCESS)
				{
					return -1;
				}
			    if(eSignalStatus == E_MI_HDMIRX_SIG_SUPPORT)
			    {
			        if(hvp_already_init == 0)
		        	{
					    HvpDevAttr.enSrcType = (MI_HVP_SourceType_e)E_MI_HVP_SRC_TYPE_HDMI;
					    ExecFunc(MI_HVP_CreateDevice(HvpDevId, &HvpDevAttr), MI_SUCCESS);
					
					    ExecFunc(MI_HDMIRX_GetTimingInfo(E_MI_HDMIRX_PORT0, &TimingInfo), MI_SUCCESS);
					    if (TimingInfo.bInterlace) {
					        TimingInfo.u32Height = TimingInfo.u32Height * 2;
					    } else {
					        TimingInfo.u32Height = TimingInfo.u32Height;
					    }
					    PrintInfo("Timing info: width=%d height=%d\n", TimingInfo.u32Width, TimingInfo.u32Height);
					    PrintInfo("HDMI RX vdec_info.v_out_width %d  vdec_info.v_out_height %d  \n",vdec_info.v_out_width,vdec_info.v_out_height);

						SIGNAL_MONITOR_PIXEX_REPETITIVE_HDMI_2_HVP(bPixel_Repetitive, TimingInfo.eOverSample);
					    SIGNAL_MONITOR_COLOR_FORMAT_HDMI_2_HVP(eColor_format, TimingInfo.ePixelFmt);
					    SIGNAL_MONITOR_COLOR_DEPTH_HDMI_2_HVP(eColor_depth, TimingInfo.eBitWidth);
							
					    memset(&HvpChnAttr, 0, sizeof(MI_HVP_ChannelAttr_t));
					    HvpChnAttr.enFrcMode = E_MI_HVP_FRC_MODE_LOCK_OUT;   // hvp->disp realtime
					    HvpChnAttr.stPqBufModeConfig.u16BufMaxCount = 10;
						HvpChnAttr.stPqBufModeConfig.eDmaColor = eColor_format;
					    HvpChnAttr.stPqBufModeConfig.u16BufMaxWidth = TimingInfo.u32Width;
					    HvpChnAttr.stPqBufModeConfig.u16BufMaxHeight = TimingInfo.u32Height;
					    HvpChnAttr.stPqBufModeConfig.eBufCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
					    HvpChnAttr.stPqBufModeConfig.eFieldType = E_MI_SYS_FIELDTYPE_NONE;
					    ExecFunc(MI_HVP_CreateChannel(HvpDevId, HvpChnId, &HvpChnAttr), MI_SUCCESS);
					
					    memset(&HvpChnParam, 0, sizeof(MI_HVP_ChannelParam_t));
					    ExecFunc(MI_HVP_GetChannelParam(HvpDevId, HvpChnId, &HvpChnParam), MI_SUCCESS);
					    // src
					    HvpChnParam.stChnSrcParam.enPixRepType = bPixel_Repetitive;
					    HvpChnParam.stChnSrcParam.enColorDepth = eColor_depth;
					    HvpChnParam.stChnSrcParam.stCropWin.u16X = (MI_U16)0;
					    HvpChnParam.stChnSrcParam.stCropWin.u16Y = (MI_U16)0;
					    HvpChnParam.stChnSrcParam.stCropWin.u16Width = (MI_U16)TimingInfo.u32Width;
					    HvpChnParam.stChnSrcParam.stCropWin.u16Height = (MI_U16)TimingInfo.u32Height;
					    HvpChnParam.stChnSrcParam.enInputColor = (MI_HVP_ColorFormat_e)E_MI_HVP_COLOR_FORMAT_YUV444;
					    // dst
					    HvpChnParam.stChnDstParam.enColor = (MI_HVP_ColorFormat_e)E_MI_HVP_COLOR_FORMAT_YUV444;
					    HvpChnParam.stChnDstParam.stCropWin.u16X = (MI_U16)0;
					    HvpChnParam.stChnDstParam.stCropWin.u16Y = (MI_U16)0;
					    HvpChnParam.stChnDstParam.stCropWin.u16Width = (MI_U16)vdec_info.v_out_width;
					    HvpChnParam.stChnDstParam.stCropWin.u16Height = (MI_U16)vdec_info.v_out_height;
					    HvpChnParam.stChnDstParam.stDispWin.u16X = (MI_U16)0;
					    HvpChnParam.stChnDstParam.stDispWin.u16Y = (MI_U16)0;
					    HvpChnParam.stChnDstParam.stDispWin.u16Width = (MI_U16)vdec_info.v_out_width;
					    HvpChnParam.stChnDstParam.stDispWin.u16Height = (MI_U16)vdec_info.v_out_height;
					    HvpChnParam.stChnDstParam.u16Width = (MI_U16)vdec_info.v_out_width;
					    HvpChnParam.stChnDstParam.u16Height = (MI_U16)vdec_info.v_out_height;
					    HvpChnParam.stChnDstParam.u16Fpsx100 = (MI_U16)TimingInfo.u32FrameRate * 100;
					    ExecFunc(MI_HVP_SetChannelParam(HvpDevId, HvpChnId, &HvpChnParam), MI_SUCCESS);
					
					    OutputPort.eModId = E_MI_MODULE_ID_HVP;
					    OutputPort.u32DevId = HvpDevId;
					    OutputPort.u32ChnId = HvpChnId;
					    OutputPort.u32PortId = 0;
					    ExecFunc(MI_SYS_SetChnOutputPortDepth(0, &OutputPort, 1, 2), MI_SUCCESS);
					    ExecFunc(MI_HVP_StartChannel(HvpDevId, HvpChnId), MI_SUCCESS);
					    ExecFunc(MI_HVP_GetResetEvent(HvpDevId, &bTrigger), MI_SUCCESS);
						if(bTrigger)
						{
					        printf("Get reset event!!\n");
					        ExecFunc(MI_HVP_ClearResetEvent(HvpDevId), MI_SUCCESS);
						}
						hvp_already_init = 1;
						printf("HDMI RX connect succeed!\n");
		        	}
					else
					{
						printf("HDMI RX stable,no demand!\n");
						break;
					}
			    }
			    else
			    {
			    	if(hvp_already_init == 1)
		        	{
				        ExecFunc(MI_HVP_ClearResetEvent(HvpDevId), MI_SUCCESS);
					    ExecFunc(MI_HVP_StopChannel(HvpDevId, HvpChnId), MI_SUCCESS);
					    ExecFunc(MI_HVP_DestroyChannel(HvpDevId, HvpChnId), MI_SUCCESS);
					    ExecFunc(MI_HVP_DestroyDevice(HvpDevId), MI_SUCCESS);
						hvp_already_init = 0;
						printf("HDMI RX pull out!\n");
			    	}
			    }
			}
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		} 
	}	
	MI_HDMIRX_CloseFd(E_MI_HDMIRX_PORT0);
	
    printf("Thread hdmi_rx_plug_detection exit\n");
    return NULL;
}
#define HDCP_KEY_PATH "/customer/hdcp.bin"
int sstar_init_hdmi_rx(buffer_object_t *buf_obj)
{
    MI_HDMIRX_Edid_t EdidInfo;
    MI_HDMIRX_PortId_e stHdmiRxPort = E_MI_HDMIRX_PORT0;
    ExecFunc(MI_HDMIRX_Init(), MI_SUCCESS);
#ifdef HDCP_KEY_PATH 

    MI_HDMIRX_Hdcp_t stHdmiRxHdcpKey;
    MI_U8 hdcp_key[289];
    memset(hdcp_key, 0, sizeof(hdcp_key));
    FILE* hdcp_key_file = fopen(HDCP_KEY_PATH, "rb");
    if (hdcp_key_file == NULL) {
        printf("Error: hdcp file not found!\n");
    }else{
        uint32_t read_size = fread(hdcp_key, 1, sizeof(hdcp_key), hdcp_key_file);
        if (read_size != sizeof(hdcp_key)) {
            printf("Error: hdcp file size(%u) too short!\n", read_size);

        }
        fclose(hdcp_key_file);
    }
    memset(&stHdmiRxHdcpKey, 0, sizeof(MI_HDMIRX_Hdcp_t));
    stHdmiRxHdcpKey.u64HdcpDataVirAddr = (MI_U64)(uintptr_t)hdcp_key;
    stHdmiRxHdcpKey.u32HdcpLength = sizeof(hdcp_key);
    MI_HDMIRX_LoadHdcp(stHdmiRxPort, &stHdmiRxHdcpKey);
#endif
    MI_HDMIRX_SetHotPlug(stHdmiRxPort, FALSE);    
    memset(&EdidInfo, 0, sizeof(MI_HDMIRX_Edid_t));
    EdidInfo.u32EdidLength = sizeof(defaultEdid);
    EdidInfo.u64EdidDataVirAddr = (MI_U64)defaultEdid;
    ExecFunc(MI_HDMIRX_UpdateEdid(E_MI_HDMIRX_PORT0, &EdidInfo), MI_SUCCESS);
    MI_HDMIRX_SetHotPlug(stHdmiRxPort, TRUE);   

    ExecFunc(MI_HDMIRX_Connect(E_MI_HDMIRX_PORT0), MI_SUCCESS);

	pthread_create(&tid_hdmi_rx_plug_detection_thread, NULL, hdmi_rx_plug_detection, (void*)buf_obj);

    return 0;
}


int sstar_deinit_hdmi_rx()
{
	if(tid_hdmi_rx_plug_detection_thread)
    {
        pthread_join(tid_hdmi_rx_plug_detection_thread, NULL);
    }
	
    ExecFunc(MI_HDMIRX_DisConnect(E_MI_HDMIRX_PORT0), MI_SUCCESS);
    ExecFunc(MI_HDMIRX_DeInit(), MI_SUCCESS);
	printf("HDMI RX deinit!\n");

    return 0;
}

