#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

#include <sys/mman.h>
#include <time.h>
//#include <xf86drm.h>
//#include <xf86drmMode.h>
//#include "drm_fourcc.h"
//#include "sstar_osd.h"
//#include "common.h"
#include "linux/dma-buf.h"
//#include <uapi/linux/dma-heap.h>
#include "st_framequeue.h"
#include "mi_common_datatype.h"
#include <libsync.h>
#include <sys/time.h>
#include "sstar_drm.h"

buffer_object_t buf_yuv;
//extern buffer_object_t _g_buf_obj[];
//#define DUMP_VDEC_OUTBUF
#define DUMP_RES

typedef struct{
    MI_U64 len;
    MI_U32 fd;
    MI_U32 fd_flags;
    MI_U64 heap_flags;
}dma_heap_allocation_data;


#define DMA_HEAP_IOC_MAGIC    'H'

    /**
    * DOC: DMA_HEAP_IOCTL_ALLOC - allocate memory from pool
    *
    * Takes a dma_heap_allocation_data struct and returns it with the fd field
    * populated with the dmabuf handle of the allocation.
    */
#define DMA_HEAP_IOCTL_ALLOC    _IOWR(DMA_HEAP_IOC_MAGIC,0,dma_heap_allocation_data)

static drmModeConnectorPtr _g_conn;
static drmModeRes *_g_res;
static drmModePlaneRes *_g_plane_res;

static uint32_t _g_used_plane[5];

static uint32_t blob_id;

static FILE *_g_out_fd = NULL;

char * mmap_phy2vadrr(int dma_fd, int len)
{
    char *dmaBuffer;
    /* Map the device to memory */
    dmaBuffer = (char *) mmap (0, len, PROT_READ | PROT_WRITE, MAP_SHARED, dma_fd, 0);
    if (dmaBuffer == MAP_FAILED)
    {
        printf ("Error: Failed to map dmabuffer device to memory");
        return 0;
    }
    return dmaBuffer;
}

void sstar_release_dma_obj(int drm_fd, dma_info_t *dma_info)
{

    if(dma_info->out_fence != 0 && dma_info->out_fence != -1)
    {
        close(dma_info->out_fence);
        dma_info->out_fence = 0;
    }

    if(dma_info->dma_buf_fd)
    {
        drmModeRmFB(drm_fd, dma_info->fb_id);
        close(dma_info->dma_buf_fd);
    }
    if(dma_info->gem_handles[0])
    {
        struct drm_gem_close args;
        memset(&args, 0, sizeof(args));
        args.handle = dma_info->gem_handles[0];
        drmIoctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &args);
    }
    if(dma_info->dma_heap_fd)
    {
        close(dma_info->dma_heap_fd);
    }
    if(dma_info->p_viraddr)
    {
        munmap(dma_info->p_viraddr, dma_info->dma_buf_len);
    }
}
void sstar_creat_dma_obj(dma_info_t *dma_info, unsigned int len)
{
	int fd_dmaheap;
    int ret = -1;
    dma_heap_allocation_data heap_data = {
        .len = 0,
        .fd = 0,
        .fd_flags = 2,
        .heap_flags = 0,
    };

    fd_dmaheap = open("/dev/mma", O_RDWR);
    if(fd_dmaheap < 0)
    {
        printf("open dma_heap/mma fail \n");
        dma_info->dma_buf_fd = -1;
        dma_info->dma_heap_fd = -1;
        return;
    }
    heap_data.len = len; // length of data to be allocated in bytes
    heap_data.fd_flags =(O_CLOEXEC | O_ACCMODE);

    ret = ioctl(fd_dmaheap, DMA_HEAP_IOCTL_ALLOC, &heap_data);
    if(ret < 0)
    {
        printf(" DMA_HEAP_IOCTL_ALLOC fail fd=%d ret=%d\n",heap_data.fd,ret);
        heap_data.fd = -1;
    }

#ifdef DUMP_VDEC_OUTBUF
    if(dma_info->p_viraddr == NULL)
    {
        dma_info->p_viraddr = mmap_phy2vadrr(heap_data.fd, len);
    }
#endif
    dma_info->dma_heap_fd = fd_dmaheap;
    dma_info->dma_buf_fd = heap_data.fd;
    dma_info->dma_buf_len = len;

}


#ifdef DUMP_RES
static void dump_fourcc(uint32_t fourcc)
{
	printf(" %c%c%c%c",
		fourcc,
		fourcc >> 8,
		fourcc >> 16,
		fourcc >> 24);
}

static void dump_mode(drmModeModeInfo *mode)
{
	printf("  %s %d %d %d %d %d %d %d %d %d %d",
	       mode->name,
	       mode->vrefresh,
	       mode->hdisplay,
	       mode->hsync_start,
	       mode->hsync_end,
	       mode->htotal,
	       mode->vdisplay,
	       mode->vsync_start,
	       mode->vsync_end,
	       mode->vtotal,
	       mode->clock);

	//printf(" flags: ");
	//mode_flag_str(mode->flags);
	//printf("; type: ");
	//mode_type_str(mode->type);
	//printf("\n");
}

#endif

drmModePlane* sstar_drm_getplane(int fd, int plane_id)
{
    drmModePlane  *plane;
    plane = drmModeGetPlane(fd, plane_id);
    if(plane)
    {
        return plane;
    }
    return NULL;

}


drmModeCrtc* sstar_drm_getcrtc(int fd, int crtc_id)
{
    drmModeCrtc  *crtc;
    crtc = drmModeGetCrtc(fd, crtc_id);
    if(crtc)
    {
        return crtc;
    }
    return NULL;

}
drmModeConnector* sstar_drm_getconn(int fd, int conn_id)
{
    drmModeConnector  *conn;
    conn = drmModeGetConnector(fd, conn_id);
    if(conn)
    {
        return conn;
    }
    return NULL;

}

int get_plane_by_format(drmModePlane* plane, uint32_t format)
{
    for(int i = 0;i < plane->count_formats;i++)
    {
        if(format == plane->formats[i])
        {
            return 0;
        }
    }
    return -1;
}

