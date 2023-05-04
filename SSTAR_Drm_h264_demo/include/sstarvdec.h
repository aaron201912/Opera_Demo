#ifndef _SSTARVDEC_H_
#define _SSTARVDEC_H_
#include "common.h"
#include "mi_sys.h"

#if defined (__cplusplus)
    extern "C" {
#endif


#define MAX_CHANNEL_NUM (64)
#define MAX_DEVICE_NUM (2)

#define VDEC_MAX_WIDTH 1920
#define VDEC_MAX_HEIGHT 1080

#define VDEC_OUTPUT_WIDTH 720
#define VDEC_OUTPUT_HEIGHT 1280

#define ExecFunc(_func_, _ret_)                         \
    if (_func_ != _ret_)                                \
    {                                                   \
        printf("[%s][%d]exec function failed\n", __FUNCTION__, __LINE__); \
        return -1;                                   \
    }                                                   \
    else                                                \
    {                                                   \
        printf("[%s](%d)exec function pass\n", __FUNCTION__, __LINE__);   \
    }


typedef struct ReOrderSlice_s {
    unsigned char *pos;
    unsigned int len;
} ReOrderSlice_t;

typedef struct
{
    int startcodeprefix_len;
    unsigned int len;
    unsigned int max_size;
    char *buf;
    unsigned short lost_packets;
} NALU_t;


int sstar_vdec_init(vdec_info_t* vdec_info, void *video_file);

int sstar_vdec_enqueueOneBuffer(int32_t dev, int32_t chn, MI_SYS_DmaBufInfo_t* mi_dma_buf_info);
int sstar_parse_video_info(char *filename, MI_S32 *width, MI_S32 *height, int *type);
int sstar_vdec_deinit();


#if defined (__cplusplus)
#endif

#endif

