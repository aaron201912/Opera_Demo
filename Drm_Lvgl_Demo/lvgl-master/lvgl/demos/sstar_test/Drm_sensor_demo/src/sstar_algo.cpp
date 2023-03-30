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

#include "mi_sys.h"
#include "mi_sys_datatype.h"

#include "mi_rgn.h"
#include "mi_ipu.h"
#include "mi_ipu_datatype.h"
#include "sstar_detection_api.h"
#include "st_rgn.h"
#include "sstar_algo.h"

#define IPU_ALIGN_NUM 32
#define IPU_MAX_NUM_OF_DMABUFF 6

#if 1
#ifndef MIN
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif
#endif


static MI_S32 ST_CreateDevice(MI_IPU_DevAttr_t* stDevAttr, char *pFirmwarePath);
static void ST_DetectCallback(std::vector<Box_t> results);
static void ST_DestoryDevice();

static void * g_detection_manager = NULL;
static MI_RGN_HANDLE g_stRgnHandle = 0;
static MI_RGN_HANDLE g_stRgnHandle1 = 1;

MI_RGN_CanvasInfo_t *_g_pstCanvasInfo;

//static MI_SYS_PixelFormat_e g_eAlgoFormat = E_MI_SYS_PIXEL_FRAME_ARGB8888; //模型输入格式
//static Rect_t g_stAlgoRes = {800, 480}; //模型输入分辨率
//int bExit = 0;
pthread_t tid_algo_thread;

/* 算法配置 */
DetectionInfo_t g_detection_info =
{
    "/config/dla/ipu_lfw.bin",
    "/customer/res/sypfa5.480302_fixed.sim_sgsimg.img",
    0.75, //threshold
    {1024, 600}, //转成显示的分辨率
    ST_CreateDevice,
    ST_DetectCallback,
    ST_DestoryDevice,
};


int  get_isp_fps(int index)
{

#if 1
    FILE *fp;
    char tmp_buf[1024];
    char buf[128];
    memset(buf, 0x00, sizeof(buf));
    memset(tmp_buf, 0x00, sizeof(tmp_buf));
    sprintf(tmp_buf,"cat /proc/mi_modules/mi_vif/mi_vif0  | grep Fps -A %d | tail -n 1 | awk '{print $NF}'",index);
    //fp = popen("cat /proc/mi_modules/mi_isp/mi_isp0  | grep fps -A 1 | tail -n 1 | awk '{print $NF}'","r");
    fp = popen(tmp_buf,"r");
    if(!fp){
        printf("popen error,errno=%d\n",errno);
        return 0;
    }

    int nread = fread(buf, 1, 128, fp);

    if(nread < 0)
    {
        pclose(fp);
        return 0;
    }
    pclose(fp);
    //printf("read ret %d byte, %s,int=%d\n",nread, buf, atoi(buf));
    return atoi(buf);
#else
        int fd[2];
        pipe(fd);
        int pid = fork();
        if(pid)
        {
            char *buf = (char *)malloc(sizeof(char)*15);
            bzero(&buf, sizeof(char));
            int n = read(fd[0], buf, 15);
            strtok(buf, "");
            return buf;
        }
        else //son process
        {
            dup2(fd[1],STDOUT_FILENO);
            close(fd[1]);
            close(fd[0]);
            //system("cat /proc/mi_modules/mi_isp/mi_isp0  | grep fps -A 1 | tail -n 1 | awk -F" " '{print $NF}'");
            system("cat /proc/mi_modules/mi_isp/mi_isp0  | grep fps -A 1 | tail -n 1 | awk '{print $NF}'");

            exit(0);
        }
#endif
}


static MI_S32 ST_CreateDevice(MI_IPU_DevAttr_t* stDevAttr, char *pFirmwarePath)
{
    return MI_IPU_CreateDevice(stDevAttr, NULL, pFirmwarePath, 0);
}

#define RECT_BORDER_WIDTH 8

void ST_DrawText()
{
    int fps = 0;
    char fps_char[128];
    ST_Point_T rgnPoint = {100, 100};
    fps = get_isp_fps(1);
    sprintf(fps_char,"Sensor0_1920x1080_fps:%d",fps);
    ST_OSD_DrawText(g_stRgnHandle, rgnPoint, fps_char, I4_RED, DMF_Font_Size_32x32);

    fps = get_isp_fps(2);
    sprintf(fps_char,"Sensor2_640x480_fps:%d",fps);
    rgnPoint.u32Y = 130;
    ST_OSD_DrawText(g_stRgnHandle, rgnPoint, fps_char, I4_RED, DMF_Font_Size_32x32);

    return;
}

static void ST_DetectCallback(std::vector<Box_t> results)
{
#if 1
    unsigned int i = 0;
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
        ST_OSD_DrawRect(g_stRgnHandle, rgnRect, RECT_BORDER_WIDTH, 1);
        ST_DrawText();
    }
    if(results.size() == 0)
    {
        ST_DrawText();
    }
	ST_OSD_Update(0, g_stRgnHandle);
#endif
    return;
}



static void ST_DestoryDevice()
{
    MI_IPU_DestroyDevice();
    return;
}

static void *ST_AlgoDemoProc(void *param)
{
    MI_S32 s32Ret = 0;
    fd_set read_fds;
    struct timeval tv;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
	MI_SYS_ChnPort_t stSclOutputPort;
	MI_S32 SclFd = -1;
    buffer_object_t * buf_obj = (buffer_object_t *)param;
	memset(&stSclOutputPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSclOutputPort.eModId = E_MI_MODULE_ID_SCL;
    stSclOutputPort.u32DevId = buf_obj->face_sclid;
    stSclOutputPort.u32ChnId = 0;
    stSclOutputPort.u32PortId = 1;
	MI_SYS_SetChnOutputPortDepth(0, &stSclOutputPort , 2, 4);
    if(MI_SYS_GetFd(&stSclOutputPort, &SclFd) < 0)
    {
        printf("MI_SYS GET Scl ch: %d fd err.\n", stSclOutputPort.u32ChnId);
        return NULL;
    }

    while(!buf_obj->bExit)
    {
        FD_ZERO(&read_fds);
        FD_SET(SclFd, &read_fds);
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        s32Ret = select(SclFd + 1, &read_fds, NULL, NULL, &tv);
        if(s32Ret < 0)
        {
            printf("select failed\n");
        }
        else if (0 == s32Ret)
        {
            printf("ST_AlgoDemoProc select timeout\n");
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
                MI_SYS_ChnOutputPortPutBuf(stBufHandle);
            }
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        }
   }

	if(SclFd > 0)
    {
        //MI_SYS_CloseFd(SclFd);
    }

    return NULL;
}

int sstar_algo_init(buffer_object_t * buf_obj)
{
    if(buf_obj->model_path)
    {
        memcpy(g_detection_info.Model, buf_obj->model_path,sizeof(g_detection_info.Model));
    }
    g_detection_info.disp_size.width = buf_obj->width;
    g_detection_info.disp_size.height = buf_obj->height;

    printf("model_path=%s \n",g_detection_info.Model);
    getDetectionManager(&g_detection_manager);
    initDetection(g_detection_manager, &g_detection_info);
    startDetect(g_detection_manager);
    pthread_create(&tid_algo_thread, NULL, ST_AlgoDemoProc, (void*)buf_obj);
    return 0;
}

int sstar_algo_deinit()
{
	if(tid_algo_thread)
    {
		pthread_join(tid_algo_thread, NULL);
	}
    stopDetect(g_detection_manager);
    putDetectionManager(g_detection_manager);
    return 0;
}


