//#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include <libavutil/avutil.h>
#include <libavutil/attributes.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
#include <libswresample/swresample.h>

#include <alloca.h>
#include <alsa/asoundlib.h>


#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000
#define PERIOD_SIZE 1024
#define DECODE_TO_ALSA_BUF_LEN (PERIOD_SIZE * 128)

#define DUMP_FILE
#define WAV_SAMPLERATE                44100

#define ExecFunc(result, value)\
    if (result != value)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__, __LINE__);\
        return -1;\
    }\

	
typedef int QDateType; //队列存储数据类型

typedef struct QueueNode //队列元素节点
{
	QDateType val;
	struct QueueNode* next;
}QueueNode;

typedef	struct Queue //队列
{
	QueueNode* head;
	QueueNode* tail;
}Queue;

typedef struct {
    int videoindex;
    int sndindex;
    AVFormatContext* pFormatCtx;
    AVCodecContext* sndCodecCtx;
    AVCodec* sndCodec;
    SwrContext *swr_ctx;
    DECLARE_ALIGNED(16,uint8_t,audio_buf) [AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];
}AudioState;

 //下面这四个结构体是为了分析wav头的
typedef struct {
    u_int magic;      /* 'RIFF' */
    u_int length;     /* filelen */
    u_int type;       /* 'WAVE' */
} WaveHeader;

typedef struct {
    u_short format;       /* see WAV_FMT_* */
    u_short channels;
    u_int sample_fq;      /* frequence of sample */
    u_int byte_p_sec;
    u_short byte_p_spl;   /* samplesize; 1 or 2 bytes */
    u_short bit_p_spl;    /* 8, 12 or 16 bit */
} WaveFmtBody;

typedef struct {
    u_int type;        /* 'data' */
    u_int length;      /* samplecount */
} WaveChunkHeader;

#define COMPOSE_ID(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define WAV_RIFF COMPOSE_ID('R','I','F','F')
#define WAV_WAVE COMPOSE_ID('W','A','V','E')
#define WAV_FMT COMPOSE_ID('f','m','t',' ')
#define WAV_DATA COMPOSE_ID('d','a','t','a')

static bool g_bExit = false;
static bool g_bPlayThreadRun = false;
static bool g_bDecodeDone = false;
static pthread_t g_playThread = 0;
static int g_samplerate = 0; 
int g_volume = 50;


#ifdef DUMP_FILE
static FILE *fp = NULL;
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
    char chanel_size;
    char frame_size;
}alsa_init_type;

alsa_init_type stAlsaInit;

int set_volume(long outvol)
{
    snd_mixer_t* handle;
    snd_mixer_elem_t* elem;
    snd_mixer_selem_id_t* sid;

    static const char* mix_name = "DAC_0";
    static const char* card = "default";

    if(outvol < 0 || outvol > 100)
    {
        printf("volume adjust range 0 ~ 100\n\r");
        return - 1;
    }
    outvol = outvol * 1023 / 100;

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
    if(snd_mixer_selem_set_playback_volume(elem, 0, outvol) < 0) {
        snd_mixer_close(handle);
        return -8;
    }
    if(snd_mixer_selem_set_playback_volume(elem, 1, outvol) < 0) {
        snd_mixer_close(handle);
        return -9;
    }

    elem = snd_mixer_elem_next(elem);
    if(snd_mixer_selem_set_playback_volume(elem, 0, outvol) < 0) {
        snd_mixer_close(handle);
        return -10;
    }
    if(snd_mixer_selem_set_playback_volume(elem, 1, outvol) < 0) {
        snd_mixer_close(handle);
        return -11;
    }

    printf("set volume\n\r");
    snd_mixer_close(handle);
    return 0;
}

//set control contents for one control
int ao_dac_attach(char *value)
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

    if(snd_ctl_ascii_elem_id_parse(id, "name=\'DAC_SEL\'"))
    {
        return -EINVAL;
    }
    if(handle == NULL && (err = snd_ctl_open(&handle, card, 0)) < 0)
    {
        return err;
    }

    snd_ctl_elem_info_set_id(info, id);
    if(err = snd_ctl_elem_info(handle, info) < 0)
    {
        snd_ctl_close(handle);
        handle = NULL;
        return err;
    }
    snd_ctl_elem_info_get_id(info, id);
    snd_ctl_elem_value_set_id(control, id);
    if((err = snd_ctl_elem_read(handle, control)) < 0)
    {
        snd_ctl_close(handle);
        handle = NULL;
        return err;
    }
    err = snd_ctl_ascii_value_parse(handle, control, info, value);
    if(err < 0)
    {
        snd_ctl_close(handle);
        handle = NULL;
        return err;
    }

    if((err = snd_ctl_elem_write(handle, control)) < 0)
    {
        snd_ctl_close(handle);
        handle = NULL;
        return err;
    }

    snd_ctl_close(handle);
    handle = NULL;
    return 0;

}

