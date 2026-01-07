#include "sstardrm.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <time.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "drm_fourcc.h"
#include "common.h"
#include <libsync.h>

static int modeset_create_fb(buffer_object_t *bo)
{
    int fd = bo->fd;
	uint32_t pitches[4] = {0}, offsets[4] = {0};
    uint32_t dmabuf_id = bo->dmabuf_id;

    drmPrimeFDToHandle(fd, dmabuf_id, bo->handles);

    if(bo->format == DRM_FORMAT_ARGB8888)
    {
        pitches[0] = bo->width*4;
        offsets[0] = 0;
    }
    else if( bo->format == DRM_FORMAT_NV12) //YUV420
    {
        pitches[0] = bo->width;
        pitches[1] = bo->width;

        bo->handles[1] = bo->handles[0];

        offsets[0] = 0;
        offsets[1] = pitches[0] * bo->height;
    }

    drmModeAddFB2( fd,
                   bo->width, bo->height, bo->format,
                   bo->handles, pitches, offsets,
                   &bo->fb_id, 0);

	// drmModeSetCrtc(fd, bo->crtc_id, bo->fb_id,
	// 		0, 0, &bo->conn_id, 1, &bo->drm_conn->modes[0]);

    // /* XXX: Actually check if this is needed */
    // drmModeDirtyFB(fd, bo->fb_id, NULL, 0);

	return 0;
}

static void modeset_destroy_fb(buffer_object_t *bo)
{
    int i, j = 0;
	struct drm_mode_destroy_dumb destroy = {};

	drmModeRmFB(bo->fd, bo->fb_id);

    if(bo->handles[0])
    {
	    destroy.handle = bo->handles[0];
	    drmIoctl(bo->fd, DRM_IOCTL_GEM_CLOSE, &destroy);
    }

}

uint32_t get_plane_with_format(int dev_fd, drmModePlaneRes *plane_res, uint32_t fmt)
{

    for(int i=0; i< plane_res->count_planes; i++)
    {
        drmModePlane *plane;
        plane = drmModeGetPlane(dev_fd, plane_res->planes[i]);
        for(int j=0; j < plane->count_formats; j++)
        {
            if(plane->formats[j] == fmt )
            {
                printf("plane[%d]=%d crtc_id=%d possible_crtcs=%d format:%d \n",i,plane_res->planes[i],plane->crtc_id,plane->possible_crtcs,fmt);
                return plane_res->planes[i];
            }
        }
    }
    return -1;
}

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

	buf->drm_res = drmModeGetResources(buf->fd);
	buf->crtc_id = buf->drm_res->crtcs[0];
	buf->conn_id = buf->drm_res->connectors[0];


    buf->drm_conn = drmModeGetConnector(buf->fd, buf->conn_id);

	buf->width = buf->drm_conn->modes[0].hdisplay;
	buf->height = buf->drm_conn->modes[0].vdisplay;

    drmSetClientCap(buf->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    buf->drm_plane_res = drmModeGetPlaneResources(buf->fd);
	buf->plane_id = get_plane_with_format(buf->fd, buf->drm_plane_res, buf->format);//plane_res->planes[1];
    printf("plane_id=%d \n",buf->plane_id);

    int i,j;
    printf("***************************conn_count=%d *******************************\n",buf->drm_res->count_connectors);
    for(i=0; i < buf->drm_res->count_connectors; i++)
    {
        drmModeConnector *conn;
        conn = drmModeGetConnector(buf->fd, buf->drm_res->connectors[i]);
        printf("conn[%d]=%d  modes: ",i,buf->drm_res->connectors[i]);
        for(j=0; j < conn->count_modes  ; j++)
        {
            dump_mode(&conn->modes[j]);

        }
        printf("\n");
    }

    printf("***************************count_crtcs=%d *******************************\n",buf->drm_res->count_crtcs);
    for(i=0; i < buf->drm_res->count_crtcs; i++)
    {
        drmModeCrtc *crtc;
        printf("crtc[%d]=%d  modes: ", i, buf->drm_res->crtcs[i]);
        crtc = drmModeGetCrtc(buf->fd, buf->drm_res->crtcs[i]);
		printf("%d\t%d\t(%d,%d)\t(%dx%d)\n",
		       crtc->crtc_id,
		       crtc->buffer_id,
		       crtc->x, crtc->y,
		       crtc->width, crtc->height);
		dump_mode(&crtc->mode);

        printf("\n");
    }

    printf("*******************************plane_id=%d count_planes=%d **************\n",buf->plane_id, buf->drm_plane_res->count_planes);
    for(i=0; i< buf->drm_plane_res->count_planes; i++)
    {
        drmModePlane *plane;
        plane = drmModeGetPlane(buf->fd, buf->drm_plane_res->planes[i]);
        printf("plane[%d]=%d crtc_id=%d possible_crtcs=%d format: ",i,buf->drm_plane_res->planes[i],plane->crtc_id,plane->possible_crtcs);
        for(j=0; j < plane->count_formats; j++)
        {
            dump_fourcc(plane->formats[j]);

        }
        printf("\n");
    }
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
        }
        drmModeFreeProperty(property);
    }

    return id;
}


