#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

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

typedef struct buffer_object_s {
    int fd;
    uint32_t dmabuf_id;
    uint32_t fb_id;

    uint32_t format;
	uint32_t conn_id;
	uint32_t crtc_id;
	uint32_t plane_id;
	uint32_t width;
	uint32_t height;
	uint32_t handles[4];
    int out_fence;
    drmModeRes *drm_res;
    drmModeConnector *drm_conn;
    drmModePlaneRes *drm_plane_res;

    uint32_t blob_id;
    drm_property_ids_t prop_ids;
}buffer_object_t;

#if defined (__cplusplus)
}
#endif
#endif
