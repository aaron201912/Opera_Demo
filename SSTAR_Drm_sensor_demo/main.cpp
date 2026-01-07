#include "common/common.h"
#include "sstar_drm.h"
#include "sstar_sensor.h"
#include "st_rgn.h"
#include "sstar_algo.h"
#include <getopt.h>
#ifdef ENABLE_LDC
#include <gles_ldc.h>
#include <libsync.h>
#include <sys/mman.h>
#include "st_framequeue.h"
#endif

#define BUF_NUM 2
int _g_snr_num = 1;
int _g_drm_flag = 0;
int _g_dev_fd = 0;
buffer_object_t _g_buf_obj[BUF_NUM];
#ifdef ENABLE_LDC
GLES_LDC_MODULE* _g_Ldcmodule = NULL;
#define DUMP_LDC_STREAM
#endif
/*****************************************************************************
 Sensor + Drm + Rgn + Algo case
******************************************************************************/

void display_help(void)
{
    printf("************************* usage *************************\n");
    printf("-s : [0/1/2]select sensor pad when chose single sensor\n");
    printf("-c : [mipi/ttl/lvds/hdmi]chose sensor panel type\n");
    printf("-i : chose iqbin path for the fisrt pipeline\n");
    printf("-I : chose iqbin path for the second pipeline\n");
    printf("-n : [1:2]enable multi sensor and select pipeline\n");
    printf("-m : chose detect model path for the fisrt pipeline\n");
    printf("-a : enable face detect funtion for the fisrt pipeline\n");
    printf("-r : set rotate mode,default value is 0. [0,1,2,3] = [0,90,180,270]\n");
    printf("-H : [0/1]enable hdr\n");
    printf("-h : help\n");
    printf("eg:./Drm_sensor -s 0 -i /customer/iq.bin -c ttl\n");
    return;
}