int init_drm_property_ids(buffer_object_t *buf, int fd, uint32_t crtc_id, uint32_t plane_id, uint32_t conn_id,drm_property_ids_t* prop_ids)
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

	drmModeCreatePropertyBlob(fd, &buf->drm_conn->modes[0],
				sizeof(buf->drm_conn->modes[0]), &buf->blob_id);


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
    drmModeAtomicAddProperty(req, crtc_id, prop_ids->MODE_ID, buf->blob_id);

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
    prop_ids = &bobj->prop_ids;

    req = drmModeAtomicAlloc();
    if (!req) {
        printf("drmModeAtomicAlloc failed \n");
        return -1;
    }
    drmModeAtomicAddProperty(req, bobj->plane_id, prop_ids->FB_ID, fb_id);
    drmModeAtomicAddProperty(req, bobj->plane_id, prop_ids->CRTC_ID, bobj->crtc_id);
    drmModeAtomicAddProperty(req, bobj->plane_id, prop_ids->CRTC_X, 0);
    drmModeAtomicAddProperty(req, bobj->plane_id, prop_ids->CRTC_Y, 0);
    drmModeAtomicAddProperty(req, bobj->plane_id, prop_ids->CRTC_W, bobj->width);
    drmModeAtomicAddProperty(req, bobj->plane_id, prop_ids->CRTC_H, bobj->height);
    drmModeAtomicAddProperty(req, bobj->plane_id, prop_ids->SRC_X,  0);
    drmModeAtomicAddProperty(req, bobj->plane_id, prop_ids->SRC_Y,  0);
    drmModeAtomicAddProperty(req, bobj->plane_id, prop_ids->SRC_W,  bobj->width << 16);//Note this,src_w must be << 16
    drmModeAtomicAddProperty(req, bobj->plane_id, prop_ids->SRC_H,  bobj->height << 16);//Note this,src_height must be << 16
    drmModeAtomicAddProperty(req, bobj->crtc_id, prop_ids->ACTIVE, 1);
    drmModeAtomicAddProperty(req, bobj->crtc_id, prop_ids->MODE_ID, bobj->blob_id);

    drmModeAtomicAddProperty(req, bobj->conn_id, prop_ids->CRTC_ID, bobj->crtc_id);
    printf("bobj->out_fence = %d\n",bobj->out_fence);
    drmModeAtomicAddProperty(req, bobj->crtc_id, prop_ids->FENCE_ID, (uint64_t)&bobj->out_fence);//use this flag,must be close out_fence

    ret = drmModeAtomicCommit(bobj->fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
    printf("bobj->out_fence = %d\n",bobj->out_fence);
    if(ret != 0)
    {
        printf("drmModeAtomicCommit failed ret=%d \n",ret);
        return -1;
    }
    drmModeAtomicFree(req);

    ret = sync_wait(bobj->out_fence, 20);
    if(ret)
    {
        printf("waring:maybe drop one drm frame, ret=%d out_fence=%d\n", ret, bobj->out_fence);
    }
    close(bobj->out_fence);

    return 0;
}

void sstar_mode_set(buffer_object_t *buf)
{
	if(modeset_create_fb(buf) != 0)
    {
        printf("modeset_create_fb fail \n");
    }

    if(drmSetClientCap(buf->fd, DRM_CLIENT_CAP_ATOMIC, 1) != 0)
    {
         printf("drmSetClientCap DRM_CLIENT_CAP_ATOMIC\n");
    }

    init_drm_property_ids(buf, buf->fd, buf->crtc_id, buf->plane_id, buf->conn_id, &buf->prop_ids);

    #if 0
    drmModeSetPlane(  buf->fd,
                      buf->plane_id,
                      buf->crtc_id,
                      buf->fb_id,
                      0,
                      0,
                      0,
                      buf->width,
                      buf->height,
                      0,0,
                      buf->width<<16,buf->height<<16);
    #else
    drm_atomic_commit(buf, buf->fb_id);
    #endif
}

void sstar_drm_deinit(buffer_object_t *buf)
{
	modeset_destroy_fb(buf);
	drmModeFreeConnector(buf->drm_conn);
	drmModeFreeResources(buf->drm_res);
	close(buf->fd);
}
