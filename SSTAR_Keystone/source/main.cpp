/* SigmaStar trade secret */
/* Copyright (c) [2019~2020] SigmaStar Technology.
All rights reserved.

Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
SigmaStar and be kept in strict confidence
(SigmaStar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of SigmaStar Confidential
Information is unlawful and strictly prohibited. SigmaStar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <map>
#include <sys/ioctl.h>
#include <vector>
#include "drm.h"
#include "xf86drm.h"
#include "xf86drmMode.h"
#include "drm_fourcc.h"
#include <signal.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>

#include <cstring>
#include <gpu_graphic_effect.h>

#include "player.h"
#include "list.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "mi_common_datatype.h"
#include "mi_hdmirx.h"
#include "mi_hdmirx_datatype.h"
#include "mi_hvp.h"
#include "mi_hvp_datatype.h"
#include "mi_scl.h"

#define VEDIO_WIDTH     1920
#define VEDIO_HEIGHT    1080

typedef struct video_info_s {
    seq_info_t seq_info; // current stream sequence info, get seq info by callback function

    int rotate;          // current rotate mode
    int ratio;           // current video display ratio

    int in_width;        // screen width, change along with the rotate mode
    int in_height;       // screen height, change along with the rotate mode
    int dec_width;       // decode output width
    int dec_height;      // decode output height
    int rot_width;       // output width after rotation
    int rot_height;      // output height after rotation
    int out_width;       // display window width
    int out_height;      // display window height
    int pos_x;           // display the starting horizontal axis of the image
    int pos_y;           // display the starting vertical axis of the image
} video_info_t;

typedef struct ui_info_s {
    seq_info_t seq_info; // current stream sequence info, get seq info by callback function

    int rotate;          // current rotate mode
    int ratio;           // current video display ratio

    int in_width;        // screen width, change along with the rotate mode
    int in_height;       // screen height, change along with the rotate mode
    int dec_width;       // decode output width
    int dec_height;      // decode output height
    int rot_width;       // output width after rotation
    int rot_height;      // output height after rotation
    int out_width;       // display window width
    int out_height;      // display window height
    int pos_x;           // display the starting horizontal axis of the image
    int pos_y;           // display the starting vertical axis of the image
} ui_info_t;


typedef struct player_obj_s {
    bool exit;
    bool player_working;
    //int  loop_cnt;
    video_info_t video_info;
    ui_info_t ui_info;
    player_info_t param;
} media_player_t;


static media_player_t _g_MediaPlayer;

#define ALIGN_BACK(x, a)            (((x) / (a)) * (a))
#define ALIGN_UP(x, a)            (((x+a-1) / (a)) * (a))

#define MIN(a,b)                    ((a)>(b)?(b):(a))
#define MAX(a, b)           ((a) < (b) ? (b) : (a))

#define ENCRYPTED_HDCP_LEN 341
#define DECRYPTED_HDCP_LEN 293
#define EDID_LEN 256
#define DMA_HEAP_IOC_MAGIC    'H'
#define DMA_HEAP_IOCTL_ALLOC    _IOWR(DMA_HEAP_IOC_MAGIC,0,dma_heap_allocation_data)

#ifndef STCHECKRESULT
#define STCHECKRESULT(_func_)                                                              \
    do                                                                                     \
    {                                                                                      \
        MI_S32 s32Ret = MI_SUCCESS;                                                        \
        s32Ret        = _func_;                                                            \
        if (s32Ret != MI_SUCCESS)                                                          \
        {                                                                                  \
            printf("[%s %d]exec function failed, error:%x\n", __func__, __LINE__, s32Ret); \
            return s32Ret;                                                                 \
        }                                                                                  \
        else                                                                               \
        {                                                                                  \
            printf("(%s %d)exec function pass\n", __FUNCTION__, __LINE__);                 \
        }                                                                                  \
    } while (0)
#endif

#ifndef SIGNAL_MONITOR_COLOR_FORMAT_HDMI_2_HVP
#define SIGNAL_MONITOR_COLOR_FORMAT_HDMI_2_HVP(__hvp, __hdmirx) \
    do                                                          \
    {                                                           \
        switch (__hdmirx)                                       \
        {                                                       \
            case E_MI_HDMIRX_PIXEL_FORMAT_RGB:                  \
                __hvp = E_MI_HVP_COLOR_FORMAT_RGB444;           \
                break;                                          \
            case E_MI_HDMIRX_PIXEL_FORMAT_YUV444:               \
                __hvp = E_MI_HVP_COLOR_FORMAT_YUV444;           \
                break;                                          \
            case E_MI_HDMIRX_PIXEL_FORMAT_YUV422:               \
                __hvp = E_MI_HVP_COLOR_FORMAT_YUV422;           \
                break;                                          \
            case E_MI_HDMIRX_PIXEL_FORMAT_YUV420:               \
                __hvp = E_MI_HVP_COLOR_FORMAT_YUV420;           \
                break;                                          \
            default:                                            \
                __hvp = E_MI_HVP_COLOR_FORMAT_MAX;              \
                break;                                          \
        }                                                       \
    } while(0)
#endif

#ifndef SIGNAL_MONITOR_COLOR_DEPTH_HDMI_2_HVP
#define SIGNAL_MONITOR_COLOR_DEPTH_HDMI_2_HVP(__hvp, __hdmirx) \
do                                                             \
    {                                                          \
        switch (__hdmirx)                                      \
        {                                                      \
            case E_MI_HDMIRX_PIXEL_BITWIDTH_8BIT:              \
                __hvp = E_MI_HVP_COLOR_DEPTH_8;                \
                break;                                         \
            case E_MI_HDMIRX_PIXEL_BITWIDTH_10BIT:             \
                __hvp = E_MI_HVP_COLOR_DEPTH_10;               \
                break;                                         \
            case E_MI_HDMIRX_PIXEL_BITWIDTH_12BIT:             \
                __hvp = E_MI_HVP_COLOR_DEPTH_12;               \
                break;                                         \
            default:                                           \
                __hvp = E_MI_HVP_COLOR_DEPTH_MAX;              \
                break;                                         \
        }                                                      \
    } while(0)
#endif

#ifndef SIGNAL_MONITOR_PIXEX_REPETITIVE_HDMI_2_HVP
#define SIGNAL_MONITOR_PIXEX_REPETITIVE_HDMI_2_HVP(__hvp, __hdmirx)      \
do                                                                       \
    {                                                                    \
        switch (__hdmirx)                                                \
        {                                                                \
            case E_MI_HDMIRX_OVERSAMPLE_1X:                              \
                __hvp = E_MI_HVP_PIX_REP_TYPE_1X;                        \
                break;                                                   \
            case E_MI_HDMIRX_OVERSAMPLE_2X:                              \
                __hvp = E_MI_HVP_PIX_REP_TYPE_2X;                        \
                break;                                                   \
            case E_MI_HDMIRX_OVERSAMPLE_3X:                              \
                __hvp = E_MI_HVP_PIX_REP_TYPE_3X;                        \
                break;                                                   \
            case E_MI_HDMIRX_OVERSAMPLE_4X:                              \
                __hvp = E_MI_HVP_PIX_REP_TYPE_4X;                        \
                break;                                                   \
            default:                                                     \
                __hvp = E_MI_HVP_PIX_REP_TYPE_MAX;                       \
                break;                                                   \
        }                                                                \
    } while(0)
#endif

enum DrmPlaneId
{
    GOP_UI_ID = 31,
    GOP_CURSOR_ID = 40,
    MOPG_ID0 = 47,
    MOPG_ID1 = 53,
    MOPG_ID2 = 59,
    MOPG_ID3 = 65,
    MOPG_ID4 = 71,
    MOPG_ID5 = 77,
    MOPG_ID6 = 83,
    MOPG_ID7 = 89,
    MOPG_ID8 = 95,
    MOPG_ID9 = 101,
    MOPG_ID10 = 107,
    MOPG_ID11 = 113,
    MOPS_ID = 119,
};

typedef enum
{
    E_STREAM_YUV422 = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
    E_STREAM_YUV422_UYVY = E_MI_SYS_PIXEL_FRAME_YUV422_UYVY,
    E_STREAM_YUV422_YVYU = E_MI_SYS_PIXEL_FRAME_YUV422_YVYU,
    E_STREAM_YUV422_VYUY = E_MI_SYS_PIXEL_FRAME_YUV422_VYUY,
    E_STREAM_ARGB8888 = E_MI_SYS_PIXEL_FRAME_ARGB8888,
    E_STREAM_ABGR8888 = E_MI_SYS_PIXEL_FRAME_ABGR8888,
    E_STREAM_BGRA8888 = E_MI_SYS_PIXEL_FRAME_BGRA8888,
    E_STREAM_YUV420 = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420,
    E_STREAM_RGB_BAYER_BASE = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_BASE,
    E_STREAM_RGB_BAYER_MAX = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_NUM,
    E_STREAM_MAX = E_MI_SYS_PIXEL_FRAME_FORMAT_MAX,
} E_VIDEO_RAW_FORMAT;

struct buffer_object {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t handle;
    uint32_t size;
    uint8_t *vaddr;
    uint32_t fb_id;
};

typedef struct
{
    int fb_id;
    int gem_handle[3];
}drm_buf_t;

typedef struct
{
    unsigned int  width;
    unsigned int  height;
    unsigned int  format;
    int fds[3];
    int stride[3];
    int dataOffset[3];
    void *priavte;
}stDmaBuf_t;

typedef struct{
    MI_U64 len;
    MI_U32 fd;
    MI_U32 fd_flags;
    MI_U64 heap_flags;
}dma_heap_allocation_data;

struct buffer_object buf;

typedef struct drm_property_ids_s {
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


typedef struct DrmModeConfiction_s
{
    int fd;
    int out_fence;
    unsigned int crtc_id;
    unsigned int conn_id;
    unsigned int width;
    unsigned int height;
    unsigned int blob_id;
    uint32_t drm_commited;
    drm_property_ids_t drm_mode_prop_ids[2];//0 for GOP;1 for mop
    drmModeRes *pDrmModeRes;
    drmModeConnectorPtr Connector;
}DrmModeConfiction_t;
static DrmModeConfiction_t g_stDrmCfg;

typedef struct ST_GfxInputBuf_Node
{
    struct list_head    list;
    MI_SYS_DmaBufInfo_t                 stDmaBufInfo;
    std::shared_ptr<GpuGraphicBuffer>   pstdInputBuffer;
}ST_GfxInputBuf_Node;

static int  _g_rotate_mode  = (int)AV_ROTATE_90;//(int)AV_ROTATE_NONE;//


MI_U32 g_u32UiWidth = 0;
MI_U32 g_u32UiHeight = 0;
MI_U16 g_u16DispWidth = 0;
MI_U16 g_u16DispHeight = 0;


MI_U8 g_u8PipelineMode;
char  g_InputFile[128];
char  g_InputUiPath[128];

MI_BOOL bShowUi = 0;

pthread_t hvp_event_thread;
static bool hvp_event_thread_running = false;

pthread_t g_pThreadScl;
MI_BOOL g_bThreadExitScl = TRUE;

pthread_t g_pThreadGfx;
MI_BOOL g_bThreadExitGfx = TRUE;

pthread_t g_pThreadUiDrm;
MI_BOOL g_bThreadExitUiDrm = TRUE;


pthread_t g_player_thread;
MI_BOOL g_bThreadExitPlayer = TRUE;

pthread_t g_video_thread;
MI_BOOL g_bThreadExitVideo = TRUE;


MI_U32 g_u32GpuLT_X, g_u32GpuLT_Y;
MI_U32 g_u32GpuLB_X, g_u32GpuLB_Y;
MI_U32 g_u32GpuRT_X, g_u32GpuRT_Y;
MI_U32 g_u32GpuRB_X, g_u32GpuRB_Y;

MI_BOOL g_bGpuInfoChange_mop = false;
MI_BOOL g_bGpuUifresh = false;

MI_U32 g_u32SclBufferCnt = 0;


std::pair<uint32_t, uint32_t> prev_gop_gem_handle(static_cast<uint32_t>(-1), static_cast<uint32_t>(-1));
std::pair<uint32_t, uint32_t> prev_mop_gem_handle(static_cast<uint32_t>(-1), static_cast<uint32_t>(-1));



std::shared_ptr<GpuGraphicEffect> g_stdGpuGfx = std::make_shared<GpuGraphicEffect>();
Rect g_RectDisplayRegion;

LIST_HEAD(g_HeadListGpuGfxInput);

pthread_mutex_t ListMutexGfx = PTHREAD_MUTEX_INITIALIZER;

MI_U8 u8EnqueueDmabufCnt = 0;

static  MI_U8 EDID_Test[256] =
{
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x4D, 0x27, 0x72, 0x09, 0x01, 0x00, 0x00, 0x00,
    0x07, 0x1E, 0x01, 0x03, 0x80, 0x73, 0x41, 0x78, 0x0A, 0xCF, 0x74, 0xA3, 0x57, 0x4C, 0xB0, 0x23,
    0x09, 0x48, 0x4C, 0x21, 0x08, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C,
    0x45, 0x00, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E, 0x01, 0x1D, 0x00, 0xBC, 0x52, 0xD0, 0x1E, 0x20,
    0xB8, 0x28, 0x55, 0x40, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x53,
    0x69, 0x67, 0x6D, 0x61, 0x73, 0x74, 0x61, 0x72, 0x0A, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFD,
    0x00, 0x3B, 0x46, 0x1F, 0x8C, 0x3C, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x00,
    0x02, 0x03, 0x3E, 0xF0, 0x53, 0x10, 0x1F, 0x14, 0x05, 0x13, 0x04, 0x20, 0x22, 0x3C, 0x3E, 0x12,
    0x16, 0x03, 0x07, 0x11, 0x15, 0x02, 0x06, 0x01, 0x2F, 0x09, 0x7F, 0x05, 0x15, 0x07, 0x50, 0x57,
    0x07, 0x00, 0x3D, 0x07, 0xC0, 0x5F, 0x7E, 0x01, 0x83, 0x01, 0x00, 0x00, 0x6E, 0x03, 0x0C, 0x00,
    0x10, 0x00, 0xB8, 0x3C, 0x2F, 0x00, 0x80, 0x01, 0x02, 0x03, 0x04, 0xE2, 0x00, 0xFB, 0x01, 0x1D,
    0x00, 0x72, 0x51, 0xD0, 0x1E, 0x20, 0x6E, 0x28, 0x55, 0x00, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E,
    0x8C, 0x0A, 0xD0, 0x8A, 0x20, 0xE0, 0x2D, 0x10, 0x10, 0x3E, 0x96, 0x00, 0x13, 0x8E, 0x21, 0x00,
    0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
};

static  MI_U8 HDCP_KEY[289] =
{
    0x00,
    0x0B,0xE4,0xCE,0xED,0x24,0x00,0x00,0x00,
    0x02,0xD6,0xD3,0xBC,0x1E,0x39,0x40,
    0xA9,0x46,0xAF,0x6B,0x46,0xF2,0x80,
    0x07,0x12,0xC0,0x19,0xE1,0xE1,0x5D,
    0xF8,0x02,0x14,0x3C,0xD9,0x8F,0xBB,
    0x3B,0xCD,0x09,0x23,0x59,0x11,0x7B,
    0x11,0x47,0x6F,0x95,0x74,0xDC,0x0A,
    0x29,0xFF,0x5C,0xFF,0xE8,0x56,0x59,
    0x58,0x8E,0x3F,0x41,0xFA,0x06,0xAB,
    0xA5,0xD8,0x37,0xE6,0x3D,0x64,0x7C,
    0xB5,0xBB,0x80,0x28,0x2F,0x2E,0x22,
    0xD5,0x87,0x68,0x90,0xB4,0x23,0xB8,
    0x02,0x2F,0x16,0x6B,0x10,0x2C,0xA9,
    0x15,0x1E,0x47,0x45,0x52,0x38,0xF8,
    0x9A,0x4A,0x5C,0x7C,0x4B,0xF7,0x70,
    0x5C,0x72,0xAC,0xBB,0xAC,0xF4,0xFD,
    0xEB,0x0C,0x2B,0x09,0x81,0xE1,0xBF,
    0x73,0xEF,0x60,0x6B,0x20,0x4C,0x5C,
    0x14,0x80,0x0D,0x0C,0x89,0x86,0x91,
    0x7B,0x06,0x5F,0x83,0xE0,0x2F,0xA3,
    0x30,0x2D,0x11,0x64,0x37,0xC2,0xB3,
    0x3B,0xA5,0x36,0x2D,0x1F,0x05,0x3B,
    0x86,0x04,0x0C,0x53,0xEE,0xD5,0x33,
    0xB5,0x27,0xB1,0x73,0x4C,0x1E,0xBB,
    0x0E,0xF6,0xAA,0x3D,0xE6,0xDB,0x6C,
    0x05,0x07,0x27,0xB9,0x29,0xE4,0x87,
    0xA2,0xEB,0xDB,0xB5,0xAA,0xD4,0x4D,
    0xB6,0x0A,0x33,0x45,0x2F,0x1A,0x58,
    0x81,0xF0,0x2E,0x5A,0x8D,0x1D,0x6E,
    0xA2,0xEA,0x9F,0x8D,0x05,0x11,0xDA,
    0xDB,0xCF,0xCF,0x57,0xCB,0x88,0x17,
    0x4F,0x29,0x8B,0xE3,0x14,0xDA,0x09,
    0xA2,0x37,0x9C,0xDE,0x0A,0xDB,0x2A,
    0xE3,0xD4,0xF9,0xFC,0x79,0x31,0xAD,
    0xFE,0xEF,0xEC,0x4B,0x2E,0xE9,0xFF,
    0x87,0x90,0x14,0xC3,0xD4,0x7D,0x39,
    0x28,0x42,0x92,0x3D,0x4E,0x43,0xB7,
    0x22,0xAE,0x4B,0x36,0x41,0x9F,0x4E,
    0xD7,0x97,0x57,0x59,0x97,0x4A,0xD8,
    0xE3,0x50,0x9A,0x03,0x88,0xEE,0x21,
    0xD8,0x53,0x6A,0x9F,0x31,0xB5,0x88,
};

static int signal_monitor_sync_hdmi_paramter_2_hvp(const MI_HDMIRX_TimingInfo_t *pstTimingInfo)
{
    MI_S32 ret = 0;
    MI_HVP_ChannelParam_t stChnParam;
    MI_HVP_DEV dev = 0;
    MI_HVP_CHN chn = 0;

    memset(&stChnParam, 0, sizeof(MI_HVP_ChannelParam_t));
    ret = MI_HVP_GetChannelParam(dev, chn, &stChnParam);
    if (ret != MI_SUCCESS)
    {
        printf("Get HVP Param error! Dev %d Chn %d\n", dev, chn);
        return -1;
    }
    SIGNAL_MONITOR_PIXEX_REPETITIVE_HDMI_2_HVP(stChnParam.stChnSrcParam.enPixRepType, pstTimingInfo->eOverSample);
    SIGNAL_MONITOR_COLOR_FORMAT_HDMI_2_HVP(stChnParam.stChnSrcParam.enInputColor, pstTimingInfo->ePixelFmt);
    SIGNAL_MONITOR_COLOR_DEPTH_HDMI_2_HVP(stChnParam.stChnSrcParam.enColorDepth, pstTimingInfo->eBitWidth);
    ret = MI_HVP_SetChannelParam(dev, chn, &stChnParam);
    if (ret != MI_SUCCESS)
    {
        printf("Set HVP param error! Dev %d Chn %d\n", dev, chn);
        return -1;
    }
    return 0;
}

void* signal_monitor_hdmi(void * arg)
{
    printf("hvp_event_thread enter\n");
    int select_ret = 0;
    MI_HVP_DEV dev = 0;
    MI_S32 ret = 0;
    MI_S32 s32Fd = -1;
    MI_BOOL bTrigger;
    fd_set read_fds;
    struct timeval tv;
    MI_HDMIRX_PortId_e port_id = E_MI_HDMIRX_PORT0;
    MI_HDMIRX_SigStatus_e cur_signal_status_hdmirx = E_MI_HDMIRX_SIG_NO_SIGNAL;
    MI_HVP_SignalStatus_e cur_signal_status = E_MI_HVP_SIGNAL_UNSTABLE;
    MI_HVP_SignalTiming_t cur_signal_timing = {0, 0, 0, 0};
    MI_HDMIRX_TimingInfo_t cur_hdmirx_timing_info;
    static MI_HVP_SignalTiming_t last_signal_timing = {0, 0, 0, 0};
    static MI_HDMIRX_TimingInfo_t last_hdmirx_timing_info;

    ret = MI_HVP_GetResetEventFd(dev, &s32Fd);
    if (ret != MI_SUCCESS)
    {
        printf("Get Reset fd Errr, Hvp Dev%d!\n", dev);
        return NULL;
    }
    while (hvp_event_thread_running)
    {
        FD_ZERO(&read_fds);
        FD_SET(s32Fd, &read_fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;
        select_ret = select(s32Fd + 1, &read_fds, NULL, NULL, &tv);
        if (select_ret < 0)
        {
            printf("Reset fd select error!!\n");
            goto EXIT_FD;
        }
        ret = MI_HVP_GetResetEvent(dev, &bTrigger);
        if (ret != MI_SUCCESS)
        {
            printf("Get Reset cnt Errr, Hvp Dev%d!\n", dev);
            goto EXIT_FD;
        }
        ret = MI_HDMIRX_GetSignalStatus(port_id, &cur_signal_status_hdmirx);
        if (ret != MI_SUCCESS)
        {
            printf("Get hdmirx Signal Status Errr, port_id %d!\n", port_id);
            goto EXIT_RST_CNT;
        }
        ret = MI_HVP_GetSourceSignalStatus(dev, &cur_signal_status);
        if (ret != MI_SUCCESS)
        {
            printf("Get Signal Status Errr, Hvp Dev%d!\n", dev);
            goto EXIT_RST_CNT;
        }
        ret = MI_HVP_GetSourceSignalTiming(dev, &cur_signal_timing);
        if (ret != MI_SUCCESS)
        {
            printf("Get Signal Timing Errr, Hvp Dev%d!\n", dev);
            goto EXIT_RST_CNT;
        }
        ret = MI_HDMIRX_GetTimingInfo(port_id, &cur_hdmirx_timing_info);
        if (ret != MI_SUCCESS)
        {
            printf("Get Hdmirx Timing error!\n");
            goto EXIT_RST_CNT;
        }
        if (cur_signal_timing.u16Width != last_signal_timing.u16Width
            || cur_signal_timing.u16Height!= last_signal_timing.u16Height
            || cur_signal_timing.bInterlace!= last_signal_timing.bInterlace
            || cur_hdmirx_timing_info.eOverSample != last_hdmirx_timing_info.eOverSample
            || cur_hdmirx_timing_info.ePixelFmt != last_hdmirx_timing_info.ePixelFmt
            || cur_hdmirx_timing_info.eBitWidth != last_hdmirx_timing_info.eBitWidth)
        {
            printf("Get Signal St W: %d H: %d, Fps: %d bInterlace: %d OverSample: %d HdmirxFmt: %d BitWidth: %d\n",
                    cur_signal_timing.u16Width, cur_signal_timing.u16Height,
                    cur_signal_timing.u16Fpsx100, (int)cur_signal_timing.bInterlace,
                    cur_hdmirx_timing_info.eOverSample, cur_hdmirx_timing_info.ePixelFmt,
                    cur_hdmirx_timing_info.eBitWidth);
            if (cur_signal_status_hdmirx == E_MI_HDMIRX_SIG_SUPPORT && cur_signal_status == E_MI_HVP_SIGNAL_STABLE)
            {
                printf("Signal stable.\n");
            }
            else
            {
                printf("Signal unstable.\n");
            }
            signal_monitor_sync_hdmi_paramter_2_hvp(&cur_hdmirx_timing_info);
            last_signal_timing = cur_signal_timing;
            last_hdmirx_timing_info = cur_hdmirx_timing_info;
        }
        EXIT_RST_CNT:
        if (bTrigger)
        {
            printf("Get reset event!!\n");
            MI_HVP_ClearResetEvent(dev);
        }
    }
EXIT_FD:
    MI_HVP_CloseResetEventFd(dev, s32Fd);
    printf("hvp_event_thread exit\n");

    return NULL;
}

static inline int sync_wait(int fd, int timeout)
{
    struct timeval time;
    fd_set fs_read;
    int ret;
    time.tv_sec = timeout / 1000;
    time.tv_usec = timeout % 1000 * 1000;

    FD_ZERO(&fs_read);
    FD_SET(fd, &fs_read);

    ret = select(fd + 1, &fs_read, NULL, NULL, &time);
    if(ret <= 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static int get_property_id(int fd, drmModeObjectProperties* props, const char* name) {
    int id = 0;
    drmModePropertyPtr property;
    int found = 0;

    for (unsigned int i = 0; !found && i < props->count_props; ++i) {
        property = drmModeGetProperty(fd, props->props[i]);
        if (!strcmp(property->name, name)) {
            id = property->prop_id;
            found = 1;
        }
        drmModeFreeProperty(property);
    }

    return id;
}

int init_drm_property_ids(uint32_t plane_id, drm_property_ids_t* prop_ids)//int fd, uint32_t crtc_id, uint32_t plane_id, uint32_t conn_id,drm_property_ids_t* prop_ids)
{

    drmModeObjectProperties* props;
    drmModeAtomicReqPtr req;
    int ret;

    if(g_stDrmCfg.fd <= 0)
    {
        printf("Please check param \n");
        return -1;
    }

    props = drmModeObjectGetProperties(g_stDrmCfg.fd, g_stDrmCfg.crtc_id, DRM_MODE_OBJECT_CRTC);
    if (!props) {
        printf("Get properties error,crtc_id=%d \n",g_stDrmCfg.crtc_id);
        return -1;
    }

    prop_ids->FENCE_ID = get_property_id(g_stDrmCfg.fd, props, "OUT_FENCE_PTR");
    prop_ids->ACTIVE = get_property_id(g_stDrmCfg.fd, props, "ACTIVE");
    prop_ids->MODE_ID = get_property_id(g_stDrmCfg.fd, props, "MODE_ID");

    drmModeFreeObjectProperties(props);


    drmModeCreatePropertyBlob(g_stDrmCfg.fd, &g_stDrmCfg.Connector->modes[0],sizeof(g_stDrmCfg.Connector->modes[0]), &g_stDrmCfg.blob_id);


    props = drmModeObjectGetProperties(g_stDrmCfg.fd, plane_id, DRM_MODE_OBJECT_PLANE);
    if (!props) {
        printf("Get properties error,plane_id=%d \n",plane_id);
        return -1;
    }

    prop_ids->FB_ID = get_property_id(g_stDrmCfg.fd, props, "FB_ID");
    prop_ids->CRTC_ID = get_property_id(g_stDrmCfg.fd, props, "CRTC_ID");
    prop_ids->CRTC_X = get_property_id(g_stDrmCfg.fd, props, "CRTC_X");
    prop_ids->CRTC_Y = get_property_id(g_stDrmCfg.fd, props, "CRTC_Y");
    prop_ids->CRTC_W = get_property_id(g_stDrmCfg.fd, props, "CRTC_W");
    prop_ids->CRTC_H = get_property_id(g_stDrmCfg.fd, props, "CRTC_H");
    prop_ids->SRC_X = get_property_id(g_stDrmCfg.fd, props, "SRC_X");
    prop_ids->SRC_Y = get_property_id(g_stDrmCfg.fd, props, "SRC_Y");
    prop_ids->SRC_W = get_property_id(g_stDrmCfg.fd, props, "SRC_W");
    prop_ids->SRC_H = get_property_id(g_stDrmCfg.fd, props, "SRC_H");
    prop_ids->realtime_mode = get_property_id(g_stDrmCfg.fd, props, "realtime_mode");
    prop_ids->zpos = get_property_id(g_stDrmCfg.fd, props, "zpos");

    drmModeFreeObjectProperties(props);

    printf("DRM property ids. FB_ID=%d CRTC_ID=%d CRTC_X=%d CRTC_Y=%d CRTC_W=%d CRTC_H=%d"
           "SRC_X=%d SRC_Y=%d SRC_W=%d SRC_H=%d, realtime_mode=%d, FENCE_ID=%d zpos=%d\n",
           prop_ids->FB_ID, prop_ids->CRTC_ID, prop_ids->CRTC_X, prop_ids->CRTC_Y, prop_ids->CRTC_W,
           prop_ids->CRTC_H, prop_ids->SRC_X, prop_ids->SRC_Y, prop_ids->SRC_W, prop_ids->SRC_H,
           prop_ids->realtime_mode,prop_ids->FENCE_ID,prop_ids->zpos);

    req = drmModeAtomicAlloc();
    if (!req) {
        printf("drmModeAtomicAlloc failed \n");
        return -1;
    }
    drmModeAtomicAddProperty(req, g_stDrmCfg.crtc_id, prop_ids->ACTIVE, 1);
    drmModeAtomicAddProperty(req, g_stDrmCfg.crtc_id, prop_ids->MODE_ID, g_stDrmCfg.blob_id);
    drmModeAtomicAddProperty(req, g_stDrmCfg.conn_id, prop_ids->CRTC_ID, g_stDrmCfg.crtc_id);
    ret = drmModeAtomicCommit(g_stDrmCfg.fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET | DRM_MODE_ATOMIC_NONBLOCK, NULL);

    if(ret != 0)
    {
        printf("111drmModeAtomicCommit failed ret=%d \n",ret);
        return -1;
    }
    drmModeAtomicFree(req);

    return 0;
}


static int add_plane_property(drmModeAtomicReqPtr req, int plane_id, uint32_t prop_id, int val)
{
    #if 0
    drmModeObjectProperties* pModeProps = NULL;
    int prop_id = -1;
    pModeProps = drmModeObjectGetProperties(g_stDrmCfg.fd, plane_id, DRM_MODE_OBJECT_PLANE);
    if (!pModeProps) {
        printf("Get properties error,plane_id=%d \n",plane_id);
        return  -1;
    }
    prop_id = get_property_id(g_stDrmCfg.fd, pModeProps, name);

    drmModeFreeObjectProperties(pModeProps);
    #endif
    drmModeAtomicAddProperty(req, plane_id, prop_id, val);
    return 0;
}


unsigned int AddDmabufToDRM(stDmaBuf_t* pstDmaBuf)
{
    int ret = -1;
    drm_buf_t *drm_buf = (drm_buf_t *)pstDmaBuf->priavte;
    unsigned int pitches[4] = {0}, offsets[4] = {0}, gem_handles[4] = {0};
    unsigned int width = 0, height = 0, fb_id = 0, drm_fmt = 0;

    width = MIN(pstDmaBuf->width, g_stDrmCfg.width);
    height = MIN(pstDmaBuf->height, g_stDrmCfg.height);

    if(pstDmaBuf->format  == E_STREAM_YUV420)
    {
        pitches[0] = width;
        pitches[1] = width;
        offsets[0] = 0;
        offsets[1] = pitches[0] * height;
        drm_fmt = DRM_FORMAT_NV12;
    }
    if(pstDmaBuf->format  == E_STREAM_ABGR8888)
    {
        //width = g_u32UiWidth;
        //height = g_u32UiHeight;
        pitches[0] = width * 4;
        offsets[0] = 0;
        drm_fmt = DRM_FORMAT_ABGR8888;
    }

    ret = drmPrimeFDToHandle(g_stDrmCfg.fd, pstDmaBuf->fds[0], &gem_handles[0]);
    if (ret) {
        printf("drmPrimeFDToHandle failed, ret=%d dma_buffer_fd=%d\n", ret, pstDmaBuf->fds[0]);
        return -1;
    }
    gem_handles[1] = gem_handles[0];

    ret = drmModeAddFB2(g_stDrmCfg.fd, width, height, drm_fmt, gem_handles, pitches, offsets, &fb_id, 0);
    //printf("add fb:\n w=%d,h=%d,fmt=%d\n", width, height, drm_fmt);
    for(int i = 0;i < 4; i++)
    {
        if(pstDmaBuf->format  == E_STREAM_ABGR8888)
        {
            //printf("%d : handle = %d  pitches = %d  offset = %d\n", i, gem_handles[i], pitches[i], offsets[i]);
        }
    }

    if (ret)
    {
        printf("drmModeAddFB2 failed, ret=%d fb_id=%d\n", ret, fb_id);
        return -1;
    }
    drm_buf->fb_id = fb_id;
    drm_buf->gem_handle[0] = gem_handles[0];

    return fb_id;
}
// drm gem list that should be released

static int drm_free_gem_handle(std::pair<uint32_t, uint32_t> *gem_handle) {
    int ret = -1;
    struct drm_gem_close gem_close {};
    uint32_t fb_id = gem_handle->first;
    gem_close.handle = gem_handle->second;

    if (fb_id == static_cast<uint32_t>(-1) || gem_close.handle == static_cast<uint32_t>(-1)) {
        return 0;
    }

    ret = drmModeRmFB(g_stDrmCfg.fd, fb_id);
    if (ret != 0) {
        printf("Failed to remove fb, ret=%d \n", ret);
    }
    ret |= drmIoctl(g_stDrmCfg.fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
    if (ret != 0) {
        printf("Failed to close gem handle %d \n", gem_close.handle);
        return -1;
    }
    gem_handle->first = static_cast<uint32_t>(-1);
    gem_handle->second = static_cast<uint32_t>(-1);
    return 0;
}



static int atomic_set_plane(int plane_id, int fb_id, int is_realtime) {
    int ret;
    drmModeAtomicReqPtr req;
    req = drmModeAtomicAlloc();
    if (!req) {
        printf("drmModeAtomicAlloc failed \n");
        return -1;
    }

    if(plane_id == GOP_UI_ID)
    {
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[0].FB_ID , fb_id);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[0].CRTC_ID, g_stDrmCfg.crtc_id);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[0].CRTC_X, 0);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[0].CRTC_Y, 0);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[0].CRTC_W, g_stDrmCfg.width);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[0].CRTC_H, g_stDrmCfg.height);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[0].SRC_X, 0);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[0].SRC_Y, 0);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[0].SRC_W, ALIGN_BACK(g_stDrmCfg.width,16) << 16);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[0].SRC_H, g_stDrmCfg.height << 16);
    }
    else
    {
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[1].FB_ID, fb_id);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_ID, g_stDrmCfg.crtc_id);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_X, _g_MediaPlayer.video_info.pos_x);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_Y, _g_MediaPlayer.video_info.pos_y);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_W, _g_MediaPlayer.video_info.out_width);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_H, _g_MediaPlayer.video_info.out_height);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[1].SRC_X, 0);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[1].SRC_Y, 0);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[1].SRC_W, ALIGN_BACK(g_stDrmCfg.width,16) << 16);
        add_plane_property(req, plane_id, g_stDrmCfg.drm_mode_prop_ids[1].SRC_H, g_stDrmCfg.height << 16);
    }
    if(is_realtime)
    {
        //add_plane_property(req, plane_id, "realtime_mode", 1);
    }
    drmModeAtomicAddProperty(req, g_stDrmCfg.crtc_id, g_stDrmCfg.drm_mode_prop_ids[1].FENCE_ID, (uint64_t)&g_stDrmCfg.out_fence);//use this flag,must be close out_fence

    ret = drmModeAtomicCommit(g_stDrmCfg.fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
    if(ret != 0)
    {
        printf("[atomic_set_plane]drmModeAtomicCommit failed ret=%d plane_id=%d\n",ret,plane_id);
    }
    g_stDrmCfg.drm_commited = 1;
    drmModeAtomicFree(req);

    ret = sync_wait(g_stDrmCfg.out_fence, 16);
    if(ret != 0)
    {
        printf("waring:maybe drop one drm frame, ret=%d out_fence=%d\n", ret, g_stDrmCfg.out_fence);
    }
    close(g_stDrmCfg.out_fence);

    return 0;
}

MI_S32 STUB_UpdataAllPointOffset()
{
    MI_BOOL bUpdataGpuGfxPointFail = false;
    if (!g_stdGpuGfx->updateLTPointOffset(g_u32GpuLT_X+2, g_u32GpuLT_Y+2))
    {
        //printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
        printf("Failed to set keystone LT correction points\n");
        bUpdataGpuGfxPointFail = true;
        return -1;
    }
    if (!g_stdGpuGfx->updateLBPointOffset(g_u32GpuLB_X+2, g_u32GpuLB_Y+2))
    {
        //printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
        printf("Failed to set keystone LB correction points\n");
        bUpdataGpuGfxPointFail = true;
        return -2;
    }
    if (!g_stdGpuGfx->updateRTPointOffset(g_u32GpuRT_X+2, g_u32GpuRT_Y+2))
    {
        //printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
        printf("Failed to set keystone RT correction points\n");
        bUpdataGpuGfxPointFail = true;
        return -3;
    }
    if (!g_stdGpuGfx->updateRBPointOffset(g_u32GpuRB_X+2, g_u32GpuRB_Y+2))
    {
        //printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
        printf("Failed to set keystone RB correction points\n");
        bUpdataGpuGfxPointFail = true;
        return -4;
    }
    if (bUpdataGpuGfxPointFail == false)
    {
        printf("Updata Keystone info done:\n");
        printf("==================================\n");
        printf("LTPointOffset X:%d, Y:%d\n", g_u32GpuLT_X, g_u32GpuLT_Y);
        printf("LBPointOffset X:%d, Y:%d\n", g_u32GpuLB_X, g_u32GpuLB_Y);
        printf("RTPointOffset X:%d, Y:%d\n", g_u32GpuRT_X, g_u32GpuRT_Y);
        printf("RBPointOffset X:%d, Y:%d\n", g_u32GpuRB_X, g_u32GpuRB_Y);
        printf("==================================\n");
        g_bGpuInfoChange_mop = false;
        g_bGpuUifresh = true;
    }
    return MI_SUCCESS;
}

MI_S32 STUB_ResetAllpointOffset()
{
    if (!g_stdGpuGfx->updateLTPointOffset(3, 3))
    {
        printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
        return -1;
    }
    if (!g_stdGpuGfx->updateLBPointOffset(3, 3))
    {
        printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
        return -2;
    }
    if (!g_stdGpuGfx->updateRTPointOffset(3, 3))
    {
        printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
        return -3;
    }
    if (!g_stdGpuGfx->updateRBPointOffset(3, 3))
    {
        printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
        return -4;
    }
    return MI_SUCCESS;
}

/* User handle event */
static int mm_cal_video_size(video_info_t *video_info, int pic_width, int pic_height)
{
    int screen_width    = 0;
    int screen_height   = 0;
    int sar_16_9_width  = 0;
    int sar_16_9_height = 0;
    int sar_4_3_width   = 0;
    int sar_4_3_height  = 0;
    int origin_width    = 0;
    int origin_height   = 0;
    double video_ratio  = 1.0 * pic_width / pic_height;
    double panel_ratio  = 1.0 * video_info->in_width / video_info->in_height;

    if (panel_ratio > 1.78) {
        sar_16_9_width  = ALIGN_UP(video_info->in_height * 16 / 9, 2);
        sar_16_9_height = video_info->in_height;
    } else {
        sar_16_9_width  = video_info->in_width;
        sar_16_9_height = ALIGN_UP(video_info->in_width * 9 / 16, 2);
    }

    if (panel_ratio > 1.34) {
        sar_4_3_width  = ALIGN_UP(video_info->in_height * 4 / 3, 2);
        sar_4_3_height = video_info->in_height;
    } else {
        sar_4_3_width  = video_info->in_width;
        sar_4_3_height = ALIGN_UP(video_info->in_width * 3 / 4, 2);
    }

    if (panel_ratio / video_ratio > 1.0) {
        origin_height = MIN(pic_height, video_info->in_height);
        origin_width  = MIN(ALIGN_UP(origin_height * pic_width / pic_height, 2), video_info->in_width);
    } else {
        origin_width  = MIN(pic_width, video_info->in_width);
        origin_height = MIN(ALIGN_UP(origin_width * pic_height / pic_width, 2), video_info->in_height);
    }
    //printf("panel_ratio=%f video_ratio=%f pic_width w/h=[%d %d], video_info w/h=[%d %d], origin_width w/h=[%d %d] \n",
    //                    panel_ratio,video_ratio,pic_width,pic_height,video_info->in_width,video_info->in_height,origin_width,origin_height);

    screen_width  = video_info->in_width;
    screen_height = video_info->in_height;

    switch (video_info->ratio) {
        case AV_ORIGIN_MODE : {
            video_info->out_width  = origin_width;
            video_info->out_height = origin_height;
            video_info->dec_width  = MIN(origin_width , pic_width);
            video_info->dec_height = MIN(origin_height, pic_height);
        }
        break;

        case AV_SAR_4_3_MODE : {
            video_info->out_width  = sar_4_3_width;
            video_info->out_height = sar_4_3_height;
            video_info->dec_width  = MIN(sar_4_3_width , pic_width);
            video_info->dec_height = MIN(sar_4_3_height, pic_height);
        }
        break;

        case AV_SAR_16_9_MODE : {
            video_info->out_width  = sar_16_9_width;
            video_info->out_height = sar_16_9_height;
            video_info->dec_width  = MIN(sar_16_9_width , pic_width);
            video_info->dec_height = MIN(sar_16_9_height, pic_height);
        }
        break;

        case AV_SCREEN_MODE :
        default : {
            video_info->out_width  = screen_width;
            video_info->out_height = screen_height;
            video_info->dec_width  = MIN(screen_width , pic_width);
            video_info->dec_height = MIN(screen_height, pic_height);
        }
        break;
    }

    video_info->dec_width  = ALIGN_UP(video_info->dec_width, 2);
    video_info->dec_height = ALIGN_UP(video_info->dec_height, 2);

    printf("origin w/h=[%d %d], screen w/h=[%d %d], sar4:3 w/h=[%d %d], sar16:9 w/h=[%d %d] panel_ratio=%f\n",
        origin_width, origin_height, screen_width, screen_height, sar_4_3_width, sar_4_3_height, sar_16_9_width, sar_16_9_height,panel_ratio);
    printf("in w/h=[%d %d], dec w/h=[%d %d], rot w/h=[%d %d], out w/h=[%d %d]\n",
                video_info->in_width, video_info->in_height, video_info->dec_width, video_info->dec_height,
                video_info->rot_width, video_info->rot_height, video_info->out_width, video_info->out_height);

    if (video_info->out_width <= 0 || video_info->out_height <= 0) {
        printf("set video sar width and height error!\n");
        return -1;
    }

    return 0;
}

