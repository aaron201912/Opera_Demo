#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <getopt.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "drm_fourcc.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "mi_iqserver.h"
#include "mi_isp.h"
#include "mi_isp_ae.h"
#include "mi_isp_cus3a_api.h"
#include "mi_isp_iq.h"
#include "mi_scl.h"
#include "mi_sensor.h"
#include "mi_sys.h"
#include "mi_vif.h"
#include "mi_rgn.h"
#include "mi_ipu.h"
#include "mi_ipu_datatype.h"
#include "mi_venc.h"
#include "mi_venc_datatype.h"
#include "mi_vdec_datatype.h"
#include "sstar_detection_api.h"
#include "st_rgn.h"
#include <semaphore.h>


#include "Live555RTSPServer.hh"
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
//#include "sstar_sensor.h"


#define ASCII_COLOR_RED                          "\033[1;31m"
#define ASCII_COLOR_WHITE                        "\033[1;37m"
#define ASCII_COLOR_YELLOW                       "\033[1;33m"
#define ASCII_COLOR_BLUE                         "\033[1;36m"
#define ASCII_COLOR_GREEN                        "\033[1;32m"
#define ASCII_COLOR_END                          "\033[0m"
	
#define PrintInfo(fmt, args...)     ({do{printf(ASCII_COLOR_WHITE"[INFO]:%s[%d]: ", __FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_END);}while(0);})
#define PrintErr(fmt, args...)      ({do{printf(ASCII_COLOR_RED  "[ERR ]:%s[%d]: ", __FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_RED);}while(0);})

#ifndef ASSERT
#define ASSERT(_x_)                                                                     \
		do																					\
		{																					\
			if (!(_x_)) 																	\
			{																				\
				printf("ASSERT FAIL: %s %s %d\n", __FILE__, __PRETTY_FUNCTION__, __LINE__); \
				abort();																	\
			}																				\
		} while (0)
#endif


#define OUTPUT_WIDTH 1024
#define OUTPUT_HEIGHT 600
#define MAIN_STREAM "video0"

#define ALIGN_NUM 4
#define MAX_NUM_OF_DMABUFF 6

#define DRM_SUCCESS  (0)
#define DRM_FAIL     (-1)

#define FRAME_QUEUE_SIZE    30

#define  SUCCESS            0
#define  FAIL               1

#ifndef MIN
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif

#define CheckFuncResult(result)\
    if (result != SUCCESS)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__, __LINE__);\
    }\

#define ExecFunc(func, _ret_)                                                                    \
    do {                                                                                         \
        MI_S32 s32TmpRet;                                                                           \
        s32TmpRet = func;                                                                        \
        if (s32TmpRet != _ret_) {                                                                \
            printf(" [%d] %s exec function [failed], result:0x%x.\n", __LINE__, #func, \
                   s32TmpRet);                                                                   \
            return -1;                                                                           \
        } else {                                                                                 \
            printf(" [%d] %s exec function [pass].\n", __LINE__, #func);               \
        }                                                                                        \
    } while (0);

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct {
    char *frame;
    void *handle; //private data
    int buf_size;
    int64_t enqueueTime;
    int64_t DequeueTime;
    pthread_mutex_t mutex;
}   frame_t;

typedef struct {
    frame_t queue[FRAME_QUEUE_SIZE];
    int rindex;                     // 读索引。待播放时读取此帧进行播放，播放后此帧成为上一帧
    int windex;                     // 写索引
    int size;                       // 总帧数
    int max_size;                   // 队列可存储最大帧数
    int rindex_shown;               // 当前是否有帧在显示
    sem_t sem_avail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
}   frame_queue_t;

typedef struct dma_info
{
    int dma_heap_fd;
    int dma_buf_fd;
    int dma_buf_len;
    int buf_in_use;
    uint32_t fb_id;
    uint32_t format;
    int out_fence;
    unsigned int gem_handles[4];
    uint32_t pitches[4];
    uint32_t offsets[4];
    pthread_mutex_t mutex;
    pthread_cond_t  condwait;
    char *p_viraddr;
	int  *cb_func;
}dma_info_t;

typedef struct video_info
{
    int v_src_width;
    int v_src_height;
    int v_out_x;
    int v_out_y;
    int v_out_width;
    int v_out_height;
    int v_out_stride;
    int v_out_size;
    int v_bframe;
    uint32_t plane_id;
    uint32_t format;
    int out_fence;
}video_info_t;

typedef struct{
    MI_U64 len;
    MI_U32 fd;
    MI_U32 fd_flags;
    MI_U64 heap_flags;
}dma_heap_allocation_data;

enum queue_state {
	IDLEQUEUE_STATE = 0,
	ENQUEUE_STATE,
	DEQUEUE_STATE,
};

typedef struct {
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
    MI_SYS_BindType_e eBindType;
    MI_U32 u32BindParam;
} Sys_BindInfo_T;

struct property_arg {
    uint32_t obj_id;
    uint32_t obj_type;
    char name[DRM_PROP_NAME_LEN+1];
    uint32_t prop_id;
    uint64_t value;
    bool optional;
};

struct crtc {
    drmModeCrtc *crtc;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
    drmModeModeInfo *mode;
};

struct encoder {
    drmModeEncoder *encoder;
};

struct connector {
    drmModeConnector *connector;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
    char *name;
};


struct fb {
    drmModeFB *fb;
};

struct plane {
    drmModePlane *plane;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
};

struct resources {
    drmModeRes *res;
    drmModePlaneRes *plane_res;
    struct crtc *crtcs;
    struct encoder *encoders;
    struct connector *connectors;
    struct fb *fbs;
    struct plane *planes;
};

struct device {
    int use_atomic;
    int fd;
    struct resources *resources;
    uint32_t conn_id;
	uint32_t crtc_id;
    uint32_t plane_id;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t handle;
	uint32_t size;
	uint32_t fb_id;
    uint32_t sensorIdx;
	uint32_t connector_type;
	uint8_t *vaddr;
    drmModeAtomicReq *req;
    drmModeConnectorPtr _g_conn;
    drmModeRes *_g_res;
    drmModePlaneRes *_g_plane_res;
    video_info_t video_info;
	sem_t sem_avail;
    dma_info_t dma_info[MAX_NUM_OF_DMABUFF];

};

enum Sstar_plane_type_e {
    GOP_RES = 0,
    GOP_UI,
    GOP_CURSOR,
    MOPG,
    MOPS,
    PLANE_TYPE_MAX,
};

typedef struct ST_VencAttr_s
{
    Sys_BindInfo_T    stVencInBindParam;
 
    MI_VENC_CHN       vencChn;
    MI_VENC_ModType_e eType;
    MI_U32            u32Width;
    MI_U32            u32Height;
    MI_U32            u32Fps;
    char              szStreamName[128];
    MI_BOOL           bUsed;
    MI_BOOL           bCreate;
    MI_VENC_DEV       DevId;
 
    MI_S32  s32DumpBuffNum;
    char    FilePath[128];
    MI_U8   u8MD5ExpectValue[16];
    MI_BOOL bNeedCheckMd5;
} ST_VencAttr_t;
 
struct ST_Stream_Attr_T
{
    MI_BOOL           bEnable;
    MI_U32            enInput;
    MI_U32            u32InputChn;
    MI_U32            u32InputPort;
    MI_VENC_CHN       vencChn;
    MI_VENC_ModType_e eType;
    float             f32Mbps;
    MI_U32            u32Width;
    MI_U32            u32Height;
    MI_U32            u32CropX;
    MI_U32            u32CropY;
    MI_U32            u32CropWidth;
    MI_U32            u32CropHeight;
 
    MI_U32            enFunc;
    const char *      pszStreamName;
    MI_SYS_BindType_e eBindType;
    MI_U32            u32BindPara;
    MI_U32            u32Cover1Handle;
    MI_U32            u32Cover2Handle;
};
 
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
 
    MI_SYS_ChnPort_t  stModuleInfo;
}ST_OutputFile_Attr_t;
 
 
typedef struct ST_InputFile_Attr_s
{
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32SleepMs;
    MI_SYS_CompressMode_e eCompress;
    MI_SYS_PixelFormat_e ePixelFormat;
    char InputFilePath[256];
    pthread_mutex_t mutex;
    pthread_t pPutDatathread;
 
    MI_SYS_ChnPort_t  stModuleInfo;
}ST_InputFile_Attr_t;
 
 
 
MI_BOOL boutputstart = TRUE;
static ST_VencAttr_t  gstVencattr[16];
ST_OutputFile_Attr_t stoutFileAttr;
ST_InputFile_Attr_t  stInputFileAttr;
 

#define PLANE_MAX 30

#define DMA_HEAP_IOC_MAGIC    'H'

    /**
    * DOC: DMA_HEAP_IOCTL_ALLOC - allocate memory from pool
    *
    * Takes a dma_heap_allocation_data struct and returns it with the fd field
    * populated with the dmabuf handle of the allocation.
    */
#define DMA_HEAP_IOCTL_ALLOC    _IOWR(DMA_HEAP_IOC_MAGIC, 0, dma_heap_allocation_data)

#define ALIGN_BACK(x, a)            (((x) / (a)) * (a))
#define ALIGN_UP(x, a)            (((x+a-1) / (a)) * (a))

static MI_S32 ST_CreateDevice(MI_IPU_DevAttr_t* stDevAttr, char *pFirmwarePath);
static void ST_DetectCallback(std::vector<Box_t> results);
static void ST_DetectCallback_Rtsp(std::vector<Box_t> results);

static void ST_DestoryDevice();

/* 算法配置 */
static DetectionInfo_t g_detection_info = 
{
    "/config/dla/ipu_lfw.bin",
    "./sypfa5.480302_fixed.sim_sgsimg.img", 
    0.5, //threshold
    {OUTPUT_WIDTH, OUTPUT_HEIGHT}, //转成显示的分辨率
    ST_CreateDevice,
    ST_DetectCallback,
    ST_DestoryDevice,
};
static DetectionInfo_t g_detection_rtsp_info = 
{
    "/config/dla/ipu_lfw.bin",
    "./sypfa5.480302_fixed.sim_sgsimg.img", 
    0.5, //threshold
    {OUTPUT_WIDTH, OUTPUT_HEIGHT}, //转成显示的分辨率
    ST_CreateDevice,
    ST_DetectCallback_Rtsp,
    ST_DestoryDevice,
};
static void * g_detection_manager = NULL;
static MI_RGN_HANDLE g_stRgnHandle = 0;
static void * g_detection_manager_rtsp = NULL;
static MI_RGN_HANDLE g_stRgnHandle_rtsp = 1;
static MI_SYS_PixelFormat_e g_eAlgoFormat = E_MI_SYS_PIXEL_FRAME_ARGB8888; //模型输入格式
static Rect_t g_stAlgoRes = {800, 480}; //模型输入分辨率



int bExit = 0;
int bExit_second = 0;




/*
drmModeConnectorPtr _g_conn;
drmModeRes *_g_res;
drmModePlaneRes *_g_plane_res;
*/
static int _g_use_buf_mode = 0;
static int _g_mi_sys_fd = -1;
MI_SYS_ChnPort_t _g_chn_port_info;

int buff_ready = 0;
bool buff_null = false;
static int g_enable_iqserver = 0;
MI_SYS_Rotate_e g_rotate = E_MI_SYS_ROTATE_NONE;
static int cache_buf_cnt = 0;

struct device g_dev;
frame_queue_t _EnQueue_t;
//frame_queue_t _DeQueue_t;
//frame_queue_t _DrmQueue_t;

const char *defaultIqPath = "/config/iqfile";

char iqfile_path[100];

int plane_type[PLANE_TYPE_MAX][PLANE_MAX];
int plane_type_cnt[PLANE_TYPE_MAX];

static MI_S32 ST_CreateDevice(MI_IPU_DevAttr_t* stDevAttr, char *pFirmwarePath)
{
    return MI_IPU_CreateDevice(stDevAttr, NULL, pFirmwarePath, 0);
}

#define RECT_BORDER_WIDTH 8
static void ST_DetectCallback(std::vector<Box_t> results)
{
    int i = 0;
    MI_RGN_CanvasInfo_t *pstCanvasInfo;
    ST_Rect_T rgnRect = {0,0,0,0};

    ST_OSD_GetCanvasInfo(0, g_stRgnHandle, &pstCanvasInfo);
    ST_OSD_Clear(g_stRgnHandle, &rgnRect);
    for(i = 0; i < results.size(); i++)
    {
        rgnRect.u32X = results[i].x;
        rgnRect.u32Y = results[i].y;
        rgnRect.u16PicW = results[i].width;
        rgnRect.u16PicH = results[i].height;
		//printf("X = %d, Y = %d, Width = %d, High = %d\n", rgnRect.u32X, rgnRect.u32Y, rgnRect.u16PicW, rgnRect.u16PicH);
        ST_OSD_DrawRect(g_stRgnHandle, rgnRect, RECT_BORDER_WIDTH, 1);
    }
	ST_OSD_Update(0, g_stRgnHandle);

    return;
}

static void ST_DetectCallback_Rtsp(std::vector<Box_t> results)
{
    int i = 0;
    MI_RGN_CanvasInfo_t *pstCanvasInfo;
    ST_Rect_T rgnRect = {0,0,0,0};

    ST_OSD_GetCanvasInfo(0, g_stRgnHandle_rtsp, &pstCanvasInfo);
    ST_OSD_Clear(g_stRgnHandle_rtsp, &rgnRect);
    for(i = 0; i < results.size(); i++)
    {
        rgnRect.u32X = results[i].x;
        rgnRect.u32Y = results[i].y;
        rgnRect.u16PicW = results[i].width;
        rgnRect.u16PicH = results[i].height;
		//printf("X = %d, Y = %d, Width = %d, High = %d\n", rgnRect.u32X, rgnRect.u32Y, rgnRect.u16PicW, rgnRect.u16PicH);
        ST_OSD_DrawRect(g_stRgnHandle_rtsp, rgnRect, RECT_BORDER_WIDTH, 1);
    }
	ST_OSD_Update(0, g_stRgnHandle_rtsp);

    return;
}


static void ST_DestoryDevice()
{
    MI_IPU_DestroyDevice();
    return;
}

