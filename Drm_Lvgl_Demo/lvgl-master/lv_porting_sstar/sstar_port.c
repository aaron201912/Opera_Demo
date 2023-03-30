/*
 * sstar_port.c
 */

#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>

#include "lvgl.h"

#include "evdev.h"
#include "lv_conf.h"
#include "lv_draw_sw.h"

#include "sstar_sys.h"
#include "sstar_memory.h"
#include "sstar_disp.h"
#include "sstar_fbdev.h"
#include "sstar_gfx.h"
#include "sstar_port.h"
#include "sstar_drm.h"
#include "mi_sys.h"

extern buffer_object_t _g_buf_obj[];
extern int _g_dev_fd;


void sstar_draw_ctx_init_cb(struct _lv_disp_drv_t * disp_drv, lv_draw_ctx_t * draw_ctx)
{
    lv_draw_sw_ctx_t * draw_sw_ctx = (lv_draw_sw_ctx_t *) draw_ctx;
    lv_draw_sw_init_ctx(disp_drv, draw_ctx);
    draw_sw_ctx->base_draw.draw_img = sstar_gfx_draw_img_cb;
    draw_sw_ctx->blend = sstar_gfx_blend_cb;
}
void sstar_draw_ctx_deinit_cb(struct _lv_disp_drv_t * disp_drv, lv_draw_ctx_t * draw_ctx)
{
    lv_draw_sw_deinit_ctx(disp_drv, draw_ctx);
}
void sstar_flush_cb(struct _lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    //sstar_gfx_wait();
    if (disp_drv->full_refresh) {
        sstar_drmfb_flush(color_p);
        lv_disp_flush_ready(disp_drv);
        return;
    }
    if (disp_drv->draw_buf->flushing_last) {
        sstar_drmfb_flush(color_p);
    }
    lv_disp_t *disp = _lv_refr_get_disp_refreshing();
    lv_area_t dirty_area = {0};
    int i = 0;
    for (i = 0; i < disp->inv_p; ++i) {
        if (!disp->inv_area_joined[i]) {
            dirty_area = disp->inv_areas[i];
            break;
        }
    }
    lv_color_t *dst_buf, *src_buf;
    if (color_p == disp_drv->draw_buf->buf1) {
        dst_buf = disp_drv->draw_buf->buf2;
        src_buf = disp_drv->draw_buf->buf1;
    } else {
        dst_buf = disp_drv->draw_buf->buf1;
        src_buf = disp_drv->draw_buf->buf2;
    }
    for (int i = 0; i < disp->inv_p; ++i) {
        if (!disp->inv_area_joined[i]) {
            _lv_area_join(&dirty_area, &dirty_area, &disp->inv_areas[i]);
        }
    }
    if (color_p == disp_drv->draw_buf->buf1) {
        sstar_gfx_copy(disp_drv->draw_buf->buf2, color_p, &dirty_area);
    } else {
        sstar_gfx_copy(disp_drv->draw_buf->buf1, color_p, &dirty_area);
    }
    printf("%s-> Calling flush_cb on (%d;%d)(%d;%d) area with %p image pointer, %d\033[0m\n",
            color_p == disp_drv->draw_buf->buf1 ? "\033[0;35m" : "\033[0;36m",
            dirty_area.x1, dirty_area.y1, dirty_area.x2, dirty_area.y2, (void *)color_p, disp_drv->draw_buf->flushing_last);
    sstar_gfx_wait();
    lv_disp_flush_ready(disp_drv);
}