static int mm_cal_rotate_size(video_info_t *video_info)
{
    if (video_info->rotate != AV_ROTATE_180 && video_info->rotate != AV_ROTATE_NONE) {
        int tmp_value;
        video_info->rot_width  = video_info->dec_height;
        video_info->rot_height = video_info->dec_width;
        tmp_value = video_info->out_width;
        video_info->out_width  = video_info->out_height;
        video_info->out_height = tmp_value;
        video_info->pos_x = MAX(0, (video_info->in_height - video_info->out_width) / 2);
        video_info->pos_y = MAX(0, (video_info->in_width - video_info->out_height) / 2);
    } else {
        video_info->rot_width  = video_info->dec_width;
        video_info->rot_height = video_info->dec_height;
        video_info->pos_x = MAX(0, (video_info->in_width - video_info->out_width) / 2);
        video_info->pos_y = MAX(0, (video_info->in_height - video_info->out_height) / 2);
    }
    printf("scaler w/h = [%d %d], dst x/y/w/h = [%d %d %d %d]\n",
        video_info->dec_width, video_info->dec_height, video_info->pos_x,
        video_info->pos_y, video_info->out_width, video_info->out_height);

    return 0;
}

static int mm_set_video_size(seq_info_t *seq_info)
{
    int ret = 0;
    int strm_width = 0;
    int strm_height = 0;

    if (seq_info->crop_width > 0 && seq_info->crop_height > 0) {
        strm_width = seq_info->crop_width;
        strm_height = seq_info->crop_height;
    } else {
        strm_width = seq_info->pic_width;
        strm_height = seq_info->pic_height;
    }

    // reset in width/height

    if (_g_MediaPlayer.video_info.rotate != AV_ROTATE_NONE && _g_MediaPlayer.video_info.rotate != AV_ROTATE_180) {

        _g_MediaPlayer.video_info.in_width  = _g_MediaPlayer.param.panel_height;
        _g_MediaPlayer.video_info.in_height = _g_MediaPlayer.param.panel_width;

    } else {

        _g_MediaPlayer.video_info.in_width  = _g_MediaPlayer.param.panel_width;
        _g_MediaPlayer.video_info.in_height = _g_MediaPlayer.param.panel_height;


    }

    // set video size in user mode
    ret = mm_cal_video_size(&_g_MediaPlayer.video_info, strm_width, strm_height);
    if (ret < 0) {
        printf("set_video_size error!\n");
        return -1;
    }

    // set video size by rotate
    mm_cal_rotate_size(&_g_MediaPlayer.video_info);
    printf("mm_set_video_size  rotate=%d dec_width=%d dec_height=%d .\n",_g_MediaPlayer.video_info.rotate, _g_MediaPlayer.video_info.dec_width, _g_MediaPlayer.video_info.dec_height);

    mm_player_set_dec_out_attr(_g_MediaPlayer.video_info.dec_width, _g_MediaPlayer.video_info.dec_height);

    return 0;
}


