#ifndef _SSTARVDEC_H_
#define _SSTARVDEC_H_
#include "common.h"
#include "mi_sys.h"
#include "libsync.h"
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

/**************************************************
* Init vdec and create thread to get video frame from a video file
* @param            \b IN: buffer object
* @return           \b OUT: 0:   successs
**************************************************/
int sstar_vdec_init(buffer_object_t *buf);

/**************************************************
* Stop Get video frame thread, and deinit vdec
* @param            \b IN: buffer object
* @return           \b OUT: 0:   successs
**************************************************/
int sstar_vdec_deinit(buffer_object_t *buf);

/**************************************************
* Parse video file info
* @param            \b IN: video file path name
* @param            \b IN: video width
* @param            \b IN: video height
* @param            \b IN: video tyoe
* @return           \b OUT: 0:   successs
                           -1:   fail
**************************************************/
int sstar_parse_video_info(char *filename, MI_S32 *width, MI_S32 *height, int *type);

#if defined (__cplusplus)
}
#endif

#endif

