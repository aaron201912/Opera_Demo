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

#ifndef __SSTAR_DET_API_H__
#define __SSTAR_DET_API_H__

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef MAX_DET_OBJECT
#define MAX_DET_OBJECT 200
#endif

#ifndef MAX_DET_STRLEN
#define MAX_DET_STRLEN 256
#endif

#ifndef MAX_DET_CLASSES
#define MAX_DET_CLASSES 10
#endif

#ifndef TYPEDEF_LABEL_PD_PERSON_E
#define TYPEDEF_LABEL_PD_PERSON_E
typedef enum
{
    E_PD_PERSON = 0
} Label_PD_Person_e;
#endif


#ifndef TYPEDEF_LABEL_FD_FACE_E
#define TYPEDEF_LABEL_FD_FACE_E
typedef enum
{
    E_FD_FACE = 0,
} Label_FD_Face_e;
#endif

#ifndef TYPEDEF_LABEL_PCN_E
#define TYPEDEF_LABEL_PCN_E
typedef enum
{
    E_PCN_PERSON = 0,
    E_PCN_BICYCLE,
    E_PCN_CAR,
    E_PCN_MOTOCYCLE,
    E_PCN_BUS,
    E_PCN_TRUCK
} Label_PCN_e;
#endif


#ifndef TYPEDEF_LABEL_PCD_E
#define TYPEDEF_LABEL_PCD_E
typedef enum
{
    E_PCD_PERSON,
    E_PCD_CAT,
    E_PCD_DOG
} Label_PCD_e;
#endif


#ifndef TYPEDEF_LABEL_FSD_E
#define TYPEDEF_LABEL_FSD_E
typedef enum
{
    E_FSD_FIRE,
    E_FSD_SMOKE
} Label_FSD_e;
#endif

#ifndef TYPEDEF_LABEL_SD_E
#define TYPEDEF_LABEL_SD_E
typedef enum
{
    E_SD_PERSON = 0,
    E_SD_BICYCLE,
    E_SD_CAR,
    E_SD_MOTOCYCLE,
    E_SD_BUS,
    E_SD_TRUCK,
    E_SD_CAT,
    E_SD_DOG,
    E_SD_HEAD,
    E_SD_FACE
} LABEL_SD_e;
#endif

#ifndef TYPEDEF_LABEL_SPD_E
#define TYPEDEF_LABEL_SPD_E
typedef enum
{
    E_SPD_PERSON = 0,
    E_SPD_HEAD,
    E_SPD_FACE
} LABEL_SPD_e;
#endif


#ifndef TYPEDEF_LABEL_S12D_E
#define TYPEDEF_LABEL_S12D_E
typedef enum
{
    E_S12D_PERSON = 0,
    E_S12D_BICYCLE,
    E_S12D_CAR,
    E_S12D_MOTOCYCLE,
    E_S12D_BUS,
    E_S12D_TRUCK,
    E_S12D_CAT,
    E_S12D_DOG,
    E_S12D_HEAD,
    E_S12D_FACE,
    E_S12D_COW,
    E_S12D_SHEEP,
} LABEL_S12D_e;
#endif

#ifndef TYPEDEF_LABEL_COWSHEEP_E
#define TYPEDEF_LABEL_COWSHEEP_E
typedef enum
{
    E_COWSHEEP_COW= 0,
    E_COWSHEEP_SHEEP,
} LABEL_COWSHEEP_e;
#endif

#ifndef TYPEDEF_LABEL_HOMEWORK_E
#define TYPEDEF_LABEL_HOMEWORK_E
typedef enum
{
    E_HOMEWORK_READ= 0,
    E_HOMEWORK_WRITE,
    E_HOMEWORK_PHONE
} LABEL_HOMEWORK_e;
#endif

#ifndef TYPEDEF_RECT_T
#define TYPEDEF_RECT_T
typedef struct
{
    MI_U32 width;
    MI_U32 height;
} Rect_t;
#endif