static void mm_video_event_handle(event_info_t *event_info)
{
    if (!event_info) {
        printf("get event info null ptr\n");
        return;
    }

    //printf("video event(%d).\n", event_info->event);

    switch (event_info->event) {
        case AV_DEC_EVENT_DECERR:
            printf("get dec err event...\n");
            break;
        case AV_DEC_EVENT_EOS:
            printf("get eos event ...\n");
            break;
        case AV_DEC_EVENT_SEQCHANGE:
            //printf("get seq change event...\n");
            printf("get seq info wh(%d %d), crop(%d %d %d %d)\n",
                   event_info->seq_info.pic_width, event_info->seq_info.pic_height,
                   event_info->seq_info.crop_x, event_info->seq_info.crop_y,
                   event_info->seq_info.crop_width, event_info->seq_info.crop_height);
            memcpy(&_g_MediaPlayer.video_info.seq_info, &event_info->seq_info, sizeof(seq_info_t));
            mm_set_video_size(&event_info->seq_info);
            break;
        case AV_DEC_EVENT_DROPPED:
            //printf("get frame drop event...\n");
            break;
        case AV_DEC_EVENT_DECDONE:
            //printf("get decframe done event...\n");
            break;
        default:
            printf("event type(%x) err\n", event_info->event);
            break;
    }
}

