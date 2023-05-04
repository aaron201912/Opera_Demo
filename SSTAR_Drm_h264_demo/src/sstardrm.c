#include "sstardrm.h"


#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

#include <sys/mman.h>
#include <time.h>


#include <xf86drm.h>
#include <xf86drmMode.h>
#include "drm_fourcc.h"
#include "sstar_osd.h"
#include "common.h"
#include "linux/dma-buf.h"
//#include <uapi/linux/dma-heap.h>
#include "mi_common_datatype.h"
#include <libsync.h>



dma_info_t* _g_dma_info_test;
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


drmModeConnectorPtr _g_conn;
drmModeRes *_g_res;
drmModePlaneRes *_g_plane_res;

uint32_t _g_used_plane[5];

uint32_t blob_id;


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

    ret = drmModeAddFB2(fd, bo->width, bo->height, bo->format, handles, pitches, offsets, &bo->fb_id, 0);
    if (ret)
    {
        printf("failed to add fb,fd=%d size=0x%x ret=%d format=%d rgb=%d nv12=%d\n", fd, bo->size, ret, bo->format, DRM_FORMAT_ARGB8888,DRM_FORMAT_NV12);
        return -1;
    }

	/* map the dumb-buffer to userspace */
	map.handle = create.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);//获取map.offset

	bo->vaddr = mmap(0, create.size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, map.offset);

	/* initialize the dumb-buffer with white-color */
	//memset(bo->vaddr, 0x00, bo->size);
	drmModeSetCrtc(fd, bo->crtc_id, bo->fb_id,
			0, 0, &bo->conn_id, 1, &_g_conn->modes[0]);

    /* XXX: Actually check if this is needed */
    drmModeDirtyFB(fd, bo->fb_id, NULL, 0);

	return 0;
}

