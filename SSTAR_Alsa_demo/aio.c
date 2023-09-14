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

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"

#include "aio.h"

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#ifndef UNUSED
#define UNUSED(var) (void)((var) = (var))
#endif

#define ASCII_COLOR_RED    "\033[1;31m"
#define ASCII_COLOR_WHITE  "\033[1;37m"
#define ASCII_COLOR_YELLOW "\033[1;33m"
#define ASCII_COLOR_BLUE   "\033[1;36m"
#define ASCII_COLOR_GREEN  "\033[1;32m"
#define ASCII_COLOR_END    "\033[0m"

#define DEMO_SUCCESS (0)
#define DEMO_FAIL    (-1)

#define DEBUG
#ifdef DEBUG

#define PrintErr(fmt, args...)                                    \
    do                                                            \
    {                                                             \
        printf(ASCII_COLOR_RED "E/[%s:%d] ", __func__, __LINE__); \
        printf(fmt, ##args);                                      \
        printf(ASCII_COLOR_END);                                  \
    } while (0)

#define PrintWarn(fmt, args...)                                      \
    do                                                               \
    {                                                                \
        printf(ASCII_COLOR_YELLOW "W/[%s:%d] ", __func__, __LINE__); \
        printf(fmt, ##args);                                         \
        printf(ASCII_COLOR_END);                                     \
    } while (0)

#define PrintInfo(fmt, args...)                                    \
    do                                                             \
    {                                                              \
        printf(ASCII_COLOR_BLUE "I/[%s:%d] ", __func__, __LINE__); \
        printf(fmt, ##args);                                       \
        printf(ASCII_COLOR_END);                                   \
    } while (0)

#define PrintDebug(fmt, args...)                                    \
    do                                                              \
    {                                                               \
        printf(ASCII_COLOR_GREEN "D/[%s:%d] ", __func__, __LINE__); \
        printf(fmt, ##args);                                        \
        printf(ASCII_COLOR_END);                                    \
    } while (0)

#define FUNC_ENTER() PrintInfo(">>>>>>\n")
#define FUNC_EXIT()  PrintInfo("<<<<<<\n")

#else
#define PrintErr(fmt, args...)
#define PrintWarn(fmt, args...)
#define PrintInfo(fmt, args...)
#define PrintDebug(fmt, args...)
#define FUNC_ENTER()
#define FUNC_EXIT()
#endif

// for ai/ao wav file info
#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

#define WAVE_FORMAT_PCM        0x0001
#define WAVE_FORMAT_IEEE_FLOAT 0x0003

//-------------------------------------------------------------------------------------------------
//  Function declaration
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Macros
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Enum
//-------------------------------------------------------------------------------------------------
typedef enum
{
    E_ERR_INVALID_DEVID = 1,    /* invlalid device ID                           */
    E_ERR_INVALID_CHNID = 2,    /* invlalid channel ID                          */
    E_ERR_ILLEGAL_PARAM = 3,    /* at lease one parameter is illagal
                                 ** eg, an illegal enumeration value             */
    E_ERR_EXIST      = 4,       /* resource exists                              */
    E_ERR_UNEXIST    = 5,       /* resource unexists                            */
    E_ERR_NULL_PTR   = 6,       /* using a NULL point                           */
    E_ERR_NOT_CONFIG = 7,       /* try to enable or initialize system, device
                                 ** or channel, before configing attribute       */
    E_ERR_NOT_SUPPORT = 8,      /* operation or type is not supported by NOW    */
    E_ERR_NOT_PERM    = 9,      /* operation is not permitted
                                 ** eg, try to change static attribute           */
    E_ERR_NOMEM        = 12,    /* failure caused by malloc memory              */
    E_ERR_NOBUF        = 13,    /* failure caused by malloc buffer              */
    E_ERR_BUF_EMPTY    = 14,    /* no data in buffer                            */
    E_ERR_BUF_FULL     = 15,    /* no buffer for new data                       */
    E_ERR_SYS_NOTREADY = 16,    /* System is not ready,maybe not initialed or
                                 ** loaded. Returning the error code when opening
                                 ** a device file failed.                        */
    E_ERR_BADADDR = 17,         /* bad address,
                                 ** eg. used for copy_from_user & copy_to_user   */
    E_ERR_BUSY = 18,            /* resource is busy,
                                 ** eg. destroy a venc chn without unregister it */
    E_ERR_CHN_NOT_STARTED = 19, /* channel not start*/
    E_ERR_CHN_NOT_STOPED  = 20, /* channel not stop*/
    E_ERR_NOT_INIT        = 21, /* module not init before use it*/
    E_ERR_INITED          = 22, /* module already init*/
    E_ERR_NOT_ENABLE      = 23, /* device  channel or port  not  enable*/
    E_ERR_NOT_DISABLE     = 24, /* device  channel or port  not  disable*/
    E_ERR_SYS_TIMEOUT     = 25, /* sys timeout*/
    E_ERR_DEV_NOT_STARTED = 26, /* device  not started*/
    E_ERR_DEV_NOT_STOPED  = 27, /* device  not stoped*/
    E_ERR_CHN_NO_CONTENT  = 28, /* there is no content in the channel.*/
    E_ERR_NOVASPACE       = 29, /* failure caused by va mmap */
    E_ERR_NOITEM          = 30, /* no item record in ringpool */
    E_ERR_FAILED,               /* unexpected error */

    E_ERR_MAX = 127, /* maxium code, private error code of all modules
                      ** must be greater than it                      */
} ERR_CODE_E;

typedef enum {
    USECASE_PLAYBACK,
    USECASE_CAPTURE,
    USECASE_PASSTHROUGH,
    USECASE_PASSTHROUGH_PLAYBACK,
    USECASE_AEC,
    USECASE_COMPRESS,
} USECASE_TYPE_E;

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------

// for ai
struct wav_header {
    unsigned int riff_id;
    unsigned int riff_sz;
    unsigned int riff_fmt;
    unsigned int fmt_id;
    unsigned int fmt_sz;
    unsigned short audio_format;
    unsigned short num_channels;
    unsigned int sample_rate;
    unsigned int byte_rate;
    unsigned short block_align;
    unsigned short bits_per_sample;
    unsigned int data_id;
    unsigned int data_sz;
};

// for ao
struct riff_wave_header
{
    unsigned int riff_id;
    unsigned int riff_sz;
    unsigned int wave_id;
};

struct chunk_header
{
    unsigned int id;
    unsigned int sz;
};

struct chunk_fmt
{
    unsigned short audio_format;
    unsigned short num_channels;
    unsigned int   sample_rate;
    unsigned int   byte_rate;
    unsigned short block_align;
    unsigned short bits_per_sample;
};

struct cmd
{
    USECASE_TYPE_E usecase_type;
    const char *interface;

    // for ai
    char *in_file_name;
    char *in_file_type;
    unsigned int in_card_id;
    unsigned int in_device_id;
    unsigned int in_channels;
    unsigned int in_rate;
    unsigned int in_times;

    // for ao
    char *out_file_name;
    char *out_file_type;
    unsigned int out_card_id;
    unsigned int out_device_id;
    unsigned int out_channels;
    unsigned int out_rate;
    unsigned int out_times;

    // for passthrough
    unsigned int pt_in_out_channels;
    unsigned int pt_in_rate;
    unsigned int pt_out_rate;
    unsigned int dma_rate;
    // unsigned int ms_mode;
    // unsigned int wire_mode;
    // unsigned int fmt;
    // unsigned int i2s_channels;
};

struct ctx
{
    

    // for ai
    snd_pcm_t *      in_pcm;
    FILE *in_file;
    pthread_t capture_thread[CARD_NUM];
    struct wav_header header;
    unsigned int capture_frames;
    unsigned int in_frame_total_size;

    // for ao
    snd_pcm_t *      out_pcm;
    FILE *out_file;
    pthread_t playback_thread[CARD_NUM];
    struct riff_wave_header wave_header;
    struct chunk_header     chunk_header;
    struct chunk_fmt        chunk_fmt;
    unsigned int out_frame_total_size;
};

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Local Variables
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  local function  prototypes
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  global function  prototypes
//-------------------------------------------------------------------------------------------------

// char* get_vol_control_name(char* card, char* lNameStr, char* rNameStr){
//     switch(){

// ADC_A_L_VOL
// ADC_A_R_VOL
// ADC_B_L_VOL
// ADC_B_R_VOL
// DMIC_0_VOL
// DMIC_1_VOL
// DMIC_2_VOL
// DMIC_3_VOL
// DMIC_4_VOL
// DMIC_5_VOL
// DMIC_6_VOL
// DMIC_7_VOL
// I2S_RXA_0_VOL
// I2S_RXA_1_VOL
// I2S_RXA_2_VOL
// I2S_RXA_3_VOL
// I2S_RXA_4_VOL
// I2S_RXA_5_VOL
// I2S_RXA_6_VOL
// I2S_RXA_7_VOL
// I2S_RXB_0_VOL
// I2S_RXB_1_VOL
// ECHO_RX_VOL

// DAC_L_VOL
// DAC_R_VOL
// I2S_TXA_L_VOL
// I2S_TXA_R_VOL
// I2S_TXB_L_VOL
// I2S_TXB_R_VOL
// SPDIF_TX_L_VOL
// SPDIF_TX_R_VOL

//     }
// }

static int aio_open(struct ctx *ctx, snd_pcm_stream_t stream, unsigned int card_id, unsigned int device_id, unsigned int channels_param, unsigned int rate_param, unsigned int dma_rate_param)
{
    int  ret      = DEMO_SUCCESS;
    snd_pcm_t **      in_out_pcm;
    if(stream == SND_PCM_STREAM_CAPTURE){
        in_out_pcm = &ctx->in_pcm;
    }
    else {
        in_out_pcm = &ctx->out_pcm;
    }
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    snd_output_t *log;
    snd_output_stdio_attach(&log, stderr, 0);
    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_sw_params_alloca(&sw_params);
    size_t n;
    int start_delay = 0;
    int stop_delay = 0;
    snd_pcm_uframes_t start_threshold, stop_threshold;

    char card[64] = "default";
    sprintf(card, "hw:%i,%i", card_id, device_id);

    /* Open PCM device for playing (playback). */
    ret = snd_pcm_open(in_out_pcm, card, stream, 0 /*mode*/);
    if (ret < 0)
    {
        PrintErr("Unable to open PCM device (%s)\n", snd_strerror(ret));
        return DEMO_FAIL;
    }

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(*in_out_pcm, hw_params);

#if 0
    unsigned int latency = 1000000 * DEFAULT_PERIOD_SIZE / rate_param * DEFAULT_PERIOD_COUNT;
    /* set alsa basic param, only support SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED */
    ret = snd_pcm_set_params(*in_out_pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, channels_param, rate_param,
                             1 /*soft_resample*/, latency /*in us*/);
    if (ret < 0)
    {
        PrintErr("Unable to set params (%s)\n", snd_strerror(ret));
        return DEMO_FAIL;
    }
#else

    int dir;
    unsigned int channels = channels_param;
    unsigned int rate = rate_param;
    snd_pcm_uframes_t period_frames = DEFAULT_PERIOD_SIZE;
    unsigned int period_count = DEFAULT_PERIOD_COUNT;

    /* Set the desired hardware parameters. */
    /* Interleaved mode */
    snd_pcm_hw_params_set_access(*in_out_pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(*in_out_pcm, hw_params, SND_PCM_FORMAT_S16_LE);

    /* channels (stereo) */
    snd_pcm_hw_params_set_channels(*in_out_pcm, hw_params, channels);

    /* sampling rate */
    snd_pcm_hw_params_set_rate_near(*in_out_pcm, hw_params, &rate, &dir);

    /* Set period size to frames. */
    snd_pcm_hw_params_set_period_size_near(*in_out_pcm, hw_params, &period_frames, &dir);
    snd_pcm_hw_params_set_periods_near(*in_out_pcm, hw_params, &period_count, &dir);

    /* Write the parameters to the driver */
    ret = snd_pcm_hw_params(*in_out_pcm, hw_params);
    if (ret < 0) {
        PrintErr("unable to set hw parameters (%s)\n", snd_strerror(ret));
        return DEMO_FAIL;
    }

#endif

    ret = snd_pcm_sw_params_current(*in_out_pcm, sw_params);
    if (ret < 0) {
        PrintErr("Unable to get current sw params.");
        return DEMO_FAIL;
    }

    if(stream == SND_PCM_STREAM_CAPTURE){
        start_delay = 1;
    }
    /* round up to closest transfer boundary */
    n = DEFAULT_PERIOD_SIZE * DEFAULT_PERIOD_COUNT;
    if (start_delay <= 0) {
        start_threshold = n + (double) rate_param * start_delay / 1000000;
    } else{
        start_threshold = (double) rate_param * start_delay / 1000000;
    }
    if (start_threshold < 1){
        start_threshold = 1;
    }
    if (start_threshold > n){
        start_threshold = n;
    }
    snd_pcm_sw_params_set_start_threshold(*in_out_pcm, sw_params, start_threshold);

    if (stop_delay <= 0) {
        stop_threshold = n + (double) rate_param * stop_delay / 1000000;
    }
    else {
        stop_threshold = (double) rate_param * stop_delay / 1000000;
    }
    snd_pcm_sw_params_set_stop_threshold(*in_out_pcm, sw_params, stop_threshold);

    if (snd_pcm_sw_params(*in_out_pcm, sw_params) < 0) {
        snd_pcm_sw_params_dump(sw_params, log);
        PrintErr("Unable to get current sw params.");
        return DEMO_FAIL;
    }

    // for dump info
    PrintInfo("===hw_params_dump====\n");
    snd_pcm_hw_params_dump(hw_params, log);
    PrintInfo("=====================\n");

    PrintInfo("===sw_params_dump====\n");
    snd_pcm_sw_params_dump(sw_params, log);
    PrintInfo("=====================\n");

    PrintInfo("===pcm_dump==========\n");
    snd_pcm_dump(*in_out_pcm, log);
    PrintInfo("=====================\n");

    snd_output_close(log);

    FUNC_EXIT();
    return ret;
}

// set control contents for one control
static int aio_control_contents_set_by_name(unsigned int card_id, char *name, char *value)
{
    FUNC_ENTER();
    int                   err      = 0;
    char                  card[64] = "default";
    static snd_ctl_t *    handle   = NULL;
    snd_ctl_elem_info_t * info;
    snd_ctl_elem_id_t *   id;
    snd_ctl_elem_value_t *control;
    snd_ctl_elem_info_alloca(&info);
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_value_alloca(&control);

    if (!name || !value)
    {
        PrintErr("Error param, specify a full control identifier: [name='name']\n");
        return -EINVAL;
    }

    sprintf(card, "hw:%i", card_id);

    if (snd_ctl_ascii_elem_id_parse(id, name))
    {
        PrintErr("Wrong control identifier:(%s)\n", name);
        return -EINVAL;
    }
    if (handle == NULL && (err = snd_ctl_open(&handle, card, 0)) < 0)
    {
        PrintErr("Control(%s) open error:(%s)\n", card, snd_strerror(err));
        return err;
    }
    snd_ctl_elem_info_set_id(info, id);
    if ((err = snd_ctl_elem_info(handle, info)) < 0)
    {
        PrintErr("Cannot find the given element from control(%s)\n", card);
        snd_ctl_close(handle);
        handle = NULL;
        return err;
    }
    snd_ctl_elem_info_get_id(info, id);
    snd_ctl_elem_value_set_id(control, id);
    if ((err = snd_ctl_elem_read(handle, control)) < 0)
    {
        PrintErr("Cannot read the given element from control(%s)\n", card);
        snd_ctl_close(handle);
        handle = NULL;
        return err;
    }
    err = snd_ctl_ascii_value_parse(handle, control, info, value);
    if (err < 0)
    {
        PrintErr("Control(%s) parse error:(%s)\n", card, snd_strerror(err));
        snd_ctl_close(handle);
        handle = NULL;
        return err;
    }
    if ((err = snd_ctl_elem_write(handle, control)) < 0)
    {
        PrintErr("Control(%s) element write error:(%s)\n", card, snd_strerror(err));
        snd_ctl_close(handle);
        handle = NULL;
        return err;
    }

    snd_ctl_close(handle);
    handle = NULL;
    FUNC_EXIT();
    return 0;
}

int aio_control_attach(unsigned int card_id, AIO_IF_E aio_if, AO_SEL_E ao_sel, AI_MULTICHANNEL_SEL_E ai_sel)
{
    FUNC_ENTER();
    int  ret            = DEMO_SUCCESS;
    char name_str[512]  = {0};
    char value_str[512] = {0};
    memset(name_str, 0, sizeof(name_str));
    memset(value_str, 0, sizeof(value_str));

    switch (aio_if)
    {
        //for ao
        case E_AO_IF_VIR_MUX:
            sprintf(name_str, "name=\'%s\'", AO_VIR);
            break;
        case E_AO_IF_HDMI_TX:
            sprintf(name_str, "name=\'%s\'", HDMI_TX);
            break;
        case E_AO_IF_DAC:
            sprintf(name_str, "name=\'%s\'", DAC);
            break;
        case E_AO_IF_ECHO:
            sprintf(name_str, "name=\'%s\'", ECHO_TX);
            break;
        case E_AO_IF_SPDIF_TX:
            sprintf(name_str, "name=\'%s\'", SPDIF_TX);
            break;
        case E_AO_IF_I2S_TX_A:
            sprintf(name_str, "name=\'%s\'", I2S_TXA);
            break;
        case E_AO_IF_I2S_TX_B:
            sprintf(name_str, "name=\'%s\'", I2S_TXB);
            break;
        // for ai
        case E_AI_IF_MCH_01:
            sprintf(name_str, "name=\'%s\'", AI_MCH_01);
            break;
        case E_AI_IF_MCH_23:
            sprintf(name_str, "name=\'%s\'", AI_MCH_23);
            break;
        case E_AI_IF_MCH_45:
            sprintf(name_str, "name=\'%s\'", AI_MCH_45);
            break;
        case E_AI_IF_MCH_67:
            sprintf(name_str, "name=\'%s\'", AI_MCH_67);
            break;
        case E_AI_IF_MCH_89:
            sprintf(name_str, "name=\'%s\'", AI_MCH_89);
            break;
        case E_AI_IF_MCH_AB:
            sprintf(name_str, "name=\'%s\'", AI_MCH_AB);
            break;
        case E_AI_IF_MCH_CD:
            sprintf(name_str, "name=\'%s\'", AI_MCH_CD);
            break;
        case E_AI_IF_MCH_EF:
            sprintf(name_str, "name=\'%s\'", AI_MCH_EF);
            break;
        case E_AI_IF_MCH_VIR:
            sprintf(name_str, "name=\'%s\'", AI_MCH_VIR);
            break;
        default:
            PrintErr("Invalid ao interface(%d)\n", aio_if);
            return DEMO_FAIL;
    }

    // 0 equals E_AO_DETACH or E_AI_DETACH
    sprintf(value_str, "%d", 0);

    if(ao_sel != E_AO_DETACH){
        sprintf(value_str, "%d", ao_sel);
    }

    if(ai_sel != E_AI_DETACH){
        sprintf(value_str, "%d", ai_sel);
    }

    ret = aio_control_contents_set_by_name(card_id, name_str, value_str);
    if (ret == DEMO_SUCCESS)
    {
        PrintInfo("card id(%d), name(%s), value(%s)\n", card_id, name_str, value_str);
    }
    else
    {
        PrintErr("set control contents error!\n");
        ret = DEMO_FAIL;
    }
    FUNC_EXIT();
    return ret;
}

int ai_adc_a_enable(unsigned int card_id, bool enable)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_ADC_A);
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int ai_adc_b_enable(unsigned int card_id, bool enable)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_ADC_B);
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int ai_dmic_enable(unsigned int card_id, bool enable, unsigned int channels)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();

    if (enable == true)
    {
        if(channels > 0){
            ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_DMIC_A_01);
        }

        if(channels > 2){
            ret |= aio_control_attach(card_id, E_AI_IF_MCH_23, E_AO_DETACH, E_AI_DMIC_A_23);
        }

        if(channels > 4){
            ret |= aio_control_attach(card_id, E_AI_IF_MCH_45, E_AO_DETACH, E_AI_DMIC_A_45);
        }

        if(channels > 6){
            ret |= aio_control_attach(card_id, E_AI_IF_MCH_67, E_AO_DETACH, E_AI_DMIC_A_67);
        }

        if(channels > 8){
            PrintErr("Invalid dmic channels(%d)\n", channels);
            return DEMO_FAIL;
        }
    }
    else
    {
        // dmic max use 8 channel
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_DETACH);
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_23, E_AO_DETACH, E_AI_DETACH);
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_45, E_AO_DETACH, E_AI_DETACH);
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_67, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int ai_echo_enable(unsigned int card_id, bool enable)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_ECHO_01);
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int ai_hdmi_enable(unsigned int card_id, bool enable)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_HDMI_01);
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int ai_i2s_a_enable(unsigned int card_id, bool enable, unsigned int channels)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();

    if (enable == true)
    {
        if(channels > 0){
            ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_I2S_RXA_0_1);
        }

        if(channels > 2){
            ret |= aio_control_attach(card_id, E_AI_IF_MCH_23, E_AO_DETACH, E_AI_I2S_RXA_2_3);
        }

        if(channels > 4){
            ret |= aio_control_attach(card_id, E_AI_IF_MCH_45, E_AO_DETACH, E_AI_I2S_RXA_4_5);
        }

        if(channels > 6){
            ret |= aio_control_attach(card_id, E_AI_IF_MCH_67, E_AO_DETACH, E_AI_I2S_RXA_6_7);
        }

        if(channels > 8){
            PrintErr("Invalid i2s_a channels(%d)\n", channels);
            return DEMO_FAIL;
        }
    }
    else
    {
        // dmic max use 8 channel
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_DETACH);
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_23, E_AO_DETACH, E_AI_DETACH);
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_45, E_AO_DETACH, E_AI_DETACH);
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_67, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int ai_i2s_b_enable(unsigned int card_id, bool enable)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_I2S_RXB_0_1);
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int ao_dac_enable(unsigned int card_id, bool enable)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        ret |= aio_control_attach(card_id, E_AO_IF_DAC, E_AO_DMA, E_AI_DETACH);
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AO_IF_DAC, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int ao_echo_enable(unsigned int card_id, bool enable)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        ret |= aio_control_attach(card_id, E_AO_IF_ECHO, E_AO_DMA, E_AI_DETACH);
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AO_IF_ECHO, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int ao_spdif_enable(unsigned int card_id, bool enable, bool pcmType)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        if (pcmType == true)
        {
            // attach to LPCM
            ret |= aio_control_attach(card_id, E_AO_IF_SPDIF_TX, E_AO_LDMA, E_AI_DETACH);
        }
        else
        {
            // attach to NLPCM
            ret |= aio_control_attach(card_id, E_AO_IF_SPDIF_TX, E_AO_NLDMA, E_AI_DETACH);
        }
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AO_IF_SPDIF_TX, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int ao_i2s_a_enable(unsigned int card_id, bool enable)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        ret |= aio_control_attach(card_id, E_AO_IF_I2S_TX_A, E_AO_DMA, E_AI_DETACH);
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AO_IF_I2S_TX_A, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int ao_i2s_b_enable(unsigned int card_id, bool enable)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        ret |= aio_control_attach(card_id, E_AO_IF_I2S_TX_B, E_AO_DMA, E_AI_DETACH);
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AO_IF_I2S_TX_B, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int passthrough_ai_adc_a_enable(unsigned int card_id, bool enable)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_VIR, E_AO_DETACH, E_AI_ADC_A);
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_VIR, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int passthrough_ao_dac_enable(unsigned int card_id, bool enable)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        ret |= aio_control_attach(card_id, E_AO_IF_VIR_MUX, E_AO_DAC0, E_AI_DETACH);
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AO_IF_VIR_MUX, E_AO_VIR_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int passthrough_ao_spdif_enable(unsigned int card_id, bool enable, bool pcmType)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        if (pcmType == true)
        {
            // attach to LPCM
            ret |= aio_control_attach(card_id, E_AO_IF_VIR_MUX, E_AO_SPDIF_LPCM_TX, E_AI_DETACH);
        }
        else
        {
            // attach to NLPCM
            ret |= aio_control_attach(card_id, E_AO_IF_VIR_MUX, E_AO_SPDIF_NLPCM_TX, E_AI_DETACH);
        }
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AO_IF_VIR_MUX, E_AO_VIR_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