int AppPcmInit(alsa_init_type *stAlsaInit, char *snd_card_name)
{
    int err = 0;
    int dir = 0;
    ao_dac_attach("0");
    ao_dac_attach("1");

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
    err = snd_pcm_hw_params_set_channels(stAlsaInit->handle, stAlsaInit->params, stAlsaInit->chanel_size);
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
	err = snd_pcm_hw_params_set_periods_near(stAlsaInit->handle, stAlsaInit->params, &stAlsaInit->period_count, &dir);
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
	
    printf("----->System configure info\n");
    printf("Alsa lib version %d.%d.%d\n", (SND_LIB_VERSION >> 16 & 0xff), (SND_LIB_VERSION >> 8 & 0xff), (SND_LIB_VERSION >> 0 & 0xff));
    return err;
}

int AppPlayDataStream(snd_pcm_t* pst_handle, char* p_databuff, snd_pcm_uframes_t send_frames)
{
    int r = 0;
    // printf("send_frames == %ld\n",send_frames);

    r = snd_pcm_writei(pst_handle, p_databuff, send_frames);
      if (r == -EPIPE)
    {
        /* EPIPE means overrun */
        fprintf(stderr, "underrun occurred\n");
        snd_pcm_prepare(pst_handle);
    }
    else if (r < 0)
    {
        fprintf(stderr, "error from write: %s\n", snd_strerror(r));
    }
    else if (r != (int)send_frames) {
        fprintf(stderr, "short write, read %d frames\n", r);
    }
    return r;
}

void AppTurnOffPcm(snd_pcm_t* pst_handle)
{
    printf("process end\n\r");
    snd_pcm_drain(pst_handle);
    snd_pcm_close(pst_handle);
    snd_pcm_hw_free(pst_handle);
    ao_dac_attach("0");
}


int insert_wave_header(FILE* fp, long data_len, int sampleRate, int chnCnt)
{
    int len;
    WaveHeader* header;
    WaveChunkHeader* chunk;
    WaveFmtBody* body;

    fseek(fp, 0, SEEK_SET);        //写到wav文件的开始处

    len = sizeof(WaveHeader)+sizeof(WaveFmtBody)+sizeof(WaveChunkHeader)*2;
    char* buf = (char*)malloc(len);
    header = (WaveHeader*)buf;
    header->magic = WAV_RIFF;
    header->length = data_len + sizeof(WaveFmtBody)+sizeof(WaveChunkHeader)*2 + 4;
    header->type = WAV_WAVE;

    chunk = buf+sizeof(WaveHeader);
    chunk->type = WAV_FMT;
    chunk->length = 16;

    body = (WaveFmtBody*)(buf+sizeof(WaveHeader)+sizeof(WaveChunkHeader));
    body->format = (u_short)0x0001;              //编码方式为pcm
    body->channels = (u_short)chnCnt;             //(u_short)0x02;      //声道数为2
    body->sample_fq = sampleRate;                 //44100;             //采样频率为44.1k
    body->byte_p_sec = sampleRate * chnCnt * 2;    //176400;           //每秒所需字节数 44100*2*2=采样频率*声道*采样位数
    body->byte_p_spl = (u_short)0x4;             //对齐无意义
    body->bit_p_spl = (u_short)16;               //采样位数16bit=2Byte


    chunk = (WaveChunkHeader*)(buf+sizeof(WaveHeader)+sizeof(WaveChunkHeader)+sizeof(WaveFmtBody));
    chunk->type = WAV_DATA;
    chunk->length = data_len;

    fwrite(buf, 1, len, fp);
    free(buf);
    return 0;
}

