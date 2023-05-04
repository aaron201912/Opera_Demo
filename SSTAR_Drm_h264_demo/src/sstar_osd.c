#include "sstar_osd.h"
#include "common.h"



void _gfx_sink_surface(void* pdate, const char  *name)
{
    FILE *fp = NULL;
    char sinkName[128] = {0};
    buffer_object_t *buf = (buffer_object_t *)pdate;
    unsigned char *pdata = buf->vaddr;
    snprintf(sinkName, sizeof(sinkName), "%s_%dx%d.raw", name, buf->width, buf->height);
    fp = fopen(sinkName, "w+");

    if(fp == NULL) {
        fprintf(stderr, "fp == NULL\n");
    } else {
        const unsigned char *p = pdata;
        long n = buf->height * buf->width * 4;

        do {
            long n0 = fwrite(p, 1, n, fp);
            n = n -n0;
            p = pdata+n0;
        } while(n > 0);

        fclose(fp);
    }

}

#if 1

/*
struct util_yuv_info {
	enum util_yuv_order order;
	unsigned int xsub;
	unsigned int ysub;
	unsigned int chroma_stride;
};
struct util_yuv_info _g_yuv;
*/

struct color_osd_yuv {
    unsigned char y;
    unsigned char u;
    unsigned char v;
};


#define MAKE_OSD_YUV_601_Y(r, g, b) \
	((( 66 * (r) + 129 * (g) +  25 * (b) + 128) >> 8) + 16)
#define MAKE_OSD_YUV_601_U(r, g, b) \
	(((-38 * (r) -  74 * (g) + 112 * (b) + 128) >> 8) + 128)
#define MAKE_OSD_YUV_601_V(r, g, b) \
	(((112 * (r) -  94 * (g) -  18 * (b) + 128) >> 8) + 128)

#define MAKE_OSD_YUV_601(r, g, b) \
	{ .y = MAKE_OSD_YUV_601_Y(r, g, b), \
	  .u = MAKE_OSD_YUV_601_U(r, g, b), \
	  .v = MAKE_OSD_YUV_601_V(r, g, b) }

static void fill_tiles_osd_yuv_planar(char *y_mem, char *u_mem,
				  char *v_mem, unsigned int width,
				  unsigned int height, unsigned int stride)
{
	unsigned int cs = 2;
	unsigned int xsub = 2;
	unsigned int ysub = 2;
	unsigned int x;
	unsigned int y;

	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			div_t d = div(x+y, width);
			uint32_t rgb32 = 0x00130502 * (d.quot >> 6)
				       + 0x000a1120 * (d.rem >> 6);
			struct color_osd_yuv color =
				MAKE_OSD_YUV_601((rgb32 >> 16) & 0xff,
					     (rgb32 >> 8) & 0xff, rgb32 & 0xff);

			y_mem[x] = color.y;
			u_mem[x/xsub*cs] = color.u;
			v_mem[x/xsub*cs] = color.v;
		}

		y_mem += stride;
		if ((y + 1) % ysub == 0) {
			u_mem += stride * cs / xsub;
			v_mem += stride * cs / xsub;
		}
	}
}

#endif

void osd_fill_canvas(void* pdate, int rgba, int width, int height, int xpos, int ypos)
{

    int	 x, y;
    buffer_object_t *buf = (buffer_object_t *)pdate;
    int  canvas_line_pixels = buf->width ;
	int  x_max  = MIN((buf->width - xpos), width);
	int  y_max  = MIN((buf->height - ypos), height);


    pixel_u *dst = (pixel_u *)buf->vaddr + ypos*canvas_line_pixels + xpos;
    printf("%s %d  width %d height %d xp %d yp %d dst=%p src_width=%d src_height=%d \n", __func__, __LINE__, width, height, xpos, ypos, dst,buf->width,buf->height);
    printf("%s %d x_max %d y_max %d canvas_line_pixels %d\n", __func__, __LINE__, x_max, y_max, canvas_line_pixels);

    for (y = 0; y < y_max; y++) {
        for (x = 0; x < x_max; x++) {
            dst[x].rgba = rgba;
        }
	    dst += canvas_line_pixels;
    }
}


void test_fill_nv12(void* pdate)
{
    buffer_object_t *buf = (buffer_object_t *)pdate;
    char* u;
    char* v;
    u = (char*)buf->vaddr + (buf->height * buf->width);
    v = (char*)buf->vaddr + (buf->height * buf->width) + 1;
    fill_tiles_osd_yuv_planar(buf->vaddr, u, v, buf->width, buf->height, buf->width);

}

void test_fill_ARGB(void* pdate, OSD_Rect_t sRect)
{
    unsigned int color = 0;
    OSD_Rect_t _srcRect;
    int vir_height;
    int vir_width;
    vir_width = sRect.u32Width;
    vir_height = sRect.u32Height;
    _srcRect.s32Xpos = sRect.s32Xpos;
    _srcRect.s32Ypos = sRect.s32Ypos;
    _srcRect.u32Width = vir_width/6;
    _srcRect.u32Height = vir_height/3;

    color = 0XFFFF0000;
    osd_fill_canvas(pdate, color,_srcRect.u32Width,_srcRect.u32Height,_srcRect.s32Xpos,_srcRect.s32Ypos);

    _srcRect.s32Xpos = vir_width / 6;
    _srcRect.s32Ypos = vir_height / 3;

    color = 0XFFFF0000;
    osd_fill_canvas(pdate, color,_srcRect.u32Width,_srcRect.u32Height,_srcRect.s32Xpos,_srcRect.s32Ypos);

    _srcRect.s32Xpos = vir_width / 3;
    _srcRect.s32Ypos = vir_height - vir_height / 3;

    color = 0XFFFF0000;
    osd_fill_canvas(pdate, color,_srcRect.u32Width,_srcRect.u32Height,_srcRect.s32Xpos,_srcRect.s32Ypos);

    _srcRect.s32Xpos = vir_width / 2;
    _srcRect.s32Ypos = 0;

    color = 0XFF0000FF;
    osd_fill_canvas(pdate, color,_srcRect.u32Width,_srcRect.u32Height,_srcRect.s32Xpos,_srcRect.s32Ypos);

    _srcRect.s32Xpos = vir_width / 2 + vir_width / 6;
    _srcRect.s32Ypos = vir_height / 3;

    color = 0XFF0000FF;
    osd_fill_canvas(pdate, color,_srcRect.u32Width,_srcRect.u32Height,_srcRect.s32Xpos,_srcRect.s32Ypos);

    _srcRect.s32Xpos = vir_width / 2 + vir_width / 3;
    _srcRect.s32Ypos = vir_height - vir_height / 3;

    color = 0XFF000080;
    osd_fill_canvas(pdate, color,_srcRect.u32Width,_srcRect.u32Height,_srcRect.s32Xpos,_srcRect.s32Ypos);

    if(0)
    {
	    _gfx_sink_surface(pdate, __FUNCTION__);
    }

}

