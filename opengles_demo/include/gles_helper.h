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

#ifndef _GLES_HELPER_H_
#define _GLES_HELPER_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>

#if defined(__cplusplus)
extern "C"
{
#endif

typedef enum
{
    GLES_ATTRIBUTE_POSITION = 0,
    GLES_ATTRIBUTE_TEXCOORDS = 1,
}GLES_ATTRIBUTE_E;

typedef struct
{
    uint32_t fboTexName;
    uint32_t fboName;
    GLES_DMABUF_INFO *bufferInfo;
    void *eglImage;
    void *sync;
    bool used;
} EGL_FRAMEBUFFER_INFO;

EGLBoolean egl_init(GLES_LDC_MODULE *module);
GLboolean gles_init(GLES_LDC_MODULE *module, uint32_t width, uint32_t height, uint32_t frameCnt);
GLboolean gles_init_shaders(GLES_LDC_MODULE *module);
GLboolean gles_init_textures(GLES_LDC_MODULE *module);
GLboolean gles_init_render_targets(GLES_LDC_MODULE *module, uint32_t width, uint32_t height, uint32_t frameCnt);

void egl_cleanup(GLES_LDC_MODULE *module);
void gles_cleanup(GLES_LDC_MODULE *module);
void gles_cleanup_shaders(GLES_LDC_MODULE *module);
void gles_cleanup_textures(GLES_LDC_MODULE *module);
void gles_cleanup_render_targets(GLES_LDC_MODULE *module);

int gles_process(GLES_LDC_MODULE *module, int32_t curFrame);
void gles_fill_dmabuf_attribs(GLES_DMABUF_INFO *dma_buf, int32_t *attribs);
GLboolean buffer_to_egl_image(EGLDisplay dpy, GLES_DMABUF_INFO *bufferInfo, EGLImageKHR *eglImage);

void gles_wait_sync(GLES_LDC_MODULE *module);
void generate_vertex_attribute(GLES_LDC_MODULE *module);

#if defined(__cplusplus)
}
#endif

#endif /* _GLES_HELPER_H_ */