void *mp3DecodeProc(void *pParam)
{
    AudioState *is = (AudioState *)pParam;
    AVPacket *packet = av_mallocz(sizeof(AVPacket));
    AVFrame *frame = av_frame_alloc();
    uint8_t *out[] = { is->audio_buf };
    int data_size = 0, got_frame = 0;
    int wavDataLen = 0;
	int wavDataSize = 0;
	int SendDataSize = 0;
#ifdef DUMP_FILE
    int file_data_size = 0;
#endif
    static bool g_bPcmDevInit = false;
	char * pbuffer = (char*)malloc(DECODE_TO_ALSA_BUF_LEN);
	int buffer_head = 0;
	int buffer_tail = 0;

    while(g_bPlayThreadRun)    //1.2 循环读取mp3文件中的数据帧
    {
        if (av_read_frame(is->pFormatCtx, packet) < 0)
            break;

        if(packet->stream_index != is->sndindex)
            continue;
        if(avcodec_decode_audio4(is->sndCodecCtx, frame, &got_frame, packet) < 0) //1.3 解码数据帧
        {
            printf("file eof");
            break;
        }

        if(got_frame <= 0) /* No data yet, get more frames */
            continue;
        data_size = av_samples_get_buffer_size(NULL, is->sndCodecCtx->channels, frame->nb_samples, is->sndCodecCtx->sample_fmt, 1);

        //1.4下面将ffmpeg解码后的数据帧转为我们需要的数据(关于"需要的数据"下面有解释)
        if(NULL==is->swr_ctx)
        {
            if(is->swr_ctx != NULL)
                swr_free(&is->swr_ctx);
            printf("frame: channnels=%d,format=%d, sample_rate=%d\n", frame->channels, frame->format, frame->sample_rate);
            is->swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, WAV_SAMPLERATE, av_get_default_channel_layout(frame->channels), frame->format, frame->sample_rate, 0, NULL);
            if(is->swr_ctx == NULL)
            {
                printf("swr_ctx == NULL");
            }
            swr_init(is->swr_ctx);
        }
        wavDataLen = swr_convert(is->swr_ctx, out, WAV_SAMPLERATE, (const uint8_t **)frame->extended_data, frame->nb_samples);
		//printf("wavDataLen = %d\n", wavDataLen);

        if(g_bPcmDevInit == false)
        {
            printf("frame->nb_samples = %d, channnels=%d, format=%d, sample_rate=%d\n", frame->nb_samples, frame->channels, frame->format, frame->sample_rate);
            printf("data_size = %d\n",data_size);
            g_bPcmDevInit = true;
            stAlsaInit.stream_type = SND_PCM_STREAM_PLAYBACK;
            stAlsaInit.handle = NULL;
            stAlsaInit.params = NULL;
            stAlsaInit.sample_size = 2;
            stAlsaInit.sample_rate = WAV_SAMPLERATE;
            stAlsaInit.chanel_size = 2; 
            stAlsaInit.access_type = SND_PCM_ACCESS_RW_INTERLEAVED;
            stAlsaInit.pcm_format = SND_PCM_FORMAT_S16_LE;
            stAlsaInit.period_frames = PERIOD_SIZE; 
			stAlsaInit.period_count = 16; 
			stAlsaInit.start_threshold = stAlsaInit.period_frames; 
			stAlsaInit.stop_threshold = (stAlsaInit.period_count) * stAlsaInit.period_frames;
            AppPcmInit(&stAlsaInit, "default");
			SendDataSize = PERIOD_SIZE * stAlsaInit.period_count * sizeof(short) * stAlsaInit.chanel_size;
            set_volume(g_volume);
        }
		wavDataSize = wavDataLen * sizeof(short) * stAlsaInit.chanel_size;
		
		memcpy((void *)(pbuffer + buffer_tail), (void *)is->audio_buf, wavDataSize);
		buffer_tail += wavDataLen * sizeof(short) * stAlsaInit.chanel_size;

		while(buffer_tail - buffer_head > SendDataSize)
		{
			 AppPlayDataStream(stAlsaInit.handle, (char *)(pbuffer + buffer_head), PERIOD_SIZE * stAlsaInit.period_count); 
			 buffer_head += SendDataSize;
		}
		if(buffer_head != 0)
		{
			memmove((void *)pbuffer, (void *)pbuffer + buffer_head, buffer_tail - buffer_head);
			buffer_tail = buffer_tail - buffer_head;
			buffer_head = 0;
		}       

#ifdef DUMP_FILE
        file_data_size += wavDataLen;

        //1.5 数据格式转换完成后就写到文件中
        if (fp)
            fwrite((short *)is->audio_buf, sizeof(short), (size_t) wavDataLen * 2, fp);
#endif
    }