void sstar_wait_cb(struct _lv_disp_drv_t * disp_drv)
{
    sstar_gfx_wait();
}
static void lvgl_disp_drv_init()
{
    static lv_disp_drv_t disp_drv = {0};
    static lv_indev_drv_t input_drv = {0};
    static lv_disp_draw_buf_t disp_buf = {0};

    lv_color_t *buf_1 = NULL;
    lv_color_t *buf_2 = NULL;
    unsigned int pixel_cnt = 0;

    // Lvgl init display buffer
    pixel_cnt = sstar_drmfb_get_xres() * sstar_drmfb_get_yres();
    buf_1 = sstar_drmfb_get_buffer(1);
    buf_2 = sstar_drmfb_get_buffer(2);
	
    printf("--> %p, %p, %d\n", buf_1, buf_2, pixel_cnt);

    lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, pixel_cnt);

    // Lvgl init display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.hor_res = sstar_drmfb_get_xres();
    disp_drv.ver_res = sstar_drmfb_get_yres();
    disp_drv.full_refresh = 1;
    //disp_drv.direct_mode = 1;

    disp_drv.flush_cb = sstar_flush_cb;
    //disp_drv.wait_cb = sstar_wait_cb;
    //disp_drv.draw_ctx_init = sstar_draw_ctx_init_cb;
    //disp_drv.draw_ctx_deinit = sstar_draw_ctx_deinit_cb;
    //disp_drv.draw_ctx_size = sizeof(lv_draw_sw_ctx_t);

    lv_disp_drv_register(&disp_drv);

    // Lvgl init input driver
    lv_indev_drv_init(&input_drv);
    input_drv.type = LV_INDEV_TYPE_POINTER;
    input_drv.read_cb = evdev_read;

    lv_indev_drv_register(&input_drv);
}

static void lvgl_disp_drv_deinit(void)
{
}

void  int_buf_obj(int i)
{
    _g_buf_obj[i].fd = _g_dev_fd;

    _g_buf_obj[i].format = DRM_FORMAT_ARGB8888;
    _g_buf_obj[i].vdec_info.format = DRM_FORMAT_NV12;

    sstar_drm_getattr(&_g_buf_obj[i]);

    if(i == 0)
    {
    	_g_buf_obj[i].sensorIdx = 0;
        _g_buf_obj[i].face_detect = 1;
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
		_g_buf_obj[i].model_path = "/customer/res/sypfa5.480302_fixed.sim_sgsimg.img";
    }
    else
    {
		#ifdef CHIP_IS_SSU9383
        _g_buf_obj[i].sensorIdx = 2; 
		#endif
		#ifdef CHIP_IS_SSD2386
		_g_buf_obj[i].sensorIdx = 1; 
		#endif
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
		_g_buf_obj[i].model_path = NULL;

    }
    _g_buf_obj[i].vdec_info.v_bframe = 0;
    sem_init(&_g_buf_obj[i].sem_avail, 0, MAX_NUM_OF_DMABUFF);
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

int sstar_lv_init(void)
{
	int i;
	_g_dev_fd = sstar_drm_open();
	if(_g_dev_fd < 0)
    {
        printf("sstar_drm_open fail\n");
        return -1;
    }

	for(i=0; i < BUF_NUM; i++)
    {
        int_buf_obj(i);
        if((_g_buf_obj[i].vdec_info.v_out_width > _g_buf_obj[i].vdec_info.v_src_width) || (_g_buf_obj[i].vdec_info.v_out_height > _g_buf_obj[i].vdec_info.v_src_height))
        {
            printf("%s_%d v_out must less than v_src, sensorIdx=%d\n",__FUNCTION__,__LINE__, _g_buf_obj[i].sensorIdx);
            continue;
        }
        sstar_drm_init(&_g_buf_obj[i]);  
    }


    lv_init();	
    evdev_init();
    lvgl_disp_drv_init();
	MI_SYS_Init(0);
	
    return 0;
	
}

void sstar_lv_deinit(void)
{
	int i;
	
	MI_SYS_Exit(0);
    lvgl_disp_drv_deinit();
	sstar_pool_free();
	lv_deinit();
	for(i=0; i < BUF_NUM; i++)
    {
        sstar_drm_init(&_g_buf_obj[i]);  
    }
	sstar_drm_close(_g_dev_fd);
}