unsigned long sstar_drmfb_va2pa(void *ptr)
{
	return 0;
}

unsigned int sstar_drmfb_get_xres()
{
    return _g_buf_obj[0].width;
}

unsigned int sstar_drmfb_get_yres()
{
    return _g_buf_obj[0].height;
}

void *sstar_drmfb_get_buffer(int buf_i)
{

	if (buf_i == 1)
	{
		return (void *)_g_buf_obj[0].vaddr_0;
	}
	else if (buf_i == 2)
	{
		return (void *)_g_buf_obj[0].vaddr_1;
	}
	else
	{
		return NULL;
	}
}

int sstar_enqueueOneBuffer(MI_SYS_ChnPort_t *chn_port_info, MI_SYS_DmaBufInfo_t* mi_dma_buf_info)
{
    // MI_SYS_ChnPort_t chnPort;
    // // set chn port info
    // memset(&chnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    // chnPort.eModId = E_MI_MODULE_ID_SCL;
    // chnPort.u32DevId = dev;
    // chnPort.u32ChnId = chn;
    // chnPort.u32PortId = 0;

    // enqueue one dma buffer
    if (MI_SUCCESS != MI_SYS_ChnOutputPortEnqueueDmabuf(chn_port_info, mi_dma_buf_info)) {
        printf("call MI_SYS_ChnOutputPortEnqueueDmabuf faild\n");
        return -1;
    }
    return 0;
}

frame_t* get_last_queue(frame_queue_t *Queue_t, enum queue_state state)
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
    return NULL;
}

void creat_dmabuf_queue(buffer_object_t *buf_obj)
{
    int i;

    frame_queue_init(&buf_obj->_EnQueue_t, MAX_NUM_OF_DMABUFF, 0);

    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        sstar_creat_dma_obj(&buf_obj->dma_info[i], buf_obj->vdec_info.v_src_size);
        if(buf_obj->dma_info[i].dma_buf_fd > 0)
        {
            buf_obj->dma_info[i].buf_in_use = IDLEQUEUE_STATE;
            //printf("int_buf_obj dma_buf_fd=%d \n",buf_obj->dma_info[i].dma_buf_fd);
        }
    }
}

int creat_outport_dmabufallocator(buffer_object_t * buf_obj)
{
    if (MI_SUCCESS != MI_SYS_CreateChnOutputPortDmabufCusAllocator(&buf_obj->chn_port_info))
    {
        printf("MI_SYS_CreateChnOutputPortDmabufCusAllocator failed \n");
        return -1;
    }
    return 0;
}

void* enqueue_buffer_loop(void* param)
{
    int dma_buf_handle = -1;
    MI_SYS_DmaBufInfo_t mi_dma_buf_info;
    int i;
    dma_info_t *dma_info;
    int ret;
    #if 0
    unsigned long eTime1;
    unsigned long eTime2;
    struct timeval timeEnqueue1;
    #endif
    buffer_object_t * buf_obj = (buffer_object_t *)param;
    memset(&mi_dma_buf_info, 0x0, sizeof(MI_SYS_DmaBufInfo_t));

    i = 0;
    while (!buf_obj->bExit_second)
    {

        sem_wait(&buf_obj->sem_avail);
        dma_info = &buf_obj->dma_info[i];
        dma_buf_handle = dma_info->dma_buf_fd;

        mi_dma_buf_info.u16Width = buf_obj->vdec_info.v_src_width;
        mi_dma_buf_info.u16Height = buf_obj->vdec_info.v_src_height;
        mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
        mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
        mi_dma_buf_info.u32Stride[0] = buf_obj->vdec_info.v_src_stride;
        mi_dma_buf_info.u32Stride[1] = buf_obj->vdec_info.v_src_stride;
        mi_dma_buf_info.u32DataOffset[0] = 0;
        mi_dma_buf_info.u32DataOffset[1] = buf_obj->vdec_info.v_src_width * buf_obj->vdec_info.v_src_height;
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        #if 0
        gettimeofday(&timeEnqueue1, NULL);
        eTime1 =timeEnqueue1.tv_sec*1000 + timeEnqueue1.tv_usec/1000;
        printf("dma_buf_handle:%d  sstar_scl_enqueueOneBuffer enqueuetime: %d \n", dma_buf_handle, (eTime1 - eTime2) );
        eTime2 = eTime1;
        #endif
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
        if(sstar_enqueueOneBuffer(&buf_obj->chn_port_info, &mi_dma_buf_info) == 0)
        {
            dma_info->buf_in_use = DEQUEUE_STATE;
            frame_queue_putbuf(&buf_obj->_EnQueue_t, (char*)&buf_obj->dma_info[i], sizeof(dma_info_t), NULL);
            //printf("sstar_drm_enqueueOneBuffer cache_buf_cnt=%d \n",cache_buf_cnt);
        }
        else
        {
            printf("sstar_drm_enqueueOneBuffer FAIL \n");
        }

        if(++i == MAX_NUM_OF_DMABUFF)
        {
            i = 0;
        }
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    }

    printf("Thread enqueue_buffer_loop exit \n");
    return (void*)0;
}

