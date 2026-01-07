#include "sstar_vdf.h"
#include <stdio.h>

static MI_VDF_CHANNEL g_VdfChn = 0;
int bVdfGetExit = 0, bVdfPutExit = 0;
static MI_U32 vdf_width = 0;
static MI_U32 vdf_height = 0;
static int vdf_mb_size = 0;
int sstar_vdf_init(MI_U32 width, MI_U32 height)
{
    MI_VDF_ChnAttr_t stpstAttr;
    memset(&stpstAttr, 0x0, sizeof(MI_VDF_ChnAttr_t));
    stpstAttr.enWorkMode = E_MI_VDF_WORK_MODE_MD;
    stpstAttr.stMdAttr.u8Enable = 1;
    stpstAttr.stMdAttr.u8MdBufCnt  = 4;
    stpstAttr.stMdAttr.u8VDFIntvl  = 0;
    stpstAttr.stMdAttr.ccl_ctrl.u16InitAreaThr = 8;
    stpstAttr.stMdAttr.ccl_ctrl.u16Step = 2;
    stpstAttr.stMdAttr.stMdDynamicParamsIn.sensitivity = 80;
    stpstAttr.stMdAttr.stMdDynamicParamsIn.learn_rate = 2000;
    stpstAttr.stMdAttr.stMdDynamicParamsIn.md_thr = 16;
    stpstAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max = 0;

    stpstAttr.stMdAttr.stMdStaticParamsIn.width     = ALIGN_UP(width, 16);//width nend 16 align;
    vdf_width = stpstAttr.stMdAttr.stMdStaticParamsIn.width;
    stpstAttr.stMdAttr.stMdStaticParamsIn.height  = ALIGN_UP(height, 2);//height nend 2 align;
    vdf_height = stpstAttr.stMdAttr.stMdStaticParamsIn.height;
    stpstAttr.stMdAttr.stMdStaticParamsIn.stride  = ALIGN_UP(width, 16);//16 nend align;
    stpstAttr.stMdAttr.stMdStaticParamsIn.color     = 1;
    stpstAttr.stMdAttr.stMdStaticParamsIn.mb_size = MDMB_MODE_MB_8x8;
    vdf_mb_size = stpstAttr.stMdAttr.stMdStaticParamsIn.mb_size;
    stpstAttr.stMdAttr.stMdStaticParamsIn.md_alg_mode  = MDALG_MODE_FG;//Only support MDALG_MODE_FG
    stpstAttr.stMdAttr.stMdStaticParamsIn.sad_out_ctrl = MDSAD_OUT_CTRL_8BIT_SAD;
    stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.num      = 4;
    stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].x = 0;
    stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].y = 0;
    stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].x = width - 1;
    stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].y = 0;
    stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].x = width - 1;
    stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].y = height - 1;
    stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].x = 0;
    stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].y = height - 1;
    printf("width %u height %u MD width %d height %d stride %d 0(%dx%d) 1(%dx%d) 2(%dx%d) 3(%dx%d)\n", width, height, 
        stpstAttr.stMdAttr.stMdStaticParamsIn.width, 
        stpstAttr.stMdAttr.stMdStaticParamsIn.height, 
        stpstAttr.stMdAttr.stMdStaticParamsIn.stride,
        stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].x,
        stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].y,
        stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].x,
        stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].y,
        stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].x,
        stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].y,
        stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].x,
        stpstAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].y);

    ExecFunc(MI_VDF_Init(), MI_SUCCESS);
    ExecFunc(MI_VDF_CreateChn(g_VdfChn, &stpstAttr), MI_SUCCESS);
    ExecFunc(MI_VDF_Run(E_MI_VDF_WORK_MODE_MD), MI_SUCCESS);
    sleep(1);
    ExecFunc(MI_VDF_EnableSubWindow(g_VdfChn, 0, 0, 1), MI_SUCCESS);

    return 0;
}

