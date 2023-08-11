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

#ifndef __UVC__
#define __UVC__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "uvc_host_common.h"
#include <linux/uvcvideo.h>
#include <linux/videodev2.h>
#include <linux/usb/video.h>

#define MAX_FMT_SUPPORT 500
#define MAX_BUF_CNT 10

#define CONTROL_GET 0
#define CONTROL_SET 1

typedef struct {
    int pixelformat;
    int width, height;
    int frame_size;
    int frame_rate;
} Video_Info_t;

typedef struct {
    int fd;
    char path[20];

    Video_Info_t video_info;

    int buf_cnt;
    void *buf_start[MAX_BUF_CNT];
    unsigned int buf_len[MAX_BUF_CNT];
} Video_Handle_t;

typedef struct {
    void *buf;
    int length;
} Video_Buffer_t;

char *format_fcc_to_str(int pixelformat);

int video_init(Video_Handle_t *video_handle);

int video_deinit(Video_Handle_t *video_handle);

int video_enum_format(Video_Handle_t *video_handle);

int video_set_format(Video_Handle_t *video_handle);

int video_streamon(Video_Handle_t *video_handle, unsigned int buf_cnt);

int video_streamoff(Video_Handle_t *video_handle);

int video_get_buf(Video_Handle_t *video_handle, Video_Buffer_t *video_buf);

int video_put_buf(Video_Handle_t *video_handle, Video_Buffer_t *video_buf);

int video_enum_standard_control(Video_Handle_t *video_handle);

int video_send_standard_control(Video_Handle_t *video_handle, unsigned int id, int *value, int dir);

int video_send_extension_control(Video_Handle_t *video_handle,
                    unsigned int unit,
                    unsigned int selector,
                    unsigned int query,
                    unsigned int size,
                    void *data);

int video_dump_buf(Video_Handle_t *video_handle, Video_Buffer_t *video_buf, char *path, int type);

#define V4L2_PIX_FMT_H265 v4l2_fourcc('H', '2', '6', '5') /* add claude.rao */

/* unit */
#define UVC_VC_EXTENSION1_UNIT_ID 6
#define UVC_VC_EXTENSION2_UNIT_ID 2

/* selector */
#define CUS_XU_SET_ISP (0x1)
#define CUS_XU_GET_ISP (0x2)

/* query */
#define UVC_RC_UNDEFINED 0x00
#define UVC_SET_CUR 0x01
#define UVC_GET_CUR 0x81
#define UVC_GET_MIN 0x82
#define UVC_GET_MAX 0x83
#define UVC_GET_RES 0x84
#define UVC_GET_LEN 0x85
#define UVC_GET_INFO 0x86
#define UVC_GET_DEF 0x87

/* data */
#define REQUEST_IFRAME (0x00000001)
#define CUS_UNKONW_CMD (0xFFFFFFFF)

/* dump mode */
#define DUMP_PICTURE 0
#define DUMP_STREAM 1

#endif
