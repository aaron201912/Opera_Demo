//#include <unistd.h>
#include <getopt.h>
#include "common/common.h"
#include "sstar_drm.h"
#include "sstar_osd.h"
#include "sstar_vdec.h"

#define BUF_NUM 2
buffer_object_t _g_buf_obj[BUF_NUM];

static int _g_show_video;
static int _g_show_gop = 0;
static int _g_show_yuv = 0;

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
        drmModeSetPlane(buf->fd, buf->plane_id, buf->crtc_id, buf->fb_id_0, 0, 0, 0, buf->width/2, buf->height/2,
                                                                0,0,(buf->width/2)<<16,(buf->height/2)<<16);   
    }
    else if(buf->format == DRM_FORMAT_NV12)
    {
        test_fill_nv12((void *)buf);
        drmModeSetPlane(buf->fd, buf->plane_id, buf->crtc_id, buf->fb_id_0, 0, buf->width/2, buf->height/2, buf->width/2, buf->height/2,
                                                                0,0,buf->width<<16,buf->height<<16);
    }
}

void display_help(void)
{
    printf("************************* Video usage *************************\n");
    printf("-g : show gop(UI) with ARGB888,default is 0\n");
    printf("-y : show mops(UI) with NV12,default is 0\n");
    printf("--vpath : Video file path\n");
    printf("-c : select panel type\n");
    printf("eg:./Drm_player -g 1 -y 1 -c ttl --vpath 720P50.h264 \n");

    return;
}

int parse_args(int argc, char **argv)
{
    int option_index=0;
    MI_S32 s32Opt = 0;
    char connector_name[20];
	memset(&connector_name, 0, 20);
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
                   memcpy(_g_buf_obj[0].video_file, optarg, strlen(optarg));
                   _g_show_video = 1;
                   _g_buf_obj[0].vdec_flag = 1;
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
                    _g_buf_obj[0].connector_type = DRM_MODE_CONNECTOR_DPI;
				} else if (!strcmp(connector_name, "mipi") || !strcmp(connector_name, "Mipi") || !strcmp(connector_name, "MIPI")) {
				    _g_buf_obj[0].connector_type = DRM_MODE_CONNECTOR_DSI;
				} else {
				   display_help();
				   return -1;
				}
                
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