void* drm_buffer_loop(void* param)
{
    frame_t *pQueue = NULL;
    int dma_buf_handle = -1;
    dma_info_t *dma_info;
    MI_SYS_DmaBufInfo_t mi_dma_buf_info;
    int timeout = 5000;
    int dequeue_try_cnt = 0;
    int ret = 0;
    #if 0
    unsigned long eTime1;
    unsigned long eTime2;
    unsigned long eTime3;
    struct timeval timeEnqueue1;
    struct timeval timeEnqueue2;
    struct timeval timeEnqueue3;
    #endif
    buffer_object_t * buf_obj = (buffer_object_t *)param;
    memset(&mi_dma_buf_info, 0x0, sizeof(MI_SYS_DmaBufInfo_t));
    MI_SYS_GetFd(&buf_obj->chn_port_info, &buf_obj->_g_mi_sys_fd);
    //usleep(500*1000);
    while (!buf_obj->bExit)
    {
        pQueue = get_last_queue(&buf_obj->_EnQueue_t, DEQUEUE_STATE);
        if( pQueue !=NULL && pQueue->frame != NULL )
        {
            dma_info = (dma_info_t *)pQueue->frame;
        }
        else
        {
            printf("drm get_last_queue ENQUEUE_STATE fail,sensorIdx=%d \n",buf_obj->sensorIdx);
            continue;
        }
        dma_buf_handle = dma_info->dma_buf_fd;
        mi_dma_buf_info.u16Width = buf_obj->vdec_info.v_src_width;
        mi_dma_buf_info.u16Height = buf_obj->vdec_info.v_src_height;
        mi_dma_buf_info.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        mi_dma_buf_info.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        mi_dma_buf_info.s32Fd[0] = dma_buf_handle;
        mi_dma_buf_info.s32Fd[1] = dma_buf_handle;
        mi_dma_buf_info.u32Stride[0] = buf_obj->vdec_info.v_src_stride;
        mi_dma_buf_info.u32Stride[1] = buf_obj->vdec_info.v_src_stride;
        mi_dma_buf_info.u32DataOffset[0] = 0;
        mi_dma_buf_info.u32DataOffset[1] = buf_obj->vdec_info.v_src_width * buf_obj->vdec_info.v_src_height;

        #if 0
        gettimeofday(&timeEnqueue1, NULL);
        eTime1 =timeEnqueue1.tv_sec*1000 + timeEnqueue1.tv_usec/1000;
        #endif
        sync_wait_sys(buf_obj->_g_mi_sys_fd, timeout);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        if (dma_buf_handle > 0)
        {
            while(0 != MI_SYS_ChnOutputPortDequeueDmabuf(&buf_obj->chn_port_info, &mi_dma_buf_info) && dequeue_try_cnt < 30)
            {
                dequeue_try_cnt++;
                sync_wait_sys(buf_obj->_g_mi_sys_fd, 10);
            }

            #if 0
                gettimeofday(&timeEnqueue2, NULL);
                eTime2 = timeEnqueue2.tv_sec*1000 + timeEnqueue2.tv_usec/1000;
            #endif
            #if 1
            if((mi_dma_buf_info.u32Status != MI_SYS_DMABUF_STATUS_INVALID) && (mi_dma_buf_info.u32Status != MI_SYS_DMABUF_STATUS_DROP))
            {
                if(0 != sstar_drm_update(buf_obj, dma_buf_handle))
                {
                    printf("Waring: drm update frame buffer failed \n");
                    continue;
                }
            }
            // else
            // {
            //      printf("Error frame,u32Status=0x%x  0x%x 0x%x\n",mi_dma_buf_info.u32Status,MI_SYS_DMABUF_STATUS_DONE,MI_SYS_DMABUF_STATUS_DROP);
            // }
            #endif
            //sstar_algo_fps();
            #if 0

            gettimeofday(&timeEnqueue3, NULL);
            eTime3 = timeEnqueue3.tv_sec*1000 + timeEnqueue3.tv_usec/1000;
            if((eTime3 - eTime2) > 16 )
            {
                printf("buf_handle:%d  sstar_drm_update_time: %d,Dequeuet_time=%d \n", dma_buf_handle, (eTime3 - eTime2),(eTime2 - eTime1));
            }
            #endif

            dma_info->buf_in_use = IDLEQUEUE_STATE;
            frame_queue_next(&buf_obj->_EnQueue_t, pQueue);
            timeout = 100;
            sem_post(&buf_obj->sem_avail);
        }
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    }

    //MI_SYS_CloseFd(buf_obj->_g_mi_sys_fd);
    MI_SYS_CloseFd(&buf_obj->_g_mi_sys_fd);
    printf("Thread drm_buffer_loop exit \n");
	buf_obj->bExit_second = 1;
    return (void*)0;
}

void destory_dmabuf_queue(buffer_object_t *buf_obj)
{
    int i;
    for ( i= 0; i < MAX_NUM_OF_DMABUFF; i++)
    {
        sstar_release_dma_obj(buf_obj->fd, &buf_obj->dma_info[i]);
    }
    frame_queue_destory(&buf_obj->_EnQueue_t);
}

void open_dump_file()
{
#ifdef DUMP_VDEC_OUTBUF
    char file_out[128];
    memset(file_out, '\0', sizeof(file_out));
    sprintf(file_out, "test_yuv.yuv");

    if(_g_out_fd == NULL)
    {
        _g_out_fd = fopen(file_out, "wb");
        if (!_g_out_fd)
        {
            printf("fopen %s falied!\n", file_out);
            fclose(_g_out_fd);
            _g_out_fd = NULL;
        }
    }
#endif
}

void wirte_dump_file(char *p_viraddr, int len)
{
#ifdef DUMP_VDEC_OUTBUF

    if(_g_out_fd && p_viraddr && len > 0)
    {
        fwrite(p_viraddr, len, 1, _g_out_fd);
    }
#endif
}

void close_dump_file()
{
#ifdef DUMP_VDEC_OUTBUF
    if(_g_out_fd)
    {
        fclose(_g_out_fd);
        _g_out_fd = NULL;
    }
#endif
}

drmModePlaneRes *plane_res;
#define PLANE_MAX 30

int plane_type[PLANE_TYPE_MAX][PLANE_MAX] = {0};
int plane_type_cnt[PLANE_TYPE_MAX] = {0};
int plane_type2[PLANE_TYPE_MAX][PLANE_MAX] = {0};
int plane_type_cnt2[PLANE_TYPE_MAX] = {0};

