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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <glm/glm.hpp>

#include <deque>
#include <thread>
#include <mutex>

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

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t format;
    int32_t fds[3];
    uint32_t stride[3];
    uint32_t dataOffset[3];
} GraphicBuffer;

enum CpuAccess {
    READ,
    WRITE,
    READWRITE,
};

class GpuGraphicBuffer {
  public:
    GpuGraphicBuffer() = delete;
    GpuGraphicBuffer(uint32_t inWidth, uint32_t inHeight, uint32_t inFormat, uint32_t inStride);
    GpuGraphicBuffer(int32_t inFd, uint32_t inWidth, uint32_t inHeight, uint32_t inFormat,
                     uint32_t *inStride, uint32_t *inPlaneOffset);
    ~GpuGraphicBuffer();
    bool initCheck() const              { return mInitCheck; }
    uint32_t getWidth() const           { return mWidth; }
    uint32_t getHeight() const          { return mHeight; }
    uint32_t getFormat() const          { return mFormat; }
    uint32_t getFd() const              { return mFd; }
    uint32_t getBufferSize() const      { return mSize; }
    uint32_t *getStride()               { return mStride; }
    uint32_t *getPlaneOffset()          { return mPlaneOffset; }
    void *map(CpuAccess access);
    void flushCache(CpuAccess access);
    void unmap(void *virAddr, CpuAccess access);

  private:
    bool initWithSize(uint32_t inWidth, uint32_t inHeight, uint32_t inFormat, uint32_t inStride);
    bool initWithFd(int32_t inFd, uint32_t inWidth, uint32_t inHeight, uint32_t inFormat,
                    uint32_t *inStride, uint32_t *inPlaneOffset);

    bool mInitCheck;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mSize;
    uint32_t mFormat;
    uint32_t mStride[3];
    uint32_t mPlaneOffset[3];
    int32_t mFd;
};

class GpuGraphicEffect {
  public:
    enum AttribLocation {
        /* Position of each vertex for vertex shader */
        LOCATION_POSITION = 0,
        /* Coordinates for texture mapping */
        LOCATION_TEXCOORDS,
    };

    enum EffectType {
        EFFECT_NONE,
        /* Rotate, mirror or filp operation */
        TRANSFORM,
        /* Keystone correction, include tranform operation */
        KEYSTONE_CORRECTION,
    };

    GpuGraphicEffect();
    virtual ~GpuGraphicEffect();

    /*
     * Init gpu graphic efffect, must call before any operation
     *
     * width: Output framebuffer width
     * height: Output framebuffer height
     * format：Output framebuffer fourcc format
     */
    int32_t init(int32_t width, int32_t height, uint32_t format);

    /*
     * Deinit gpu graphic efffect, must call after not need to use
     * gpu graphic efffect anymore
     */
    int32_t deinit();

    /*
     * Force clean gpu graphic efffect input buffer cache if needed,
     * buffer cache will clean in deinit operation
     */
    void cleanBufferCache();

    /*
     * Update keystone correction left top point offset
     *
     * x: point x direction coordinate
     * y: point y direction coordinate
     */
    bool updateLTPointOffset(uint32_t x, uint32_t y);

    /*
     * Update keystone correction right top point offset
     *
     * x: point x direction coordinate
     * y: point y direction coordinate
     */
    bool updateRTPointOffset(uint32_t x, uint32_t y);

    /*
     * Update keystone correction left bottom point offset
     *
     * x: point x direction coordinate
     * y: point y direction coordinate
     */
    bool updateLBPointOffset(uint32_t x, uint32_t y);

    /*
     * Update keystone correction right bottom point offset
     *
     * x: point x direction coordinate
     * y: point y direction coordinate
     */
    bool updateRBPointOffset(uint32_t x, uint32_t y);

    /*
     * Update input buffer transform status(rotate/mirror/filp) on framebuffer
     *
     * transform: transform status(rotate/mirror/filp)
     */
    void updateTransformStatus(Transform transform);

    /*
     * Gpu graphic efffect process, before use it to do keystone correction or buffer
     * transform operation, you need call updateLT/RT/RB/LBPointOffset to update keystone correction
     * point offest or updateTransformStatus to update buffer transform status.
     *
     * inputBuffer: Input buffer for gpu graphic efffect process
     * displayFrame: Input buffer display region on framebuffer, this must not exceed framebuffer size
     * outputBuffer: Output buffer you can get after gpu graphic efffect process
     */
    int32_t process(std::shared_ptr<GpuGraphicBuffer> inputBuffer, Rect displayFrame,
                    std::shared_ptr<GpuGraphicBuffer>& outputBuffer);

  private:
    EGLBoolean eglInit();
    GLboolean glesInit();
    GLboolean glesInitShaders();
    void glesInitTexturing();
    GLboolean glesInitRenderTargets();
    void eglCleanup();
    void glesCleanup();
    void glesCleanupRenderTargets();
    void glesCleanupTextures();
    void glesCleanupShaders();

    void updateEffectType();
    void generateVertex(Rect displayFrame);
    uint64_t getUniqueId(int32_t fd);
    EGLImageKHR createEGLImageIfNeeded(std::shared_ptr<GpuGraphicBuffer> buffer, bool isFbo);
    bool generateKeystoneCorrectionVertexAndTexCoord(glm::vec2 position[4],
                                                     int32_t widthSegments, int32_t heightSegments,
                                                     float*& pVertex, int32_t vertexStride,
                                                     float*& pTexCood, int32_t texStride);
    bool updateDisplayDeformationMatrix();

    std::mutex mInitMutex;
    bool mInit;
    uint32_t mCurrentFrame;
    uint32_t mFboWidth;
    uint32_t mFboHeight;
    uint32_t mFboFormat;
    GLint mTarget;
    GLuint mProgram;
    GLuint mTexture;
    GLuint mVertexShader;
    GLuint mFragmentShader;

    /* location of the projection matrix uniform */
    GLuint mProjectionMatrixLoc;

    EGLDisplay mDisplay;
    EGLContext mContext;
    pid_t mThreadTid;

    // Maximum size of mImageCache. If more images would be cached, then (approximately)
    // the last recently used buffer should be kicked out.
    uint32_t mImageCacheSize = 0;

    // Cache of input images, keyed by corresponding dma buffer ID.
    std::deque<std::pair<uint64_t, EGLImageKHR>> mImageCache;
    std::mutex mImageCacheMutex;
    std::mutex mMainMutex;

    // Keystone correction points offset
    uint32_t mPointLBOffsetX = 0;
    uint32_t mPointLBOffsetY = 0;
    uint32_t mPointRBOffsetX = 0;
    uint32_t mPointRBOffsetY = 0;
    uint32_t mPointRTOffsetX = 0;
    uint32_t mPointRTOffsetY = 0;
    uint32_t mPointLTOffsetX = 0;
    uint32_t mPointLTOffsetY = 0;

    glm::mat4 mDeformationMatrix;

    // Rotate, mirror or flip status
    Transform mTransform = NONE;

    EffectType mEffectType = EFFECT_NONE;

    typedef struct {
        uint32_t fboTexName;
        uint32_t fboName;
        std::shared_ptr<GpuGraphicBuffer> buffer;
        EGLImageKHR eglImage;
    } EglFramebufferInfo;
    std::deque<EglFramebufferInfo> mFramebufferInfo;
};
