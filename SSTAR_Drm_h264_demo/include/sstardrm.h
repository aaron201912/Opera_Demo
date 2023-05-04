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




#ifndef NULL
#define NULL 0
#endif

void sstar_drm_open(buffer_object_t *buf);
void sstar_drm_init(buffer_object_t *buf);
void sstar_mode_set(buffer_object_t *buf);
void sstar_drm_deinit(buffer_object_t *buf);
int  sstar_drm_update(buffer_object_t *buf, int dma_buffer_fd) ;
void sstar_creat_dma_obj(dma_info_t *dma_info, unsigned int len);
void sstar_release_dma_obj(dma_info_t *dma_info);
int init_drm_property_ids(int fd, uint32_t crtc_id, uint32_t plane_id, uint32_t conn_id,drm_property_ids_t* prop_ids);


#endif