static void free_resources(struct resources *res)
{
	int i;

	if (!res)
		return;

#define free_resource(_res, __res, type, Type)					\
	do {									\
		if (!(_res)->type##s)						\
			break;							\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			if (!(_res)->type##s[i].type)				\
				break;						\
			drmModeFree##Type((_res)->type##s[i].type);		\
		}								\
		free((_res)->type##s);						\
	} while (0)

#define free_properties(_res, __res, type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			drmModeFreeObjectProperties(res->type##s[i].props);	\
			free(res->type##s[i].props_info);			\
		}								\
	} while (0)

	if (res->res) {
		free_properties(res, res, crtc);

		free_resource(res, res, crtc, Crtc);
		free_resource(res, res, encoder, Encoder);

		for (i = 0; i < res->res->count_connectors; i++)
			free(res->connectors[i].name);

		free_resource(res, res, connector, Connector);
		free_resource(res, res, fb, FB);

		drmModeFreeResources(res->res);
	}

	if (res->plane_res) {
		free_properties(res, plane_res, plane);

		free_resource(res, plane_res, plane, Plane);

		drmModeFreePlaneResources(res->plane_res);
	}

	free(res);
}

static struct resources *get_resources(buffer_object_t *buf_obj)
{
	struct resources *res;
	int i;

	res = (struct resources *)calloc(1, sizeof(*res));
	if (res == 0)
		return NULL;

	drmSetClientCap(buf_obj->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	drmSetClientCap(buf_obj->fd, DRM_CLIENT_CAP_WRITEBACK_CONNECTORS, 1);

	res->res = drmModeGetResources(buf_obj->fd);
	if (!res->res) {
		fprintf(stderr, "drmModeGetResources failed: %s\n",
			strerror(errno));
		goto error;
	}

	res->crtcs = (struct crtc *)calloc(res->res->count_crtcs, sizeof(*res->crtcs));
	res->encoders = (struct encoder *)calloc(res->res->count_encoders, sizeof(*res->encoders));
	res->connectors = (struct connector *)calloc(res->res->count_connectors, sizeof(*res->connectors));
	res->fbs = (struct fb *)calloc(res->res->count_fbs, sizeof(*res->fbs));

	if (!res->crtcs || !res->encoders || !res->connectors || !res->fbs)
		goto error;

#define get_resource(_res, __res, type, Type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			(_res)->type##s[i].type =				\
				drmModeGet##Type(buf_obj->fd, (_res)->__res->type##s[i]); \
			if (!(_res)->type##s[i].type)				\
				fprintf(stderr, "could not get %s %i: %s\n",	\
					#type, (_res)->__res->type##s[i],	\
					strerror(errno));			\
		}								\
	} while (0)

	get_resource(res, res, crtc, Crtc);
	get_resource(res, res, encoder, Encoder);
	get_resource(res, res, connector, Connector);
	get_resource(res, res, fb, FB);

	/* Set the name of all connectors based on the type name and the per-type ID. */
    #if 0
	for (i = 0; i < res->res->count_connectors; i++) {
		struct connector *connector = &res->connectors[i];
		drmModeConnector *conn = connector->connector;
		int num;

		num = asprintf(&connector->name, "%s-%u",
			 util_lookup_connector_type_name(conn->connector_type),
			 conn->connector_type_id);
		if (num < 0)
			goto error;
	}
    #endif
#define get_properties(_res, __res, type, Type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			struct type *obj = &res->type##s[i];			\
			unsigned int j;						\
			obj->props =						\
				drmModeObjectGetProperties(buf_obj->fd, obj->type->type##_id, \
							   DRM_MODE_OBJECT_##Type); \
			if (!obj->props) {					\
				fprintf(stderr,					\
					"could not get %s %i properties: %s\n", \
					#type, obj->type->type##_id,		\
					strerror(errno));			\
				continue;					\
			}							\
			obj->props_info = (drmModePropertyRes**)calloc(obj->props->count_props,	\
						 sizeof(*obj->props_info));	\
			if (!obj->props_info)					\
				continue;					\
			for (j = 0; j < obj->props->count_props; ++j)		\
				obj->props_info[j] =				\
					drmModeGetProperty(buf_obj->fd, obj->props->props[j]); \
		}								\
	} while (0)

	get_properties(res, res, crtc, CRTC);
	get_properties(res, res, connector, CONNECTOR);

	for (i = 0; i < res->res->count_crtcs; ++i)
		res->crtcs[i].mode = &res->crtcs[i].crtc->mode;

	res->plane_res = drmModeGetPlaneResources(buf_obj->fd);
	if (!res->plane_res) {
		fprintf(stderr, "drmModeGetPlaneResources failed: %s\n",
			strerror(errno));
		return res;
	}

	res->planes = (struct plane*)calloc(res->plane_res->count_planes, sizeof(*res->planes));
	if (!res->planes)
		goto error;

	get_resource(res, plane_res, plane, Plane);
	get_properties(res, plane_res, plane, PLANE);

	return res;

error:
	free_resources(res);
	return NULL;
}

void sort_plane_type(int fd, int crtc_idx) 
{
    drmModePlaneResPtr plane_res;
    drmModePlanePtr plane_prt;
    drmModeObjectPropertiesPtr plane_props_ptr;
    drmModePropertyPtr plane_prop_ptr;
    int i, j;

    plane_res = drmModeGetPlaneResources(fd);
    if (!plane_res) {
        printf("drmModeGetPlaneResources failed: %s\n",
            strerror(errno));
    }
    for (i = 0; i < plane_res->count_planes; i++)
    {
         plane_prt = drmModeGetPlane(fd, plane_res->planes[i]);
         if (plane_prt->possible_crtcs != (1 << crtc_idx)) {
             continue;
         }
         plane_props_ptr = drmModeObjectGetProperties(fd, plane_prt->plane_id, DRM_MODE_OBJECT_PLANE);
         for (j = 0; j < plane_props_ptr->count_props; j++)
         {
             plane_prop_ptr = drmModeGetProperty(fd, plane_props_ptr->props[j]);
             if (!strcmp(plane_prop_ptr->name, "sstar type"))
             {
               if (crtc_idx == 0) {
                   plane_type[plane_props_ptr->prop_values[j]][plane_type_cnt[plane_props_ptr->prop_values[j]]++] = plane_prt->plane_id;
                   //printf("plane_type[%d][%d]=%d\n", plane_props_ptr->prop_values[j], plane_type_cnt[plane_props_ptr->prop_values[j]], plane_prt->plane_id);
               } else if (crtc_idx == 1) {
			       plane_type2[plane_props_ptr->prop_values[j]][plane_type_cnt2[plane_props_ptr->prop_values[j]]++] = plane_prt->plane_id;
			   }

             }
             drmModeFreeProperty(plane_prop_ptr);
         }
         drmModeFreeObjectProperties(plane_props_ptr);
    }

}

