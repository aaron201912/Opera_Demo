#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <mi_gfx.h>
#include <mi_gfx_datatype.h>
#include "../common/verify_gfx_type.h"
#include "../common/verify_gfx.h"
#include "../common/blitutil.h"
#include "convert_from_argb.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>


//test
//Start of frame buffer mem
static char *frameBuffer = NULL;

/**
 *dump fix info of Framebuffer
 */


#define MAX_H ((unsigned int)480)
#define MAX_W ((unsigned int)800)

int __create_src_surface_I8(MI_GFX_Surface_t *srcSurf, MI_GFX_Rect_t *srcRect, char **data)
{
    MI_U32 color = 0;
    MI_U16 fence = 0;
    MI_S32 ret = 0;

    if ((ret = _gfx_alloc_surface(srcSurf, data, "blitsrc")) < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
        return ret;
    }

    memset(*data, 1, srcSurf->u32Width * srcSurf->u32Height / 3);
    memset(*data + srcSurf->u32Width * srcSurf->u32Height / 3, 2, srcSurf->u32Width * srcSurf->u32Height / 3);
    memset(*data + srcSurf->u32Width * srcSurf->u32Height * 2 / 3, 3, srcSurf->u32Width * srcSurf->u32Height / 3);
    return 0;
}

int __create_src_surface_ARGB(MI_GFX_Surface_t *srcSurf, MI_GFX_Rect_t *Rect, char **data)
{
    MI_U32 color = 0;
    MI_U16 fence = 0;
    MI_S32 ret = 0;
    MI_GFX_Rect_t rect = *Rect;
    MI_GFX_Rect_t *srcRect = &rect;

    if (srcSurf->eColorFmt == E_MI_GFX_FMT_I8 || srcSurf->eColorFmt == E_MI_GFX_FMT_I4 || srcSurf->eColorFmt == E_MI_GFX_FMT_I2)
    {
        if (__create_src_surface_I8(srcSurf, srcRect, data))
            return -1;

        MI_GFX_Palette_t stPalette;
        memset(&stPalette, 0, sizeof(stPalette));
        stPalette.u16PalStart = 1;
        stPalette.u16PalEnd = 3;

        stPalette.aunPalette[1].RGB.u8A = 0XFF;
        stPalette.aunPalette[1].RGB.u8R = 0XFF;
        stPalette.aunPalette[1].RGB.u8G = 0;
        stPalette.aunPalette[1].RGB.u8B = 0;

        stPalette.aunPalette[2].RGB.u8A = 0XFF;
        stPalette.aunPalette[2].RGB.u8R = 0;
        stPalette.aunPalette[2].RGB.u8G = 0XFF;
        stPalette.aunPalette[2].RGB.u8B = 0;

        stPalette.aunPalette[3].RGB.u8A = 0XFF;
        stPalette.aunPalette[3].RGB.u8R = 0;
        stPalette.aunPalette[3].RGB.u8G = 0;
        stPalette.aunPalette[3].RGB.u8B = 0;

        MI_GFX_SetPalette(0, E_MI_GFX_FMT_I8, &stPalette);
        return 0;
    }

    if ((ret = _gfx_alloc_surface(srcSurf, data, "blendSrc")) < 0)
    {
        return ret;
    }
    srcRect->s32Xpos = 0;
    srcRect->s32Ypos = 0;
    srcRect->u32Width = srcSurf->u32Width / 6;
    srcRect->u32Height = srcSurf->u32Height / 3;
    color = 0XFFFF0000;
    MI_GFX_QuickFill(0, srcSurf, srcRect, color, &fence);

    srcRect->s32Xpos = srcSurf->u32Width / 6;
    srcRect->s32Ypos = srcSurf->u32Height / 3;

    color = 0X80FF0000;
    MI_GFX_QuickFill(0, srcSurf, srcRect, color, &fence);

    srcRect->s32Xpos = srcSurf->u32Width / 3;
    srcRect->s32Ypos = srcSurf->u32Height - srcSurf->u32Height / 3;

    color = 0X80800000;
    MI_GFX_QuickFill(0, srcSurf, srcRect, color, &fence);

    srcRect->s32Xpos = srcSurf->u32Width / 2;
    srcRect->s32Ypos = 0;

    color = 0X800000FF;
    MI_GFX_QuickFill(0, srcSurf, srcRect, color, &fence);

    srcRect->s32Xpos = srcSurf->u32Width / 2 + srcSurf->u32Width / 6;
    srcRect->s32Ypos = srcSurf->u32Height / 3;

    color = 0X800000FF;
    MI_GFX_QuickFill(0, srcSurf, srcRect, color, &fence);

    srcRect->s32Xpos = srcSurf->u32Width / 2 + srcSurf->u32Width / 3;
    srcRect->s32Ypos = srcSurf->u32Height - srcSurf->u32Height / 3;

    color = 0XFF000080;
    MI_GFX_QuickFill(0, srcSurf, srcRect, color, &fence);
    MI_GFX_WaitAllDone(0, FALSE, fence);
    {
    FILE *fp = NULL;
    fp = fopen("image_480x800.rgb", "r");

    if(fp == NULL) {
        fprintf(stderr, "fp == NULL\n");
    } else {
        const char *p = *data;
        long n = srcSurf->u32Height * srcSurf->u32Stride;


    fread(p,1,n,fp);

        fclose(fp);
    }

}
    return 0;
}
int __refill_dst_surface_ARGB(MI_GFX_Surface_t *srcSurf, MI_GFX_Rect_t *Rect)
{
    MI_U32 color = 0;
    MI_U16 fence = 0;

    Rect->s32Xpos = 0;
    Rect->s32Ypos = 0;
    Rect->u32Width = srcSurf->u32Width / 2;
    Rect->u32Height = srcSurf->u32Height / 2;
    color = 0X80FFFFFF;
    MI_GFX_QuickFill(0, srcSurf, Rect, color, &fence);
    Rect->s32Xpos = srcSurf->u32Width / 2;
    Rect->s32Ypos = 0;
    Rect->u32Width = srcSurf->u32Width / 2;
    Rect->u32Height = srcSurf->u32Height / 2;
    color = 0X80FFFF00;
    MI_GFX_QuickFill(0, srcSurf, Rect, color, &fence);
    Rect->s32Xpos = 0;
    Rect->s32Ypos = srcSurf->u32Height / 2;
    Rect->u32Width = srcSurf->u32Width;
    Rect->u32Height = srcSurf->u32Height / 2;
    color = 0XFFFFFFFF;
    MI_GFX_QuickFill(0, srcSurf, Rect, color, &fence);
    MI_GFX_WaitAllDone(0, FALSE, fence);
    return 0;
}

