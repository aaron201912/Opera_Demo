/*
* XXX.c - Sigmastar
*
* Copyright (c) [2019~2020] SigmaStar Technology.
*
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License version 2 for more details.
*
*/

#include "st_uvc_host.h"

int video_init(Video_Handle_t *video_handle)
{
    int fd;
    int err;
    int flags = O_RDWR | O_NONBLOCK;

    struct v4l2_capability cap;

    fd = open(video_handle->path, flags, 0);
    if(fd < 0)
    {
        DEMO_ERR(video_handle, "Cannot open video device: %s.\n", strerror(errno));
        return -errno;
    }

    if(ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
    {
        DEMO_ERR(video_handle, "ioctl(VIDIOC_QUERYCAP): %s.\n", strerror(errno));
        err = -errno;
        goto fail;
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        DEMO_ERR(video_handle, "Not a video capture device.\n");
        err = -ENODEV;
        goto fail;
    }

    if(!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        DEMO_ERR(video_handle, "The device does not support the streaming I/O method.\n");
        err = -ENOSYS;
        goto fail;
    }

    video_handle->fd = fd;

    return 0;

fail:
    close(fd);
    return err;
}


int video_deinit(Video_Handle_t *video_handle)
{
    close(video_handle->fd);
    return 0;
}

char *format_fcc_to_str(int pixelformat)
{
    switch(pixelformat)
    {
        case V4L2_PIX_FMT_YUYV:
            return "YUYV";
        case V4L2_PIX_FMT_NV12:
            return "NV12";
        case V4L2_PIX_FMT_MJPEG:
            return "MJPEG";
        case V4L2_PIX_FMT_H264:
            return "H264";
        case V4L2_PIX_FMT_H265:
            return "H265";
        default:
            return "Not Known!";
    }
}

int video_enum_format(Video_Handle_t *video_handle)
{
    int index = 0, select;
    Video_Info_t info[MAX_FMT_SUPPORT];

    struct v4l2_fmtdesc fmt = {};
    struct v4l2_frmsizeenum frmsize = {};
    struct v4l2_frmivalenum framival = {};

    for(int i = 0; ; i++)
    {
        fmt.index = i;
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if(ioctl(video_handle->fd, VIDIOC_ENUM_FMT, &fmt) < 0)
            break;

#if 0
        /* h265 can not identify by some host, but still can get buffer */
        if(strcmp(format_fcc_to_str(fmt.pixelformat), "Not Known!") == 0)
            continue;
#endif

        for(int j = 0; ; j++)
        {
            frmsize.index = j;
            frmsize.pixel_format = fmt.pixelformat;

            if(ioctl(video_handle->fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) < 0)
                break;

            /* don't enum frameinterval, use default value */
            for(int k = 0; ; k++)
            {
                framival.index = k;
                framival.pixel_format = fmt.pixelformat;
                framival.width = frmsize.discrete.width;
                framival.height = frmsize.discrete.height;

                if(ioctl(video_handle->fd, VIDIOC_ENUM_FRAMEINTERVALS, &framival) < 0)
                    break;

                info[index].pixelformat = frmsize.pixel_format;
                info[index].width = frmsize.discrete.width;
                info[index].height = frmsize.discrete.height;
                info[index].frame_rate = framival.discrete.denominator;
                index++;
            }
        }
    }

    if(index == 0)
    {
        DEMO_ERR(video_handle, "Cannot enum video format.\n");
        goto fail;
    }

    printf("====================Support These Formats====================\n");

    for(int i = 0; i < index; i++)
    {
        printf("[%d]    Format: %s    Resolution: %d x %d    Fps: %d\n",
                i,
                format_fcc_to_str(info[i].pixelformat),
                info[i].width,
                info[i].height,
                info[i].frame_rate);
    }

    do {
        scanf("%d", &select);
        if(select >= index)
            DEMO_ERR(video_handle, "Invalid choice.\n");
    } while(select >= index);

    video_handle->video_info.pixelformat = info[select].pixelformat;
    video_handle->video_info.width = info[select].width;
    video_handle->video_info.height = info[select].height;
    video_handle->video_info.frame_rate = info[select].frame_rate;

    return 0;

fail:
    close(video_handle->fd);
    return -1;
}

int video_set_format(Video_Handle_t *video_handle)
{
    int err;

    struct v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = video_handle->video_info.width;
    fmt.fmt.pix.height = video_handle->video_info.height;
    fmt.fmt.pix.pixelformat = video_handle->video_info.pixelformat;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if(ioctl(video_handle->fd, VIDIOC_S_FMT, &fmt) < 0)
    {
        DEMO_ERR(video_handle, "ioctl(VIDIOC_S_FMT): %s.\n", strerror(errno));
        err = -errno;
        goto fail;
    }

    struct v4l2_streamparm param = {};
    param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    param.parm.capture.timeperframe.numerator = 1;
    param.parm.capture.timeperframe.denominator = video_handle->video_info.frame_rate;

    if(ioctl(video_handle->fd, VIDIOC_S_PARM, &param) < 0)
    {
        DEMO_ERR(video_handle, "ioctl(VIDIOC_S_PARM): %s.\n", strerror(errno));
        err = -errno;
        goto fail;
    }

    video_handle->video_info.width = fmt.fmt.pix.width;
    video_handle->video_info.height = fmt.fmt.pix.height;
    video_handle->video_info.pixelformat = fmt.fmt.pix.pixelformat;
#if 0
    /* can't get update fps currently, don't know why */
    video_handle->video_info.frame_rate = param.parm.capture.timeperframe.denominator;
#endif

    switch(video_handle->video_info.pixelformat)
    {
        case V4L2_PIX_FMT_YUYV:
            video_handle->video_info.frame_size = video_handle->video_info.width * video_handle->video_info.height * 2.0;
            break;
        case V4L2_PIX_FMT_NV12:
            video_handle->video_info.frame_size = video_handle->video_info.width * video_handle->video_info.height * 1.5;
            break;
        case V4L2_PIX_FMT_MJPEG:
            video_handle->video_info.frame_size = video_handle->video_info.width * video_handle->video_info.height * 2.0 / 6;
            break;
        case V4L2_PIX_FMT_H264:
            video_handle->video_info.frame_size = video_handle->video_info.width * video_handle->video_info.height * 2.0 / 7;
            break;
        case V4L2_PIX_FMT_H265:
            video_handle->video_info.frame_size = video_handle->video_info.width * video_handle->video_info.height * 2.0 / 8;
            break;
    }

    printf("Format: %s    Resolution: %d x %d    Fps: %d    Expect Frame Size: %d\n",
        format_fcc_to_str(video_handle->video_info.pixelformat), video_handle->video_info.width, video_handle->video_info.height, video_handle->video_info.frame_rate, video_handle->video_info.frame_size);

    return 0;

fail:
    close(video_handle->fd);
    return err;
}

int video_streamon(Video_Handle_t *video_handle, unsigned int buf_cnt)
{
    int err;

    struct v4l2_requestbuffers req = {};
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.count = buf_cnt;
    req.memory = V4L2_MEMORY_MMAP;

    if(ioctl(video_handle->fd, VIDIOC_REQBUFS, &req) < 0)
    {
        DEMO_ERR(video_handle, "ioctl(VIDIOC_REQBUFS): %s.\n", strerror(errno));
        err = -errno;
        goto fail;
    }

    if(req.count < 2)
    {
        DEMO_ERR(video_handle, "Insufficient buffer memory.\n");
        err = -ENOMEM;
        goto fail;
    }

    video_handle->buf_cnt = req.count;

    for(int i = 0; i < video_handle->buf_cnt; i++)
    {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.index = i;
        buf.memory = V4L2_MEMORY_MMAP;

        if(ioctl(video_handle->fd, VIDIOC_QUERYBUF, &buf) < 0)
        {
            DEMO_ERR(video_handle, "ioctl(VIDIOC_QUERYBUF): %s.\n", strerror(errno));
            err = -errno;
            goto fail;
        }

        video_handle->buf_len[i] = buf.length;
        if(video_handle->video_info.frame_size > 0 && video_handle->buf_len[i] < video_handle->video_info.frame_size)
        {
            DEMO_ERR(video_handle, "buf_len[%d] = %d < expected frame size %d.\n",
                    i, video_handle->buf_len[i], video_handle->video_info.frame_size);
            err = -ENOMEM;
            goto fail;
        }

        video_handle->buf_start[i] = mmap(NULL, buf.length,
                                    PROT_READ | PROT_WRITE, MAP_SHARED,
                                    video_handle->fd, buf.m.offset);
        if(video_handle->buf_start[i] == MAP_FAILED)
        {
            DEMO_ERR(video_handle, "mmap: %s.\n", strerror(errno));
            err = -errno;
            goto fail;
        }

        DEMO_INFO(video_handle, "mmap: index%d start0x%p.\n", i, video_handle->buf_start[i]);

        if(ioctl(video_handle->fd, VIDIOC_QBUF, &buf) < 0)
        {
            DEMO_ERR(video_handle, "ioctl(VIDIOC_QBUF): %s.\n", strerror(errno));
            err = -errno;
            goto fail;
        }
    }

    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(ioctl(video_handle->fd, VIDIOC_STREAMON, &type) < 0)
    {
        DEMO_ERR(video_handle, "ioctl(VIDIOC_STREAMON): %s.\n", strerror(errno));
        err = -errno;
        goto fail;
    }

    return 0;

fail:
    close(video_handle->fd);
    return err;
}

int video_streamoff(Video_Handle_t *video_handle)
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(ioctl(video_handle->fd, VIDIOC_STREAMOFF, &type) < 0)
        DEMO_ERR(video_handle, "ioctl(VIDIOC_STREAMOFF): %s.\n", strerror(errno));

    for(int i = 0; i < video_handle->buf_cnt; i++)
        munmap(video_handle->buf_start[i], video_handle->buf_len[i]);

    return 0;
}

