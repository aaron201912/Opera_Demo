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

#ifndef _ST_RGN_H_
#define _ST_RGN_H_

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

#include "mi_rgn.h"
#include "common.h"

#define MAX_OSD_NUM_PER_CHN     4
#define ST_OSD_HANDLE_INVALID   0xFFFF

#define RGB2PIXEL1555(r,g,b)	\
	((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) | 0x8000)


#define I4_RED      (1)
#define I4_GREEN    (2)
#define I4_BLUE     (3)
#define I4_BLACK    (12)

typedef enum
{
	ST_OSD_ATTACH_DIVP,
	ST_OSD_ATTACH_VPE,

	ST_OSD_ATTACH_BUTT,
} ST_OSD_ATTACH_TYPE_E;

typedef enum
{
    DMF_Font_Size_16x16 = 0,    // ascii 8x16
    DMF_Font_Size_32x32,        // ascii 16x32
    DMF_Font_Size_48x48,        // ascii 24x48
    DMF_Font_Size_64x64,        // ascii 32x64

    DMF_Font_Size_BUTT,
} DMF_Font_Size_E;

typedef struct
{
    MI_S32 divpChn;
} ST_OSD_ATTACH_DIVP_S;

typedef struct
{
    MI_S32 vpeChn;
    MI_S32 vpePort;
} ST_OSD_ATTACH_VPE_S;

typedef union
{
    ST_OSD_ATTACH_DIVP_S stDivp;
    ST_OSD_ATTACH_VPE_S stVpe;
} ST_OSD_ATTACH_INFO_U;

typedef struct
{
    MI_RGN_HANDLE hHandle;
    ST_OSD_ATTACH_TYPE_E enAttachType;
    ST_OSD_ATTACH_INFO_U uAttachInfo;
} ST_OSD_INFO_S;

typedef struct ST_Sys_Rect_s
{
    MI_U32 u32X;
    MI_U32 u32Y;
    MI_U16 u16PicW;
    MI_U16 u16PicH;
} ST_Rect_T;

typedef struct
{
    MI_U32 u32X;
    MI_U32 u32Y;
} ST_Point_T;

typedef struct
{
    ST_Rect_T stRect;
    MI_RGN_PixelFormat_e ePixelFmt;
    MI_RGN_ChnPort_t stRgnChnPort;
    MI_U32 u32Layer;
    char szBitmapFile[64];
    char szAsciiBitmapFile[64];
} ST_OSD_Attr_T;

#ifndef ExecFunc
#define ExecFunc(_func_, _ret_) \
    do{ \
        MI_S32 s32Ret = MI_SUCCESS; \
        s32Ret = _func_; \
        if (s32Ret != _ret_) \
        { \
            printf("[%s %d]exec function failed, error:%x\n", __func__, __LINE__, s32Ret); \
            return s32Ret; \
        } \
        else \
        { \
            printf("[%s %d]exec function pass\n", __func__, __LINE__); \
        } \
    } while(0)
#endif

/**************************************************
* Init rgn, and init bitmapfile for OSD
* @param            \b IN: chipid, select 0
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_Init(MI_S32 s32ChipId);

/**************************************************
* Deinit rgn, and deinit bitmapfile
* @param            \b IN: chipid, select 0
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_Deinit(MI_S32 s32ChipId);

/**************************************************
* Draw a point on OSD
* @param            \b IN: rgn handle
* @param            \b IN: draw point 
* @param            \b IN: draw color
* @return           \b OUT: 0:   success
                            1:   fail
**************************************************/
MI_S32 ST_OSD_DrawPoint(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, MI_U32 u32Color);