int __create_dst_surface_ARGB(MI_GFX_Surface_t *srcSurf, MI_GFX_Rect_t *Rect, char **data)
{
    MI_U32 color = 0;
    MI_U16 fence = 0;
    MI_S32 ret = 0;
    MI_GFX_Rect_t rect = *Rect;
    MI_GFX_Rect_t *srcRect = &rect;

    if ((ret = _gfx_alloc_surface(srcSurf, data, "blendDst")) < 0)
    {
        return ret;
    }

    srcRect->s32Xpos = 0;
    srcRect->s32Ypos = 0;
    srcRect->u32Width = srcSurf->u32Width / 2;
    srcRect->u32Height = srcSurf->u32Height / 2;
    color = 0X80FF0000;
    MI_GFX_QuickFill(0, srcSurf, srcRect, color, &fence);

    srcRect->s32Xpos = srcSurf->u32Width / 2;
    srcRect->s32Ypos = 0;
    srcRect->u32Width = srcSurf->u32Width / 2;
    srcRect->u32Height = srcSurf->u32Height / 2;
    color = 0X8000FF00;
    MI_GFX_QuickFill(0, srcSurf, srcRect, color, &fence);

    srcRect->s32Xpos = 0;
    srcRect->s32Ypos = srcSurf->u32Height / 2;
    srcRect->u32Width = srcSurf->u32Width;
    srcRect->u32Height = srcSurf->u32Height / 2;
    color = 0XFF0000FF;
    MI_GFX_QuickFill(0, srcSurf, srcRect, color, &fence);
    MI_GFX_WaitAllDone(0, FALSE, fence);
    {
    FILE *fp = NULL;
    fp = fopen("image_800x480.rgb", "r");

    if(fp == NULL) {
        fprintf(stderr, "fp == NULL\n");
    } else {
        const char *p = *data;
        long n = srcSurf->u32Height * srcSurf->u32Stride;


    fread(p,1,n,fp);

        fclose(fp);
    }

}
    return 0;
}
int __fill_surface_YUV420SP(MI_GFX_Surface_t *srcSurf, MI_GFX_Surface_t *dstSurf, char *srcData, char *dstData)
{

    int width;
    int height;

    const unsigned char *src_argb;
    int src_stride_argb;
    unsigned char *dst_y;
    int dst_stride_y;
    unsigned char *dst_uv;
    int dst_stride_uv;

    unsigned char *src_bgra = srcData;
    unsigned char *dst_argb = dstData;
    int src_stride_bgra = srcSurf->u32Stride;
    int dst_stride_argb = dstSurf->u32Stride;
    const unsigned char *shuffler = kShuffleMaskBGRAToARGB;
    width = dstSurf->u32Width;
    height = dstSurf->u32Height;

    ARGBShuffle(src_bgra, src_stride_bgra, dst_argb, dst_stride_argb, shuffler, width, height);
    MI_SYS_FlushInvCache(dstData, height * dstSurf->u32Stride);
    memcpy(srcData, dstData, height * dstSurf->u32Stride);

    width = dstSurf->u32Width;
    height = dstSurf->u32Height;
    src_argb = srcData;
    src_stride_argb = srcSurf->u32Stride;
    dst_y = dstData;
    dst_stride_y = dstSurf->u32Width;
    dst_uv = dstData + dst_stride_y * height;
    dst_stride_uv = dstSurf->u32Width;

    ARGBToNV12(src_argb, src_stride_argb, dst_y, dst_stride_y, dst_uv, dst_stride_uv, width, height);
    MI_SYS_FlushInvCache(dstData, dstSurf->u32Height * dstSurf->u32Stride);
    memcpy(srcData, dstData, dstSurf->u32Height * dstSurf->u32Stride);
    MI_SYS_FlushInvCache(srcData, width * height * 3 / 2);
    return 0;
}