int video_get_buf(Video_Handle_t *video_handle, Video_Buffer_t *video_buf)
{
    int err;

    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    err = ioctl(video_handle->fd, VIDIOC_DQBUF, &buf);
    if(err < 0)
    {
        if(errno != EAGAIN)
            DEMO_ERR(video_handle, "ioctl(VIDIOC_DQBUF): %s.\n", strerror(errno));
        return -errno;
    }

    if(buf.index >= video_handle->buf_cnt)
    {
        DEMO_ERR(video_handle, "Invalid buffer index received.\n");
        err = -EAGAIN;
        goto fail;
    }

    if(buf.flags & V4L2_BUF_FLAG_ERROR)
    {
        DEMO_ERR(video_handle, "Dequeued v4l2 buffer contains corrupted data (%d bytes).\n", buf.bytesused);
        err = -EAGAIN;
        goto fail;
    }

    if(!buf.bytesused || (video_handle->video_info.frame_size > 0 && buf.bytesused > video_handle->video_info.frame_size))
    {
        DEMO_ERR(video_handle, "Dequeued v4l2 buffer contains %d bytes, but %d were expected. Flags: 0x%08X.\n",
                    buf.bytesused, video_handle->video_info.frame_size, buf.flags);
        err = -EAGAIN;
        goto fail;
    }

    /* copy to user buffer */
    video_buf->length = buf.bytesused;
    video_buf->buf = calloc(1, buf.bytesused);
    if(!video_buf->buf)
    {
        DEMO_ERR(video_handle, "Error allocating a packet.\n");
        err = -ENOMEM;
        goto fail;
    }

    memcpy(video_buf->buf, video_handle->buf_start[buf.index], buf.bytesused);

    if(ioctl(video_handle->fd, VIDIOC_QBUF, &buf) < 0)
    {
        DEMO_ERR(video_handle, "ioctl(VIDIOC_QBUF): %s.\n", strerror(errno));
        video_put_buf(video_handle, video_buf);
        return -errno;
    }

    return 0;

fail:
    if(ioctl(video_handle->fd, VIDIOC_QBUF, &buf) < 0)
    {
        DEMO_ERR(video_handle, "ioctl(VIDIOC_QBUF): %s.\n", strerror(errno));
        err = -errno;
    }

    return err;
}

