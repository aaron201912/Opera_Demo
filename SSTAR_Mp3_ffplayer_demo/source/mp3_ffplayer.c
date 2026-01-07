/* Sgs trade secret */
/* Copyright (c) [2019~2020] Sgs Technology Ltd.
All rights reserved.

Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
Sgs and be kept in strict confidence
(Sgs Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of Sgs Confidential
Information is unlawful and strictly prohibited. Sgs hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <ctype.h>
#include <getopt.h>

#include <libavutil/avutil.h>
#include <libavutil/attributes.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavutil/mathematics.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/file.h>
#include <libavutil/log.h>
#include <libavutil/time.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>

#include <alloca.h>
#include <alsa/asoundlib.h>

/* ALSA Playback Volume */

#define DAC_VOL       "DAC_0"


#define PERIOD_SIZE         (1024)
#define WAV_SAMPLERATE      (44100)

static inline int get_env_value(const char *str, int value)
{
    int i = 0;
    int len = 0;
    int tmp_value = value;

    if (!str) {
        return value;
    }
    const char *env_str = getenv(str);
    if (!env_str) {
        return value;
    }
    len = strlen(env_str);
    if (len > 10) { // length of INT_MAX
        return value;
    }
    for (i = 0; i < len; i++) {
        if (!isdigit(env_str[i])) {
            return value;
        }
    }
    if (i == len) {
        tmp_value = atoi(env_str);
        if (tmp_value > ((int)(~0U>>1))) { // INT_MAX
            return value;
        }
    }
    return tmp_value;
}

/* For debug log */
typedef enum {
    LOG_LEVEL_N = 0,
    LOG_LEVEL_E,
    LOG_LEVEL_W,
    LOG_LEVEL_I,
    LOG_LEVEL_D,
    LOG_LEVEL_V,
} loglevel_e;

