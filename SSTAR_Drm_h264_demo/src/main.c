#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "drm_fourcc.h"
#include "sstar_osd.h"
#include "common.h"
#include "sstardrm.h"
#include "st_framequeue.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include <libsync.h>
#include <getopt.h>
#include "sstarvdec.h"

#define BUF_NUM 2

buffer_object_t _g_buf_obj[BUF_NUM];
frame_queue_t _EnQueue_t;
frame_queue_t _DeQueue_t;
frame_queue_t _DrmQueue_t;
char _g_video_file[128];

static int _g_show_video;
static int _g_show_gop;
static int _g_show_yuv;

#define MAX_NUM_OF_DMABUFF 3
#define ALIGN_NUM 32

enum queue_state {
	IDLEQUEUE_STATE = 0,
	ENQUEUE_STATE,
	DEQUEUE_STATE,
};

void show_ui(buffer_object_t* buf)
{
    OSD_Rect_t _srcRect;
    if(buf->format == DRM_FORMAT_ARGB8888)
    {
        _srcRect.s32Xpos = 0;
        _srcRect.s32Ypos = 0;
        _srcRect.u32Width = buf->width/2;
        _srcRect.u32Height = buf->height/2;
        test_fill_ARGB((void *)buf, _srcRect);
    }
    else if(buf->format == DRM_FORMAT_NV12)
    {
        test_fill_nv12((void *)buf);
        drmModeSetPlane(buf->fd, buf->plane_id, buf->crtc_id, buf->fb_id, 0, buf->width/2, buf->height/2, buf->width/2, buf->height/2,
                                                                0,0,buf->width<<16,buf->height<<16);
    }
}


static int get_nv12_stride_and_size(int width, int height, int* pixel_stride,
                                     int* size) {

    int luma_stride;

    /* 4:2:0 formats must have buffers with even height and width as the clump size is 2x2 pixels.
     * Width will be even stride aligned anyway so just adjust height here for size calculation. */
    height = ALIGN_BACK(height, ALIGN_NUM);

    luma_stride = ALIGN_BACK(width, ALIGN_NUM);

    if (size != NULL) {
        *size = (height * luma_stride * 3)/2 ;
    }

    if (pixel_stride != NULL) {
        *pixel_stride = luma_stride;
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
        pQueue = frame_queue_peek_last(Queue_t, 5);
        dma_info = (dma_info_t *)pQueue->frame;
        if(dma_info != NULL && pQueue->buf_size != 0 && dma_info->buf_in_use == state)
        {
            //printf("state=%d count=%d queue_size=%d \n",state,count_num,Queue_t->size);
            return pQueue;
        }
        else
        {
            usleep(5*1000);
            frame_queue_next(Queue_t, pQueue);
            count_num --;
        }
    }
    printf("get_queue_last fail state=%d \n",state);
    return NULL;
}

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
    buffer_object_t * buf_obj = (buffer_object_t *)param;

    mi_chn_port_info.eModId = E_MI_MODULE_ID_VDEC;
    mi_chn_port_info.u32DevId = 0;
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
                sstar_creat_dma_obj(&buf_obj->dma_info[i], buf_obj->vdec_info.v_out_size);
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
            mi_dma_buf_info.u16Width = buf_obj->vdec_info.v_out_width;
            mi_dma_buf_info.u16Height = buf_obj->vdec_info.v_out_height;
            mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
            mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
            mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
            mi_dma_buf_info.u32Stride[0] = buf_obj->vdec_info.v_out_stride;
            mi_dma_buf_info.u32Stride[1] = buf_obj->vdec_info.v_out_stride;
            mi_dma_buf_info.u32DataOffset[0] = 0;
            mi_dma_buf_info.u32DataOffset[1] = buf_obj->vdec_info.v_out_width * buf_obj->vdec_info.v_out_height;
            if(sstar_vdec_enqueueOneBuffer(0,0,&mi_dma_buf_info) == 0)
            {
                dma_info->buf_in_use = ENQUEUE_STATE;
                frame_queue_putbuf(&_DeQueue_t, pQueue->frame, sizeof(int), NULL);
            }
            else
            {
                bExit = 1;
                break;
            }
        }
        else
        {
            printf("get_last_queue _EnQueue_t IDLEQUEUE_STATE error \n");
            bExit = 1;
            break;
        }
        frame_queue_next(&_EnQueue_t, pQueue);

        ret = sync_wait(mi_sys_fd, 50);
        if (0 == ret && (dma_buf_handle > 0))
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
                }
                else
                {
                    printf("get_last_queue _DeQueue_t ENQUEUE_STATE error \n");
                    bExit = 1;
                    break;

                }
                frame_queue_next(&_DeQueue_t, pQueue);
            }
            else
            {
                printf("MI_SYS_ChnOutputPortDequeueDmabuf error ret=%d \n", ret);
                bExit = 1;
                break;

            }
        }
        else
        {
            printf("MI_SYS_ChnOutputPortDequeueDmabuf fail dma_buf_handle=%d \n",dma_buf_handle);
        }
     }

    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        sstar_release_dma_obj(&buf_obj->dma_info[i]);
    }

    MI_SYS_CloseFd(mi_sys_fd);
    if (MI_SUCCESS != MI_SYS_DestroyChnOutputPortDmabufCusAllocator(&mi_chn_port_info))
    {
        printf("warning: MI_SYS_DestroyChnOutputPortDmabufCusAllocator fail, device=%d, channel=%d \n",
               mi_chn_port_info.u32DevId, mi_chn_port_info.u32ChnId);
    }
    printf("Thread mi_buffer_loop exit \n");
    return (void*)0;
}



