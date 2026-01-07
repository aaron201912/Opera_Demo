#include "sstar_vdec.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "UtilS_SPS_PPS.h"
#include "mi_vdec.h"


//#define DUMP_VDEC_ES

#define MAX_ONE_FRM_SIZE (2 * 1024 * 1024)
#define DROP_SLICE_ID (3)

MI_VDEC_CodecType_e  _eCodecType = E_MI_VDEC_CODEC_TYPE_H264;
ReOrderSlice_t _stVdecSlice[100];
MI_U32 _u32SliceIdx = 0;
//static int bExit = 0;
MI_BOOL _bFrmMode = FALSE;      //根据输入视频是否带B帧设置
MI_BOOL _bReOrderSlice = FALSE;
MI_BOOL _bSleep = FALSE;
MI_BOOL _bDropFrm = FALSE;
static char file_path[128];



static pthread_t _g_tid_video;

MI_U64 getOsTime(void)
{
    MI_U64 u64CurTime = 0;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    u64CurTime = ((unsigned long long)(tv.tv_sec))*1000 + tv.tv_usec/1000;
    return u64CurTime;
}

/********************************************Vdec**************************************************/
NALU_t *AllocNALU(int buffersize)
{
    NALU_t *n;
    if ((n = (NALU_t*)calloc (1, sizeof (NALU_t))) == NULL)
    {
        printf("AllocNALU: calloc n error\n");
        return NULL;
    }
    n->max_size = buffersize;
    //printf("AllocNALU: calloc buf size = %d\n", n->max_size);

    if ((n->buf = (char*)calloc (buffersize, sizeof (char))) == NULL)
    {
        free (n);
        printf("AllocNALU: calloc n->buf error\n");
        return NULL;
    }
    return n;
}

void FreeNALU(NALU_t *n)
{
    if (n)
    {
        if (n->buf)
        {
            free(n->buf);
            n->buf=NULL;
        }
        free (n);
    }
}

int FindStartCode2 (unsigned char *Buf)
{
    if((Buf[0] != 0) || (Buf[1] != 0) || (Buf[2] != 1))
        return 0;
    else
        return 1;
}

int FindStartCode3 (unsigned char *Buf)
{
    if((Buf[0] != 0) || (Buf[1] != 0) || (Buf[2] != 0) || (Buf[3] != 1))
        return 0;
    else
        return 1;
}

int GetAnnexbNALU (NALU_t *nalu, MI_S32 chn, FILE *fp, int eExit)
{
    int pos = 0;
    int StartCodeFound, rewind;
    unsigned char *Buf;
    int info2 = 0, info3 = 0;

    if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL)
    {
        printf("GetAnnexbNALU: Could not allocate Buf memory\n");
        return -1;
    }

    nalu->startcodeprefix_len=3;
    if (3 != fread (Buf, 1, 3, fp))
    {
        free(Buf);
        return -1;
    }
    info2 = FindStartCode2 (Buf);
    if(info2 != 1)
    {
        if(1 != fread(Buf+3, 1, 1, fp))
        {
            free(Buf);
            return -1;
        }
        info3 = FindStartCode3 (Buf);
        if (info3 != 1)
        {
            free(Buf);
            return -1;
        }
        else
        {
            pos = 4;
            nalu->startcodeprefix_len = 4;
        }
    }
    else
    {
        nalu->startcodeprefix_len = 3;
        pos = 3;
    }
    StartCodeFound = 0;
    info2 = 0;
    info3 = 0;
    while (!StartCodeFound)
    {
        if (feof (fp))
        {
            nalu->len = (pos-1);
            memcpy (nalu->buf, &Buf[0], nalu->len);
            free(Buf);
            if (!eExit)
            {
                fseek(fp, 0, 0);
            }
            else
            {
                printf("end of file...\n");
            }
            return pos-1;
        }
        Buf[pos++] = fgetc (fp);
        info3 = FindStartCode3(&Buf[pos-4]);
        if(info3 != 1)
            info2 = FindStartCode2(&Buf[pos-3]);
        StartCodeFound = (info2 == 1 || info3 == 1);
    }
    rewind = (info3 == 1) ? -4 : -3;
    if (0 != fseek (fp, rewind, SEEK_CUR))
    {
        free(Buf);
        printf("GetAnnexbNALU: Cannot fseek in the bit stream file\n");
    }
    nalu->len = (pos+rewind);
    memcpy (nalu->buf, &Buf[0], nalu->len);
    free(Buf);
    return (pos+rewind);
}

