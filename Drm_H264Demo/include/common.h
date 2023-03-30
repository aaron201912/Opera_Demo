#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined (__cplusplus)
extern "C" {
#endif


typedef struct drm_property_ids {
    uint32_t FB_ID;
    uint32_t CRTC_ID;
    uint32_t CRTC_X;
    uint32_t CRTC_Y;
    uint32_t CRTC_W;
    uint32_t CRTC_H;
    uint32_t SRC_X;
    uint32_t SRC_Y;
    uint32_t SRC_W;
    uint32_t SRC_H;
    uint32_t FENCE_ID;
    uint32_t ACTIVE;
    uint32_t MODE_ID;

} drm_property_ids_t;


typedef struct dma_info
{
    int dma_heap_fd;
    int dma_buf_fd;
    int dma_buf_len;
    int buf_in_use;
    uint32_t fb_id;
    uint32_t format;
    int out_fence;
    unsigned int gem_handles[4];
    uint32_t pitches[4];
    uint32_t offsets[4];
    pthread_mutex_t mutex;
    pthread_cond_t  condwait;
    char *p_viraddr;
	int  *cb_func;
}dma_info_t;

typedef struct vdec_info
{
    int v_src_width;
    int v_src_height;
    int v_out_x;
    int v_out_y;
    int v_out_width;
    int v_out_height;
    int v_out_stride;
    int v_out_size;
    int v_bframe;
    uint32_t plane_id;
    uint32_t format;
    int out_fence;
    drm_property_ids_t prop_ids;
}vdec_info_t;



typedef struct buffer_object_s {
    int fd;
    int used;
    uint32_t format;
	uint32_t conn_id;
	uint32_t crtc_id;
	uint32_t plane_id;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t handle;
	uint32_t size;
	uint32_t fb_id;
    drm_property_ids_t prop_ids;
	uint8_t *vaddr;
    int use_vdec;
    vdec_info_t vdec_info;
    dma_info_t dma_info[3];
}buffer_object_t;


#define ALIGN_BACK(x, a)            (((x) / (a)) * (a))
#define ALIGN_UP(x, a)            (((x+a-1) / (a)) * (a))

int bExit;


#if defined (__cplusplus)
#endif
#endif

