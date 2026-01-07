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
#ifndef __PLAYER_H__
#define __PLAYER_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define AV_NOTHING          (0x0000)
#define AV_AUDIO_COMPLETE   (0x0001)
#define AV_VIDEO_COMPLETE   (0x0002)
#define AV_PLAY_PAUSE       (0x0004)
#define AV_ACODEC_ERROR     (0x0008)
#define AV_VCODEC_ERROR     (0x0010)
#define AV_NOSYNC           (0x0020)
#define AV_READ_TIMEOUT     (0x0040)
#define AV_NO_NETWORK       (0x0080)
#define AV_INVALID_FILE     (0x0100)
#define AV_AUDIO_MUTE       (0x0200)
#define AV_AUDIO_PAUSE      (0x0400)
#define AV_PLAY_LOOP        (0x0800)
#define AV_VIDEO_OVER_SPEC  (0x1000)
#define AV_AUDIO_OVER_SPEC  (0x2000)
#define AV_RUNNING          (0x4000)

#define AV_PLAY_COMPLETE    (AV_AUDIO_COMPLETE | AV_VIDEO_COMPLETE)
#define AV_PLAY_ERROR       (AV_ACODEC_ERROR | AV_VCODEC_ERROR | AV_NOSYNC | AV_READ_TIMEOUT | AV_NO_NETWORK | AV_INVALID_FILE)

enum {
    AV_MODE_ONCE,
    AV_MODE_LOOP,
};

enum {
    AV_ROTATE_NONE,
    AV_ROTATE_90,
    AV_ROTATE_180,
    AV_ROTATE_270
};

enum {
    AV_ORIGIN_MODE   = 0,
    AV_SCREEN_MODE   = 1,
    AV_SAR_4_3_MODE  = 2,
    AV_SAR_16_9_MODE = 3,
    AV_AUTO_MODE     = 4,
};

enum {
    AV_PIXEL_FMT_YUV420P    = 0,  // AV_PIX_FMT_YUV420P
    AV_PIXEL_FMT_NV12       = 23, // AV_PIX_FMT_NV12
};

enum {
    AV_PCM_FMT_U8           = 0, // AV_SAMPLE_FMT_U8,
    AV_PCM_FMT_S16          = 1, // AV_SAMPLE_FMT_S16,
    AV_PCM_FMT_S32          = 2, // AV_SAMPLE_FMT_S32,
    AV_PCM_FMT_FLT          = 3, // AV_SAMPLE_FMT_FLT,
};

/**
 * decoder event type.
 * < only used for hard decoding >
 */
enum {
    PLAYBACK_EVENT_NONE                = 0,  // no event
    PLAYBACK_EVENT_SEQCHANGE,                // sequence changed
    PLAYBACK_EVENT_DECERR,                   // decoding error
    PLAYBACK_EVENT_DROPPED,                  // frame dropped
    PLAYBACK_EVENT_DECDONE,                  // decode done, do nothing
    PLAYBACK_EVENT_OPENED,                   // player opened successfully
    PLAYBACK_EVENT_STARTED,                  // player started
    PLAYBACK_EVENT_STOPED,                   // player stoped
    PLAYBACK_EVENT_PAUSED,                   // player paused
    PLAYBACK_EVENT_RESUMED,                  // player resumed
    PLAYBACK_EVENT_STREAMEND,                // video and audio are end of stream
    PLAYBACK_EVENT_SEEKED,                   // player seeked successfully
    PLAYBACK_EVENT_BUFFERING,                // player is buffering data from network
    PLAYBACK_EVENT_BUFFERED,                 // player has buffered data
    PLAYBACK_EVENT_BUFFERING_TIMEOUT,        // player buffers data from network out of time
    PLAYBACK_EVENT_AUDIO_UNSUPPORTED,        // input audio format unsupported, such as: ac3, eac3, and etc
    PLAYBACK_EVENT_VIDEO_UNSUPPORTED,        // input video format unsupported, such as: field, 10bits, and etc
    PLAYBACK_EVENT_INVALID_FILE,             // input invalid url
    PLAYBACK_EVENT_NETWORK_UNREACHABLE,      // player cannot access the network
};