uint32_t get_sstar_prop_plane(int dev_fd, SSTAR_PLANE_TYPE_E plane_type)
{
    int i, j;
    drmModePlane *plane_prt;
    drmModeObjectPropertiesPtr plane_props_ptr;
    drmModePropertyPtr plane_prop_ptr;
    for (i = 0; i < _g_plane_res->count_planes; i++)
    {
         plane_prt = sstar_drm_getplane(dev_fd, _g_plane_res->planes[i]);
         //if (plane_prt->possible_crtcs != (1 << crtc_idx)) {
         //    continue;
         //}
         plane_props_ptr = drmModeObjectGetProperties(dev_fd, plane_prt->plane_id, DRM_MODE_OBJECT_PLANE);
         printf("%s count_planes=%d planes[%d]=%d plane_id=%d count_props=%d \n",__FUNCTION__,_g_plane_res->count_planes, i, _g_plane_res->planes[i], plane_prt->plane_id, plane_props_ptr->count_props);
         for (j = 0; j < plane_props_ptr->count_props; j++)
         {
             plane_prop_ptr = drmModeGetProperty(dev_fd, plane_props_ptr->props[j]);
             if (!strcmp(plane_prop_ptr->name, "sstar type") && plane_type == plane_props_ptr->prop_values[j])
             {

                //printf("%s count_enums[%d]=%d name=%s planes_id=%d \n",__FUNCTION__,j, plane_prop_ptr->count_enums, plane_prop_ptr->enums[3].name, _g_plane_res->planes[i]);
                //printf("%s prop_values[%d]=%ld  planes_id=%d \n",__FUNCTION__, j, plane_props_ptr->prop_values[j], _g_plane_res->planes[i]);
                //plane_type[plane_props_ptr->prop_values[j]][plane_type_cnt[plane_props_ptr->prop_values[j]]++] = plane_prt->plane_id;
                return _g_plane_res->planes[i];

             }
             drmModeFreeProperty(plane_prop_ptr);
         }
         drmModeFreeObjectProperties(plane_props_ptr);
    }
    return 0;
}

uint32_t get_plane_with_format(int dev_fd, uint32_t fmt)
{
    for(int i=0; i< _g_plane_res->count_planes; i++)
    {
        drmModePlane *plane;
        plane = sstar_drm_getplane(dev_fd,  _g_plane_res->planes[i]);
        for(int j=0; j < plane->count_formats; j++)
        {
            if(plane->formats[j] == fmt && _g_used_plane[j] !=  _g_plane_res->planes[i])
            {
                //dump_fourcc(plane->formats[j]);
                //printf("get_plane plane[%d]=%d crtc_id=%d possible_crtcs=%d format:%d \n",i, _g_plane_res->planes[i], plane->crtc_id,plane->possible_crtcs,fmt);
                _g_used_plane[j] =  _g_plane_res->planes[i];
                return  _g_plane_res->planes[i];
            }
        }
    }
    return -1;
}

static int modeset_create_fb(buffer_object_t *bo)
{
	struct drm_mode_create_dumb create = {};
 	struct drm_mode_map_dumb map = {};
    int ret;
	uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
    int fd = bo->fd;
	/* create a dumb-buffer, the pixel format is ARGB888 */

    if(bo->format == DRM_FORMAT_ARGB8888)
    {
    	create.width = bo->width;
    	create.height = bo->height;
    	create.bpp = 32; //ARGB8888
    }
    else if(bo->format == DRM_FORMAT_NV12)
    {
        create.width = bo->width;
        create.height = bo->height*3/2;
        create.bpp = 8; //nv12
    } 
	
    ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);//创建gem_object，并分配物理内存,并返回handle，该handle可以找到对应的gem_object
	if (ret) {
		printf("DRM_IOCTL_MODE_CREATE_DUMB fail fd %d ret %d w %d h %d \n", fd, ret, create.width, create.height);
		return -1;
	}
	
	/* bind the dumb-buffer to an FB object */
	bo->pitch = create.pitch;
	bo->size = create.size;
	bo->handle = create.handle;

    if(bo->format == DRM_FORMAT_ARGB8888)
    {
        offsets[0] = 0;
        handles[0] = bo->handle;
        pitches[0] = bo->pitch;
    }
    else if(bo->format == DRM_FORMAT_NV12)
    {
        offsets[0] = 0;
        offsets[1] = bo->pitch * bo->height;
        handles[0] = bo->handle;
        handles[1] = handles[0];
        pitches[0] = bo->pitch;
        pitches[1] = bo->pitch;
    }

    ret = drmModeAddFB2(fd, bo->width, bo->height, bo->format, handles, pitches, offsets, &bo->fb_id_0, 0);
    if (ret)
    {
        printf("11111failed to add fb,fd=%d size=0x%x ret=%d format=%d rgb=%d nv12=%d\n", fd, bo->size, ret, bo->format, DRM_FORMAT_ARGB8888,DRM_FORMAT_NV12);
        return -1;
    }

	printf("add fb,fd=%d size=0x%x ret=%d format=%d rgb=%d nv12=%d\n", bo->fb_id_0, bo->size, ret, bo->format, DRM_FORMAT_ARGB8888,DRM_FORMAT_NV12);
	
	/* map the dumb-buffer to userspace */
	map.handle = create.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);//获取map.offset

	bo->vaddr_0 = mmap(0, create.size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, map.offset);

	#if 0
	drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);//创建gem_object，并分配物理内存,并返回handle，该handle可以找到对应的gem_object

	/* bind the dumb-buffer to an FB object */
	bo->pitch = create.pitch;
	bo->size = create.size;
	bo->handle = create.handle;

    if(bo->format == DRM_FORMAT_ARGB8888)
    {
        offsets[0] = 0;
        handles[0] = bo->handle;
        pitches[0] = bo->pitch;
    }
    else if(bo->format == DRM_FORMAT_NV12)
    {
        offsets[0] = 0;
        offsets[1] = bo->pitch * bo->height;
        handles[0] = bo->handle;
        handles[1] = handles[0];
        pitches[0] = bo->pitch;
        pitches[1] = bo->pitch;
    }

    ret = drmModeAddFB2(fd, bo->width, bo->height, bo->format, handles, pitches, offsets, &bo->fb_id_1, 0);
    if (ret)
    {
        printf("222222failed to add fb,fd=%d size=0x%x ret=%d format=%d rgb=%d nv12=%d\n", bo->fb_id_1, bo->size, ret, bo->format, DRM_FORMAT_ARGB8888,DRM_FORMAT_NV12);
        return -1;
    }

	printf("add fb,fd=%d size=0x%x ret=%d format=%d rgb=%d nv12=%d\n", bo->fb_id_1, bo->size, ret, bo->format, DRM_FORMAT_ARGB8888,DRM_FORMAT_NV12);
	
	/* map the dumb-buffer to userspace */
	map.handle = create.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);//获取map.offset

	bo->vaddr_1 = mmap(0, create.size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, map.offset);
	#endif

	return 0;
}