static void modeset_destroy_fb(buffer_object_t *bo)
{
	struct drm_mode_destroy_dumb destroy = {};

	drmModeRmFB(bo->fd, bo->fb_id);

	munmap(bo->vaddr, bo->size);

	destroy.handle = bo->handle;
	drmIoctl(bo->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}

void sstar_release_dma_obj(dma_info_t *dma_info)
{
    if(dma_info->dma_buf_fd)
    {
        drmModeRmFB(dma_info->dma_buf_fd, dma_info->fb_id);
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
    else
    {
        printf(" DMA_HEAP_IOCTL_ALLOC success fd=%d\n",heap_data.fd);
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


void open_dump_file()
{
#ifdef DUMP_VDEC_OUTBUF
    char file_out[128];
    memset(file_out, '\0', sizeof(file_out));
    sprintf(file_out, "test_yuv.yuv");

    if(_g_out_fd = NULL)
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

    fclose(_g_out_fd);
    _g_out_fd = NULL;
#endif
}
drmModePlaneRes *plane_res;

uint32_t get_plane_with_format(int dev_fd, drmModePlaneRes *plane_res, uint32_t fmt)
{

    for(int i=0; i< plane_res->count_planes; i++)
    {
        drmModePlane *plane;
        plane = sstar_drm_getplane(dev_fd, plane_res->planes[i]);
        for(int j=0; j < plane->count_formats; j++)
        {
            if(plane->formats[j] == fmt && _g_used_plane[j] != plane_res->planes[i])
            {
                //dump_fourcc(plane->formats[j]);
                printf("plane[%d]=%d crtc_id=%d possible_crtcs=%d format:%d \n",i,plane_res->planes[i],plane->crtc_id,plane->possible_crtcs,fmt);
                _g_used_plane[j] = plane_res->planes[i];
                return plane_res->planes[i];
            }
        }
    }
    return -1;
}

void sstar_drm_open(buffer_object_t *buf)
{
	buf->fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if(buf->fd < 0)
    {
        printf("Open dri/card0 fail \n");
        return ;
    }
}

void sstar_drm_init(buffer_object_t *buf)
{
    open_dump_file();

    if(!_g_res)
    {
	    _g_res = drmModeGetResources(buf->fd);
    }
	buf->crtc_id = _g_res->crtcs[0];
	buf->conn_id = _g_res->connectors[0];

    if(!_g_conn)
    {
    	_g_conn = drmModeGetConnector(buf->fd, buf->conn_id);
    }
	buf->width = _g_conn->modes[0].hdisplay;
	buf->height = _g_conn->modes[0].vdisplay;

    if(!_g_plane_res)
    {
        drmSetClientCap(buf->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
        _g_plane_res = drmModeGetPlaneResources(buf->fd);
    }
	buf->plane_id = get_plane_with_format(buf->fd, _g_plane_res, buf->format);//plane_res->planes[1];
    if(buf->use_vdec)
    {
    	buf->vdec_info.plane_id = get_plane_with_format(buf->fd, _g_plane_res, buf->vdec_info.format);//plane_res->planes[1];
    }
    printf("plane_id=%d vdec_plane_id=%d \n",buf->plane_id, buf->vdec_info.plane_id);

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

#if 0
	if(modeset_create_fb(buf->fd, buf) != 0)
    {
        printf("modeset_create_fb fail \n");
        return ;
    }

    ret = drmSetClientCap(buf->fd, DRM_CLIENT_CAP_ATOMIC, 1);
    printf("drmSetClientCap DRM_CLIENT_CAP_ATOMIC ret=%d  \n",ret);
    init_drm_property_ids(buf->fd, buf->crtc_id, buf->vdec_info.plane_id , buf->conn_id, &buf->prop_ids);
#endif

}

void sstar_mode_set(buffer_object_t *buf)
{
    int ret;
	if(modeset_create_fb(buf) != 0)
    {
        printf("modeset_create_fb fail \n");
        return ;
    }

    ret = drmSetClientCap(buf->fd, DRM_CLIENT_CAP_ATOMIC, 1);
    if(ret != 0)
    {
        printf("drmSetClientCap DRM_CLIENT_CAP_ATOMIC ret=%d  \n",ret);
    }
    init_drm_property_ids(buf->fd, buf->crtc_id, buf->plane_id , buf->conn_id, &buf->prop_ids);
    if(buf->use_vdec)
    {
        init_drm_property_ids(buf->fd, buf->crtc_id, buf->vdec_info.plane_id , buf->conn_id, &buf->vdec_info.prop_ids);
    }
    return ;
}


void sstar_drm_deinit(buffer_object_t *buf)
{
	modeset_destroy_fb(buf);

	drmModeFreeConnector(_g_conn);
	drmModeFreeResources(_g_res);

	close(buf->fd);
    close_dump_file();

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


int init_drm_property_ids(int fd, uint32_t crtc_id, uint32_t plane_id, uint32_t conn_id,drm_property_ids_t* prop_ids)
{

    drmModeObjectProperties* props;
    drmModeAtomicReqPtr req;
    int ret;
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

    drmModeFreeObjectProperties(props);
    printf("DRM property ids. FB_ID=%d CRTC_ID=%d CRTC_X=%d CRTC_Y=%d CRTC_W=%d CRTC_H=%d"
           "SRC_X=%d SRC_Y=%d SRC_W=%d SRC_H=%d, FENCE_ID=%d \n",
           prop_ids->FB_ID, prop_ids->CRTC_ID, prop_ids->CRTC_X, prop_ids->CRTC_Y, prop_ids->CRTC_W,
           prop_ids->CRTC_H, prop_ids->SRC_X, prop_ids->SRC_Y, prop_ids->SRC_W, prop_ids->SRC_H,
           prop_ids->FENCE_ID);

    req = drmModeAtomicAlloc();
    if (!req) {
        printf("drmModeAtomicAlloc failed \n");
        return -1;
    }
    drmModeAtomicAddProperty(req, crtc_id, prop_ids->ACTIVE, 1);
    drmModeAtomicAddProperty(req, crtc_id, prop_ids->MODE_ID, blob_id);
    drmModeAtomicAddProperty(req, conn_id, prop_ids->CRTC_ID, crtc_id);
    ret = drmModeAtomicCommit(fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);

    if(ret != 0)
    {
        printf("drmModeAtomicCommit failed ret=%d \n",ret);
        return -1;
    }
    drmModeAtomicFree(req);

    return 0;
}

static int drm_atomic_commit(buffer_object_t *bobj ,int fb_id) {
    int ret;
    drmModeAtomicReqPtr req;
    struct drm_property_ids* prop_ids;
    prop_ids = &bobj->vdec_info.prop_ids;

#if 1
    req = drmModeAtomicAlloc();
    if (!req) {
        printf("drmModeAtomicAlloc failed \n");
        return -1;
    }
    //printf("fb_id=%d crtc_id=%d width=%d height=%d \n",fb_id,bobj->crtc_id,bobj->width,bobj->height);
    drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->FB_ID, fb_id);
    drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_ID, bobj->crtc_id);
    //drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_X, bobj->vdec_info.v_out_x/4);
    //drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_Y, bobj->vdec_info.v_out_y/4);
    //drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_W, bobj->vdec_info.v_out_width/2);
    //drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_H, bobj->vdec_info.v_out_height/2);
    drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_X, bobj->vdec_info.v_out_x);
    drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_Y, bobj->vdec_info.v_out_y);
    drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_W, bobj->vdec_info.v_out_width);
    drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->CRTC_H, bobj->vdec_info.v_out_height);
    drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->SRC_X, bobj->vdec_info.v_out_x);
    drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->SRC_Y, bobj->vdec_info.v_out_y);
    drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->SRC_W, bobj->vdec_info.v_out_width << 16);//Note this,src_w must be << 16
    drmModeAtomicAddProperty(req, bobj->vdec_info.plane_id, prop_ids->SRC_H, bobj->vdec_info.v_out_height << 16);//Note this,src_height must be << 16
    drmModeAtomicAddProperty(req, bobj->crtc_id, prop_ids->ACTIVE, 1);
    drmModeAtomicAddProperty(req, bobj->crtc_id, prop_ids->MODE_ID, blob_id);
    drmModeAtomicAddProperty(req, bobj->conn_id, prop_ids->CRTC_ID, bobj->crtc_id);

    drmModeAtomicAddProperty(req, bobj->crtc_id, prop_ids->FENCE_ID, &bobj->vdec_info.out_fence);//use this flag,must be close out_fence

    ret = drmModeAtomicCommit(bobj->fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
    if(ret != 0)
    {
        printf("drmModeAtomicCommit failed ret=%d \n",ret);
        //return -1;
    }
    drmModeAtomicFree(req);

    ret = sync_wait(bobj->vdec_info.out_fence, 20);
    if(ret)
    {
        printf("waring:maybe drop one drm frame, ret=%d out_fence=%d\n", ret, bobj->vdec_info.out_fence);
    }
    close(bobj->vdec_info.out_fence);

#else

	/* note src coords (last 4 args) are in Q16 format */
	if (drmModeSetPlane(bobj->fd, bobj->plane_id, bobj->crtc_id, fb_id,
			    0, 300, 100, 512, 300,
			    300, 100, 512<<16, 300<<16)) {
		printf("failed to enable plane: %s\n");
		return -1;
	}
    drmModeDirtyFB(bobj->fd, fb_id, NULL, 0);

#endif
    return 0;
}