/**
 * decoder sequence info.
 * < only used for hard decoding >
 */
typedef struct seq_info_s {
    int     pic_width;
    int     pic_height;
    int     crop_x;
    int     crop_y;
    int     crop_width;
    int     crop_height;
} seq_info_t;

/**
 * decoder event info.
 * < only used for hard decoding >
 */
typedef struct event_info_s {
    int event;
    seq_info_t seq_info;
} event_info_t;

enum {
    AV_CODEC_TYPE_H264 = 1,
    AV_CODEC_TYPE_H265 = 2,
};

enum {
    AV_PROTOCOL_FILE     = 0,
    AV_PROTOCOL_DLNA     = 1,
    AV_PROTOCOL_MIRACAST = 2,
    AV_PROTOCOL_AIRPLAY  = 3,
};

enum {
    AV_MEDIA_TYPE_VIDEO = 0,
    AV_MEDIA_TYPE_AUDIO = 1,
};

typedef struct frame_info_s {
    int      process_id;
    int      dma_buf_fd[3];
    uint8_t  *data[3];
    uint32_t stride[3];
    int      width, height;
    int      format;
    uint8_t  *extended_data;
    uint64_t size;
    int      nb_samples;
    int      sample_rate;
    uint64_t channel_layout;
    int      channels;
    int64_t  pts;
    int      key_frame;
    void     *opaque;
} frame_info_t;

typedef struct player_info_s {
    /* For local or DLNA player, input file name */
    const char* url;

    /* Display panel info, set by user
     * !!!NOTE: if user do rotate after get video frame and set panel rotate, please set w/h after panel rotate.
     * such as, origin panel w=1920 h=1080, after panel rotate w=1080 h=1920, so set panel_width=1080 panel_height=1920.
     */
    int         panel_width;
    int         panel_height;

    /* Static player property */
    int         protocol;

    /* For airplay protocol, video size info, set by user */
    int         video_width;
    int         video_height;

    /* If video es isn't annex-b, user maybe set it to decoder */
    uint8_t     *extradata;
    int         extradata_size;

    /* For airplay protocol, video decoder output size info.
     * User need to calculate the final video output size based on video size and panel size
     */
    int         out_width;
    int         out_height;

    /* For airplay protocol, video decoder type, must be H264/H265 */
    int         codec_type;
    bool        bframe;

    /* For airplay protocol, video frame rate. if frame_rate=30*1000, it means 30FPS */
    int         frame_rate;

    /* For airplay protocol, input audio frame info, set by user.
     * Such as: sample_rate=48K channels=2 format=S16, maybe need to resample.
     */
    int         sample_rate;
    int         channels;
    int         format;
    int         nb_samples;

    /* For miracast protocol, register read_packet callback to ffmpeg */
    void        * opaque;
    int         (*read_packet)(void *opaque, uint8_t *buf, int buf_size);
} player_info_t;

/**
 * Open file or url and set the windows size
 */
__attribute__((visibility("default"))) int mm_player_open(player_info_t *input);

/**
 * Close file or url
 */
__attribute__((visibility("default"))) int mm_player_close(void);

/**
 * Stop playing
 */
__attribute__((visibility("default"))) int mm_player_pause(void);

/**
 * Resume playing
 */
__attribute__((visibility("default"))) int mm_player_resume(void);

/**
 * Seek file forward or backward in the current position for time
 */
__attribute__((visibility("default"))) int mm_player_seek(double time);

/**
 * Seek file to the setting time
 */
__attribute__((visibility("default"))) int mm_player_seek2time(double time);

/**
 * Get the current playing time of video file
 */
__attribute__((visibility("default"))) int mm_player_getposition(double *position);

/**
 * Get the total time of video file
 */
__attribute__((visibility("default"))) int mm_player_getduration(double *duration);

