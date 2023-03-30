#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "drm_fourcc.h"
#include "sstar_osd.h"
#include "common.h"
#include "sstar_drm.h"
#include "st_framequeue.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include <libsync.h>
#include <getopt.h>
#include "sstarvdec.h"
#include "sstar_sensor.h"
#include <linux/ioctl.h>
#include <sys/time.h>

extern buffer_object_t _g_buf_obj[];
sem_t vdec_sem_avail;

frame_queue_t _Vdec_EnQueue_t;
frame_queue_t _Vdec_DeQueue_t;
frame_queue_t _Vdec_DrmQueue_t;
char _g_video_file[128]= "/customer/res/720P50.h264";

pthread_t vdec_tid_drm_buf_thread;
pthread_t vdec_tid_enqueue_buf_thread;


//#define VDEC_MAX_NUM_OF_DMABUFF 6
#define VDEC_ALIGN_NUM 32

int bVdecExit = 0;
MI_SYS_ChnPort_t _g_chn_port_info;

static int _g_mi_sys_fd = -1;


static int vdec_get_nv12_stride_and_size(int width, int height, int* pixel_stride,
                                     int* size) {

    int luma_stride;

    /* 4:2:0 formats must have buffers with even height and width as the clump size is 2x2 pixels.
     * Width will be even stride aligned anyway so just adjust height here for size calculation. */
    height = ALIGN_BACK(height, VDEC_ALIGN_NUM);

    luma_stride = ALIGN_BACK(width, VDEC_ALIGN_NUM);

    if (size != NULL) {
        *size = (height * luma_stride * 3)/2 ;
    }

    if (pixel_stride != NULL) {
        *pixel_stride = luma_stride;
    }
    return 0;
}



frame_t* vdec_get_last_queue(frame_queue_t *Queue_t, enum queue_state state)
{
    frame_t *pQueue = NULL;
    dma_info_t *dma_info;
    int count_num = 2000;
    while(count_num)
    {
        pQueue = frame_queue_peek_last(Queue_t, 1);
        dma_info = (dma_info_t *)pQueue->frame;
        if(dma_info != NULL && pQueue->buf_size != 0 && dma_info->buf_in_use == state)
        {
            //printf("state=%d count=%d queue_size=%d \n",state,count_num,Queue_t->size);
            return pQueue;
        }
        else
        {
            usleep(1*1000);
            frame_queue_next(Queue_t, pQueue);
            count_num --;
        }
    }
    printf("get_queue_last fail state=%d \n",state);
    return NULL;
}

void* vdec_enqueue_buffer_loop(void* param)
{
    int dma_buf_handle = -1;
    MI_SYS_DmaBufInfo_t mi_dma_buf_info;
    int timeout = 500;
    int i;
	int ret;
    dma_info_t *dma_info;

    buffer_object_t * buf_obj = (buffer_object_t *)param;
    memset(&mi_dma_buf_info, 0x0, sizeof(MI_SYS_DmaBufInfo_t));

    i = 0;
    while (!bVdecExit)
    {
        sem_wait(&vdec_sem_avail);
        dma_info = &buf_obj->dma_info[i];
        dma_buf_handle = dma_info->dma_buf_fd;

        mi_dma_buf_info.u16Width = buf_obj->vdec_info.v_out_width;
        mi_dma_buf_info.u16Height = buf_obj->vdec_info.v_out_height;
        mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
        mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
        mi_dma_buf_info.u32Stride[0] = buf_obj->vdec_info.v_out_stride;
        mi_dma_buf_info.u32Stride[1] = buf_obj->vdec_info.v_out_stride;
        mi_dma_buf_info.u32DataOffset[0] = 0;
        mi_dma_buf_info.u32DataOffset[1] = buf_obj->vdec_info.v_out_width * buf_obj->vdec_info.v_out_height;

		#if 1
		if((dma_info->out_fence != 0) && (dma_info->out_fence != -1))
        {
            ret = sync_wait_sys(dma_info->out_fence, 50);
            if(ret != 0)
            {
                printf("Waring:maybe drop one drm frame, ret=%d out_fence=%d\n", ret, dma_info->out_fence);
            }
            close(dma_info->out_fence);
            dma_info->out_fence = 0;
        }
        #endif
		
        if(sstar_vdec_enqueueOneBuffer(_g_chn_port_info.u32DevId, _g_chn_port_info.u32ChnId, &mi_dma_buf_info) == 0)
        {
            dma_info->buf_in_use = DEQUEUE_STATE;
            frame_queue_putbuf(&_Vdec_EnQueue_t, (char*)&buf_obj->dma_info[i], sizeof(dma_info_t), NULL);
        }
        else
        {
            printf("sstar_vdec_enqueueOneBuffer FAIL \n");
        }

        if(++i == MAX_NUM_OF_DMABUFF)
        {
            i = 0;
        }
    }

    printf("Thread enqueue_buffer_loop exit \n");
    buf_obj->bExit_second = 1;
    return (void*)0;
}