void * sstar_video_thread(void* arg)
{
    MI_VDEC_CHN vdecChn = 0;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 s32TimeOutMs = 20;
    MI_U32 u32WrPtr = 0;
    MI_S32 s32NaluType = 0;
    MI_U8 *pu8Buf = NULL;
    MI_U8 *pSlice2FrmBuf = NULL;
    MI_U8 *pSliceReOrder = NULL;
    MI_U64 u64Pts = 0;
    MI_U64 u64FisrtSlicePushTime = 0;
    MI_BOOL bFirstFrm = FALSE;
    MI_BOOL frameType = 0;
    MI_U32 u32SliceCount = 0;
    MI_U32 u32FpBackLen = 0; // if send stream failed, file pointer back length
    FILE *fReadFile = NULL;
    NALU_t *pstNalu;
    MI_VDEC_VideoStream_t stVdecStream;
    buffer_object_t *buf_obj = (char *)arg;
    char * video_file = buf_obj->video_file;


    pSlice2FrmBuf = (unsigned char *)malloc(MAX_ONE_FRM_SIZE);
    if (!pSlice2FrmBuf) {
        printf("ST_VdecSendStream: malloc frame buf error\n");
        return NULL;
    }

    if (_bReOrderSlice) {
        pSliceReOrder = (unsigned char *)malloc(MAX_ONE_FRM_SIZE);
        if (!pSliceReOrder) {
            printf("ST_VdecSendStream: malloc reorder buf error\n");
            return NULL;
        }
    }

    //char *video_file = (char *)arg;

    pstNalu = AllocNALU(MAX_ONE_FRM_SIZE);
    if (!pstNalu) {
        return NULL;
    }
    //prctl(PR_SET_NAME, "send_thrd_0");
    printf("Open video file:%s  \n",video_file);
    fReadFile = fopen(video_file, "rb"); //ES
    if (!fReadFile)
    {
        printf("Open video file:%s failed! \n",video_file);
        return NULL;
    }
#ifdef DUMP_VDEC_ES
    FILE *h264_fd = fopen("/customer/h264_dump.es", "w+");
#endif

    pu8Buf = (MI_U8 *)malloc(MAX_ONE_FRM_SIZE);
    if (!pu8Buf) {
        printf("ST_VdecSendStream: malloc pbuf error\n");
        return NULL;
    }

    while (!buf_obj->bExit_second)
    {
        s32Ret = GetAnnexbNALU(pstNalu, vdecChn, fReadFile, buf_obj->bExit_second);
        if (s32Ret <= 0) {

            printf("GetAnnexbNALU: read nal data error\n");
            continue;
        }

        stVdecStream.pu8Addr = (MI_U8 *)pstNalu->buf;
        if(9 == pstNalu->len
            && 0 == *(pstNalu->buf)
            && 0 == *(pstNalu->buf+1)
            && 0 == *(pstNalu->buf+2)
            && 1 == *(pstNalu->buf+3)
            && 0x68 == *(pstNalu->buf+4)
            && 0 == *(pstNalu->buf+pstNalu->len-1))
        {
            stVdecStream.u32Len = 8;
        }
        else {
            stVdecStream.u32Len = pstNalu->len;
        }
        stVdecStream.u64PTS = u64Pts;
        stVdecStream.bEndOfFrame = 1;
        stVdecStream.bEndOfStream = 0;

        if ( feof (fReadFile))
        {
            stVdecStream.bEndOfStream = 1;
            printf("set end of file flag done\n");
        }

        u32FpBackLen = stVdecStream.u32Len; //back length
        if(0x00 == stVdecStream.pu8Addr[0] && 0x00 == stVdecStream.pu8Addr[1]
            && 0x00 == stVdecStream.pu8Addr[2] && 0x01 == stVdecStream.pu8Addr[3]
            && (0x65 == stVdecStream.pu8Addr[4] || 0x61 == stVdecStream.pu8Addr[4]
            || 0x26 == stVdecStream.pu8Addr[4] || 0x02 == stVdecStream.pu8Addr[4]
            || 0x41 == stVdecStream.pu8Addr[4]))
        {
            usleep(30 * 1000);    //帧率控制不超过30
        }

        if (_eCodecType == E_MI_VDEC_CODEC_TYPE_H265) {
            if (0x00 == stVdecStream.pu8Addr[0] && 0x00 == stVdecStream.pu8Addr[1] && 0x00 == stVdecStream.pu8Addr[2] && 0x01 == stVdecStream.pu8Addr[3]) {
                bFirstFrm = (stVdecStream.pu8Addr[6] & 0x80);
                s32NaluType = (stVdecStream.pu8Addr[4] & 0x7E) >> 1;
            }

            if (0x00 == stVdecStream.pu8Addr[0] && 0x00 == stVdecStream.pu8Addr[1] && 0x01 == stVdecStream.pu8Addr[2]) {
                bFirstFrm = (stVdecStream.pu8Addr[5] & 0x80);
                s32NaluType = (stVdecStream.pu8Addr[3] & 0x7E) >> 1;
            }

            if (s32NaluType <= 31) {
                ///frame type
                frameType = 1;
            } else {
                frameType = 0;
            }
        } else {
            if (0x00 == stVdecStream.pu8Addr[0] && 0x00 == stVdecStream.pu8Addr[1] && 0x00 == stVdecStream.pu8Addr[2] && 0x01 == stVdecStream.pu8Addr[3]) {
                bFirstFrm = (stVdecStream.pu8Addr[5] & 0x80);
                s32NaluType = stVdecStream.pu8Addr[4] & 0xF;
            }

            if (0x00 == stVdecStream.pu8Addr[0] && 0x00 == stVdecStream.pu8Addr[1] && 0x01 == stVdecStream.pu8Addr[2]) {
                bFirstFrm = (stVdecStream.pu8Addr[4] & 0x80);
                s32NaluType = stVdecStream.pu8Addr[3] & 0xF;
            }
            if (1 <= s32NaluType && s32NaluType <= 5) {
                ///frame type
                frameType = 1;
            } else {
                frameType = 0;
            }
            //printf("nal data[4]: %x, data[5]: %x, bFirstFrm: %d\n", stVdecStream.pu8Addr[4], stVdecStream.pu8Addr[5], bFirstFrm);
        }

        if (bFirstFrm) {
            u32SliceCount = 0;
        }

        if (u64FisrtSlicePushTime == 0) {
            u64FisrtSlicePushTime = getOsTime();
        }

        if (_bDropFrm && frameType && (getOsTime() - u64FisrtSlicePushTime > 3000) && (u32SliceCount == DROP_SLICE_ID)) {
            printf("drop slice, id=%d, 0x%02x, type:%d\n", u32SliceCount, stVdecStream.pu8Addr[4], s32NaluType);
            u32SliceCount++;
            continue;
        }

        if (_bFrmMode) {
            if (u32WrPtr && bFirstFrm) {
                MI_U8 *pTmp = stVdecStream.pu8Addr;
                MI_U32 u32TmpLen = stVdecStream.u32Len;

                stVdecStream.pu8Addr = pSlice2FrmBuf;
                stVdecStream.u32Len = u32WrPtr;

                if (_bReOrderSlice && frameType && (getOsTime() - u64FisrtSlicePushTime > 3000)) {
                    int WLen = 0;
                    for (int sliceID = 0; sliceID < _u32SliceIdx; ++sliceID) {
                        if (sliceID == 0) {
                            memcpy(pSliceReOrder, _stVdecSlice[0].pos, _stVdecSlice[0].len);
                            WLen += _stVdecSlice[0].len;
                        } else {
                            memcpy(pSliceReOrder + WLen, _stVdecSlice[_u32SliceIdx - sliceID].pos, _stVdecSlice[_u32SliceIdx - sliceID].len);
                            WLen += _stVdecSlice[_u32SliceIdx - sliceID].len;
                        }
                    }

                    stVdecStream.pu8Addr = pSliceReOrder;
                    //printf("reorder done...\n");
                }
                //printf("send data to vdec addr: %p, length: %d\n", stVdecStream.pu8Addr, stVdecStream.u32Len);
                if (MI_SUCCESS != (s32Ret = MI_VDEC_SendStream(0, vdecChn, &stVdecStream, s32TimeOutMs)))
                {
                    printf("chn[%d]: MI_VDEC_SendStream %d fail, 0x%X\n", vdecChn, stVdecStream.u32Len, s32Ret);
                    fseek(fReadFile, - u32FpBackLen, SEEK_CUR);
                    continue;
                } else {
                    stVdecStream.pu8Addr = pTmp;
                    stVdecStream.u32Len = u32TmpLen;
                    u32WrPtr = 0;
                    if (_bReOrderSlice) {
                        _u32SliceIdx = 0;
                    }
                }

                //usleep(30 * 1000);
            }

            memcpy(pSlice2FrmBuf + u32WrPtr, stVdecStream.pu8Addr, stVdecStream.u32Len);
            if (_bReOrderSlice) {
                _stVdecSlice[_u32SliceIdx].pos = pSlice2FrmBuf + u32WrPtr;
                _stVdecSlice[_u32SliceIdx].len = stVdecStream.u32Len;
                _u32SliceIdx++;
            }
            u32WrPtr += stVdecStream.u32Len;
            //printf("pSlice2FrmBuf addr %p, length %d\n", pSlice2FrmBuf, u32WrPtr);
        }
        else
        {

#ifdef DUMP_VDEC_ES

            fwrite(stVdecStream.pu8Addr, stVdecStream.u32Len, 1, h264_fd);
#endif
            //printf("send data to vdec addr: %p, length: %d\n", stVdecStream.pu8Addr, stVdecStream.u32Len);
            if (MI_SUCCESS != (s32Ret = MI_VDEC_SendStream(0, vdecChn, &stVdecStream, s32TimeOutMs)))
            {
                printf("chn[%d]: MI_VDEC_SendStream %d fail, 0x%X\n", vdecChn, stVdecStream.u32Len, s32Ret);
                fseek(fReadFile, - u32FpBackLen, SEEK_CUR);
                usleep(30 * 1000);
            }
        }

        if ( feof (fReadFile))
        {
            printf("end of stream, wait dec done...\n");
            usleep(5 * 1000 * 1000);
            buf_obj->bExit_second = 1;
        }

        u32SliceCount++;
    }
#ifdef DUMP_VDEC_ES
    fclose(h264_fd);
#endif
    free(pu8Buf);
    FreeNALU(pstNalu);
    fclose(fReadFile);
    free(pSlice2FrmBuf);
    printf("sstar_video_thread exit\n");

    return NULL;
}

