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

#ifdef __cplusplus
extern "C" {
#endif
//common
#define DRM_SUCCESS  (0)
#define DRM_FAIL     (-1)
#ifndef NULL
#define NULL 0
#endif
//dma
enum queue_state {
	IDLEQUEUE_STATE = 0,
	ENQUEUE_STATE,
	DEQUEUE_STATE,
};

//drm
typedef enum {
    GOP_RES = 0,
    GOP_UI,
    GOP_CURSOR,
    MOPG,
    MOPS,
    PLANE_TYPE_MAX,
}SSTAR_PLANE_TYPE_E;

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

//drm funtion
/**************************************************
* Open drm dev
* @return           \b OUT: drm dev file descriptor
**************************************************/
int sstar_drm_open();

/**************************************************
* Close drm dev
* @param            \b IN: drm dev file descriptor
**************************************************/
void sstar_drm_close(int fd);

/**************************************************
* Select drm plane, print drm crtc and plane property and commit crtc property
* @param            \b IN: buffer object
**************************************************/
void sstar_drm_init(buffer_object_t *buf);

/**************************************************
* create drm fb or commit drm crtc property
* @param            \b IN: buffer object
**************************************************/
void sstar_mode_set(buffer_object_t *buf);

/**************************************************
* Release drm resource
* @param            \b IN: buffer object
**************************************************/
void sstar_drm_deinit(buffer_object_t *buf);

/**************************************************
* Get dma buffer, commit drm plane property or update drm display
* @param            \b IN: buffer object
* @param            \b IN: dma file descriptor
* @return           \b OUT: drm dev file descriptor
**************************************************/
int sstar_drm_update(buffer_object_t *buf, int dma_buffer_fd);

/**************************************************
* This interface is not completed
**************************************************/
unsigned long sstar_drmfb_va2pa(void *ptr);

/**************************************************
* Get drm fb width
* @return           \b OUT: drm fb width
**************************************************/
unsigned int sstar_drmfb_get_xres();

/**************************************************
* Get drm fb height
* @return           \b OUT: drm fb height
**************************************************/
unsigned int sstar_drmfb_get_yres();

/**************************************************
* Release drm resource
* @param            \b IN: drm fb buff id
* @return           \b OUT: drm fb buff address
**************************************************/
void *sstar_drmfb_get_buffer(int buf_i);

/**************************************************
* Commit drm crtc property
* @param            \b IN: buffer object
* @return           \b OUT: 0:   success
                           -1:   fail
**************************************************/
int init_drm_property_ids(buffer_object_t *buf, uint32_t plane_id, drm_property_ids_t* prop_ids);

/**************************************************
* Select and get drm crtc、conn_id、width、height value
* @param            \b IN: buffer object
* @return           \b OUT: 0:   success
                           -1:   fail
**************************************************/
int sstar_drm_getattr(buffer_object_t *buf);

/**************************************************
* Sort planes by plane type and Store it in plane_type array
* @param            \b IN: drm dev file descriptor
* @param            \b IN: drm crtc id
**************************************************/
void sort_plane_type(int fd, int crtc_idx);

/**************************************************
* Release drm release
* @param            \b IN: drm resource
**************************************************/
static void free_resources(struct resources *res);

/**************************************************
* Get drm release
* @param            \b IN: buffer object
* @return           \b OUT: drm resource
**************************************************/
static struct resources *get_resources(buffer_object_t *buf_obj);

//dma fution
/**************************************************
* Alloc dma buff and fill dma info
* @param            \b IN: dma info
* @param            \b IN: dma buff len
**************************************************/
void sstar_creat_dma_obj(dma_info_t *dma_info, unsigned int len);

/**************************************************
* Release dma buff and dma info
* @param            \b IN: dma file descriptor 
* @param            \b IN: dma info
**************************************************/
void sstar_release_dma_obj(int drm_fd, dma_info_t *dma_info);

/**************************************************
* Enqueue one dma buffer from MI module
* @param            \b IN: output MI module type、channel and port info
* @param            \b IN: dma buff info
* @return           \b OUT: 0:   success
                           -1:   fail
**************************************************/
int sstar_enqueueOneBuffer(MI_SYS_ChnPort_t *chn_port_info, MI_SYS_DmaBufInfo_t* mi_dma_buf_info);

/**************************************************
* Get last drm buff from queue 
* @param            \b IN: frame queue struct 
* @param            \b IN: queue state
* @return           \b OUT: frame struct
**************************************************/
frame_t* get_last_queue(frame_queue_t *Queue_t, enum queue_state state);

/**************************************************
* Create dma buffer queue 
* @param            \b IN: buffer object 
**************************************************/
void creat_dmabuf_queue(buffer_object_t *buf_obj);

/**************************************************
* Create outputport drm buff allocator 
* @param            \b IN: buffer object
* @return           \b OUT: 0:   success
                           -1:   fail
**************************************************/
int creat_outport_dmabufallocator(buffer_object_t * buf_obj);

/**************************************************
* Enqueue buff thread, put MI module output dma buff in queue
* @param            \b IN: buffer object
**************************************************/
void* enqueue_buffer_loop(void* param);

/**************************************************
* Enqueue buff thread, get dma buff from queue to drm display
* @param            \b IN: buffer object
**************************************************/
void* drm_buffer_loop(void* param);

/**************************************************
* destory dma buffer queue
* @param            \b IN: buffer object 
**************************************************/
void destory_dmabuf_queue(buffer_object_t *buf_obj);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