#define player_logd(fmt, args...)                                                       \
    do {                                                                                \
        if (get_env_value("debug_player_log_level", LOG_LEVEL_W) >= LOG_LEVEL_D) {      \
            av_log(NULL, AV_LOG_INFO, "[%s %d] " fmt, __FUNCTION__, __LINE__, ##args);  \
        }                                                                               \
    } while (0)

#define player_logi(fmt, args...)                                                       \
    do {                                                                                \
        if (get_env_value("debug_player_log_level", LOG_LEVEL_W) >= LOG_LEVEL_I) {      \
            av_log(NULL, AV_LOG_INFO, "[%s %d] " fmt, __FUNCTION__, __LINE__, ##args);  \
        }                                                                               \
    } while (0)

#define player_logw(fmt, args...)                                                         \
    do {                                                                                  \
        if (get_env_value("debug_player_log_level", LOG_LEVEL_W) >= LOG_LEVEL_W) {        \
            av_log(NULL, AV_LOG_WARNING, "[%s %d] " fmt, __FUNCTION__, __LINE__, ##args); \
        }                                                                                 \
    } while (0)

#define player_loge(fmt, args...)                                                       \
    do {                                                                                \
        if (get_env_value("debug_player_log_level", LOG_LEVEL_W) >= LOG_LEVEL_E) {      \
            av_log(NULL, AV_LOG_ERROR, "[%s %d] " fmt, __FUNCTION__, __LINE__, ##args); \
        }                                                                               \
    } while (0)

/*************************************************************************************/

#define DUMP_FILE
#ifdef DUMP_FILE

typedef struct {
    uint32_t magic;          // "RIFF"
    uint32_t overall_size;   // 36 + data_size
    uint32_t type;           // "WAVE"
} WaveRIFFHeader;

typedef struct {
    uint32_t marker;                // "fmt "
    uint32_t length;                // 16 for PCM
    uint16_t format;                // PCM = 1
    uint16_t channels;              // mono = 1, stereo = 2
    uint32_t sample_rate;           // 44100, 16000, etc
    uint32_t byte_rate;             // sample_rate * channels * bytes_per_sample
    uint16_t block_align;           // channels * bytes_per_sample
    uint16_t bits_per_sample;       // 8 bits = 8, 16 bits = 16, etc.
} WaveFMTChunk;

typedef struct {
    uint32_t type;      // "data"
    uint32_t size;      // num_samples * channels * bytes_per_sample
} WaveDATAChunk;

#define COMPOSE_ID(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define WAV_RIFF    COMPOSE_ID('R','I','F','F')
#define WAV_WAVE    COMPOSE_ID('W','A','V','E')
#define WAV_FMT     COMPOSE_ID('f','m','t',' ')
#define WAV_DATA    COMPOSE_ID('d','a','t','a')

static FILE *fp = NULL;
static const char *dump_path = NULL;

static int wave_insert_header(FILE* fp, long data_len, int sample_rate, int channels)
{
    WaveRIFFHeader riffHeader;
    WaveFMTChunk fmtChunk;
    WaveDATAChunk dataChunk;

    riffHeader.magic = WAV_RIFF;
    riffHeader.overall_size = sizeof(WaveRIFFHeader) + sizeof(WaveFMTChunk) + data_len;
    riffHeader.type = WAV_WAVE;

    uint32_t bits_per_sample = 16;
    fmtChunk.marker = WAV_FMT;
    fmtChunk.length = 16;
    fmtChunk.format = 1;
    fmtChunk.channels = channels;
    fmtChunk.sample_rate = sample_rate;
    fmtChunk.byte_rate = sample_rate * channels * bits_per_sample / 8;
    fmtChunk.block_align = channels * bits_per_sample / 8;
    fmtChunk.bits_per_sample = bits_per_sample;

    dataChunk.type = WAV_DATA;
    dataChunk.size = data_len;

    fseek(fp, 0, SEEK_SET);
    fwrite(&riffHeader, sizeof(WaveRIFFHeader), 1, fp);
    fwrite(&fmtChunk, sizeof(WaveFMTChunk), 1, fp);
    fwrite(&dataChunk, sizeof(WaveDATAChunk), 1, fp);

    return 0;
}

static int wave_header_size(void)
{
    return sizeof(WaveRIFFHeader) + sizeof(WaveFMTChunk) + sizeof(WaveDATAChunk);
}
#endif

typedef struct {
    snd_pcm_stream_t stream_type;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_uframes_t period_count;
    snd_pcm_uframes_t period_frames;
    snd_pcm_uframes_t start_threshold;
    snd_pcm_uframes_t stop_threshold;
    snd_pcm_access_t access_type;
    snd_pcm_format_t pcm_format;
    unsigned int sample_rate;
    char sample_size;
    char channel_size;
    char frame_size;
    int  delay_timems;
} alsa_init_type;

typedef struct {
    int video_index;
    int audio_index;
    AVFormatContext* format_ctx;
    AVCodecContext* codec_ctx;
    const AVCodec* codec;
    SwrContext *swr_ctx;
    int sample_rate;
    int volume;
    pthread_t play_thread;
    bool abort_request;
} AudioState;

static bool bExit = false;
static alsa_init_type stAlsaInit;
static int write_total_size = 0;

/***************************************************************************************/

static int AppSetVolume(long outvol)
{
    snd_mixer_t* handle;
    snd_mixer_elem_t* elem;
    snd_mixer_selem_id_t* sid;

    static const char* mix_name = DAC_VOL;
    static const char* card = "default";

    if(outvol < 0 || outvol > 100) {
        player_logw("volume adjust range 0 ~ 100\n\r");
        return - 1;
    }
    outvol = outvol * 1023 / 100;
    player_logi("set volume:%d\n", (int)outvol);

    snd_mixer_selem_id_alloca(&sid);
    //sets simple-mixer index and name
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, mix_name);

    if ((snd_mixer_open(&handle, 0)) < 0){
        return -1;
    }
    if ((snd_mixer_attach(handle, card)) < 0) {
        snd_mixer_close(handle);
        return -2;
    }
    if ((snd_mixer_selem_register(handle, NULL, NULL)) < 0) {
        snd_mixer_close(handle);
        return -3;
    }
    if (snd_mixer_load(handle) < 0) {
        snd_mixer_close(handle);
        return -4;
    }
    elem = snd_mixer_find_selem(handle, sid);
    if (!elem) {
        snd_mixer_close(handle);
        return -5;
    }
    if (snd_mixer_selem_set_playback_volume(elem, 0, outvol) < 0) {
        snd_mixer_close(handle);
        return -8;
    }
    if (snd_mixer_selem_set_playback_volume(elem, 1, outvol) < 0) {
        snd_mixer_close(handle);
        return -9;
    }

    elem = snd_mixer_elem_next(elem);
    if (snd_mixer_selem_set_playback_volume(elem, 0, outvol) < 0) {
        snd_mixer_close(handle);
        return -10;
    }
    if (snd_mixer_selem_set_playback_volume(elem, 1, outvol) < 0) {
        snd_mixer_close(handle);
        return -11;
    }

    snd_mixer_close(handle);
    return 0;
}

//set control contents for one control
static int AppAttachDev(char *value)
{
    int   err = 0;
    char  card[64] = "hw:0";
    static snd_ctl_t *handle = NULL;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_value_t *control;
    snd_ctl_elem_info_alloca(&info);
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_value_alloca(&control);

    if (snd_ctl_ascii_elem_id_parse(id, "name=\'DAC_SEL\'")) {
        return -EINVAL;
    }
    if (handle == NULL && (err = snd_ctl_open(&handle, card, 0)) < 0) {
        return err;
    }

    snd_ctl_elem_info_set_id(info, id);
    if ((err = snd_ctl_elem_info(handle, info)) < 0) {
        snd_ctl_close(handle);
        handle = NULL;
        return err;
    }
    snd_ctl_elem_info_get_id(info, id);
    snd_ctl_elem_value_set_id(control, id);
    if ((err = snd_ctl_elem_read(handle, control)) < 0) {
        snd_ctl_close(handle);
        handle = NULL;
        return err;
    }
    err = snd_ctl_ascii_value_parse(handle, control, info, value);
    if (err < 0) {
        snd_ctl_close(handle);
        handle = NULL;
        return err;
    }

    if ((err = snd_ctl_elem_write(handle, control)) < 0) {
        snd_ctl_close(handle);
        handle = NULL;
        return err;
    }

    snd_ctl_close(handle);
    handle = NULL;
    return 0;

}

static int AppPcmInit(alsa_init_type *stAlsaInit, char *snd_card_name)
{
    int err = 0;
    int dir = 0;

    AppAttachDev("0");
    AppAttachDev("1");

    // wait for amp start
    if (stAlsaInit->delay_timems > 0) {
        usleep(stAlsaInit->delay_timems * 1000);
    }

    /* Open PCM device for recording (playback). */
    err = snd_pcm_open(&stAlsaInit->handle, snd_card_name, stAlsaInit->stream_type, 0);
    if (err < 0) {
        fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(err));
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&stAlsaInit->params);
    /* Fill it in with default values. */
    err = snd_pcm_hw_params_any(stAlsaInit->handle, stAlsaInit->params);
    if (err < 0) {
        fprintf(stderr, "Error in snd_pcm_hw_params_any %s\n", snd_strerror(err));
        return err;
    }
    /* Set the desired hardware parameters. */
    /* Interleaved mode */
    err = snd_pcm_hw_params_set_access(stAlsaInit->handle, stAlsaInit->params, stAlsaInit->access_type);
    if (err < 0) {
        fprintf(stderr, "Error in snd_pcm_hw_params_set_access %s\n", snd_strerror(err));
        return err;
    }
    /* Signed 16-bit little-endian format */
    err = snd_pcm_hw_params_set_format(stAlsaInit->handle, stAlsaInit->params, stAlsaInit->pcm_format);
    if (err < 0) {
        fprintf(stderr, "Error in snd_pcm_hw_params_set_format %s\n", snd_strerror(err));
        return err;
    }
    /* Two channels (stereo) */

    err = snd_pcm_hw_params_set_channels(stAlsaInit->handle, stAlsaInit->params, stAlsaInit->channel_size);
    if (err < 0) {
        fprintf(stderr, "Error in snd_pcm_hw_params_set_channels %s\n", snd_strerror(err));
        return err;
    }
    /* second sampling rate (CD quality) */
    err = snd_pcm_hw_params_set_rate_near(stAlsaInit->handle, stAlsaInit->params, &stAlsaInit->sample_rate, &dir);
    if (err < 0) {
        fprintf(stderr, "Error in snd_pcm_hw_params_set_rate_near %s\n", snd_strerror(err));
        return err;
    }
    /* Set buffer size. */
//    err = snd_pcm_hw_params_set_buffer_size_near(stAlsaInit->handle, stAlsaInit->params, &stAlsaInit->buffer_frames);
//    if (err < 0) {
//        fprintf(stderr, "Error in snd_pcm_hw_params_set_buffer_size_near %s\n", snd_strerror(err));
//        return err;
//    }
    /* Set period size. */
    err = snd_pcm_hw_params_set_period_size_near(stAlsaInit->handle, stAlsaInit->params, &stAlsaInit->period_frames, &dir);
    if (err < 0) {
        fprintf(stderr, "Error in snd_pcm_hw_params_set_period_size_near %s\n", snd_strerror(err));
        return err;
    }
    err = snd_pcm_hw_params_set_periods_near(stAlsaInit->handle, stAlsaInit->params, (unsigned int*)&stAlsaInit->period_count, &dir);
    if (err < 0) {
        fprintf(stderr, "Error in snd_pcm_hw_params_set_periods_near %s\n", snd_strerror(err));
        return err;
    }
    /* Write the parameters to the driver */
    err = snd_pcm_hw_params(stAlsaInit->handle, stAlsaInit->params);
    if (err < 0) {
        fprintf(stderr, "unable to set hw snd_pcm_hw_params: %s\n",
            snd_strerror(err));
        return err;
    }
    /* Allocate a software parameters object. */
    snd_pcm_sw_params_alloca(&stAlsaInit->sw_params);
    /* get current sw parameters */
    err = snd_pcm_sw_params_current(stAlsaInit->handle, stAlsaInit->sw_params);
    if (err < 0) {
        fprintf(stderr, "unable to get sw snd_pcm_sw_params: %s\n",
            snd_strerror(err));
        return err;
    }
    /* set waterline value */
    err = snd_pcm_sw_params_set_start_threshold(stAlsaInit->handle, stAlsaInit->sw_params, stAlsaInit->start_threshold);
    if (err < 0) {
        fprintf(stderr, "unable to set sw snd_pcm_sw_params_set_start_threshold: %s\n",
            snd_strerror(err));
        return err;
    }
    err = snd_pcm_sw_params_set_stop_threshold(stAlsaInit->handle, stAlsaInit->sw_params, stAlsaInit->stop_threshold);
    if (err < 0) {
        fprintf(stderr, "unable to set sw snd_pcm_sw_params_set_stop_threshold: %s\n",
            snd_strerror(err));
        return err;
    }
    /* Write the sw parameters to the driver */
    err = snd_pcm_sw_params(stAlsaInit->handle, stAlsaInit->sw_params);
    if (err < 0) {
        fprintf(stderr, "unable to set sw snd_pcm_sw_params: %s\n",
            snd_strerror(err));
        return err;
    }

    player_logi("Alsa lib version %d.%d.%d\n", (SND_LIB_VERSION >> 16 & 0xff), (SND_LIB_VERSION >> 8 & 0xff), (SND_LIB_VERSION >> 0 & 0xff));
    return err;
}

static int AppPlayDataStream(snd_pcm_t* pst_handle, char* p_databuff, int data_size)
{
    int ret = 0;
    snd_pcm_uframes_t send_frames = snd_pcm_bytes_to_frames(pst_handle, data_size);
    write_total_size += data_size;
    player_logd("send_frames=%d write_total_size=%d\n", (int)send_frames, write_total_size);
    ret = snd_pcm_writei(pst_handle, p_databuff, send_frames);
    if (ret == -EPIPE) {
        /* EPIPE means overrun */
        fprintf(stderr, "underrun occurred\n");
        snd_pcm_prepare(pst_handle);
    }
    else if (ret < 0) {
        fprintf(stderr, "error from write: %s\n", snd_strerror(ret));
    }
    else if (ret != (int)send_frames) {
        fprintf(stderr, "short write, read %d frames\n", ret);
    }
    return ret;
}

static void AppTurnOffPcm(snd_pcm_t* pst_handle)
{
    if (!pst_handle) {
        player_loge("null handle!\n");
        return;
    }
    snd_pcm_drain(pst_handle);
    snd_pcm_close(pst_handle);
    snd_pcm_hw_free(pst_handle);
    AppAttachDev("0");
}

static void *mp3DecodeProc(void *param)
{
    AudioState *is = (AudioState *)param;
    bool device_initialized = false;
    bool flag_end_of_file = 0;
    int ret = 0;
    int data_size = 0;
    int resample_count = 0;
    bool format_same = true;
    uint8_t **out_data = NULL;
#ifdef DUMP_FILE
    int fileDataSize = 0;
#endif
    AVPacket packet, *pkt = &packet;
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        player_loge("av_frame_alloc failed!\n");
        return NULL;
    }

    while (true) {
        if (is->abort_request) {
            break;
        }

        if (!flag_end_of_file) {
            // read audio packet from file
            ret = av_read_frame(is->format_ctx, pkt);
            if (ret < 0) {
                if(ret == AVERROR_EOF) {
                    flag_end_of_file = true;
                    avcodec_flush_buffers(is->codec_ctx);
                    player_logi("end of file!\n");
                    continue;
                } else {
                    player_loge("av_read_frame error, ret:%s\n", av_err2str(ret));
                    break;
                }
            }
            if (pkt->stream_index != is->audio_index) {
                continue;
            }
            // send packet to decoder and wait for decoding
            ret = avcodec_send_packet(is->codec_ctx, pkt);
            if(ret < 0) {
                player_loge("avcodec_send_packet fail:%s\n", av_err2str(ret));
                av_packet_unref(pkt);
                continue;
            }
            av_packet_unref(pkt);
        }
        // receive frame after decode done
        ret = avcodec_receive_frame(is->codec_ctx, frame);
        if (ret < 0) {
            if (ret != AVERROR(EAGAIN)) {
                player_loge("avcodec_receive_frame fail:%s\n", av_err2str(ret));
                break;
            }
            if (flag_end_of_file) {
                is->abort_request = true;
                // wait alsa playback done
                // max_alsa_cache = period_frames(1024) * period_count(16) = 16384(sampling points)
                // max_alsa_playback_time = max_alsa_cache / sample_rate(44100) = 372ms
                // therefore, you can wait for 1s(>372ms) to finish all the cached data
                sleep(1);
                player_logi("exit!\n");
            }
            continue;
        }

        data_size = av_samples_get_buffer_size(NULL, is->codec_ctx->channels, frame->nb_samples, is->codec_ctx->sample_fmt, 1);

        format_same = format_same && (av_get_default_channel_layout(frame->channels) == AV_CH_LAYOUT_STEREO) &&
            (frame->format == AV_SAMPLE_FMT_S16) && (frame->sample_rate == WAV_SAMPLERATE);

        // pcm data need to resample for audio output device
        if (!is->swr_ctx && !format_same) {
            player_logi("channnels=%d format=%d sample_rate=%d\n", frame->channels, frame->format, frame->sample_rate);
            is->swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, WAV_SAMPLERATE,
                                             av_get_default_channel_layout(frame->channels),
                                             frame->format, frame->sample_rate, 0, NULL);
            if (!is->swr_ctx) {
                player_loge("swr_alloc_set_opts failed!\n");
                av_frame_free(&frame);
                return NULL;
            }
            ret = swr_init(is->swr_ctx);
            if (ret < 0) {
                player_loge("swr_init failed!\n");
                av_frame_free(&frame);
                return NULL;
            }
        }
        if (!format_same) {
            int out_nb_samples = swr_get_out_samples(is->swr_ctx, frame->nb_samples);
            int out_linesize;
            av_samples_alloc_array_and_samples(&out_data, &out_linesize, AV_CH_LAYOUT_STEREO,
                                      out_nb_samples, AV_SAMPLE_FMT_S16, 0);

            resample_count = swr_convert(is->swr_ctx, out_data, WAV_SAMPLERATE, (const uint8_t **)frame->extended_data, frame->nb_samples);
        }

        if (!device_initialized) {
            device_initialized = true;
            stAlsaInit.stream_type      = SND_PCM_STREAM_PLAYBACK;
            stAlsaInit.handle           = NULL;
            stAlsaInit.params           = NULL;
            stAlsaInit.sample_size      = 2;
            stAlsaInit.sample_rate      = WAV_SAMPLERATE;
            stAlsaInit.channel_size     = 2;
            stAlsaInit.access_type      = SND_PCM_ACCESS_RW_INTERLEAVED;
            stAlsaInit.pcm_format       = SND_PCM_FORMAT_S16_LE;
            stAlsaInit.period_frames    = PERIOD_SIZE;
            stAlsaInit.period_count     = 16;
            stAlsaInit.start_threshold  = stAlsaInit.period_frames;
            stAlsaInit.stop_threshold   = stAlsaInit.period_count * stAlsaInit.period_frames;
            ret = AppPcmInit(&stAlsaInit, "default");
            if (ret < 0) {
                player_loge("alsa init failed!\n");
                av_frame_free(&frame);
                return NULL;
            }
            AppSetVolume(is->volume);
        }
        data_size = format_same ? data_size : snd_pcm_frames_to_bytes(stAlsaInit.handle, resample_count);
        player_logd("resample_count=%d sample_rate=%d, origin nb_samples=%d sample_rate=%d, data_size:%d\n",
                    resample_count, WAV_SAMPLERATE, frame->nb_samples, frame->sample_rate, data_size);
#ifdef DUMP_FILE
        if (dump_path && strlen(dump_path) >= 1) {
            fileDataSize += data_size;
            //write pcm data to wav file
            if (fp) {
                fwrite(out_data[0], (size_t)data_size, 1, fp);
            }
        }
#endif
        AppPlayDataStream(stAlsaInit.handle, format_same ? (char*)frame->extended_data[0] : (char*)out_data[0], data_size);
        if (!format_same && out_data) {
            av_freep(&out_data[0]);
            av_freep(&out_data);
        }
        av_frame_unref(frame);
    }

#ifdef DUMP_FILE
    if (dump_path && strlen(dump_path) >= 1) {
        player_logi("fileDataSize=%d\n", fileDataSize);
        if (fp) {
            wave_insert_header(fp, fileDataSize, WAV_SAMPLERATE, 2);
            fclose(fp);
        }
    }
#endif

    if (is->swr_ctx != NULL) {
        swr_free(&is->swr_ctx);
    }
    av_frame_free(&frame);

    return NULL;
}