/**
 * Set the audio volume.
 * volume = [0]~[100]
 */
__attribute__((visibility("default"))) int mm_player_set_volume(int volume);

/**
 * Set the audio volume in whole range.
 * volume = [min]~[max], by hw spec
 */
__attribute__((visibility("default"))) int mm_player_set_volume2(int volume);

/**
 * Mute the audio.
 * volumn = false/true
 */
__attribute__((visibility("default"))) int mm_player_set_mute(bool mute);

/**
 * Set the windows size.
 * NOTE: UNSUPPORT
 */
__attribute__((visibility("default"))) int mm_player_set_window(int x, int y, int width, int height);

/**
 * Set player others options.
 * key: option name; value: reserved; flags: option value. Such as:
 * mm_player_set_opts("video_only", "", 0); -- "1"=enable; "0"=disable
 * mm_player_set_opts("audio_only", "", 0); -- "1"=enable; "0"=disable
 * mm_player_set_opts("video_rotate", "", AV_ROTATE_NONE);
 * mm_player_set_opts("video_ratio", "", AV_SCREEN_MODE);
 * mm_player_set_opts("audio_format", "", AV_PCM_FMT_S16); -- set audio format, such as: s16le
 * mm_player_set_opts("audio_channels", "", 2); -- set audio channel number, such as: 2
 * mm_player_set_opts("audio_samplerate", "", 48000); -- set audio sample rate, such as: 48000 44100
 * mm_player_set_opts("sound_card", "", 0); -- set audio card id, default:0
 * mm_player_set_opts("play_mode", "", AV_MODE_LOOP); -- set player mode, such as loop or once.
 * mm_player_set_opts("video_bypass", "", 0); -- set bypass mode or user mode, if user want to get yuv, need to set false.
 * mm_player_set_opts("audio_bypass", "", 0); -- set bypass mode or user mode, if user want to get pcm, need to set false.
 * mm_player_set_opts("spec_file_path", file_path, 0); -- set spec file path to check spec limitation.
 * mm_player_set_opts("alsa_pcm_name", "your_dmix_name", 0); -- set alsa dmix config name, only for multi-process need to set
 * mm_player_set_opts("disp_type", "drm", 0); -- set display device type, such as:drm,ttl,mipi,hdmi
 * mm_player_set_opts("audio_device", "speaker", 0); -- set audio device type, such as:auto,speaker,spdif,headphone,hdmi-arc
 */
__attribute__((visibility("default"))) int mm_player_set_opts(const char *key, const char *value, int flags);

/**
 * Set the decode output attribute.
 * width: decode output width
 * height: decode output height
 */
__attribute__((visibility("default"))) int mm_player_set_dec_out_attr(int width, int height);

/**
 * Set the video rotation.
 * rotate = [0]~[3]
 * AV_ROTATE_NONE = 0
 * AV_ROTATE_90   = 1
 * AV_ROTATE_180  = 2
 * AV_ROTATE_270  = 3
 * width: input rotate width
 * height: input rotate height
 * NOTE: Play pause or play music album does not support rotation
 */
__attribute__((visibility("default"))) int mm_player_set_rotate(int rotate, int width, int height);

/**
 * Get player real status.
 * Return value:
 * AV_ACODEC_ERROR  -- video deocder error
 * AV_VCODEC_ERROR  -- audio deocder error
 * AV_NOSYNC        -- audio and video is not sync
 * AV_READ_TIMEOUT  -- read data from network over time out
 * AV_NO_NETWORK    -- not find network
 * AV_INVALID_FILE  -- the file name or url is invalid
 * AV_NOTHING       -- player is normal
 * AV_PLAY_COMPLETE -- end of file
 * AV_PLAY_PAUSE    -- stoping player
 */
__attribute__((visibility("default"))) int mm_player_get_status(void);

/**
 * Enable/disable display if player is working. Clear screen if player is closed.
 * Please call the api after exit player to clear display.
 * NOTE: UNSUPPORT
 */
