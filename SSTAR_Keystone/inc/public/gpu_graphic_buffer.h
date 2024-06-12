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

#include "gpu_graphic_common.h"

#include <string>

enum CpuAccess {
    READ,
    WRITE,
    READWRITE,
};

class GpuGraphicBuffer {
  public:
    GpuGraphicBuffer() = delete;
    GpuGraphicBuffer(uint32_t inWidth, uint32_t inHeight, uint32_t inFormat,
                     uint32_t inStride, bool enableAfbc = false);
    GpuGraphicBuffer(int32_t inFd, uint32_t inWidth, uint32_t inHeight, uint32_t inFormat,
                     uint32_t* inStride, uint32_t* inPlaneOffset);
    ~GpuGraphicBuffer();
    bool initCheck() const              { return mInitCheck; }
    uint32_t getWidth() const           { return mWidth; }
    uint32_t getHeight() const          { return mHeight; }
    uint32_t getFormat() const          { return mFormat; }
    uint64_t getModifier() const        { return mModifier; }
    uint32_t getFd() const              { return mFd; }
    uint32_t getBufferSize() const      { return mSize; }
    uint32_t *getStride()               { return mStride; }
    uint32_t *getPlaneOffset()          { return mPlaneOffset; }
    bool getInUse()                     { return mInUse; }
    void setInUse(bool inUse)           { mInUse = inUse; }

    void *map(CpuAccess access);
    void flushCache(CpuAccess access);
    void unmap(void *virAddr, CpuAccess access);

  private:
    bool initWithSize(uint32_t inWidth, uint32_t inHeight, uint32_t inFormat, uint32_t inStride);
    bool initAfbcWithSize(uint32_t inWidth, uint32_t inHeight, uint32_t inFormat, uint32_t inStride);
    bool initWithFd(int32_t inFd, uint32_t inWidth, uint32_t inHeight, uint32_t inFormat,
                    uint32_t* inStride, uint32_t* inPlaneOffset);
    int32_t allocDmaBuffer(std::string heapName, uint32_t length);

    bool mInitCheck;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mSize;
    uint32_t mFormat;
    uint64_t mModifier;
    uint32_t mStride[3];
    uint32_t mPlaneOffset[3];
    int32_t mFd;
    bool mInUse;
};