#ifdef DUMP_FILE
    file_data_size *= frame->channels * 2;            // 计算字节数,2chn & 16bits
    printf("file_data_size=%d", file_data_size);
    //第2步添加上wav的头
    insert_wave_header(fp, file_data_size, WAV_SAMPLERATE, 2);
    fclose(fp);
#endif

    if (is->swr_ctx != NULL)
        swr_free(&is->swr_ctx);
    av_free_packet(packet);
    av_free(frame);
    g_bDecodeDone = true;

    return NULL;
}

int init_ffplayer(AudioState* is, char* filepath)
{
    int i=0;
    int ret;
    is->sndindex = -1;

#ifdef DUMP_FILE
    int wavHeaderLen = 0;
#endif

    if(NULL == filepath)
    {
        printf("input file is NULL");
        return -1;
    }

    avcodec_register_all();
    //avfilter_register_all();
    av_register_all();

    is->pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&is->pFormatCtx, filepath, NULL, NULL)!=0)
        return -1;

    if(avformat_find_stream_info(is->pFormatCtx, NULL)<0)
        return -1;
    av_dump_format(is->pFormatCtx,0, 0, 0);
    is->videoindex = av_find_best_stream(is->pFormatCtx, AVMEDIA_TYPE_VIDEO, is->videoindex, -1, NULL, 0);
    is->sndindex = av_find_best_stream(is->pFormatCtx, AVMEDIA_TYPE_AUDIO,is->sndindex, is->videoindex, NULL, 0);
    printf("videoindex=%d, sndindex=%d\n", is->videoindex, is->sndindex);
    if(is->sndindex != -1)
    {
    	g_samplerate = is->pFormatCtx->streams[is->sndindex]->codec->sample_rate;
		printf("g_samplerate = %d\n", g_samplerate);
        is->sndCodecCtx = is->pFormatCtx->streams[is->sndindex]->codec;
        is->sndCodec = avcodec_find_decoder(is->sndCodecCtx->codec_id);
        if(is->sndCodec == NULL)
        {
            printf("Codec not found");
            return -1;
        }
        if(avcodec_open2(is->sndCodecCtx, is->sndCodec, NULL) < 0)
            return -1;
    }

#ifdef DUMP_FILE
    fp = fopen("./test.wav", "wb+");
    if (fp)
    {
        wavHeaderLen = sizeof(WaveHeader) + sizeof(WaveFmtBody) + sizeof(WaveChunkHeader) * 2;
        fseek(fp, wavHeaderLen, SEEK_SET);
        printf("wavHeaderLen=%d", wavHeaderLen);
    }
#endif

    g_bDecodeDone = false;
    g_bPlayThreadRun = true;
    ret = pthread_create(&g_playThread, NULL, mp3DecodeProc, (void *)is);
    if (ret != 0)
    {
        printf("pthread_create mp3DecodeProc failed!\n");
    }
    return 0;
}

void deinit_ffplayer(AudioState* is)
{
    g_bPlayThreadRun = false;

    if (g_playThread)
    {
        pthread_join(g_playThread, NULL);
        g_playThread = NULL;
    }

    avcodec_close(is->sndCodecCtx);
    avformat_close_input(&is->pFormatCtx);
}

void signalHandler(int signo)
{
    switch (signo)
    {
        case SIGINT:
            printf("catch exit signal\n");
            g_bExit = true;
            break;
        default:
            break;
    }
}

int main(int argc, char **argv)
{
    int ret = 0;
    AudioState* is = (AudioState*) av_mallocz(sizeof(AudioState));

    signal(SIGINT, signalHandler);

    if( (ret=init_ffplayer(is, argv[1])) != 0)
    {
        printf("init_ffmpeg error");
        goto uninstall;
    }

	if (argc < 2)
    {
        printf("please input a mp3 file!\n");
        printf("eg: ./mp3Player [file] [volume] , the default volume is -10\n");
        return -1;
    }

    if (argc > 2)
    {
        g_volume = atoi(argv[2]);
    }


    while (1)
    {
        if (g_bExit || g_bDecodeDone)
        {
            break;
        }

        usleep(30000);
    }

    deinit_ffplayer(is);

uninstall:
    AppTurnOffPcm(stAlsaInit.handle);

    av_free(is);

    return 0;
}
