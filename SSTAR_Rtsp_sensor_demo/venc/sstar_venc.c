#include "sstar_venc.h"

MI_S32 sstar_venc_init(sstar_venc_info_t *venc_info)
{
    MI_VENC_ChnAttr_t           stVencChnAttr;
    MI_VENC_InputSourceConfig_t stVencInputSourceConfig;
	MI_VENC_InitParam_t stInitParam;
    Sys_BindInfo_T stBindInfo;
	memset(&stInitParam, 0, sizeof(MI_VENC_InitParam_t));
	stInitParam.u32MaxWidth = ALIGN_UP(1920, ALIGN_NUM);
    stInitParam.u32MaxHeight = ALIGN_UP(1080, ALIGN_NUM);
	ExecFunc(MI_VENC_CreateDev(venc_info->venc_dev_id, &stInitParam), MI_SUCCESS);

    /* 设置编码器属性与码率控制属性 */
    memset(&stVencChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    stVencChnAttr.stVeAttr.stAttrH264e.u32PicWidth         = ALIGN_BACK(venc_info->venc_out_width, ALIGN_NUM);
    stVencChnAttr.stVeAttr.stAttrH264e.u32PicHeight        = ALIGN_BACK(venc_info->venc_out_height, ALIGN_NUM);
    stVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth      = ALIGN_BACK(venc_info->venc_out_width, ALIGN_NUM);
    stVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight     = ALIGN_BACK(venc_info->venc_out_height, ALIGN_NUM);
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
    ExecFunc(MI_VENC_CreateChn(venc_info->venc_dev_id, venc_info->venc_chn_id, &stVencChnAttr), MI_SUCCESS);

    /*! 设置输入源的属性
     *  设置了E_MI_VENC_INPUT_MODE_RING_ONE_FRM那APP 在调用 MI_SYS_BindChnPort2 需要设置
     *  E_MI_SYS_BIND_TYPE_HW_RING/和相应 ring buffer 高度
     *  */
    memset(&stVencInputSourceConfig, 0, sizeof(MI_VENC_InputSourceConfig_t));
    stVencInputSourceConfig.eInputSrcBufferMode = E_MI_VENC_INPUT_MODE_NORMAL_FRMBASE;
    ExecFunc(MI_VENC_SetInputSourceConfig(venc_info->venc_dev_id, venc_info->venc_chn_id, &stVencInputSourceConfig), MI_SUCCESS);

    ExecFunc(MI_VENC_SetMaxStreamCnt(venc_info->venc_dev_id, venc_info->venc_chn_id, 4), MI_SUCCESS);

	memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = venc_info->venc_src_chn_port.eModId;
    stBindInfo.stSrcChnPort.u32DevId = venc_info->venc_src_chn_port.u32DevId;
    stBindInfo.stSrcChnPort.u32ChnId = venc_info->venc_src_chn_port.u32ChnId;
    stBindInfo.stSrcChnPort.u32PortId = venc_info->venc_src_chn_port.u32PortId;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = venc_info->venc_dev_id;
    stBindInfo.stDstChnPort.u32ChnId = venc_info->venc_chn_id;
	stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = venc_info->venc_src_fps;
    stBindInfo.u32DstFrmrate = venc_info->venc_dst_fps;
    stBindInfo.eBindType = venc_info->venc_bind_type;
	//stBindInfo.u32BindParam = stSnrPlaneInfo.stCapRect.u16Height;
    ExecFunc(bind_port(&stBindInfo), MI_SUCCESS);
    printf("bind %d(%d %d %d)->VENC(%d %d 0),bind type:%d ", venc_info->venc_src_chn_port.eModId, (int)venc_info->venc_src_chn_port.u32DevId, (int)venc_info->venc_src_chn_port.u32ChnId,
               (int)venc_info->venc_src_chn_port.u32PortId, (int)venc_info->venc_dev_id, (int)venc_info->venc_chn_id, stBindInfo.eBindType);
	ExecFunc(MI_VENC_StartRecvPic(venc_info->venc_dev_id, venc_info->venc_chn_id), MI_SUCCESS);
    return 0;

}

MI_S32 sstar_venc_deinit(sstar_venc_info_t *venc_info)
{
    Sys_BindInfo_T stBindInfo;
    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId = venc_info->venc_src_chn_port.eModId;
    stBindInfo.stSrcChnPort.u32DevId = venc_info->venc_src_chn_port.u32DevId;
    stBindInfo.stSrcChnPort.u32ChnId = venc_info->venc_src_chn_port.u32ChnId;
    stBindInfo.stSrcChnPort.u32PortId = venc_info->venc_src_chn_port.u32PortId;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = venc_info->venc_dev_id;
    stBindInfo.stDstChnPort.u32ChnId = venc_info->venc_chn_id;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = venc_info->venc_src_fps;
    stBindInfo.u32DstFrmrate = venc_info->venc_dst_fps;
    stBindInfo.eBindType = venc_info->venc_bind_type;
    ExecFunc(unbind_port(&stBindInfo), MI_SUCCESS);
	ExecFunc(MI_VENC_StopRecvPic(venc_info->venc_dev_id, venc_info->venc_chn_id), MI_SUCCESS);
    ExecFunc(MI_VENC_DestroyChn(venc_info->venc_dev_id, venc_info->venc_chn_id), MI_SUCCESS);
	ExecFunc(MI_VENC_DestroyDev(venc_info->venc_dev_id), MI_SUCCESS);
    return 0;
}