static int init_player(AudioState* is, const char* url, int volume)
{
    int ret;

    if (!is || !url) {
        player_loge("invalid input filepath!\n");
        return -1;
    }
    memset(is, 0, sizeof(AudioState));
    is->audio_index =
    is->video_index = -1;
    is->volume = volume;
    is->format_ctx = avformat_alloc_context();
    if (!is->format_ctx) {
        player_loge("avformat_alloc_context failed!\n");
        return -1;
    }

    ret = avformat_open_input(&is->format_ctx, url, NULL, NULL);
    if (ret < 0) {
        player_loge("avformat_open_input failed, ret:%s\n", av_err2str(ret));
        return -1;
    }

    ret = avformat_find_stream_info(is->format_ctx, NULL);
    if(ret < 0) {
        player_loge("avformat_find_stream_info failed, ret:%s\n", av_err2str(ret));
        return -1;
    }

    av_dump_format(is->format_ctx, 0, url, 0);
    is->video_index = av_find_best_stream(is->format_ctx, AVMEDIA_TYPE_VIDEO, is->video_index, -1, NULL, 0);
    is->audio_index = av_find_best_stream(is->format_ctx, AVMEDIA_TYPE_AUDIO, is->audio_index, is->video_index, NULL, 0);
    if (is->audio_index < 0) {
        player_loge("don't find valid audio stream!\n");
        return -1;
    }
    player_logi("video_index=%d, audio_index=%d\n", is->video_index, is->audio_index);

    AVCodecParameters *codecpar = is->format_ctx->streams[is->audio_index]->codecpar;
    is->sample_rate = codecpar->sample_rate;
    player_logi("audio samplerate=%d\n", is->sample_rate);

    is->codec = avcodec_find_decoder(codecpar->codec_id);
    if(!is->codec) {
        player_logw("don't found decoder, codec_id:%d\n", codecpar->codec_id);
        return -1;
    }

    is->codec_ctx = avcodec_alloc_context3(is->codec);
    if (!is->codec_ctx) {
        player_loge("avcodec_alloc_context3 failed!\n");
        return -1;
    }

    ret = avcodec_parameters_to_context(is->codec_ctx, codecpar);
    if (ret < 0) {
        player_loge("avcodec_parameters_to_context failed\n");
        return -1;
    }
    is->codec_ctx->request_sample_fmt = AV_SAMPLE_FMT_S16;

    ret = avcodec_open2(is->codec_ctx, is->codec, NULL);
    if (ret < 0) {
        player_loge("avcodec_open2 failed, ret:%s\n", av_err2str(ret));
        return -1;
    }

#ifdef DUMP_FILE
    if (!dump_path) {
        dump_path = getenv("debug_player_dump_wav");
    }
    if (dump_path && strlen(dump_path) >= 1) {
        char name[256];
        memset(name, '\0', sizeof(name));
        sprintf(name, "%s/test.wav", dump_path);
        fp = fopen(name, "wb+");
        if (fp) {
            int wavHeaderLen = wave_header_size();
            fseek(fp, wavHeaderLen, SEEK_SET);
            player_logi("dump_path:%s wavHeaderLen=%d\n", dump_path, wavHeaderLen);
        }
    }
#endif

    is->abort_request = false;
    ret = pthread_create(&is->play_thread, NULL, mp3DecodeProc, (void *)is);
    if (ret != 0) {
        player_loge("pthread_create mp3DecodeProc failed!\n");
        return -1;
    }

    return 0;
}

