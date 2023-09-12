#ifndef _RTSP_H_
#define _RTSP_H_
#include "Live555RTSPServer.hh"
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#if defined (__cplusplus)
extern "C" {
#endif
#include "mi_venc.h"
#include "mi_venc_datatype.h"
#include "common.h"
#define MAIN_STREAM0 "video0"
#define MAIN_STREAM1 "video1"
#define MAIN_STREAM2 "video2"

#define PrintInfo(fmt, args...)     ({do{printf(ASCII_COLOR_WHITE"[INFO]:%s[%d]: ", __FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_END);}while(0);})
#define PrintErr(fmt, args...)      ({do{printf(ASCII_COLOR_RED  "[ERR ]:%s[%d]: ", __FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_RED);}while(0);})

typedef struct
{
    MI_VENC_CHN       vencChn;
    MI_VENC_ModType_e enType;
    char              szStreamName[64];
 
    MI_BOOL bWriteFile;
    int     fd;
    char    szDebugFile[128];
} ST_StreamInfo_T;
typedef struct ST_OutputFile_Attr_s
{
    MI_S32 s32DumpBuffNum;
    char FilePath[128];
    pthread_mutex_t Portmutex;
    pthread_t pGetDatathread;
 
    MI_U16 u16Depth;
    MI_U16 u16UserDepth;
    MI_U32 u32Maxlen;
    void *pData;
    MI_U32 u32ReadFromRtsp;
    MI_U32 U32ReturnValue;
	MI_VENC_CHN vencChn; 
    MI_SYS_ChnPort_t  stModuleInfo;
}ST_OutputFile_Attr_t;

/**************************************************
* Get video stream from venc by select
* @param            \b IN: input data
**************************************************/
void *vencGetFrame(void *data);

/**************************************************
* When the client open the code stream callback to open venc stream data and request the idr frame
* @param            \b IN: client code stream name
* @param            \b IN: input arg
* @return           \b OUT: stream info, used to client read the video stream and close it
**************************************************/
void *ST_OpenStream(char const *szStreamName, void *arg);

/**************************************************
* When the client close the code stream callback to free stream info
* @param            \b IN: stream info handle
* @param            \b IN: input arg
* @return           \b OUT: 0    success
**************************************************/
int ST_CloseStream(void *handle, void *arg);

/**************************************************
* When open video stream callback to read video stream
* @param            \b IN: stream info handle
* @param            \b IN: read stream buffer
* @param            \b IN: read stream buffer len
* @param            \b IN: read stream timeout
* @param            \b IN: input arg
* @return           \b OUT: read stream size
**************************************************/
int ST_VideoReadStream(void *handle, unsigned char *ucpBuf, int BufLen, struct timeval *p_Timestamp, void *arg);

/**************************************************
* Create the rtsp server and media session, start the rtsp session
* @param            \b IN: buffer object list
* @param            \b IN: rtsp channel number
* @return           \b OUT: 0:   success
**************************************************/
int Start_Rtsp(buffer_object_t buf_obj[], int rtsp_num);

/**************************************************
* Stop rtsp server video stream thread, and free rtsp server
* @param            \b IN: buffer object list
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 Stop_Rtsp(void);

#if defined (__cplusplus)
} /* extern "C" */
#endif

#endif