void* drm_buffer_loop(void* param)
{
    frame_t *pQueue = NULL;
    int dma_buf_handle = -1;
    dma_info_t *dma_info;
    buffer_object_t * buf_obj = (buffer_object_t *)param;
    while (!bExit)
    {
        pQueue = get_last_queue(&_DrmQueue_t, DEQUEUE_STATE);
        if( pQueue !=NULL && pQueue->frame != NULL )
        {
            dma_info = (dma_info_t *)pQueue->frame;
            dma_buf_handle = dma_info->dma_buf_fd;
            if(0 != sstar_drm_update(buf_obj, dma_buf_handle))
            {
                printf("drm update frame buffer failed \n");
                frame_queue_next(&_DrmQueue_t, pQueue);
                bExit = 1;
                break;
            }
            dma_info->buf_in_use = IDLEQUEUE_STATE;
            frame_queue_putbuf(&_EnQueue_t, pQueue->frame, pQueue->buf_size, NULL);
        }
        else
        {
            printf("get_last_queue _DrmQueue_t DEQUEUE_STATE error \n");
            bExit = 1;
            break;
        }
        frame_queue_next(&_DrmQueue_t, pQueue);
    }

    printf("Thread drm_buffer_loop exit \n");
    return (void*)0;
}

void display_help(void)
{
    printf("************************* Video usage *************************\n");
    printf("-g : show gop(UI) with ARGB888,default is 0\n");
    printf("-g : show mops(UI) with NV12,default is 0\n");
    printf("--vpath : Video file path\n");
    printf("eg:./Drm_player -g 1 -y 1 --vpath 720P50.h264 \n");

    return;
}

int parse_args(int argc, char **argv)
{
    int option_index=0;
    MI_S32 s32Opt = 0;

    struct option long_options[] = {
            {"vpath", required_argument, NULL, 'V'},
            {"apath", required_argument, NULL, 'a'},
            {"hdmi", no_argument, NULL, 'M'},
            {"help", no_argument, NULL, 'h'},
            {0, 0, 0, 0}
    };

    while ((s32Opt = getopt_long(argc, argv, "X:Y:W:H:Y:R:N:P:s:c:v:h:g:y:",long_options, &option_index))!= -1 )
    {
        switch(s32Opt)
        {
            //video
            case 'V':
            {
                if(strlen(optarg) != 0)
                {
                   strcpy(_g_video_file, optarg);
                   _g_show_video = 1;
                }
                break;
            }
            case 'g'://ARGB8888
            {
                _g_show_gop = atoi(optarg);
                break;
            }
            case 'y'://nv12
            {
                _g_show_yuv = atoi(optarg);
                printf("_g_show_yuv is enable \n");
                break;
            }

            case '?':
            {
                if(optopt == 'V')
                {
                    printf("Missing Video file path, please --vpath 'video_path' \n");
                }
                return -1;
                break;
            }
            case 'h':
            default:
            {
                display_help();
                return -1;
                break;
            }
        }
    }
    return 0;
}


