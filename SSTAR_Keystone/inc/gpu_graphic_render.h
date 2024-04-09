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
#include "gpu_graphic_buffer.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <glm/glm.hpp>

#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>

typedef struct {
    // Source buffer crop region.
    Rect srcRect;
    // Destination  buffer operation region.
    Rect dstRect;
    // Mirror/filp or rotate operation.
    Transform transfrom;
    // Scale down filter for texture.
    GLuint scaleDownfilter = GL_NEAREST;
    // Scale up filter for texture.
    GLuint scaleUpfilter = GL_NEAREST;
    // Source buffer blending factor.
    GLuint srcBlendfactor = GL_SRC_ALPHA;
    // Destination  buffer blending factor.
    GLuint dstBlendfactor = GL_ONE_MINUS_SRC_ALPHA;
} GrBitblitInfo;

typedef struct {
    // Fill region.
    Rect rect;
    // Fill color.
    uint32_t color;
} GrFillInfo;

class GpuGraphicRender {
  public:
    enum AttribLocation {
        /* Position of each vertex for vertex shader */
        LOCATION_POSITION = 0,
        /* Coordinates for texture mapping */
        LOCATION_TEXCOORDS,
    };

    enum RenderType {
        /* Fill render type */
        FILL = 0,
        /* Bitblit render type */
        BITBLIT,
    };

    GpuGraphicRender();
    virtual ~GpuGraphicRender();

    /*
     * Init gpu graphic render, must call before any operation
     *
     * format：Gpu graphic render fourcc format
     */
    int32_t init(uint32_t format);

    /*
     * Deinit gpu graphic render, must call after not need to use
     * gpu graphic render anymore
     */
    int32_t deinit();

    /*
     * Force clean gpu graphic render buffer cache if needed,
     * buffer cache will clean in deinit operation
     */
    void cleanBufferCache();

    /*
     * Fill a specific area of the destination  bitmap with a certain color
     *
     * dstBuffer: Destination  bitmap buffer
     * info：Information of fill region and color
     */
    int32_t fill(std::shared_ptr<GpuGraphicBuffer> dstBuffer, GrFillInfo info);

    /*
     * Bitblit from source bitmap to destination  bitmap
     *
     * srcBuffer: Source bitmap buffer
     * dstBuffer: Destination  bitmap buffer
     * info：Information of bitblit operation, such as blend, region.
     */
    int32_t bitblit(std::shared_ptr<GpuGraphicBuffer> srcBuffer,
                       std::shared_ptr<GpuGraphicBuffer> dstBuffer, GrBitblitInfo info);

  private:
    EGLBoolean eglInit();
    GLboolean glesInit();
    GLboolean glesInitShaders();
    void glesInitTextures();
    void eglCleanup();
    void glesCleanup();
    void glesCleanupTextures();
    void glesCleanupShaders();
    EGLImageKHR createEGLImageIfNeeded(std::shared_ptr<GpuGraphicBuffer> buffer);
    void generateVertex(FloatRect cropRect, Rect dstRect, Transform transfrom);
    uint64_t getUniqueId(int32_t fd);
    int32_t fillThreaded(std::shared_ptr<GpuGraphicBuffer> dstBuffer, GrFillInfo info);
    int32_t bitblitThreaded(std::shared_ptr<GpuGraphicBuffer> srcBuffer,
                            std::shared_ptr<GpuGraphicBuffer> dstBuffer, GrBitblitInfo info);
    int32_t setSchedFifo();
    void threadMain();

    mutable std::mutex mInitMutex;
    bool mInit;
    mutable std::condition_variable mInitCondition;

    uint32_t mFormat;
    GLint mTarget;
    GLuint mProgram;
    GLuint mDstTexture;
    GLuint mFramebuffer;
    GLuint mSrcTexture;
    GLuint mVertexShader;
    GLuint mFragmentShader;

    /* location of the projection matrix uniform */
    GLuint mProjectionMatrixLoc;

    // Maximum size of mImageCache. If more images would be cached, then (approximately)
    // the last recently used buffer should be kicked out.
    uint32_t mImageCacheSize = 0;

    // Cache of input images, keyed by corresponding dma buffer ID.
    std::deque<std::pair<uint64_t, EGLImageKHR>> mImageCache;
    std::mutex mImageCacheMutex;

    const char* const mThreadName = "Gpu graphic render thread";
    // Protects the creation and destruction of mThread.
    mutable std::mutex mThreadMutex;
    std::thread mThread;
    bool mRunning = true;
    mutable std::condition_variable mCondition;
    bool mWaitForProcess;
    mutable std::mutex mProcessMutex;
    mutable std::condition_variable mProcessCondition;
    mutable std::mutex mMainMutex;

    std::shared_ptr<GpuGraphicBuffer> mDstBuffer;
    std::shared_ptr<GpuGraphicBuffer> mSrcBuffer;
    GrBitblitInfo mBitblitInfo;
    GrFillInfo mFillInfo;
    int32_t mProcessStatus;
    RenderType mRenderType;

    EGLDisplay mDisplay;
    EGLContext mContext;
    pid_t mThreadTid;
};