__attribute__((visibility("default"))) int mm_player_flush_screen(bool enable);

/**
 * This function is a blocking function, with a value equal to 0 indicating success in getting a frame
 * and a value less than 0 indicating failure, you need retry it.
 * User get a video frame from player, and must be paired with the mm_player_put_video_frame function.
 * Return parameter frame_info_t, for video the following variables are used:
 * int      process_id;
 * int      dma_buf_fd;
 * uint8_t  *data[3];
 * int      stride[3];
 * int      width, height;
 * int      format;
 * int64_t  pts;
 * int      key_frame;
 */
__attribute__((visibility("default"))) int mm_player_get_video_frame(frame_info_t *frame_info);

/**
 * User put back a video frame to player, and must be paired with the mm_player_get_video_frame function.
 */
__attribute__((visibility("default"))) int mm_player_put_video_frame(frame_info_t *frame_info);

/**
 * This function is a blocking function, with a value equal to 0 indicating success in getting a frame
 * and a value less than 0 indicating failure, you need retry it.
 * User get a audio frame from player, and must be paired with the mm_player_put_audio_frame function.
 * Return parameter frame_info_t, for audio the following variables are used:
 * uint8_t  *extended_data;
 * uint64_t size;
 * int      nb_samples;
 * int      sample_rate;
 * uint64_t channel_layout;
 * int      channels;
 * int      format;
 * int64_t  pts;
 * int      key_frame;
 */
__attribute__((visibility("default"))) int mm_player_get_audio_frame(frame_info_t *frame_info);

/**
 * User put back a audio frame to player, and must be paired with the mm_player_get_audio_frame function.
 */
__attribute__((visibility("default"))) int mm_player_put_audio_frame(frame_info_t *frame_info);

/**
 * Call the api after mm_player_get_audio_frame, send pcm data to device.
 * User can perform algorithm processing on audio data and sends the processed data to deivce through this api
 */
__attribute__((visibility("default"))) int mm_player_write_audio_frame(uint8_t *buffer, int size);

/**
 * Call the api after mm_player_get_video_frame, send yuv data to display.
 * User can perform algorithm processing on yuv data and sends the processed data to display through this api
 * NOTE: the input frame_info must be the same as when mm_player_get_video_frame.
 */
__attribute__((visibility("default"))) int mm_player_write_video_frame(frame_info_t *frame_info);

/* register event callback function
 * h26x hard decoding event type:
 * AV_CODEC_EVENT_NONE
 * AV_CODEC_EVENT_SEQCHANGE
 * AV_CODEC_EVENT_EOS
 * AV_CODEC_EVENT_DECERR
 * AV_CODEC_EVENT_DROPPED
 * AV_CODEC_EVENT_DECDONE
 *
 * soft decoding event type:
 * AV_CODEC_EVENT_SEQCHANGE
 *
 */
__attribute__((visibility("default"))) int mm_player_register_event_cb(void (*event_handler)(event_info_t *event_info));

/* Send video data to decoder, and send audio data to output device.
 * timestamp: video or audio pts
 * media_type: AV_MEDIA_TYPE_AUDIO/AV_MEDIA_TYPE_VIDEO
 */
__attribute__((visibility("default"))) int mm_player_send_stream(const uint8_t *buffer, int size, int64_t timestamp, int media_type);

/* Only for airplay, support to add audio or video stream alone and start playing.
 * player_info_t: refer to mm_player_open
 * media_type: AV_MEDIA_TYPE_AUDIO/AV_MEDIA_TYPE_VIDEO
 */
__attribute__((visibility("default"))) int mm_player_add_stream(player_info_t *input, int media_type);

/* Only for airplay, support to remove audio or video stream alone and stop playing.
 * media_type: AV_MEDIA_TYPE_AUDIO/AV_MEDIA_TYPE_VIDEO
 */
__attribute__((visibility("default"))) int mm_player_remove_stream(int media_type);

#ifdef __cplusplus
}
#endif // __cplusplu

#endif