int aec_ai_adc_a_echo_enable(unsigned int card_id, bool enable)
{
    int ret = DEMO_SUCCESS;
    FUNC_ENTER();
    if (enable == true)
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_ADC_A);
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_23, E_AO_DETACH, E_AI_ECHO_01);
    }
    else
    {
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_01, E_AO_DETACH, E_AI_DETACH);
        ret |= aio_control_attach(card_id, E_AI_IF_MCH_23, E_AO_DETACH, E_AI_DETACH);
    }
    FUNC_EXIT();
    return ret;
}

void help()
{
    printf("usage: $ [usecase] -i [interface] [options]\n");
    printf("[usecase name]                                  [support interface]\n");
    printf("<playback>                                      <dac/echo_tx/spdif/i2s_a/i2s_b>\n");
    printf("<capture>                                       <adc_a/adc_b/dmic/echo_rx/hdmi_rx/i2s_a/i2s_b>\n");
    printf("<passthrough>                                   <adc_a-spdif/adc_a-dac>\n");
    printf("<passthrough_playback>                          <>\n");
    printf("<aec>                                           <spdif-adc_a-echo/dac-adc_a-echo>\n");
    printf("<compress>                                      <>\n");
    printf("options:\n");
    printf("-A | --card <capture card number>               The card to capture audio\n");
    printf("-a | --card <playback card number>              The card to playback audio\n");
    printf("-D | --device <capture device number>           The device to capture audio\n");
    printf("-d | --device <playback device number>          The device to playback audio\n");
    printf("-C | --Channel <capture channel>                The channel to capture audio\n");
    printf("-c | --Channel <playback channel>               The channel to playback audio\n");
    printf("-R | --Rate <capture sample rate>               The sample rate to capture audio\n");
    printf("-r | --Rate <playback sample rate>              The sample rate to playback audio\n");
    printf("-F | --file <capture file>                      The file name to capture audio\n");
    printf("-f | --file <playback file>                     The file name to playback audio\n");
    printf("-T | --time <capture time>                      The time to capture audio\n");
    printf("-t | --time <playback time>                     The time to playback audio\n");
    return;
}

