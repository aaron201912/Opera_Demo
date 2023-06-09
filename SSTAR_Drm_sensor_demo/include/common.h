#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <semaphore.h>
#include "st_framequeue.h"
#include "mi_sys.h"
//#include "sstardrm.h"

#if defined (__cplusplus)
extern "C" {
#endif

#define ALIGN_NUM 16
#define MAX_NUM_OF_DMABUFF 3

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
    uint32_t zpos;

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
    int v_src_stride;
    int v_src_size;

    int v_out_x;
    int v_out_y;
    int v_out_width;
    int v_out_height;
    int v_out_stride;
    int v_out_size;
    int v_bframe;
    uint32_t plane_id;
    uint32_t format;
    int plane_type;
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
    uint32_t sensorIdx;
    uint32_t face_sclid;
    uint32_t drm_commited;
    int rotate ;
    frame_queue_t _EnQueue_t;
    int _g_mi_sys_fd;
    MI_SYS_ChnPort_t chn_port_info;
    int face_detect;
    int venc_flag;
    char *iq_file;
    char *model_path;
    drm_property_ids_t prop_ids;
    uint8_t *vaddr;
    vdec_info_t vdec_info;
    sem_t sem_avail;
    int bExit;
    int bExit_second;
    dma_info_t dma_info[MAX_NUM_OF_DMABUFF];
}buffer_object_t;



#define ALIGN_BACK(x, a)            (((x) / (a)) * (a))
#define ALIGN_UP(x, a)            (((x+a-1) / (a)) * (a))

#define CheckFuncResult(result)\
    if (result != SUCCESS)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__, __LINE__);\
    }\

#if 0
#define ExecFunc(func, _ret_)                                                                    \
    do {                                                                                         \
        MI_S32 s32TmpRet;                                                                           \
        s32TmpRet = func;                                                                        \
        if (s32TmpRet != _ret_) {                                                                \
            printf(" [%d] %s exec function [failed], result:0x%x.\n", __LINE__, #func, \
                   s32TmpRet);                                                                   \
            return -1;                                                                           \
        } else {                                                                                 \
            printf(" [%d] %s exec function [pass].\n", __LINE__, #func);               \
        }                                                                                        \
    } while (0);
#endif

#if defined (__cplusplus)
}
#endif
#endif

