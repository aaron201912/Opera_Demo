/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2011-2014, 2017-2018, 2020-2021 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file
 * Platform related definitions
 */

#ifndef _EGLPLATFORM_H_
#define _EGLPLATFORM_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include <KHR/khrplatform.h>
#include <stdint.h>

#include "mali_fbdev_types.h"

#ifndef EGLAPI
#define EGLAPI
#endif

#ifndef EGLAPIENTRY
#define EGLAPIENTRY
#endif

#ifndef EGLAPIENTRYP
#define EGLAPIENTRYP EGLAPIENTRY *
#endif

typedef fbdev_window * EGLNativeWindowType;

typedef int32_t EGLNativePixmapType;

#if defined(EGL_FBDEV) && EGL_FBDEV
typedef void * EGLNativeDisplayType;
#elif defined(EGL_DUMMY) && EGL_DUMMY
typedef dummy_display * EGLNativeDisplayType;
#else
#error "Window system unknown"
#endif

typedef EGLNativeWindowType NativeWindowType;
typedef EGLNativePixmapType NativePixmapType;
typedef EGLNativeDisplayType NativeDisplayType;

typedef int32_t EGLint;

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* C++ / C typecast macros for special EGL handle values */
#if defined(__cplusplus)
#define EGL_CAST(type, value) (static_cast<type>(value))
#else
#define EGL_CAST(type, value) ((type) (value))
#endif

#endif /* _EGLPLATFORM_H_ */