static long parse_long(const char *str, int *err)
{
    FUNC_ENTER();
    long  val;
    char *endptr;

    errno = 0;
    val   = strtol(str, &endptr, 0);

    if (errno != 0 || *endptr != '\0')
    {
        *err = -1;
    }
    else
    {
        *err = 0;
    }
    FUNC_EXIT();
    return val;
}

static int close_flag = 0;

void stream_close(int sig)
{
    FUNC_ENTER();
    /* allow the stream to be closed gracefully */
    signal(sig, SIG_IGN);
    close_flag = 1;
    FUNC_EXIT();
}

void cmd_init(struct cmd *cmd)
{
    FUNC_ENTER();
    cmd->usecase_type = USECASE_PLAYBACK;
    cmd->interface = NULL;

    cmd->in_file_name = NULL;
    cmd->in_file_type = NULL;
    cmd->in_card_id   = 0;
    cmd->in_device_id = 0;
    cmd->in_channels  = 0;
    cmd->in_rate      = 0;
    cmd->in_times     = 0;

    cmd->out_file_name = NULL;
    cmd->out_file_type = NULL;
    cmd->out_card_id   = 0;
    cmd->out_device_id = 0;
    cmd->out_channels  = 0;
    cmd->out_rate      = 0;
    cmd->out_times     = 0;

    cmd->pt_in_out_channels = 0;
    cmd->pt_in_rate = 0;
    cmd->pt_out_rate = 0;
    cmd->dma_rate      = 0;
    FUNC_EXIT();
}