static void sstar_player_default_opts()
{
    mm_player_set_opts("audio_format", "", AV_PCM_FMT_S16);
    mm_player_set_opts("audio_channels", "", 2);
    mm_player_set_opts("audio_samplerate", "", 44100);

    mm_player_set_opts("video_bypass", "", 0);
    mm_player_set_opts("video_rotate", "", _g_MediaPlayer.video_info.rotate);
    mm_player_set_opts("video_ratio", "", _g_MediaPlayer.video_info.ratio);

    mm_player_register_event_cb(mm_video_event_handle);

    printf("ssplayer video_rotate=%d _g_rotate_mode=%d url:%s panel_height=%d panel_width=%d\n",_g_MediaPlayer.video_info.rotate,_g_rotate_mode, _g_MediaPlayer.param.url,_g_MediaPlayer.param.panel_height,_g_MediaPlayer.param.panel_width);
}

static void sstar_player_init()
{
    // set default ratio mode
    _g_MediaPlayer.video_info.ratio = (int)AV_SCREEN_MODE;
    _g_MediaPlayer.param.protocol = AV_PROTOCOL_FILE;

    // set default disp windows size
    _g_MediaPlayer.param.panel_width = g_stDrmCfg.width;
    _g_MediaPlayer.param.panel_height = g_stDrmCfg.height;

    _g_MediaPlayer.param.url = g_InputFile;

    sstar_player_default_opts();

}

static MI_S32 sstar_scl_init()
{
    MI_U32 u32SclChnId = 0;
    MI_U32 u32SclPortId = 0;
    MI_U32 u32SclDevId = 0;

    if (g_u8PipelineMode == 0) //HDMI_RX -> HVP -> SCL
    {
        u32SclDevId = 8;
    }
    else if (g_u8PipelineMode == 1)//File -> FFMPEG -> SCL
    {
        u32SclDevId = 1;
    }


    MI_SCL_DevAttr_t stSclDevAttr;
    MI_SCL_ChannelAttr_t stSclChnAttr;
    MI_SCL_ChnParam_t stSclChnParam;
    MI_SYS_WindowRect_t stSclInputCrop;
    MI_SCL_OutPortParam_t  stSclOutPortParam;

    memset(&stSclDevAttr, 0x00, sizeof(MI_SCL_DevAttr_t));
    stSclDevAttr.u32NeedUseHWOutPortMask = E_MI_SCL_HWSCL0;//E_MI_SCL_HWSCL1 is already user in ssplayer for rotate

    STCHECKRESULT(MI_SCL_CreateDevice(u32SclDevId, &stSclDevAttr));

    memset(&stSclChnAttr, 0x00, sizeof(MI_SCL_ChannelAttr_t));
    memset(&stSclInputCrop, 0x00, sizeof(MI_SYS_WindowRect_t));
    memset(&stSclChnParam, 0x00, sizeof(MI_SCL_ChnParam_t));

    STCHECKRESULT(MI_SCL_CreateChannel(u32SclDevId, u32SclChnId, &stSclChnAttr));
    STCHECKRESULT(MI_SCL_SetChnParam(u32SclDevId, u32SclChnId, &stSclChnParam));

    STCHECKRESULT(MI_SCL_StartChannel(u32SclDevId, u32SclChnId));

    memset(&stSclOutPortParam, 0x0, sizeof(MI_SCL_OutPortParam_t));
    stSclOutPortParam.ePixelFormat = (MI_SYS_PixelFormat_e) E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stSclOutPortParam.stSCLOutputSize.u16Width= ALIGN_BACK(g_stDrmCfg.width,16);
    stSclOutPortParam.stSCLOutputSize.u16Height= g_stDrmCfg.height;

    stSclOutPortParam.bMirror = FALSE;
    stSclOutPortParam.bFlip = FALSE;
    stSclOutPortParam.eCompressMode = (MI_SYS_CompressMode_e)E_MI_SYS_COMPRESS_MODE_NONE;

    STCHECKRESULT(MI_SCL_SetOutputPortParam(u32SclDevId, u32SclChnId, u32SclPortId, &stSclOutPortParam));
    STCHECKRESULT(MI_SCL_EnableOutputPort(u32SclDevId, u32SclChnId, u32SclPortId));


    MI_SYS_ChnPort_t stSclChnPort;
    memset(&stSclChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));

    stSclChnPort.eModId    = E_MI_MODULE_ID_SCL;
    stSclChnPort.u32DevId  = u32SclDevId;
    stSclChnPort.u32ChnId  = u32SclChnId;
    stSclChnPort.u32PortId = u32SclPortId;

    STCHECKRESULT(MI_SYS_CreateChnOutputPortDmabufCusAllocator(&stSclChnPort));

    return MI_SUCCESS;

}
static MI_S32 sstar_scl_deinit()
{
    MI_U32 u32SclChnId = 0;
    MI_U32 u32SclPortId = 0;
    MI_U32 u32SclDevId = 0;
    MI_SYS_ChnPort_t stSclChnPort;

    if (g_u8PipelineMode == 0) //HDMI_RX -> HVP -> SCL
    {
        u32SclDevId = 8;
    }
    else if (g_u8PipelineMode == 1)//File -> FFMPEG -> SCL
    {
        u32SclDevId = 1;
    }

    memset(&stSclChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));

    stSclChnPort.eModId    = E_MI_MODULE_ID_SCL;
    stSclChnPort.u32DevId  = u32SclDevId;
    stSclChnPort.u32ChnId  = u32SclChnId;
    stSclChnPort.u32PortId = u32SclPortId;
    STCHECKRESULT(MI_SYS_DestroyChnOutputPortDmabufCusAllocator(&stSclChnPort));

    STCHECKRESULT(MI_SCL_DisableOutputPort(u32SclDevId, u32SclChnId, u32SclPortId));
    STCHECKRESULT(MI_SCL_StopChannel(u32SclDevId, u32SclChnId));
    STCHECKRESULT(MI_SCL_DestroyChannel(u32SclDevId, u32SclChnId));
    STCHECKRESULT(MI_SCL_DestroyDevice(u32SclDevId));
    return MI_SUCCESS;
}