int fill_surface_YUV420SP(MI_GFX_Surface_t *srcSurf, MI_GFX_Surface_t *dstSurf, char *srcData, char *dstData)
{

    FILE *fp = NULL;
    fp = fopen("image_800x480.yuv", "r");

    if(fp == NULL) {
        fprintf(stderr, "fp == NULL\n");
    } else {
        const char *p = dstData;
        long n = srcSurf->u32Height * srcSurf->u32Width *3/2;


    fread(p,1,n,fp);

        fclose(fp);
    }


    return 0;
}

int __test_rotate_ARGB_0(MI_GFX_Surface_t srcSurf, MI_GFX_Rect_t srcRect,
                         MI_GFX_Surface_t dstSurf, MI_GFX_Rect_t dstRect, char *dstData, MI_BOOL bSinkSurf)
{
    Surface src, dst;
    RECT srcClip;
    clock_gettime(CLOCK_MONOTONIC, &ts1);

    dstSurf.eColorFmt = srcSurf.eColorFmt;
    dstSurf.u32Stride = dstSurf.u32Width * getBpp(dstSurf.eColorFmt);

    src.eGFXcolorFmt = srcSurf.eColorFmt;
    src.BytesPerPixel = getBpp(srcSurf.eColorFmt);
    src.h = srcSurf.u32Height;
    src.w = srcSurf.u32Width;
    src.pitch = src.w * src.BytesPerPixel;
    src.phy_addr = srcSurf.phyAddr;

    dst.eGFXcolorFmt = dstSurf.eColorFmt;
    dst.BytesPerPixel = getBpp(dstSurf.eColorFmt);
    dst.h = src.h;
    dst.w = src.w;
    dst.pitch = dst.w * dst.BytesPerPixel;
    dst.phy_addr = dstSurf.phyAddr;

    srcClip.left = 0;
    srcClip.top = 0;
    srcClip.bottom = src.h;
    srcClip.right = src.w;
    SstarBlitNormal(&src, &dst, &srcClip);
    JDEC_PERF(ts1, ts2, 1);

    if (bSinkSurf)
        _gfx_sink_surface(&dstSurf, dstData, __FUNCTION__);

    return 0;
}

int __test_rotate_ARGB_90(MI_GFX_Surface_t srcSurf, MI_GFX_Rect_t srcRect,
                          MI_GFX_Surface_t dstSurf, MI_GFX_Rect_t dstRect, char *dstData, MI_BOOL bSinkSurf)
{
    Surface src, dst;
    RECT srcClip;
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    dstSurf.eColorFmt = srcSurf.eColorFmt;
    dstSurf.u32Stride = dstSurf.u32Width * getBpp(dstSurf.eColorFmt);

    dstSurf.u32Height = srcSurf.u32Width;
    dstSurf.u32Width = srcSurf.u32Height;
    dstSurf.u32Stride = dstSurf.u32Width * getBpp(dstSurf.eColorFmt);

    src.eGFXcolorFmt = srcSurf.eColorFmt;
    src.BytesPerPixel = getBpp(srcSurf.eColorFmt);
    src.h = srcSurf.u32Height;
    src.w = srcSurf.u32Width;
    src.pitch = src.w * src.BytesPerPixel;
    src.phy_addr = srcSurf.phyAddr;

    dst.eGFXcolorFmt = dstSurf.eColorFmt;
    dst.BytesPerPixel = getBpp(dstSurf.eColorFmt);
    dst.h = dstSurf.u32Height;
    dst.w = dstSurf.u32Width;
    dst.pitch = dst.w * dst.BytesPerPixel;
    dst.phy_addr = dstSurf.phyAddr;

    srcClip.left = 0;
    srcClip.top = 0;
    srcClip.bottom = src.h;
    srcClip.right = src.w;
    SstarBlitCW(&src, &dst, &srcClip);
    JDEC_PERF(ts1, ts2, 1);

    if (bSinkSurf)
        _gfx_sink_surface(&dstSurf, dstData, __FUNCTION__);

    return 0;
}