int sstar_vdf_bind_scl(MI_SYS_ChnPort_t scl_src_chn_port)
{
    Sys_BindInfo_T stBindInfo;

    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stSrcChnPort.u32DevId = scl_src_chn_port.u32DevId;
    stBindInfo.stSrcChnPort.u32ChnId = scl_src_chn_port.u32ChnId;
    stBindInfo.stSrcChnPort.u32PortId = scl_src_chn_port.u32PortId;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDF;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    // stBindInfo.u32SrcFrmrate = scl_info->scl_src_fps;
    // stBindInfo.u32DstFrmrate = scl_info->scl_dst_fps;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    printf("scl(%d %d %d) bind VDF\n", scl_src_chn_port.u32DevId, scl_src_chn_port.u32ChnId, scl_src_chn_port.u32PortId);
    ExecFunc(bind_port(&stBindInfo), MI_SUCCESS);
}

int sstar_vdf_unbind_scl(MI_SYS_ChnPort_t scl_src_chn_port)
{
    Sys_BindInfo_T stBindInfo;

    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stSrcChnPort.u32DevId = scl_src_chn_port.u32DevId;
    stBindInfo.stSrcChnPort.u32ChnId = scl_src_chn_port.u32ChnId;
    stBindInfo.stSrcChnPort.u32PortId = scl_src_chn_port.u32PortId;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDF;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    // stBindInfo.u32SrcFrmrate = scl_info->scl_src_fps;
    // stBindInfo.u32DstFrmrate = scl_info->scl_dst_fps;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    printf("scl(%d %d %d) unbind VDF\n", scl_src_chn_port.u32DevId, scl_src_chn_port.u32ChnId, scl_src_chn_port.u32PortId);
    ExecFunc(unbind_port(&stBindInfo), MI_SUCCESS);
}

int sstar_vdf_get_result(MI_VDF_Result_t *pstVdfResult)
{
    int ret = MI_SUCCESS;
    ret = MI_VDF_GetResult(g_VdfChn, pstVdfResult, 0);
    return ret;
}

int sstar_vdf_put_result(MI_VDF_Result_t *pstVdfResult)
{
    int ret = MI_SUCCESS;
    ret = MI_VDF_PutResult(g_VdfChn, pstVdfResult);
    return ret;
}

int sstar_vdf_deinit()
{
    STCHECKRESULT(MI_VDF_EnableSubWindow(g_VdfChn, 0, 0, 0));
    STCHECKRESULT(MI_VDF_Stop(E_MI_VDF_WORK_MODE_MD));
    STCHECKRESULT(MI_VDF_DestroyChn(g_VdfChn));
    STCHECKRESULT(MI_VDF_Uninit());
    return 0;
}

int sstar_vdf_get_input_buff(MI_SYS_BUF_HANDLE* bufHandle, MI_U32 width, MI_U32 height, MI_SYS_BufInfo_t* stBufInfo)
{
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_ChnPort_t stSrcChnPort;
    time_t stTime = 0;
    memset(&stBufConf, 0, sizeof(MI_SYS_BufConf_t));
    memset(&stSrcChnPort, 0, sizeof(stSrcChnPort));

    if(bufHandle == NULL || stBufInfo == NULL)
    {
        printf("Err,Null point \n");
        return -1;
    }

    stBufConf.u64TargetPts = time(&stTime);//stTv.tv_sec*1000000 + stTv.tv_usec;
    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.stFrameCfg.u16Width = width;
    stBufConf.stFrameCfg.u16Height = height;
    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.u32Flags = MI_SYS_MAP_VA;
    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

    stSrcChnPort.eModId = E_MI_MODULE_ID_VDF;
    stSrcChnPort.u32ChnId = 0;
    stSrcChnPort.u32DevId = 0;
    stSrcChnPort.u32PortId = 0;

    if(MI_SUCCESS != MI_SYS_ChnInputPortGetBuf(&stSrcChnPort, &stBufConf, stBufInfo, bufHandle, 0))
    {
        // printf("MI_SYS_ChnInputPortGetBuf fail \n");
        return -1;
    }
    return 0;
}

int sstar_vdf_put_input_buff(MI_SYS_BUF_HANDLE bufHandle, MI_SYS_BufInfo_t* stBufInfo)
{
    if(MI_SUCCESS != MI_SYS_ChnInputPortPutBuf(bufHandle, stBufInfo, 0))
    {
        printf("MI_SYS_ChnInputPortGetBuf fail \n");
        return -1;
    }
    return 0;
}

