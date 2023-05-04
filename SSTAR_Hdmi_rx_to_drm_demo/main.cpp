#include "common.h"
#include "sstar_drm.h"
#include "sstar_hdmi_rx.h"

#define BUF_NUM 1

buffer_object_t _g_buf_obj[BUF_NUM];
/*****************************************************************************
 HDMI_RX + HVP + Drm case
******************************************************************************/
int _g_drm_flag = 0;
int _g_dev_fd = 0;

void  int_buf_obj(int i)
{
    _g_buf_obj[i].format = DRM_FORMAT_ARGB8888;
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
    _g_buf_obj[i].vdec_info.plane_type = MOPS;
    _g_buf_obj[i].connector_type = DRM_MODE_CONNECTOR_DPI;
    _g_buf_obj[i].hvp_realtime = 1;
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

int main(int argc, char *argv[])
{
    int i;
    MI_SYS_Init(0);
    for(i=0; i < BUF_NUM; i++)
    {
		int_buf_obj(i);
        sstar_drm_init(&_g_buf_obj[i]);
        printf("buf->width=%d buf->height=%d v_src_width=%d v_src_height=%d v_out_width=%d v_out_height=%d\n",_g_buf_obj[i].width,
            _g_buf_obj[i].height,_g_buf_obj[i].vdec_info.v_src_width,_g_buf_obj[i].vdec_info.v_src_height,_g_buf_obj[i].vdec_info.v_out_width,_g_buf_obj[i].vdec_info.v_out_height);
        printf("_g_video_width=%d _g_video_height=%d out_size=%d _g_buf_obj[%d]\n",_g_buf_obj[i].vdec_info.v_out_width,_g_buf_obj[i].vdec_info.v_out_height,_g_buf_obj[i].vdec_info.v_out_size,i);
		creat_dmabuf_queue(&_g_buf_obj[i]);
        sstar_init_hdmi_rx(&_g_buf_obj[i]);
		sstar_drm_update(&_g_buf_obj[i], _g_buf_obj[i].dma_info->dma_buf_fd);
    }
    getchar();
    for(i=0; i < BUF_NUM; i++)
    {
    	bHdmi_Rx_plug_Exit = 1;
        sstar_deinit_hdmi_rx();
		deint_buf_obj(&_g_buf_obj[i]);
		sstar_drm_deinit(&_g_buf_obj[i]);
    }

    if(_g_drm_flag == 1)
    	sstar_drm_close(_g_dev_fd);
    MI_SYS_Exit(0);
	return 0;
}