int __test_rotate_ARGB_90_(MI_GFX_Surface_t srcSurf, MI_GFX_Rect_t srcRect,
                           MI_GFX_Surface_t dstSurf, MI_GFX_Rect_t dstRect, char *dstData, MI_BOOL bSinkSurf)
{
    Surface src, dst;
    RECT srcClip;
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    dstSurf.eColorFmt = srcSurf.eColorFmt;
    dstSurf.u32Stride = dstSurf.u32Width * getBpp(dstSurf.eColorFmt);

    dstSurf.u32Height = srcSurf.u32Width;
    dstSurf.u32Width = srcSurf.u32Height;
    dstSurf.u32Stride = dstSurf.u32Width * getBpp(dstSurf.eColorFmt);

    src.eGFXcolorFmt = srcSurf.eColorFmt;
    src.BytesPerPixel = getBpp(srcSurf.eColorFmt);
    src.h = srcSurf.u32Height;
    src.w = srcSurf.u32Width;
    src.pitch = src.w * src.BytesPerPixel;
    src.phy_addr = srcSurf.phyAddr;

    dst.eGFXcolorFmt = dstSurf.eColorFmt;
    dst.BytesPerPixel = getBpp(dstSurf.eColorFmt);
    dst.h = dstSurf.u32Height;
    dst.w = dstSurf.u32Width;
    dst.pitch = dst.w * dst.BytesPerPixel;
    dst.phy_addr = dstSurf.phyAddr;

    srcClip.left = 0;
    srcClip.top = 0;
    srcClip.bottom = src.h;
    srcClip.right = src.w;
    SstarBlitCW(&src, &dst, &srcClip);
    JDEC_PERF(ts1, ts2, 1);

    if (bSinkSurf)
        _gfx_sink_surface(&dstSurf, dstData, __FUNCTION__);

    return 0;
}