void * sstar_vdf_feed_buff_thread(void * param)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_SYS_FrameBufExtraConfig_t stBufExtraConf = {0};
    MI_S32 s32Fd = 0;
    int u32Index = 0;
    fd_set read_fds;
    MI_SYS_ChnPort_t *stChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    struct timeval TimeoutVal;
    stChnPort = (MI_SYS_ChnPort_t *) param;

    printf("mi %d dev %d chn %d port %d\n", stChnPort->eModId, stChnPort->u32DevId, stChnPort->u32ChnId, stChnPort->u32PortId);

    memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stDstChnPort.eModId = E_MI_MODULE_ID_VDF;
    stDstChnPort.u32ChnId = 0;
    stDstChnPort.u32DevId = 0;
    stDstChnPort.u32PortId = 0;

    s32Ret = MI_SYS_GetFd(stChnPort,&s32Fd);
    if(s32Ret != MI_SUCCESS)
    {
        printf("MI_SYS_GetFd error %d\n", s32Ret);
        return NULL;
    }
    if (MI_SUCCESS != MI_SYS_SetChnOutputPortDepth(0, stChnPort, 2, 4))
    {
        printf("MI_SYS_SetChnOutputPortDepth error \n");
        return NULL;
    }

    stBufExtraConf.u16BufHAlignment = 16;
    stBufExtraConf.u16BufVAlignment = 2;
    s32Ret = MI_SYS_SetChnOutputPortBufExtConf(stChnPort, &stBufExtraConf);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_SetChnOutputPortBufExtConf err:%x, chn:%d,port:%d\n", s32Ret,
        stChnPort->u32ChnId, stChnPort->u32PortId);
        return NULL;
    }
    s32Ret = MI_SYS_SetChnOutputPortUserFrc(stChnPort, 30, 15);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_ SetChnOutputPortUserFrc err:%x, chn:%d,port:%d\n", s32Ret,
            stChnPort->u32ChnId, stChnPort->u32PortId);
        return NULL;
    }

    while (!bVdfPutExit)
    {
        FD_ZERO(&read_fds);
        FD_SET(s32Fd,&read_fds);
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &TimeoutVal);

        if(s32Ret < 0)
        {
            printf("select fail,continue\n");
            usleep(10*1000);
            continue;
        }
        else if(s32Ret == 0)
        {
            printf("GetOutputDataThread select timeout,Exit\n");
            continue;
        }
        else
        {
            if(FD_ISSET(s32Fd,&read_fds))
            {
                usleep(10 * 1000);
                if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(stChnPort, &stBufInfo, &hHandle))
                {
#if 0
                    MI_SYS_BUF_HANDLE bufHandle;
                    MI_SYS_BufInfo_t tmp_stBufInfo;

                    if(0 == sstar_vdf_get_input_buff(&bufHandle, vdf_width, vdf_height, &tmp_stBufInfo))
                    {
                        if(tmp_stBufInfo.stFrameData.u32BufSize == stBufInfo.stFrameData.u32BufSize)
                        {
                            // printf("vdf buff size:%u, scl out buff size:%u\n", tmp_stBufInfo.stFrameData.u32BufSize, stBufInfo.stFrameData.u32BufSize);
                            memcpy(tmp_stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[0], tmp_stBufInfo.stFrameData.u32BufSize);
                            // for (u32Index = 0; u32Index < tmp_stBufInfo.stFrameData.u16Height; u32Index ++)
                            // {
                            //     memcpy(tmp_stBufInfo.stFrameData.pVirAddr[0]+(u32Index*tmp_stBufInfo.stFrameData.u32Stride[0]),
                            //             stBufInfo.stFrameData.pVirAddr[0]+(u32Index*tmp_stBufInfo.stFrameData.u16Width), tmp_stBufInfo.stFrameData.u16Width);
                            // }
                            // for (u32Index = 0; u32Index < tmp_stBufInfo.stFrameData.u16Height / 2; u32Index ++)
                            // {
                            //     memcpy(tmp_stBufInfo.stFrameData.pVirAddr[1]+(u32Index*tmp_stBufInfo.stFrameData.u32Stride[1]),
                            //                 stBufInfo.stFrameData.pVirAddr[1]+(u32Index*tmp_stBufInfo.stFrameData.u16Width)+(stBufInfo.stFrameData.u16Height),tmp_stBufInfo.stFrameData.u16Width);
                            // }
                        }else
                        {
                            printf("check buff size, vdf buff size:%u, scl out buff size:%u\n", tmp_stBufInfo.stFrameData.u32BufSize, stBufInfo.stFrameData.u32BufSize);
                        }
                        sstar_vdf_put_input_buff(bufHandle, &tmp_stBufInfo);
                    }
                    MI_SYS_ChnOutputPortPutBuf(hHandle);
#else
                    s32Ret = MI_SYS_ChnPortInjectBuf(hHandle, &stDstChnPort);
                    if(MI_SUCCESS != s32Ret)
                    {
                        printf("MI_SYS_ChnPortInjectBuf stDstChnPort fail\n");
                    }
#endif
                }else
                {
                    MI_SYS_ChnOutputPortPutBuf(hHandle);
                }

            }
        }
    }

    MI_SYS_CloseFd(s32Fd);

    return NULL;
}

