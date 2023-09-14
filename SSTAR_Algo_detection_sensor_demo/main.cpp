#include "common/common.h"
#include "sstar_drm.h"
#include "sstar_sensor.h"
#include "st_rgn.h"
#include "sstar_algo.h"

#define BUF_NUM 1

buffer_object_t _g_buf_obj[BUF_NUM];

int _g_snr_num = 1;
int _g_drm_flag = 0;
int _g_dev_fd = 0;

void display_help(void)
{
    printf("************************* sensor usage *************************\n");
    printf("-p : select sensor pad\n");
	printf("-c : select panel type\n");
	printf("-b : select iq file\n");    
	printf("-r : rotate 0：NONE    1：90    2：180    3：270\n");
    printf("eg:./Algo_detect_sensor -p 0 -c ttl/mipi -b iqfile -r 1 \n");

    return;
}

int parse_arg(int argc, char **argv)
{
    MI_S32 s32Opt = 0;
    char connector_name[20];
	memset(&connector_name, 0, 20);
    while ((s32Opt = getopt(argc, argv, "h:p:c:b:r:q:")) != -1 )
    {
        switch(s32Opt)
        {
            case 'p':
            {
				if (optarg == NULL) {
				    _g_buf_obj[0].sensorIdx = 0;
				} else {
                    _g_buf_obj[0].sensorIdx = atoi(optarg);
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
                    _g_buf_obj[0].connector_type = DRM_MODE_CONNECTOR_DPI;
				} else if (!strcmp(connector_name, "mipi") || !strcmp(connector_name, "Mipi") || !strcmp(connector_name, "MIPI")) {
				    _g_buf_obj[0].connector_type = DRM_MODE_CONNECTOR_DSI;
				} else if (!strcmp(connector_name, "lvds") || !strcmp(connector_name, "Lvds") || !strcmp(connector_name, "LVDS")) {
				    _g_buf_obj[0].connector_type = DRM_MODE_CONNECTOR_LVDS;
				} else if (!strcmp(connector_name, "hdmi") || !strcmp(connector_name, "hdmi") || !strcmp(connector_name, "hdmi")) {
				    _g_buf_obj[0].connector_type = DRM_MODE_CONNECTOR_HDMIA;
				}
                else {
				   display_help();
				   return DRM_FAIL;
				}
                break;
            }
		    case 'b':
            {
                if(optarg != NULL)
                {
                    _g_buf_obj[0].iq_file = (char*)malloc(128);
                    if(_g_buf_obj[0].iq_file != NULL)
                    {
                        strcpy(_g_buf_obj[0].iq_file, optarg);
                    }
                }
                break;
            }
            case 'r':
            {
                if (optarg != NULL) {
                    _g_buf_obj[0].scl_rotate = (MI_SYS_Rotate_e)atoi(optarg);
					if (_g_buf_obj[0].scl_rotate >= E_MI_SYS_ROTATE_NUM) {
					    _g_buf_obj[0].scl_rotate = E_MI_SYS_ROTATE_NONE;
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
	printf("g_dev.connector_type %d\n",_g_buf_obj[0].connector_type);
    if (_g_buf_obj[0].connector_type == 0) {
	    display_help();
	    return DRM_FAIL;
	}    
    return DRM_SUCCESS;
}

void  int_buf_obj(int i)
{
    _g_buf_obj[i].format = 0;
    _g_buf_obj[i].vdec_info.format = DRM_FORMAT_NV12;
    _g_buf_obj[i].Hdr_Used = 0;
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
    _g_buf_obj[i].face_detect = 1;
    _g_buf_obj[i].vdec_info.plane_type = MOPS;
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

int main(int argc, char **argv)
{
    int i;
    pthread_t tid_drm_buf_thread[BUF_NUM];
    pthread_t tid_enqueue_buf_thread[BUF_NUM];
    if(parse_arg(argc, argv) != 0)
    {
        return 0;
    }
	/************************************************
    create pipeline
    *************************************************/
    Sensor_Attr_t SensorAttr;
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
        if(_g_buf_obj[i].scl_rotate > E_MI_SYS_ROTATE_NONE)
        {
            _g_buf_obj[i].chn_port_info.eModId = E_MI_MODULE_ID_SCL;
            _g_buf_obj[i].chn_port_info.u32DevId = 7;
            _g_buf_obj[i].chn_port_info.u32ChnId = 0;
            _g_buf_obj[i].chn_port_info.u32PortId = 0;
        } else
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

