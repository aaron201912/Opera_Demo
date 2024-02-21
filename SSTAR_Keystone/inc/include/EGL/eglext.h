/* Copyright:
 * ----------------------------------------------------------------------------
 * This confidential and proprietary software may be used only as authorized
 * by a licensing agreement from ARM Limited.
 *      (C) COPYRIGHT 2013, 2016, 2020-2021 ARM Limited, ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorized copies and
 * copies may only be made to the extent permitted by a licensing agreement
 * from ARM Limited.
 * ----------------------------------------------------------------------------
 */


/* Include the original header */
#include "eglext_khronos.h"

#ifndef EGL_EXT_surface_compression
#define EGL_EXT_surface_compression 1

#define EGL_SURFACE_COMPRESSION_EXT                     0x34B0
#define EGL_SURFACE_COMPRESSION_PLANE1_EXT              0x328E
#define EGL_SURFACE_COMPRESSION_PLANE2_EXT              0x328F
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_NONE_EXT     0x34B1
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_DEFAULT_EXT  0x34B2
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_1BPC_EXT     0x34B4
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_2BPC_EXT     0x34B5
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_3BPC_EXT     0x34B6
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_4BPC_EXT     0x34B7
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_5BPC_EXT     0x34B8
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_6BPC_EXT     0x34B9
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_7BPC_EXT     0x34BA
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_8BPC_EXT     0x34BB
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_9BPC_EXT     0x34BC
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_10BPC_EXT    0x34BD
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_11BPC_EXT    0x34BE
#define EGL_SURFACE_COMPRESSION_FIXED_RATE_12BPC_EXT    0x34BF
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYSUPPORTEDCOMPRESSIONRATESEXTPROC) (EGLDisplay dpy, EGLConfig config, const EGLAttrib *attrib_list, EGLint *rates, EGLint rate_size, EGLint *num_rates);
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglQuerySupportedCompressionRatesEXT (EGLDisplay dpy, EGLConfig config, const EGLAttrib *attrib_list, EGLint *rates, EGLint rate_size, EGLint *num_rates);
#endif

#endif /* EGL_EXT_surface_compression */
