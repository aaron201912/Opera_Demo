#include "common/common.h"
#include "sstar_sensor.h"
#include "st_rgn.h"
#include "sstar_algo.h"
#include "rtsp.h"
#include <getopt.h>

#define BUF_NUM 2
int _g_snr_num = 1;
buffer_object_t _g_buf_obj[BUF_NUM];
/*****************************************************************************
 Sensor + Drm + Rgn + Algo case
******************************************************************************/

void display_help(void)
{
    printf("************************* usage *************************\n");
    printf("-s : [0/1]select sensor pad when chose single sensor\n");
    printf("-i : chose iqbin path for the fisrt pipeline\n");
    printf("-I : chose iqbin path for the second pipeline\n");
    printf("-n : [1:2]enable multi sensor and select pipeline\n");
    printf("-m : chose detect model path for the fisrt pipeline\n");
    printf("-a : enable face detect funtion for the fisrt pipeline\n");
    printf("-r : set rotate mode,default value is 0. [0,1,2,3] = [0,90,180,270]\n");
    printf("-h : [0/1]enable hdr\n");    
    printf("eg:./Drm_sensor -s 0 -i /customer/iq.bin\n");
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
            {"model", required_argument, NULL, 'm'},
            {"vpath", required_argument, NULL, 'V'},
            {"apath", required_argument, NULL, 'a'},
            {"hdmi", no_argument, NULL, 'M'},
            {"help", no_argument, NULL, 'h'},
            {0, 0, 0, 0}
    };

    while ((s32Opt = getopt_long(argc, argv, "X:Y:W:H:Y:R:n:m:c:a:s:i:I:r:h:g:d:y:v:",long_options, &option_index))!= -1 )
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
            case 'h':
				_g_buf_obj[0].Hdr_Used = atoi(optarg);
				break;
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
    _g_buf_obj[i].venc_flag = 1;
    if(i == 0){
        _g_buf_obj[i].pszStreamName = MAIN_STREAM0;
        _g_buf_obj[i].vencChn = 0;
    	_g_buf_obj[i].width = ALIGN_BACK(1920, ALIGN_NUM);
    	_g_buf_obj[i].height = ALIGN_BACK(1080, ALIGN_NUM);               
    	_g_buf_obj[i].vdec_info.v_src_width = _g_buf_obj[i].width;
    	_g_buf_obj[i].vdec_info.v_src_height = _g_buf_obj[i].height;
    	_g_buf_obj[i].vdec_info.v_src_stride = _g_buf_obj[i].vdec_info.v_src_width;
    	_g_buf_obj[i].vdec_info.v_src_size = (_g_buf_obj[i].vdec_info.v_src_height * _g_buf_obj[i].vdec_info.v_src_stride * 3)/2;
	
    	_g_buf_obj[i].vdec_info.v_out_x = 0;
    	_g_buf_obj[i].vdec_info.v_out_y = 0;
    	_g_buf_obj[i].vdec_info.v_out_width = _g_buf_obj[i].width;
    	_g_buf_obj[i].vdec_info.v_out_height = _g_buf_obj[i].height;
    	_g_buf_obj[i].vdec_info.v_out_stride = _g_buf_obj[i].vdec_info.v_out_width;
    	_g_buf_obj[i].vdec_info.v_out_size = (_g_buf_obj[i].vdec_info.v_out_height * _g_buf_obj[i].vdec_info.v_out_stride * 3)/2;
			
	}
    else
    {
		#ifdef CHIP_IS_SSU9383
        _g_buf_obj[i].sensorIdx = 2; 
		#endif
		#ifdef CHIP_IS_SSD2386
		_g_buf_obj[i].sensorIdx = 1; 
		#endif
        _g_buf_obj[i].pszStreamName = MAIN_STREAM1;
        _g_buf_obj[i].vencChn = 1;
        _g_buf_obj[i].face_detect = 0;
    	_g_buf_obj[i].width = ALIGN_BACK(1920, ALIGN_NUM);
    	_g_buf_obj[i].height = ALIGN_BACK(1080, ALIGN_NUM);               
    	_g_buf_obj[i].vdec_info.v_src_width = _g_buf_obj[i].width;
    	_g_buf_obj[i].vdec_info.v_src_height = _g_buf_obj[i].height;
    	_g_buf_obj[i].vdec_info.v_src_stride = _g_buf_obj[i].vdec_info.v_src_width;
    	_g_buf_obj[i].vdec_info.v_src_size = (_g_buf_obj[i].vdec_info.v_src_height * _g_buf_obj[i].vdec_info.v_src_stride * 3)/2;
	
    	_g_buf_obj[i].vdec_info.v_out_x = 0;
    	_g_buf_obj[i].vdec_info.v_out_y = 0;
    	_g_buf_obj[i].vdec_info.v_out_width = _g_buf_obj[i].width;
    	_g_buf_obj[i].vdec_info.v_out_height = _g_buf_obj[i].height;
    	_g_buf_obj[i].vdec_info.v_out_stride = _g_buf_obj[i].vdec_info.v_out_width;
    	_g_buf_obj[i].vdec_info.v_out_size = (_g_buf_obj[i].vdec_info.v_out_height * _g_buf_obj[i].vdec_info.v_out_stride * 3)/2;
			
	}
    _g_buf_obj[i].vdec_info.v_bframe = 0;
    sem_init(&_g_buf_obj[i].sem_avail, 0, MAX_NUM_OF_DMABUFF);
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
	/************************************************
    Step1:  create pipeline
    *************************************************/
    Sensor_Attr_t SensorAttr;
    if(parse_args(argc, argv) != 0)
    {
        return 0;
    }
    MI_SYS_Init(0);
    for(i=0; i< _g_snr_num && i < BUF_NUM; i++)
    {
        _g_buf_obj[i].bExit = 0;
		_g_buf_obj[i].bExit_second = 0;
		_g_buf_obj[i].id = i;
		int_buf_obj(i);
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
    }
	/************************************************
    Step2:  create rtsp
    *************************************************/
    if(_g_buf_obj[0].venc_flag || _g_buf_obj[1].venc_flag)
        ExecFunc(Start_Rtsp(_g_buf_obj, _g_snr_num), MI_SUCCESS);
    getchar();
    for(i = 0;i < BUF_NUM; i++)
    {
       _g_buf_obj[i].bExit = 1;
    }
    if(_g_buf_obj[0].venc_flag || _g_buf_obj[1].venc_flag)
		ExecFunc(Stop_Rtsp(), MI_SUCCESS);
	/************************************************
    Step3:  destosy pipeline
    *************************************************/
    for(i=0; i< _g_snr_num && i < BUF_NUM; i++)
    {
        if(_g_buf_obj[i].face_detect)
        {
            sstar_algo_deinit();
            sstar_deinit_rgn(&_g_buf_obj[i]);
        }
        destroy_snr_pipeline(&_g_buf_obj[i]);
		deint_buf_obj(&_g_buf_obj[i]);
    }
    MI_SYS_Exit(0);
	return 0;
}

