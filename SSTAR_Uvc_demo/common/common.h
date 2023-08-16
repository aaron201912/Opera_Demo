#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "st_framequeue.h"
#include "mi_sys.h"
#include "mi_common_datatype.h"
#include "mi_isp.h"
#include "mi_scl.h"
#include "mi_sensor.h"
#include "mi_sensor_datatype.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "mi_vif.h"
#include "mi_venc.h"
#include "mi_venc_datatype.h"
//#include "mi_vdec.h"
//#include "mi_vdec_datatype.h"

#if defined (__cplusplus)
extern "C" {
#endif
/***************************************************************
ALIGN DEFINE
***************************************************************/
#define ALIGN_NUM 16
#define ALIGN_BACK(x, a)            (((x) / (a)) * (a))
#define ALIGN_UP(x, a)            (((x+a-1) / (a)) * (a))
/***************************************************************
FUNC DEFINE
***************************************************************/

#define ASCII_COLOR_RED                          "\033[1;31m"
#define ASCII_COLOR_WHITE                        "\033[1;37m"
#define ASCII_COLOR_YELLOW                       "\033[1;33m"
#define ASCII_COLOR_BLUE                         "\033[1;36m"
#define ASCII_COLOR_GREEN                        "\033[1;32m"
#define ASCII_COLOR_END                          "\033[0m"

#define CheckFuncResult(result)\
    if (result != SUCCESS)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__, __LINE__);\
    }\

#ifndef STCHECKRESULT
#define STCHECKRESULT(_func_) \
		do{ \
			MI_S32 s32Ret = MI_SUCCESS; \
			s32Ret = _func_; \
			if (s32Ret != MI_SUCCESS) \
			{ \
				printf("[%s %d]exec function failed, error:%x\n", __func__, __LINE__, s32Ret); \
				return s32Ret; \
			} \
			else \
			{ \
				printf("[%s %d]exec function pass\n", __func__, __LINE__); \
			} \
		} while(0)
#endif

#ifndef ExecFunc
#define ExecFunc(_func_, _ret_) \
		do{ \
			MI_S32 s32Ret = MI_SUCCESS; \
			s32Ret = _func_; \
			if (s32Ret != _ret_) \
			{ \
				printf("[%s %d]exec function failed, error:%x\n", __func__, __LINE__, s32Ret); \
				return s32Ret; \
			} \
			else \
			{ \
				printf("[%s %d]exec function pass\n", __func__, __LINE__); \
			} \
		} while(0)
#endif

#define PrintInfo(fmt, args...)                                    \
    do {                                                           \
        printf(ASCII_COLOR_BLUE "I/[%s:%d] ", __func__, __LINE__); \
        printf(fmt, ##args);                                       \
        printf(ASCII_COLOR_END);                                   \
    } while (0)

#define ST_DBG(fmt, args...)                                       \
    do {                                                           \
        printf(ASCII_COLOR_GREEN "[DBG][%s:%d] ", __func__, __LINE__);\
        printf(fmt, ##args);                                       \
    } while (0)

#define ST_WARN(fmt, args...)                                       \
    do {                                                           \
        printf(ASCII_COLOR_YELLOW "[WARN][%s:%d] ", __func__, __LINE__);\
        printf(fmt, ##args);                                       \
    } while (0)

#define ST_INFO(fmt, args...)                                       \
    do {                                                           \
        printf("[INFO][%s:%d] ", __func__, __LINE__);\
        printf(fmt, ##args);                                       \
    } while (0)

#define ST_ERR(fmt, args...)                                       \
    do {                                                           \
        printf(ASCII_COLOR_RED "[ERR][%s:%d] ", __func__, __LINE__);\
        printf(fmt, ##args);                                       \
    } while (0)

/***************************************************************
SENSOR PARAM
***************************************************************/
typedef struct {
    MI_VIF_SNRPad_e eSensorPadID;

    MI_VIF_GROUP u32VifGroupID;
    MI_U32 u32VifDev;
    MI_VIF_PORT u32vifOutPortId;
    MI_SYS_BindType_e eVif2IspType;


    MI_ISP_CHANNEL u32IspChnId;
    MI_ISP_DEV u32IspDevId;
    MI_ISP_PORT u32IspOutPortId;
    MI_SYS_Rotate_e   isp_eRot;
    MI_SYS_BindType_e eIsp2SclType;

    MI_SCL_DEV u32SclDevId ;
    MI_SCL_CHANNEL u32SclChnId;
    MI_SCL_PORT u32SclOutPortId ;
    MI_SYS_Rotate_e   scl_eRot;  
    MI_U32 u32HWOutPortMask;

    MI_BOOL bUsed;
    MI_BOOL bPlaneMode;
    MI_U8 u8ResIndex;
    MI_U16 u16Width;
    MI_U16 u16Height;
} Sensor_Attr_t;
/***************************************************************
DMA PARAM
***************************************************************/
#define MAX_NUM_OF_DMABUFF 3
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
/***************************************************************
DRM PARAM
***************************************************************/
struct crtc {
    drmModeCrtc *crtc;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
    drmModeModeInfo *mode;
};

struct encoder {
    drmModeEncoder *encoder;
};

struct connector {
    drmModeConnector *connector;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
    char *name;
};

struct fb {
    drmModeFB *fb;
};

struct plane {
    drmModePlane *plane;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
};

struct resources {
    drmModeRes *res;
    drmModePlaneRes *plane_res;
    struct crtc *crtcs;
    struct encoder *encoders;
    struct connector *connectors;
    struct fb *fbs;
    struct plane *planes;
};

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
    uint32_t realtime_mode;
} drm_property_ids_t;

/***************************************************************
SYSBIND PARAM
***************************************************************/
typedef struct {
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
    MI_SYS_BindType_e eBindType;
    MI_U32 u32BindParam;
} Sys_BindInfo_T;

/***************************************************************
OBJECT_BUFF PARAM
***************************************************************/

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
	int id;
    int used;
    struct resources *resources;
    uint32_t format;
    uint32_t conn_id;
    uint32_t crtc_id;
    uint32_t plane_id;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t handle;
    uint32_t size;
    uint32_t fb_id_0;
	uint32_t fb_id_1;
    uint32_t sensorIdx;
    uint32_t face_sclid;
    uint32_t drm_commited;
	uint32_t connector_type;
	const char* pszStreamName;
	MI_VENC_CHN vencChn;
    int isp_rotate;
    int scl_rotate;
	int Hdr_Used;
    frame_queue_t _EnQueue_t;
    int _g_mi_sys_fd;
    MI_SYS_ChnPort_t chn_port_info;
    MI_SYS_ChnPort_t rgn_chn_port_info;
    int face_detect;
    int venc_flag;
    int vdec_flag;
    int hvp_realtime;
    char *iq_file;
    char *model_path;
    drm_property_ids_t prop_ids;
    drmModeAtomicReq *req;
    drmModeConnectorPtr _g_conn;
    drmModeRes *_g_res;
    drmModePlaneRes *_g_plane_res;
    uint8_t *vaddr_0;
	uint8_t *vaddr_1;
    vdec_info_t vdec_info;
    sem_t sem_avail;
    dma_info_t dma_info[MAX_NUM_OF_DMABUFF];
	int bExit;
	int bExit_second;
    char video_file[128];
}buffer_object_t;
extern buffer_object_t _g_buf_obj[];

MI_S32 bind_port(Sys_BindInfo_T* pstBindInfo);
MI_S32 unbind_port(Sys_BindInfo_T* pstBindInfo);

#if defined (__cplusplus)
}
#endif
#endif