int __test_rotate_ARGB_180(MI_GFX_Surface_t srcSurf, MI_GFX_Rect_t srcRect,
                           MI_GFX_Surface_t dstSurf, MI_GFX_Rect_t dstRect, char *dstData, MI_BOOL bSinkSurf)
{
    Surface src, dst;
    RECT srcClip;
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    dstSurf.eColorFmt = srcSurf.eColorFmt;
    dstSurf.u32Stride = dstSurf.u32Width * getBpp(dstSurf.eColorFmt);

    src.eGFXcolorFmt = srcSurf.eColorFmt;
    src.BytesPerPixel = getBpp(srcSurf.eColorFmt);
    src.h = srcSurf.u32Height;
    src.w = srcSurf.u32Width;
    src.pitch = src.w * src.BytesPerPixel;
    src.phy_addr = srcSurf.phyAddr;

    dst.eGFXcolorFmt = dstSurf.eColorFmt;
    dst.BytesPerPixel = getBpp(dstSurf.eColorFmt);
    dst.h = src.h;
    dst.w = src.w;
    dst.pitch = dst.w * dst.BytesPerPixel;
    dst.phy_addr = dstSurf.phyAddr;

    srcClip.left = 0;
    srcClip.top = 0;
    srcClip.bottom = src.h;
    srcClip.right = src.w;
    SstarBlitHVFlip(&src, &dst, &srcClip);
    JDEC_PERF(ts1, ts2, 1);

    if (bSinkSurf)
        _gfx_sink_surface(&dstSurf, dstData, __FUNCTION__);

    return 0;
}
int __test_rotate_ARGB_270(MI_GFX_Surface_t srcSurf, MI_GFX_Rect_t srcRect,
                           MI_GFX_Surface_t dstSurf, MI_GFX_Rect_t dstRect, char *dstData, MI_BOOL bSinkSurf)
{
    Surface src, dst;
    RECT srcClip;
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    dstSurf.eColorFmt = srcSurf.eColorFmt;
    dstSurf.u32Stride = dstSurf.u32Width * getBpp(dstSurf.eColorFmt);

    dstSurf.u32Height = srcSurf.u32Width;
    dstSurf.u32Width = srcSurf.u32Height;
    dstSurf.u32Stride = dstSurf.u32Width * getBpp(dstSurf.eColorFmt);

    src.eGFXcolorFmt = srcSurf.eColorFmt;
    src.BytesPerPixel = getBpp(srcSurf.eColorFmt);
    src.h = srcSurf.u32Height;
    src.w = srcSurf.u32Width;
    src.pitch = src.w * src.BytesPerPixel;
    src.phy_addr = srcSurf.phyAddr;

    dst.eGFXcolorFmt = dstSurf.eColorFmt;
    dst.BytesPerPixel = getBpp(dstSurf.eColorFmt);
    dst.h = dstSurf.u32Height;
    dst.w = dstSurf.u32Width;
    dst.pitch = dst.w * dst.BytesPerPixel;
    dst.phy_addr = dstSurf.phyAddr;

    srcClip.left = 0;
    srcClip.top = 0;
    srcClip.bottom = src.h;
    srcClip.right = src.w;
    SstarBlitCCW(&src, &dst, &srcClip);
    JDEC_PERF(ts1, ts2, 1);

    if (bSinkSurf)
        _gfx_sink_surface(&dstSurf, dstData, __FUNCTION__);

    return 0;
}
int __test_rotate_YUV420SP_0(MI_GFX_Surface_t srcSurf, MI_GFX_Rect_t srcRect,
                             MI_GFX_Surface_t dstSurf, MI_GFX_Rect_t dstRect, char *dstData, MI_BOOL bSinkSurf)
{
    Surface srcY, dstY;
    Surface srcUV, dstUV;
    RECT srcClip;
    clock_gettime(CLOCK_MONOTONIC, &ts1);

    srcY.eGFXcolorFmt = E_MI_GFX_FMT_I8;
    srcY.BytesPerPixel = 1;
    srcY.h = srcSurf.u32Height;
    srcY.w = srcSurf.u32Width;
    srcY.pitch = srcY.w * srcY.BytesPerPixel;
    srcY.phy_addr = srcSurf.phyAddr;

    dstY.eGFXcolorFmt = E_MI_GFX_FMT_I8;
    dstY.BytesPerPixel = 1;
    dstY.h = srcY.h;
    dstY.w = srcY.w;
    dstY.pitch = dstY.w * dstY.BytesPerPixel;
    dstY.phy_addr = dstSurf.phyAddr;

    srcClip.left = 0;
    srcClip.top = 0;
    srcClip.bottom = srcY.h;
    srcClip.right = srcY.w;
    SstarBlitNormal(&srcY, &dstY, &srcClip);

    srcUV.eGFXcolorFmt = E_MI_GFX_FMT_ARGB4444;
    srcUV.BytesPerPixel = 2;
    srcUV.h = srcSurf.u32Height / 2;
    srcUV.w = srcSurf.u32Width / 2;
    srcUV.pitch = srcUV.w * srcUV.BytesPerPixel;
    srcUV.phy_addr = srcSurf.phyAddr + srcY.h * srcY.pitch;

    dstUV.eGFXcolorFmt = E_MI_GFX_FMT_ARGB4444;
    dstUV.BytesPerPixel = 2;
    dstUV.h = srcUV.h;
    dstUV.w = srcUV.w;
    dstUV.pitch = dstUV.w * dstUV.BytesPerPixel;
    dstUV.phy_addr = dstSurf.phyAddr + dstY.h * dstY.pitch;

    srcClip.left = 0;
    srcClip.top = 0;
    srcClip.bottom = srcUV.h;
    srcClip.right = srcUV.w;
    SstarBlitNormal(&srcUV, &dstUV, &srcClip);
    JDEC_PERF(ts1, ts2, 1);

    if (bSinkSurf)
        sink_yuv_surface(&dstSurf, dstData, __FUNCTION__);

    return 0;
}