static int sample_ctx_deinit(struct ctx *ctx, struct cmd *cmd)
{
    FUNC_ENTER();
    int ret = DEMO_SUCCESS;
    // and then detach
    if(strcasecmp(cmd->interface, "adc_a") == 0)
    {
        ai_adc_a_enable(cmd->in_card_id, false);
    } 
    else if (strcasecmp(cmd->interface, "adc_b") == 0)
    {
        ai_adc_b_enable(cmd->in_card_id, false);
    }
    else if (strcasecmp(cmd->interface, "dmic") == 0)
    {
        ai_dmic_enable(cmd->in_card_id, false, cmd->in_channels);
    }
    else if (strcasecmp(cmd->interface, "echo_rx") == 0)
    {
        ai_echo_enable(cmd->in_card_id, false);
    }
    else if (strcasecmp(cmd->interface, "hdmi_rx") == 0)
    {
        ai_hdmi_enable(cmd->in_card_id, false);
    }
    else if (strcasecmp(cmd->interface, "i2s_a") == 0)
    {
        ai_i2s_a_enable(cmd->in_card_id, false, cmd->in_channels);
    }
    else if (strcasecmp(cmd->interface, "i2s_b") == 0)
    {
        ai_i2s_b_enable(cmd->in_card_id, false);
    }
    // for ao
    else if (strcasecmp(cmd->interface, "dac") == 0)
    {
        ao_dac_enable(cmd->out_card_id, false);
    }
    else if (strcasecmp(cmd->interface, "echo_tx") == 0)
    {
        ao_echo_enable(cmd->out_card_id, false);
    }
    else if (strcasecmp(cmd->interface, "spdif") == 0)
    {
        ao_spdif_enable(cmd->out_card_id, false, true);
    }
    else if (strcasecmp(cmd->interface, "i2s_a") == 0)
    {
        ao_i2s_a_enable(cmd->out_card_id, false);
    }
    else if (strcasecmp(cmd->interface, "i2s_b") == 0)
    {
        ao_i2s_b_enable(cmd->out_card_id, false);
    }
    // for passthrough
    else if (strcasecmp(cmd->interface, "adc_a-spdif") == 0)
    {
        passthrough_ai_adc_a_enable(cmd->in_card_id, false);
        passthrough_ao_spdif_enable(cmd->out_card_id, false, true);
    }
    else if (strcasecmp(cmd->interface, "adc_a-dac") == 0)
    {
        passthrough_ai_adc_a_enable(cmd->in_card_id, false);
        passthrough_ao_dac_enable(cmd->out_card_id, false);
    }
    // for aec
    else if (strcasecmp(cmd->interface, "spdif-adc_a-echo") == 0)
    {
        aec_ai_adc_a_echo_enable(cmd->in_card_id, false);
        ao_spdif_enable(cmd->out_card_id, false, true);
        ao_echo_enable(cmd->out_card_id, false);
    }
    else if (strcasecmp(cmd->interface, "dac-adc_a-echo") == 0)
    {
        aec_ai_adc_a_echo_enable(cmd->in_card_id, false);
        ao_dac_enable(cmd->out_card_id, false);
        ao_echo_enable(cmd->out_card_id, false);
    }
    else
    {
        PrintErr("error interface (%s)\n", cmd->interface);
        return DEMO_FAIL;
    }
    if(cmd->usecase_type == USECASE_CAPTURE || cmd->usecase_type == USECASE_PLAYBACK){

        if (cmd->in_file_type != NULL && strcmp(cmd->in_file_type, "wav") == 0)
        {
            if(cmd->usecase_type == USECASE_CAPTURE){
                /* write header now all information is known */
                ctx->header.data_sz = ctx->capture_frames * ctx->header.block_align;
                ctx->header.riff_sz = ctx->header.data_sz + sizeof(ctx->header) - 8;
                fseek(ctx->in_file, 0, SEEK_SET);
                fwrite(&ctx->header, sizeof(struct wav_header), 1, ctx->in_file);
            }
        }

        if ((ctx->in_file != NULL) && (cmd->usecase_type == USECASE_CAPTURE))
        {
            fclose(ctx->in_file);
        }

        if ((ctx->out_file != NULL) && (cmd->usecase_type == USECASE_PLAYBACK))
        {
            fclose(ctx->out_file);
        }
    }
    else if(cmd->usecase_type == USECASE_AEC)
    {
        if (cmd->in_file_type != NULL && strcmp(cmd->in_file_type, "wav") == 0)
        {
            /* write header now all information is known */
            ctx->header.data_sz = ctx->capture_frames * ctx->header.block_align;
            ctx->header.riff_sz = ctx->header.data_sz + sizeof(ctx->header) - 8;
            fseek(ctx->in_file, 0, SEEK_SET);
            fwrite(&ctx->header, sizeof(struct wav_header), 1, ctx->in_file);
        }

        if (ctx->in_file != NULL)
        {
            fclose(ctx->in_file);
        }

        if (ctx->out_file != NULL)
        {
            fclose(ctx->out_file);
        }
    }
    FUNC_EXIT();
    return ret;
}

