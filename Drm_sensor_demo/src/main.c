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
#include "sstar_sensor.h"
#include "sstar_algo.h"
#include <sys/time.h>
#include "mi_venc.h"

#ifdef ENABLE_LDC
#include <gles_ldc.h>
#endif

//#define DUMP_VENC_STREAM
//#define DUMP_LDC_STREAM

//#define ENABLE_LDC


#define BUF_NUM 2
int _g_snr_num = 1;
int _g_dev_fd;
int _g_sensorIdx = 2;

buffer_object_t _g_buf_obj[BUF_NUM];

#ifdef ENABLE_LDC
GLES_LDC_MODULE* _g_Ldcmodule = NULL;
#endif


enum queue_state {
	IDLEQUEUE_STATE = 0,
	ENQUEUE_STATE,
	DEQUEUE_STATE,
};

frame_t* get_last_queue(frame_queue_t *Queue_t, enum queue_state state)
{
    frame_t *pQueue = NULL;
    dma_info_t *dma_info;
    int count_num = 2000;
    while(count_num)
    {
        pQueue = frame_queue_peek_last(Queue_t, 1);
        dma_info = (dma_info_t *)pQueue->frame;
        if(dma_info != NULL && pQueue->buf_size != 0 && dma_info->buf_in_use == state)
        {
            //printf("state=%d count=%d queue_size=%d \n",state,count_num,Queue_t->size);
            return pQueue;
        }
        else
        {
            usleep(1*1000);
            frame_queue_next(Queue_t, pQueue);
            count_num --;
        }
    }
    return NULL;
}

int creat_outport_dmabufallocator(buffer_object_t * buf_obj)
{
    Sensor_Attr_t sensorAttr;
    if(get_sensor_attr(buf_obj->sensorIdx, &sensorAttr) != 0)
    {
        printf("get_sensor_attr fail \n");
        return -1;
    }
    buf_obj->chn_port_info.eModId = E_MI_MODULE_ID_SCL;
    buf_obj->chn_port_info.u32DevId = sensorAttr.u32SclDevId;
    buf_obj->chn_port_info.u32ChnId = sensorAttr.u32SclChnId;
    buf_obj->chn_port_info.u32PortId = sensorAttr.u32SclOutPortId;

    if (MI_SUCCESS != MI_SYS_CreateChnOutputPortDmabufCusAllocator(&buf_obj->chn_port_info))
    {
        printf("MI_SYS_CreateChnOutputPortDmabufCusAllocator failed \n");
        return -1;
    }
    return 0;
}

void destory_outport_dmabufallocator(MI_SYS_ChnPort_t* mi_chn_port_info)
{

     MI_SYS_DestroyChnOutputPortDmabufCusAllocator(mi_chn_port_info);
}

#ifdef ENABLE_LDC

int ldc_init()
{
    MAP_INFO info = {1920, 1080, 1920, 1088, 61, 35, 32};
    int res;
    _g_Ldcmodule = (GLES_LDC_MODULE*)malloc(sizeof(GLES_LDC_MODULE));
    memset(_g_Ldcmodule, 0, sizeof(GLES_LDC_MODULE));

    float* mapx = (float*)malloc(sizeof(float) * info.mapGridX * info.mapGridY);
    float* mapy = (float*)malloc(sizeof(float) * info.mapGridX * info.mapGridY);

    FILE* px = fopen("/customer/mapX.bin", "r+");
    if(px == NULL )
    {
        printf("Open mapX bin failed \n");
        return -1;
    }
    FILE* py = fopen("/customer/mapY.bin", "r+");
    if(py == NULL )
    {
        printf("Open mapY bin failed \n");
        return -1;
    }

    fread(mapx, sizeof(float), info.mapGridX * info.mapGridY, px);
    fread(mapy, sizeof(float), info.mapGridX * info.mapGridY, py);
    fclose(px), fclose(py);

    // the the tid of init and get_render_buffer must be consistent
    res = gles_ldc_init(_g_Ldcmodule, (void*)mapx, (void*)mapy, &info, 3); // the consistent tid to init
    if (res)
    {
        printf("gles_ldc_init error \n");
        return -1;
    }
    gles_ldc_update_map(_g_Ldcmodule, (void*)mapx, (void*)mapy, info.mapGridX, info.mapGridY, info.gridSize);
    free(mapx), free(mapy);
    printf("gles_ldc_init success \n");
    return 0;
}