int _g_drm_flag = 0;
int _g_dev_fd = 0;
void  int_buf_obj(int i)
{
    if(i == 0)
    {
        if(_g_show_gop)
            _g_buf_obj[i].format = DRM_FORMAT_ARGB8888;
        if(_g_show_video)
            _g_buf_obj[i].vdec_info.format = DRM_FORMAT_NV12;
    }
    else if(i == 1)
    {
        _g_buf_obj[i].connector_type = _g_buf_obj[0].connector_type;
        if(_g_show_yuv && _g_buf_obj[0].used)
            _g_buf_obj[i].format = DRM_FORMAT_NV12;

    }
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
    _g_buf_obj[i].vdec_info.plane_type = MOPG;

    sstar_drm_getattr(&_g_buf_obj[i]);
    if(_g_buf_obj[i].connector_type == DRM_MODE_CONNECTOR_DPI){
        _g_buf_obj[i].vdec_info.v_src_width = ALIGN_BACK(_g_buf_obj[i].width, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_src_height = ALIGN_BACK(_g_buf_obj[i].height, ALIGN_NUM);
    }
    else if(_g_buf_obj[i].connector_type == DRM_MODE_CONNECTOR_DSI)
    {
        _g_buf_obj[i].vdec_info.v_src_width = ALIGN_BACK(720, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_src_height = ALIGN_BACK(1280/2, ALIGN_NUM);
    } 
    _g_buf_obj[i].vdec_info.v_src_stride = _g_buf_obj[i].vdec_info.v_src_width;
    _g_buf_obj[i].vdec_info.v_src_size = (_g_buf_obj[i].vdec_info.v_src_height * _g_buf_obj[i].vdec_info.v_src_stride * 3)/2;

    _g_buf_obj[i].vdec_info.v_out_x = 0;
    _g_buf_obj[i].vdec_info.v_out_y = 0;
    if(_g_buf_obj[i].connector_type == DRM_MODE_CONNECTOR_DPI)
    {
        _g_buf_obj[i].vdec_info.v_out_width = ALIGN_BACK(_g_buf_obj[i].width, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_out_height = ALIGN_BACK(_g_buf_obj[i].height, ALIGN_NUM);
    }
    else if(_g_buf_obj[i].connector_type == DRM_MODE_CONNECTOR_DSI)
    {
        _g_buf_obj[i].vdec_info.v_out_width = ALIGN_BACK(720, ALIGN_NUM);
        _g_buf_obj[i].vdec_info.v_out_height = ALIGN_BACK(1280/2, ALIGN_NUM);
    }
    _g_buf_obj[i].vdec_info.v_out_stride = _g_buf_obj[i].vdec_info.v_out_width;
    _g_buf_obj[i].vdec_info.v_out_size = (_g_buf_obj[i].vdec_info.v_out_height * _g_buf_obj[i].vdec_info.v_out_stride * 3)/2;
    _g_buf_obj[i].vdec_info.v_bframe = 0;
    sem_init(&_g_buf_obj[i].sem_avail, 0, MAX_NUM_OF_DMABUFF);
    _g_buf_obj[i].used = 1;
	printf("buf->width=%d buf->height=%d v_src_width=%d v_src_height=%d v_out_width=%d v_out_height=%d\n",_g_buf_obj[i].width,
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

int main(int argc, char **argv)
{
    int i;
    pthread_t tid_drm_buf_thread[BUF_NUM];
    pthread_t tid_enqueue_buf_thread[BUF_NUM];
	/************************************************
    Step1:  create pipeline
    *************************************************/
    if(parse_args(argc, argv) != 0)
    {
        return 0;
    }
    getchar();
    MI_SYS_Init(0);
    for(i=0; i < BUF_NUM; i++)
    {
        _g_buf_obj[i].bExit = 0;
		_g_buf_obj[i].bExit_second = 0;
		//_g_buf_obj[i].id = i + 2;
		int_buf_obj(i);
        sstar_drm_init(&_g_buf_obj[i]);
        printf("buf->width=%d buf->height=%d v_src_width=%d v_src_height=%d v_out_width=%d v_out_height=%d\n",_g_buf_obj[i].width,
            _g_buf_obj[i].height,_g_buf_obj[i].vdec_info.v_src_width,_g_buf_obj[i].vdec_info.v_src_height,_g_buf_obj[i].vdec_info.v_out_width,_g_buf_obj[i].vdec_info.v_out_height);
        if(_g_show_gop || _g_show_yuv)
        {
            show_ui(&_g_buf_obj[i]);
        }
        printf("already show ui/nv12 \n");
    }
	
    for(i=0; i < BUF_NUM; i++)
    {
        if(0 == i)
        {
            creat_dmabuf_queue(&_g_buf_obj[i]);
            sstar_vdec_init(&_g_buf_obj[i]);
            _g_buf_obj[i].chn_port_info.eModId = E_MI_MODULE_ID_VDEC;
            _g_buf_obj[i].chn_port_info.u32DevId = 0;
            _g_buf_obj[i].chn_port_info.u32ChnId = 0;
            _g_buf_obj[i].chn_port_info.u32PortId = 0;

            creat_outport_dmabufallocator(&_g_buf_obj[i]);	
            pthread_create(&tid_enqueue_buf_thread[i], NULL, enqueue_buffer_loop, (void*)&_g_buf_obj[i]);
            #ifndef ENABLE_LDC
            pthread_create(&tid_drm_buf_thread[i], NULL, drm_buffer_loop, (void*)&_g_buf_obj[i]);
            #else
            pthread_create(&tid_drm_buf_thread[i], NULL, ldc_thread_loop, (void*)&_g_buf_obj[i]);
            #endif
        }
    }
    getchar();
    for(i = 0;i < BUF_NUM; i++)
    {
       _g_buf_obj[i].bExit = 1;
    }
	/************************************************
    Step3:  destosy pipeline
    *************************************************/
    for(i = 0;i < BUF_NUM; i++)
    {
        if(0 == i)
        {
            if(tid_drm_buf_thread[i])
            {
                pthread_join(tid_drm_buf_thread[i], NULL);
            }
            if(tid_enqueue_buf_thread[i])
            {
                pthread_join(tid_enqueue_buf_thread[i], NULL);
            }
            sstar_vdec_deinit(&_g_buf_obj[i]);
            destory_dmabuf_queue(&_g_buf_obj[i]);
        }
        deint_buf_obj(&_g_buf_obj[i]);
		sstar_drm_deinit(&_g_buf_obj[i]);
    }
	/************************************************
    Step4:  close drm device
    *************************************************/
    if(_g_drm_flag == 1)
    	sstar_drm_close(_g_dev_fd);
    MI_SYS_Exit(0);
	return 0;
}