int sstar_parse_video_info(char *filename, MI_S32 *width, MI_S32 *height, int *type)
{
    int seek_length;
    int find_sps_code = 0, find_pps_code = 0;
    int sps_pos = 0, pps_pos = 0;
    FILE *fp = NULL;
    unsigned char *temp_buf = NULL;

    fp = fopen(filename, "rb");
    if (!fp) {
        printf("open %s error!\n", filename);
        return -1;
    }

    temp_buf = (unsigned char *)malloc(1024 * sizeof(unsigned char));
    if (!temp_buf) {
        printf("sstar_parse_video_info: malloc buf error\n");
        goto fail;
    }

    if (3 != fread (temp_buf, 1, 3, fp)) {
        goto fail;
    }
    seek_length = 3;
    while (!find_sps_code || !find_pps_code) {
        temp_buf[seek_length ++] = fgetc(fp);

        if (FindStartCode3(&temp_buf[seek_length - 4]) != 1) {
            if (FindStartCode2(&temp_buf[seek_length - 3]) != 1) {

            } else {
                //start_code_len = 3;
                temp_buf[seek_length] = fgetc(fp);
                if ((temp_buf[seek_length] & 0x1F) == 7) {
                    find_sps_code = 1;
                    sps_pos = seek_length;
                } else if ((temp_buf[seek_length] & 0x1F) == 8) {
                    find_pps_code = 1;
                    pps_pos = seek_length;
                }
                seek_length ++;
            }
        } else {
            //start_code_len = 4;
            temp_buf[seek_length] = fgetc(fp);
            if ((temp_buf[seek_length] & 0x1F) == 7) {
                find_sps_code = 1;
                sps_pos = seek_length;
            } else if ((temp_buf[seek_length] & 0x1F) == 8) {
                find_pps_code = 1;
                pps_pos = seek_length;
            }
            seek_length ++;
        }
        if (seek_length >= 1024) {
            printf("parse_video_info: cant't find start code\n");
            goto fail;
        }
    }
    fclose(fp);
    printf("sps position = [%d], pps length = [%d]\n", sps_pos, (pps_pos - sps_pos - 4));

    SPS sps_buf;
    get_bit_context bitcontext;
    memset(&bitcontext,0x00,sizeof(get_bit_context));
    bitcontext.buf = temp_buf + sps_pos + 1;
    bitcontext.buf_size = (pps_pos - sps_pos) - 4;
    h264dec_seq_parameter_set(&bitcontext, &sps_buf);
    *width  = h264_get_width(&sps_buf);
    *height = h264_get_height(&sps_buf);
    //根据reoder的数量判断视频是否有B帧
    if (sps_buf.vui_parameters.num_reorder_frames > 0) {
        *type = 1;
    } else {
        *type = 0;
    }
    printf("h264 of sps w/h = [%d %d], has bframe: %d\n", *width, *height, *type);
    free(temp_buf);
    return 0;

fail:
    fclose(fp);
    free(temp_buf);
    return -1;
}

