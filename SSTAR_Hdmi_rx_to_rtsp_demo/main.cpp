#include "common/common.h"
#include "sstar_hdmi_rx.h"
#include "rtsp.h"
#include "sstar_scl.h"
#include "sstar_venc.h"

#define BUF_NUM 1

buffer_object_t _g_buf_obj[BUF_NUM];

void  int_buf_obj(int i)
{
    _g_buf_obj[i].pszStreamName = MAIN_STREAM0;
    _g_buf_obj[i].vencChn = 0;
    _g_buf_obj[i].venc_flag = 1;
    _g_buf_obj[i].hvp_realtime = 1;
    _g_buf_obj[i].width = 1920;
    _g_buf_obj[i].height = 1080;
    
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
        printf("buf->width=%d buf->height=%d v_src_width=%d v_src_height=%d v_out_width=%d v_out_height=%d\n",_g_buf_obj[i].width,
            _g_buf_obj[i].height,_g_buf_obj[i].vdec_info.v_src_width,_g_buf_obj[i].vdec_info.v_src_height,_g_buf_obj[i].vdec_info.v_out_width,_g_buf_obj[i].vdec_info.v_out_height);
        printf("_g_video_width=%d _g_video_height=%d out_size=%d _g_buf_obj[%d]\n",_g_buf_obj[i].vdec_info.v_out_width,_g_buf_obj[i].vdec_info.v_out_height,_g_buf_obj[i].vdec_info.v_out_size,i);
        sstar_init_hdmi_rx(&_g_buf_obj[i]);
    }
    ExecFunc(Start_Rtsp(_g_buf_obj, BUF_NUM), MI_SUCCESS);
    getchar();
    ExecFunc(Stop_Rtsp(), MI_SUCCESS);
    for(i=0; i < BUF_NUM; i++)
    {
        bHdmi_Rx_plug_Exit = 1;
        sstar_deinit_hdmi_rx();
        deint_buf_obj(&_g_buf_obj[i]);
    }
    MI_SYS_Exit(0);
    return 0;
}