static void *ST_AlgoDemoProc(void *arg)
{
    MI_S32 s32Ret = 0;
    fd_set read_fds;
    struct timeval tv;
    MI_SYS_ChnPort_t *pstChnOutputPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
	MI_SYS_ChnPort_t stSclOutputPort;
	MI_S32 SclFd = -1;

	memset(&stSclOutputPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSclOutputPort.eModId = E_MI_MODULE_ID_SCL;
    stSclOutputPort.u32DevId = 0;
    stSclOutputPort.u32ChnId = 0;
    stSclOutputPort.u32PortId = 1;
	MI_SYS_SetChnOutputPortDepth(0, &stSclOutputPort , 2, 4);
    if(MI_SYS_GetFd(&stSclOutputPort, &SclFd) < 0)
    {
        printf("MI_SYS GET Scl ch: %d fd err.\n", stSclOutputPort.u32ChnId);
        return NULL;
    }

    while(!bExit)
    {
        FD_ZERO(&read_fds);
        FD_SET(SclFd, &read_fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;
        s32Ret = select(SclFd + 1, &read_fds, NULL, NULL, &tv);
        if(s32Ret < 0)
        {
            printf("select failed\n");
        }
        else if (0 == s32Ret)
        {
            printf("select timeout\n");
        }
        else
        {
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
            if(FD_ISSET(SclFd, &read_fds))
            {
                memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
                s32Ret = MI_SYS_ChnOutputPortGetBuf(&stSclOutputPort, &stBufInfo, &stBufHandle);
                if(MI_SUCCESS != s32Ret || E_MI_SYS_BUFDATA_FRAME != stBufInfo.eBufType)
                {
                    printf("get scl buffer fail,ret:%x\n",s32Ret);
                    MI_SYS_ChnOutputPortPutBuf(stBufHandle);
                    continue;
                }
                //printf("MI_SYS_ChnOutputPortGetBuf OK\n");
                if(MI_SUCCESS != doDetectPF(g_detection_manager, &stBufInfo))
                {
                    printf("doDetectFace fail,ret:%x\n",s32Ret);
                }
				if(MI_SUCCESS != doDetectPF(g_detection_manager_rtsp, &stBufInfo))
                {
                    printf("doDetectFace rtsp fail,ret:%x\n",s32Ret);
                }
                MI_SYS_ChnOutputPortPutBuf(stBufHandle);
            }
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        }
   }

	if(SclFd > 0)
    {
        MI_SYS_CloseFd(SclFd);
    }

    return NULL;
}


#if 0
static  int sync_wait(int fd, int timeout)
{
	struct pollfd fds = {0};
	int ret;

	fds.fd = fd;
	fds.events = POLLIN;

	do {
		ret = poll(&fds, 1, timeout);
		if (ret > 0) {
			if (fds.revents & (POLLERR | POLLNVAL)) {
				errno = EINVAL;
				return -1;
			}
			return 0;
		} else if (ret == 0) {
			errno = ETIME;
			return -1;
		}
	} while (ret == -1 && (errno == EINTR || errno == EAGAIN));

	return ret;
}
#else
static int sync_wait(int fd, int timeout)
{
    struct timeval time;
    fd_set fs_read;
    int ret;
    time.tv_sec = timeout / 1000;
    time.tv_usec = timeout % 1000 * 1000;

    FD_ZERO(&fs_read);
    FD_SET(fd, &fs_read);

    //ret = select(fd + 1, &fs_read, NULL, NULL, NULL);
    ret = select(fd + 1, &fs_read, NULL, NULL, &time);
    if(ret <= 0)
    {
        printf("select fail\n");
        return -1;
    }
    else
    {
        //printf("select success\n");
        return 0;
    }
}
#endif
void * ST_GetOutputDataThread(void * args)
{
    printf("ST_GetOutputDataThread start\n");
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    ST_OutputFile_Attr_t *pstOutFileAttr = ((ST_OutputFile_Attr_t *)(args));
    FILE *fp = NULL;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 s32Fd = 0;
    MI_BOOL bFileOpen = FALSE;
    MI_SYS_ChnPort_t stChnPort;
    char FilePath[256];
    fd_set set;
 
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = pstOutFileAttr->stModuleInfo.eModId;
    stChnPort.u32DevId = pstOutFileAttr->stModuleInfo.u32DevId;
    stChnPort.u32ChnId = pstOutFileAttr->stModuleInfo.u32ChnId;
    stChnPort.u32PortId = pstOutFileAttr->stModuleInfo.u32PortId;
    strcpy(FilePath, pstOutFileAttr->FilePath);
    printf("ST_GetOutputDataThread we will to MI_SYS_GetFd\n");
    s32Ret = MI_SYS_GetFd(&stChnPort,&s32Fd);
 
    if(s32Ret != MI_SUCCESS)
    {
        PrintInfo("MI_SYS_GetFd error %d\n",s32Ret);
        return NULL;
    }
    printf("ST_GetOutputDataThread MI_SYS_GetFd success\n");
    FD_ZERO(&set);
    FD_SET(s32Fd,&set);
    printf("ST_GetOutputDataThread begin to get output buf and write it to file");
    select(s32Fd + 1, &set, NULL, NULL, NULL);
 
    if(FD_ISSET(s32Fd,&set))
    {
 
    retry:
 
        printf("MI_SYS_ChnOutputPortGetBuf begin\n");
        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hHandle))
        {
            printf("MI_SYS_ChnOutputPortGetBuf success\n");
            if(pstOutFileAttr->s32DumpBuffNum > 0)
            {
                if(bFileOpen == FALSE)
                {
 
                    fp = fopen("result.yuv","wb+");
                    if(fp == NULL)
                    {
                        printf("file open fail\n");
                        pstOutFileAttr->s32DumpBuffNum = 0;
                        MI_SYS_ChnOutputPortPutBuf(hHandle);
                        goto retry;
                        // continue;
                    }
 
                    bFileOpen = TRUE;
                    printf("open file %s \n", FilePath);
                }
                printf("get out success \n");
 
                pstOutFileAttr->s32DumpBuffNum--;
                printf(
"=======begin writ port %d file id %d, file path %s, bufsize %d, stride %d, height %d\n", stChnPort.u32PortId, pstOutFileAttr->s32DumpBuffNum, FilePath,
                    stBufInfo.stFrameData.u32BufSize,stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u16Height);
                fwrite(stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32BufSize, 1, fp);
                printf(
"=======end   writ port %d file id %d, file path %s \n", stChnPort.u32PortId, pstOutFileAttr->s32DumpBuffNum, FilePath);
 
            }
 
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            printf("finished one\n");
            boutputstart = FALSE;
        }
        else
        {
            goto retry;
        }
 
    }
    printf("ST_GetOutputDataThread return\n");
    return NULL;
}
 
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
    MI_VDEC_VideoStream_t stVdecStream;
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_VENC_Pack_t *pstPack = NULL;
    static MI_U32 u32GetBufCnt = 0;
 
    MI_U32 u32Size = 0;
    MI_U32 u32Maxlen = 0;
    MI_U32 u32Len = 0;
 
    ST_OutputFile_Attr_t *pstOutFileAttr = ((ST_OutputFile_Attr_t *)(data));
    char FilePath[256];
 
    strcpy(FilePath, pstOutFileAttr->FilePath);
    TimeoutVal.tv_sec = 2;
    TimeoutVal.tv_usec = 0;
 
    //保存文件为venc_out.es
    fd = open(FilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    vencFd = MI_VENC_GetFd(0, 0);
    if (vencFd <= 0){
        printf("vencGetFrame get fd err\n");
        return NULL;
    }
 
    //printf("vencGetFrame start\n");
    while(boutputstart){
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
                s32Ret = MI_VENC_Query(0, 0, &stStat);//查询编码通道状态
                if (MI_SUCCESS != s32Ret){
                    printf("vencGetFrame MI_VENC_Query err\n");
                    usleep(10 * 1000); // sleep 10 ms
                    continue;
                }
 
                //获取编码后的码流，写入文件venc_out.es中
                memset(&stStream, 0, sizeof(MI_VENC_Stream_t));
                memset(&stVdecStream, 0, sizeof(MI_VDEC_VideoStream_t));
                stStream.pstPack = (MI_VENC_Pack_t *)malloc(sizeof(MI_VENC_Pack_t) * stStat.u32CurPacks);
                if(NULL == stStream.pstPack){
                    printf("vencGetFrame NULL == stStream.pstPac\n");
                    return NULL;
                }
 
                //printf("vencGetFrame MI_VENC_GetStream\n");
                stStream.u32PackCount = stStat.u32CurPacks;
                s32Ret = MI_VENC_GetStream(0, 0, &stStream, -1);//获得编码码流
                if (MI_SUCCESS == s32Ret){
                    if(pstOutFileAttr->u32ReadFromRtsp == 0)
                    {
                        if(boutputstart){
                            printf("vencGetFrame write  %d \n",i);
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
                        MI_VENC_ReleaseStream(0, 0, &stStream);
                        return NULL;
                    }
                    //释放码流缓存
                    MI_VENC_ReleaseStream(0, 0, &stStream);
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
    strncmp(szStreamName, MAIN_STREAM, strlen(MAIN_STREAM));
 
 
    pstStreamInfo->vencChn = 0;
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
 
 
    // return (int)_ST_GetDataDirect(pstStreamInfo->vencChn, (void *)ucpBuf, BufLen);
    stoutFileAttr.u32Maxlen = BufLen;
    stoutFileAttr.pData = (void *)ucpBuf;
    stoutFileAttr.u32ReadFromRtsp = 1;
 
    //PrintInfo("ST_VideoReadStream \n");
    vencGetFrame((void *)(&stoutFileAttr));
 
    return stoutFileAttr.U32ReturnValue;
 
}

 
int Start_Rtsp(void)
{
    unsigned int      rtspServerPortNum = 554;
    int               arraySize         = 16;
    char *            urlPrefix         = NULL;
    int               iRet              = 0;
    int               i                 = 0;
 
    ServerMediaSession *   mediaSession = NULL;
    ServerMediaSubsession *subSession   = NULL;
    Live555RTSPServer *    pRTSPServer  = NULL;
 
    char const *szStreamName = (char *)malloc(sizeof(char));
    strncmp(szStreamName, MAIN_STREAM, strlen(MAIN_STREAM));
 
    pRTSPServer = new Live555RTSPServer();
    if (pRTSPServer == NULL)
    {
        PrintErr("malloc error\n");
        return -1;
    }
 
    /* 设置rtsp服务期的端口号信息，成功返回0 */
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
 
    printf("=================URL===================\n");
    printf("%s%s\n", urlPrefix, MAIN_STREAM);
    printf("=================URL===================\n");
 
    /* 向服务器创建media会话 */
    pRTSPServer->createServerMediaSession(mediaSession, MAIN_STREAM, NULL, NULL);
    printf("============createNew=============\n");
    /* 客户端请求播放 */
    subSession = WW_H264VideoFileServerMediaSubsession::createNew(*(pRTSPServer->GetUsageEnvironmentObj()),
                                                                      szStreamName, ST_OpenStream,
                                                                      ST_VideoReadStream, ST_CloseStream, 30);
 
    printf("============addSubsession=============\n");
    /* 添加相关的会话信息 */
    pRTSPServer->addSubsession(mediaSession, subSession);
    printf("============addServerMediaSession=============\n");
    pRTSPServer->addServerMediaSession(mediaSession);
    printf("============pRTSPServer->Start=============\n");
    /* 发送流媒体信息 */
    pRTSPServer->Start();
 
    return 0;
}

MI_S32 ST_HandleWriteRawData(MI_ModuleId_e EModid, char * FileName)
{
    MI_SYS_ChnPort_t stChnPort;
    stChnPort.eModId = EModid;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = 0;
 
    stoutFileAttr.s32DumpBuffNum=1;
 
    strcpy(stoutFileAttr.FilePath, FileName);
    stoutFileAttr.u16Depth = 2;
    stoutFileAttr.u16UserDepth=3;
    memcpy(&stoutFileAttr.stModuleInfo,&stChnPort,sizeof(MI_SYS_ChnPort_t));
    printf("write %s pthread_create\n", FileName);
 
    if(EModid != E_MI_MODULE_ID_VENC)
    {
        pthread_create(&stoutFileAttr.pGetDatathread, NULL, ST_GetOutputDataThread, (void *)(&stoutFileAttr));
    }
    else
    {
        stoutFileAttr.u32ReadFromRtsp = 0;
        pthread_create(&stoutFileAttr.pGetDatathread, NULL, vencGetFrame, (void *)(&stoutFileAttr));
    }
 
    return MI_SUCCESS;
}

int frame_queue_init(frame_queue_t *f, int max_size, int frame_size)
{
    int i;
    memset(f, 0, sizeof(frame_queue_t));

    CheckFuncResult(pthread_mutex_init(&f->mutex, NULL));
    CheckFuncResult(pthread_cond_init(&f->cond,NULL));

    f->max_size = max_size;
    f->size = 0;
    for (i = 0; i < f->max_size; i++)
    {
        if(frame_size != 0)
        {
            if (!(f->queue[i].frame = (char *)malloc(frame_size)))
            {
                return -1;
            }
            _g_use_buf_mode = 1;
        }
        else
        {
            f->queue[i].frame = NULL;
        }
        f->queue[i].buf_size = frame_size;
        CheckFuncResult(pthread_mutex_init(&f->queue[i].mutex, NULL));
    }
	sem_init(&f->sem_avail, 0, max_size);
    return 0;
}

void frame_queue_putbuf(frame_queue_t *f,char *frame,int buf_len ,void * handle)
{
    //struct timeval timeEnqueue;
    int windex;
    windex = f->windex;

    //printf("frame_queue_putbuf begin f=%llx rindex=%d \n",f,f->rindex);
    //pthread_mutex_lock(&f->mutex);
    pthread_mutex_lock(&f->queue[f->windex].mutex);
    //f->queue[f->windex].enqueueTime = (int64_t)timeEnqueue.tv_sec * 1000000 + timeEnqueue.tv_usec;
    if(handle != NULL)
    {
        f->queue[f->windex].handle = handle;
    }
    if(buf_len > 0 && (f->queue[f->windex].frame != NULL))
    {
        memcpy(f->queue[f->windex].frame,frame,buf_len);
        f->queue[f->windex].buf_size = buf_len;
    }
    else
    {
        f->queue[f->windex].frame = frame;
        f->queue[f->windex].buf_size = buf_len;
    }

    if (++f->windex == f->max_size)
    {
        f->windex = 0;
    }
    if(f->size < f->max_size)
        f->size++;
    pthread_mutex_unlock(&f->queue[windex].mutex);
    //pthread_mutex_unlock(&f->mutex);
    //pthread_cond_signal(&f->cond);
    //printf("frame_queue_putbuf end f=%llx rindex=%d \n",f,f->rindex);
}

#if 0

// 向队列尾部压入一帧，只更新计数与写指针，因此调用此函数前应将帧数据写入队列相应位置
void frame_queue_push(frame_queue_t *f)
{
    if (++f->windex == f->max_size)
    {
        f->windex = 0;
    }
    pthread_mutex_lock(&f->mutex);
    f->size++;
    pthread_mutex_unlock(&f->mutex);
    pthread_cond_signal(&f->cond);
}
#endif


//frame_queue_peek_lasty用完之后要call frame_queue_next归还

frame_t *frame_queue_peek_last(frame_queue_t *f,int wait_ms)
{
#if 0
    struct timespec now_time;
    struct timespec out_time;
    unsigned long now_time_us;

    clock_gettime(CLOCK_MONOTONIC, &now_time);
    out_time.tv_sec = now_time.tv_sec;
    out_time.tv_nsec = now_time.tv_nsec;
    out_time.tv_sec += wait_ms/1000;   //ms 可能超1s

    now_time_us = out_time.tv_nsec/1000 + 1000*(wait_ms%1000); //计算us
    out_time.tv_sec += now_time_us/1000000; //us可能超1s
    now_time_us = now_time_us%1000000;
    out_time.tv_nsec = now_time_us * 1000;//us
#endif
    //printf("frame_queue_peek_last begin f=%llx rindex=%d \n",f,f->rindex);
    //pthread_mutex_lock(&f->mutex);
    //pthread_cond_timedwait(&f->cond,&f->mutex,&out_time);
    pthread_mutex_lock(&f->queue[f->rindex].mutex);
    //printf("frame_queue_peek_last end f=%llx  rindex=%d \n",f,f->rindex);
    return &f->queue[f->rindex];
}


void frame_queue_next(frame_queue_t *f,frame_t* pFrame)
{
    //printf("frame_queue_next begin f=%llx rindex=%d \n",f,f->rindex);
    //pthread_mutex_unlock(&pFrame->mutex);
    //pthread_mutex_unlock(&f->mutex);
    if(_g_use_buf_mode)
    {
        memset(f->queue[f->rindex].frame, 0, pFrame->buf_size);
    }
    else
    {
        f->queue[f->rindex].frame = NULL;
    }
    pFrame->buf_size = 0;
    f->queue[f->rindex].enqueueTime = 0;

    if (++f->rindex == f->max_size)
        f->rindex = 0;
    //pthread_mutex_lock(&f->mutex);
    if(f->size > 0)
	    f->size--;
	pthread_mutex_unlock(&pFrame->mutex);
    //pthread_mutex_unlock(&f->mutex);
    //printf("frame_queue_next end f=%llx rindex=%d \n",f,f->rindex);
}

void frame_queue_flush(frame_queue_t *f)
{
    //printf("queue valid size : %d, rindex : %d\n", f->size, f->rindex);
    pthread_mutex_lock(&f->mutex);
    for (; f->size > 0; f->size --)
    {
        frame_t *vp = &f->queue[(f->rindex ++) % f->max_size];
        if(vp->frame)
        {
            if(_g_use_buf_mode && vp->frame != NULL)
            {
                free(vp->frame);
                vp->frame = NULL;
            }
        }
        if (f->rindex >= f->max_size)
            f->rindex = 0;
    }
    f->rindex = 0;
    f->windex = 0;
    f->size   = 0;
    pthread_mutex_unlock(&f->mutex);
}

void frame_queue_destory(frame_queue_t *f)
{
    int i;
    for (i = 0; i < f->max_size; i++) {
        frame_t *vp = &f->queue[i];
        if(vp->frame)
        {
            if(_g_use_buf_mode && vp->frame != NULL)
            {
                free(vp->frame);
                vp->frame = NULL;
            }
        }
    }
    pthread_mutex_destroy(&f->mutex);
    pthread_cond_destroy(&f->cond);
}

static bool set_property(struct device *dev, struct property_arg *p)
{
	drmModeObjectProperties *props = NULL;
	drmModePropertyRes **props_info = NULL;
	const char *obj_type;
	int ret;
	int i;

	p->obj_type = 0;
	p->prop_id = 0;

#define find_object(_res, __res, type, Type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			struct type *obj = &(_res)->type##s[i];			\
			if (obj->type->type##_id != p->obj_id)			\
				continue;					\
			p->obj_type = DRM_MODE_OBJECT_##Type;			\
			obj_type = #Type;					\
			props = obj->props;					\
			props_info = obj->props_info;				\
		}								\
	} while(0)								\

	find_object(dev->resources, res, crtc, CRTC);
	if (p->obj_type == 0)
		find_object(dev->resources, res, connector, CONNECTOR);
	if (p->obj_type == 0)
		find_object(dev->resources, plane_res, plane, PLANE);
	if (p->obj_type == 0) {
		fprintf(stderr, "Object %i not found, can't set property\n",
			p->obj_id);
		return false;
	}

	if (!props) {
		fprintf(stderr, "%s %i has no properties\n",
			obj_type, p->obj_id);
		return false;
	}

	for (i = 0; i < (int)props->count_props; ++i) {
		if (!props_info[i])
			continue;
		if (strcmp(props_info[i]->name, p->name) == 0)
			break;
	}

	if (i == (int)props->count_props) {
		if (!p->optional)
			fprintf(stderr, "%s %i has no %s property\n",
				obj_type, p->obj_id, p->name);
		return false;
	}

	p->prop_id = props->props[i];

	if (!dev->use_atomic)
		ret = drmModeObjectSetProperty(dev->fd, p->obj_id, p->obj_type,
									   p->prop_id, p->value);
	else
		ret = drmModeAtomicAddProperty(dev->req, p->obj_id, p->prop_id, p->value);

	if (ret < 0)
		fprintf(stderr, "failed to set %s %i property %s to %" PRIu64 ": %s\n",
			obj_type, p->obj_id, p->name, p->value, strerror(errno));

	return true;
}

static void add_property(struct device *dev, uint32_t obj_id,
			       const char *name, uint64_t value)
{
	struct property_arg p;

	p.obj_id = obj_id;
	strcpy(p.name, name);
	p.value = value;

	set_property(dev, &p);
}

static void free_resources(struct resources *res)
{
	int i;

	if (!res)
		return;

#define free_resource(_res, __res, type, Type)					\
	do {									\
		if (!(_res)->type##s)						\
			break;							\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			if (!(_res)->type##s[i].type)				\
				break;						\
			drmModeFree##Type((_res)->type##s[i].type);		\
		}								\
		free((_res)->type##s);						\
	} while (0)

#define free_properties(_res, __res, type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			drmModeFreeObjectProperties(res->type##s[i].props);	\
			free(res->type##s[i].props_info);			\
		}								\
	} while (0)

	if (res->res) {
		free_properties(res, res, crtc);

		free_resource(res, res, crtc, Crtc);
		free_resource(res, res, encoder, Encoder);

		for (i = 0; i < res->res->count_connectors; i++)
			free(res->connectors[i].name);

		free_resource(res, res, connector, Connector);
		free_resource(res, res, fb, FB);

		drmModeFreeResources(res->res);
	}

	if (res->plane_res) {
		free_properties(res, plane_res, plane);

		free_resource(res, plane_res, plane, Plane);

		drmModeFreePlaneResources(res->plane_res);
	}

	free(res);
}

static struct resources *get_resources(struct device *dev)
{
	struct resources *res;
	int i;

	res = (struct resources *)calloc(1, sizeof(*res));
	if (res == 0)
		return NULL;

	drmSetClientCap(dev->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	drmSetClientCap(dev->fd, DRM_CLIENT_CAP_WRITEBACK_CONNECTORS, 1);

	res->res = drmModeGetResources(dev->fd);
	if (!res->res) {
		fprintf(stderr, "drmModeGetResources failed: %s\n",
			strerror(errno));
		goto error;
	}

	res->crtcs = (struct crtc *)calloc(res->res->count_crtcs, sizeof(*res->crtcs));
	res->encoders = (struct encoder *)calloc(res->res->count_encoders, sizeof(*res->encoders));
	res->connectors = (struct connector *)calloc(res->res->count_connectors, sizeof(*res->connectors));
	res->fbs = (struct fb *)calloc(res->res->count_fbs, sizeof(*res->fbs));

	if (!res->crtcs || !res->encoders || !res->connectors || !res->fbs)
		goto error;

#define get_resource(_res, __res, type, Type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			(_res)->type##s[i].type =				\
				drmModeGet##Type(dev->fd, (_res)->__res->type##s[i]); \
			if (!(_res)->type##s[i].type)				\
				fprintf(stderr, "could not get %s %i: %s\n",	\
					#type, (_res)->__res->type##s[i],	\
					strerror(errno));			\
		}								\
	} while (0)

	get_resource(res, res, crtc, Crtc);
	get_resource(res, res, encoder, Encoder);
	get_resource(res, res, connector, Connector);
	get_resource(res, res, fb, FB);

	/* Set the name of all connectors based on the type name and the per-type ID. */
    #if 0
	for (i = 0; i < res->res->count_connectors; i++) {
		struct connector *connector = &res->connectors[i];
		drmModeConnector *conn = connector->connector;
		int num;

		num = asprintf(&connector->name, "%s-%u",
			 util_lookup_connector_type_name(conn->connector_type),
			 conn->connector_type_id);
		if (num < 0)
			goto error;
	}
    #endif
#define get_properties(_res, __res, type, Type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			struct type *obj = &res->type##s[i];			\
			unsigned int j;						\
			obj->props =						\
				drmModeObjectGetProperties(dev->fd, obj->type->type##_id, \
							   DRM_MODE_OBJECT_##Type); \
			if (!obj->props) {					\
				fprintf(stderr,					\
					"could not get %s %i properties: %s\n", \
					#type, obj->type->type##_id,		\
					strerror(errno));			\
				continue;					\
			}							\
			obj->props_info = (drmModePropertyRes**)calloc(obj->props->count_props,	\
						 sizeof(*obj->props_info));	\
			if (!obj->props_info)					\
				continue;					\
			for (j = 0; j < obj->props->count_props; ++j)		\
				obj->props_info[j] =				\
					drmModeGetProperty(dev->fd, obj->props->props[j]); \
		}								\
	} while (0)

	get_properties(res, res, crtc, CRTC);
	get_properties(res, res, connector, CONNECTOR);

	for (i = 0; i < res->res->count_crtcs; ++i)
		res->crtcs[i].mode = &res->crtcs[i].crtc->mode;

	res->plane_res = drmModeGetPlaneResources(dev->fd);
	if (!res->plane_res) {
		fprintf(stderr, "drmModeGetPlaneResources failed: %s\n",
			strerror(errno));
		return res;
	}

	res->planes = (struct plane*)calloc(res->plane_res->count_planes, sizeof(*res->planes));
	if (!res->planes)
		goto error;

	get_resource(res, plane_res, plane, Plane);
	get_properties(res, plane_res, plane, PLANE);

	return res;

error:
	free_resources(res);
	return NULL;
}

static MI_S32 SysInit() 
{
    MI_SYS_Version_t stVersion;
    MI_U64 u64Pts = 0;
    ExecFunc(MI_SYS_Init(0), DRM_SUCCESS);
    memset(&stVersion, 0x0, sizeof(MI_SYS_Version_t));
    ExecFunc(MI_SYS_GetVersion(0, &stVersion), DRM_SUCCESS);
    printf("u8Version:%s\n", stVersion.u8Version);
    ExecFunc(MI_SYS_GetCurPts(0, &u64Pts), DRM_SUCCESS);
    printf("u64Pts:0x%llx\n", u64Pts);
    u64Pts = 0xF1237890F1237890;
    ExecFunc(MI_SYS_InitPtsBase(0, u64Pts), DRM_SUCCESS);
    u64Pts = 0xE1237890E1237890;
    ExecFunc(MI_SYS_SyncPts(0, u64Pts), DRM_SUCCESS);
    return DRM_SUCCESS;
}

static MI_S32 SysBind(Sys_BindInfo_T* pstBindInfo) {
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

static MI_S32 SysUnbind(Sys_BindInfo_T* pstBindInfo) {
    MI_S32 ret;

    ret = MI_SYS_UnBindChnPort(0, &pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort);
    printf("unbind src(%d-%d-%d-%d)  dst(%d-%d-%d-%d)  %d...\n", pstBindInfo->stSrcChnPort.eModId,
           pstBindInfo->stSrcChnPort.u32DevId, pstBindInfo->stSrcChnPort.u32ChnId,
           pstBindInfo->stSrcChnPort.u32PortId, pstBindInfo->stDstChnPort.eModId,
           pstBindInfo->stDstChnPort.u32DevId, pstBindInfo->stDstChnPort.u32ChnId,
           pstBindInfo->stDstChnPort.u32PortId, pstBindInfo->eBindType);

    return ret;
}

static int get_nv12_stride_and_size(int width, int height, int* pixel_stride,
                                     int* size) {

    int luma_stride;

    /* 4:2:0 formats must have buffers with even height and width as the clump size is 2x2 pixels.
     * Width will be even stride aligned anyway so just adjust height here for size calculation. */
    height = ALIGN_UP(height, ALIGN_NUM);

    luma_stride = ALIGN_UP(width, ALIGN_NUM);

    if (size != NULL) {
        *size = (height * luma_stride * 3)/2 ;
    }

    if (pixel_stride != NULL) {
        *pixel_stride = luma_stride;
    }
    return 0;
}

void sstar_mode_set(struct device *dev);
static int drm_atomic_commit(struct device *dev ,int fb_id) {
    int ret;

    drmModeAtomicFree(dev->req);
    dev->req = drmModeAtomicAlloc();
    if (!dev->req) {
        printf("drmModeAtomicAlloc failed \n");
        return -1;
    }
    sstar_mode_set(dev);

   	add_property(dev, dev->video_info.plane_id, "FB_ID", fb_id);
	add_property(dev, dev->video_info.plane_id, "CRTC_ID", dev->crtc_id);
	add_property(dev, dev->video_info.plane_id, "SRC_X", 0 << 16);
	add_property(dev, dev->video_info.plane_id, "SRC_Y", 0 << 16);
	add_property(dev, dev->video_info.plane_id, "SRC_W",  dev->video_info.v_out_width << 16);
	add_property(dev, dev->video_info.plane_id, "SRC_H", dev->video_info.v_out_height << 16);
	add_property(dev, dev->video_info.plane_id, "CRTC_X", 0);
	add_property(dev, dev->video_info.plane_id, "CRTC_Y", 0);
	add_property(dev, dev->video_info.plane_id, "CRTC_W", dev->width);
	add_property(dev, dev->video_info.plane_id, "CRTC_H", dev->height);

    ret = drmModeAtomicCommit(dev->fd, dev->req, DRM_MODE_ATOMIC_ALLOW_MODESET | DRM_MODE_ATOMIC_NONBLOCK, NULL);
    if(ret != 0)
    {
        printf("drmModeAtomicCommit failed ret=%d \n",ret);
        //return -1;
    }
    ret = sync_wait(dev->video_info.out_fence, 50);
    if(ret != 0)
    {
        printf("waring:maybe drop one drm frame, ret=%d out_fence=%d\n", ret, dev->video_info.out_fence);
    }
    close(dev->video_info.out_fence);


    return 0;
}

dma_info_t* get_dmainfo_by_handle(struct device *dev, int dma_buf_fd)
{

    for(int i = 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        if(dma_buf_fd == dev->dma_info[i].dma_buf_fd)
        {
            return &dev->dma_info[i];
        }
    }
    return NULL;

}

int sstar_drm_update(struct device *dev, int dma_buffer_fd) {
    int ret;
    dma_info_t* dma_info;


    dma_info = get_dmainfo_by_handle(dev, dma_buffer_fd);


    if(dma_info != NULL)
    {

        if(!dma_info->gem_handles[0])
        {
            ret = drmPrimeFDToHandle(dev->fd, dma_buffer_fd, &dma_info->gem_handles[0]);
            if (ret) {
                printf("drmPrimeFDToHandle failed, ret=%d dma_buffer_fd=%d\n", ret, dma_buffer_fd);
                return -1;
            }
            dma_info->gem_handles[1] = dma_info->gem_handles[0];
            dma_info->pitches[0] = dev->video_info.v_out_stride;
            dma_info->pitches[1] = dev->video_info.v_out_stride;
            dma_info->offsets[0] = 0;
            dma_info->offsets[1] = dev->video_info.v_out_stride * dev->video_info.v_out_height;
            dma_info->format = DRM_FORMAT_NV12;
            ret = drmModeAddFB2(dev->fd, dev->video_info.v_out_width,
                                dev->video_info.v_out_height, dma_info->format, dma_info->gem_handles, dma_info->pitches,
                                dma_info->offsets, &dma_info->fb_id, 0);
            if (ret)
            {
                printf("drmModeAddFB2 failed, ret=%d fb_id=%d\n", ret, dma_info->fb_id);
                return -1;
            }
        }
        ret = drm_atomic_commit(dev, dma_info->fb_id);
        if (ret) {
            printf("AtomicCommit failed, ret=%d out_fence=%d\n", ret, dev->video_info.out_fence);
            return -1;
        }

    }


    return 0;
}


frame_t* get_last_queue(frame_queue_t *Queue_t, enum queue_state state)
{
    frame_t *pQueue = NULL;
    dma_info_t *dma_info;
    int count_num = 20;
    while(count_num)
    {
        pQueue = frame_queue_peek_last(Queue_t, 2);
        dma_info = (dma_info_t *)pQueue->frame;
        if(dma_info != NULL && pQueue->buf_size != 0 && dma_info->buf_in_use == state)
        {
            //printf("state=%d count=%d queue_size=%d \n",state,count_num,Queue_t->size);
            return pQueue;
        }
        else
        {
            usleep(2*1000);
            frame_queue_next(Queue_t, pQueue);
            count_num --;
        }
    }
    return NULL;
}

void sstar_release_dma_obj(dma_info_t *dma_info)
{
    if(dma_info->dma_buf_fd)
    {
        drmModeRmFB(dma_info->dma_buf_fd, dma_info->fb_id);
    }
    if(dma_info->dma_heap_fd)
    {
        close(dma_info->dma_heap_fd);
    }
    if(dma_info->p_viraddr)
    {
        munmap(dma_info->p_viraddr, dma_info->dma_buf_len);
    }
}

void sstar_creat_dma_obj(dma_info_t *dma_info, unsigned int len)
{

	int fd_dmaheap;
    int ret = -1;
    dma_heap_allocation_data heap_data = {
        .len = 0,
        .fd = 0,
        .fd_flags = 2,
        .heap_flags = 0,
    };

    fd_dmaheap = open("/dev/mma", O_RDWR);
    if(fd_dmaheap < 0)
    {
        printf("open dma_heap/mma fail \n");
        dma_info->dma_buf_fd = -1;
        dma_info->dma_heap_fd = -1;
        return;
    }
    heap_data.len = len; // length of data to be allocated in bytes
    heap_data.fd_flags =(O_CLOEXEC | O_ACCMODE);


    ret = ioctl(fd_dmaheap, DMA_HEAP_IOCTL_ALLOC, &heap_data);
    if(ret < 0)
    {
        printf(" DMA_HEAP_IOCTL_ALLOC fail fd=%d ret=%d\n",heap_data.fd,ret);
        heap_data.fd = -1;
    }

    dma_info->dma_heap_fd = fd_dmaheap;
    dma_info->dma_buf_fd = heap_data.fd;
    dma_info->dma_buf_len = len;

}

int sstar_scl_enqueueOneBuffer(int32_t dev, int32_t chn, MI_SYS_DmaBufInfo_t* mi_dma_buf_info)
{
    MI_SYS_ChnPort_t chnPort;
    // set chn port info
    memset(&chnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    chnPort.eModId = E_MI_MODULE_ID_SCL;
    chnPort.u32DevId = dev;
    chnPort.u32ChnId = chn;
    chnPort.u32PortId = 0;

    // enqueue one dma buffer
    if (MI_SUCCESS != MI_SYS_ChnOutputPortEnqueueDmabuf(&chnPort, mi_dma_buf_info)) {
        printf("call MI_SYS_ChnOutputPortEnqueueDmabuf faild\n");
        return -1;
    }
    return 0;
}


int creat_outport_dmabufallocator(MI_SYS_ChnPort_t* mi_chn_port_info)
{

    mi_chn_port_info->eModId = E_MI_MODULE_ID_SCL;
    mi_chn_port_info->u32DevId = 0;
    mi_chn_port_info->u32ChnId = 0;
    mi_chn_port_info->u32PortId = 0;

    if (MI_SUCCESS != MI_SYS_CreateChnOutputPortDmabufCusAllocator(mi_chn_port_info))
    {
        printf("MI_SYS_CreateChnOutputPortDmabufCusAllocator failed \n");
        return DRM_FAIL;
    }
    return DRM_SUCCESS;
}
void destory_outport_dmabufallocator(MI_SYS_ChnPort_t* mi_chn_port_info)
{

    if (MI_SUCCESS != MI_SYS_DestroyChnOutputPortDmabufCusAllocator(mi_chn_port_info))
    {
        printf("warning: MI_SYS_DestroyChnOutputPortDmabufCusAllocator  fail\n");
    }
}

void creat_dmabuf_queue()
{
    int i;

    frame_queue_init(&_EnQueue_t, MAX_NUM_OF_DMABUFF, 0);

    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        sstar_creat_dma_obj(&g_dev.dma_info[i], g_dev.video_info.v_out_size);
        if(g_dev.dma_info[i].dma_buf_fd > 0)
        {
            g_dev.dma_info[i].buf_in_use = IDLEQUEUE_STATE;
            printf("g_dev dma_buf_fd=%d \n",g_dev.dma_info[i].dma_buf_fd);
        }
    }
}

void destory_dmabuf_queue()
{
    int i;
    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        sstar_release_dma_obj(&g_dev.dma_info[i]);
    }
    frame_queue_destory(&_EnQueue_t);
}



void* enqueue_buffer_loop(void* param)
{
    int dma_buf_handle = -1;
    MI_SYS_DmaBufInfo_t mi_dma_buf_info;
    int i;
    dma_info_t *dma_info;
    //unsigned long eTime1;
    //unsigned long eTime2;
    //struct timeval timeEnqueue1;

    struct device * buf_obj = (struct device *)param;
    memset(&mi_dma_buf_info, 0x0, sizeof(MI_SYS_DmaBufInfo_t));

    i = 0;
    while (!bExit)
    {

        sem_wait(&buf_obj->sem_avail);
        dma_info = &buf_obj->dma_info[i];
        dma_buf_handle = dma_info->dma_buf_fd;

		mi_dma_buf_info.u16Width = buf_obj->video_info.v_out_width;
		mi_dma_buf_info.u16Height = buf_obj->video_info.v_out_height;
		mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
		mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
		mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
		mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
		mi_dma_buf_info.u32Stride[0] = buf_obj->video_info.v_out_stride;
		mi_dma_buf_info.u32Stride[1] = buf_obj->video_info.v_out_stride;
		mi_dma_buf_info.u32DataOffset[0] = 0;
		mi_dma_buf_info.u32DataOffset[1] = buf_obj->video_info.v_out_width * buf_obj->video_info.v_out_height;


        #if 0

        gettimeofday(&timeEnqueue1, NULL);
        eTime1 =timeEnqueue1.tv_sec*1000 + timeEnqueue1.tv_usec/1000;
        printf("dma_buf_handle:%d  sstar_scl_enqueueOneBuffer enqueuetime: %d \n", dma_buf_handle, (eTime1 - eTime2) );
        eTime2 = eTime1;
        #endif
        if(sstar_scl_enqueueOneBuffer(_g_chn_port_info.u32DevId, _g_chn_port_info.u32ChnId,  &mi_dma_buf_info) == 0)
        {
            cache_buf_cnt++;

            dma_info->buf_in_use = DEQUEUE_STATE;
            frame_queue_putbuf(&_EnQueue_t, (char*)&buf_obj->dma_info[i], sizeof(dma_info_t), NULL);
			buff_ready = 1;
            //printf("sstar_vdec_enqueueOneBuffer cache_buf_cnt=%d \n",cache_buf_cnt);
        }
        else
        {
            printf("sstar_vdec_enqueueOneBuffer FAIL \n");
        }

        if(++i == MAX_NUM_OF_DMABUFF)
        {
            i = 0;
        }
    }

    printf("Thread enqueue_buffer_loop exit \n");
    bExit_second = 1;
    return (void*)0;
}

#define DRM_BUFF_TIMEOUT_COUNT 1000
void* drm_buffer_loop(void* param)
{
    frame_t *pQueue = NULL;
    int dma_buf_handle = -1;
    dma_info_t *dma_info;
    MI_SYS_DmaBufInfo_t mi_dma_buf_info;
    int timeout = 500;
    #if 0
    unsigned long eTime1;
    unsigned long eTime2;
    unsigned long eTime3;
    struct timeval timeEnqueue1;
    struct timeval timeEnqueue2;
    struct timeval timeEnqueue3;
    #endif
    int ret;
    struct device * buf_obj = (struct device *)param;
    memset(&mi_dma_buf_info, 0x0, sizeof(MI_SYS_DmaBufInfo_t));
	int count = DRM_BUFF_TIMEOUT_COUNT;

    MI_SYS_GetFd(&_g_chn_port_info, &_g_mi_sys_fd);
    while (!bExit_second)
    {
        while(!buff_ready && (count > 0)) {
            usleep(1*1000);
		    count--;
	    }
		count = DRM_BUFF_TIMEOUT_COUNT;
        pQueue = get_last_queue(&_EnQueue_t, DEQUEUE_STATE);
        if( pQueue !=NULL && pQueue->frame != NULL )
        {
            dma_info = (dma_info_t *)pQueue->frame;
        }
        else
        {
            printf("get_last_queue DEQUEUE_STATE fail \n");
            continue;
        }
        dma_buf_handle = dma_info->dma_buf_fd;
		mi_dma_buf_info.u16Width = buf_obj->video_info.v_out_width;
		mi_dma_buf_info.u16Height = buf_obj->video_info.v_out_height;
		mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
		mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
		mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
		mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
		mi_dma_buf_info.u32Stride[0] = buf_obj->video_info.v_out_stride;
		mi_dma_buf_info.u32Stride[1] = buf_obj->video_info.v_out_stride;
		mi_dma_buf_info.u32DataOffset[0] = 0;
		mi_dma_buf_info.u32DataOffset[1] = buf_obj->video_info.v_out_width * buf_obj->video_info.v_out_height;


        #if 0
        gettimeofday(&timeEnqueue1, NULL);
        eTime1 =timeEnqueue1.tv_sec*1000 + timeEnqueue1.tv_usec/1000;
        #endif
        ret = sync_wait(_g_mi_sys_fd, timeout);

        if (dma_buf_handle > 0)
        {
            ret = MI_SYS_ChnOutputPortDequeueDmabuf(&_g_chn_port_info, &mi_dma_buf_info);
            if (ret != 0)
            {
                dma_info->buf_in_use = IDLEQUEUE_STATE;
                frame_queue_next(&_EnQueue_t, pQueue);
                sem_post(&buf_obj->sem_avail);
                printf("Waring:DequeueDmabuf fail ret=0x%x \n", ret);
                continue;
            }

            #if 0
                gettimeofday(&timeEnqueue2, NULL);
                eTime2 = timeEnqueue2.tv_sec*1000 + timeEnqueue2.tv_usec/1000;
            #endif
            if(0 != sstar_drm_update(buf_obj, dma_buf_handle))
            {
                printf("drm update frame buffer failed \n");
                //bExit = 1;
                //break;
            }

            #if 0//(eTime3 - eTime2) > 20 || (eTime2 - eTime1) >20 )
            {
                gettimeofday(&timeEnqueue3, NULL);
                eTime3 = timeEnqueue3.tv_sec*1000 + timeEnqueue3.tv_usec/1000;
                printf("buf_handle:%d  sstar_drm_update_time: %d,Dequeuet_time=%d \n", dma_buf_handle, (eTime3 - eTime2),(eTime2 - eTime1));
            }
            #endif

            cache_buf_cnt --;
            dma_info->buf_in_use = IDLEQUEUE_STATE;
            frame_queue_next(&_EnQueue_t, pQueue);
            timeout = 50;
            sem_post(&buf_obj->sem_avail);
        }
        else
        {
            dma_info->buf_in_use = IDLEQUEUE_STATE;
            frame_queue_next(&_EnQueue_t, pQueue);
            sem_post(&buf_obj->sem_avail);
            printf("sync_wait fail dma_buf_handle=%d \n",dma_buf_handle);
        }

    }

    //MI_SYS_CloseFd(_g_mi_sys_fd);
    printf("Thread drm_buffer_loop exit \n");
    return (void*)0;
}

/*

void* enqueue_buffer_loop(void* param)
{
    int dma_buf_num = 0;
    int ret = -1;
    int dma_buf_handle = -1;
    MI_SYS_DmaBufInfo_t mi_dma_buf_info;
    MI_SYS_ChnPort_t mi_chn_port_info;
    frame_t *pQueue = NULL;
    int mi_sys_fd = -1;
    int i;
    dma_info_t *dma_info;

    struct device * buf_obj = (struct device *)param;
    static bool wait_buff_ready = false;


    mi_chn_port_info.eModId = E_MI_MODULE_ID_SCL;
    mi_chn_port_info.u32DevId = 7;
    mi_chn_port_info.u32ChnId = 0;
    mi_chn_port_info.u32PortId = 0;

    if (MI_SUCCESS != MI_SYS_CreateChnOutputPortDmabufCusAllocator(&mi_chn_port_info))
    {
        printf("MI_SYS_CreateChnOutputPortDmabufCusAllocator failed \n");
        return NULL;
    }

    MI_SYS_GetFd(&mi_chn_port_info, &mi_sys_fd);

    while (!bExit)
    {
        if (dma_buf_num == 0)
        {
            for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
            {
                sstar_creat_dma_obj(&buf_obj->dma_info[i], buf_obj->video_info.v_out_size);
                if(buf_obj->dma_info[i].dma_buf_fd > 0)
                {
                    buf_obj->dma_info[i].buf_in_use = IDLEQUEUE_STATE;
                    frame_queue_putbuf(&_EnQueue_t, (char*)&buf_obj->dma_info[i], sizeof(dma_info_t), NULL);
                }
                dma_buf_num ++;
            }
        }

        pQueue = get_last_queue(&_EnQueue_t, IDLEQUEUE_STATE);
        if( pQueue !=NULL && pQueue->frame != NULL)
        {
            dma_info = (dma_info_t *)pQueue->frame;
            dma_buf_handle = dma_info->dma_buf_fd;
            memset(&mi_dma_buf_info, 0x0, sizeof(MI_SYS_DmaBufInfo_t));
            mi_dma_buf_info.u16Width = buf_obj->video_info.v_out_width;
            mi_dma_buf_info.u16Height = buf_obj->video_info.v_out_height;
            mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
            mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
            mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
            mi_dma_buf_info.u32Stride[0] = buf_obj->video_info.v_out_stride;
            mi_dma_buf_info.u32Stride[1] = buf_obj->video_info.v_out_stride;
            mi_dma_buf_info.u32DataOffset[0] = 0;
            mi_dma_buf_info.u32DataOffset[1] = buf_obj->video_info.v_out_width * buf_obj->video_info.v_out_height;
            if(sstar_scl_enqueueOneBuffer(mi_chn_port_info.u32DevId, mi_chn_port_info.u32ChnId, &mi_dma_buf_info) == 0)
            {
                //printf("sstar_vdec_enqueueOneBuffer success \n");
                dma_info->buf_in_use = ENQUEUE_STATE;
                frame_queue_putbuf(&_DeQueue_t, pQueue->frame, sizeof(int), NULL);
            }
            else
            {
                printf("sstar_vdec_enqueueOneBuffer FAIL \n");
                continue;
            }
        }
        else
        {
            printf("get_last_queue _EnQueue_t IDLEQUEUE_STATE error \n");
			continue;
        }
        frame_queue_next(&_EnQueue_t, pQueue);
		if (!wait_buff_ready) {
           usleep(500*1000);
		   wait_buff_ready = true;
    	}

        ret = sync_wait(mi_sys_fd, 33);
        if (ret == 0 && (dma_buf_handle > 0))
        {
            ret = MI_SYS_ChnOutputPortDequeueDmabuf(&mi_chn_port_info, &mi_dma_buf_info);
            if (ret == 0)
            {
                pQueue = get_last_queue(&_DeQueue_t, ENQUEUE_STATE);
                if( pQueue !=NULL && pQueue->frame != NULL )
                {
                    dma_info = (dma_info_t *)pQueue->frame;
                    dma_info->buf_in_use = DEQUEUE_STATE;
                    frame_queue_putbuf(&_DrmQueue_t, pQueue->frame, pQueue->buf_size, NULL);
                    buff_ready = 1;
                }
                else
                {
                    printf("get_last_queue _DeQueue_t ENQUEUE_STATE error \n");
					continue;

                }
                frame_queue_next(&_DeQueue_t, pQueue);
            }
            else
            {
                printf("MI_SYS_ChnOutputPortDequeueDmabuf error ret=%d \n", ret);
				continue;

            }
        }
        else
        {
            printf("MI_SYS_ChnOutputPortDequeueDmabuf fail dma_buf_handle=%d \n",dma_buf_handle);
        }
     }
	while (!buff_null) {

	};
	usleep(500*1000);
    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        sstar_release_dma_obj(&buf_obj->dma_info[i]);
    }

    
    if (MI_SUCCESS != MI_SYS_DestroyChnOutputPortDmabufCusAllocator(&mi_chn_port_info))
    {
        printf("warning: MI_SYS_DestroyChnOutputPortDmabufCusAllocator fail, device=%d, channel=%d \n",
               mi_chn_port_info.u32DevId, mi_chn_port_info.u32ChnId);
    }
    MI_SYS_CloseFd(mi_sys_fd);
    printf("Thread mi_buffer_loop exit \n");
    return (void*)0;
}


#define DRM_BUFF_TIMEOUT_COUNT 1000
void* drm_buffer_loop(void* param)
{
    frame_t *pQueue = NULL;
    int dma_buf_handle = -1;
    dma_info_t *dma_info;
    struct device * buf_obj = (struct device *)param;
    
    int count = DRM_BUFF_TIMEOUT_COUNT;
  

    while (!bExit || !buff_null)
    {
        while(!buff_ready && (count > 0)) {
            usleep(1*1000);
			count--;
	    }
		count = DRM_BUFF_TIMEOUT_COUNT;
		//buff_ready = 0;
        
        pQueue = get_last_queue(&_DrmQueue_t, DEQUEUE_STATE);
        if( pQueue !=NULL && pQueue->frame != NULL )
        {
            dma_info = (dma_info_t *)pQueue->frame;
            dma_buf_handle = dma_info->dma_buf_fd;
            if(0 != sstar_drm_update(buf_obj, dma_buf_handle))
            {
                printf("drm update frame buffer failed \n");
                frame_queue_next(&_DrmQueue_t, pQueue);
                continue;
            }
            dma_info->buf_in_use = IDLEQUEUE_STATE;
            frame_queue_putbuf(&_EnQueue_t, pQueue->frame, pQueue->buf_size, NULL);
        }
        else
        {
            //printf("get_last_queue _DrmQueue_t DEQUEUE_STATE error \n");
			if (bExit) {
                buff_null = true;
			}
            continue;
        }
        frame_queue_next(&_DrmQueue_t, pQueue);
    }

    printf("Thread drm_buffer_loop exit \n");
    return (void*)0;
}
*/
void display_help(void)
{
    printf("************************* sensor usage *************************\n");
    printf("-p : select sensor pad\n");
	printf("-c : select panel type\n");
	printf("-b : select iq file\n");
	printf("-r : rotate 0：NONE    1：90    2：180    3：270\n");
    printf("eg:./Drm_sensor -p 0 -c ttl/mipi -b iqfile -r 1 \n");

    return;
}



void  int_buf_obj()
{

    g_dev.video_info.format = DRM_FORMAT_NV12;
    //g_dev.video_info.v_src_width = OUTPUT_WIDTH;
    //g_dev.video_info.v_src_height = OUTPUT_HEIGHT;
    sem_init(&g_dev.sem_avail, 0, MAX_NUM_OF_DMABUFF);
}
/*
void creat_dmabuf_queue()
{
    frame_queue_init(&_EnQueue_t, MAX_NUM_OF_DMABUFF, 0);
    frame_queue_init(&_DeQueue_t, MAX_NUM_OF_DMABUFF, 0);
    frame_queue_init(&_DrmQueue_t, MAX_NUM_OF_DMABUFF, 0);
}

void destory_dmabuf_queue()
{
    frame_queue_destory(&_EnQueue_t);
    frame_queue_destory(&_DeQueue_t);
    frame_queue_destory(&_DrmQueue_t);
}
*/
void get_output_size(int *out_width, int *out_height)
{
    if(g_dev.video_info.v_out_width != 0 && g_dev.video_info.v_out_height != 0)
    {

        *out_width = g_dev.video_info.v_out_width;
        *out_height = g_dev.video_info.v_out_height;
        return ;
    }
}

void sstar_drm_open(struct device *dev)
{
	dev->fd = open("/dev/card0", O_RDWR | O_CLOEXEC);
    if (dev->fd < 0) {
      dev->fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    }
    //dev->fd = drmOpen("sstar", 0);
    if(dev->fd < 0)
    {
        printf("Failed to open device '%s': %s \n","sstar", strerror(errno));
        return;
    }
}


drmModePlane* sstar_drm_getplane(int fd, int plane_id)
{
    drmModePlane  *plane;
    plane = drmModeGetPlane(fd, plane_id);
    if(plane)
    {
        return plane;
    }
    return NULL;

}


static void sort_plane_type(int fd, int crtc_idx) 
{
    drmModePlaneResPtr plane_res;
    drmModePlanePtr plane_prt;
    drmModeObjectPropertiesPtr plane_props_ptr;
    drmModePropertyPtr plane_prop_ptr;
    int i, j;

    plane_res = drmModeGetPlaneResources(fd);
    if (!plane_res) {
        printf("drmModeGetPlaneResources failed: %s\n",
            strerror(errno));
    }
    for (i = 0; i < plane_res->count_planes; i++)
    {
         plane_prt = drmModeGetPlane(fd, plane_res->planes[i]);
         if (plane_prt->possible_crtcs != (1 << crtc_idx)) {
             continue;
         }
         plane_props_ptr = drmModeObjectGetProperties(fd, plane_prt->plane_id, DRM_MODE_OBJECT_PLANE);
         for (j = 0; j < plane_props_ptr->count_props; j++)
         {
             plane_prop_ptr = drmModeGetProperty(fd, plane_props_ptr->props[j]);
             if (!strcmp(plane_prop_ptr->name, "sstar type"))
             {
               plane_type[plane_props_ptr->prop_values[j]][plane_type_cnt[plane_props_ptr->prop_values[j]]++] = plane_prt->plane_id;		 

             }
             drmModeFreeProperty(plane_prop_ptr);
         }
         drmModeFreeObjectProperties(plane_props_ptr);
    }

}


uint32_t get_plane_with_format(int dev_fd, drmModePlaneRes *plane_res, uint32_t fmt)
{

    for(int i=0; i< plane_res->count_planes; i++)
    {
        drmModePlane *plane;
        plane = sstar_drm_getplane(dev_fd, plane_res->planes[i]);
        for(int j=0; j < plane->count_formats; j++)
        {
            if(plane->formats[j] == fmt)
            {
                //dump_fourcc(plane->formats[j]);
                printf("plane[%d]=%d crtc_id=%d possible_crtcs=%d format:%d \n",i,plane_res->planes[i],plane->crtc_id,plane->possible_crtcs,fmt);

                return plane_res->planes[i];
            }
        }
    }
    return -1;
}
int sstar_drm_init(struct device *dev)
{
    drmModeConnector *conn;
    int i;

    dev->resources = get_resources(dev);
    if(!dev->_g_res)
    {
	    dev->_g_res = drmModeGetResources(dev->fd);
    }
    

	dev->crtc_id = dev->_g_res->crtcs[0];
	//dev->conn_id = dev->_g_res->connectors[0];
	for (i = 0; i < dev->_g_res->count_connectors; i++)
	{
	    conn = drmModeGetConnector(dev->fd, dev->_g_res->connectors[i]);
		if (conn->connector_type == dev->connector_type)
		{
            dev->conn_id = dev->_g_res->connectors[i];
		    break;
		}
	}
    if (i >= dev->_g_res->count_connectors) {
        printf("can not find panel type, please check ! \n");
        return DRM_FAIL;
    }

    if(!dev->_g_conn)
    {
    	dev->_g_conn = drmModeGetConnector(dev->fd, dev->conn_id);
    }
	dev->width = dev->_g_conn->modes[0].hdisplay;
	dev->height = dev->_g_conn->modes[0].vdisplay;

    if(!dev->_g_plane_res)
    {
        drmSetClientCap(dev->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
        dev->_g_plane_res = drmModeGetPlaneResources(dev->fd);
    }
    sort_plane_type(dev->fd, 0);
	//dev->plane_id = get_plane_with_format(dev->fd, _g_plane_res, dev->format);//plane_res->planes[1];
	//dev->video_info.plane_id = get_plane_with_format(dev->fd, dev->_g_plane_res, dev->video_info.format);//plane_res->planes[1];
    dev->video_info.plane_id = plane_type[MOPS][0];
    printf("video_info_plane_id=%d dev->crtc_id %d dev->conn_id %d\n",dev->video_info.plane_id,dev->crtc_id,dev->conn_id);

    dev->req = drmModeAtomicAlloc();
	if (!dev->req) {
        printf("drmModeAtomicAlloc failed \n");
    }

    return DRM_SUCCESS;
}


void sstar_mode_set(struct device *dev)
{
    uint32_t blob_id;

    drmModeCreatePropertyBlob(dev->fd, &dev->_g_conn->modes[0],sizeof(dev->_g_conn->modes[0]), &blob_id);
    add_property(dev, dev->conn_id, "CRTC_ID", dev->crtc_id);
    add_property(dev, dev->crtc_id, "MODE_ID", blob_id);
    add_property(dev, dev->crtc_id, "ACTIVE", 1);
	add_property(dev, dev->crtc_id, "OUT_FENCE_PTR", (unsigned long long)&dev->video_info.out_fence);


    return ;
}

int get_vif_from_snrpad(MI_SNR_PADID eSnrPad, MI_VIF_GROUP *vifGroupId, MI_VIF_DEV *u32VifDev)
{
    switch(eSnrPad)
    {
        case 0:
            {
                *vifGroupId = 0;
                *u32VifDev = 0;
                break;
            }
        case 1:
            {
                *vifGroupId = 2;
                *u32VifDev = 8;
                break;
            }
        case 2:
            {
                *vifGroupId = 1;
                *u32VifDev = 4;
                break;
            }
        case 3:
            {
                *vifGroupId = 3;
                *u32VifDev = 12;
                break;
            }
        default:
            {
                printf("Invalid SnrPadid \n");
                return -1;
            }
    }
    return 0;

}

MI_S32 Camera_PipelineInit(int sensorIdx) 
{
    MI_SNR_PADInfo_t stSnrPadInfo;
    MI_SNR_PlaneInfo_t stSnrPlaneInfo;
    MI_SNR_Res_t stSnrRes;
    MI_U32 u32ResCount = 0;
    MI_U32 choiceRes = 0;


    MI_SNR_PADID snrPadId = sensorIdx;
    ///MI_VIF_DEV s32vifDev = (MI_VIF_DEV)gs8SnrPad;
    //MI_VIF_CHN s32vifChn = (MI_VIF_CHN)(gs8SnrPad * 4);

    ExecFunc(SysInit(), DRM_SUCCESS);
    memset(&stSnrPadInfo, 0x0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlaneInfo, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stSnrRes, 0x0, sizeof(MI_SNR_Res_t));
    ExecFunc(MI_SNR_SetPlaneMode(snrPadId, FALSE), DRM_SUCCESS);
    ExecFunc(MI_SNR_QueryResCount(snrPadId, &u32ResCount), DRM_SUCCESS);
    for (int u8ResIndex = 0; u8ResIndex < u32ResCount; u8ResIndex++) 
	{
        ExecFunc(MI_SNR_GetRes(snrPadId, u8ResIndex, &stSnrRes), DRM_SUCCESS);
        printf(
                "index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc "
                "%s\n",
                u8ResIndex, stSnrRes.stCropRect.u16X, stSnrRes.stCropRect.u16Y,
                stSnrRes.stCropRect.u16Width, stSnrRes.stCropRect.u16Height,
                stSnrRes.stOutputSize.u16Width, stSnrRes.stOutputSize.u16Height, stSnrRes.u32MaxFps,
                stSnrRes.u32MinFps, stSnrRes.strResDesc);
    }

    ExecFunc(MI_SNR_GetRes(snrPadId, choiceRes, &stSnrRes), DRM_SUCCESS);
    ExecFunc(MI_SNR_SetRes(snrPadId, choiceRes), DRM_SUCCESS);
    ExecFunc(MI_SNR_Enable(snrPadId), DRM_SUCCESS);
    printf("Snr pad id:%d, res count:%d, You select %d res\n", (int)snrPadId, u32ResCount,
               choiceRes);
    /************************************************
    Step1:  Init Vif
    *************************************************/
    MI_VIF_GROUP VifGroupId = 0;
    MI_VIF_DEV VifDevId = 0;
    MI_VIF_DEV VifChnId = 0;
    MI_VIF_PORT VifPortId = 0;
    MI_SYS_ChnPort_t stChnPort;
    MI_VIF_GroupAttr_t stVifGroupAttr;
    MI_VIF_DevAttr_t stVifDevAttr;
    MI_VIF_OutputPortAttr_t stVifPortAttr;

    get_vif_from_snrpad(sensorIdx, &VifGroupId, &VifDevId);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stVifGroupAttr, 0x0, sizeof(MI_VIF_GroupAttr_t));
    memset(&stVifDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));
    memset(&stVifPortAttr, 0x0, sizeof(MI_VIF_OutputPortAttr_t));
    ExecFunc(MI_SNR_GetPadInfo(snrPadId, &stSnrPadInfo), DRM_SUCCESS);
    ExecFunc(MI_SNR_GetPlaneInfo(snrPadId, 0, &stSnrPlaneInfo), DRM_SUCCESS);
    printf(
                "MI_SNR_GetPlaneInfo %d, outputsize(%d, %d, %d, %d)\n",
                snrPadId,stSnrPlaneInfo.stCapRect.u16X,stSnrPlaneInfo.stCapRect.u16Y,
                stSnrPlaneInfo.stCapRect.u16Width,stSnrPlaneInfo.stCapRect.u16Height);

    stVifGroupAttr.eIntfMode = E_MI_VIF_MODE_MIPI;
    stVifGroupAttr.eWorkMode = E_MI_VIF_WORK_MODE_1MULTIPLEX;
    stVifGroupAttr.eHDRType = E_MI_VIF_HDR_TYPE_OFF;
    if (stVifGroupAttr.eIntfMode == E_MI_VIF_MODE_BT656) {
        stVifGroupAttr.eClkEdge = (MI_VIF_ClkEdge_e)stSnrPadInfo.unIntfAttr.stBt656Attr.eClkEdge;
    } else {
        stVifGroupAttr.eClkEdge = E_MI_VIF_CLK_EDGE_DOUBLE;
    }
    ExecFunc(MI_VIF_CreateDevGroup(VifGroupId, &stVifGroupAttr), DRM_SUCCESS);
    stVifDevAttr.stInputRect.u16X = stSnrPlaneInfo.stCapRect.u16X;
    stVifDevAttr.stInputRect.u16Y = stSnrPlaneInfo.stCapRect.u16Y;
    stVifDevAttr.stInputRect.u16Width = stSnrPlaneInfo.stCapRect.u16Width;
    stVifDevAttr.stInputRect.u16Height = stSnrPlaneInfo.stCapRect.u16Height;
    if (stSnrPlaneInfo.eBayerId >= E_MI_SYS_PIXEL_BAYERID_MAX) {
        stVifDevAttr.eInputPixel = stSnrPlaneInfo.ePixel;
    } else {
        stVifDevAttr.eInputPixel = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(
                stSnrPlaneInfo.ePixPrecision, stSnrPlaneInfo.eBayerId);
    }
    ExecFunc(MI_VIF_SetDevAttr(VifDevId, &stVifDevAttr), DRM_SUCCESS);
    ExecFunc(MI_VIF_EnableDev(VifDevId), DRM_SUCCESS);
    stVifPortAttr.stCapRect.u16X = stSnrPlaneInfo.stCapRect.u16X;
    stVifPortAttr.stCapRect.u16Y = stSnrPlaneInfo.stCapRect.u16Y;
    stVifPortAttr.stCapRect.u16Width = stSnrPlaneInfo.stCapRect.u16Width;
    stVifPortAttr.stCapRect.u16Height = stSnrPlaneInfo.stCapRect.u16Height;
    stVifPortAttr.stDestSize.u16Width = stSnrPlaneInfo.stCapRect.u16Width;
    stVifPortAttr.stDestSize.u16Height = stSnrPlaneInfo.stCapRect.u16Height;
    stVifPortAttr.eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    if (stSnrPlaneInfo.eBayerId >= E_MI_SYS_PIXEL_BAYERID_MAX) {
        stVifPortAttr.ePixFormat = stSnrPlaneInfo.ePixel;
    } else {
        stVifPortAttr.ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(
                stSnrPlaneInfo.ePixPrecision, stSnrPlaneInfo.eBayerId);
    }
    ExecFunc(MI_VIF_SetOutputPortAttr(VifDevId, VifPortId, &stVifPortAttr), DRM_SUCCESS);
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = VifDevId;
    stChnPort.u32ChnId = VifChnId;
    stChnPort.u32PortId = VifPortId;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(0, &stChnPort, 0, 4), DRM_SUCCESS);
    ExecFunc(MI_VIF_EnableOutputPort(VifDevId, VifPortId), DRM_SUCCESS);
    printf("vif enabled, dev: %d, chn:%d, port:%d\n", (int)VifDevId, (int)VifChnId, VifPortId);
    /************************************************
    Step2:  Init Isp
    *************************************************/
    MI_ISP_DEV mIspDev = 0;
    MI_ISP_CHANNEL mIspChn = 0;
	MI_ISP_PORT ispPort = 0;
    MI_ISP_DevAttr_t stIspDevAttr;
    MI_ISP_ChannelAttr_t stIspChnAttr;
    MI_ISP_ChnParam_t stIspChnParam;
    memset(&stIspDevAttr, 0x0, sizeof(MI_ISP_DevAttr_t));
    memset(&stIspChnAttr, 0x0, sizeof(MI_ISP_ChannelAttr_t));
    memset(&stIspChnParam, 0x0, sizeof(MI_ISP_ChnParam_t));
    stIspDevAttr.u32DevStitchMask = E_MI_ISP_DEVICEMASK_ID0 | E_MI_ISP_DEVICEMASK_ID1;
    
    ExecFunc(MI_ISP_CreateDevice(mIspDev, &stIspDevAttr), DRM_SUCCESS);

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
            return DRM_FAIL;
    }
    stIspChnParam.eHDRType = E_MI_ISP_HDR_TYPE_OFF;
    stIspChnParam.e3DNRLevel = E_MI_ISP_3DNR_LEVEL1;
    stIspChnParam.bMirror = FALSE;
    stIspChnParam.bFlip = FALSE;
    stIspChnParam.eRot = E_MI_SYS_ROTATE_NONE;
    ExecFunc(MI_ISP_CreateChannel(mIspDev, mIspChn, &stIspChnAttr), DRM_SUCCESS);
    ExecFunc(MI_ISP_SetChnParam(mIspDev, mIspChn, &stIspChnParam), DRM_SUCCESS);
    ExecFunc(MI_ISP_StartChannel(mIspDev, mIspChn), DRM_SUCCESS);
    printf("isp chn enabled, dev: %d, chn:%d\n", (int)mIspDev, (int)mIspChn);
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
    stBindInfo.stDstChnPort.u32DevId = mIspDev;
    stBindInfo.stDstChnPort.u32ChnId = mIspChn;
    stBindInfo.u32SrcFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.u32DstFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    ExecFunc(SysBind(&stBindInfo), DRM_SUCCESS);
    printf("bind VIF(%d %d %d) -> ISP(%d %d), bind type:%d", (int)VifDevId, (int)VifChnId,
               (int)VifPortId, (int)mIspDev, (int)mIspChn, E_MI_SYS_BIND_TYPE_REALTIME);
    /************************************************
    Step4:  Init Scl
    *************************************************/
    MI_SCL_DEV SclDevId = 0;
	MI_SCL_CHANNEL sclChn = 0;
	MI_SCL_PORT sclPort = 0;
    MI_SCL_DevAttr_t stSclDevAttr;
    MI_SCL_ChannelAttr_t stSclChnAttr;
    MI_SCL_ChnParam_t stSclChnParam;
    memset(&stSclDevAttr, 0x0, sizeof(MI_SCL_DevAttr_t));
    memset(&stSclChnAttr, 0x0, sizeof(MI_SCL_ChannelAttr_t));
    memset(&stSclChnParam, 0x0, sizeof(MI_SCL_ChnParam_t));
    stSclDevAttr.u32NeedUseHWOutPortMask = E_MI_SCL_HWSCL0 | E_MI_SCL_HWSCL2 | E_MI_SCL_HWSCL3;

    ExecFunc(MI_SCL_CreateDevice(SclDevId, &stSclDevAttr), DRM_SUCCESS);
    ExecFunc(MI_SCL_CreateChannel(SclDevId, sclChn, &stSclChnAttr), DRM_SUCCESS);
    ExecFunc(MI_SCL_SetChnParam(SclDevId, sclChn, &stSclChnParam), DRM_SUCCESS);
    ExecFunc(MI_SCL_StartChannel(SclDevId, sclChn), DRM_SUCCESS);
    printf("scl chn start, (%d %d)", (int)SclDevId, (int)sclChn);
    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_ISP;
    stBindInfo.stSrcChnPort.u32DevId = mIspDev;
    stBindInfo.stSrcChnPort.u32ChnId = mIspChn;
    stBindInfo.stSrcChnPort.u32PortId = ispPort;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stDstChnPort.u32DevId = SclDevId;
    stBindInfo.stDstChnPort.u32ChnId = sclChn;
    stBindInfo.u32SrcFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.u32DstFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    ExecFunc(SysBind(&stBindInfo), DRM_SUCCESS);
    printf("bind ISP(%d %d %d)->SCL(%d %d),bind type:%d ", (int)mIspDev, (int)mIspChn,
               (int)ispPort, (int)SclDevId, (int)sclChn, E_MI_SYS_BIND_TYPE_REALTIME);
    MI_ISP_OutPortParam_t stIspOutputParam;
    memset(&stIspOutputParam, 0x0, sizeof(MI_ISP_OutPortParam_t));
    ExecFunc(MI_ISP_GetInputPortCrop(mIspDev, mIspChn, &stIspOutputParam.stCropRect), DRM_SUCCESS);
    stIspOutputParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    ExecFunc(MI_ISP_SetOutputPortParam(mIspDev, mIspChn, ispPort, &stIspOutputParam), DRM_SUCCESS);
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_ISP;
    stChnPort.u32DevId = mIspDev;
    stChnPort.u32ChnId = mIspChn;
    stChnPort.u32PortId = ispPort;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(0, &stChnPort, 0, 3), DRM_SUCCESS);
    ExecFunc(MI_ISP_EnableOutputPort(mIspDev, mIspChn, ispPort), DRM_SUCCESS);
	MI_SCL_OutPortParam_t pstOutPortParam;
	memset(&pstOutPortParam, 0x0, sizeof(MI_SCL_OutPortParam_t));
	ExecFunc(MI_SCL_GetInputPortCrop(SclDevId, sclChn, &pstOutPortParam.stSCLOutCropRect), DRM_SUCCESS);
	printf("MI_SCL_GetInputPortCrop (%d, %d, %d, %d) \n",pstOutPortParam.stSCLOutCropRect.u16X,pstOutPortParam.stSCLOutCropRect.u16Y,
	pstOutPortParam.stSCLOutCropRect.u16Width, pstOutPortParam.stSCLOutCropRect.u16Height);
	pstOutPortParam.stSCLOutCropRect.u16X = 0;
    pstOutPortParam.stSCLOutCropRect.u16Y = 0;
	//pstOutPortParam.stSCLOutCropRect.u16Width = stSnrPlaneInfo.stCapRect.u16Width;
	//pstOutPortParam.stSCLOutCropRect.u16Height = stSnrPlaneInfo.stCapRect.u16Height;
	pstOutPortParam.stSCLOutCropRect.u16Width = pstOutPortParam.stSCLOutCropRect.u16Width;
	pstOutPortParam.stSCLOutCropRect.u16Height = pstOutPortParam.stSCLOutCropRect.u16Height;
	if ((g_rotate == E_MI_SYS_ROTATE_90) || (g_rotate == E_MI_SYS_ROTATE_270)) {
	    pstOutPortParam.stSCLOutputSize.u16Width = ALIGN_UP(g_dev.video_info.v_out_height, ALIGN_NUM);
	    pstOutPortParam.stSCLOutputSize.u16Height = ALIGN_UP(g_dev.video_info.v_out_width, ALIGN_NUM);
	} else { 
      pstOutPortParam.stSCLOutputSize.u16Width = ALIGN_UP(g_dev.video_info.v_out_width, ALIGN_NUM);
	  pstOutPortParam.stSCLOutputSize.u16Height = ALIGN_UP(g_dev.video_info.v_out_height, ALIGN_NUM);
	}
	pstOutPortParam.bFlip = FALSE;
	pstOutPortParam.bMirror = FALSE;
	pstOutPortParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
	ExecFunc(MI_SCL_SetOutputPortParam(SclDevId, sclChn, sclPort, &pstOutPortParam), DRM_SUCCESS);
	memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_SCL;
    stChnPort.u32DevId = SclDevId;
    stChnPort.u32ChnId = sclChn;
    stChnPort.u32PortId = sclPort;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(0, &stChnPort, 0, 3), DRM_SUCCESS);
    ExecFunc(MI_SCL_EnableOutputPort(SclDevId, sclChn, sclPort), DRM_SUCCESS);	

	/************************************************
	Step5:  Init Rgn
	*************************************************/	
	MI_RGN_Attr_t stRegion;
	MI_RGN_ChnPort_t stRgnChnPort;
	MI_RGN_ChnPortParam_t stRgnChnPortParam;

	g_detection_info.disp_size.width = pstOutPortParam.stSCLOutputSize.u16Width;
	g_detection_info.disp_size.height = pstOutPortParam.stSCLOutputSize.u16Height;
	printf("detection_info.disp_size.width = %d,detection_info.disp_size.height = %d\n",g_detection_info.disp_size.width, g_detection_info.disp_size.height);

	ST_OSD_Init(0);
	stRegion.eType = E_MI_RGN_TYPE_OSD;
	stRegion.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_I4;
	stRegion.stOsdInitParam.stSize.u32Width = g_detection_info.disp_size.width;
	stRegion.stOsdInitParam.stSize.u32Height = g_detection_info.disp_size.height;
	ST_OSD_Create(0, g_stRgnHandle, &stRegion);
	stRgnChnPort.eModId = E_MI_MODULE_ID_SCL;
	stRgnChnPort.s32DevId = SclDevId;
	stRgnChnPort.s32ChnId = sclChn;
	stRgnChnPort.s32PortId = sclPort;
	memset(&stRgnChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
	stRgnChnPortParam.bShow = TRUE;
	stRgnChnPortParam.stPoint.u32X = 0;
	stRgnChnPortParam.stPoint.u32Y = 0;
	stRgnChnPortParam.unPara.stOsdChnPort.u32Layer = 99;
	ExecFunc(MI_RGN_AttachToChn(0, g_stRgnHandle, &stRgnChnPort, &stRgnChnPortParam), DRM_SUCCESS);

    g_detection_rtsp_info.disp_size.width = stSnrPlaneInfo.stCapRect.u16Width;
	g_detection_rtsp_info.disp_size.height = stSnrPlaneInfo.stCapRect.u16Height;
	stRegion.stOsdInitParam.stSize.u32Width = g_detection_rtsp_info.disp_size.width;
	stRegion.stOsdInitParam.stSize.u32Height = g_detection_rtsp_info.disp_size.height;
	ST_OSD_Create(0, g_stRgnHandle_rtsp, &stRegion);
	stRgnChnPort.eModId = E_MI_MODULE_ID_SCL;
	stRgnChnPort.s32DevId = SclDevId;
	stRgnChnPort.s32ChnId = sclChn;
	stRgnChnPort.s32PortId = 2;
	memset(&stRgnChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
	stRgnChnPortParam.bShow = TRUE;
	stRgnChnPortParam.stPoint.u32X = 0;
	stRgnChnPortParam.stPoint.u32Y = 0;
	stRgnChnPortParam.unPara.stOsdChnPort.u32Layer = 99;
	ExecFunc(MI_RGN_AttachToChn(0, g_stRgnHandle_rtsp, &stRgnChnPort, &stRgnChnPortParam), DRM_SUCCESS);

	/************************************************
    Step6:  Init Scl for Algo
    *************************************************/
	sclPort = 1; 
	memset(&pstOutPortParam, 0x0, sizeof(MI_SCL_OutPortParam_t));
	pstOutPortParam.stSCLOutputSize.u16Width = g_stAlgoRes.width;
	pstOutPortParam.stSCLOutputSize.u16Height = g_stAlgoRes.height;
	pstOutPortParam.ePixelFormat = g_eAlgoFormat;
	pstOutPortParam.bMirror = 0;
	pstOutPortParam.bFlip = 0;
	ExecFunc(MI_SCL_SetOutputPortParam(SclDevId, sclChn, sclPort, &pstOutPortParam), DRM_SUCCESS);
	ExecFunc(MI_SCL_EnableOutputPort(SclDevId, sclChn, sclPort), DRM_SUCCESS);

    if (g_rotate > E_MI_SYS_ROTATE_NONE) {
    /************************************************
    Step7:  scl rotate
    *************************************************/
    SclDevId = 7;
    sclChn = 0;
    sclPort = 0;
    memset(&stSclDevAttr, 0x0, sizeof(MI_SCL_DevAttr_t));
    memset(&stSclChnAttr, 0x0, sizeof(MI_SCL_ChannelAttr_t));
    memset(&stSclChnParam, 0x0, sizeof(MI_SCL_ChnParam_t));
    stSclDevAttr.u32NeedUseHWOutPortMask = E_MI_SCL_HWSCL1;
    stSclChnParam.eRot = g_rotate;
    
    ExecFunc(MI_SCL_CreateDevice(SclDevId, &stSclDevAttr), DRM_SUCCESS);
    ExecFunc(MI_SCL_CreateChannel(SclDevId, sclChn, &stSclChnAttr), DRM_SUCCESS);
    ExecFunc(MI_SCL_SetChnParam(SclDevId, sclChn, &stSclChnParam), DRM_SUCCESS);
    ExecFunc(MI_SCL_StartChannel(SclDevId, sclChn), DRM_SUCCESS);
    printf("scl chn start, (%d %d)", (int)SclDevId, (int)sclChn);
    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stDstChnPort.u32DevId = SclDevId;
    stBindInfo.stDstChnPort.u32ChnId = sclChn;
	stBindInfo.stDstChnPort.u32PortId = sclPort;
    stBindInfo.u32SrcFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.u32DstFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    ExecFunc(SysBind(&stBindInfo), DRM_SUCCESS);
    printf("bind ISP(%d %d %d)->SCL(%d %d),bind type:%d ", (int)mIspDev, (int)mIspChn,
               (int)ispPort, (int)SclDevId, (int)sclChn, E_MI_SYS_BIND_TYPE_REALTIME);
	memset(&pstOutPortParam, 0x0, sizeof(MI_SCL_OutPortParam_t));
	//ExecFunc(MI_SCL_GetInputPortCrop(SclDevId, sclChn, &pstOutPortParam.stSCLOutCropRect), DRM_SUCCESS);
	//printf("MI_SCL_GetInputPortCrop (%d, %d, %d, %d) \n",pstOutPortParam.stSCLOutCropRect.u16X,pstOutPortParam.stSCLOutCropRect.u16Y,
	//pstOutPortParam.stSCLOutCropRect.u16Width, pstOutPortParam.stSCLOutCropRect.u16Height);
	pstOutPortParam.stSCLOutCropRect.u16X = 0;
    pstOutPortParam.stSCLOutCropRect.u16Y = 0;
	pstOutPortParam.stSCLOutCropRect.u16Width = pstOutPortParam.stSCLOutCropRect.u16Width;
	pstOutPortParam.stSCLOutCropRect.u16Height = pstOutPortParam.stSCLOutCropRect.u16Height;

    pstOutPortParam.stSCLOutputSize.u16Width = ALIGN_UP(g_dev.video_info.v_out_width, ALIGN_NUM);
	pstOutPortParam.stSCLOutputSize.u16Height = ALIGN_UP(g_dev.video_info.v_out_height, ALIGN_NUM);
	
	pstOutPortParam.bFlip = FALSE;
	pstOutPortParam.bMirror = FALSE;
	pstOutPortParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
	ExecFunc(MI_SCL_SetOutputPortParam(SclDevId, sclChn, sclPort, &pstOutPortParam), DRM_SUCCESS);
	memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_SCL;
    stChnPort.u32DevId = SclDevId;
    stChnPort.u32ChnId = sclChn;
    stChnPort.u32PortId = sclPort;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(0, &stChnPort, 0, 3), DRM_SUCCESS);
    ExecFunc(MI_SCL_EnableOutputPort(SclDevId, sclChn, sclPort), DRM_SUCCESS);	
    }
	/************************************************
    Step6:  Init Scl for venc
    *************************************************/
    SclDevId = 0;
    sclChn = 0;
    sclPort = 2;
	memset(&pstOutPortParam, 0x0, sizeof(MI_SCL_OutPortParam_t));
	pstOutPortParam.stSCLOutputSize.u16Width = stSnrPlaneInfo.stCapRect.u16Width;
	pstOutPortParam.stSCLOutputSize.u16Height = stSnrPlaneInfo.stCapRect.u16Height;
	pstOutPortParam.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
	pstOutPortParam.bMirror = 0;
	pstOutPortParam.bFlip = 0;
	ExecFunc(MI_SCL_SetOutputPortParam(SclDevId, sclChn, sclPort, &pstOutPortParam), DRM_SUCCESS);
	ExecFunc(MI_SCL_EnableOutputPort(SclDevId, sclChn, sclPort), DRM_SUCCESS);

    /************************************************
    Step8:  venc init
    *************************************************/
    MI_VENC_DEV VencDevId = 0;
    MI_VENC_CHN VencChnId = 0;
    MI_S32 VencPortId = 0;
    MI_VENC_ChnAttr_t           stVencChnAttr;
    MI_VENC_InputSourceConfig_t stVencInputSourceConfig;
	MI_VENC_InitParam_t stInitParam;

	memset(&stInitParam, 0, sizeof(MI_VENC_InitParam_t));
	stInitParam.u32MaxWidth = stSnrPlaneInfo.stCapRect.u16Width;
    stInitParam.u32MaxHeight = stSnrPlaneInfo.stCapRect.u16Height;
	ExecFunc(MI_VENC_CreateDev(VencDevId, &stInitParam), DRM_SUCCESS);
	
    /* 设置编码器属性与码率控制属性 */
    memset(&stVencChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    stVencChnAttr.stVeAttr.stAttrH264e.u32PicWidth         = stSnrPlaneInfo.stCapRect.u16Width;
    stVencChnAttr.stVeAttr.stAttrH264e.u32PicHeight        = stSnrPlaneInfo.stCapRect.u16Height;
    stVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth      = stSnrPlaneInfo.stCapRect.u16Width;
    stVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight     = stSnrPlaneInfo.stCapRect.u16Height;
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
    ExecFunc(MI_VENC_CreateChn(VencDevId, VencChnId, &stVencChnAttr), DRM_SUCCESS);
 
    /*! 设置输入源的属性
     *  设置了E_MI_VENC_INPUT_MODE_RING_ONE_FRM那APP 在调用 MI_SYS_BindChnPort2 需要设置
     *  E_MI_SYS_BIND_TYPE_HW_RING/和相应 ring buffer 高度
     *  */
    memset(&stVencInputSourceConfig, 0, sizeof(MI_VENC_InputSourceConfig_t));
    stVencInputSourceConfig.eInputSrcBufferMode = E_MI_VENC_INPUT_MODE_NORMAL_FRMBASE;
    ExecFunc(MI_VENC_SetInputSourceConfig(VencDevId, VencChnId, &stVencInputSourceConfig), DRM_SUCCESS);
 
    ExecFunc(MI_VENC_SetMaxStreamCnt(VencDevId, VencChnId, 4), DRM_SUCCESS);
	memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 2;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = VencDevId;
    stBindInfo.stDstChnPort.u32ChnId = VencChnId;
	stBindInfo.stDstChnPort.u32PortId = VencPortId;
    stBindInfo.u32SrcFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.u32DstFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
	//stBindInfo.u32BindParam = stSnrPlaneInfo.stCapRect.u16Height;
    ExecFunc(SysBind(&stBindInfo), DRM_SUCCESS);
    printf("bind SCL(%d %d %d)->VENC(%d %d),bind type:%d ", (int)0, (int)0,
               (int)0, (int)VencDevId, (int)VencChnId, stBindInfo.eBindType);
	ExecFunc(MI_VENC_StartRecvPic(VencDevId, VencChnId), DRM_SUCCESS);
    /************************************************
    Step9:  Optional
    *************************************************/
   
    if(g_enable_iqserver)
    {
        ExecFunc(MI_IQSERVER_SetDataPath((char*)defaultIqPath), DRM_SUCCESS);
        printf("MI_IQSERVER_Open...\n");
        ExecFunc(MI_IQSERVER_Open(), DRM_SUCCESS);

    }
	ExecFunc(MI_ISP_ApiCmdLoadBinFile(mIspDev, mIspChn, (char *)iqfile_path,  0), DRM_SUCCESS);

    return DRM_SUCCESS;
}

MI_S32 Camera_PipelineDeinit(int sensorIdx) 
{
    MI_VIF_GROUP VifGroupId;
    MI_VIF_DEV VifDevId;
    MI_VIF_DEV VifChnId = 0;
    MI_VIF_PORT VifPortId = 0;
    MI_SNR_Res_t stSnrRes;
	MI_SNR_PADID snrPadId = sensorIdx;
    Sys_BindInfo_T stBindInfo;

    memset(&stSnrRes, 0x0, sizeof(MI_SNR_Res_t));
    ExecFunc(MI_SNR_GetRes(snrPadId, 0, &stSnrRes), DRM_SUCCESS);

    VifGroupId = 0;
    VifDevId = 0;

    get_vif_from_snrpad(sensorIdx, &VifGroupId, &VifDevId);

    MI_ISP_DEV IspDevId = 0;
    MI_ISP_CHANNEL IspChnId = 0;
    MI_ISP_PORT IspPort = 0;
    MI_SCL_DEV SclDevId = 0;
	MI_SCL_CHANNEL sclChn = 0;
	MI_SCL_PORT sclPort = 1;


    /************************************************
    Step1:  Unbind Scl and venc
    *************************************************/
    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 2;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.u32DstFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    ExecFunc(SysUnbind(&stBindInfo), DRM_SUCCESS);
	/************************************************
     Step1:	Unbind Scl and venc
    *************************************************/
	ExecFunc(MI_VENC_StopRecvPic(0, 0), DRM_SUCCESS);
    ExecFunc(MI_VENC_DestroyChn(0, 0), DRM_SUCCESS);
	ExecFunc(MI_VENC_DestroyDev(0), DRM_SUCCESS);
	/************************************************
    Step1:  Unbind Scl and Isp
    *************************************************/
    ExecFunc(MI_SCL_DisableOutputPort(SclDevId, sclChn, sclPort), DRM_SUCCESS);
	ExecFunc(MI_SCL_DisableOutputPort(SclDevId, sclChn, 2), DRM_SUCCESS);
	if (g_rotate > E_MI_SYS_ROTATE_NONE) {
    ExecFunc(MI_SCL_DisableOutputPort(7, sclChn, 0), DRM_SUCCESS);

	/************************************************
    Step1:  Unbind Scl and Scl
    *************************************************/
    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stDstChnPort.u32DevId = 7;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.u32DstFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    ExecFunc(SysUnbind(&stBindInfo), DRM_SUCCESS);
	}
    /************************************************
    Step1:  Unbind Scl and Isp
    *************************************************/
    sclPort = 0;
    MI_RGN_ChnPort_t stRgnChnPort;
    stRgnChnPort.eModId = E_MI_MODULE_ID_SCL;
    stRgnChnPort.s32DevId = SclDevId;
    stRgnChnPort.s32ChnId = sclChn;
    stRgnChnPort.s32PortId = sclPort;
    ExecFunc(MI_RGN_DetachFromChn(0, g_stRgnHandle, &stRgnChnPort), DRM_SUCCESS);
	stRgnChnPort.s32PortId = 2;
	ExecFunc(MI_RGN_DetachFromChn(0, g_stRgnHandle_rtsp, &stRgnChnPort), DRM_SUCCESS);
    ST_OSD_Destroy(0, g_stRgnHandle);
	ST_OSD_Destroy(0, g_stRgnHandle_rtsp);
    ST_OSD_Deinit(0);

    /************************************************
    Step1:  Unbind Scl and Isp
    *************************************************/
    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_ISP;
    stBindInfo.stSrcChnPort.u32DevId = IspDevId;
    stBindInfo.stSrcChnPort.u32ChnId = IspChnId;
    stBindInfo.stSrcChnPort.u32PortId = IspPort;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
    stBindInfo.stDstChnPort.u32DevId = SclDevId;
    stBindInfo.stDstChnPort.u32ChnId = sclChn;
    stBindInfo.stDstChnPort.u32PortId = sclPort;
    stBindInfo.u32SrcFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.u32DstFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    ExecFunc(SysUnbind(&stBindInfo), DRM_SUCCESS);




    /************************************************
    Step0:  Deinit Scl
    *************************************************/
    if (g_rotate > E_MI_SYS_ROTATE_NONE) {
    ExecFunc(MI_SCL_StopChannel(7, IspChnId), DRM_SUCCESS);
    ExecFunc(MI_SCL_DestroyChannel(7, IspChnId), DRM_SUCCESS);
    ExecFunc(MI_SCL_DestroyDevice(7), DRM_SUCCESS);
    }
    ExecFunc(MI_SCL_DisableOutputPort(SclDevId, sclChn, sclPort), DRM_SUCCESS);
    ExecFunc(MI_SCL_StopChannel(SclDevId, IspChnId), DRM_SUCCESS);
    ExecFunc(MI_SCL_DestroyChannel(SclDevId, IspChnId), DRM_SUCCESS);

    ExecFunc(MI_SCL_DestroyDevice(SclDevId), DRM_SUCCESS);

    /************************************************
    Step1:  Unbind Vif and Isp
    *************************************************/
    memset(&stBindInfo, 0x0, sizeof(Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = VifDevId;
    stBindInfo.stSrcChnPort.u32ChnId = VifChnId;
    stBindInfo.stSrcChnPort.u32PortId = VifPortId;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_ISP;
    stBindInfo.stDstChnPort.u32DevId = IspDevId;
    stBindInfo.stDstChnPort.u32ChnId = IspChnId;
    stBindInfo.u32SrcFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.u32DstFrmrate = stSnrRes.u32MaxFps;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    ExecFunc(SysUnbind(&stBindInfo), DRM_SUCCESS);
    /************************************************
    Step2:  Deinit lsp
    *************************************************/
    ExecFunc(MI_ISP_StopChannel(IspDevId, IspChnId), DRM_SUCCESS);
    ExecFunc(MI_ISP_DestroyChannel(IspDevId, IspChnId), DRM_SUCCESS);
    ExecFunc(MI_ISP_DestoryDevice(IspDevId), DRM_SUCCESS);

    /************************************************
    Step3:  Deinit Vif
    *************************************************/
    ExecFunc(MI_VIF_DisableOutputPort(VifDevId, VifPortId), DRM_SUCCESS);
    ExecFunc(MI_VIF_DisableDev(VifDevId), DRM_SUCCESS);
    ExecFunc(MI_VIF_DestroyDevGroup(VifGroupId), DRM_SUCCESS);

    /************************************************
    Step4:  Deinit Sensor
    *************************************************/
    ExecFunc(MI_SNR_Disable(snrPadId), DRM_SUCCESS);
	/************************************************
    Step5:  Optional
    *************************************************/
    if(g_enable_iqserver)
    {
        ExecFunc(MI_IQSERVER_Close(), DRM_SUCCESS);
    }
    return DRM_SUCCESS;
}

void IPU_DetectionInit(void)
{
    getDetectionManager(&g_detection_manager);
    initDetection(g_detection_manager, &g_detection_info);
    startDetect(g_detection_manager);
	getDetectionManager(&g_detection_manager_rtsp);
    initDetection(g_detection_manager_rtsp, &g_detection_rtsp_info);
    startDetect(g_detection_manager_rtsp);
}

int main(int argc, char **argv)
{
    MI_S32 s32Opt = 0;
    pthread_t tid_algo_thread;
    pthread_t tid_drm_buf_thread;
    pthread_t tid_enqueue_buf_thread;
    int ret;
    char connector_name[20];
	
    bExit = 0;

    memset(&g_dev, 0, sizeof(g_dev));
	memset(&connector_name, 0, 20);
    while ((s32Opt = getopt(argc, argv, "h:p:c:b:r:q")) != -1 )
    {
        switch(s32Opt)
        {
            case 'p':
            {
				if (optarg == NULL) {
				    g_dev.sensorIdx = 0;
				} else {
                    g_dev.sensorIdx = atoi(optarg);
				}
                break;
            }
            case 'c':
            {
                if (optarg == NULL) {
					printf("Missing panel type -c 'ttl or mipi' \n");
				    break; 

				}
				printf("Missing panel type %s %d\n",optarg,strlen(optarg));
				memcpy(connector_name, optarg, strlen(optarg));
				printf("connector_name %s %d\n",connector_name,strlen(connector_name));
				if (!strcmp(connector_name, "ttl") || !strcmp(connector_name, "Ttl") || !strcmp(connector_name, "TTL")) {
                    g_dev.connector_type = DRM_MODE_CONNECTOR_DPI;
				} else if (!strcmp(connector_name, "mipi") || !strcmp(connector_name, "Mipi") || !strcmp(connector_name, "MIPI")) {
				    g_dev.connector_type = DRM_MODE_CONNECTOR_DSI;
				} else {
				   display_help();
				   return DRM_FAIL;
				}
                break;
            }
            case 'b':
            {
                if (optarg != NULL) {
				    memcpy(iqfile_path, optarg, strlen(optarg));
				} else {
				    memcpy(iqfile_path, defaultIqPath, strlen(defaultIqPath));
				}
				printf("iqfile_path %s \n",iqfile_path);
                break;
            }
		    case 'q':
            {
                g_enable_iqserver = TRUE;
                break;
            }
            case 'r':
            {
                if (optarg != NULL) {
                    g_rotate = (MI_SYS_Rotate_e)atoi(optarg);
					if (g_rotate >= E_MI_SYS_ROTATE_NUM) {
					    g_rotate = E_MI_SYS_ROTATE_NONE;
					    printf("rotate 0：NONE    1：90    2：180    3：270 \n");
					    return DRM_FAIL;
					}
                }
                break;

            }

            case '?':
            {
                if(optopt == 'p')
                {
                    printf("Missing sensor pad id please -p 'sensor pad' \n");
                }
                return DRM_FAIL;
                //break;
            }
            case 'h':
            default:
            {
                display_help();
                return DRM_FAIL;
            }
        }
    }
	printf("g_dev.connector_type %d\n",g_dev.connector_type);
    if (g_dev.connector_type == 0) {
	    display_help();
	    return DRM_FAIL;
	}
    sstar_drm_open(&g_dev);

    g_dev.use_atomic = 1;

    int_buf_obj();
    ret = drmSetClientCap(g_dev.fd, DRM_CLIENT_CAP_ATOMIC, 1);
    if(ret != 0)
    {
        printf("drmSetClientCap DRM_CLIENT_CAP_ATOMIC ret=%d  \n",ret);
    }

	ExecFunc(sstar_drm_init(&g_dev), DRM_SUCCESS);
    sstar_mode_set(&g_dev);

    //g_dev.video_info.v_out_width = ALIGN_UP(MIN(g_dev.video_info.v_src_width,g_dev.width), ALIGN_NUM);
    //g_dev.video_info.v_out_height = ALIGN_UP(MIN(g_dev.video_info.v_src_height,g_dev.height), ALIGN_NUM);
    g_dev.video_info.v_out_width = ALIGN_UP(g_dev.width, ALIGN_NUM);
    g_dev.video_info.v_out_height = ALIGN_UP(g_dev.height, ALIGN_NUM);

    get_nv12_stride_and_size(g_dev.video_info.v_out_width, g_dev.video_info.v_out_height,
                                        &g_dev.video_info.v_out_stride, &g_dev.video_info.v_out_size);
    //printf("_g_video_width=%d _g_video_height=%d out_size=%d test\n",g_dev.video_info.v_out_width,g_dev.video_info.v_out_height,g_dev.video_info.v_out_size);

	creat_dmabuf_queue();

    Camera_PipelineInit(g_dev.sensorIdx);
    ExecFunc(creat_outport_dmabufallocator(&_g_chn_port_info), DRM_SUCCESS);
	    printf("creat_outport_dmabufallocator(%d-%d-%d-%d) \n", _g_chn_port_info.eModId,
           _g_chn_port_info.u32DevId, _g_chn_port_info.u32ChnId,
           _g_chn_port_info.u32PortId);

	IPU_DetectionInit();
	ExecFunc(Start_Rtsp(), DRM_SUCCESS);
    pthread_create(&tid_algo_thread, NULL, ST_AlgoDemoProc, NULL);

    pthread_create(&tid_enqueue_buf_thread, NULL, enqueue_buffer_loop, (void*)&g_dev);
    pthread_create(&tid_drm_buf_thread, NULL, drm_buffer_loop, (void*)&g_dev);

	getchar();

    bExit = 1;

	if(tid_algo_thread)
    {
		pthread_join(tid_algo_thread, NULL);
	}
    if(tid_enqueue_buf_thread)
    {
        pthread_join(tid_enqueue_buf_thread, NULL);
    }

    if(tid_drm_buf_thread)
    {
        pthread_join(tid_drm_buf_thread, NULL);
    }
	usleep(1000*1000);
	destory_outport_dmabufallocator( &_g_chn_port_info);
    Camera_PipelineDeinit(g_dev.sensorIdx);
    destory_dmabuf_queue();

    //sstar_drm_deinit(&g_dev);
	drmModeFreeConnector(g_dev._g_conn);
	drmModeFreeResources(g_dev._g_res);
    free_resources(g_dev.resources);
	return 0;
}