static struct timeval tstart, tend;
void * sstar_vdf_get_result_thread(void * param)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 s32Fd = 0;
    struct timeval TimeoutVal;
    fd_set read_fds;
    MI_U32 frame_count = 0, move_count = 0;
    MI_U32 md_count = 0;
    MI_U32 col = 0, row = 0;
    MI_U8 *FgArry = NULL;
    float time = 0.0;

    col = vdf_width >> (vdf_mb_size + 2);
    row = vdf_height >> (vdf_mb_size + 2);
    FgArry = (MI_U8 *)malloc(col * row + 1);
    if(FgArry == NULL)
    {
        printf("malloc fail, return\n");
        return NULL;
    }
    memset(FgArry, 0, (col * row + 1));

    MI_VDF_Result_t pstVdfResult;
    memset(&pstVdfResult, 0x00, sizeof(MI_VDF_Result_t));
    pstVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_MD;

    MI_SYS_ChnPort_t stChnPort;
    memset(&stChnPort, 0x00, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VDF;
    stChnPort.u32DevId =  0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = 0;

    s32Ret = MI_SYS_GetFd(&stChnPort,&s32Fd);
    gettimeofday(&tstart, NULL);

    while(!bVdfGetExit)
    {
        FD_ZERO(&read_fds);
        FD_SET(s32Fd,&read_fds);
        TimeoutVal.tv_sec = 5;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if(s32Ret < 0)
        {
            printf("select fail,continue\n");
            usleep(10*1000);
            continue;
        }
        else if(s32Ret == 0)
        {
            printf("GetVdfResultThread select timeout,Exit\n");
            continue;
        }
        else
        {
            if(FD_ISSET(s32Fd,&read_fds))
            {
                if(MI_SUCCESS == sstar_vdf_get_result(&pstVdfResult))
                {
                    if(pstVdfResult.stMdResult.u8Enable == 1)
                    {
                        if(pstVdfResult.stMdResult.pstMdResultObj != NULL)
                        {
                            if(pstVdfResult.stMdResult.stSubResultSize.u32RstStatusLen == col * row)
                            {
                                memcpy(FgArry, pstVdfResult.stMdResult.pstMdResultStatus->paddr, col * row);
                                frame_count++;
                                for(int i = 0;i < col;i++)
                                {
                                    for(int j = 0;j < row;j++)
                                    {
                                        if(FgArry[i * col + j] == 255)
                                            md_count++;
                                    }
                                }
                                if(md_count > 0)
                                    move_count++;

                                md_count = 0;
                                // memset(FgArry, 0x0, col * row + 1);
                            }else
                            {
                                printf("Vdf_GetResult check col %u row %u u32RstStatusLen %u\n", col, row, pstVdfResult.stMdResult.stSubResultSize.u32RstStatusLen);
                            }
                        }

                        sstar_vdf_put_result(&pstVdfResult);

                    }
                    else
                    {
                        printf("Vdf_GetResult success u8Enable=%d\n", pstVdfResult.stMdResult.u8Enable);
                        sstar_vdf_put_result(&pstVdfResult);
                    }
                }
                else
                {
                    printf("Vdf_GetResult fail\n");
                }
            }
        }

        gettimeofday(&tend, NULL);
        time = (tend.tv_sec - tstart.tv_sec) * 1000.0 + (tend.tv_usec - tstart.tv_usec) / 1000.0;
        if(frame_count % 15 == 0)
            printf("VDF_MD_Run cost fps %f count %u move count %u\n", frame_count * 1000 / time, frame_count, move_count);
    }
    if(FgArry != NULL)
    {
        free(FgArry);
    }
    return NULL;
}