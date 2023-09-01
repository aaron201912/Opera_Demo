#include "common.h"

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

MI_S32 unbind_port(Sys_BindInfo_T* pstBindInfo)
{
    MI_S32 ret;

    ret = MI_SYS_UnBindChnPort(0, &pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort);

    return ret;
}