void *ST_UI_Task(void * arg)
{
    int BigUiWidth = ALIGN_BACK(g_stDrmCfg.width,16);
    int BigUiHeight = g_stDrmCfg.height;
    //int SmallUiWidth  = g_u32UiWidth;
    //int SmallUiHeight = g_u32UiHeight;
    int SmallUiWidth  = _g_MediaPlayer.ui_info.in_width;
    int SmallUiHeight = _g_MediaPlayer.ui_info.in_height;


    int startX = 100;
    int startY = 100;
    FILE* file = nullptr;

    uint32_t* BigUiImageData    = (uint32_t*)malloc(BigUiWidth * BigUiHeight * sizeof(uint32_t));
    uint32_t* SmallUiImageData  = (uint32_t*)malloc(SmallUiWidth * SmallUiHeight * sizeof(uint32_t));

    file = fopen(g_InputUiPath, "r+");
    if (file)
    {
        for (int i = 0; i < SmallUiWidth * SmallUiHeight; i++)
        {
            uint8_t pixelData[4];
            fread(pixelData, sizeof(uint8_t), 4, file);
            SmallUiImageData[i] = (pixelData[0] << 24) | (pixelData[1] << 16) | (pixelData[2] << 8) | pixelData[3];
        }
        fclose(file);
    }
    else
    {
        printf("无法打开文件 %s\n", g_InputUiPath);
    }

    for (int y = 0; y < BigUiHeight; y++)
    {
        for (int x = 0; x < BigUiWidth; x++)
        {
            if (x < 5 || x >= BigUiWidth - 5 || y < 5 || y >= BigUiHeight - 5)
            {
                // 在边缘区域绘制黑色像素
                int index = y * BigUiWidth + x; // 计算像素索引
                BigUiImageData[index] = 0xFF000000; // 设置为黑色（Alpha: FF, Red: 00, Green: 00, Blue: 00）
            } else
            {
                // 其余部分设置为透明
                int index = y * BigUiWidth + x; // 计算像素索引
                BigUiImageData[index] = 0x00000000; // 设置为透明（Alpha: 00, Red: 00, Green: 00, Blue: 00）
            }
        }
    }

    for (int y = 0; y < SmallUiHeight; y++)
    {
        for (int x = 0; x < SmallUiWidth; x++)
        {
            uint32_t tmpData = SmallUiImageData[y * SmallUiWidth + x];

            int bigX = startX + x;
            int bigY = startY + y;
            if (bigX < BigUiWidth && bigY < BigUiHeight)
            {
                BigUiImageData[bigY * BigUiWidth + bigX] = tmpData;
            }
        }
    }

    std::shared_ptr<GpuGraphicBuffer> inputBuffer;
    std::shared_ptr<GpuGraphicBuffer> outputBuffer;
    std::shared_ptr<GpuGraphicEffect> gpugfx = std::make_shared<GpuGraphicEffect>();

    uint32_t u32UiFormat;
    u32UiFormat = DRM_FORMAT_ABGR8888;
    void *pVaddr = nullptr;

    int32_t ret;
    stDmaBuf_t OutputBuf;
    drm_buf_t drm_buf;

    inputBuffer = std::make_shared<GpuGraphicBuffer>(BigUiWidth, BigUiHeight, u32UiFormat, BigUiWidth * 4);
    if (!inputBuffer->initCheck()) {
        printf("Create GpuGraphicBuffer failed\n");
        return NULL;
    }

    pVaddr = mmap(NULL, inputBuffer->getBufferSize(), PROT_WRITE|PROT_READ, MAP_SHARED, inputBuffer->getFd(), 0);
    if(!pVaddr) {
        printf("Failed to mmap dma buffer\n");
        return NULL;
    }

    memcpy(pVaddr, BigUiImageData, BigUiWidth*BigUiHeight*sizeof(uint32_t));

    ret = gpugfx->init(BigUiWidth, BigUiHeight, u32UiFormat);
    if (ret) {
        printf("Failed to init gpu graphic effect\n");
        return NULL;
    }

    if (!gpugfx->updateLTPointOffset(g_u32GpuLT_X+1, g_u32GpuLT_Y+1))
    {
        //printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
        printf("Failed to set keystone LT correction points\n");
    }
    if (!gpugfx->updateLBPointOffset(g_u32GpuLB_X+1, g_u32GpuLB_Y+1))
    {
        //printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
        printf("Failed to set keystone LB correction points\n");
    }
    if (!gpugfx->updateRTPointOffset(g_u32GpuRT_X+1, g_u32GpuRT_Y+1))
    {
        //printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
        printf("Failed to set keystone RT correction points\n");
    }
    if (!gpugfx->updateRBPointOffset(g_u32GpuRB_X+1, g_u32GpuRB_Y+1))
    {
        //printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
        printf("Failed to set keystone RB correction points\n");
    }
    ret = gpugfx->process(inputBuffer, g_RectDisplayRegion, outputBuffer);
    if (ret) {
        printf("Gpu graphic effect process failed\n");
        return NULL;
    }

    if(munmap(pVaddr, inputBuffer->getBufferSize()) != 0)
    {
        printf("Failed to munmap dma buffer\n");
    }
    pVaddr = nullptr;

    OutputBuf.width   = outputBuffer->getWidth();
    OutputBuf.height  = outputBuffer->getHeight();
    OutputBuf.format  = E_STREAM_ABGR8888;
    OutputBuf.fds[0]  = outputBuffer->getFd();
    OutputBuf.stride[0] = outputBuffer->getWidth() * 4;
    OutputBuf.dataOffset[0] = 0;

    OutputBuf.priavte = (void *)&drm_buf;

    ret = AddDmabufToDRM(&OutputBuf);
    if (ret != 0)
    {
        //printf("AddDmabufToDRM:%d\n", ret);
    }
    atomic_set_plane(GOP_UI_ID, drm_buf.fb_id, 0);
    drm_free_gem_handle(&prev_gop_gem_handle);

    prev_gop_gem_handle.first = drm_buf.fb_id;
    prev_gop_gem_handle.second = drm_buf.gem_handle[0];

    while(g_bThreadExitUiDrm == TRUE)
    {
        if(g_bGpuUifresh == true)
        {
            g_bGpuUifresh = false;

            if (!gpugfx->updateLTPointOffset(g_u32GpuLT_X+1, g_u32GpuLT_Y+1))
            {
                //printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
                printf("Failed to set keystone LT correction points\n");
            }
            if (!gpugfx->updateLBPointOffset(g_u32GpuLB_X+1, g_u32GpuLB_Y+1))
            {
                //printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
                printf("Failed to set keystone LB correction points\n");
            }
            if (!gpugfx->updateRTPointOffset(g_u32GpuRT_X+1, g_u32GpuRT_Y+1))
            {
                //printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
                printf("Failed to set keystone RT correction points\n");
            }
            if (!gpugfx->updateRBPointOffset(g_u32GpuRB_X+1, g_u32GpuRB_Y+1))
            {
                //printf("[LINE: %d]Failed to set keystone correction points\n",__LINE__);
                printf("Failed to set keystone RB correction points\n");
            }

            ret = gpugfx->process(inputBuffer, g_RectDisplayRegion, outputBuffer);
            if (ret)
            {
                printf("Gpu graphic effect process failed\n");
                return NULL;
            }

            OutputBuf.width   = outputBuffer->getWidth();
            OutputBuf.height  = outputBuffer->getHeight();
            OutputBuf.format  = E_STREAM_ABGR8888;
            OutputBuf.fds[0]  = outputBuffer->getFd();
            OutputBuf.stride[0] = outputBuffer->getWidth() * 4;
            OutputBuf.dataOffset[0] = 0;

            OutputBuf.priavte = (void *)&drm_buf;

            ret = AddDmabufToDRM(&OutputBuf);
            if (ret != 0)
            {
                //printf("AddDmabufToDRM:%d\n", ret);
            }

            atomic_set_plane(GOP_UI_ID, drm_buf.fb_id, 0);
            drm_free_gem_handle(&prev_gop_gem_handle);
            prev_gop_gem_handle.first = drm_buf.fb_id;
            prev_gop_gem_handle.second = drm_buf.gem_handle[0];

        }
    }

    inputBuffer = nullptr;
    gpugfx->deinit();

    return NULL;
}

void *ST_SCL_Task(void * arg)
{
    MI_S32 s32Ret = MI_SUCCESS;
    uint32_t u32Width  = ALIGN_BACK(g_stDrmCfg.width,16);
    uint32_t u32Height = g_stDrmCfg.height;
    std::shared_ptr<GpuGraphicBuffer> GfxInputBuffer;
    uint32_t u32GpuInputFormat = DRM_FORMAT_NV12;
    MI_S32 s32SclChn = 0;
    ST_GfxInputBuf_Node *ListSclToGfx;

    //*******scl port0*********
    MI_SYS_ChnPort_t stSclChnPort0;
    MI_SYS_DmaBufInfo_t stDmaBufInfo;

    MI_U8  u8SclDev;
    if (g_u8PipelineMode == 0)
    {
        u8SclDev = 8;
    }
    else if(g_u8PipelineMode == 1)
    {
        u8SclDev = 1;
    }

    memset(&stSclChnPort0, 0x0, sizeof(MI_SYS_ChnPort_t));

    stSclChnPort0.eModId = E_MI_MODULE_ID_SCL;
    stSclChnPort0.u32DevId = u8SclDev;
    stSclChnPort0.u32ChnId = s32SclChn;
    stSclChnPort0.u32PortId = 0x0;

    //STUB_ResetAllpointOffset();
    ///*******g_bThreadExitScl*********//
    while (g_bThreadExitScl == TRUE)
    {
        if(u8EnqueueDmabufCnt > 3)
        {
            continue;
        }

        ListSclToGfx = new ST_GfxInputBuf_Node;
        if(ListSclToGfx == NULL)
        {
            printf("ListSclToGfx malloc Error!\n");
        }

        if(g_bGpuInfoChange_mop == true)
        {
            STUB_UpdataAllPointOffset();
        }

        GfxInputBuffer = std::make_shared<GpuGraphicBuffer>(u32Width, u32Height, u32GpuInputFormat, u32Width);
        if (!GfxInputBuffer->initCheck())
        {
            printf("Create GpuGraphicBuffer failed\n");
            continue;
        }
        memset(&stDmaBufInfo, 0x00, sizeof(MI_SYS_DmaBufInfo_t));
        stDmaBufInfo.s32Fd[0] = GfxInputBuffer->getFd();
        stDmaBufInfo.s32Fd[1] = GfxInputBuffer->getFd();
        stDmaBufInfo.u16Width = (MI_U16)GfxInputBuffer->getWidth();
        stDmaBufInfo.u16Height = (MI_U16)GfxInputBuffer->getHeight();
        stDmaBufInfo.u32Stride[0] = GfxInputBuffer->getWidth();
        stDmaBufInfo.u32Stride[1] = GfxInputBuffer->getWidth();
        stDmaBufInfo.u32DataOffset[0] = 0;
        stDmaBufInfo.u32DataOffset[1] = stDmaBufInfo.u16Width *stDmaBufInfo.u16Height;
        if(GfxInputBuffer->getFormat() == DRM_FORMAT_NV12)
        {
            stDmaBufInfo.eFormat = (MI_SYS_PixelFormat_e)E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        }

        pthread_mutex_lock(&ListMutexGfx);

        ListSclToGfx->pstdInputBuffer = GfxInputBuffer;
        memcpy(&(ListSclToGfx->stDmaBufInfo), &stDmaBufInfo, sizeof(MI_SYS_DmaBufInfo_t));

        list_add_tail(&ListSclToGfx->list, &g_HeadListGpuGfxInput);

        pthread_mutex_unlock(&ListMutexGfx);

        s32Ret = MI_SYS_ChnOutputPortEnqueueDmabuf(&stSclChnPort0, &stDmaBufInfo);
        if (s32Ret != 0)
        {
            printf("MI_SYS_ChnOutputPortEnqueueDmabuf fail\n");
            return NULL;
        }
        else{
            u8EnqueueDmabufCnt++;
        }
    }
    printf("g_bThreadExitScl == end!!!\n ");

    return NULL;
}