static void modeset_destroy_fb(buffer_object_t *bo)
{
	struct drm_mode_destroy_dumb destroy = {};

	drmModeRmFB(bo->fd, bo->fb_id_0);

	munmap(bo->vaddr_0, bo->size);

	destroy.handle = bo->handle;
	drmIoctl(bo->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    #if 0
	drmModeRmFB(bo->fd, bo->fb_id_1);

	munmap(bo->vaddr_1, bo->size);

	destroy.handle = bo->handle;
	drmIoctl(bo->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    #endif
}

void sstar_mode_set(buffer_object_t *buf)
{
    int ret;
    ret = drmSetClientCap(buf->fd, DRM_CLIENT_CAP_ATOMIC, 1);
    if(ret != 0)
    {
        printf("drmSetClientCap DRM_CLIENT_CAP_ATOMIC ret=%d  \n",ret);
    }
    init_drm_property_ids(buf);

	if(buf->vdec_info.plane_type == MOPG)
	{
		if(modeset_create_fb(buf) != 0)
	    {
	        printf("modeset_create_fb fail \n");
	        return ;
	    }
	}
    return ;
}

void sstar_drm_deinit(buffer_object_t *buf)
{
    if(buf->vdec_info.plane_type == MOPG)
	{
        modeset_destroy_fb(buf);
    }
    if(_g_conn != NULL)
    {
    	drmModeFreeConnector(_g_conn);
        _g_conn = NULL;
    }
    if(_g_res != NULL)
    {
    	drmModeFreeResources(_g_res);
        _g_res = NULL;
    }
}

int sstar_drm_open()
{
    int dev_fd;
	dev_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if(dev_fd  < 0)
    {
        printf("Open dri/card0 fail \n");
        return -1;
    }
    open_dump_file();
    return dev_fd;
}

int sstar_drm_getattr(buffer_object_t *buf)
{
    int i;
	if(buf == NULL)
    {
        printf("%s_%d_error point \n", __FUNCTION__, __LINE__);
        return -1;
    }
    drmModeConnector *conn;

    buf->resources = get_resources(buf);
    if(!buf->_g_res)
    {
	    buf->_g_res = drmModeGetResources(buf->fd);
    }
	printf("get buf _g_res count_connectors [%d] success!\n",buf->_g_res->count_connectors);
	buf->crtc_id = buf->_g_res->crtcs[0];
	printf("set buf crtc [%d] success!\n",buf->crtc_id);
	for (i = 0; i < buf->_g_res->count_connectors; i++)
	{
	    conn = drmModeGetConnector(buf->fd, buf->_g_res->connectors[i]);
		if (conn->connector_type == buf->connector_type)
		{
            buf->conn_id = buf->_g_res->connectors[i];
		    break;
		}
	}
    if (i > buf->_g_res->count_connectors) {
        printf("can not find panel type, please check ! \n");
        return -1;
    }
	printf("set buf conn_id [%d] success!\n",buf->conn_id);
    if(!buf->_g_conn)
    {
    	buf->_g_conn = drmModeGetConnector(buf->fd, buf->conn_id);
    }
	buf->width = buf->_g_conn->modes[0].hdisplay;
	buf->height = buf->_g_conn->modes[0].vdisplay;
	printf("get buf width [%d] and height [%d] success!\n",buf->width,buf->height);
    if(!buf->_g_plane_res)
    {
        drmSetClientCap(buf->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
        buf->_g_plane_res = drmModeGetPlaneResources(buf->fd);
    }
    //printf("buf->width=%d buf->height=%d buf->crtc_id %d buf->conn_id %d\n",buf->width,buf->height,buf->crtc_id,buf->conn_id);
	_g_res = buf->_g_res;
	_g_conn = buf->_g_conn;
	_g_plane_res = buf->_g_plane_res;
    return 0;
}

void sstar_drm_close(int fd)
{
	close(fd);
    close_dump_file();
}

void sstar_drm_init(buffer_object_t *buf)
{
    int i,j;
    buf->plane_id = get_plane_with_format(buf->fd,  buf->format);
    sort_plane_type(buf->fd, 0);
    #if 0
    for(i = 0;i < PLANE_TYPE_MAX; i++)
    {
        for(j = 0;j < PLANE_MAX; j ++)
        {
            printf("plane_type[%d][%d]=%d\n", i, j, plane_type[i][j]);
        }
    }
    #endif
	buf->vdec_info.plane_id = plane_type[buf->vdec_info.plane_type][0];
    printf("plane_id=%d vdec_plane_id=%d width=%d height=%d\n",buf->plane_id, buf->vdec_info.plane_id,buf->width,buf->height);
#ifdef DUMP_RES
    printf("***************************conn_count=%d *******************************\n",_g_res->count_connectors);
    for(i=0; i < _g_res->count_connectors; i++)
    {
        drmModeConnector *conn;
        conn = sstar_drm_getconn(buf->fd, _g_res->connectors[i]);
        printf("conn[%d]=%d  modes: ",i,_g_res->connectors[i]);
        for(j=0; j < conn->count_modes  ; j++)
        {
            dump_mode(&conn->modes[j]);

        }
        printf("\n");
    }

    printf("***************************count_crtcs=%d *******************************\n",_g_res->count_crtcs);
    for(i=0; i < _g_res->count_crtcs; i++)
    {
        drmModeCrtc *crtc;
        printf("crct[%d]=%d  modes: ", i, _g_res->crtcs[i]);
        crtc = sstar_drm_getcrtc(buf->fd, _g_res->crtcs[i]);
        printf("%d\t%d\t(%d,%d)\t(%dx%d)\n",
               crtc->crtc_id,
               crtc->buffer_id,
               crtc->x, crtc->y,
               crtc->width, crtc->height);
        dump_mode(&crtc->mode);

        printf("\n");
    }

    printf("*******************************plane_id=%d count_planes=%d **************\n",buf->plane_id, _g_plane_res->count_planes);
    for(i=0; i< _g_plane_res->count_planes; i++)
    {
        drmModePlane *plane;
        plane = sstar_drm_getplane(buf->fd, _g_plane_res->planes[i]);
        printf("plane[%d]=%d crtc_id=%d possible_crtcs=%d format: ",i,_g_plane_res->planes[i],plane->crtc_id,plane->possible_crtcs);
        for(j=0; j < plane->count_formats; j++)
        {
            dump_fourcc(plane->formats[j]);
        }
        printf("\n");
    }
#endif

    sstar_mode_set(buf);
}

static int get_property_id(int fd, drmModeObjectProperties* props, const char* name) {
    int id = 0;
    drmModePropertyPtr property;
    int found = 0;

    for (int i = 0; !found && i < props->count_props; ++i) {
        property = drmModeGetProperty(fd, props->props[i]);
        if (!strcmp(property->name, name)) {
            id = property->prop_id;
            found = 1;
            //printf("name=%s values=%llu \n",property->name,property->values[0]);
        }
        drmModeFreeProperty(property);
    }

    return id;
}

int init_drm_property_ids(buffer_object_t *buf)//int fd, uint32_t crtc_id, uint32_t plane_id, uint32_t conn_id,drm_property_ids_t* prop_ids)
{

    drmModeObjectProperties* props;
    drmModeAtomicReqPtr req;
    int ret;
    int fd = buf->fd;
    uint32_t crtc_id = buf->crtc_id;
    uint32_t plane_id = buf->vdec_info.plane_id;
    uint32_t conn_id = buf->conn_id;
    drm_property_ids_t* prop_ids = &buf->vdec_info.prop_ids;

    if(fd == -1 || crtc_id == -1 || plane_id == -1 || conn_id  == -1)
    {
        printf("Please check param \n");
        return -1;
    }

    props = drmModeObjectGetProperties(fd, crtc_id, DRM_MODE_OBJECT_CRTC);
    if (!props) {
        printf("Get properties error,crtc_id=%d \n",crtc_id);
        return -1;
    }

    prop_ids->FENCE_ID = get_property_id(fd, props, "OUT_FENCE_PTR");
    prop_ids->ACTIVE = get_property_id(fd, props, "ACTIVE");
    prop_ids->MODE_ID = get_property_id(fd, props, "MODE_ID");

    drmModeFreeObjectProperties(props);


	drmModeCreatePropertyBlob(fd, &_g_conn->modes[0],
				sizeof(_g_conn->modes[0]), &blob_id);


    props = drmModeObjectGetProperties(fd, plane_id, DRM_MODE_OBJECT_PLANE);
    if (!props) {
        printf("Get properties error,plane_id=%d \n",plane_id);
        return -1;
    }

    prop_ids->FB_ID = get_property_id(fd, props, "FB_ID");
    prop_ids->CRTC_ID = get_property_id(fd, props, "CRTC_ID");
    prop_ids->CRTC_X = get_property_id(fd, props, "CRTC_X");
    prop_ids->CRTC_Y = get_property_id(fd, props, "CRTC_Y");
    prop_ids->CRTC_W = get_property_id(fd, props, "CRTC_W");
    prop_ids->CRTC_H = get_property_id(fd, props, "CRTC_H");
    prop_ids->SRC_X = get_property_id(fd, props, "SRC_X");
    prop_ids->SRC_Y = get_property_id(fd, props, "SRC_Y");
    prop_ids->SRC_W = get_property_id(fd, props, "SRC_W");
    prop_ids->SRC_H = get_property_id(fd, props, "SRC_H");
    prop_ids->realtime_mode = get_property_id(fd, props, "realtime_mode");
    prop_ids->zpos = get_property_id(fd, props, "zpos");

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
    drmModeAtomicAddProperty(req, crtc_id, prop_ids->ACTIVE, 1);
    drmModeAtomicAddProperty(req, crtc_id, prop_ids->MODE_ID, blob_id);
    drmModeAtomicAddProperty(req, conn_id, prop_ids->CRTC_ID, crtc_id);
    ret = drmModeAtomicCommit(fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET | DRM_MODE_ATOMIC_NONBLOCK, NULL);

    if(ret != 0)
    {
        printf("111drmModeAtomicCommit failed ret=%d \n",ret);
        return -1;
    }
    drmModeAtomicFree(req);

    return 0;
}

//static int  test_flag = 0;
static int drm_atomic_commit(buffer_object_t *bobj ,dma_info_t* dma_info) {
    int ret;
    drmModeAtomicReqPtr req;
    struct drm_property_ids* prop_ids;
    uint32_t fb_id;
    fb_id = dma_info->fb_id;
    prop_ids = &bobj->vdec_info.prop_ids;

    if(!bobj->drm_commited)
    {
        req = drmModeAtomicAlloc();
        if (!req) {
            printf("drmModeAtomicAlloc failed \n");
            return -1;
        }
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->FB_ID, fb_id);
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_ID, bobj->crtc_id);
		drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->SRC_X, 0 << 16);
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->SRC_Y, 0 << 16);
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->SRC_W, bobj->vdec_info.v_src_width << 16);//Note this,src_w must be << 16
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->SRC_H, bobj->vdec_info.v_src_height << 16);//Note this,src_height must be << 16
		printf("FB_ID = %d\n", fb_id);
		printf("SRC_X = 0\n");
		printf("SRC_Y = 0\n");
		printf("SRC_W = %d\n", bobj->vdec_info.v_src_width);
		printf("SRC_H = %d\n", bobj->vdec_info.v_src_height);
		drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_X, bobj->vdec_info.v_out_x);
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_Y, bobj->vdec_info.v_out_y);
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_W, bobj->vdec_info.v_out_width);
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_H, bobj->vdec_info.v_out_height);   
        printf("CRTC_X = %d\n", bobj->vdec_info.v_out_x);
		printf("CRTC_Y = %d\n", bobj->vdec_info.v_out_y);
        printf("CRTC_W = %d\n", bobj->vdec_info.v_out_width);
        printf("CRTC_H = %d\n", bobj->vdec_info.v_out_height);

        drmModeAtomicAddProperty(req, bobj->crtc_id, prop_ids->ACTIVE, 1);
        drmModeAtomicAddProperty(req, bobj->crtc_id, prop_ids->MODE_ID, blob_id);
        drmModeAtomicAddProperty(req, bobj->conn_id, prop_ids->CRTC_ID, bobj->crtc_id);
        drmModeAtomicAddProperty(req, bobj->crtc_id, prop_ids->FENCE_ID, &bobj->vdec_info.out_fence);//use this flag,must be close out_fence
		printf("CRTC_ID = %d\n", bobj->crtc_id);
		printf("MODE_ID = %d\n", blob_id);
		printf("FENCE_ID = %d\n", &bobj->vdec_info.out_fence);
		printf("bobj->vdec_info.plane_id = %d\n", bobj->vdec_info.plane_id);
		printf("bobj->vdec_info.plane_type = %d\n", bobj->vdec_info.plane_type);	
        if(bobj->vdec_info.plane_type == MOPS)
        {
            if(bobj->hvp_realtime == 1)
            {
                drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->realtime_mode, 1);
            }
            drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->zpos, 1);
        }
        else if(bobj->vdec_info.plane_type == MOPG)
        {
            drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->zpos, 0);
        }

        ret = drmModeAtomicCommit(bobj->fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
        if(ret == 0)
            bobj->drm_commited = 1;
        else
        {
            printf("Waring: drmModeAtomicCommit failed ret=%d fd=%d fb_id=%d plane_id=%d\n",ret , bobj->fd, fb_id, bobj->vdec_info.plane_id);
            //return -1;
        }
        drmModeAtomicFree(req);

        ret = sync_wait(bobj->vdec_info.out_fence, 50);
        if(ret != 0)
        {
            printf("Waring:maybe drop one drm frame, ret=%d vdec_info.out_fence=%d\n", ret, bobj->vdec_info.out_fence);
        }
        close(bobj->vdec_info.out_fence);

    }
    else
    {
        drm_sstar_update_plane_t new_plane;
        #if 1
        new_plane.planeId = bobj->vdec_info.plane_id;
        new_plane.fbId = fb_id;
        new_plane.fence = (uint64_t)&dma_info->out_fence;
        ret = drmIoctl(bobj->fd, DRM_IOCTL_SSTAR_UPDATE_PLANE, &new_plane);
		//printf("DRM_IOCTL_SSTAR_UPDATE_PLANE, buf_id=%d plane_id=%d fb_id=%d\n", bobj->id, new_plane.planeId, new_plane.fbId);
        if(ret != 0 || dma_info->out_fence == -1)
        {
            printf("DRM_IOCTL_SSTAR_UPDATE_PLANE, ret=%d out_fence=%d\n", ret, dma_info->out_fence);
        }
        #endif
    }
    return 0;
}