dma_info_t* get_dmainfo_by_handle(buffer_object_t *buf, int dma_buf_fd)
{

    for(int i=0;i < 3;i++)
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
    wirte_dump_file(dma_info->p_viraddr, dma_info->dma_buf_len);

    if(dma_info != NULL)
    {
        _g_dma_info_test = dma_info;
        if(!dma_info->gem_handles[0])
        {
            ret = drmPrimeFDToHandle(buf->fd, dma_buffer_fd, &dma_info->gem_handles[0]);
            if (ret) {
                printf("drmPrimeFDToHandle failed, ret=%d dma_buffer_fd=%d\n", ret, dma_buffer_fd);
                return -1;
            }
            dma_info->gem_handles[1] = dma_info->gem_handles[0];
            dma_info->pitches[0] = buf->vdec_info.v_out_stride;
            dma_info->pitches[1] = buf->vdec_info.v_out_stride;
            dma_info->offsets[0] = 0;
            dma_info->offsets[1] = buf->vdec_info.v_out_stride * buf->vdec_info.v_out_height;
            dma_info->format = buf->vdec_info.format;
            printf("%s_%d dma_buf_fd=%d gem_handles=%d pitches=%d\n",__FUNCTION__,__LINE__, dma_info->dma_buf_fd , dma_info->gem_handles[0],dma_info->pitches[0]);
            ret = drmModeAddFB2(buf->fd, buf->vdec_info.v_out_width,
                                buf->vdec_info.v_out_height, dma_info->format, dma_info->gem_handles, dma_info->pitches,
                                dma_info->offsets, &dma_info->fb_id, 0);
            if (ret)
            {
                printf("drmModeAddFB2 failed, ret=%d fb_id=%d\n", ret, dma_info->fb_id);
                return -1;
            }
        }
#if 1
        ret = drm_atomic_commit(buf, dma_info->fb_id);
        if (ret) {
            printf("AtomicCommit failed, ret=%d out_fence=%d\n", ret, buf->vdec_info.out_fence);
            return -1;
        }
#else
        drmModeSetPlane(buf->fd, buf->vdec_info->plane_id, buf->crtc_id, dma_info->fb_id, 0, 0,0,1024,600,0,0,1024,600);

#endif
    }
    else
    {
        ret = drm_atomic_commit(buf, buf->fb_id);
        if (ret != 0) {
            printf("AtomicCommit failed, ret=%d \n", ret);
            return -1;
        }
    }


#if 0
else
{
#if 0
    unsigned char *u, *v;
    dma_info = get_dmainfo_by_handle(buf, dma_buffer_fd);

    //memcpy(_g_dma_info_test->p_viraddr,_g_pviadd,_g_dma_info_test->dma_buf_len);
    memset((void *)_g_dma_info_test->p_viraddr, 0xff, _g_dma_info_test->dma_buf_len);
    wirte_dump_file(_g_dma_info_test->p_viraddr, _g_dma_info_test->dma_buf_len);
    usleep(100*1000);
    if(test_value != 0xff)
    {
        test_value += 0x11;
    }
    else
    {
        test_value += 0x00;
    }
#endif
#if 0
    memset((void *)_g_dma_info_test->p_viraddr, 0xff, _g_dma_info_test->dma_buf_len);
    //memcpy(_g_dma_info_test->p_viraddr, dma_info->p_viraddr, _g_dma_info_test->dma_buf_len);
    ret = drm_atomic_commit(buf, _g_dma_info_test->fb_id);
    printf("drmModeAddFB2, ret=%d out_fence=%d\n", ret, buf->vdec_info.out_fence);
    //ret = sync_wait(buf->vdec_info.out_fence, 20);
    //close(buf->vdec_info.out_fence);
    wirte_dump_file(_g_dma_info_test->p_viraddr, _g_dma_info_test->dma_buf_len);
    usleep(10*1000);
    printf("p_viraddr=%llx p_viraddr=%llx dma_buf_len=%d sync_wait ret=%d\n", _g_dma_info_test->p_viraddr, dma_info->p_viraddr,_g_dma_info_test->dma_buf_len,ret);

    u = _g_dma_info_test->p_viraddr + (buf->vdec_info.v_out_height * buf->vdec_info.v_out_stride);
    v = _g_dma_info_test->p_viraddr + (buf->vdec_info.v_out_height * buf->vdec_info.v_out_stride) + 1;
    fill_tiles_yuv_planar(_g_dma_info_test->p_viraddr, u, v, buf->vdec_info.v_out_width, buf->vdec_info.v_out_height, buf->vdec_info.v_out_stride);
    ret = drm_atomic_commit(buf, _g_dma_info_test->fb_id);
    ret = sync_wait(buf->vdec_info.out_fence, 20);
    //close(buf->vdec_info.out_fence);
    //wirte_dump_file(_g_dma_info_test->p_viraddr, _g_dma_info_test->dma_buf_len);
    usleep(100*1000);
#endif

    //memset((void *)buf_yuv.vaddr, 0xff, _g_dma_info_test->dma_buf_len);

    u = buf_yuv.vaddr + (buf->vdec_info.v_out_height * buf->vdec_info.v_out_stride);
    v = buf_yuv.vaddr + (buf->vdec_info.v_out_height * buf->vdec_info.v_out_stride) + 1;
    fill_tiles_yuv_planar(buf_yuv.vaddr, u, v, buf->vdec_info.v_out_width, buf->vdec_info.v_out_height, buf->vdec_info.v_out_stride);
    ret = drm_atomic_commit(buf, buf_yuv.fb_id);
    //ret = sync_wait(buf->vdec_info.out_fence, 20);
    //close(buf->vdec_info.out_fence);
    //wirte_dump_file(_g_dma_info_test->p_viraddr, _g_dma_info_test->dma_buf_len);
    usleep(100*1000);

}
#endif
    return 0;
}



