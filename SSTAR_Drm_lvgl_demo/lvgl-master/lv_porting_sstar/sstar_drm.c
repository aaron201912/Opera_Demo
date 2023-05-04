#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

#include <sys/mman.h>
#include <time.h>
//#include <xf86drm.h>
//#include <xf86drmMode.h>
#include "drm_fourcc.h"
#include "sstar_osd.h"
#include "common.h"
#include "linux/dma-buf.h"
//#include <uapi/linux/dma-heap.h>
#include "mi_common_datatype.h"
#include <libsync.h>
#include <sys/time.h>
#include "sstar_drm.h"

buffer_object_t buf_yuv;

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

buffer_object_t _g_buf_obj[BUF_NUM];
int _g_dev_fd;
drmModeConnectorPtr _g_conn;
drmModeRes *_g_res;
drmModePlaneRes *_g_plane_res;

uint32_t _g_used_plane[5];

uint32_t blob_id;

int flag = 0;

FILE *_g_out_fd = NULL;

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

uint8_t *lastvaddr = NULL;

void sstar_drmfb_flush(const lv_color_t *color_p)
{	
		
	if (lastvaddr == (uint8_t *)color_p)
	{
		printf("the fb buf is same %p \n", lastvaddr);
		return;
	}

	if (flag == 1)
	{
		memset((uint8_t *)color_p + _g_buf_obj[0].width *  40 * 4, 0,  _g_buf_obj[0].width *  (_g_buf_obj[0].height - 40) * 4);	
	}
	
	if ((uint8_t *)color_p == _g_buf_obj[0].vaddr_0)
	drmModeSetPlane(_g_buf_obj[0].fd, _g_buf_obj[0].plane_id, _g_buf_obj[0].crtc_id, _g_buf_obj[0].fb_id_0, 0, 0, 0, _g_buf_obj[0].width, _g_buf_obj[0].height,
                                                                0,0,_g_buf_obj[0].width<<16,_g_buf_obj[0].height<<16);
	if ((uint8_t *)color_p == _g_buf_obj[0].vaddr_1)
	drmModeSetPlane(_g_buf_obj[0].fd, _g_buf_obj[0].plane_id, _g_buf_obj[0].crtc_id, _g_buf_obj[0].fb_id_1, 0, 0, 0, _g_buf_obj[0].width, _g_buf_obj[0].height,
                                                                0,0,_g_buf_obj[0].width<<16,_g_buf_obj[0].height<<16);

	lastvaddr = (uint8_t *)color_p;
	return;
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

int plane_type[PLANE_TYPE_MAX][PLANE_MAX];
int plane_type_cnt[PLANE_TYPE_MAX];

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

	#if 1 
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


	drmModeRmFB(bo->fd, bo->fb_id_1);

	munmap(bo->vaddr_1, bo->size);

	destroy.handle = bo->handle;
	drmIoctl(bo->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}

void sstar_mode_set(buffer_object_t *buf)
{
    int ret;
	if(buf->vdec_info.plane_type == MOPG)
	{
		if(modeset_create_fb(buf) != 0)
	    {
	        printf("modeset_create_fb fail \n");
	        return ;
	    }
	}
	
    ret = drmSetClientCap(buf->fd, DRM_CLIENT_CAP_ATOMIC, 1);
    if(ret != 0)
    {
        printf("drmSetClientCap DRM_CLIENT_CAP_ATOMIC ret=%d  \n",ret);
    }
    init_drm_property_ids(buf);
    return ;
}

void sstar_drm_deinit(buffer_object_t *buf)
{
	//modeset_destroy_fb(buf);
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

    if(!_g_res)
    {
        _g_res = drmModeGetResources(dev_fd);
    }
    if(!_g_conn)
    {
        _g_conn = drmModeGetConnector(dev_fd, _g_res->connectors[0]);
    }
    if(!_g_plane_res)
    {
        drmSetClientCap(dev_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
        _g_plane_res = drmModeGetPlaneResources(dev_fd);
    }
    return dev_fd;
}

int sstar_drm_getattr(buffer_object_t *buf)
{
    if(buf == NULL)
    {
        printf("%s_%d_error point \n", __FUNCTION__, __LINE__);
        return -1;
    }

    if(_g_res)
    {
        buf->crtc_id = _g_res->crtcs[0];
        buf->conn_id = _g_res->connectors[0];
    }
    if(_g_conn)
    {
        buf->width = _g_conn->modes[0].hdisplay;
        buf->height = _g_conn->modes[0].vdisplay;
    }

    return 0;
}

void sstar_drm_close(int fd)
{
	close(fd);
    close_dump_file();
}

void sstar_drm_init(buffer_object_t *buf)
{

    buf->plane_id = get_plane_with_format(buf->fd,  buf->format);
    buf->vdec_info.plane_id = get_sstar_prop_plane(buf->fd, buf->vdec_info.plane_type);
    printf("plane_id=%d vdec_plane_id=%d width=%d height=%d\n",buf->plane_id, buf->vdec_info.plane_id,buf->width,buf->height);
#ifdef DUMP_RES
    int i,j;
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
    prop_ids->zpos = get_property_id(fd, props, "zpos");

    drmModeFreeObjectProperties(props);
    printf("DRM property ids. FB_ID=%d CRTC_ID=%d CRTC_X=%d CRTC_Y=%d CRTC_W=%d CRTC_H=%d"
           "SRC_X=%d SRC_Y=%d SRC_W=%d SRC_H=%d, FENCE_ID=%d zpos=%d\n",
           prop_ids->FB_ID, prop_ids->CRTC_ID, prop_ids->CRTC_X, prop_ids->CRTC_Y, prop_ids->CRTC_W,
           prop_ids->CRTC_H, prop_ids->SRC_X, prop_ids->SRC_Y, prop_ids->SRC_W, prop_ids->SRC_H,
           prop_ids->FENCE_ID,prop_ids->zpos);

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
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_X, bobj->vdec_info.v_out_x);
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_Y, bobj->vdec_info.v_out_y);
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_W, bobj->vdec_info.v_out_width);
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_H, bobj->vdec_info.v_out_height);
		printf("bobj->vdec_info.v_out_x = %d\n", bobj->vdec_info.v_out_x);
		printf("bobj->vdec_info.v_out_y = %d\n", bobj->vdec_info.v_out_y);
		printf("bobj->vdec_info.v_out_width = %d\n", bobj->vdec_info.v_out_width);
		printf("bobj->vdec_info.v_out_height = %d\n", bobj->vdec_info.v_out_height);
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->SRC_X, 0 << 16);
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->SRC_Y, 0 << 16);
		printf("bobj->vdec_info.v_src_width = %d\n", bobj->vdec_info.v_src_width);
		printf("bobj->vdec_info.v_src_height = %d\n", bobj->vdec_info.v_src_height);
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->SRC_W, bobj->vdec_info.v_src_width << 16);//Note this,src_w must be << 16
        drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->SRC_H, bobj->vdec_info.v_src_height << 16);//Note this,src_height must be << 16
        drmModeAtomicAddProperty(req, bobj->crtc_id, prop_ids->ACTIVE, 1);
        drmModeAtomicAddProperty(req, bobj->crtc_id, prop_ids->MODE_ID, blob_id);
        drmModeAtomicAddProperty(req, bobj->conn_id, prop_ids->CRTC_ID, bobj->crtc_id);
        drmModeAtomicAddProperty(req, bobj->crtc_id, prop_ids->FENCE_ID, &bobj->vdec_info.out_fence);//use this flag,must be close out_fence
        if(bobj->vdec_info.plane_type == MOPS)
        {
            drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->zpos, 1);
        }
        else if(bobj->vdec_info.plane_type == MOPG)
        {
            drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->zpos, 0);
        }
        bobj->drm_commited = 1;

        ret = drmModeAtomicCommit(bobj->fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
        if(ret != 0)
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