static int sample_deinit(struct ctx *ctx, struct cmd *cmd)
{
    FUNC_ENTER();
    int ret = DEMO_SUCCESS;
    // need close first
    // snd_pcm_drain(pcm);

    if(cmd->usecase_type == USECASE_PLAYBACK){
        snd_pcm_close(ctx->out_pcm);
    }
    else if(cmd->usecase_type == USECASE_CAPTURE){
        snd_pcm_close(ctx->in_pcm);
    }
    else if(cmd->usecase_type == USECASE_PASSTHROUGH || cmd->usecase_type == USECASE_AEC) {
        snd_pcm_close(ctx->in_pcm);
        snd_pcm_close(ctx->out_pcm);
    }

    FUNC_EXIT();
    return ret;
}

static int sample_ctx_init(struct ctx *ctx, struct cmd *cmd)
{
    FUNC_ENTER();
    int  ret      = DEMO_SUCCESS;
    bool need_capture = false;
    bool need_playback = false;

    if(cmd->usecase_type == USECASE_CAPTURE)
    {
        need_capture = true;
    }
    if(cmd->usecase_type == USECASE_PLAYBACK)
    {
        need_playback = true;
    }
    if(cmd->usecase_type == USECASE_AEC)
    {
        need_capture = true;
        need_playback = true;
        // nedd add echo 2 channels
        cmd->in_channels += 2;
    }

    if(need_capture || need_playback)
    {
        if(need_capture){
            if (cmd->in_file_name == NULL)
            {
                PrintErr("file name not specified\n");
                return DEMO_FAIL;
            }
            // prepare file
            if (cmd->in_file_name != NULL && cmd->in_file_type == NULL && (cmd->in_file_type = strrchr(cmd->in_file_name, '.')) != NULL)
            {
                cmd->in_file_type++;
            }
            else
            {
                PrintErr("Invalid file(name:%s, type:%s).\n", cmd->in_file_name, cmd->in_file_type);
                help();
                return DEMO_FAIL;
            }
        }
        if(need_playback){
            if (cmd->out_file_name == NULL)
            {
                PrintErr("file name not specified\n");
                return DEMO_FAIL;
            }
            // prepare file
            if (cmd->out_file_name != NULL && cmd->out_file_type == NULL && (cmd->out_file_type = strrchr(cmd->out_file_name, '.')) != NULL)
            {
                cmd->out_file_type++;
            }
            else
            {
                PrintErr("Invalid file(name:%s, type:%s).\n", cmd->out_file_name, cmd->out_file_type);
                help();
                return DEMO_FAIL;
            }
        }

        if(need_capture){
            ctx->in_file = fopen(cmd->in_file_name, "wb");
            if (ctx->in_file == NULL)
            {
                PrintErr("failed to open '%s'\n", cmd->in_file_name);
                return DEMO_FAIL;
            }
        }
        if(need_playback){
            ctx->out_file = fopen(cmd->out_file_name, "rb");
            if (ctx->out_file == NULL)
            {
                PrintErr("failed to open '%s'\n", cmd->out_file_name);
                return DEMO_FAIL;
            }
        }

        if ((cmd->in_file_type != NULL && strcmp(cmd->in_file_type, "wav") == 0) || (cmd->out_file_type != NULL && strcmp(cmd->out_file_type, "wav") == 0))
        {
            if(need_capture){
                ctx->header.riff_id = ID_RIFF;
                ctx->header.riff_sz = 0;
                ctx->header.riff_fmt = ID_WAVE;
                ctx->header.fmt_id = ID_FMT;
                ctx->header.fmt_sz = 16;
                ctx->header.audio_format = FORMAT_PCM;
                ctx->header.num_channels = cmd->in_channels;
                ctx->header.sample_rate = cmd->in_rate;
                ctx->header.bits_per_sample = 16; // only support 16bit
                ctx->header.byte_rate = (ctx->header.bits_per_sample / 8) * cmd->in_channels * cmd->in_rate;
                ctx->header.block_align = cmd->in_channels * (ctx->header.bits_per_sample / 8);
                ctx->header.data_id = ID_DATA;
                ctx->in_frame_total_size = cmd->in_times * cmd->in_rate;
                //ctx->out_file = NULL;
                fseek(ctx->in_file, sizeof(struct wav_header), SEEK_SET);
            }

            if(need_playback){
                if (fread(&ctx->wave_header, sizeof(ctx->wave_header), 1, ctx->out_file) != 1)
                {
                    PrintErr("error: '%s' does not contain a riff/wave header\n", cmd->out_file_name);
                    fclose(ctx->out_file);
                    return DEMO_FAIL;
                }
                if ((ctx->wave_header.riff_id != ID_RIFF) || (ctx->wave_header.wave_id != ID_WAVE))
                {
                    PrintErr("error: '%s' is not a riff/wave file\n", cmd->out_file_name);
                    fclose(ctx->out_file);
                    return DEMO_FAIL;
                }
                unsigned int more_chunks = 1;
                do
                {
                    if (fread(&ctx->chunk_header, sizeof(ctx->chunk_header), 1, ctx->out_file) != 1)
                    {
                        PrintErr("error: '%s' does not contain a data chunk\n", cmd->out_file_name);
                        fclose(ctx->out_file);
                        return DEMO_FAIL;
                    }
                    switch (ctx->chunk_header.id)
                    {
                        case ID_FMT:
                            if (fread(&ctx->chunk_fmt, sizeof(ctx->chunk_fmt), 1, ctx->out_file) != 1)
                            {
                                PrintErr("error: '%s' has incomplete format chunk\n", cmd->out_file_name);
                                fclose(ctx->out_file);
                                return DEMO_FAIL;
                            }
                            /* If the format header is larger, skip the rest */
                            if (ctx->chunk_header.sz > sizeof(ctx->chunk_fmt))
                            {
                                fseek(ctx->out_file, ctx->chunk_header.sz - sizeof(ctx->chunk_fmt), SEEK_CUR);
                            }
                            break;
                        case ID_DATA:
                            /* Stop looking for chunks */
                            more_chunks = 0;
                            break;
                        default:
                            /* Unknown chunk, skip bytes */
                            fseek(ctx->out_file, ctx->chunk_header.sz, SEEK_CUR);
                            break;
                    }
                } while (more_chunks);
                cmd->out_channels = ctx->chunk_fmt.num_channels;
                cmd->out_rate     = ctx->chunk_fmt.sample_rate;
                ctx->out_frame_total_size = cmd->out_times * cmd->out_rate;
                //ctx->in_file = NULL;
            }
        }
        else if ((cmd->in_file_type != NULL && strcmp(cmd->in_file_type, "pcm") == 0) || (cmd->out_file_type != NULL && strcmp(cmd->out_file_type, "pcm") == 0))
        {
            if(need_capture){
                fseek(ctx->in_file, 0, SEEK_SET);
            }
            if(need_playback){
                fseek(ctx->out_file, 0, SEEK_SET);
            }
            if(need_capture && need_playback){
                fseek(ctx->in_file, 0, SEEK_SET);
                fseek(ctx->out_file, 0, SEEK_SET);
            }
        }
    }
    else if(cmd->usecase_type == USECASE_PASSTHROUGH) {
        cmd->pt_in_rate         = cmd->out_rate;
        cmd->dma_rate           = cmd->pt_in_rate;
        cmd->pt_in_out_channels = cmd->out_channels;
        cmd->pt_out_rate        = cmd->out_rate;
    }

    PrintInfo("interface(%s), in_file_name(%s), out_file_name(%s) \n", cmd->interface, cmd->in_file_name, cmd->out_file_name);
    PrintInfo("in: card(%u), device(%u) \n", cmd->in_card_id, cmd->in_device_id);
    PrintInfo("out: card(%u), device(%u) \n", cmd->out_card_id, cmd->out_device_id);
    PrintInfo("in: channels(%u), rate(%u) \n", cmd->in_channels, cmd->in_rate);
    PrintInfo("out: channels(%u), rate(%u) \n", cmd->out_channels, cmd->out_rate);
    PrintInfo("passthrough-in: channels(%u), rate(%u) \n", cmd->pt_in_out_channels, cmd->pt_in_rate);
    PrintInfo("passthrough-out: channels(%u), rate(%u) \n", cmd->pt_in_out_channels, cmd->pt_out_rate);

    if(cmd->usecase_type == USECASE_CAPTURE && (cmd->in_channels == 0 || cmd->in_rate == 0)){
        PrintErr("Error capture param !\n");
        ret = DEMO_FAIL;
    }

    if(cmd->usecase_type == USECASE_PLAYBACK && (cmd->out_channels == 0 || cmd->out_rate == 0)){
        PrintErr("Error playback param !\n");
        ret = DEMO_FAIL;
    }

    // passthrough only support 1 or 2 channels, out channel must equals in channel
    if(cmd->usecase_type == USECASE_PASSTHROUGH && (cmd->pt_in_out_channels > 2 || cmd->pt_in_rate == 0 || cmd->pt_out_rate == 0)){
        PrintErr("Error passthrough param cmd->pt_in_out_channels(%u) cmd->pt_in_rate(%u) cmd->pt_out_rate(%u)!\n", 
                cmd->pt_in_out_channels, cmd->pt_in_rate, cmd->pt_out_rate);
        ret = DEMO_FAIL;
    }

    if(cmd->usecase_type == USECASE_AEC && (cmd->in_channels == 2 || cmd->in_rate == 0 || cmd->out_channels == 0 || cmd->out_rate == 0)){
        PrintErr("Error aec param !\n");
        ret = DEMO_FAIL;
    }

    if(cmd->in_card_id > (CARD_NUM-1) || cmd->in_device_id > (DEVICE_NUM-1) ||
        cmd->out_card_id > (CARD_NUM-1) || cmd->out_device_id > (DEVICE_NUM-1)){
        PrintErr("Error device/card param !\n");
        ret = DEMO_FAIL;
    }

    FUNC_EXIT();
    return ret;
}

