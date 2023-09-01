#include "rtsp.h"

#ifndef ASSERT
#define ASSERT(_x_)                                                                    \
		do																					\
		{																					\
			if (!(_x_)) 																	\
			{																				\
				printf("ASSERT FAIL: %s %s %d\n", __FILE__, __PRETTY_FUNCTION__, __LINE__); \
				abort();																	\
			}																				\
		} while (0)
#endif

#ifndef MIN
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif
static int boutputstart = 0;
static int _g_rtsp_num = 2;
static Live555RTSPServer *g_pRTSPServer = NULL;
void *vencGetFrame(void *data)
{
    MI_S32 vencFd = -1;
    MI_S32 s32Ret = MI_SUCCESS;
    int fd = -1;
    MI_U32 i = 0;
    fd_set read_fds;
    struct timeval TimeoutVal;
    MI_VENC_ChnStat_t stStat;
    MI_VENC_Stream_t stStream;
    //MI_VDEC_VideoStream_t stVdecStream;
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_VENC_Pack_t *pstPack = NULL;
    static MI_U32 u32GetBufCnt = 0;
 	MI_VENC_CHN vencChn = 0;
    MI_U32 u32Size = 0;
    MI_U32 u32Maxlen = 0;
    MI_U32 u32Len = 0;
 
    ST_OutputFile_Attr_t *pstOutFileAttr = ((ST_OutputFile_Attr_t *)(data));
    char FilePath[256];
 
    strcpy(FilePath, pstOutFileAttr->FilePath);
    TimeoutVal.tv_sec = 2;
    TimeoutVal.tv_usec = 0;
 	vencChn = pstOutFileAttr->vencChn;
    //保存文件为venc_out.es
    fd = open(FilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    vencFd = MI_VENC_GetFd(0, vencChn);
    if (vencFd <= 0){
        printf("vencGetFrame get channel [%d] fd err\n",vencChn);
        return NULL;
    }
    //printf("vencGetFrame start\n");
    while(!boutputstart){
        FD_ZERO(&read_fds);
        FD_SET(vencFd, &read_fds);
 
        s32Ret = select(vencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0){
            printf("vencGetFrame select failed\n");
            usleep(10 * 1000);
            continue;
        }else if(0 == s32Ret){
            usleep(10 * 1000);
            continue;
        }else{
            if (FD_ISSET(vencFd, &read_fds)){
                memset(&stStat, 0, sizeof(MI_VENC_ChnStat_t));
                s32Ret = MI_VENC_Query(0, vencChn, &stStat);//查询编码通道状态
                if (MI_SUCCESS != s32Ret){
                    printf("vencGetFrame chn [%d] MI_VENC_Query err\n",vencChn);
                    usleep(10 * 1000); // sleep 10 ms
                    continue;
                }
 
                //获取编码后的码流，写入文件venc_out.es中
                memset(&stStream, 0, sizeof(MI_VENC_Stream_t));
                //memset(&stVdecStream, 0, sizeof(MI_VDEC_VideoStream_t));
                stStream.pstPack = (MI_VENC_Pack_t *)malloc(sizeof(MI_VENC_Pack_t) * stStat.u32CurPacks);
                if(NULL == stStream.pstPack){
                    printf("vencGetFrame NULL == stStream.pstPac\n");
                    return NULL;
                }
 
                //printf("<<<<<<<<<<<<vencGetFrame MI_VENC_GetStream<<<<<<<<<<<<<<<<<<<<<\n");
                stStream.u32PackCount = stStat.u32CurPacks;
                s32Ret = MI_VENC_GetStream(0, vencChn, &stStream, -1);//获得编码码流
                if (MI_SUCCESS == s32Ret){
                    if(pstOutFileAttr->u32ReadFromRtsp == 0)
                    {
                        if(boutputstart){
                            printf("vencGetFrame chn [%d] write %d \n",vencChn,i);
                            write(fd, stStream.pstPack[0].pu8Addr, stStream.pstPack[0].u32Len);
                            if(i >= 30){
                                boutputstart = FALSE;
                                i = 0;
                            }
                        }
                        i++;
                    }
                    else
                    {
                        u32Maxlen = pstOutFileAttr->u32Maxlen;
                        u32Size = MIN(stStream.pstPack[0].u32Len, u32Maxlen);
                        ASSERT(u32Size);
                        for (MI_U8 i = 0; i < stStream.u32PackCount; i++)
                        {
                            memcpy((char *)pstOutFileAttr->pData + u32Len, stStream.pstPack[i].pu8Addr, stStream.pstPack[i].u32Len);
                            u32Len += stStream.pstPack[i].u32Len;
                        }
                        pstOutFileAttr->U32ReturnValue = u32Size;
                        //释放码流缓存
                        MI_VENC_ReleaseStream(0, vencChn, &stStream);
                        free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        return NULL;
                    }
                    //释放码流缓存
                    MI_VENC_ReleaseStream(0, vencChn, &stStream);
                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                }
            }
        }
    }
    close(fd);
    return 0;
}

void *ST_OpenStream(char const *szStreamName, void *arg)
{
    ST_StreamInfo_T * pstStreamInfo = NULL;
    MI_U32            i             = 0;
    MI_S32            s32Ret        = MI_SUCCESS;
 
    pstStreamInfo = (ST_StreamInfo_T *)malloc(sizeof(ST_StreamInfo_T));
    if (pstStreamInfo == NULL)
    {
        PrintErr("malloc error\n");
        return NULL;
    }
    PrintInfo("ST_OpenStream\n");
    memset(pstStreamInfo, 0, sizeof(ST_StreamInfo_T));
 
    /* 构建stream的字段信息 */
	for(i=0;i < _g_rtsp_num;i++)
	{
		//printf("found stream %s\n", szStreamName);
		//printf("[%d] stream %s\n", i, _g_buf_obj[i].pszStreamName);
		if(!strncmp(szStreamName, _g_buf_obj[i].pszStreamName, strlen(_g_buf_obj[i].pszStreamName)))
    		break;
	}
	
	if(i > _g_rtsp_num)
	{
		printf("not found this stream,\"%s\"\n",szStreamName);
		free(pstStreamInfo);
		return NULL;
	}
    pstStreamInfo->vencChn = _g_buf_obj[i].vencChn;
    pstStreamInfo->enType  = E_MI_VENC_MODTYPE_H264E;
    snprintf(pstStreamInfo->szStreamName, sizeof(pstStreamInfo->szStreamName) - 1, "%s", szStreamName);
 
    /* 成功找到流数据后，请求一张IDR帧 */
    s32Ret = MI_VENC_RequestIdr(0, pstStreamInfo->vencChn, TRUE);
 
    PrintInfo("open stream \"%s\" success, chn:%d\n", szStreamName, pstStreamInfo->vencChn);
 
    if (MI_SUCCESS != s32Ret)
    {
        PrintInfo("request IDR fail, error:%x\n", s32Ret);
    }
 
    return pstStreamInfo;
}
 
int ST_CloseStream(void *handle, void *arg)
{
    if (handle == NULL)
    {
        return -1;
    }
 
    ST_StreamInfo_T *pstStreamInfo = (ST_StreamInfo_T *)(handle);
 
    PrintInfo("close \"%s\" success\n", pstStreamInfo->szStreamName);
 
    free(pstStreamInfo);
 
    return 0;
}
 
int ST_VideoReadStream(void *handle, unsigned char *ucpBuf, int BufLen, struct timeval *p_Timestamp, void *arg)
{
    ST_StreamInfo_T *pstStreamInfo = (ST_StreamInfo_T *) (handle);
    ST_OutputFile_Attr_t stoutFileAttr;
    // return (int)_ST_GetDataDirect(pstStreamInfo->vencChn, (void *)ucpBuf, BufLen);
    stoutFileAttr.u32Maxlen = BufLen;
    stoutFileAttr.pData = (void *)ucpBuf;
    stoutFileAttr.u32ReadFromRtsp = 1;
 	stoutFileAttr.vencChn = pstStreamInfo->vencChn;
    //VideoGetFrame((void *)(&stoutFileAttr));
    vencGetFrame((void *)(&stoutFileAttr));
    return stoutFileAttr.U32ReturnValue;
 
}

//int Start_Rtsp(buffer_object_t buf_obj[], void *(*eFunc)(void*))
int Start_Rtsp(buffer_object_t buf_obj[], int rtsp_num)
{
    unsigned int      rtspServerPortNum = 554;
    int               arraySize         = 16;
    char *            urlPrefix         = NULL;
    int               iRet              = 0;
    int               i                 = 0;
    //VideoGetFrame = eFunc;
    ServerMediaSession *   mediaSession = NULL;
    ServerMediaSubsession *subSession   = NULL;
    Live555RTSPServer *    pRTSPServer  = NULL;
    _g_rtsp_num = rtsp_num;
    char const *szStreamName = (char *)malloc(sizeof(char));
    pRTSPServer = new Live555RTSPServer();
    if (pRTSPServer == NULL)
    {
        PrintErr("malloc error\n");
        return -1;
    }

    /* 设置rtsp服务器的端口号信息，成功返回0 */
    iRet = pRTSPServer->SetRTSPServerPort(rtspServerPortNum);
 
    /* 再次尝试SetRTSPServerPort */
    while (iRet)
    {
        rtspServerPortNum++;
        if (rtspServerPortNum > 65535)
        {
            PrintInfo("Failed to create RTSP server: %s\n", pRTSPServer->getResultMsg());
            delete pRTSPServer;
            pRTSPServer = NULL;
            return -2;
        }
 
        iRet = pRTSPServer->SetRTSPServerPort(rtspServerPortNum);
    }
 
    /* 添加url前缀 */
    urlPrefix = pRTSPServer->rtspURLPrefix();
    printf("*************\n");
 	for(i=0;i < _g_rtsp_num;i++)
	{	
		if(buf_obj[i].venc_flag)
		{
			printf("=================URL===================\n");
    		printf("%s%s\n", urlPrefix, buf_obj[i].pszStreamName);
    		printf("=================URL===================\n");
    		/* 向服务器创建media会话 */
    		pRTSPServer->createServerMediaSession(mediaSession, buf_obj[i].pszStreamName, NULL, NULL);
    		printf("============createNew=============\n");
    		/* 客户端请求播放 */
    		subSession = WW_H264VideoFileServerMediaSubsession::createNew(*(pRTSPServer->GetUsageEnvironmentObj()),
                                                                     buf_obj[i].pszStreamName, ST_OpenStream,
                                                                      ST_VideoReadStream, ST_CloseStream, 30);
 
    		printf("============addSubsession=============\n");
    		/* 添加相关的会话信息 */
    		pRTSPServer->addSubsession(mediaSession, subSession);
    		printf("============addServerMediaSession=============\n");
    		pRTSPServer->addServerMediaSession(mediaSession);
    		printf("============pRTSPServer->Start=============\n");
		}
	}
    /* 发送流媒体信息 */
    pRTSPServer->Start();

	g_pRTSPServer = pRTSPServer;
    return 0;
}

MI_S32 Stop_Rtsp(void)
{
    boutputstart = 1;
    if(g_pRTSPServer)
    {
        g_pRTSPServer->Join();
        delete g_pRTSPServer;
        g_pRTSPServer = NULL;
    }

    return 0;
}