void* vdec_drm_buffer_loop(void* param)
{
    frame_t *pQueue = NULL;
    int dma_buf_handle = -1;
    dma_info_t *dma_info;
    MI_SYS_DmaBufInfo_t mi_dma_buf_info;
    unsigned long eTime1;
    unsigned long eTime2;
    unsigned long eTime3;
    int timeout = 500;
    int ret;
    buffer_object_t * buf_obj = (buffer_object_t *)param;
    memset(&mi_dma_buf_info, 0x0, sizeof(MI_SYS_DmaBufInfo_t));

    MI_SYS_GetFd(&_g_chn_port_info, &_g_mi_sys_fd);
    while (!buf_obj->bExit_second)
    {			
		pQueue = vdec_get_last_queue(&_Vdec_EnQueue_t, DEQUEUE_STATE);
        if( pQueue !=NULL && pQueue->frame != NULL )
        {
            dma_info = (dma_info_t *)pQueue->frame;
				
        }
        else
        {
            printf("get_last_queue ENQUEUE_STATE fail \n");
            break;
        }
        dma_buf_handle = dma_info->dma_buf_fd;
        mi_dma_buf_info.u16Width = buf_obj->vdec_info.v_out_width;
        mi_dma_buf_info.u16Height = buf_obj->vdec_info.v_out_height;
        mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
        mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
        mi_dma_buf_info.u32Stride[0] = buf_obj->vdec_info.v_out_stride;
        mi_dma_buf_info.u32Stride[1] = buf_obj->vdec_info.v_out_stride;
        mi_dma_buf_info.u32DataOffset[0] = 0;
        mi_dma_buf_info.u32DataOffset[1] = buf_obj->vdec_info.v_out_width * buf_obj->vdec_info.v_out_height;

		#if 1
        ret = sync_wait(_g_mi_sys_fd, timeout);
		if(ret < 0){
			printf("Waring:sync_wait fail ret=0x%x \n", ret);
		}
		#endif
        if (dma_buf_handle > 0)
        {
			do {
				ret = MI_SYS_ChnOutputPortDequeueDmabuf(&_g_chn_port_info, &mi_dma_buf_info);
				
			} while (ret != 0 && !buf_obj->bExit_second);
            
            if (ret != 0)
            {
                bVdecExit = 1;
				buf_obj->bExit_second = 1;
				break;
                printf("Waring:DequeueDmabuf fail ret=0x%x \n", ret);
				//continue;
            }
            
			if((mi_dma_buf_info.u32Status != MI_SYS_DMABUF_STATUS_INVALID) && (mi_dma_buf_info.u32Status != MI_SYS_DMABUF_STATUS_DROP))
            {
                if(0 != sstar_drm_update(buf_obj, dma_buf_handle))
	            {
	                printf("drm update frame buffer failed \n");
					continue;
	            }
            }
            else
            {
                printf("Error frame,u32Status=0x%x  0x%x 0x%x\n",mi_dma_buf_info.u32Status,MI_SYS_DMABUF_STATUS_DONE,MI_SYS_DMABUF_STATUS_DROP);
            }

			#if 0
            if(0 != sstar_drm_update(buf_obj, dma_buf_handle))
            {
                printf("drm update frame buffer failed \n");
				continue;
            }
            if((mi_dma_buf_info.u32Status == MI_SYS_DMABUF_STATUS_INVALID) || (mi_dma_buf_info.u32Status == MI_SYS_DMABUF_STATUS_DROP))
            {
                printf("Error frame,u32Status=0x%x  0x%x 0x%x\n",mi_dma_buf_info.u32Status,MI_SYS_DMABUF_STATUS_DONE,MI_SYS_DMABUF_STATUS_DROP);
            }
            #endif

            dma_info->buf_in_use = IDLEQUEUE_STATE;
            frame_queue_next(&_Vdec_EnQueue_t, pQueue);
            timeout = 100;
            sem_post(&vdec_sem_avail);
        }
        else
        {
            printf("sync_wait fail dma_buf_handle=%d \n",dma_buf_handle);
        }
    }

    MI_SYS_CloseFd(_g_mi_sys_fd);
    printf("Thread drm_buffer_loop exit \n");
    return (void*)0;
}

void display_help(void)
{
    printf("************************* Video usage *************************\n");
    printf("-g : show gop(UI) with ARGB888,default is 0\n");
    printf("-g : show mops(UI) with NV12,default is 0\n");
    printf("--vpath : Video file path\n");
    printf("eg:./Drm_player -g 1 -y 1 --vpath 720P50.h264 \n");

    return;
}

void vdec_creat_dmabuf_queue()
{
	int i;

    frame_queue_init(&_Vdec_EnQueue_t, MAX_NUM_OF_DMABUFF, 0);
    frame_queue_init(&_Vdec_DeQueue_t, MAX_NUM_OF_DMABUFF, 0);
    frame_queue_init(&_Vdec_DrmQueue_t, MAX_NUM_OF_DMABUFF, 0);

    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        sstar_creat_dma_obj(&_g_buf_obj[0].dma_info[i], _g_buf_obj[0].vdec_info.v_out_size);
        if(_g_buf_obj[0].dma_info[i].dma_buf_fd > 0)
        {
            _g_buf_obj[0].dma_info[i].buf_in_use = IDLEQUEUE_STATE;          
        }
    }
	sem_init(&vdec_sem_avail, 0, MAX_NUM_OF_DMABUFF);
}

