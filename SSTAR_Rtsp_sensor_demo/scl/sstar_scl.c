#include"sstar_scl.h"

MI_S32 sstar_scl_init(sstar_scl_info_t *scl_info)
{
    MI_SCL_DevAttr_t stCreateDevAttr;
    MI_SCL_ChannelAttr_t stSclChnAttr;
    MI_SCL_ChnParam_t stSclChnParam;
    MI_SCL_OutPortParam_t stSclOutputParam;
    MI_SYS_ChnPort_t stChnPort;
    Sys_BindInfo_T stBindInfo;

	memset(&stCreateDevAttr, 0x0, sizeof(MI_SCL_DevAttr_t));
	stCreateDevAttr.u32NeedUseHWOutPortMask = scl_info->scl_hw_outport_mask;
	ExecFunc(MI_SCL_CreateDevice(scl_info->scl_dev_id, &stCreateDevAttr), MI_SUCCESS);

	memset(&stSclChnAttr, 0x0, sizeof(MI_SCL_ChannelAttr_t));
	ExecFunc(MI_SCL_CreateChannel(scl_info->scl_dev_id, scl_info->scl_chn_id, &stSclChnAttr), MI_SUCCESS);

	memset(&stSclChnParam, 0x0, sizeof(MI_SCL_ChnParam_t));
	stSclChnParam.eRot = scl_info->scl_rotate;
	ExecFunc(MI_SCL_SetChnParam(scl_info->scl_dev_id, scl_info->scl_chn_id, &stSclChnParam), MI_SUCCESS);
	ExecFunc(MI_SCL_StartChannel(scl_info->scl_dev_id, scl_info->scl_chn_id), MI_SUCCESS);
	printf("scl chn start, (%d %d)", (int)scl_info->scl_dev_id, (int)scl_info->scl_chn_id);

    if(scl_info->scl_src_module_inited)
    {
        memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = scl_info->scl_src_chn_port.eModId;
        stBindInfo.stSrcChnPort.u32DevId = scl_info->scl_src_chn_port.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId = scl_info->scl_src_chn_port.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = scl_info->scl_src_chn_port.u32PortId;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
        stBindInfo.stDstChnPort.u32DevId = scl_info->scl_dev_id;
        stBindInfo.stDstChnPort.u32ChnId = scl_info->scl_chn_id;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = scl_info->scl_src_fps;
        stBindInfo.u32DstFrmrate = scl_info->scl_dst_fps;
        stBindInfo.eBindType = scl_info->scl_bind_type;
        ExecFunc(bind_port(&stBindInfo), MI_SUCCESS);
        printf("bind %d(%d %d %d)->SCL(%d %d),bind type:%d ", 
            scl_info->scl_src_chn_port.eModId, scl_info->scl_src_chn_port.u32DevId, 
            scl_info->scl_src_chn_port.u32ChnId, scl_info->scl_src_chn_port.u32PortId, 
            (int)scl_info->scl_dev_id, (int)scl_info->scl_chn_id, (int)scl_info->scl_bind_type);
    }

    memset(&stSclOutputParam, 0x0, sizeof(MI_SCL_OutPortParam_t));
	stSclOutputParam.stSCLOutCropRect.u16X = 0;
	stSclOutputParam.stSCLOutCropRect.u16Y = 0;
	stSclOutputParam.stSCLOutCropRect.u16Width = 0;
	stSclOutputParam.stSCLOutCropRect.u16Height = 0;
	stSclOutputParam.stSCLOutputSize.u16Width = scl_info->scl_out_width;
	stSclOutputParam.stSCLOutputSize.u16Height = scl_info->scl_out_height;
	printf("SCL%d Crop.Width=%d CropRect.Height=%d Out.Width=%d Out.Height=%d\n", (int)scl_info->scl_dev_id, 
        stSclOutputParam.stSCLOutCropRect.u16Width, stSclOutputParam.stSCLOutCropRect.u16Height,
		stSclOutputParam.stSCLOutputSize.u16Width, stSclOutputParam.stSCLOutputSize.u16Height);
	stSclOutputParam.bFlip = FALSE;
	stSclOutputParam.bMirror = FALSE;
	stSclOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stSclOutputParam.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
	ExecFunc(MI_SCL_SetOutputPortParam(scl_info->scl_dev_id, scl_info->scl_chn_id, scl_info->scl_outport, &stSclOutputParam), MI_SUCCESS);

	memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
	stChnPort.eModId = E_MI_MODULE_ID_SCL;
	stChnPort.u32DevId = scl_info->scl_dev_id;
	stChnPort.u32ChnId = scl_info->scl_chn_id;
	stChnPort.u32PortId = scl_info->scl_outport;
	ExecFunc(MI_SYS_SetChnOutputPortDepth(0, &stChnPort, 0, 3), MI_SUCCESS);
	ExecFunc(MI_SCL_EnableOutputPort(scl_info->scl_dev_id, scl_info->scl_chn_id, scl_info->scl_outport), MI_SUCCESS);
    printf("Scl%d Init done \n", scl_info->scl_dev_id);
    return MI_SUCCESS;
}

MI_S32 sstar_scl_deinit(sstar_scl_info_t *scl_info)
{
    if(scl_info->scl_src_module_inited)
    {
        Sys_BindInfo_T stBindInfo;
        memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = scl_info->scl_src_chn_port.eModId;
        stBindInfo.stSrcChnPort.u32DevId = scl_info->scl_src_chn_port.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId = scl_info->scl_src_chn_port.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = scl_info->scl_src_chn_port.u32PortId;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
        stBindInfo.stDstChnPort.u32DevId = scl_info->scl_dev_id;
        stBindInfo.stDstChnPort.u32ChnId = scl_info->scl_chn_id;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = scl_info->scl_src_fps;
        stBindInfo.u32DstFrmrate = scl_info->scl_dst_fps;
        stBindInfo.eBindType = scl_info->scl_bind_type;
        ExecFunc(unbind_port(&stBindInfo), MI_SUCCESS);
    }
	ExecFunc(MI_SCL_DisableOutputPort(scl_info->scl_dev_id, scl_info->scl_chn_id, scl_info->scl_outport), MI_SUCCESS);
	ExecFunc(MI_SCL_StopChannel(scl_info->scl_dev_id, scl_info->scl_chn_id), MI_SUCCESS);
	ExecFunc(MI_SCL_DestroyChannel(scl_info->scl_dev_id, scl_info->scl_chn_id), MI_SUCCESS);
	ExecFunc(MI_SCL_DestroyDevice(scl_info->scl_dev_id), MI_SUCCESS);
    return MI_SUCCESS;
}