dma_info_t* get_dmainfo_by_handle(buffer_object_t *buf, int dma_buf_fd)
{

    for(int i=0;i < MAX_NUM_OF_DMABUFF;i++)
    {
        if(dma_buf_fd == buf->dma_info[i].dma_buf_fd)
        {
            return &buf->dma_info[i];
        }
    }
    return NULL;

}

int sstar_drm_update(buffer_object_t *buf, int dma_buffer_fd) {
    int ret;
    dma_info_t* dma_info;
    dma_info = get_dmainfo_by_handle(buf, dma_buffer_fd);

    if(dma_info != NULL)
    {
        wirte_dump_file(dma_info->p_viraddr, dma_info->dma_buf_len);
        if(!dma_info->gem_handles[0])
        {
            ret = drmPrimeFDToHandle(buf->fd, dma_buffer_fd, &dma_info->gem_handles[0]);
            if (ret) {
                printf("drmPrimeFDToHandle failed,fd=%d ret=%d dma_buffer_fd=%d\n",buf->fd, ret, dma_buffer_fd);
                return -1;
            }
            dma_info->gem_handles[1] = dma_info->gem_handles[0];
            dma_info->pitches[0] = buf->vdec_info.v_out_stride;
            dma_info->pitches[1] = buf->vdec_info.v_out_stride;
            dma_info->offsets[0] = 0;
            dma_info->offsets[1] = buf->vdec_info.v_out_stride * buf->vdec_info.v_out_height;
            dma_info->format = DRM_FORMAT_NV12;
            ret = drmModeAddFB2(buf->fd, buf->vdec_info.v_out_width,
                                buf->vdec_info.v_out_height, dma_info->format, dma_info->gem_handles, dma_info->pitches,
                                dma_info->offsets, &dma_info->fb_id, 0);
            if (ret)
            {
                printf("drmModeAddFB2 failed, ret=%d fb_id=%d\n", ret, dma_info->fb_id);
                return -1;
            }
        }
        ret = drm_atomic_commit(buf, dma_info);
        if (ret) {
            printf("AtomicCommit failed, ret=%d out_fence=%d\n", ret, buf->vdec_info.out_fence);
            return -1;
        }
    }
    return 0;
}