void  ldc_deinit()
{
    gles_ldc_deinit(_g_Ldcmodule);
}
#endif

void* enqueue_buffer_loop(void* param)
{
    int dma_buf_handle = -1;
    MI_SYS_DmaBufInfo_t mi_dma_buf_info;
    int i;
    dma_info_t *dma_info;
    int ret;
    #if 0
    unsigned long eTime1;
    unsigned long eTime2;
    struct timeval timeEnqueue1;
    #endif
    buffer_object_t * buf_obj = (buffer_object_t *)param;
    memset(&mi_dma_buf_info, 0x0, sizeof(MI_SYS_DmaBufInfo_t));

    i = 0;
    while (!buf_obj->bExit)
    {

        sem_wait(&buf_obj->sem_avail);
        dma_info = &buf_obj->dma_info[i];
        dma_buf_handle = dma_info->dma_buf_fd;

        mi_dma_buf_info.u16Width = buf_obj->vdec_info.v_src_width;
        mi_dma_buf_info.u16Height = buf_obj->vdec_info.v_src_height;
        mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
        mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
        mi_dma_buf_info.u32Stride[0] = buf_obj->vdec_info.v_out_stride;
        mi_dma_buf_info.u32Stride[1] = buf_obj->vdec_info.v_out_stride;
        mi_dma_buf_info.u32DataOffset[0] = 0;
        mi_dma_buf_info.u32DataOffset[1] = buf_obj->vdec_info.v_src_width * buf_obj->vdec_info.v_src_height;
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        #if 0

        gettimeofday(&timeEnqueue1, NULL);
        eTime1 =timeEnqueue1.tv_sec*1000 + timeEnqueue1.tv_usec/1000;
        printf("dma_buf_handle:%d  sstar_scl_enqueueOneBuffer enqueuetime: %d \n", dma_buf_handle, (eTime1 - eTime2) );
        eTime2 = eTime1;
        #endif
        #if 1
        if((dma_info->out_fence != 0) && (dma_info->out_fence != -1))
        {
            ret = sync_wait_sys(dma_info->out_fence, 50);
            if(ret != 0)
            {
                printf("Waring:maybe drop one drm frame, ret=%d out_fence=%d\n", ret, dma_info->out_fence);
            }
            close(dma_info->out_fence);
            dma_info->out_fence = 0;
        }
        #endif
        if(sstar_scl_enqueueOneBuffer(buf_obj->chn_port_info.u32DevId, buf_obj->chn_port_info.u32ChnId, &mi_dma_buf_info) == 0)
        {
            dma_info->buf_in_use = DEQUEUE_STATE;
            frame_queue_putbuf(&buf_obj->_EnQueue_t, (char*)&buf_obj->dma_info[i], sizeof(dma_info_t), NULL);
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
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    }

    printf("Thread enqueue_buffer_loop exit \n");
    buf_obj->bExit_second = 1;
    return (void*)0;
}

#ifdef ENABLE_LDC

void* ldc_thread_loop(void* param)
{
    frame_t *pQueue = NULL;
    int dma_buf_handle = -1;
    dma_info_t *dma_info;
    MI_SYS_DmaBufInfo_t mi_dma_buf_info;
    int timeout = 5000;
    int dequeue_try_cnt = 0;
    FILE* p = NULL;
    void *pVaddr = NULL;

    GLES_DMABUF_INFO texInfo;
    GLES_DMABUF_INFO* renderInfo = NULL;
    buffer_object_t * buf_obj = (buffer_object_t *)param;
    memset(&mi_dma_buf_info, 0x0, sizeof(MI_SYS_DmaBufInfo_t));

    MI_SYS_GetFd(&buf_obj->chn_port_info, &buf_obj->_g_mi_sys_fd);

    if(ldc_init() != 0)
    {
        printf("ldc_init error \n");
        return NULL;
    }
    int datalen = (1920*1088)/ 2 * 3;

#ifdef DUMP_LDC_STREAM
    p = fopen("/tmp/test_ldc.yuv", "wb");
#endif
    while (!buf_obj->bExit_second)
    {
        pQueue = get_last_queue(&buf_obj->_EnQueue_t, DEQUEUE_STATE);
        if( pQueue !=NULL && pQueue->frame != NULL )
        {
            dma_info = (dma_info_t *)pQueue->frame;
        }
        else
        {
            printf("get_last_queue ENQUEUE_STATE fail,sensorIdx=%d \n",buf_obj->sensorIdx);
            continue;
        }
        dma_buf_handle = dma_info->dma_buf_fd;
        mi_dma_buf_info.u16Width = buf_obj->vdec_info.v_src_width;
        mi_dma_buf_info.u16Height = buf_obj->vdec_info.v_src_height;
        mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
        mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
        mi_dma_buf_info.u32Stride[0] = buf_obj->vdec_info.v_src_stride;
        mi_dma_buf_info.u32Stride[1] = buf_obj->vdec_info.v_src_stride;
        mi_dma_buf_info.u32DataOffset[0] = 0;
        mi_dma_buf_info.u32DataOffset[1] = buf_obj->vdec_info.v_src_width * buf_obj->vdec_info.v_src_height;
        texInfo.fds[0] = dma_buf_handle;
        texInfo.fds[1] = dma_buf_handle;
        texInfo.dataOffset[0] = mi_dma_buf_info.u32DataOffset[0];
        texInfo.dataOffset[1] = mi_dma_buf_info.u32DataOffset[1];
        texInfo.width = mi_dma_buf_info.u16Width;
        texInfo.height = mi_dma_buf_info.u16Height;
        texInfo.stride[0] = mi_dma_buf_info.u32Stride[0];
        texInfo.stride[1] = mi_dma_buf_info.u32Stride[1];

        sync_wait_sys(buf_obj->_g_mi_sys_fd, timeout);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        if (dma_buf_handle > 0)
        {
            while(0 != MI_SYS_ChnOutputPortDequeueDmabuf(&buf_obj->chn_port_info, &mi_dma_buf_info) && dequeue_try_cnt < 10)
            {
                dequeue_try_cnt++;
                sync_wait_sys(buf_obj->_g_mi_sys_fd, 10);
            }

            if((mi_dma_buf_info.u32Status != MI_SYS_DMABUF_STATUS_INVALID) && (mi_dma_buf_info.u32Status != MI_SYS_DMABUF_STATUS_DROP))
            {
                gles_ldc_input_texture_buffer(_g_Ldcmodule, &texInfo);

                renderInfo = gles_ldc_get_render_buffer(_g_Ldcmodule); // the consistent tid to wait render
                if (renderInfo != NULL)
                {
                    if(p)
                    {
                        pVaddr = mmap(NULL, datalen, PROT_WRITE|PROT_READ, MAP_SHARED, renderInfo->fds[0], 0);
                        if(!pVaddr)
                        {
                            printf("failed, mmap render dma-buf return fail\n");
                        }
                        //printf("test_ldc u16Width=%d u16Height=%d\n",mi_dma_buf_info.u16Width, mi_dma_buf_info.u16Height);
                        if(pVaddr)
                        {
                            fwrite(pVaddr, sizeof(char), datalen, p);
                        }
                        munmap(pVaddr, datalen);
                    }
                    gles_ldc_put_render_buffer(_g_Ldcmodule, renderInfo);
                }

            }
            else
            {
                printf("Error frame,u32Status=0x%x  0x%x 0x%x\n",mi_dma_buf_info.u32Status,MI_SYS_DMABUF_STATUS_DONE,MI_SYS_DMABUF_STATUS_DROP);
            }

            dma_info->buf_in_use = IDLEQUEUE_STATE;
            frame_queue_next(&buf_obj->_EnQueue_t, pQueue);
            timeout = 100;
            sem_post(&buf_obj->sem_avail);
        }
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    }
    if(p)
    {
        fclose(p);
        p = NULL;
    }
    ldc_deinit();

    printf("Thread ldc_thread_loop exit \n");
    return (void*)0;
}
#endif

void* drm_buffer_loop(void* param)
{
    frame_t *pQueue = NULL;
    int dma_buf_handle = -1;
    dma_info_t *dma_info;
    MI_SYS_DmaBufInfo_t mi_dma_buf_info;
    int timeout = 5000;
    int dequeue_try_cnt = 0;
    #if 0
    unsigned long eTime1;
    unsigned long eTime2;
    unsigned long eTime3;
    struct timeval timeEnqueue1;
    struct timeval timeEnqueue2;
    struct timeval timeEnqueue3;
    #endif
    buffer_object_t * buf_obj = (buffer_object_t *)param;
    memset(&mi_dma_buf_info, 0x0, sizeof(MI_SYS_DmaBufInfo_t));

    MI_SYS_GetFd(&buf_obj->chn_port_info, &buf_obj->_g_mi_sys_fd);
    //usleep(500*1000);
    while (!buf_obj->bExit_second)
    {
        pQueue = get_last_queue(&buf_obj->_EnQueue_t, DEQUEUE_STATE);
        if( pQueue !=NULL && pQueue->frame != NULL )
        {
            dma_info = (dma_info_t *)pQueue->frame;
        }
        else
        {
            printf("get_last_queue ENQUEUE_STATE fail,sensorIdx=%d \n",buf_obj->sensorIdx);
            continue;
        }
        dma_buf_handle = dma_info->dma_buf_fd;
        mi_dma_buf_info.u16Width = buf_obj->vdec_info.v_src_width;
        mi_dma_buf_info.u16Height = buf_obj->vdec_info.v_src_height;
        mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
        mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
        mi_dma_buf_info.u32Stride[0] = buf_obj->vdec_info.v_src_stride;
        mi_dma_buf_info.u32Stride[1] = buf_obj->vdec_info.v_src_stride;
        mi_dma_buf_info.u32DataOffset[0] = 0;
        mi_dma_buf_info.u32DataOffset[1] = buf_obj->vdec_info.v_src_width * buf_obj->vdec_info.v_src_height;

        #if 0
        gettimeofday(&timeEnqueue1, NULL);
        eTime1 =timeEnqueue1.tv_sec*1000 + timeEnqueue1.tv_usec/1000;
        #endif
        sync_wait_sys(buf_obj->_g_mi_sys_fd, timeout);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        if (dma_buf_handle > 0)
        {
            while(0 != MI_SYS_ChnOutputPortDequeueDmabuf(&buf_obj->chn_port_info, &mi_dma_buf_info) && dequeue_try_cnt < 10)
            {
                dequeue_try_cnt++;
                sync_wait_sys(buf_obj->_g_mi_sys_fd, 10);
            }

            #if 0
                gettimeofday(&timeEnqueue2, NULL);
                eTime2 = timeEnqueue2.tv_sec*1000 + timeEnqueue2.tv_usec/1000;
            #endif

            if((mi_dma_buf_info.u32Status != MI_SYS_DMABUF_STATUS_INVALID) && (mi_dma_buf_info.u32Status != MI_SYS_DMABUF_STATUS_DROP))
            {
                if(0 != sstar_drm_update(buf_obj, dma_buf_handle))
                {
                    printf("Waring: drm update frame buffer failed \n");
                }
            }
            else
            {
                printf("Error frame,u32Status=0x%x  0x%x 0x%x\n",mi_dma_buf_info.u32Status,MI_SYS_DMABUF_STATUS_DONE,MI_SYS_DMABUF_STATUS_DROP);
            }

            #if 0

            gettimeofday(&timeEnqueue3, NULL);
            eTime3 = timeEnqueue3.tv_sec*1000 + timeEnqueue3.tv_usec/1000;
            if((eTime3 - eTime2) > 16 )
            {
                printf("buf_handle:%d  sstar_drm_update_time: %d,Dequeuet_time=%d \n", dma_buf_handle, (eTime3 - eTime2),(eTime2 - eTime1));
            }
            #endif

            dma_info->buf_in_use = IDLEQUEUE_STATE;
            frame_queue_next(&buf_obj->_EnQueue_t, pQueue);
            timeout = 100;
            sem_post(&buf_obj->sem_avail);
        }
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    }

    //MI_SYS_CloseFd(buf_obj->_g_mi_sys_fd);
    printf("Thread drm_buffer_loop exit \n");
    return (void*)0;
}


void display_help(void)
{
    printf("************************* usage *************************\n");
    printf("-s : [0/2]select sensor pad\n");
    printf("-i : chose iqbin path for the fisrt pipeline\n");
    printf("-I : chose iqbin path for the Second pipeline\n");
    printf("-n : [0:1]chose multi sensor\n");
    printf("-m : chose detect model path for the fisrt pipeline\n");
    printf("-a : enable face detect funtion for the fisrt pipeline\n");
    printf("-r : set rotate mode,default value is 0. [0,1,2,3] = [0,90,180,270]\n");
    printf("eg:./Drm_sensor -s 0 -i /customer/iq.bin -r 2\n");
    return;
}

int parse_args(int argc, char **argv)
{
    int option_index=0;
    MI_S32 s32Opt = 0;

    struct option long_options[] = {
            {"help", no_argument, NULL, 'h'},
            {0, 0, 0, 0}
    };

    while ((s32Opt = getopt_long(argc, argv, "X:Y:W:H:Y:R:n:m:a:s:i:I:r:h:g:v:y:",long_options, &option_index))!= -1 )
    {
        switch(s32Opt)
        {
            case 'i':
            {
                if(strlen(optarg) != 0)
                {
                   _g_buf_obj[0].iq_file = (char*)malloc(128);
                   if(_g_buf_obj[0].iq_file != NULL)
                    {
                        strcpy(_g_buf_obj[0].iq_file, optarg);
                    }

                }
                break;
            }
            case 'I':
            {
                if(strlen(optarg) != 0)
                {
                   _g_buf_obj[1].iq_file = (char*)malloc(128);
                   if(_g_buf_obj[1].iq_file != NULL)
                    {
                        strcpy(_g_buf_obj[1].iq_file, optarg);
                    }

                }
                break;
            }
            case 'm':
            {
                if(strlen(optarg) != 0)
                {
                   _g_buf_obj[0].model_path = (char*)malloc(128);
                   if(_g_buf_obj[0].model_path != NULL)
                    {
                        strcpy(_g_buf_obj[0].model_path, optarg);
                    }

                }
                break;
            }

            case 's'://ARGB8888
            {
                //_g_buf_obj[0].sensorIdx = atoi(optarg);
                _g_sensorIdx = atoi(optarg);
                break;
            }
            case 'n'://ARGB8888
            {
                _g_snr_num = atoi(optarg);
                break;
            }
            case 'a'://face detect
            {
                _g_buf_obj[0].face_detect = atoi(optarg);
                break;
            }

            case 'r':
            {
                _g_buf_obj[0].rotate = atoi(optarg);
                break;
            }
            case 'v':
            {
                _g_buf_obj[0].venc_flag = atoi(optarg);
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


void creat_dma_buf(buffer_object_t *buf_obj)
{
    int i;
    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        sstar_creat_dma_obj(&buf_obj->dma_info[i], buf_obj->vdec_info.v_out_size);
        if(buf_obj->dma_info[i].dma_buf_fd > 0)
        {
            buf_obj->dma_info[i].buf_in_use = IDLEQUEUE_STATE;
            buf_obj->dma_info[i].out_fence = 0;
            printf("int_buf_obj dma_buf_fd=%d \n",buf_obj->dma_info[i].dma_buf_fd);
        }
    }
}
void destory_dma_buf(buffer_object_t *buf_obj)
{
    int i;
    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        sstar_release_dma_obj(buf_obj->fd, &buf_obj->dma_info[i]);
    }
}

void  int_buf_obj(int i)
{
    _g_buf_obj[i].fd = _g_dev_fd;

    _g_buf_obj[i].format = DRM_FORMAT_ARGB8888;
    _g_buf_obj[i].vdec_info.format = DRM_FORMAT_NV12;
    _g_buf_obj[i].bExit = 0;
    _g_buf_obj[i].bExit_second = 0;
    sstar_drm_getattr(&_g_buf_obj[i]);

    if(_g_buf_obj[i].venc_flag)
    {
        _g_buf_obj[i].width = 1920;
        _g_buf_obj[i].height = 1080;
    }

#ifdef ENABLE_LDC
    _g_buf_obj[i].width = 1920;
    _g_buf_obj[i].height = 1080;
#endif
    if(i == 0)
    {
        if(_g_snr_num == 1)
        {
            _g_buf_obj[i].sensorIdx = _g_sensorIdx;
        }
        _g_buf_obj[i].vdec_info.plane_type = MOPG;

        _g_buf_obj[i].vdec_info.v_src_width = ALIGN_BACK(_g_buf_obj[i].width, ALIGN_NUM);//720;
        _g_buf_obj[i].vdec_info.v_src_height = ALIGN_BACK(_g_buf_obj[i].height, ALIGN_NUM);//1280;
        _g_buf_obj[i].vdec_info.v_src_stride = _g_buf_obj[i].vdec_info.v_src_width;
        _g_buf_obj[i].vdec_info.v_src_size = (_g_buf_obj[i].vdec_info.v_src_height * _g_buf_obj[i].vdec_info.v_src_stride * 3)/2;


        _g_buf_obj[i].vdec_info.v_out_x = 0;
        _g_buf_obj[i].vdec_info.v_out_y = 0;
        _g_buf_obj[i].vdec_info.v_out_width = ALIGN_BACK(_g_buf_obj[i].width, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_out_height = ALIGN_BACK(_g_buf_obj[i].height, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_out_stride = _g_buf_obj[i].vdec_info.v_out_width;
        _g_buf_obj[i].vdec_info.v_out_size = (_g_buf_obj[i].vdec_info.v_out_height * _g_buf_obj[i].vdec_info.v_out_stride * 3)/2;
    }
    else
    {
        //_g_buf_obj[i].sensorIdx = 2;
        _g_buf_obj[i].sensorIdx = _g_sensorIdx;
        _g_buf_obj[i].face_detect = 0;
        _g_buf_obj[i].vdec_info.plane_type = MOPS;

        _g_buf_obj[i].vdec_info.v_src_width = ALIGN_BACK(_g_buf_obj[i].width/2, ALIGN_NUM);//720;
        _g_buf_obj[i].vdec_info.v_src_height = ALIGN_BACK(_g_buf_obj[i].height/2, ALIGN_NUM);//1280;
        _g_buf_obj[i].vdec_info.v_src_stride = _g_buf_obj[i].vdec_info.v_src_width;
        _g_buf_obj[i].vdec_info.v_src_size = (_g_buf_obj[i].vdec_info.v_src_height * _g_buf_obj[i].vdec_info.v_src_stride * 3)/2;


        _g_buf_obj[i].vdec_info.v_out_x = ALIGN_BACK(_g_buf_obj[i].width/2, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_out_y = 0;
        _g_buf_obj[i].vdec_info.v_out_width = ALIGN_BACK(_g_buf_obj[i].width/2, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_out_height = ALIGN_BACK(_g_buf_obj[i].height/2, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_out_stride = _g_buf_obj[i].vdec_info.v_out_width;
        _g_buf_obj[i].vdec_info.v_out_size = (_g_buf_obj[i].vdec_info.v_out_height * _g_buf_obj[i].vdec_info.v_out_stride * 3)/2;

    }
    _g_buf_obj[i].vdec_info.v_bframe = 0;
    _g_buf_obj[i].model_path = NULL;
    sem_init(&_g_buf_obj[i].sem_avail, 0, MAX_NUM_OF_DMABUFF);

}

int  deint_buf_obj(buffer_object_t *buf_obj)
{
    if(buf_obj->iq_file != NULL)
    {
        free(buf_obj->iq_file);
        buf_obj->iq_file = NULL;
    }
    if(buf_obj->model_path != NULL)
    {
        free(buf_obj->model_path);
        buf_obj->model_path = NULL;
    }

    return 0;
}
void creat_dmabuf_queue(buffer_object_t *buf_obj)
{
    int i;

    frame_queue_init(&buf_obj->_EnQueue_t, MAX_NUM_OF_DMABUFF, 0);

    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        sstar_creat_dma_obj(&buf_obj->dma_info[i], buf_obj->vdec_info.v_src_size);
        if(buf_obj->dma_info[i].dma_buf_fd > 0)
        {
            buf_obj->dma_info[i].buf_in_use = IDLEQUEUE_STATE;
            //printf("int_buf_obj dma_buf_fd=%d \n",buf_obj->dma_info[i].dma_buf_fd);
        }
    }
}

void destory_dmabuf_queue(buffer_object_t *buf_obj)
{
    int i;
    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        sstar_release_dma_obj(buf_obj->fd, &buf_obj->dma_info[i]);
    }
    frame_queue_destory(&buf_obj->_EnQueue_t);
}

void get_output_size(int snr_index, int *out_width, int *out_height)
{
    for(int i=0; i<BUF_NUM; i++)
    {
        if(_g_buf_obj[i].sensorIdx == snr_index)
        {
            *out_width = _g_buf_obj[i].vdec_info.v_out_width > _g_buf_obj[i].vdec_info.v_src_width ? _g_buf_obj[i].vdec_info.v_src_width: _g_buf_obj[i].vdec_info.v_out_width;
            *out_height = _g_buf_obj[i].vdec_info.v_out_height > _g_buf_obj[i].vdec_info.v_src_height ? _g_buf_obj[i].vdec_info.v_src_height: _g_buf_obj[i].vdec_info.v_out_height;
            printf("%s_%d i=%d v_out_width=%d v_out_height=%d\n",__FUNCTION__,__LINE__, i, *out_width, *out_height);
            return ;
        }
    }
}

void *vencGetFrame(void* param)
{
    MI_S32 vencFd = -1;
    MI_S32 s32Ret = MI_SUCCESS;
    fd_set read_fds;
    struct timeval TimeoutVal;
    MI_VENC_ChnStat_t stStat;
    MI_VENC_Stream_t stStream;

    FILE *venc_out_fd = NULL;

    buffer_object_t * buf_obj = (buffer_object_t *)param;
#ifdef DUMP_VENC_STREAM
    venc_out_fd = fopen("./venc.h264", "wb");
    if (!venc_out_fd)
    {
        printf("fopen %s falied!\n", venc_out_fd);
        fclose(venc_out_fd);
        venc_out_fd = NULL;
    }
#endif

    vencFd = MI_VENC_GetFd(0, 0);
    if (vencFd <= 0){
        printf("vencGetFrame get fd err\n");
        return NULL;
    }

    printf("vencGetFrame start\n");
    while(!buf_obj->bExit){
        FD_ZERO(&read_fds);
        FD_SET(vencFd, &read_fds);
        TimeoutVal.tv_sec = 2;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(vencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret <= 0){
            printf("vencGetFrame select failed\n");
            continue;
        }
        else
        {
            if (FD_ISSET(vencFd, &read_fds)){
                memset(&stStat, 0, sizeof(MI_VENC_ChnStat_t));
                s32Ret = MI_VENC_Query(0, 0, &stStat);//查询编码通道状态
                if (MI_SUCCESS != s32Ret || stStat.u32CurPacks == 0){
                    printf("vencGetFrame MI_VENC_Query err\n");
                    usleep(10 * 1000); // sleep 10 ms
                    continue;
                }

                //获取编码后的码流，写入文件venc_out.es中
                memset(&stStream, 0, sizeof(MI_VENC_Stream_t));
                stStream.pstPack = (MI_VENC_Pack_t *)malloc(sizeof(MI_VENC_Pack_t) * stStat.u32CurPacks);
                if(NULL == stStream.pstPack){
                    printf("vencGetFrame NULL == stStream.pstPac\n");
                    return NULL;
                }

                //printf("vencGetFrame MI_VENC_GetStream\n");
                stStream.u32PackCount = stStat.u32CurPacks;
                s32Ret = MI_VENC_GetStream(0, 0, &stStream, -1);//获得编码码流
                if (MI_SUCCESS == s32Ret)
                {

                    if(venc_out_fd && stStream.pstPack[0].pu8Addr && stStream.pstPack[0].u32Len > 0)
                    {
                        if(venc_out_fd)
                        {
                            fwrite(stStream.pstPack[0].pu8Addr, stStream.pstPack[0].u32Len, 1, venc_out_fd);
                        }
                    }
                    //释放码流缓存
                    s32Ret = MI_VENC_ReleaseStream(0, 0, &stStream);
                    if(s32Ret != MI_SUCCESS)
                    {
                        printf("MI_VENC_ReleaseStream fail\n");
                        return NULL;
                    }
                }
            }
        }
    }
    if(venc_out_fd)
    {
        fclose(venc_out_fd);
        venc_out_fd = NULL;
    }
    MI_VENC_GetFd(0, 0);
    printf("MI_VENC_ReleaseStream exit\n");
    return NULL;
}

int main(int argc, char **argv)
{
    int i;
    pthread_t tid_drm_buf_thread[BUF_NUM];
    pthread_t tid_enqueue_buf_thread[BUF_NUM];
    int bFlag = 0;
    if(parse_args(argc, argv) != 0)
    {
        return 0;
    }
    _g_dev_fd = sstar_drm_open();//Only open once
    if(_g_dev_fd < 0)
    {
        printf("sstar_drm_open fail\n");
        return -1;
    }
    MI_SYS_Init(0);

    for(i=0; i< _g_snr_num && i < BUF_NUM; i++)
    {
        int_buf_obj(i);
        if((_g_buf_obj[i].vdec_info.v_out_width > _g_buf_obj[i].vdec_info.v_src_width) || (_g_buf_obj[i].vdec_info.v_out_height > _g_buf_obj[i].vdec_info.v_src_height))
        {
            printf("%s_%d v_out must less than v_src, sensorIdx=%d\n",__FUNCTION__,__LINE__, _g_buf_obj[i].sensorIdx);
            continue;
        }
        if(_g_buf_obj[i].venc_flag == 0)
        {
            sstar_drm_init(&_g_buf_obj[i]);
            init_sensor_attr(&_g_buf_obj[i], _g_snr_num);

            creat_dmabuf_queue(&_g_buf_obj[i]);
            create_snr_pipeline(&_g_buf_obj[i]);
            creat_outport_dmabufallocator(&_g_buf_obj[i]);
            if(_g_buf_obj[i].face_detect != 0)
            {
                sstar_algo_init(&_g_buf_obj[i]);
            }

            pthread_create(&tid_enqueue_buf_thread[i], NULL, enqueue_buffer_loop, (void*)&_g_buf_obj[i]);
            #ifndef ENABLE_LDC
            pthread_create(&tid_drm_buf_thread[i], NULL, drm_buffer_loop, (void*)&_g_buf_obj[i]);
            #else
            pthread_create(&tid_drm_buf_thread[i], NULL, ldc_thread_loop, (void*)&_g_buf_obj[i]);
            #endif
        }
        else
        {
            init_sensor_attr(&_g_buf_obj[i], _g_snr_num);
            create_snr_pipeline(&_g_buf_obj[i]);
            if(_g_buf_obj[i].face_detect != 0)
            {
                sstar_algo_init(&_g_buf_obj[i]);
            }
            pthread_create(&tid_drm_buf_thread[i], NULL, vencGetFrame, (void*)&_g_buf_obj[i]);
        }
        #if 0
        if(_g_buf_obj[i].iq_file != NULL)
        {
            sleep(1);
            MI_ISP_ApiCmdLoadBinFile(0, i, _g_buf_obj[i].iq_file, 0);
            printf("MI_ISP_ApiCmdLoadBinFile Isp:0 Chn:%d path:%s \n", i, _g_buf_obj[i].iq_file);
        }
        #endif
    }

    printf("please input 'q' to exit\n");
    while (!bFlag)
    {
        usleep(100 * 1000);
        if(getchar() == 'q')
        {
            bFlag = 1;
            printf("### Drm_sensor Exit ###\n");
        }
    }
    for(i=0; i< _g_snr_num && i < BUF_NUM; i++)
    {

        _g_buf_obj[i].bExit = 1;

        if(_g_buf_obj[i].venc_flag == 0)
        {
            if(tid_enqueue_buf_thread[i])
            {
                pthread_join(tid_enqueue_buf_thread[i], NULL);
            }
            if(tid_drm_buf_thread[i])
            {
                pthread_join(tid_drm_buf_thread[i], NULL);
            }
            if(_g_buf_obj[i].face_detect)
            {
                sstar_algo_deinit();
            }
            destroy_snr_pipeline(&_g_buf_obj[i]);
            //destory_outport_dmabufallocator( &_g_buf_obj.chn_port_info);
            deint_buf_obj(&_g_buf_obj[i]);
            destory_dmabuf_queue(&_g_buf_obj[i]);
            sstar_drm_deinit(&_g_buf_obj[i]);
        }
        else
        {
            if(_g_buf_obj[i].face_detect)
            {
                sstar_algo_deinit();
            }
            destroy_snr_pipeline(&_g_buf_obj[i]);
            deint_buf_obj(&_g_buf_obj[i]);
        }

    }
    sstar_drm_close(_g_dev_fd);
    MI_SYS_Exit(0);
	return 0;
}

