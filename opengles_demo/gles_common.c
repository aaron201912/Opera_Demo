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

#include <gles_common.h>
#include <string>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/dma-heap.h>

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include "common.h"
#include "sstardrm.h"
#include <libsync.h>
#include <getopt.h>
#include <sys/time.h>


int main(int argc, char** argv)
{
    GLES_LDC_MODULE* module = NULL;
    GLES_DMABUF_INFO* renderInfo = NULL;
    MAP_INFO info = {1024, 600};
    uint32_t u32Width = info.outResolutionW;
    uint32_t u32Height = info.outResolutionH;
    uint32_t datalen = u32Width * u32Height * 4;
    void *pVaddr = NULL;
    FILE* p = NULL;
    int res = -1;

    buffer_object_t buf_obj;
    memset(&buf_obj, 0x00, sizeof(buf_obj));
	sstar_drm_open(&buf_obj);

    module = (GLES_LDC_MODULE*)malloc(sizeof(GLES_LDC_MODULE));
    memset(module, 0, sizeof(GLES_LDC_MODULE));

    // the the tid of init and get_render_buffer must be consistent
    res = gles_common_init(module, &info, 3); // the consistent tid to init
    if (res)
    {
        printf("gles_common_init failed\n");
        return 0;
    }

    renderInfo = gles_common_get_render_buffer(module); // the consistent tid to wait render
    if (renderInfo == NULL)
    {
        printf("gles_common_get_render_buffer: get render buffer failed\n");
        goto DEINIT;
    }

    buf_obj.format = DRM_FORMAT_ARGB8888;
    buf_obj.dmabuf_id = renderInfo->fds[0];
    sstar_drm_init(&buf_obj);//get crtc、plane、connector

    sstar_mode_set(&buf_obj);

    getchar();
    pVaddr = mmap(NULL, datalen, PROT_WRITE|PROT_READ, MAP_SHARED, renderInfo->fds[0], 0);
    if(!pVaddr)
    {
         printf("failed, mmap render dma-buf return fail\n");
         gles_common_put_render_buffer(module, renderInfo);
         goto DEINIT;
    }

    p = fopen("/mnt/render-1920x1080.rgb", "wb");
    res = fwrite(pVaddr, sizeof(char), datalen, p);
    fclose(p);

    gles_common_put_render_buffer(module, renderInfo);
DEINIT:
    sstar_drm_deinit(&buf_obj);
    gles_common_deinit(module);
    free(module);
    renderInfo = NULL, module = NULL, pVaddr = NULL, p = NULL;
}