static void deinit_player(AudioState* is)
{
    is->abort_request = true;
    if (is->play_thread) {
        pthread_join(is->play_thread, NULL);
    }
    if (is->codec_ctx) {
        avcodec_free_context(&is->codec_ctx);
    }
    if (is->format_ctx) {
        avformat_close_input(&is->format_ctx);
    }
    player_logi("close success!\n");
}

static void signalHandler(int signal)
{
    switch (signal) {
        case SIGINT:
            printf("catch exit signal\n");
            bExit = true;
            break;
        default:
            break;
    }
}

static void usage(const char* me) {
    fprintf(stderr,
            "usage: %s, eg:./mp3player -i [file] -v [volume]\n"
            "\t\t[-i] set an input mp3 file\n"
            "\t\t[-v] the audio playback volume, default is 50\n"
            "\t\t[-d] set dump file path for debug, eg: -d /mnt\n"
            "\t\t[-t] set alsa amp delay time(ms), default is 0\n",
            me);
    exit(1);
}

int main(int argc, char **argv)
{
    int ret = 0;
    const char *url = NULL;
    int volume = 50;
    const char* me = argv[0];

    signal(SIGINT, signalHandler);

    player_logi("ffmpeg library version:%s\n", av_version_info());

    memset(&stAlsaInit, 0, sizeof(alsa_init_type));
    while ((ret = getopt(argc, argv, "hi:v:d:t:")) >= 0) {
        switch (ret) {
            case 'i': {
                url = optarg;
                player_logi("set input url:%s\n", url);
                break;
            }
            case 'v': {
                volume = atoi(optarg);
                player_logi("set volume:%d\n", volume);
                break;
            }
            case 'd': {
                dump_path = optarg;
                player_logi("set dump_path:%s\n", dump_path);
                break;
            }
            case 't': {
                stAlsaInit.delay_timems = atoi(optarg);
                player_logi("set amp delay time:%dms\n", stAlsaInit.delay_timems);
                break;
            }
            case 'h':
            default: {
                usage(me);
            }
        }
    }

    argc -= optind;
    argv += optind;
    if (argc < 0) {
        usage(me);
    }

    AudioState* is = (AudioState*)av_mallocz(sizeof(AudioState));
    if (!is) {
        player_loge("av_mallocz failed!\n");
        return 0;
    }

    if ((ret = init_player(is, url, volume)) != 0) {
        player_loge("init player error!\n");
        goto uninstall;
    }

    while (true) {
        if (bExit || is->abort_request) {
            break;
        }
        usleep(30000);
    }

uninstall:
    deinit_player(is);
    AppTurnOffPcm(stAlsaInit.handle);

    av_free(is);
    player_logi("player exit success!\n");

    return 0;
}
