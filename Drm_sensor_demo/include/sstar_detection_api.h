 #pragma once
#ifndef __SSTAR_DETECT_API_H__
#define __SSTAR_DETECT_API_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "mi_sys_datatype.h"
#include "mi_common_datatype.h"

#ifndef TYPEDEF_LABEL_PERSON_E
#define TYPEDEF_LABEL_PERSON_E
typedef enum
{
    PERSON,
}Label_Person_e;
#endif 

#ifndef TYPEDEF_LABEL_FACE_E
#define TYPEDEF_LABEL_FACE_E
typedef enum
{
    FACE,
}Label_Face_e;
#endif 

#ifndef TYPEDEF_LABEL_PCN_E
#define TYPEDEF_LABEL_PCN_E
typedef enum
{
    PCN_PERSON,
    PCN_BICYCLE,
    PCN_CAR,
    PCN_MOTOCYCLE,
    PCN_BUS,
    PCN_TRUCK,
}Label_PCN_e;
#endif 

#ifndef TYPEDEF_LABEL_PCD_E
#define TYPEDEF_LABEL_PCD_E
typedef enum
{
    PCD_PERSON,
    PCD_CAT,
    PCD_DOG,
}Label_PCD_e;
#endif 

#ifndef TYPEDEF_RECT_T
#define TYPEDEF_RECT_T
typedef struct
{
    MI_U32 width;
    MI_U32 height;
}Rect_t;
#endif 

#ifndef TYPEDEF_INPUTATTR_T
#define TYPEDEF_INPUTATTR_T
typedef struct
{
    MI_U32 width;
    MI_U32 height;
    MI_IPU_ELEMENT_FORMAT format;
}InputAttr_t;
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
}Box_t;
#endif 

typedef struct
{
    char ipu_firmware_path[128];           // ipu_firmware.bin path
    char Model[128];                       // detect model path
    MI_FLOAT threshold;                    // confidence
    Rect_t disp_size;
    MI_S32 (*create_device_callback)(MI_IPU_DevAttr_t* stDevAttr, char *pFirmwarePath);
    void (*detect_callback)(std::vector<Box_t> results);
    void (*destory_device_callback)();
}DetectionInfo_t;

MI_S32 getDetectionManager(void** manager);
MI_S32 initDetection(void* manager, DetectionInfo_t * InitInfo);
MI_S32 startDetect(void* manager);
MI_S32 setTracker(void* manager, MI_S32 tk_type, MI_S32 md_type);
MI_S32 openStableBox(void* manager, bool stable);
// InputAttr_t getInputAttr(void* manager);
Rect_t getInputSize(void* manager);
MI_S32 setThreshold(void* manager, float threshold);
MI_S32 doDetectFace(void* manager, const MI_SYS_BufInfo_t *stBufInfo);
MI_S32 doSyncDetectFace(void* manager, const MI_SYS_BufInfo_t *stBufInfo, std::vector<Box_t> *results);
MI_S32 doDetectPerson(void* manager, const MI_SYS_BufInfo_t *stBufInfo);
MI_S32 doDetectTS(void* manager, const MI_SYS_BufInfo_t *stBufInfo);
MI_S32 doDetectPF(void* manager, const MI_SYS_BufInfo_t *stBufInfo);
MI_S32 doDetect(void* manager, const MI_SYS_BufInfo_t *stBufInfo);
MI_S32 doSnDetect(void* manager, const MI_SYS_BufInfo_t *stBufInfo);
MI_S32 stopDetect(void* manager);
MI_S32 putDetectionManager(void* manager);



#ifdef __cplusplus
}
#endif

#endif
