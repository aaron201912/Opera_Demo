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
#ifndef __HDMI_PLAYER_H__
#define __HDMI_PLAYER_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* audio output device, select by @hdmi_player_select_audio_output_device */
typedef enum {
    E_HDMI_PLAYER_AO_DEV_AUTO = 0,      // speaker and spdif devices output audio simultaneously
    E_HDMI_PLAYER_AO_DEV_SPEAKER,       // speaker devices output audio
    E_HDMI_PLAYER_AO_DEV_HEADPHONE,     // headphone devices output audio
    E_HDMI_PLAYER_AO_DEV_SPDIF,         // spdif devices output audio
    E_HDMI_PLAYER_AO_DEV_HDMI_ARC,      // hdmi-arc devices output audio
    E_HDMI_PLAYER_AO_DEV_MAX,
} hdmi_player_ao_dev_e;

/* audio output sound format, select by @hdmi_player_select_audio_output_device */
typedef enum {
    E_HDMI_PLAYER_AO_FORMAT_PCM = 0,   // pcm format
    E_HDMI_PLAYER_AO_FORMAT_RAW,       // raw format
    E_HDMI_PLAYER_AO_FORMAT_MAX,
} hdmi_player_ao_format_e;

/* Input and output audio sample rate */
typedef enum {
    E_HDMI_PLAYER_AIO_SAMPLE_RATE_ORIGINAL = 0, // original sample rate for hdmi rx input
    E_HDMI_PLAYER_AIO_SAMPLE_RATE_16K,          // 16kHz
    E_HDMI_PLAYER_AIO_SAMPLE_RATE_32K,          // 32kHz
    E_HDMI_PLAYER_AIO_SAMPLE_RATE_44K,          // 44kHz
    E_HDMI_PLAYER_AIO_SAMPLE_RATE_48K,          // 48kHz
    E_HDMI_PLAYER_AIO_SAMPLE_RATE_MAX,
} hdmi_player_aio_sample_rate_e;

/* hdmi player notification events */
typedef enum {
    E_HDMI_PLAYER_EVENT_UNSUPPORTED = 0, // exceeds player capabilities
    E_HDMI_PLAYER_EVENT_MAX,
} hdmi_player_event_e;

/* audio input/output frame */
typedef struct hdmi_player_pcm_frame_s {
    /* frame data buffer address */
    uint8_t *data;
    /* frame valid data size */
    int32_t size;
    /* frame data buffer size */
    int32_t buf_size;
    /* frame sample rate */
    int32_t sample_rate;
    /* frame channel count */
    int32_t channels;
    /* frame timestamp */
    int64_t pts;
} hdmi_player_audio_frame_t;

/* video frame info provided by the application to hdmiplay for avsync */
typedef struct hdmi_player_video_frame_s {
    /* frame buffer handle */
    void *buf_handle;
    /* frame timestamp */
    int64_t pts;
} hdmi_player_video_frame_t;

/* DRM information provided by the application to hdmiplay for avsync */
typedef struct hdmi_player_drm_info_s {
    /* drm device fd */
    int32_t fd;
    /* vsync refresh rate */
    float panel_refresh_rate;
} hdmi_player_drm_info_t;

/* hdmi player event monitoring callback, which requires application registration and is used to monitor events.
*  example:
*   static void hdmi_player_event_cb(hdmi_player_event_e event)
*   {
*       printf("app hdmi_player_event_cb: event:%d\n", event);
*   }
*/
typedef void (*hdmi_player_event_cb_t)(hdmi_player_event_e event);

/* hdmi player video frame drop callback, which requires application registration and is used to drop video frame
*  example:
*   void hdmi_player_video_frame_drop_cb(hdmi_player_video_frame_t frame)
*   {
*       GraphicBuffer *graphic_buffer = (GraphicBuffer *)frame.buf_handle;
*       // release or put back to pool
*   }
*/
typedef void (*hdmi_player_video_frame_drop_cb_t)(hdmi_player_video_frame_t frame);

/* hdmi player video frame flip callback, which requires application registration and is used to flip video frame
*  example:
*   void hdmi_player_video_frame_drop_cb(hdmi_player_video_frame_t frame)
*   {
*       GraphicBuffer *graphic_buffer = (GraphicBuffer *)frame.buf_handle;
*       // send to display
*   }
*/
typedef void (*hdmi_player_video_frame_flip_cb_t)(hdmi_player_video_frame_t frame);