void *ST_GFX_Task(void * arg)
{
    MI_S32 s32Ret = MI_SUCCESS;

    MI_U8  u8SclDev;
    if (g_u8PipelineMode == 0)
    {
        u8SclDev = 8;
    }
    else if(g_u8PipelineMode == 1)
    {
        u8SclDev = 1;
    }
    MI_S32 s32SclChn = 0;

    //*******scl port0*********
    MI_SYS_ChnPort_t stSclChnPort0;
    MI_SYS_DmaBufInfo_t stDmaBufInfo;
    MI_SYS_DmaBufInfo_t stDmaOutputBufInfo2;
    memset(&stSclChnPort0, 0x0, sizeof(MI_SYS_ChnPort_t));

    stSclChnPort0.eModId = E_MI_MODULE_ID_SCL;
    stSclChnPort0.u32DevId = u8SclDev;
    stSclChnPort0.u32ChnId = s32SclChn;
    stSclChnPort0.u32PortId = 0x0;

    /************************************************
    step :init GPU
    *************************************************/
    uint32_t u32Width  = ALIGN_BACK(g_stDrmCfg.width,16);
    uint32_t u32Height = g_stDrmCfg.height;
    uint32_t u32GpuInputFormat = DRM_FORMAT_NV12;

    g_RectDisplayRegion.left = 0;
    g_RectDisplayRegion.top = 0;
    g_RectDisplayRegion.right = u32Width;
    g_RectDisplayRegion.bottom = u32Height;

    int ret = g_stdGpuGfx->init(u32Width, u32Height, u32GpuInputFormat);
    if (ret) {
        printf("Failed to init gpu graphic effect\n");
        return NULL;
    }

    STUB_ResetAllpointOffset();

    struct timeval tv;
    MI_S32 s32FdSclPort0 = -1;
    fd_set ReadFdsSclPort0;

    if (MI_SUCCESS != (s32Ret = MI_SYS_GetFd(&stSclChnPort0, &s32FdSclPort0)))
    {
        printf("GET scl Fd Failed , scl Chn = %d\n", s32SclChn);
        return NULL;
    }
    printf("Get scl Fd = %d, scl Chn = %d\n", s32FdSclPort0, s32SclChn);

    MI_BOOL bGetBufListGfx = 0;

    std::shared_ptr<GpuGraphicBuffer> GfxInputBuffer;
    std::shared_ptr<GpuGraphicBuffer> GfxOutputBuffer;

    ST_GfxInputBuf_Node *GfxReadListNode;
    while(g_bThreadExitGfx == TRUE)
    {
        bGetBufListGfx = 0;

        pthread_mutex_lock(&ListMutexGfx);
        if(!(list_empty(&g_HeadListGpuGfxInput)))
        {
            GfxReadListNode = list_first_entry(&g_HeadListGpuGfxInput, ST_GfxInputBuf_Node, list);
            GfxInputBuffer = GfxReadListNode->pstdInputBuffer;

            memcpy(&stDmaBufInfo, &GfxReadListNode->stDmaBufInfo, sizeof(MI_SYS_DmaBufInfo_t));
            bGetBufListGfx = 1;
            list_del(&(GfxReadListNode->list));
            delete GfxReadListNode;
        }
        pthread_mutex_unlock(&ListMutexGfx);

        if(bGetBufListGfx)
        {
            stDmaBuf_t OutputBuf;
            drm_buf_t drm_buf;
            FD_ZERO(&ReadFdsSclPort0);
            FD_SET(s32FdSclPort0, &ReadFdsSclPort0);
            tv.tv_sec = 2;

            s32Ret = select(s32FdSclPort0 + 1, &ReadFdsSclPort0, NULL, NULL, &tv);
            if (s32Ret < 0)
            {
                printf("gfx_bgb_Task%d select failed\n", s32SclChn);
                sleep(1);
                break;
            }
            else if (0 == s32Ret)
            {
                continue;
            }
            else
            {
                s32Ret = MI_SYS_ChnOutputPortDequeueDmabuf(&stSclChnPort0, &stDmaOutputBufInfo2);
                if(stDmaOutputBufInfo2.u32Status == MI_SYS_DMABUF_STATUS_INVALID)
                {
                    printf("SclOutputPort DequeueDmabuf statu is INVALID\n");
                    //s32Ret = MI_SYS_ChnOutputPortDropDmabuf(&stSclChnPort0, &stDmaBufInfo);
                    s32Ret = MI_SYS_ChnOutputPortDropDmabuf(&stSclChnPort0, &stDmaOutputBufInfo2);
                    if(MI_SUCCESS != s32Ret)
                    {
                        printf("failed, drop dma-buf return fail\n");
                    }
                    GfxInputBuffer = nullptr;
                    continue;
                }
                if(stDmaOutputBufInfo2.u32Status == MI_SYS_DMABUF_STATUS_DROP)
                {
                    printf("SclOutputPort DequeueDmabuf statu is DROP\n");
                    GfxInputBuffer = nullptr;
                    continue;
                }
                else
                {
                    //printf("SclOutputPort DequeueDmabuf statu is Done\n");
                }
                int ret = g_stdGpuGfx->process(GfxInputBuffer, g_RectDisplayRegion, GfxOutputBuffer);
                if (ret) {
                    printf("Gpu graphic effect process failed\n");

                }

                GfxInputBuffer = nullptr;
                u8EnqueueDmabufCnt--;

                OutputBuf.width   = GfxOutputBuffer->getWidth();
                OutputBuf.height  = GfxOutputBuffer->getHeight();
                OutputBuf.format  = E_STREAM_YUV420;
                OutputBuf.fds[0]  = GfxOutputBuffer->getFd();
                OutputBuf.fds[1]  = GfxOutputBuffer->getFd();
                OutputBuf.stride[0] = GfxOutputBuffer->getWidth();
                OutputBuf.stride[1] = GfxOutputBuffer->getWidth();
                OutputBuf.dataOffset[0] = 0;
                OutputBuf.dataOffset[1] = OutputBuf.width * OutputBuf.height;
                OutputBuf.priavte = (void *)&drm_buf;

                s32Ret = AddDmabufToDRM(&OutputBuf);
                if (s32Ret != 0)
                {
                    //printf("AddDmabufToDRM:%d\n", s32Ret);
                }
                atomic_set_plane(MOPG_ID0, drm_buf.fb_id, 0);
                drm_free_gem_handle(&prev_mop_gem_handle);
                prev_mop_gem_handle.first = drm_buf.fb_id;
                prev_mop_gem_handle.second = drm_buf.gem_handle[0];

            }
        }
    }
    g_stdGpuGfx->deinit();
    MI_SYS_CloseFd(s32FdSclPort0);
    FD_ZERO(&ReadFdsSclPort0);

    return NULL;
}



static void * sstar_player_thread(void *args)
{
    int ret;
    char     IqApiBinFilePath[128];
    strcpy(IqApiBinFilePath, g_InputFile);

    //_g_MediaPlayer.player_working = true;
    while (g_bThreadExitPlayer == TRUE) {
        ret = mm_player_get_status();
        if (ret < 0) {
            printf("mmplayer has been closed!\n");
            _g_MediaPlayer.player_working = false;
            continue;
        }
        if (ret & AV_PLAY_ERROR) {
            mm_player_close();
            _g_MediaPlayer.player_working = false;
            g_bThreadExitPlayer = false;
            printf("mmplayer AV_PLAY_ERROR!\n");
        }
        else if ((ret & AV_PLAY_COMPLETE))
        {
            _g_MediaPlayer.player_working = false;
            ret = mm_player_close();
            if (ret < 0)
            {
                printf("[%s %d]mm_player_close fail!\n", __FUNCTION__, __LINE__);
            }
            printf("restart \n");

            //mm_player_set_opts("video_bypass", "", 0);
            //mm_player_set_opts("video_rotate", "", _g_MediaPlayer.video_info.rotate);
            //mm_player_register_event_cb(mm_video_event_handle);
            sstar_player_init();

            ret = mm_player_open(&_g_MediaPlayer.param);
            if (ret < 0)
            {
                g_bThreadExitPlayer = false;
            }
            _g_MediaPlayer.player_working = true;

        }
        usleep(10 * 1000);
    }
    printf("[%s %d]exit!\n", __FUNCTION__, __LINE__);

    return NULL;
}