static int sample_init(struct ctx *ctx, struct cmd *cmd)
{
    FUNC_ENTER();
    int  ret      = DEMO_SUCCESS;

    // need attach first
    if(strcasecmp(cmd->interface, "adc_a") == 0)
    {
        ai_adc_a_enable(cmd->in_card_id, true);
    } 
    else if (strcasecmp(cmd->interface, "adc_b") == 0)
    {
        ai_adc_b_enable(cmd->in_card_id, true);
    }
    else if (strcasecmp(cmd->interface, "dmic") == 0)
    {
        ai_dmic_enable(cmd->in_card_id, true, cmd->in_channels);
    }
    else if (strcasecmp(cmd->interface, "echo_rx") == 0)
    {
        ai_echo_enable(cmd->in_card_id, true);
    }
    else if (strcasecmp(cmd->interface, "hdmi_rx") == 0)
    {
        ai_hdmi_enable(cmd->in_card_id, true);
    }
    else if (strcasecmp(cmd->interface, "i2s_a") == 0)
    {
        ai_i2s_a_enable(cmd->in_card_id, true, cmd->in_channels);
    }
    else if (strcasecmp(cmd->interface, "i2s_b") == 0)
    {
        ai_i2s_b_enable(cmd->in_card_id, true);
    }
    // for ao
    else if (strcasecmp(cmd->interface, "dac") == 0)
    {
        ao_dac_enable(cmd->out_card_id, true);
    }
    else if (strcasecmp(cmd->interface, "echo_tx") == 0)
    {
        ao_echo_enable(cmd->out_card_id, true);
    }
    else if (strcasecmp(cmd->interface, "spdif") == 0)
    {
        ao_spdif_enable(cmd->out_card_id, true, true);
    }
    else if (strcasecmp(cmd->interface, "i2s_a") == 0)
    {
        ao_i2s_a_enable(cmd->out_card_id, true);
    }
    else if (strcasecmp(cmd->interface, "i2s_b") == 0)
    {
        ao_i2s_b_enable(cmd->out_card_id, true);
    }
    // for passthrough
    else if (strcasecmp(cmd->interface, "adc_a-spdif") == 0)
    {
        passthrough_ai_adc_a_enable(cmd->in_card_id, true);
        passthrough_ao_spdif_enable(cmd->out_card_id, true, true);
    }
    else if (strcasecmp(cmd->interface, "adc_a-dac") == 0)
    {
        passthrough_ai_adc_a_enable(cmd->in_card_id, true);
        passthrough_ao_dac_enable(cmd->out_card_id, true);
    }
    // for aec
    else if (strcasecmp(cmd->interface, "spdif-adc_a-echo") == 0)
    {
        aec_ai_adc_a_echo_enable(cmd->in_card_id, true);
        ao_spdif_enable(cmd->out_card_id, true, true);
        ao_echo_enable(cmd->out_card_id, true);
    }
    else if (strcasecmp(cmd->interface, "dac-adc_a-echo") == 0)
    {
        aec_ai_adc_a_echo_enable(cmd->in_card_id, true);
        ao_dac_enable(cmd->out_card_id, true);
        ao_echo_enable(cmd->out_card_id, true);
    }
    else
    {
        PrintErr("error interface (%s)\n", cmd->interface);
        return DEMO_FAIL;
    }

    // and then open
    if(cmd->usecase_type == USECASE_PLAYBACK){
        aio_open(ctx, SND_PCM_STREAM_PLAYBACK, cmd->out_card_id, cmd->out_device_id, cmd->out_channels, cmd->out_rate, 0);
    }
    else if(cmd->usecase_type == USECASE_CAPTURE){
        aio_open(ctx, SND_PCM_STREAM_CAPTURE, cmd->in_card_id, cmd->in_device_id, cmd->in_channels, cmd->in_rate, 0);
    }
    else if(cmd->usecase_type == USECASE_PASSTHROUGH) {
        aio_open(ctx, SND_PCM_STREAM_CAPTURE, cmd->in_card_id, cmd->in_device_id, cmd->pt_in_out_channels, cmd->pt_in_rate, 0);
        aio_open(ctx, SND_PCM_STREAM_PLAYBACK, cmd->out_card_id, cmd->out_device_id, cmd->pt_in_out_channels, cmd->pt_out_rate, cmd->dma_rate);
    }
    else if(cmd->usecase_type == USECASE_AEC) {
        aio_open(ctx, SND_PCM_STREAM_PLAYBACK, cmd->out_card_id, cmd->out_device_id, cmd->out_channels, cmd->out_rate, 0);
        aio_open(ctx, SND_PCM_STREAM_CAPTURE, cmd->in_card_id, cmd->in_device_id, cmd->in_channels, cmd->in_rate, 0);
    }

    FUNC_EXIT();
    return ret;
}