int video_put_buf(Video_Handle_t *video_handle, Video_Buffer_t *video_buf)
{
    video_buf->length = 0;
    if(video_buf->buf)
        free(video_buf->buf);

    return 0;
}

int video_enum_standard_control(Video_Handle_t *video_handle)
{
    struct v4l2_queryctrl ctrl = {};

    printf("====================Support These Controls====================\n");

    for(int i = V4L2_CID_BASE; i < V4L2_CID_LASTP1; i++)
    {
        ctrl.id = i;

        if(ioctl(video_handle->fd, VIDIOC_QUERYCTRL, &ctrl) < 0)
            continue;

        printf("Id: %d    Type: %d    Name: %s    Minimum: %d    Maximum: %d    Step: %d    Default Value: %d    flags: %d\n",
            ctrl.id, ctrl.type, ctrl.name, ctrl.minimum, ctrl.maximum, ctrl.step, ctrl.default_value, ctrl.flags);
    }

    return 0;
}

int video_send_standard_control(Video_Handle_t *video_handle, unsigned int id, int *value, int dir)
{
    struct v4l2_control ctrl = {};
    ctrl.id = id;

    switch(dir)
    {
        case CONTROL_GET:
            if(ioctl(video_handle->fd, VIDIOC_G_CTRL, &ctrl) < 0)
            {
                DEMO_ERR(video_handle, "ioctl(VIDIOC_G_CTRL): %s.\n", strerror(errno));
                return -errno;
            }
            *value = ctrl.value;
            break;

        case CONTROL_SET:
            ctrl.value = *value;
            if(ioctl(video_handle->fd, VIDIOC_S_CTRL, &ctrl) < 0)
            {
                DEMO_ERR(video_handle, "ioctl(VIDIOC_S_CTRL): %s.\n", strerror(errno));
                return -errno;
            }
            break;

        default:
            DEMO_ERR(video_handle, "Unknown direction.\n");
            return -1;
    }

    return 0;
}