static void * sstar_video_thread(void * arg)
{
    int ret;
    int i = 0;
    frame_info_t frame_info;
    MI_SYS_BUF_HANDLE hSclHandle = 0;
    MI_SYS_BufConf_t stSclBufConf;
    MI_SYS_BufInfo_t stSclBufInfo;
    MI_SYS_ChnPort_t stSclChnInput;

    struct timeval stTv;

    memset(&stSclChnInput, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stSclBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    memset(&stSclBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

    stSclChnInput.eModId = E_MI_MODULE_ID_SCL;
    stSclChnInput.u32DevId = 1;
    stSclChnInput.u32ChnId = 0;
    stSclChnInput.u32PortId = 0;

    while (g_bThreadExitVideo == TRUE)
    {
        if( _g_MediaPlayer.player_working == 1)
        {
            ret = mm_player_get_video_frame(&frame_info);
            if (ret < 0) {
                usleep(10 * 1000);
                continue;
            }
            if (frame_info.format == AV_PIXEL_FMT_NV12)
            {
                stSclBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
                gettimeofday(&stTv, NULL);
                stSclBufConf.u64TargetPts = stTv.tv_sec*1000000 + stTv.tv_usec;
                stSclBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
                stSclBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                stSclBufConf.stFrameCfg.u16Width = frame_info.width;
                stSclBufConf.stFrameCfg.u16Height = frame_info.height;


                ret = MI_SYS_ChnInputPortGetBuf(&stSclChnInput,&stSclBufConf,&stSclBufInfo,&hSclHandle,0);
                if(ret !=0 )
                {
                    printf("MI_SYS_ChnInputPortGetBuf error!\n");
                }
                for(i=0 ;i < frame_info.height;i++)
                {
                    memcpy(stSclBufInfo.stFrameData.pVirAddr[0] + (i*stSclBufInfo.stFrameData.u32Stride[0]), frame_info.data[0] + (i*frame_info.stride[0]), stSclBufInfo.stFrameData.u32Stride[0]);
                }
                for(i=0 ;i < frame_info.height / 2;i++)
                {
                    memcpy(stSclBufInfo.stFrameData.pVirAddr[1] + (i*stSclBufInfo.stFrameData.u32Stride[1]), frame_info.data[1] + (i*frame_info.stride[1]), stSclBufInfo.stFrameData.u32Stride[1]);
                }
                ret = MI_SYS_ChnInputPortPutBuf(hSclHandle,&stSclBufInfo, FALSE);
                if(ret !=0 )
                {
                    printf("MI_SYS_ChnInputPortPutBuf error!\n");
                }
                g_u32SclBufferCnt++;
                //printf("scl input buffer ok!\n");
            }
            mm_player_put_video_frame();
        }
        else
        {
            //printf("===player is closed==cnt:%d===\n",cnt);
            continue;
        }
    }
    printf("[%s %d]exit!\n", __FUNCTION__, __LINE__);
    return NULL;
}



static MI_S32 sstar_init_drm()
{
    int ret;
    /************************************************
    step :init DRM(DISP)
    *************************************************/

    g_stDrmCfg.fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if(g_stDrmCfg.fd < 0)
    {
        printf("Open dri/card0 fail \n");
        return -1;
    }

    ret = drmSetClientCap(g_stDrmCfg.fd, DRM_CLIENT_CAP_ATOMIC, 1);
    if(ret != 0)
    {
        printf("drmSetClientCap DRM_CLIENT_CAP_ATOMIC fail ret=%d  \n",ret);
    }

    ret = drmSetClientCap(g_stDrmCfg.fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    if(ret != 0)
    {
        printf("drmSetClientCap DRM_CLIENT_CAP_UNIVERSAL_PLANES fail ret=%d  \n",ret);
    }

    g_stDrmCfg.pDrmModeRes = drmModeGetResources(g_stDrmCfg.fd);
    g_stDrmCfg.crtc_id = g_stDrmCfg.pDrmModeRes->crtcs[0];
    g_stDrmCfg.conn_id = g_stDrmCfg.pDrmModeRes->connectors[0];

    g_stDrmCfg.Connector = drmModeGetConnector(g_stDrmCfg.fd, g_stDrmCfg.conn_id);
	if (g_stDrmCfg.Connector->connector_type == DRM_MODE_CONNECTOR_DSI)//MIPI
	{
	    printf("This is MIPI Panel \n");
	}
    else
    {

	    printf("This is TTL Panel \n");
    }
    g_stDrmCfg.width = g_stDrmCfg.Connector->modes[0].hdisplay;
    g_stDrmCfg.height = g_stDrmCfg.Connector->modes[0].vdisplay;

    init_drm_property_ids(GOP_UI_ID, &g_stDrmCfg.drm_mode_prop_ids[0]);
    init_drm_property_ids(MOPG_ID0, &g_stDrmCfg.drm_mode_prop_ids[1]);

    return 0;

}

static void sstar_deinit_drm()
{

    if(g_stDrmCfg.Connector != NULL)
    {
    	drmModeFreeConnector(g_stDrmCfg.Connector);
        g_stDrmCfg.Connector = NULL;
    }
    if(g_stDrmCfg.pDrmModeRes != NULL)
    {
    	drmModeFreeResources(g_stDrmCfg.pDrmModeRes);
        g_stDrmCfg.pDrmModeRes = NULL;
    }

    //destory property blob
    if(g_stDrmCfg.blob_id)
    {
        drmModeDestroyPropertyBlob(g_stDrmCfg.fd, g_stDrmCfg.blob_id);
    }

    if(g_stDrmCfg.fd)
    {
        close(g_stDrmCfg.fd);
    }

}



static MI_S32 STUB_PipelineMode0Init()
{
    MI_S32 ret = MI_SUCCESS;

    uint32_t frame_rate = 60;

    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
    MI_SYS_BindType_e eBindType;
    MI_U32 u32BindParam;

    /************************************************
    step :init HDMI_RX
    *************************************************/
    MI_HDMIRX_PortId_e u8HdmirxPort = E_MI_HDMIRX_PORT0;
    MI_HDMIRX_Hdcp_t stHdmirxHdcpKey;
    MI_HDMIRX_Edid_t stHdmirxEdid;

    MI_HDMIRX_Init();

    stHdmirxHdcpKey.u64HdcpDataVirAddr = (MI_U64)HDCP_KEY;
    stHdmirxHdcpKey.u32HdcpLength = sizeof(HDCP_KEY);
    ret = MI_HDMIRX_LoadHdcp(u8HdmirxPort, &stHdmirxHdcpKey);
    if (ret != MI_SUCCESS) {
        printf("MI_HDMIRX_LoadHdcp error: %d\n", ret);
    }

    stHdmirxEdid.u64EdidDataVirAddr = (MI_U64)EDID_Test;
    stHdmirxEdid.u32EdidLength = 256;
    ret = MI_HDMIRX_UpdateEdid(u8HdmirxPort, &stHdmirxEdid);
    if (ret != MI_SUCCESS) {
        printf("MI_HDMIRX_UpdateEdid error: %d\n", ret);
    }

    ret = MI_HDMIRX_Connect(u8HdmirxPort);
    if (ret != MI_SUCCESS) {
        printf("MI_HDMIRX_Connect error: %d\n", ret);
        return ret;
    }
    printf("MI HdmiRx Init done\n");

    /************************************************
    step :init HVP
    *************************************************/
    MI_HVP_DEV u32HvpDevId = 0;
    MI_HVP_CHN u32HvpChnId = 0;
    MI_HVP_DeviceAttr_t stHvpDevAttr;
    MI_HVP_ChannelAttr_t stHvpChnAttr;
    MI_HVP_ChannelParam_t stHvpChnParam;


    memset(&stHvpDevAttr, 0, sizeof(MI_HVP_DeviceAttr_t));
    stHvpDevAttr.enSrcType = (MI_HVP_SourceType_e)E_MI_HVP_SRC_TYPE_HDMI;
    ret = MI_HVP_CreateDevice(u32HvpDevId, &stHvpDevAttr);
    if (ret != MI_SUCCESS) {
        printf("MI_HVP_CreateDevice error_ret:%d\n", ret);
        return ret;
    }

    stHvpChnAttr.enFrcMode = (MI_HVP_FrcMode_e)E_MI_HVP_FRC_MODE_FBL;

    stHvpChnAttr.stPqBufModeConfig.u16BufMaxCount = 1;
    stHvpChnAttr.stPqBufModeConfig.u16BufMaxCount = ceil(frame_rate / 60) + 1;

    printf("[HVP]pqbuf=%d\n", stHvpChnAttr.stPqBufModeConfig.u16BufMaxCount);

    stHvpChnAttr.stPqBufModeConfig.eDmaColor        = E_MI_HVP_COLOR_FORMAT_YUV444;
    stHvpChnAttr.stPqBufModeConfig.u16BufMaxWidth   = VEDIO_WIDTH;
    stHvpChnAttr.stPqBufModeConfig.u16BufMaxHeight  = VEDIO_HEIGHT;
    stHvpChnAttr.stPqBufModeConfig.eBufCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stHvpChnAttr.stPqBufModeConfig.eFieldType       = E_MI_SYS_FIELDTYPE_NONE;

    ret = MI_HVP_CreateChannel(u32HvpDevId, u32HvpChnId, &stHvpChnAttr);
    if (ret != MI_SUCCESS) {
        printf("MI_HVP_CreateChannel error:%d\n",ret);
        return ret;//err
    }

    memset(&stHvpChnParam, 0x0, sizeof(MI_HVP_ChannelParam_t));
    stHvpChnParam.stChnSrcParam.enPixRepType = E_MI_HVP_PIX_REP_TYPE_1X;
    stHvpChnParam.stChnSrcParam.enInputColor = E_MI_HVP_COLOR_FORMAT_RGB444;
    stHvpChnParam.stChnSrcParam.enColorDepth = E_MI_HVP_COLOR_DEPTH_8;
    stHvpChnParam.stChnSrcParam.enColorRange = E_MI_HVP_COLOR_RANGE_TYPE_FULL;
    stHvpChnParam.stChnSrcParam.stCropWin.u16X = 0;
    stHvpChnParam.stChnSrcParam.stCropWin.u16Y = 0;
    stHvpChnParam.stChnSrcParam.stCropWin.u16Width = VEDIO_WIDTH;
    stHvpChnParam.stChnSrcParam.stCropWin.u16Height = VEDIO_HEIGHT;

    stHvpChnParam.stChnDstParam.bFlip = 0;
    stHvpChnParam.stChnDstParam.bMirror = 0;
    stHvpChnParam.stChnDstParam.enColor = E_MI_HVP_COLOR_FORMAT_YUV444;
    stHvpChnParam.stChnDstParam.u16Fpsx100 = 60;
    stHvpChnParam.stChnDstParam.u16Width = VEDIO_WIDTH;
    stHvpChnParam.stChnDstParam.u16Height = VEDIO_HEIGHT;
    stHvpChnParam.stChnDstParam.stDispWin.u16X = 0;
    stHvpChnParam.stChnDstParam.stDispWin.u16Y = 0;
    stHvpChnParam.stChnDstParam.stDispWin.u16Width = VEDIO_WIDTH;
    stHvpChnParam.stChnDstParam.stDispWin.u16Height = VEDIO_HEIGHT;
    stHvpChnParam.stChnDstParam.stCropWin.u16X = 0;
    stHvpChnParam.stChnDstParam.stCropWin.u16Y = 0;
    stHvpChnParam.stChnDstParam.stCropWin.u16Width = VEDIO_WIDTH;
    stHvpChnParam.stChnDstParam.stCropWin.u16Height = VEDIO_HEIGHT;
    ret = MI_HVP_SetChannelParam(u32HvpDevId, u32HvpChnId, &stHvpChnParam);
    if (MI_SUCCESS != ret)
    {
        return ret;
    }

    ret = MI_HVP_StartChannel(u32HvpDevId, u32HvpChnId);
    if (ret != MI_SUCCESS) {
        printf("MI_HVP_CreateChannel error\n");
        return ret;//err
    }

    hvp_event_thread_running = true;
    pthread_create(&hvp_event_thread, NULL, signal_monitor_hdmi, NULL);

    printf("MI Hvp Init done\n");

    /************************************************
    step :init SCL
    *************************************************/
    sstar_scl_init();

    /************************************************
    step :bind HVP -> SCL
    *************************************************/
    MI_U32 u32SclDevId = 8;
    MI_U32 u32SclChnId = 0;
    MI_U32 u32SclPortId = 0;

    memset(&stSrcChnPort, 0x00, sizeof(MI_SYS_ChnPort_t));
    memset(&stDstChnPort, 0x00, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_HVP;
    stSrcChnPort.u32DevId = u32HvpDevId;
    stSrcChnPort.u32ChnId = u32HvpChnId;
    stSrcChnPort.u32PortId = 0;
    stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
    stDstChnPort.u32DevId = u32SclDevId;
    stDstChnPort.u32ChnId = u32SclChnId;
    stDstChnPort.u32PortId =u32SclPortId;
    u32SrcFrmrate = 60;
    u32DstFrmrate = 60;
    eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    u32BindParam = 0;
    STCHECKRESULT(MI_SYS_BindChnPort2(0, &stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate, eBindType, u32BindParam));

    //STCHECKRESULT(MI_SYS_CreateChnOutputPortDmabufCusAllocator(&stDstChnPort));

    pthread_create(&g_pThreadScl, NULL, ST_SCL_Task, NULL);
    pthread_create(&g_pThreadGfx, NULL, ST_GFX_Task, NULL);
    if(bShowUi)
    {
        pthread_create(&g_pThreadUiDrm, NULL, ST_UI_Task, NULL);
    }

    return MI_SUCCESS;
}//STUB_PipelineMode0Init end


static MI_S32 STUB_PipelineMode1Init()
{
    int ret;
    sstar_player_init();
    sstar_scl_init();

    ret = mm_player_open(&_g_MediaPlayer.param);
    if (ret < 0)
    {
        printf("mm_player_open fail \n");
        return -1;
    }
    _g_MediaPlayer.player_working = true;

    pthread_create(&g_player_thread, NULL, sstar_player_thread, NULL);
    pthread_create(&g_video_thread, NULL, sstar_video_thread, NULL);

    while(g_u32SclBufferCnt == 0)
    {
        usleep(10);
        //printf("Scl do not have buffer!!!!!\n");
    }
    pthread_create(&g_pThreadScl, NULL, ST_SCL_Task, NULL);
    pthread_create(&g_pThreadGfx, NULL, ST_GFX_Task, NULL);

    if(bShowUi)
    {
        pthread_create(&g_pThreadUiDrm, NULL, ST_UI_Task, NULL);
    }

    return MI_SUCCESS;
}


static MI_S32 STUB_PipelineMode0Deinit()
{
    int ret = -1;
    MI_HDMIRX_PortId_e u8HdmirxPort = E_MI_HDMIRX_PORT0;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_HVP_DEV u32HvpDevId = 0;
    MI_HVP_CHN u32HvpChnId = 0;
    MI_U32 u32SclDevId = 8;
    MI_U32 u32SclChnId = 0;
    MI_U32 u32SclPortId = 0;
    /************************************************
    step :unbind HVP -> SCL
    *************************************************/
    memset(&stSrcChnPort, 0x00, sizeof(MI_SYS_ChnPort_t));
    memset(&stDstChnPort, 0x00, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_HVP;
    stSrcChnPort.u32DevId = u32HvpDevId;
    stSrcChnPort.u32ChnId = u32HvpChnId;
    stSrcChnPort.u32PortId = 0;
    stDstChnPort.eModId = E_MI_MODULE_ID_SCL;
    stDstChnPort.u32DevId = u32SclDevId;
    stDstChnPort.u32ChnId = u32SclChnId;
    stDstChnPort.u32PortId =u32SclPortId;
    STCHECKRESULT(MI_SYS_UnBindChnPort(0, &stSrcChnPort, &stDstChnPort));

    /************************************************
    step :deinit SCL
    *************************************************/
    sstar_scl_deinit();


    /************************************************
    step :deinit HVP
    *************************************************/
    hvp_event_thread_running = false;
    pthread_join(hvp_event_thread, NULL);

    ret = MI_HVP_StopChannel(u32HvpDevId, u32HvpChnId);
    if (ret != MI_SUCCESS) {
        printf("MI_HVP_StopChannel error");
        return ret;
    }

    ret = MI_HVP_DestroyChannel(u32HvpDevId, u32HvpChnId);
    if (ret != MI_SUCCESS) {
        printf("MI_HVP_DestroyChannel error");
        return ret;
    }

    ret = MI_HVP_DestroyDevice(u32HvpDevId);
    if (ret != MI_SUCCESS) {
        printf("MI_HVP_DestroyDevice error");
        return ret;
    }

    /************************************************
    step :deinit HDMI_RX
    *************************************************/
    ret = MI_HDMIRX_DeInit();
    if (ret != MI_SUCCESS) {
        printf("DeInit CEC error");
    }
    ret = MI_HDMIRX_DisConnect(u8HdmirxPort);
    if (ret != MI_SUCCESS) {
        printf("Deinit hdmirx fail");
        return ret;
    }
    return MI_SUCCESS;
}


static MI_S32 STUB_PipelineMode1Deinit()
{
    g_bThreadExitPlayer = false;
    pthread_join(g_player_thread, NULL);
    g_bThreadExitVideo = false;
    pthread_join(g_video_thread, NULL);
    mm_player_close();

    /************************************************
    step :deinit SCL
    *************************************************/
    sstar_scl_deinit();

    return MI_SUCCESS;

}

static MI_S32 STUB_BaseModuleInit()
{
    int ret = 0;
    /************************************************
    step :init SYS
    *************************************************/
    MI_U64           u64Pts = 0;
    MI_SYS_Version_t stVersion;
    struct timeval   curStamp;
    STCHECKRESULT(MI_SYS_Init(0));
    STCHECKRESULT(MI_SYS_GetVersion(0, &stVersion));
    printf("Get MI_SYS_Version:%s\n", stVersion.u8Version);

    gettimeofday(&curStamp, NULL);
    u64Pts = (curStamp.tv_sec) * 1000000ULL + curStamp.tv_usec;
    STCHECKRESULT(MI_SYS_InitPtsBase(0, u64Pts));

    gettimeofday(&curStamp, NULL);
    u64Pts = (curStamp.tv_sec) * 1000000ULL + curStamp.tv_usec;
    STCHECKRESULT(MI_SYS_SyncPts(0, u64Pts));

    STCHECKRESULT(MI_SYS_GetCurPts(0, &u64Pts));
    printf("%s:%d Get MI_SYS_CurPts:0x%llx(%ds,%dus)\n", __func__, __LINE__, u64Pts, (int)curStamp.tv_sec,
           (int)curStamp.tv_usec);
    //***********************************************
    //end init sys

    ret = sstar_init_drm();
    if(ret != 0)
    {
        printf("init_drm fail \n");
        return -1;
    }

    if (g_u8PipelineMode == 0)
    {
        STUB_PipelineMode0Init();
        //HDMI_RX -> HVP -> SCL
    }
    else if (g_u8PipelineMode == 1)
    {
        STUB_PipelineMode1Init();
        //File -> FFMPEG -> SCL
    }


    return MI_SUCCESS;
}


static MI_S32 STUB_BaseModuleDeinit()
{
    g_bThreadExitScl = false;
    pthread_join(g_pThreadScl, NULL);

    g_bThreadExitGfx = false;
    pthread_join(g_pThreadGfx, NULL);

    if(bShowUi)
    {
        g_bThreadExitUiDrm = false;
        pthread_join(g_pThreadUiDrm, NULL);
    }

    sstar_deinit_drm();

    if (g_u8PipelineMode == 0)
    {
        STUB_PipelineMode0Deinit();
        //HDMI_RX -> HVP -> SCL
    }//g_u8PipelineMode 0 end
    else if (g_u8PipelineMode == 1)
    {
        STUB_PipelineMode1Deinit();
        //File -> FFMPEG -> SCL
    }//g_u8PipelineMode 1 end

    STCHECKRESULT(MI_SYS_Exit(0));

    return MI_SUCCESS;

}


void usage_help(void)
{
    printf("Usage:./prog_keystone -m 0 -u xxx -w xxx -h xxx ) HDMI_RX->HVP->SCL->GPU->DRM\n");
    printf("Usage:./prog_keystone -m 1 -i xxx -u xxx -w xxx -h xxx ) FILE->FFMPEG->SCL->GPU->DRM\n");
}

MI_S32 parse_args(int argc, char **argv)
{
    MI_BOOL bInputVedioFile = false;
    g_u8PipelineMode = 0xff;
    for (int i = 0; i < argc; i++)
    {


        if (0 == strcmp(argv[i], "-m"))
        {
            g_u8PipelineMode = atoi(argv[i+1]);
            if((g_u8PipelineMode != 0) && (g_u8PipelineMode != 1))
            {
                printf("<error>[-m]:Only supports mode0 and mode 1\n");
                return -1;
            }
        }
        if (0 == strcmp(argv[i], "-i"))
        {
            strcpy(g_InputFile, argv[i + 1]);
            if(g_u8PipelineMode != 1)
            {
                printf("<error>[-i]:Only mode 1 requires inputting the corresponding video file\n");
                return -1;
            }
            else
            {
                bInputVedioFile = true;
            }
        }
        if (0 == strcmp(argv[i], "-r"))
        {
            _g_MediaPlayer.video_info.rotate = atoi(argv[i+1]);
            if(_g_MediaPlayer.video_info.rotate > 3)
            {
                printf("<error>[-r]:Only supports 0-3\n");

                return -1;
            }
        }
        if (0 == strcmp(argv[i], "-u"))
        {
            bShowUi = 1;
            strcpy(g_InputUiPath, argv[i + 1]);
        }
        if (0 == strcmp(argv[i], "-w"))
        {
            if(bShowUi == 0)
            {
                printf("<error>[-w]:should first input - u to select the input UI image\n");
                return -1;
            }
            _g_MediaPlayer.ui_info.in_width = atoi(argv[i+1]);  //UI SRC SIZE

        }
        if (0 == strcmp(argv[i], "-h"))
        {
            if(bShowUi == 0)
            {
                printf("<error>[-h]:should first input - u to select the input UI image\n");
                return -1;
            }
            _g_MediaPlayer.ui_info.in_height = atoi(argv[i+1]);  //UI SRC SIZE
        }

    }
    if(g_u8PipelineMode == 1)
    {
        if(bInputVedioFile != true)
        {
            printf("<error>[-i]:mode 1 requires input of video stream file\n");
            return -1;
        }
    }
    if(bShowUi)
    {
        if((_g_MediaPlayer.ui_info.in_height == 0) || (_g_MediaPlayer.ui_info.in_width == 0))
        {
            printf("<error>[-u]:Should Input '-w/-h' To Set UI Image Size\n");
            return -1;
        }
    }
    return MI_SUCCESS;
}

void STUB_Common_Pause(void)
{
    char ch;
    MI_U16 u16GpuMaxW = g_stDrmCfg.width / 3;
    MI_U16 u16GpuMaxH = g_stDrmCfg.height / 3;
    printf("press q to exit\n");
    while ((ch = getchar()) != 'q')
    {
        if(ch == 'q')
        {
            break;
        }
        if(ch == 'a')
        {
            printf("======== GpuLT[+] =======\n");
            if((g_u32GpuLT_X + 10 >= u16GpuMaxW) || (g_u32GpuLT_Y + 10 >= u16GpuMaxH))
            {
                printf("Set Gpu PointOffse Fail, the PointOffse can not More than disp w/3 h/3 !!!\n");
            }
            else
            {
                    g_u32GpuLT_X = g_u32GpuLT_X + 10;
                    g_u32GpuLT_Y = g_u32GpuLT_Y + 10;
                    g_bGpuInfoChange_mop = true;
            }
        }
        if(ch == 'b')
        {
            printf("======== GpuLT[-] =======\n");
            if(g_u32GpuLT_X < 10)
            {
                g_u32GpuLT_X = 0;
                g_u32GpuLT_Y = 0;
                printf("Set Gpu PointOffse Fail, the PointOffse can not Less than zero !!!\n");
            }
            else
            {
                g_u32GpuLT_X = g_u32GpuLT_X - 10;
                g_u32GpuLT_Y = g_u32GpuLT_Y - 10;
                g_bGpuInfoChange_mop = true;
            }
        }
        if(ch == 'c')
        {
            printf("======== GpuLB[+] =======\n");
            if((g_u32GpuLB_X + 10 >= u16GpuMaxW) || (g_u32GpuLB_Y + 10 >= u16GpuMaxH))
            {
                printf("Set Gpu PointOffse Fail, the PointOffse can not More than disp w/3 h/3 !!!\n");
            }
            else
            {
                    g_u32GpuLB_X = g_u32GpuLB_X + 10;
                    g_u32GpuLB_Y = g_u32GpuLB_Y + 10;
                    g_bGpuInfoChange_mop = true;
            }
        }
        if(ch == 'd')
        {
            printf("======== GpuLB[-] =======\n");
            if(g_u32GpuLB_X < 10)
            {
                g_u32GpuLB_X = 0;
                g_u32GpuLB_Y = 0;
                printf("Set Gpu PointOffse Fail, the PointOffse can not Less than zero !!!\n");
            }
            else
            {
                g_u32GpuLB_X = g_u32GpuLB_X - 10;
                g_u32GpuLB_Y = g_u32GpuLB_Y - 10;
                g_bGpuInfoChange_mop = true;
            }
        }
        if(ch == 'e')
        {
            printf("======== GpuRT[+] =======\n");
            if((g_u32GpuRT_X + 10 >= u16GpuMaxW) || (g_u32GpuRT_Y + 10 >= u16GpuMaxH))
            {
                printf("Set Gpu PointOffse Fail, the PointOffse can not More than disp w/3 h/3 !!!\n");
            }
            else
            {
                    g_u32GpuRT_X = g_u32GpuRT_X + 10;
                    g_u32GpuRT_Y = g_u32GpuRT_Y + 10;
                    g_bGpuInfoChange_mop = true;
            }
        }
        if(ch == 'f')
        {
            printf("======== GpuRT[-] =======\n");
            if(g_u32GpuRT_X < 10)
            {
                g_u32GpuRT_X = 0;
                g_u32GpuRT_Y = 0;
                printf("Set Gpu PointOffse Fail, the PointOffse can not Less than zero !!!\n");
            }
            else
            {
                g_u32GpuRT_X = g_u32GpuRT_X - 10;
                g_u32GpuRT_Y = g_u32GpuRT_Y - 10;
                g_bGpuInfoChange_mop = true;
            }
        }
        if(ch == 'g')
        {
            printf("======== GpuRB[+] =======\n");
            if((g_u32GpuRB_X + 10 >= u16GpuMaxW) || (g_u32GpuRB_Y + 10 >= u16GpuMaxH))
            {
                printf("Set Gpu PointOffse Fail, the PointOffse can not More than disp w/3 h/3 !!!\n");
            }
            else
            {
                    g_u32GpuRB_X = g_u32GpuRB_X + 10;
                    g_u32GpuRB_Y = g_u32GpuRB_Y + 10;
                    g_bGpuInfoChange_mop = true;
            }
        }
        if(ch == 'h')
        {
            printf("======== GpuRB[-] =======\n");
            if(g_u32GpuRB_X < 10)
            {
                g_u32GpuRB_X = 0;
                g_u32GpuRB_Y = 0;
                printf("Set Gpu PointOffse Fail, the PointOffse can not Less than zero !!!\n");
            }
            else
            {
                g_u32GpuRB_X = g_u32GpuRB_X - 10;
                g_u32GpuRB_Y = g_u32GpuRB_Y - 10;
                g_bGpuInfoChange_mop = true;
            }
        }

        printf("press q to exit\n");
        usleep(10 * 1000);
    }
}


MI_S32 main(int argc, char **argv)
{
    int ret;
    g_u32GpuLT_X = 0;
    g_u32GpuLT_Y = 0;

    g_u32GpuLB_X = 0;
    g_u32GpuLB_Y = 0;

    g_u32GpuRT_X = 0;
    g_u32GpuRT_Y = 0;

    g_u32GpuRB_X = 0;
    g_u32GpuRB_Y = 0;

    memset(&_g_MediaPlayer, 0, sizeof(media_player_t));

    ret = parse_args(argc, argv);
    if(ret != MI_SUCCESS)
    {
        printf("<error>:Get Invalid Parameter,Please Check The Parameters!\n");
        usage_help();
        return -1;
    }

    STCHECKRESULT(STUB_BaseModuleInit());

    STUB_Common_Pause();

    STCHECKRESULT(STUB_BaseModuleDeinit());

    return 0;
}