// int sstar_vdec_enqueueOneBuffer(int32_t dev, int32_t chn, MI_SYS_DmaBufInfo_t* mi_dma_buf_info)
// {
//     MI_SYS_ChnPort_t chnPort;
//     // set chn port info
//     memset(&chnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
//     chnPort.eModId = E_MI_MODULE_ID_VDEC;
//     chnPort.u32DevId = dev;
//     chnPort.u32ChnId = chn;
//     chnPort.u32PortId = 0;

//     // enqueue one dma buffer
//     if (MI_SUCCESS != MI_SYS_ChnOutputPortEnqueueDmabuf(&chnPort, mi_dma_buf_info)) {
//         printf("call MI_SYS_ChnOutputPortEnqueueDmabuf faild\n");
//         return -1;
//     }
//     return 0;
// }


int sstar_vdec_init(buffer_object_t *buf_obj)
{
    MI_VDEC_ChnAttr_t stChnAttr;
    MI_VDEC_CHN VdecChn     = 0;
    MI_VDEC_InitParam_t stVdecInitParam;
    MI_VDEC_OutputPortAttr_t stOutputPortAttr;
    MI_U32 u32DevIndex = 0;
    vdec_info_t vdec_info = buf_obj->vdec_info;
    //MI_SYS_Init(0);

    memset(&stVdecInitParam, 0x0, sizeof(MI_VDEC_InitParam_t));
    stVdecInitParam.u16MaxWidth = ALIGN_UP(VDEC_MAX_WIDTH, ALIGN_NUM);
    stVdecInitParam.u16MaxHeight = ALIGN_UP(VDEC_MAX_HEIGHT, ALIGN_NUM);

    MI_VDEC_CreateDev(u32DevIndex, &stVdecInitParam);
    MI_VDEC_SetOutputPortLayoutMode(u32DevIndex, E_MI_VDEC_OUTBUF_LAYOUT_AUTO);

    memset(&stChnAttr, 0x0, sizeof(MI_VDEC_ChnAttr_t));
    stChnAttr.eCodecType                     = vdec_info.pixelformat;
    stChnAttr.u32PicWidth                    = ALIGN_BACK(vdec_info.v_out_width, ALIGN_NUM);
    stChnAttr.u32PicHeight                   = ALIGN_BACK(vdec_info.v_out_height, ALIGN_NUM);
    stChnAttr.eVideoMode                     = E_MI_VDEC_VIDEO_MODE_FRAME;
    stChnAttr.u32BufSize                     = 2 * 1920 * 1080;
    // if(stChnAttr.u32PicWidth * stChnAttr.u32PicHeight <= (1920 * 1080)){
    //     stChnAttr.u32BufSize = 4 * 1024 * 1024;
    // }else if(stChnAttr.u32PicWidth * stChnAttr.u32PicHeight <= (4096 * 2176)){
    //     stChnAttr.u32BufSize = 16 * 1024 * 1024;
    // }else {
    //     stChnAttr.u32BufSize = 24 * 1024 * 1024;
    // }
    stChnAttr.eDpbBufMode                    = E_MI_VDEC_DPB_MODE_NORMAL;
    stChnAttr.stVdecVideoAttr.u32RefFrameNum = 16;
    stChnAttr.u32Priority                    = 0;
    stChnAttr.stVdecVideoAttr.stErrHandlePolicy.bUseCusPolicy = FALSE;
    stChnAttr.stVdecVideoAttr.bDisableLowLatency =  FALSE;
    ExecFunc(MI_VDEC_CreateChn(u32DevIndex, VdecChn, &stChnAttr), MI_SUCCESS);
    ExecFunc(MI_VDEC_StartChn(u32DevIndex, VdecChn), MI_SUCCESS);

    memset(&stOutputPortAttr, 0x0, sizeof(MI_VDEC_OutputPortAttr_t));
    stOutputPortAttr.u16Width  = ALIGN_BACK(vdec_info.v_out_width, 16);
    stOutputPortAttr.u16Height = ALIGN_BACK(vdec_info.v_out_height, 16);

    ExecFunc(MI_VDEC_SetOutputPortAttr(u32DevIndex, VdecChn, &stOutputPortAttr), MI_SUCCESS);

    MI_SYS_ChnPort_t chnPort;
    memset(&chnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    chnPort.eModId = E_MI_MODULE_ID_VDEC;
    chnPort.u32DevId = 0;
    chnPort.u32ChnId = 0;
    chnPort.u32PortId = 0;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(0, &chnPort, 0, 3), MI_SUCCESS);

    if(buf_obj->video_file)
    {
        strcpy(file_path,buf_obj->video_file);
    }
#ifndef UVC_HOST_ENABLE
    pthread_create(&_g_tid_video, NULL, sstar_video_thread, (void *)buf_obj);
#endif
    return 0;
}

int sstar_vdec_deinit(buffer_object_t *buf_obj)
{
    MI_VDEC_CHN VdecChn     = 0;
    MI_U32 u32DevIndex = 0;
#ifndef UVC_HOST_ENABLE
    buf_obj->bExit_second = 1;
    if(_g_tid_video)
    {
        pthread_join(_g_tid_video, NULL);
    }
#endif
    ExecFunc(MI_VDEC_StopChn(u32DevIndex, VdecChn), MI_SUCCESS);
    ExecFunc(MI_VDEC_DestroyChn(u32DevIndex, VdecChn), MI_SUCCESS);
    MI_VDEC_DestroyDev(u32DevIndex);
    return 0;
}
