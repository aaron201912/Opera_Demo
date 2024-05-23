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

#pragma once

#include <stdio.h>
#include <stdint.h>

extern int gGpugfxInitCount;

typedef struct {
    /// Minimum X coordinate of the rectangle.
    uint32_t left;
    /// Minimum Y coordinate of the rectangle.
    uint32_t top;
    /// Maximum X coordinate of the rectangle.
    uint32_t right;
    /// Maximum Y coordinate of the rectangle.
    uint32_t bottom;
} Rect;

typedef struct {
    /// Minimum X float coordinate of the rectangle.
    float left;
    /// Minimum Y float coordinate of the rectangle.
    float top;
    /// Maximum X float coordinate of the rectangle.
    float right;
    /// Maximum Y float coordinate of the rectangle.
    float bottom;
} FloatRect;

enum Transform {
    NONE = 0,
    /* Flip source image horizontally */
    FLIP_H,
    /* Flip source image vertically */
    FLIP_V,
    /* Rotate source image 90 degrees clock-wise */
    ROT_90,
    /* Rotate source image 180 degrees */
    ROT_180,
    /* RRotate source image 270 degrees clock-wise */
    ROT_270,
    /* Flip source image horizontally, the rotate 90 degrees clock-wise */
    FLIP_H_ROT_90,
    /* Flip source image vertically, the rotate 90 degrees clock-wise */
    FLIP_V_ROT_90,
};

#define GPUGFX_LOGE(args...) printf("ERR: " args)
#define GPUGFX_LOGW(args...) printf("WARN: " args)
#define GPUGFX_LOGI(args...) printf("INFO: " args)
#define GPUGFX_LOGD(args...) printf("DBG: " args)
#define GPUGFX_LOGV(args...) printf("VERBOSE: " args)

#define GPUGFX_ALIGN_UP(val, alignment) ((((val) + (alignment) - 1) / (alignment)) * (alignment))
#define GPUGFX_IS_ALIGNED(val, alignment) (!((uintptr_t)(val) & ((alignment)-1)))

#define GPUGFX_STRIDE_ALIGN 64
#define GPUGFX_AFBC_WIDTH_ALIGN 16
#define GPUGFX_AFBC_HEIGHT_ALIGN 16
#define GPUGFX_AFBC_TOTAL_PIXELS_IN_BLOCK 256
#define GPUGFX_AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY 16