int parse_args(int argc, char **argv)
{
    int option_index=0;
    MI_S32 s32Opt = 0;
    int sensorIdx;
    int venc_flag;
    char connector_name[20] = {0};
    struct option long_options[] = {
            {"iqbin0", required_argument, NULL, 'i'},
            {"iqbin1", required_argument, NULL, 'I'},
            {"model", required_argument, NULL, 'm'},
            {"sensorId", required_argument, NULL, 's'},
            {"sensorNum", required_argument, NULL, 'n'},
            {"faceDet", required_argument, NULL, 'a'},
            {"rotate", required_argument, NULL, 'r'},
            {"connect", required_argument, NULL, 'c'},
            {"HDR", required_argument, NULL, 'H'},
            {"help", no_argument, NULL, 'h'},
            {0, 0, 0, 0}
    };

    while ((s32Opt = getopt_long(argc, argv, "i:I:m:s:n:a:r:c:H:",long_options, &option_index))!= -1 )
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
            case 's'://single sensor chose eSnrPadId
            {
                _g_buf_obj[0].sensorIdx = atoi(optarg);
                break;
            }
            case 'n'://pipeline select
            {
                _g_snr_num = atoi(optarg);
                _g_buf_obj[0].sensorIdx = 0;
                break;
            }
            case 'a'://face detect
            {
                _g_buf_obj[0].face_detect = atoi(optarg);
                break;
            }

            case 'r'://rotate
            {
                _g_buf_obj[0].isp_rotate = atoi(optarg);
                break;
            }
            case 'c':
            {
                if (optarg == NULL) {
                    printf("Missing panel type -c 'ttl or mipi' \n");
                    break;
                }
                memcpy(connector_name, optarg, strlen(optarg));
                printf("connector_name %s %d\n",connector_name,strlen(connector_name));
                if(_g_snr_num < 3)
                {
                    if (!strcmp(connector_name, "ttl") || !strcmp(connector_name, "Ttl") || !strcmp(connector_name, "TTL")) {
                        _g_buf_obj[0].connector_type = DRM_MODE_CONNECTOR_DPI;
                    } else if (!strcmp(connector_name, "mipi") || !strcmp(connector_name, "Mipi") || !strcmp(connector_name, "MIPI")) {
                        _g_buf_obj[0].connector_type = DRM_MODE_CONNECTOR_DSI;
                    } else if (!strcmp(connector_name, "lvds") || !strcmp(connector_name, "Lvds") || !strcmp(connector_name, "LVDS")) {
                        _g_buf_obj[0].connector_type = DRM_MODE_CONNECTOR_LVDS;
                    }else if (!strcmp(connector_name, "hdmi") || !strcmp(connector_name, "Hdmi") || !strcmp(connector_name, "HDMI")) {
                        _g_buf_obj[0].connector_type = DRM_MODE_CONNECTOR_HDMIA;
                    }
                    else {
                        display_help();
                        return DRM_FAIL;
                    }
                }
                break;
            }
            case 'H':
                _g_buf_obj[0].Hdr_Used = atoi(optarg);
                break;
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

void  int_buf_obj(int i)
{
    _g_buf_obj[i].format = 0;
    _g_buf_obj[i].vdec_info.format = DRM_FORMAT_NV12;
    if(_g_drm_flag == 0)
    {
        _g_dev_fd = sstar_drm_open();//Only open once
        if(_g_dev_fd < 0)
        {
            printf("sstar_drm_open fail\n");
            return;
        }
        _g_buf_obj[i].fd = _g_dev_fd;
        _g_drm_flag = 1;
        printf("sstar_drm_open success!\n");
    }
    else
    {
        _g_buf_obj[i].fd = _g_dev_fd;
    }
#ifdef ENABLE_LDC
    _g_buf_obj[i].width = 1920;
    _g_buf_obj[i].height = 1080;
#endif
    if(i == 0){
        _g_buf_obj[i].scl_rotate = 0;
        _g_buf_obj[i].vdec_info.plane_type = MOPG;
        sstar_drm_getattr(&_g_buf_obj[i]);

        _g_buf_obj[i].vdec_info.v_src_width = ALIGN_BACK(_g_buf_obj[i].width, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_src_height = ALIGN_BACK(_g_buf_obj[i].height, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_src_stride = _g_buf_obj[i].vdec_info.v_src_width;
        _g_buf_obj[i].vdec_info.v_src_size = (_g_buf_obj[i].vdec_info.v_src_height * _g_buf_obj[i].vdec_info.v_src_stride * 3)/2;

        _g_buf_obj[i].vdec_info.v_out_x = 0;
        _g_buf_obj[i].vdec_info.v_out_y = 0;
        _g_buf_obj[i].vdec_info.v_out_width = ALIGN_BACK(_g_buf_obj[i].width, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_out_height = ALIGN_BACK(_g_buf_obj[i].height, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_out_stride = _g_buf_obj[i].vdec_info.v_out_width;
        _g_buf_obj[i].vdec_info.v_out_size = (_g_buf_obj[i].vdec_info.v_out_height * _g_buf_obj[i].vdec_info.v_out_stride * 3)/2;

    }
    else{
        #ifdef CHIP_IS_SSU9383
        _g_buf_obj[i].sensorIdx = 2;
        #endif
        #ifdef CHIP_IS_SSD2386
        _g_buf_obj[i].sensorIdx = 1;
        #endif
        _g_buf_obj[i].face_detect = 0;
        _g_buf_obj[i].vdec_info.plane_type = MOPS;
        _g_buf_obj[i].connector_type = _g_buf_obj[0].connector_type;
        sstar_drm_getattr(&_g_buf_obj[i]);
        _g_buf_obj[i].width = _g_buf_obj[i].width/2;
        _g_buf_obj[i].height = _g_buf_obj[i].height/2;
        _g_buf_obj[i].vdec_info.v_src_width = ALIGN_BACK(_g_buf_obj[i].width, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_src_height = ALIGN_BACK(_g_buf_obj[i].height, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_src_stride = _g_buf_obj[i].vdec_info.v_src_width;
        _g_buf_obj[i].vdec_info.v_src_size = (_g_buf_obj[i].vdec_info.v_src_height * _g_buf_obj[i].vdec_info.v_src_stride * 3)/2;

        _g_buf_obj[i].vdec_info.v_out_x = 0;
        _g_buf_obj[i].vdec_info.v_out_y = 0;
        _g_buf_obj[i].vdec_info.v_out_width = ALIGN_BACK(_g_buf_obj[i].width, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_out_height = ALIGN_BACK(_g_buf_obj[i].height, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_out_stride = _g_buf_obj[i].vdec_info.v_out_width;
        _g_buf_obj[i].vdec_info.v_out_size = (_g_buf_obj[i].vdec_info.v_out_height * _g_buf_obj[i].vdec_info.v_out_stride * 3)/2;

    }
    _g_buf_obj[i].vdec_info.v_bframe = 0;
    sem_init(&_g_buf_obj[i].sem_avail, 0, MAX_NUM_OF_DMABUFF);
    printf("buf->fb=%d buf->width=%d buf->height=%d v_src_width=%d v_src_height=%d v_out_width=%d v_out_height=%d\n",_g_buf_obj[i].fd, _g_buf_obj[i].width,
        _g_buf_obj[i].height,_g_buf_obj[i].vdec_info.v_src_width,_g_buf_obj[i].vdec_info.v_src_height,_g_buf_obj[i].vdec_info.v_out_width,_g_buf_obj[i].vdec_info.v_out_height);
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

#ifdef ENABLE_LDC
int sstar_ldc_init()
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

void  sstar_ldc_deinit()
{
    gles_ldc_deinit(_g_Ldcmodule);
}

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
    int ret = 0;
    #if 0
    unsigned long eTime1;
    unsigned long eTime2;
    unsigned long eTime3;
    struct timeval timeEnqueue1;
    struct timeval timeEnqueue2;
    struct timeval timeEnqueue3;
    #endif
    buffer_object_t * buf_obj = (buffer_object_t *)param;
    MI_SYS_GetFd(&buf_obj->chn_port_info, &buf_obj->_g_mi_sys_fd);
    if(sstar_ldc_init() != 0)
    {
        printf("ldc_init error \n");
        return NULL;
    }
    int datalen = (1920*1088)/ 2 * 3;

#ifdef DUMP_LDC_STREAM
    p = fopen("./test_ldc.yuv", "wb");
#endif

    //usleep(500*1000);
    while (!buf_obj->bExit)
    {
        memset(&mi_dma_buf_info, 0x0, sizeof(MI_SYS_DmaBufInfo_t));
        pQueue = get_last_queue(&buf_obj->_EnQueue_t, DEQUEUE_STATE);
        if( pQueue !=NULL && pQueue->frame != NULL )
        {
            dma_info = (dma_info_t *)pQueue->frame;
        }
        else
        {
            printf("drm get_last_queue ENQUEUE_STATE fail,sensorIdx=%d \n",buf_obj->sensorIdx);
            continue;
        }
        dma_buf_handle = dma_info->dma_buf_fd;
        // mi_dma_buf_info.u16Width = buf_obj->vdec_info.v_src_width;
        // mi_dma_buf_info.u16Height = buf_obj->vdec_info.v_src_height;
        // mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        // mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        // mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
        // mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
        // mi_dma_buf_info.u32Stride[0] = buf_obj->vdec_info.v_src_stride;
        // mi_dma_buf_info.u32Stride[1] = buf_obj->vdec_info.v_src_stride;
        // mi_dma_buf_info.u32DataOffset[0] = 0;
        // mi_dma_buf_info.u32DataOffset[1] = buf_obj->vdec_info.v_src_width * buf_obj->vdec_info.v_src_height;
        texInfo.fds[0] = dma_buf_handle;
        texInfo.fds[1] = dma_buf_handle;
        texInfo.dataOffset[0] = 0;
        texInfo.dataOffset[1] = buf_obj->vdec_info.v_src_width * buf_obj->vdec_info.v_src_height;
        texInfo.width = buf_obj->vdec_info.v_src_width;
        texInfo.height = buf_obj->vdec_info.v_src_height;
        texInfo.stride[0] = buf_obj->vdec_info.v_src_stride;
        texInfo.stride[1] = buf_obj->vdec_info.v_src_stride;

        #if 0
        gettimeofday(&timeEnqueue1, NULL);
        eTime1 =timeEnqueue1.tv_sec*1000 + timeEnqueue1.tv_usec/1000;
        #endif
        sync_wait_sys(buf_obj->_g_mi_sys_fd, timeout);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        if (dma_buf_handle > 0)
        {
            while(dequeue_try_cnt < 100)
            {
                ret = MI_SYS_ChnOutputPortDequeueDmabuf(&buf_obj->chn_port_info, &mi_dma_buf_info);
                if(ret == 0)
                    break;
                dequeue_try_cnt++;
                sync_wait_sys(buf_obj->_g_mi_sys_fd, 10);
            }

            #if 0
                gettimeofday(&timeEnqueue2, NULL);
                eTime2 = timeEnqueue2.tv_sec*1000 + timeEnqueue2.tv_usec/1000;
            #endif
            #if 1
            if(ret == 0 && (mi_dma_buf_info.u32Status == MI_SYS_DMABUF_STATUS_DONE))
            {
                // if(0 != sstar_drm_update(buf_obj, dma_buf_handle))
                // {
                //     printf("Waring: drm update frame buffer failed \n");
                //     continue;
                // }
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
            //else
            //{
            //    printf("Error frame,u32Status=0x%x  0x%x 0x%x\n",mi_dma_buf_info.u32Status, MI_SYS_DMABUF_STATUS_DONE, MI_SYS_DMABUF_STATUS_DROP);
            //}
            #endif
            //sstar_algo_fps();
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
            dequeue_try_cnt = 0;
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
    sstar_ldc_deinit();

    //MI_SYS_CloseFd(buf_obj->_g_mi_sys_fd);
    MI_SYS_CloseFd(buf_obj->_g_mi_sys_fd);
    printf("Thread drm_buffer_loop exit \n");
	buf_obj->bExit_second = 1;
    sem_post(&buf_obj->sem_avail);
    return (void*)0;
}
#endif

int main(int argc, char **argv)
{
    int i;
    pthread_t tid_drm_buf_thread[BUF_NUM];
    pthread_t tid_enqueue_buf_thread[BUF_NUM];
    /************************************************
    create pipeline
    *************************************************/
    Sensor_Attr_t SensorAttr;
    if(parse_args(argc, argv) != 0)
    {
        return 0;
    }
    MI_SYS_Init(0);
    for(i=0; i < _g_snr_num && i < BUF_NUM; i++)
    {
        _g_buf_obj[i].bExit = 0;
        _g_buf_obj[i].bExit_second = 0;
        _g_buf_obj[i].id = i;
        int_buf_obj(i);
        sstar_drm_init(&_g_buf_obj[i]);
        creat_dmabuf_queue(&_g_buf_obj[i]);
        init_sensor_attr(&_g_buf_obj[i], _g_snr_num);
        if(get_sensor_attr(_g_buf_obj[i].sensorIdx, &SensorAttr))
        {
            printf("get_sensor_attr fail \n");
            return -1;
        }
        if((_g_buf_obj[i].scl_rotate == E_MI_SYS_ROTATE_90) || (_g_buf_obj[i].scl_rotate == E_MI_SYS_ROTATE_270))
        {
            _g_buf_obj[i].chn_port_info.eModId = E_MI_MODULE_ID_SCL;
            _g_buf_obj[i].chn_port_info.u32DevId = 7;
            _g_buf_obj[i].chn_port_info.u32ChnId = 0;
            _g_buf_obj[i].chn_port_info.u32PortId = 0;
        }else
        {
            _g_buf_obj[i].chn_port_info.eModId = E_MI_MODULE_ID_SCL;
            _g_buf_obj[i].chn_port_info.u32DevId = SensorAttr.u32SclDevId;
            _g_buf_obj[i].chn_port_info.u32ChnId = SensorAttr.u32SclChnId;
            _g_buf_obj[i].chn_port_info.u32PortId = SensorAttr.u32SclOutPortId;
        }
        create_snr_pipeline(&_g_buf_obj[i]);
        if(_g_buf_obj[i].face_detect != 0)
        {
            _g_buf_obj[i].rgn_chn_port_info.eModId = E_MI_MODULE_ID_SCL;
            _g_buf_obj[i].rgn_chn_port_info.u32DevId = SensorAttr.u32SclDevId;
            _g_buf_obj[i].rgn_chn_port_info.u32ChnId = SensorAttr.u32SclChnId;
            _g_buf_obj[i].rgn_chn_port_info.u32PortId = SensorAttr.u32SclOutPortId;
            sstar_init_rgn(&_g_buf_obj[i]);
            sstar_algo_init(&_g_buf_obj[i]);
        }
        creat_outport_dmabufallocator(&_g_buf_obj[i]);
        pthread_create(&tid_enqueue_buf_thread[i], NULL, enqueue_buffer_loop, (void*)&_g_buf_obj[i]);
        #ifndef ENABLE_LDC
        pthread_create(&tid_drm_buf_thread[i], NULL, drm_buffer_loop, (void*)&_g_buf_obj[i]);
        #else
        pthread_create(&tid_drm_buf_thread[i], NULL, ldc_thread_loop, (void*)&_g_buf_obj[i]);
        #endif
    }
    getchar();
    for(i=0; i< _g_snr_num && i < BUF_NUM; i++)
    {
       _g_buf_obj[i].bExit = 1;
    }
    /************************************************
    destosy pipeline
    *************************************************/
    for(i=0; i< _g_snr_num && i < BUF_NUM; i++)
    {
        if(tid_drm_buf_thread[i])
        {
            pthread_join(tid_drm_buf_thread[i], NULL);
        }
        if(tid_enqueue_buf_thread[i])
        {
            pthread_join(tid_enqueue_buf_thread[i], NULL);
        }
        if(_g_buf_obj[i].face_detect)
        {
            sstar_algo_deinit();
            sstar_deinit_rgn(&_g_buf_obj[i]);
        }
        destroy_snr_pipeline(&_g_buf_obj[i]);
        //destory_outport_dmabufallocator( &_g_buf_obj.chn_port_info);
        deint_buf_obj(&_g_buf_obj[i]);
        destory_dmabuf_queue(&_g_buf_obj[i]);
        sstar_drm_deinit(&_g_buf_obj[i]);
    }
    /************************************************
    close drm device
    *************************************************/
    if(_g_drm_flag == 1)
        sstar_drm_close(_g_dev_fd);
    MI_SYS_Exit(0);
    return 0;
}