static int sample_passthrough(void)
{
    /* catch ctrl-c to shutdown cleanly */
    signal(SIGINT, stream_close);
    while (!close_flag) {
        PrintInfo("passthrough running ...\n");
        sleep(3);
    }
    return 0;
}

static int sample_capture(struct ctx *ctx)
{
    FUNC_ENTER();
    int    ret = DEMO_SUCCESS;
    int rc;
    char *buffer;
    unsigned int size;
    unsigned int total_frames_read;
    unsigned int bytes_per_frame;
    snd_pcm_uframes_t frames = snd_pcm_bytes_to_frames(ctx->in_pcm, DEFAULT_PERIOD_SIZE * DEFAULT_PERIOD_COUNT);

    size = snd_pcm_frames_to_bytes(ctx->in_pcm, frames);
    buffer = malloc(size);
    if (!buffer) {
        PrintErr("unable to allocate %zu bytes\n", size);
        return -1;
    }

    bytes_per_frame = snd_pcm_frames_to_bytes(ctx->in_pcm, 1);
    total_frames_read = 0;
    if(ctx->in_frame_total_size == 0)
    {
        PrintErr("pcm null,Please set the recording time .\n");
        goto FAIL;
    }
    PrintInfo("in_frame size(%u) buffer size(%u), frames(%u) bytes_per_frame(%u) \n", ctx->in_frame_total_size, size, frames, bytes_per_frame);
    /* catch ctrl-c to shutdown cleanly */
    signal(SIGINT, stream_close);

    while (!close_flag) {
        if (ctx->in_pcm) {
            rc = snd_pcm_readi(ctx->in_pcm, (void*)buffer, frames);
            if (rc == -EPIPE) {
                /* EPIPE means overrun */
                PrintErr("overrun occurred\n");
                snd_pcm_prepare(ctx->in_pcm);
            } else if (rc < 0) {
                PrintErr("error from read: %s\n", snd_strerror(rc));
                break;
            } else if (rc != (int)frames) {
                PrintErr("short read, read %d frames\n", rc);
            } else {
                total_frames_read += frames;
                if (fwrite(buffer, bytes_per_frame, frames, ctx->in_file) != frames) {
                    PrintErr("Error capturing sample\n");
                    break;
                }
                PrintInfo("total_read data(%u), read frames(%u) \n", total_frames_read * bytes_per_frame, frames * bytes_per_frame);
                if(total_frames_read > ctx->in_frame_total_size)
                {
                    break;
                }
            }
        } else {
            PrintErr("pcm null.\n");
            ret = E_ERR_NOT_INIT;
            break;
        }
    }

FAIL:
    free(buffer);
    FUNC_EXIT();
    return 0;
}

static int sample_playback(struct ctx *ctx)
{
    FUNC_ENTER();
    int    ret = DEMO_SUCCESS;
    char * buffer;
    size_t buffer_size         = 0;
    size_t num_read            = 0;
    size_t remaining_data_size = ctx->chunk_header.sz;
    size_t played_data_size    = 0;
    size_t read_size           = 0;

    // frames to bytes
    buffer_size = snd_pcm_frames_to_bytes(ctx->out_pcm, DEFAULT_PERIOD_SIZE*DEFAULT_PERIOD_COUNT);
    buffer      = (char *)malloc(buffer_size);
    if (!buffer)
    {
        PrintErr("unable to allocate %zu bytes\n", buffer_size);
        return -1;
    }

    /* catch ctrl-c to shutdown cleanly */
    signal(SIGINT, stream_close);

    if(ctx->out_frame_total_size != 0)
    {
        remaining_data_size = ctx->out_frame_total_size * 4;
    }
    PrintInfo("data size(%u), buffer_size(%u)\n", remaining_data_size, buffer_size);

    do
    {
        memset(buffer, 0, sizeof(buffer));
        read_size                = remaining_data_size > buffer_size ? buffer_size : remaining_data_size;
        num_read                 = fread(buffer, 1, read_size, ctx->out_file);
        snd_pcm_uframes_t frames = snd_pcm_bytes_to_frames(ctx->out_pcm, num_read);
        PrintInfo("remaining data size(%u), write frames(%u) \n", remaining_data_size, frames);
        if (num_read > 0)
        {
            if (ctx->out_pcm)
            {
                int ret = snd_pcm_writei(ctx->out_pcm, (void *)buffer, frames);
                if (ret == -EPIPE)
                {
                    PrintErr("underrun occured\n");
                }
                else if (ret < 0)
                {
                    PrintErr("error from writei (%d:%s)\n", ret, snd_strerror(ret));
                    break;
                }
            }
            else
            {
                PrintErr("pcm null.\n");
                break;
            }

            remaining_data_size -= num_read;
            played_data_size += snd_pcm_frames_to_bytes(ctx->out_pcm, frames);
        }
    } while (!close_flag && num_read > 0 && remaining_data_size > 0);

    free(buffer);
    PrintInfo("Played (%zu) bytes. Remains (%zu) bytes.\n", played_data_size, remaining_data_size);

    FUNC_EXIT();
    return 0;
}

