#ifndef _SSTARDRM_H_
#define _SSTARDRM_H_
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "drm_fourcc.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include <drm.h>
#include "lv_drv_conf.h"
#include "lvgl/lvgl.h"

#define BUF_NUM 2

#ifndef NULL
#define NULL 0
#endif


typedef enum {
    GOP_RES = 0,
    GOP_UI,
    GOP_CURSOR,
    MOPG,
    MOPS,
    PLANE_TYPE_MAX,
}SSTAR_PLANE_TYPE_E;

enum queue_state {
	IDLEQUEUE_STATE = 0,
	ENQUEUE_STATE,
	DEQUEUE_STATE,
};


#if 0

typedef struct drm_sstar_update_plane
{
    uint32_t plane_id;
    uint32_t fbid;
    uint32_t format;
    uint16_t width;
    uint16_t height;
    uint64_t modifier;
    uint64_t phyAddr[3];
    uint32_t pitch[4];
    uint32_t offset[4];
    uint64_t fence;
}  drm_sstar_update_plant_t;

#define DRM_IOC_MAGIC    'd'

#define DRM_SSTAR_UPDATE_PLANE 0x00

#define DRM_IOCTL_SSTAR_UPDATE_PLANE    _IOWR(DRM_IOC_MAGIC, 0, drm_sstar_update_plant_t)
#endif

#define DRM_SSTAR_UPDATE_PLANE 0x00
#define DRM_IOCTL_SSTAR_UPDATE_PLANE \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_UPDATE_PLANE, struct drm_sstar_update_plane)

typedef struct drm_sstar_update_plane {
    uint32_t planeId;
    uint32_t fbId; /* Use drm framebuffer if (fbId > 0) */
    /* Otherwise, use the following parameter */
    uint32_t format;
    uint16_t width;
    uint16_t height;
    uint64_t modifier;
    uint64_t phyAddr[3];
    uint32_t pitches[4]; /* stride */
    uint32_t offsets[4];
    uint64_t fence; /* Out fence ptr */
} drm_sstar_update_plane_t;



int sstar_drm_open();
void sstar_drm_close(int fd);
void sstar_drm_init(buffer_object_t *buf);
void sstar_mode_set(buffer_object_t *buf);
void sstar_drm_deinit(buffer_object_t *buf);
int  sstar_drm_update(buffer_object_t *buf, int dma_buffer_fd) ;
void sstar_drmfb_flush(const lv_color_t *color_p);
unsigned long sstar_drmfb_va2pa(void *ptr);
unsigned int sstar_drmfb_get_xres();
unsigned int sstar_drmfb_get_yres();
void *sstar_drmfb_get_buffer(int buf_i);
void sstar_creat_dma_obj(dma_info_t *dma_info, unsigned int len);
void sstar_release_dma_obj(int drm_fd, dma_info_t *dma_info);
int init_drm_property_ids(buffer_object_t *buf);
int sstar_drm_getattr(buffer_object_t *buf);


#endif

