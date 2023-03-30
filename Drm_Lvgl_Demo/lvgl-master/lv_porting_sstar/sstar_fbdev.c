/*
 * sstar_fbdev.c
 */
//#ifndef USE_DRM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include "lvgl.h"

#include "sstar_fbdev.h"
#if 0
static int fd;
static struct fb_fix_screeninfo finfo;
static struct fb_var_screeninfo vinfo;
static char *framebuffer;

void sstar_fbdev_flush(const lv_color_t *color_p)
{
    uint32_t yoffset = (char*)color_p == framebuffer ? 0 : vinfo.yres;
    if (yoffset == vinfo.yoffset) {
        return;
    }
    vinfo.yoffset = yoffset;
    if (-1 == ioctl(fd, FBIOPAN_DISPLAY, &vinfo)) {
        printf("BUG_ON: FBIOPAN_DISPLAY.\n");
        exit(-1);
    }
}

unsigned int sstar_fbdev_get_xres()
{
    return vinfo.xres;
}

unsigned int sstar_fbdev_get_yres()
{
    return vinfo.yres;
}

unsigned int sstar_fbdev_get_bpp()
{
    return vinfo.bits_per_pixel;
}

unsigned long sstar_fbdev_va2pa(void *ptr)
{
    return finfo.smem_start + ((char*)ptr - (char*)framebuffer);
}

void *sstar_fbdev_get_buffer(int buf_i)
{
    if (vinfo.yres_virtual >= vinfo.yres * buf_i) {
        return framebuffer + vinfo.yres * finfo.line_length * ( buf_i - 1 );
    }
    return NULL;
}

int sstar_fbdev_init()
{
    const char *fb_dev_name = "/dev/fb0";

    // Open fb_dev
    fd = open(fb_dev_name, O_RDWR);
    if (fd == -1) {
        printf("ERR %s -> [%d]", __FILE__, __LINE__);
        return -1;
    }

    // Get finfo and vinfo
    if (-1 == ioctl(fd, FBIOGET_FSCREENINFO, &finfo)) {
        printf("ERR %s -> [%d]", __FILE__, __LINE__);
        close(fd);
        return -1;
    }
    if (-1 == ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("ERR %s -> [%d]", __FILE__, __LINE__);
        close(fd);
        return -1;
    }

    printf(">>>>>> %dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    framebuffer =
        mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (NULL == framebuffer) {
        printf("ERR %s -> [%d]", __FILE__, __LINE__);
        close(fd);
        return -1;
    }

    printf("%p <-> %lx\n", framebuffer, finfo.smem_start);

    return 0;
}

void sstar_fbdev_deinit()
{
    munmap(framebuffer, finfo.smem_len);
    framebuffer = NULL;
    close(fd);
}
#else
unsigned long sstar_fbdev_va2pa(void *ptr)
{
    return 0;
}

unsigned int sstar_fbdev_get_xres()
{
    return 0;
}

unsigned int sstar_fbdev_get_yres()
{
    return 0;
}
void *sstar_fbdev_get_buffer(int buf_i)
{
    return NULL;
}
void sstar_fbdev_flush(const lv_color_t *color_p)
{
	return;
}
#endif