void  int_buf_obj()
{
    if(_g_show_gop  || (!_g_show_gop && !_g_show_yuv && _g_show_video))
    {
        _g_buf_obj[0].used = 1;
        _g_buf_obj[0].format = DRM_FORMAT_ARGB8888;
        if(!_g_show_yuv && _g_show_video)
        {
            _g_buf_obj[0].use_vdec = 1;
            _g_buf_obj[0]. vdec_info.format = DRM_FORMAT_NV12;
        }
    }

    if(_g_show_yuv)
    {
        if(_g_buf_obj[0].used)
        {
            _g_buf_obj[1].fd = _g_buf_obj[0].fd;
            _g_buf_obj[1].used = 1;
            _g_buf_obj[1].format = DRM_FORMAT_NV12;
            if(_g_show_video)
            {
                _g_buf_obj[1].use_vdec = 1;
                _g_buf_obj[1]. vdec_info.format = DRM_FORMAT_NV12;
            }
        }
        else
        {
            _g_buf_obj[0].used = 1;
            _g_buf_obj[0].format = DRM_FORMAT_NV12;
            if(_g_show_video)
            {
                _g_buf_obj[0].use_vdec = 1;
                _g_buf_obj[0]. vdec_info.format = DRM_FORMAT_NV12;
            }
        }
    }

}
int main(int argc, char **argv)
{
    pthread_t tid_drm_buf_thread;
    pthread_t tid_enqueue_buf_thread;
    int i;

    bExit = 0;
    if(parse_args(argc, argv) != 0)
    {
        return 0;
    }

    sstar_drm_open(&_g_buf_obj[0]);//Only open once
    int_buf_obj();

    for(i=0; i < BUF_NUM; i++)
    {
        if(_g_buf_obj[i].used)
        {
            sstar_drm_init(&_g_buf_obj[i]);
            sstar_mode_set(&_g_buf_obj[i]);
            if(_g_show_gop || _g_show_yuv)
            {
                show_ui(&_g_buf_obj[i]);
            }
        }
    }
    printf("already show ui/nv12 \n");
    getchar();

    for(i=0; i < BUF_NUM; i++)
    {
        if(_g_buf_obj[i].use_vdec)
        {
            sstar_parse_video_info(_g_video_file, &_g_buf_obj[i].vdec_info.v_src_width, &_g_buf_obj[i].vdec_info.v_src_height, &_g_buf_obj[i].vdec_info.v_bframe);
            //printf("_g_video_width=%d _g_video_height=%d b_frame=%d _g_video_file=%s\n",_g_buf_obj[0].vdec_info.v_src_width,_g_buf_obj[0].vdec_info.v_src_height, _g_buf_obj[0].vdec_info.v_bframe,_g_video_file);
            if(_g_buf_obj[i].vdec_info.v_src_width <=0 || _g_buf_obj[i].vdec_info.v_src_height <= 0)
            {
                printf("parse video info error \n");
                return 0;
            }


            _g_buf_obj[i].vdec_info.v_out_width = ALIGN_BACK(MIN(_g_buf_obj[i].vdec_info.v_src_width,_g_buf_obj[i].width), ALIGN_NUM);
            _g_buf_obj[i].vdec_info.v_out_height = ALIGN_BACK(MIN(_g_buf_obj[i].vdec_info.v_src_height,_g_buf_obj[i].height), ALIGN_NUM);
            get_nv12_stride_and_size(_g_buf_obj[i].vdec_info.v_out_width, _g_buf_obj[i].vdec_info.v_out_height,
                                                &_g_buf_obj[i].vdec_info.v_out_stride, &_g_buf_obj[i].vdec_info.v_out_size);

            sstar_vdec_init(&_g_buf_obj[i].vdec_info, (void *)_g_video_file);

            frame_queue_init(&_EnQueue_t, MAX_NUM_OF_DMABUFF, 0);
            frame_queue_init(&_DeQueue_t, MAX_NUM_OF_DMABUFF, 0);
            frame_queue_init(&_DrmQueue_t, MAX_NUM_OF_DMABUFF, 0);
        
            pthread_create(&tid_enqueue_buf_thread, NULL, enqueue_buffer_loop, (void*)&_g_buf_obj[i]);
            pthread_create(&tid_drm_buf_thread, NULL, drm_buffer_loop, (void*)&_g_buf_obj[i]);
            printf("already show Video \n");
        }
    }
	getchar();
    for(int i=0; i < BUF_NUM; i++)
    {
        if(_g_buf_obj[i].use_vdec)
        {
            bExit = 1;
            if(tid_enqueue_buf_thread)
            {
                pthread_join(tid_enqueue_buf_thread, NULL);
            }

            if(tid_drm_buf_thread)
            {
                pthread_join(tid_drm_buf_thread, NULL);
            }
            sstar_vdec_deinit();
            frame_queue_destory(&_EnQueue_t);
            frame_queue_destory(&_DeQueue_t);
            frame_queue_destory(&_DrmQueue_t);
        }
    }

    sstar_drm_deinit(&_g_buf_obj[0]);


	return 0;
}