/**************************************************
* Draw a line on OSD
* @param            \b IN: rgn handle
* @param            \b IN: draw start point
* @param            \b IN: draw end point
* @param            \b IN: draw line thickness
* @param            \b IN: draw color
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_DrawLine(MI_RGN_HANDLE hHandle, ST_Point_T stPoint0, ST_Point_T stPoint1, MI_U8 u8BorderWidth, MI_U32 u32Color);

/**************************************************
* Draw a rect on OSD
* @param            \b IN: rgn handle
* @param            \b IN: draw rect  
* @param            \b IN: draw line thickness
* @param            \b IN: draw color
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_DrawRect(MI_RGN_HANDLE hHandle, ST_Rect_T stRect, MI_U8 u8BorderWidth, MI_U32 u32Color);

/**************************************************
* Fast draw a rect on OSD
* @param            \b IN: rgn handle
* @param            \b IN: draw rect  
* @param            \b IN: draw line thickness
* @param            \b IN: draw color
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_DrawRectFast(MI_RGN_HANDLE hHandle, ST_Rect_T stRect, MI_U8 u8BorderWidth, MI_U32 u32Color);

/**************************************************
* Fast clear a rect on OSD
* @param            \b IN: rgn handle
* @param            \b IN: draw rect  
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_ClearRectFast(MI_RGN_HANDLE hHandle, ST_Rect_T stRect);

/**************************************************
* Fill a rect on OSD
* @param            \b IN: rgn handle
* @param            \b IN: draw rect  
* @param            \b IN: draw color
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_FillRect(MI_RGN_HANDLE hHandle, ST_Rect_T stRect, MI_U32 u32Color);

/**************************************************
* Draw a circle on OSD
* @param            \b IN: rgn handle
* @param            \b IN: draw point
* @param            \b IN: circle radii
* @param            \b IN: draw circle start angle
* @param            \b IN: draw circle end angle
* @param            \b IN: draw line thickness
* @param            \b IN: draw color
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_DrawCircle(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, int radii, int from, int end, MI_U8 u8BorderWidth, MI_U32 u32Color);

/**************************************************
* Fill a circle on OSD
* @param            \b IN: rgn handle
* @param            \b IN: draw point
* @param            \b IN: circle radii
* @param            \b IN: draw circle start angle
* @param            \b IN: draw circle end angle
* @param            \b IN: draw color
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_FillCircle(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, int radii, int from, int end, MI_U32 u32Color);

/**************************************************
* Draw a text on OSD
* @param            \b IN: rgn handle
* @param            \b IN: draw point
* @param            \b IN: text string
* @param            \b IN: draw text color
* @param            \b IN: DMF font size
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_DrawText(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, const char *szString, MI_U32 u32Color, DMF_Font_Size_E enSize);

/**************************************************
* Clear drawed OSD region by image format
* @param            \b IN: rgn handle
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_Clear(MI_RGN_HANDLE hHandle);

/**************************************************
* Get canvas info
* @param            \b IN: chipid, select 0
* @param            \b IN: rgn handle
* @param            \b OUT: canvas info
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_GetCanvasInfo(MI_S32 s32ChipId, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t** ppstCanvasInfo);

/**************************************************
* Update the display canvas data
* @param            \b IN: chipid, select 0
* @param            \b IN: rgn handle
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_Update(MI_S32 s32ChipId, MI_RGN_HANDLE hHandle);

/**************************************************
* Create OSD region
* @param            \b IN: chipid, select 0
* @param            \b IN: rgn handle
* @param            \b IN: region type struct
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_Create(MI_S32 s32ChipId, MI_RGN_HANDLE hHandle, MI_RGN_Attr_t *pstRegion);

/**************************************************
* Destroy OSD region
* @param            \b IN: chipid, select 0
* @param            \b IN: rgn handle
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 ST_OSD_Destroy(MI_S32 s32ChipId, MI_RGN_HANDLE hHandle);

/**************************************************
* Init rgn module and bitmapfile for OSD, Create OSD region and overlays it to prestage module outputport
* @param            \b IN: buffer object
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 sstar_init_rgn(buffer_object_t *buf_obj);

/**************************************************
* Deinit rgn and bitmapfile, Destroy OSD region and remove it from the prestage module outputport
* @param            \b IN: buffer object
* @return           \b OUT: 0:   success
**************************************************/
MI_S32 sstar_deinit_rgn(buffer_object_t *buf_obj);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif //_ST_RGN_H_