/* hdmi player configuration parameters, users can configure it according to their needs */
typedef struct hdmi_player_config_s {
    /* audio output mute */
    bool mute;
    /* audio game mode */
    bool game_mode;
    /* audio output volume [0-100] */
    int32_t volume;
    /* audio output device */
    hdmi_player_ao_dev_e ao_dev;
    /* audio output format */
    hdmi_player_ao_format_e ao_format;
    /* drm info, users need to open drm in advance */
    hdmi_player_drm_info_t drm_info;
    /* input(hdmi-rx) audio sample rate */
    hdmi_player_aio_sample_rate_e ai_sample_rate;
    /* output(playback) audio sample rate */
    hdmi_player_aio_sample_rate_e ao_sample_rate;
    /* users can monitor player events by registering a callback function */
    hdmi_player_event_cb_t event_cb;
    /* users need to register this callback to let avsync notify the user to drop video frames */
    hdmi_player_video_frame_drop_cb_t video_frame_drop_cb;
    /* users need to register this callback to let avsync notify the user to flip video frames */
    hdmi_player_video_frame_flip_cb_t video_frame_flip_cb;
} hdmi_player_config_t;

/*
* Open hdmi player by configuring parameters
*
* Parameters
* [IN]    cfg  -  hdmi player configuration parameters
* Returns
*     Zero if successful, otherwise a negative error code.
*/
__attribute__((visibility("default"))) int32_t hdmi_player_open(hdmi_player_config_t cfg);
/*
* Close hdmi player and release resources
*
* Parameters
*     void
* Returns
*     Zero if successful, otherwise a negative error code.
*/
__attribute__((visibility("default"))) int32_t hdmi_player_close(void);
/*
* Set game mode, hw RDMA bypass to WDMA, the user does not to read
* and write pcm, and do avsync
*
* Parameters
* [IN]    enable  -  true - enable game mode, false - disable game mode
* Returns
*     Zero if successful, otherwise a negative error code.
*/
__attribute__((visibility("default"))) int32_t hdmi_player_set_game_mode(bool enable);
/*
* Query  game mode
*
* Parameters
*     void
* Returns
*     true - enable game mode, false - disable game mode
*/
__attribute__((visibility("default"))) bool hdmi_player_get_game_mode(void);
/*
* Select audio output device
*
* Parameters
* [IN]    dev          -  audio output device
* Returns
*     Zero if successful, otherwise a negative error code.
*/
__attribute__((visibility("default"))) int32_t hdmi_player_select_audio_output_device(hdmi_player_ao_dev_e dev);
/*
* Select audio output sound format
*
* Parameters
* [IN]    format -  audio output sound format
* Returns
*     Zero if successful, otherwise a negative error code.
*/
__attribute__((visibility("default"))) int32_t hdmi_player_select_audio_output_format(hdmi_player_ao_format_e format);
/*
* Adjust audio output volume
*
* Parameters
* [IN]    volume   -  volume value range [0-100]
* Returns
*     Zero if successful, otherwise a negative error code.
*/
__attribute__((visibility("default"))) int32_t hdmi_player_set_volume(int32_t volume);
/*
* Adjust audio output volume
*
* Parameters
* [IN]    mute  -  false - unmute, true - mute
* Returns
*     Zero if successful, otherwise a negative error code.
*/
__attribute__((visibility("default"))) int32_t hdmi_player_set_mute(bool mute);
/*
* Block reading pcm frame, can be terminated via @hdmi_player_close
*
* Parameters
* [OUT]    frame  -  just fill in an empty frame pointer
* Returns
*     Zero if successful, otherwise a negative error code.
*/
__attribute__((visibility("default"))) int32_t hdmi_player_get_pcm(hdmi_player_audio_frame_t **frame);
/*
*  Put back frame data to player, used with @hdmi_player_get_pcm
*
* Parameters
* [IN]    frame  -  the frame is what was read by @hdmi_player_get_pcm
* Returns
*     Zero if successful, otherwise a negative error code.
*/
__attribute__((visibility("default"))) int32_t hdmi_player_put_pcm(hdmi_player_audio_frame_t *frame);
/*
*  Write the frame processed by the user algorithm to the player for output playback
*
* Parameters
* [IN]    frame  -  The frame that is processed by the caller and allocates memory by itself
* Returns
*     Zero if successful, otherwise a negative error code.
*/
__attribute__((visibility("default"))) int32_t hdmi_player_write_pcm(hdmi_player_audio_frame_t *frame);
/*
* Send a video frame to do avsync
*
* Parameters
* [IN]    frame -  video frame, The user needs to get the video frame from scl and fill in
*                  the buffer handle and pts (system time) before sending it in.
* Returns
*     Zero if successful, otherwise a negative error code.
*/
__attribute__((visibility("default"))) int32_t hdmi_player_video_render(hdmi_player_video_frame_t *frame);

#ifdef __cplusplus
}
#endif // __cplusplu

#endif // __HDMI_PLAYER_H__