int video_send_extension_control(Video_Handle_t *video_handle,
                    unsigned int unit,
                    unsigned int selector,
                    unsigned int query,
                    unsigned int size,
                    void *data)
{
    struct uvc_xu_control_query ctrl = {};
    ctrl.unit = unit;
    ctrl.selector = selector;
    ctrl.query = query;
    ctrl.size = size;
    ctrl.data = data;

    if(ioctl(video_handle->fd, UVCIOC_CTRL_QUERY, &ctrl) < 0)
    {
        DEMO_ERR(video_handle, "ioctl(UVCIOC_CTRL_QUERY): %s.\n", strerror(errno));
        return -errno;
    }

    return 0;
}

int video_dump_buf(Video_Handle_t *video_handle, Video_Buffer_t *video_buf, char *path, int type)
{
    int fd;

    switch(type)
    {
        case DUMP_PICTURE:
            fd = open(path, O_TRUNC | O_RDWR | O_CREAT, 0777);
            if(fd < 0)
            {
                DEMO_WRN(video_handle, "open %s: %s.\n", path, strerror(errno));
                goto fail;
            }

            if(write(fd, video_buf->buf, video_buf->length) != video_buf->length)
                DEMO_ERR(video_handle, "write %s: %s.\n", path, strerror(errno));

            close(fd);
            break;

        case DUMP_STREAM:
            fd = open(path, O_APPEND | O_RDWR | O_CREAT, 0777);
            if(fd < 0)
            {
                DEMO_WRN(video_handle, "open %s: %s.\n", path, strerror(errno));
                goto fail;
            }

            if(write(fd, video_buf->buf, video_buf->length) != video_buf->length)
                DEMO_WRN(video_handle, "write %s: %s.\n", path, strerror(errno));

            close(fd);
            break;

        default:
            DEMO_WRN(video_handle, "Unknown dump type.\n");
            goto fail;
    }

    return 0;

fail:
    return -1;
}