void* sample_passthrough_playback(void* data)
{
    FUNC_ENTER();
    struct ctx* ctx = (struct ctx*)data;
    sample_playback(ctx);
    FUNC_EXIT();
    return NULL;
}

void* sample_aec_capture(void* data)
{
    FUNC_ENTER();
    struct ctx* ctx = (struct ctx*)data;
    sample_capture(ctx);
    FUNC_EXIT();
    return NULL;
}

void* sample_aec_playback(void* data)
{
    FUNC_ENTER();
    struct ctx* ctx = (struct ctx*)data;
    sample_playback(ctx);
    FUNC_EXIT();
    return NULL;
}

int main(int argc, char *argv[])
{
    int    ret = DEMO_SUCCESS;
    int                  c = 0;
    char *usecase_type_str;
    struct cmd           cmd;
    struct ctx           ctx;
    struct optparse      opts;
    struct optparse_long long_options[] = {{"in_card", 'A', OPTPARSE_REQUIRED},
                                           {"out_card", 'a', OPTPARSE_REQUIRED},
                                           {"in_device", 'D', OPTPARSE_REQUIRED},
                                           {"out_device", 'd', OPTPARSE_REQUIRED},
                                           {"interface", 'i', OPTPARSE_REQUIRED},
                                           {"in_file", 'F', OPTPARSE_REQUIRED},
                                           {"out_file", 'f', OPTPARSE_REQUIRED},
                                           {"in_channels", 'C', OPTPARSE_REQUIRED},
                                           {"out_channels", 'c', OPTPARSE_REQUIRED},
                                           {"in_rate", 'R', OPTPARSE_REQUIRED},
                                           {"out_rate", 'r', OPTPARSE_REQUIRED},
                                           {"in_time", 'T', OPTPARSE_REQUIRED},
                                           {"out_time", 't', OPTPARSE_REQUIRED},
                                           {"help", 'h', OPTPARSE_NONE},

                                           {0, 0, 0}};

    FUNC_ENTER();
    if (argc < 2) {
        help();
        return -1;
    }

    cmd_init(&cmd);
    optparse_init(&opts, argv);

    // parsing command line
    while ((c = optparse_long(&opts, long_options, NULL)) != -1)
    {
        switch (c)
        {
            case 'h':
                help();
                return 0;
            case 'A':
                if (sscanf(opts.optarg, "%u", &cmd.in_card_id) != 1)
                {
                    PrintErr("failed parsing in_card_id number '%s'\n", argv[1]);
                    return 1;
                }
                break;
            case 'a':
                if (sscanf(opts.optarg, "%u", &cmd.out_card_id) != 1)
                {
                    PrintErr("failed parsing out_card_id number '%s'\n", argv[1]);
                    return 1;
                }
                break;
            case 'D':
                if (sscanf(opts.optarg, "%u", &cmd.in_device_id) != 1)
                {
                    PrintErr("failed parsing in_device_id number '%s'\n", argv[1]);
                    return 1;
                }
                break;
            case 'd':
                if (sscanf(opts.optarg, "%u", &cmd.out_device_id) != 1)
                {
                    PrintErr("failed parsing out_device_id number '%s'\n", argv[1]);
                    return 1;
                }
                break;
            case 'C':
                if (sscanf(opts.optarg, "%u", &cmd.in_channels) != 1)
                {
                    PrintErr("failed parsing in_channels number '%s'\n", argv[1]);
                    return 1;
                }
                break;
            case 'c':
                if (sscanf(opts.optarg, "%u", &cmd.out_channels) != 1)
                {
                    PrintErr("failed parsing out_channels number '%s'\n", argv[1]);
                    return 1;
                }
                break;
            case 'R':
                if (sscanf(opts.optarg, "%u", &cmd.in_rate) != 1)
                {
                    PrintErr("failed parsing in_rate number '%s'\n", argv[1]);
                    return 1;
                }
                break;
            case 'r':
                if (sscanf(opts.optarg, "%u", &cmd.out_rate) != 1)
                {
                    PrintErr("failed parsing out_rate number '%s'\n", argv[1]);
                    return 1;
                }
                break;
            case 'i':
                cmd.interface = opts.optarg;
                break;
            case 'F':
                cmd.in_file_name = opts.optarg;
                break;
            case 'f':
                cmd.out_file_name = opts.optarg;
                break;
            case 't':
                if (sscanf(opts.optarg, "%u", &cmd.out_times) != 1)
                {
                    PrintErr("failed parsing in_card_id number '%s'\n", argv[1]);
                    return 1;
                }
                break;
            case 'T':
                if (sscanf(opts.optarg, "%u", &cmd.in_times) != 1)
                {
                    PrintErr("failed parsing in_card_id number '%s'\n", argv[1]);
                    return 1;
                }
                break;
            default:
            {
                PrintErr("Invalid switch or option -%c needs an argument.\n", c);
                help();
                return 1;
            }
        }
    }
    usecase_type_str = optparse_arg(&opts);
    if(strcasecmp(usecase_type_str, "playback") == 0)
    {
        cmd.usecase_type = USECASE_PLAYBACK;
    }
    else if (strcasecmp(usecase_type_str, "capture") == 0)
    {
        cmd.usecase_type = USECASE_CAPTURE;
    }
    else if (strcasecmp(usecase_type_str, "passthrough") == 0)
    {
        cmd.usecase_type = USECASE_PASSTHROUGH;
    }
    else if (strcasecmp(usecase_type_str, "passthrough_playback") == 0)
    {
        // voip
        cmd.usecase_type = USECASE_PASSTHROUGH_PLAYBACK;
    }
    else if (strcasecmp(usecase_type_str, "aec") == 0)
    {
        cmd.usecase_type = USECASE_AEC;
    }
    else if (strcasecmp(usecase_type_str, "compress") == 0)
    {
        // spdif + nlpcm
        cmd.usecase_type = USECASE_COMPRESS;
    }

    if (!cmd.interface || (!cmd.in_file_name && cmd.usecase_type == USECASE_CAPTURE) || (!cmd.out_file_name && cmd.usecase_type == USECASE_PLAYBACK))
    {
        PrintErr("Invalid interface or file.\n");
        help();
        return 1;
    }

    ret = sample_ctx_init(&ctx, &cmd);
    if(ret < 0){
        PrintErr("sample_ctx_init error!\n");
        return DEMO_FAIL;
    }

    sample_init(&ctx, &cmd);

    if (cmd.usecase_type == USECASE_PLAYBACK)
    {
        sample_playback(&ctx);
    }
    else if(cmd.usecase_type == USECASE_CAPTURE)
    {
        sample_capture(&ctx);
    }
    else if(cmd.usecase_type == USECASE_PASSTHROUGH)
    {
        sample_passthrough();
    }
    else if(cmd.usecase_type == USECASE_PASSTHROUGH_PLAYBACK)
    {
        sample_passthrough_playback(&ctx);
    }
    else if(cmd.usecase_type == USECASE_AEC)
    {
        pthread_create(&ctx.playback_thread[cmd.out_card_id], NULL, sample_aec_playback, &ctx);
        pthread_create(&ctx.capture_thread[cmd.in_card_id], NULL, sample_aec_capture, &ctx);

        pthread_join(ctx.playback_thread[cmd.out_card_id], NULL);
        pthread_join(ctx.capture_thread[cmd.in_card_id], NULL);
    }

    sample_deinit(&ctx, &cmd);

    sample_ctx_deinit(&ctx, &cmd);

    FUNC_EXIT();
    return 0;
}