#ifndef TYPEDEF_INPUTATTR_T
#define TYPEDEF_INPUTATTR_T
typedef struct
{
    MI_U32 width;
    MI_U32 height;
    MI_IPU_ELEMENT_FORMAT format;
} InputAttr_t;
#endif

#ifndef TYPEDEF_BOX_T
#define TYPEDEF_BOX_T
typedef struct
{
    MI_U32 x;
    MI_U32 y;
    MI_U32 width;
    MI_U32 height;
    MI_U32 class_id;
    MI_FLOAT score;
    MI_U64 pts;
} Box_t;
#endif

typedef struct
{
    char ipu_firmware_path[MAX_DET_STRLEN]; // ipu_firmware.bin path
    char model[MAX_DET_STRLEN];             // detect model path
    MI_FLOAT threshold;                     // confidence
    Rect_t disp_size;
    MI_BOOL had_create_device; // set true to handle ipu device outside algo lib
} DetectionInfo_t;

#ifndef TYPEDEF_ALGO_INPUT_T
#define TYPEDEF_ALGO_INPUT_T
typedef struct
{
    void *p_vir_addr;
    MI_PHY phy_addr;
    MI_U32 buf_size;
    MI_U64 pts;
} ALGO_Input_t;
#endif


#ifndef TYPEDEF_SORT_TYPE_E
#define TYPEDEF_SORT_TYPE_E
typedef enum
{
    E_SCORE_DESCENDING = 0,
    E_AREA_DESCENDING,
} SORT_TYPE_e;
#endif


#ifndef TYPEDEF_SORT_INPUT_T
#define TYPEDEF_SORT_INPUT_T
typedef struct
{
    MI_S32 class_indexs[MAX_DET_CLASSES];
    MI_S32 class_num;
    SORT_TYPE_e sort_type;
}Sort_Input_t;
#endif

typedef struct
{
    MI_BOOL enable; // false
    MI_BOOL ignore_static_objects;   //false
    MI_FLOAT static_sensitive;    //0.85
    MI_BOOL stable_bbox;     //false
    MI_FLOAT stable_sensitive;    //0.63
    MI_S32 ignore_frame_number; //[0-5] ignore the id first number box of detect,default=0
} DetTrackParams_t;

typedef struct
{
    MI_BOOL enable; // false
}
DetStrictModeParams_t;


typedef struct
{
    void *model_buffer;
    MI_U32 model_buffer_len;
    MI_FLOAT threshold;                     // confidence
    Rect_t disp_size;
    MI_BOOL had_create_device; // set true to handle ipu device outside algo lib
} DetectionInfo2_t;


MI_S32 ALGO_DET_CreateHandle(void **handle);
MI_S32 ALGO_DET_InitHandle(void *handle, DetectionInfo_t *init_info);
MI_S32 ALGO_DET_InitHandle2(void *handle, DetectionInfo2_t *init_info);
MI_S32 ALGO_DET_SetTracker(void *handle, MI_S32 tk_type, MI_S32 md_type);
MI_S32 ALGO_DET_SetTracker2(void *handle, DetTrackParams_t track_params);
MI_S32 ALGO_DET_SetStableBox(void *handle, MI_BOOL stable);
MI_S32 ALGO_DET_GetInputAttr(void *handle, InputAttr_t *input_attr);
MI_S32 ALGO_DET_SetThreshold(void *handle, MI_FLOAT threshold);
MI_S32 ALGO_DET_SetStrictMode(void *handle, DetStrictModeParams_t params);
MI_S32 ALGO_DET_Run(void *handle, const ALGO_Input_t *algo_input, Box_t bboxes[MAX_DET_OBJECT], MI_S32 *num_bboxes);
MI_S32 ALGO_DET_SortResult(void *handle, const Sort_Input_t *sort_input, Box_t bboxes[MAX_DET_OBJECT], MI_S32 *num_bboxes);
MI_S32 ALGO_DET_DeinitHandle(void *handle);
MI_S32 ALGO_DET_ReleaseHandle(void *handle);

#ifdef __cplusplus
}
#endif

#endif