int __test_rotate_YUV420SP_90(MI_GFX_Surface_t srcSurf, MI_GFX_Rect_t srcRect,
                              MI_GFX_Surface_t dstSurf, MI_GFX_Rect_t dstRect, char *dstData, MI_BOOL bSinkSurf)
{
    Surface srcY, dstY;
    Surface srcUV, dstUV;
    RECT srcClip;
    clock_gettime(CLOCK_MONOTONIC, &ts1);

    srcY.eGFXcolorFmt = E_MI_GFX_FMT_I8;
    srcY.BytesPerPixel = 1;
    srcY.h = srcSurf.u32Height;
    srcY.w = srcSurf.u32Width;
    srcY.pitch = srcY.w * srcY.BytesPerPixel;
    srcY.phy_addr = srcSurf.phyAddr;

    dstY.eGFXcolorFmt = E_MI_GFX_FMT_I8;
    dstY.BytesPerPixel = 1;
    dstY.h = srcY.w;
    dstY.w = srcY.h;
    dstY.pitch = dstY.w * dstY.BytesPerPixel;
    dstY.phy_addr = dstSurf.phyAddr;

    srcClip.left = 0;
    srcClip.top = 0;
    srcClip.bottom = srcY.h;
    srcClip.right = srcY.w;
    SstarBlitCW(&srcY, &dstY, &srcClip);

    srcUV.eGFXcolorFmt = E_MI_GFX_FMT_ARGB4444;
    srcUV.BytesPerPixel = 2;
    srcUV.h = srcSurf.u32Height / 2;
    srcUV.w = srcSurf.u32Width / 2;
    srcUV.pitch = srcUV.w * srcUV.BytesPerPixel;
    srcUV.phy_addr = srcSurf.phyAddr + srcY.h * srcY.pitch;

    dstUV.eGFXcolorFmt = E_MI_GFX_FMT_ARGB4444;
    dstUV.BytesPerPixel = 2;
    dstUV.h = srcUV.w;
    dstUV.w = srcUV.h;
    dstUV.pitch = dstUV.w * dstUV.BytesPerPixel;
    dstUV.phy_addr = dstSurf.phyAddr + dstY.h * dstY.pitch;

    srcClip.left = 0;
    srcClip.top = 0;
    srcClip.bottom = srcUV.h;
    srcClip.right = srcUV.w;
    SstarBlitCW(&srcUV, &dstUV, &srcClip);
    JDEC_PERF(ts1, ts2, 1);

    if (bSinkSurf)
        sink_yuv_surface(&dstSurf, dstData, __FUNCTION__);

    return 0;
}
int __test_rotate_YUV420SP_180(MI_GFX_Surface_t srcSurf, MI_GFX_Rect_t srcRect,
                               MI_GFX_Surface_t dstSurf, MI_GFX_Rect_t dstRect, char *dstData, MI_BOOL bSinkSurf)
{
    Surface srcY, dstY;
    Surface srcUV, dstUV;
    RECT srcClip;
    clock_gettime(CLOCK_MONOTONIC, &ts1);

    srcY.eGFXcolorFmt = E_MI_GFX_FMT_I8;
    srcY.BytesPerPixel = 1;
    srcY.h = srcSurf.u32Height;
    srcY.w = srcSurf.u32Width;
    srcY.pitch = srcY.w * srcY.BytesPerPixel;
    srcY.phy_addr = srcSurf.phyAddr;

    dstY.eGFXcolorFmt = E_MI_GFX_FMT_I8;
    dstY.BytesPerPixel = 1;
    dstY.h = srcY.h;
    dstY.w = srcY.w;
    dstY.pitch = dstY.w * dstY.BytesPerPixel;
    dstY.phy_addr = dstSurf.phyAddr;

    srcClip.left = 0;
    srcClip.top = 0;
    srcClip.bottom = srcY.h;
    srcClip.right = srcY.w;
    SstarBlitHVFlip(&srcY, &dstY, &srcClip);

    srcUV.eGFXcolorFmt = E_MI_GFX_FMT_ARGB4444;
    srcUV.BytesPerPixel = 2;
    srcUV.h = srcSurf.u32Height / 2;
    srcUV.w = srcSurf.u32Width / 2;
    srcUV.pitch = srcUV.w * srcUV.BytesPerPixel;
    srcUV.phy_addr = srcSurf.phyAddr + srcY.h * srcY.pitch;

    dstUV.eGFXcolorFmt = E_MI_GFX_FMT_ARGB4444;
    dstUV.BytesPerPixel = 2;
    dstUV.h = srcUV.h;
    dstUV.w = srcUV.w;
    dstUV.pitch = dstUV.w * dstUV.BytesPerPixel;
    dstUV.phy_addr = dstSurf.phyAddr + dstY.h * dstY.pitch;

    srcClip.left = 0;
    srcClip.top = 0;
    srcClip.bottom = srcUV.h;
    srcClip.right = srcUV.w;
    SstarBlitHVFlip(&srcUV, &dstUV, &srcClip);
    JDEC_PERF(ts1, ts2, 1);

    if (bSinkSurf)
        sink_yuv_surface(&dstSurf, dstData, __FUNCTION__);

    return 0;
}