void vdec_destory_dmabuf_queue()
{
	int i;
    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
		sstar_release_dma_obj(_g_buf_obj[0].fd, &_g_buf_obj[0].dma_info[i]);
    }

    frame_queue_destory(&_Vdec_EnQueue_t);
    frame_queue_destory(&_Vdec_DeQueue_t);
    frame_queue_destory(&_Vdec_DrmQueue_t);
	sem_destroy(&vdec_sem_avail);
}

int vdec_creat_outport_dmabufallocator( MI_SYS_ChnPort_t* mi_chn_port_info)
{

    mi_chn_port_info->eModId = E_MI_MODULE_ID_VDEC;
    mi_chn_port_info->u32DevId = 0;
    mi_chn_port_info->u32ChnId = 0;
    mi_chn_port_info->u32PortId = 0;

    if (MI_SUCCESS != MI_SYS_CreateChnOutputPortDmabufCusAllocator(mi_chn_port_info))
    {
        printf("MI_SYS_CreateChnOutputPortDmabufCusAllocator failed \n");
        return -1;
    }
    return 0;
}

void vdec_destory_outport_dmabufallocator(MI_SYS_ChnPort_t* mi_chn_port_info)
{

    if (MI_SUCCESS != MI_SYS_DestroyChnOutputPortDmabufCusAllocator(mi_chn_port_info))
    {
        printf("warning: MI_SYS_DestroyChnOutputPortDmabufCusAllocator  fail\n");
    }
}

void vdecinit()
{
	bVdecExit = 0;
	_g_buf_obj[0].bExit_second = 0;
	
	sstar_parse_video_info(_g_video_file, &_g_buf_obj[0].vdec_info.v_src_width, &_g_buf_obj[0].vdec_info.v_src_height, &_g_buf_obj[0].vdec_info.v_bframe);
    printf("_g_video_width=%d _g_video_height=%d b_frame=%d _g_video_file=%s\n",_g_buf_obj[0].vdec_info.v_src_width,_g_buf_obj[0].vdec_info.v_src_height, _g_buf_obj[0].vdec_info.v_bframe,_g_video_file);
    if(_g_buf_obj[0].vdec_info.v_src_width <=0 || _g_buf_obj[0].vdec_info.v_src_height <= 0)
    {
        printf("parse video info error \n");
        return 0;
    }
	
	_g_buf_obj[0].vdec_info.v_out_width = ALIGN_UP(_g_buf_obj[0].width, VDEC_ALIGN_NUM);
	_g_buf_obj[0].vdec_info.v_out_height = ALIGN_UP(_g_buf_obj[0].height, VDEC_ALIGN_NUM);

    vdec_get_nv12_stride_and_size(_g_buf_obj[0].vdec_info.v_out_width, _g_buf_obj[0].vdec_info.v_out_height,
                                        &_g_buf_obj[0].vdec_info.v_out_stride, &_g_buf_obj[0].vdec_info.v_out_size);
	
	sem_init(&_g_buf_obj[0].sem_avail, 0, MAX_NUM_OF_DMABUFF);
	memset(_g_buf_obj[0].dma_info, 0, sizeof(dma_info_t) * MAX_NUM_OF_DMABUFF);

    sstar_vdec_init(&_g_buf_obj[0].vdec_info, (void *)_g_video_file);

	_g_buf_obj[0].vdec_info.v_src_width = _g_buf_obj[0].vdec_info.v_out_width;
	_g_buf_obj[0].vdec_info.v_src_height = _g_buf_obj[0].vdec_info.v_out_height;

	vdec_creat_dmabuf_queue();
	vdec_creat_outport_dmabufallocator(&_g_chn_port_info);
	
    pthread_create(&vdec_tid_enqueue_buf_thread, NULL, vdec_enqueue_buffer_loop, (void*)&_g_buf_obj[0]);
    pthread_create(&vdec_tid_drm_buf_thread, NULL, vdec_drm_buffer_loop, (void*)&_g_buf_obj[0]);
    printf("already show Video \n");
}

void vdecdeinit()
{
	bVdecExit = 1;
    if(vdec_tid_enqueue_buf_thread)
    {
        pthread_join(vdec_tid_enqueue_buf_thread, NULL);
    }

    if(vdec_tid_drm_buf_thread)
    {
        pthread_join(vdec_tid_drm_buf_thread, NULL);
    }
	
	sstar_vdec_deinit();
	//vdec_destory_outport_dmabufallocator(&_g_chn_port_info);
	vdec_destory_dmabuf_queue();	
	_g_buf_obj[0].drm_commited = 0;
}


