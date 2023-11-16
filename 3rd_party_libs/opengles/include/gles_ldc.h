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

#ifndef _GLES_LDC_H_
#define _GLES_LDC_H_

#include <stdint.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C"
{
#endif

typedef struct
{
    uint32_t width;
    uint32_t height;

    uint32_t format;

    int32_t fds[3];
    uint32_t stride[3];
    uint32_t dataOffset[3];
} GLES_DMABUF_INFO;

typedef struct
{
    uint32_t inResolutionW;
    uint32_t inResolutionH;
    uint32_t outResolutionW;
    uint32_t outResolutionH;
    uint32_t mapGridX;
    uint32_t mapGridY;
    uint32_t gridSize;
} MAP_INFO;

typedef struct
{
    bool isInit;

    uint32_t fboWidth;
    uint32_t fboHeight;
    uint32_t frameCount;
    uint32_t vertexCount;

    uint32_t program;
    uint32_t texture;
    uint32_t vertexShader;
    uint32_t fragmentShader;
    uint32_t mvpMatrixHandle;

    void *display;
    void *context;

    void *texCoords;
    void *positions;

    MAP_INFO mapInfo;
} GLES_LDC_MODULE;

int gles_ldc_init(GLES_LDC_MODULE *module, void *mapX, void *mapY, MAP_INFO *info, uint32_t frameCnt);
void gles_ldc_deinit(GLES_LDC_MODULE *module);
void gles_ldc_update_map(GLES_LDC_MODULE *module, void *mapX, void *mapY, uint32_t mapGridX, uint32_t mapGridY,
                                uint32_t gridSize);
void gles_ldc_input_texture_buffer(GLES_LDC_MODULE *module, GLES_DMABUF_INFO *textureBufInfo);
GLES_DMABUF_INFO* gles_ldc_get_render_buffer(GLES_LDC_MODULE *module);
void gles_ldc_put_render_buffer(GLES_LDC_MODULE *module, GLES_DMABUF_INFO *renderBufInfo);

#if defined(__cplusplus)
}
#endif

#endif /* _GLES_LDC_H_ */