int __test_rotate_YUV420SP_270(MI_GFX_Surface_t srcSurf, MI_GFX_Rect_t srcRect,
                               MI_GFX_Surface_t dstSurf, MI_GFX_Rect_t dstRect, char *dstData, MI_BOOL bSinkSurf)
{
    Surface srcY, dstY;
    Surface srcUV, dstUV;
    RECT srcClip;
    clock_gettime(CLOCK_MONOTONIC, &ts1);

    srcY.eGFXcolorFmt = E_MI_GFX_FMT_I8;
    srcY.BytesPerPixel = 1;
    srcY.h = srcSurf.u32Height;
    srcY.w = srcSurf.u32Width;
    srcY.pitch = srcY.w * srcY.BytesPerPixel;
    srcY.phy_addr = srcSurf.phyAddr;

    dstY.eGFXcolorFmt = E_MI_GFX_FMT_I8;
    dstY.BytesPerPixel = 1;
    dstY.h = srcY.w;
    dstY.w = srcY.h;
    dstY.pitch = dstY.w * dstY.BytesPerPixel;
    dstY.phy_addr = dstSurf.phyAddr;

    srcClip.left = 0;
    srcClip.top = 0;
    srcClip.bottom = srcY.h;
    srcClip.right = srcY.w;
    SstarBlitCCW(&srcY, &dstY, &srcClip);

    srcUV.eGFXcolorFmt = E_MI_GFX_FMT_ARGB4444;
    srcUV.BytesPerPixel = 2;
    srcUV.h = srcSurf.u32Height / 2;
    srcUV.w = srcSurf.u32Width / 2;
    srcUV.pitch = srcUV.w * srcUV.BytesPerPixel;
    srcUV.phy_addr = srcSurf.phyAddr + srcY.h * srcY.pitch;

    dstUV.eGFXcolorFmt = E_MI_GFX_FMT_ARGB4444;
    dstUV.BytesPerPixel = 2;
    dstUV.h = srcUV.w;
    dstUV.w = srcUV.h;
    dstUV.pitch = dstUV.w * dstUV.BytesPerPixel;
    dstUV.phy_addr = dstSurf.phyAddr + dstY.h * dstY.pitch;

    srcClip.left = 0;
    srcClip.top = 0;
    srcClip.bottom = srcUV.h;
    srcClip.right = srcUV.w;
    SstarBlitCCW(&srcUV, &dstUV, &srcClip);
    JDEC_PERF(ts1, ts2, 1);

    if (bSinkSurf)
        sink_yuv_surface(&dstSurf, dstData, __FUNCTION__);

    return 0;
}

void __test_blit_clip_DST(MI_GFX_Surface_t srcSurf, MI_GFX_Rect_t srcRect,
                          MI_GFX_Surface_t dstSurf, MI_GFX_Rect_t dstRect, char *dstData, MI_BOOL bSinkSurf)
{
    dstRect.s32Xpos = 0;
    dstRect.s32Ypos = 0;
    dstRect.u32Width = 480;
    dstRect.u32Height = 480;

    clock_gettime(CLOCK_MONOTONIC, &ts1);

    MI_GFX_Opt_t stOpt;
    MI_U16 u16Fence;
    //   dstSurf.eColorFmt = srcSurf.eColorFmt;
    //   dstSurf.u32Stride = 800*4;
    printf("des: rect[%d,%d,%d,%d][%d,%d]\n", dstRect.s32Xpos, dstRect.s32Ypos, dstRect.u32Width, dstRect.u32Height, dstSurf.eColorFmt, dstSurf.u32Stride);

    memset(&stOpt, 0, sizeof(stOpt));
    stOpt.stClipRect.s32Xpos = srcRect.s32Xpos;
    stOpt.stClipRect.s32Ypos = srcRect.s32Ypos;
    stOpt.stClipRect.u32Width = srcRect.u32Width;
    stOpt.stClipRect.u32Height = srcRect.u32Height;

    stOpt.u32GlobalSrcConstColor = 0xFF000000;
    stOpt.u32GlobalDstConstColor = 0xFF000000;
    stOpt.eSrcDfbBldOp = E_MI_GFX_DFB_BLD_ONE;
    stOpt.eDstDfbBldOp = E_MI_GFX_DFB_BLD_ZERO;
    stOpt.eMirror = E_MI_GFX_MIRROR_NONE;
    stOpt.eRotate = E_MI_GFX_ROTATE_0;
    //start = clock();

    MI_GFX_BitBlit(0, &srcSurf, &srcRect, &dstSurf, &dstRect, &stOpt, &u16Fence);
    MI_GFX_WaitAllDone(0, FALSE, u16Fence);
    JDEC_PERF(ts1, ts2, 1);

    if (bSinkSurf)
        _gfx_sink_surface(&dstSurf, dstData, __FUNCTION__);

    //memset(dstData, 0, dstSurf.u32Height * dstSurf.u32Stride);
}

void __test_blit_color_ARGB8888(MI_GFX_Surface_t srcSurf, MI_GFX_Rect_t srcRect,
                                MI_GFX_Surface_t dstSurf, MI_GFX_Rect_t dstRect, char *dstData, MI_BOOL bSinkSurf)
{
    clock_gettime(CLOCK_MONOTONIC, &ts1);

    MI_GFX_Opt_t stOpt;
    MI_U16 u16Fence;

    memset(&stOpt, 0, sizeof(stOpt));

    dstSurf.eColorFmt = E_MI_GFX_FMT_ARGB8888;
    dstSurf.u32Stride = dstSurf.u32Width * getBpp(dstSurf.eColorFmt);
    stOpt.stClipRect.s32Xpos = dstRect.s32Xpos;
    stOpt.stClipRect.s32Ypos = dstRect.s32Ypos;
    stOpt.stClipRect.u32Width = dstRect.u32Width;
    stOpt.stClipRect.u32Height = dstRect.u32Height;

    stOpt.u32GlobalSrcConstColor = 0xFF000000;
    stOpt.u32GlobalDstConstColor = 0xFF000000;
    stOpt.eSrcDfbBldOp = E_MI_GFX_DFB_BLD_ONE;
    stOpt.eDstDfbBldOp = E_MI_GFX_DFB_BLD_ZERO;
    stOpt.eMirror = E_MI_GFX_MIRROR_NONE;
    stOpt.eRotate = E_MI_GFX_ROTATE_0;
    //start = clock();

    MI_GFX_BitBlit(0, &srcSurf, &srcRect, &dstSurf, &dstRect, &stOpt, &u16Fence);
    MI_GFX_WaitAllDone(0, FALSE, u16Fence);
    JDEC_PERF(ts1, ts2, 1);

    if (bSinkSurf)
        _gfx_sink_surface(&dstSurf, dstData, __FUNCTION__);

    //memset(dstData, 0, dstSurf.u32Height * dstSurf.u32Stride);
}
int main()
{
    MI_S32 ret = 0;

    MI_SYS_Init(0);
    MI_GFX_Open(0);
    MI_GFX_Surface_t srcSurf1;
    MI_GFX_Rect_t srcRect1;
    char *srcData1;
    srcSurf1.eColorFmt = E_MI_GFX_FMT_ARGB8888;
    srcSurf1.phyAddr = 0;
    srcSurf1.u32Width = 480;
    srcSurf1.u32Height = 800;
    srcSurf1.u32Stride = srcSurf1.u32Width * getBpp(srcSurf1.eColorFmt);
    srcRect1.s32Xpos = 0;
    srcRect1.s32Ypos = 0;
    srcRect1.u32Height = srcSurf1.u32Height;
    srcRect1.u32Width = srcSurf1.u32Width;
    __create_src_surface_ARGB(&srcSurf1, &srcRect1, &srcData1);

    MI_GFX_Surface_t srcSurf2;
    MI_GFX_Rect_t srcRect2;
    char *srcData2;
    srcSurf2.eColorFmt = E_MI_GFX_FMT_ARGB8888;
    srcSurf2.phyAddr = 0;
    srcSurf2.u32Width = 800;
    srcSurf2.u32Height = 480;
    srcSurf2.u32Stride = srcSurf2.u32Width * getBpp(srcSurf2.eColorFmt);
    srcRect2.s32Xpos = 0;
    srcRect2.s32Ypos = 0;
    srcRect2.u32Height = srcSurf2.u32Height;
    srcRect2.u32Width = srcSurf2.u32Width;

    __create_dst_surface_ARGB(&srcSurf2, &srcRect2, &srcData2);

    MI_GFX_Surface_t dstSurf;
    MI_GFX_Rect_t dstRect;
    char *dstData;
    dstSurf.phyAddr = 0;
    dstSurf.eColorFmt = E_MI_GFX_FMT_ARGB8888;
    dstSurf.u32Width = 800;
    dstSurf.u32Height = 480;
    dstSurf.u32Stride = dstSurf.u32Width * getBpp(dstSurf.eColorFmt);
    dstRect.s32Xpos = 0;
    dstRect.s32Ypos = 0;
    dstRect.u32Height = dstSurf.u32Height;
    dstRect.u32Width = dstSurf.u32Width;
    _gfx_alloc_surface(&dstSurf, &dstData, "dst");
    printf("show __create_dst_surface_ARGB\n");
    getchar();

    _gfx_sink_surface(&srcSurf1, srcData1, "blendsrc1_ARGB8888");
    _gfx_sink_surface(&srcSurf2, srcData2, "blendsrc2_ARGB8888");

    __test_rotate_ARGB_0(srcSurf2, srcRect2, dstSurf, dstRect, dstData, 1);

    printf("show __test_rotate_ARGB_0\n");
    getchar();

    __test_rotate_ARGB_90(srcSurf1, srcRect1, dstSurf, dstRect, dstData, 1);
    printf("show __test_rotate_ARGB_90\n");
    getchar();

    __test_rotate_ARGB_180(srcSurf2, srcRect2, dstSurf, dstRect, dstData, 1);
    printf("show __test_rotate_ARGB_180\n");
    getchar();

    __test_rotate_ARGB_270(srcSurf1, srcRect1, dstSurf, dstRect, dstData, 1);
    printf("show __test_rotate_ARGB_270\n");
    getchar();
    return 0;
}
