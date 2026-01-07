# Detection Algorithm

---

## REVISION HISTORY

| Revision No. | Description    | Date       |
| ------------ | -------------- | ---------- |
| 1.0          | First version  | 04/25/2023 |
| 1.1          | Second version | 10/30/2023 |
| 3.0          | Third version  | 04/11/2024 |
| 3.1          | Remove deprecated models introduction  | 10/31/2024 |
| 3.2          | Add ALGO_DET_SortResult API | 12/03/2024 |
| 3.3          | Adjust models description | 12/06/2024 |

## 1. General Description

### 1.1. Algorithm Description

The mainly specific detection labels are described as follows:

- Face detection(SFD/SYFL) outputs one label, i.e. face(class_id=0), where SYFL represents the face detection model with key points;

- Fire and smoke detection(SFSD) outputs two labels, i.e. fire(class_id=0), smoke(class_id=1);

- Person + vehicle + pet + head + face(SD) outputs ten labels, i.e. person(class_id=0), bicycle(class_id=1), cat(class_id=2), motorcycle(class_id=3), bus(class_id=4), truck(class_id=5), cat(class_id=6), dog(class_id=7), head(class_id=8), face(class_id=9);

- Person + head + face(SPD) outputs three labels, i.e. person(class_id=0), head(class_id=1), face(class_id=2);

- Bag Detection(SBD) outputs one label, i.e. bag(class_id=0);

- The y in the model name indicates YUV input, and the number indicates the resolution. The specific resolution can be obtained through [ALGO_DET_GetInputAttr](#25-algo_det_getinputattr);


## 2. API Reference

This function module provides the following APIs:

| API Name                                              | Function                         |
| ---------------------------------------------------- | ---------------------------- |
| [ALGO_DET_CreateHandle](#21-algo_det_createhandle)   | Create handle                     |
| [ALGO_DET_InitHandle](#22-algo_det_inithandle)       | Initialize handle                    |
| [ALGO_DET_SetTracker](#23-algo_det_settracker)       | Switch to set post-processing tracker algorithm       |
| [ALGO_DET_SetStableBox](#24-algo_det_setstablebox)   | Switch to set post-processing stable box         |
| [ALGO_DET_GetInputAttr](#25-algo_det_getinputattr)   | Get model attribute information           |
| [ALGO_DET_SetThreshold](#26-algo_det_setthreshold)   | Set threshold                     |
| [ALGO_DET_Run](#27-algo_det_run)                     | Run detection algorithm                  |
| [ALGO_DET_DeinitHandle](#28-algo_det_deinithandle)   | Deinitialize detection algorithm                  |
| [ALGO_DET_ReleaseHandle](#29-algo_det_releasehandle) | Release handle                     |
| [ALGO_DET_SetTracker2](#210-algo_det_settracker2)    | Set parameters of post-processing tracker algorithm |
| [ALGO_DET_SetStrictMode](#211-algo_det_setstrictmode)    | Set parameters of strict detection mode |
| [ALGO_DET_InitHandle2](#212-algo_det_inithandle2)       | Initialize the algorithm using the model memory block pointer   |
| [ALGO_DET_SortResult](#213-algo_det_sortresult)       | Sort detection results  |


### 2.1. ALGO_DET_CreateHandle

-   Function

    Create handle

-   Syntax

        MI_S32 ALGO_DET_CreateHandle(void **handle);

-   Parameter

    | Parameter Name | Description | Input/Output |
    | -------------- | ----------- | ------------ |
    | handle         | Handle      | Input        |

-   Return Value

    | Return Value | Description                                     |
    | ------------ | ----------------------------------------------- |
    | 0            | Successful.                                          |
    | Other        | Failed, see [Error Code](#errorcode) for details  |

-   Dependency

    - Header file: sstar_det_api.h
    - Library file: libsstaralgo_det.a / libsstaralgo_det.so

### 2.2. ALGO_DET_InitHandle

-   Function

    Initialize handle

-   Syntax

        MI_S32 ALGO_DET_InitHandle(void *handle, DetectionInfo_t *init_info);


-   Parameter

    | Parameter Name | Description                                                                                      | Input/Output |
    | -------------- | ------------------------------------------------------------------------------------------------ | ------------ |
    | handle         | Handle                                                                                           | Input        |
    | init_info      | Detection algorithm configuration option, see [DetectionInfo_t](#34-detectioninfo_t) for details | Input        |

-   Return Value

    | Return Value | Description                                     |
    | ------------ | ----------------------------------------------- |
    | 0            | Successful.                                          |
    | Other        | Failed, see [Error Code](#errorcode) for details  |

-   Dependency

    - Header file: sstar_det_api.h
    - Library file: libsstaralgo_det.a / libsstaralgo_det.so

### 2.3. ALGO_DET_SetTracker

-   Function

    Switch to set post-processing tracker algorithm

-   Syntax

        MI_S32 ALGO_DET_SetTracker(void *handle, MI_S32 tk_type, MI_S32 md_type);


-   Parameter

    | Parameter Name | Description                                                                                                                            | Input/Output |
    | -------------- | -------------------------------------------------------------------------------------------------------------------------------------- | ------------ |
    | handle         | Handle                                                                                                                                 | Input        |
    | tk_type        | Tracker algorithm switch (set 0 to disable, 1 to enable; default is disable)                                                           | Input        |
    | md_type        | Moving target detection switch (0: Both moving and stationary targets are detected, 1: Only moving targets are detected. Default is 0) | Input        |

-   Return Value

    | Return Value | Description                                     |
    | ------------ | ----------------------------------------------- |
    | 0            | Successful.                                          |
    | Other        | Failed, see [Error Code](#errorcode) for details  |

-   Dependency

    - Header file: sstar_det_api.h
    - Library file: libsstaralgo_det.a / libsstaralgo_det.so

### 2.4. ALGO_DET_SetStableBox

-   Function

    Switch to set post-processing stable box algorithm

-   Syntax


        MI_S32 ALGO_DET_SetStableBox(void *handle, bool stable);


-   Parameter

    | Parameter Name | Description                                                                          | Input/Output |
    | -------------- | ------------------------------------------------------------------------------------ | ------------ |
    | handle         | Handle                                                                               | Input        |
    | stable         | Stable box algorithm switch (set false to disable, true to enable; default is disable) | Input        |

-   Return Value

    | Return Value | Description                                     |
    | ------------ | ----------------------------------------------- |
    | 0            | Successful.                                          |
    | Other        | Failed, see [Error Code](#errorcode) for details  |

-   Dependency

    - Header file: sstar_det_api.h
    - Library file: libsstaralgo_det.a / libsstaralgo_det.so

### 2.5. ALGO_DET_GetInputAttr

-   Function

    Get model attribute information, including model input resolution and input data type

-   Syntax

        MI_S32 ALGO_DET_GetInputAttr(void *handle, InputAttr_t *input_attr);

-   Parameter

    | Parameter Name | Description                                                                                | Input/Output |
    | -------------- | ------------------------------------------------------------------------------------------ | ------------ |
    | handle         | Handle                                                                                     | Input        |
    | input_attr     | Pointer to the saved attribute information, see [InputAttr_t](#32-inputattr_t) for details | Input        |

-   Return Value

    | Return Value | Description                                     |
    | ------------ | ----------------------------------------------- |
    | 0            | Successful.                                          |
    | Other        | Failed, see [Error Code](#errorcode) for details  |

-   Dependency

    - Header file: sstar_det_api.h
    - Library file: libsstaralgo_det.a / libsstaralgo_det.so

### 2.6. ALGO_DET_SetThreshold

-   Function

    Set detection threshold

-   Syntax

        MI_S32 ALGO_DET_SetThreshold(void *handle, MI_FLOAT threshold);

-   Parameter

    | Parameter Name | Description | Input/Output |
    | -------------- | ----------- | ------------ |
    | handle         | Handle      | Input        |
    | threshold      | Threshold   | Input        |

-   Return Value

    | Return Value | Description                                     |
    | ------------ | ----------------------------------------------- |
    | 0            | Successful.                                          |
    | Other        | Failed, see [Error Code](#errorcode) for details  |

-   Dependency

    - Header file: sstar_det_api.h
    - Library file: libsstaralgo_det.a / libsstaralgo_det.so


### 2.7. ALGO_DET_Run

-   Function

    Run detection algorithm

-   Syntax

        MI_S32 ALGO_DET_Run(void *handle, const ALGO_Input_t *algo_input, Box_t bboxes[MAX_DET_OBJECT], MI_S32 *num_bboxes);

-   Parameter

    | Parameter Name | Description                                           | Input/Output |
    | -------------- | ----------------------------------------------------- | ------------ |
    | handle         | Handle                                                | Input        |
    | algo_input     | Input image buffer information                        | Input        |
    | bboxes         | Used to save the array of detection result boxes      | Input        |
    | num_bboxes     | Pointer to the saved number of detection result boxes | Input        |

-   Return Value

    | Return Value | Description                                     |
    | ------------ | ----------------------------------------------- |
    | 0            | Successful.                                          |
    | Other        | Failed, see [Error Code](#errorcode) for details  |

-   Dependency

    - Header file: sstar_det_api.h
    - Library file: libsstaralgo_det.a / libsstaralgo_det.so

### 2.8. ALGO_DET_DeinitHandle

-   Function

    Deinitialize detection algorithm

-   Syntax

        MI_S32 ALGO_DET_DeinitHandle(void *handle);

-   Parameter

    | Parameter Name | Description | Input/Output |
    | -------------- | ----------- | ------------ |
    | handle         | Handle      | Input        |

-   Return Value

    | Return Value | Description                                     |
    | ------------ | ----------------------------------------------- |
    | 0            | Successful.                                          |
    | Other        | Failed, see [Error Code](#errorcode) for details  |

-   Dependency

    - Header file: sstar_det_api.h
    - Library file: libsstaralgo_det.a / libsstaralgo_det.so

### 2.9. ALGO_DET_ReleaseHandle

-   Function

    Release the resources occupied by the handle

-   Syntax

        MI_S32 ALGO_DET_ReleaseHandle(void *handle);

-   Parameter

    | Parameter Name | Description | Input/Output |
    | -------------- | ----------- | ------------ |
    | handle         | Handle      | Input        |

-   Return Value

    | Return Value | Description                                     |
    | ------------ | ----------------------------------------------- |
    | 0            | Successful.                                          |
    | Other        | Failed, see [Error Code](#errorcode) for details  |

-   Dependency

    - Header file: sstar_det_api.h
    - Library file: libsstaralgo_det.a / libsstaralgo_det.so


### 2.10. ALGO_DET_SetTracker2

- Function

    Set parameters of post-processing tracker algorithm

- Syntax

        MI_S32 ALGO_DET_SetTracker2(void *handle, DetTrackParams_t track_params);

- Parameter

    | Parameter Name     | Description           | Input/Output |
    | ------------ | -------------- | --------- |
    | handle       | Handle           | Input      |
    | track_params | track parameters structure | Input      |

- Return Value

    | Return Value | Description                            |
    | ------ | ------------------------------- |
    | 0      | Successful.                            |
    | Other   | Failed, see [Error Code](#errorcode) for details  |

- Dependency

    - Header file: sstar_det_api.h
    - Library file: libsstaralgo_det.a / libsstaralgo_det.so


### 2.11. ALGO_DET_SetStrictMode

- Function

    Set parameters of strict detection mode 

- Syntax

        MI_S32 ALGO_DET_SetStrictMode(void *handle, DetStrictModeParams_t params);

- Parameter

    | Parameter Name     | Description           | Input/Output |
    | ------------ | -------------- | --------- |
    | handle       | Handle           | Input      |
    | params |parameters of strict detection mode, see[DetStrictModeParams_t](#37-detstrictmodeparams_t) for details | Input      |

- Return Value

    | Return Value | Description                            |
    | ------ | ------------------------------- |
    | 0      | Successful.                            |
    | Other   | Failed, see [Error Code](#errorcode) for details  |

- Dependency

    - Header file: sstar_det_api.h
    - Library file: libsstaralgo_det.a / libsstaralgo_det.so


### 2.12. ALGO_DET_InitHandle2

- Function

    Initialize the algorithm using the model memory block pointer

- Syntax

        MI_S32 ALGO_DET_InitHandle2(void *handle, DetectionInfo2_t *init_info);


- Parameter

    | Parameter Name  | Description                                                      | Input/Output |
    | --------- | --------------------------------------------------------- | --------- |
    | handle    | Handle                                                      | Input      |
    | init_info | detection algorithm configuration items, see [DetectionInfo2_t](#38-detectioninfo2_t) for details | Input      |

- Return Value

    | Return Value | Description                            |
    | ------ | ------------------------------- |
    | 0      | Successful.                            |
    | Other   | Failed, see [Error Code](#errorcode) for details  |

- Dependency

    - Header file: sstar_det_api.h
    - Library file: libsstaralgo_det.a / libsstaralgo_det.so


### 2.13. ALGO_DET_SortResult

- Function

    Sort detection results

- Syntax

        MI_S32 ALGO_DET_SortResult(void *handle, const Sort_Input_t *sort_input, Box_t bboxes[MAX_DET_OBJECT], MI_S32 *num_bboxes);


- Parameter

    | Parameter Name   | Description                         | Input/Output |
    | ---------- | ---------------------------- | --------- |
    | handle     | Handle                         | Input      |
    | sort_input | sort input structure         | Input      |
    | bboxes         | Used to save the array of sorted result boxes      | Input/Output        |
    | num_bboxes     | Pointer to the saved number of sorted result boxes | Input/Output        |

- Return Value

    | Return Value | Description                            |
    | ------ | ------------------------------- |
    | 0      | Successful. NOTE：this api will change bboxes and num_bboxes  |
    | Other   | Failed, see [Error Code](#errorcode) for details  |

- Dependency

    - Header file: sstar_det_api.h
    - Library file: libsstaralgo_det.a / libsstaralgo_det.so



## 3. Structure/Enumeration Type

The detection related data types are defined as follows:

| Data Type                                  | Definition                                                                             |
| ------------------------------------------ | -------------------------------------------------------------------------------------- |
| [Rect_t](#31-rect_t)                       | Define the resolution size                                                              |
| [InputAttr_t](#32-inputattr_t)             | Algorithm input structure                                                              |
| [Box_t](#33-box_t)                         | Algorithm output structure                                                             |
| [DetectionInfo_t](#34-detectioninfo_t)     | Algorithm parameter related structure                                                  |
| [ALGO_Input_t](#35-algo_input_t)           | Algorithm input image data information                                                 |
| [DetTrackParams_t](#36-dettrackparams_t) | Detection algorithm tracking configuration items |
| [DetStrictModeParams_t](#37-detstrictmodeparams_t) | Strict detection mode related structure |
| [DetectionInfo2_t](#38-detectioninfo2_t) | Structure that uses a model memory block pointer to initialize the algorithm    |
| [Sort_Input_t](#39-sort_input_t)       | Sort input related structure |

### 3.1 Rect_t

- Description

    Define the resolution size

- Definition

        typedef struct{
            MI_U32 width;
            MI_U32 height;
        }Rect_t;


- Member

   | Member Name | Description             |
   | ----------- | ----------------------- |
   | width       | Model input data width  |
   | height      | Model input data height |

- Related data type and interface

    [DetectionInfo_t](#34-detectioninfo_t)

    [ALGO_DET_InitHandle](#22-algo_det_inithandle)

### 3.2 InputAttr_t

- Description

    Define the resolution size and model type

- Definition

        typedef struct{
            MI_U32 width;
            MI_U32 height;
            MI_IPU_ELEMENT_FORMAT format;
        }InputAttr_t;


- Member

    | Member Name | Description             |
    | ----------- | ----------------------- |
    | width       | Model input data width  |
    | height      | Model input data height |
    | format      | Model input data format |

- Related data type and interface

    [ALGO_DET_GetInputAttr](#25-algo_det_getinputattr)

### 3.3 Box_t

- Description

    Algorithm output structure

- Definition

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

- Member

    | Member Name         | Description                               |
    | ------------------- | ----------------------------------------- |
    | x, y, width, height | Detection result box position             |
    | class_id            | Detection result box class ID             |
    | score               | Detection result box score                |
    | pts                 | Timestamp of the incoming detection frame |

- Related data type and interface

    [ALGO_DET_Run](#27-algo_det_run)

### 3.4 DetectionInfo_t

- Description

    Detection algorithm related configuration option

- Definition

        typedef struct
        {
            char ipu_firmware_path[MAX_DET_STRLEN]; // ipu_firmware.bin path
            char model[MAX_DET_STRLEN];             // detect model path
            MI_FLOAT threshold;                     // confidence
            Rect_t disp_size;
        } DetectionInfo_t;

- Member

    | Member Name                       | Description                                                                   |
    | --------------------------------- | ----------------------------------------------------------------------------- |
    | ipu_firmware_path[MAX_DET_STRLEN] | ipu_firmware_path                                                             |
    | Model[MAX_DET_STRLEN]             | Model folder path                                                             |
    | threshold                         | Detection threshold (0~1)                                                     |
    | disp_size                         | Display the resolution of the code stream (to map the detection box position) |

- Related data type and interface

    [ALGO_DET_InitHandle](#22-algo_det_inithandle)

### 3.5 ALGO_Input_t

- Description

    Detection algorithm related configuration option

- Definition

        typedef struct
        {
            void *p_vir_addr;
            MI_PHY phy_addr;
            MI_U32 buf_size;
            MI_U64 pts;
        } ALGO_Input_t;

- Member

    | Member Name | Description                   |
    | ----------- | ----------------------------- |
    | p_vir_addr  | Input buffer virtual address  |
    | phy_addr    | Input buffer physical address |
    | buf_size    | Input buffer size             |
    | pts         | Input buffer timestamp        |

- Related data type and interface

    [ALGO_DET_Run](#27-algo_det_run)


### 3.6 DetTrackParams_t

-   Description

    Detection algorithm tracking configuration items

-   Definition

        typedef struct
        {
            MI_BOOL enable; // false
            MI_BOOL ignore_static_objects;   //false
            MI_FLOAT static_sensitive;    //0.85
            MI_BOOL stable_bbox;     //false
            MI_FLOAT stable_sensitive;    //0.63
            MI_S32 ignore_frame_number; //[0-5] ignore the id first number box of detect,default=0
        } DetTrackParams_t;

-   Member

    | Member Name              | Description                                                                              |
    | --------------------- | --------------------------------------------------------------------------------- |
    | enable                | Whether to enable the tracking function. The default value is false, which means it is not enabled.                                         |
    | ignore_static_objects | Whether to filter static objects, default is false, i.e. do not filter static objects                                                       |
    | static_sensitive      | Static detecton threshold(0.0~1.0), default is 0.85. Larger threshold will keep boxes exhibiting minor movement |
    | stable_bbox           | Whether to enable stable box, default is false, i.e. disable stable box                                              |
    | stable_sensitive      | Stable box threshold (0.0~1.0), default is 0.63. Smaller threshold will get better stable boxes                 |
    | ignore_frame_number   | Filter the first frames, using to filter occasional false alarm, (0~5), default is 0, i.e. do not filter the first frames                  |


-   Related data type and interface

    [ALGO_DET_SetTracker2](#210-algo_det_settracker2)


### 3.7 DetStrictModeParams_t

-   Description

    Strict detection mode related structure

-   Definition

        typedef struct
        {
            MI_BOOL enable; // false
        }
        DetStrictModeParams_t;

-   Member

    | Member Name              | Description                                                                              |
    | --------------------- | --------------------------------------------------------------------------------- |
    | enable                | Whether to run algorithm in strict mode, which can reduce false alarm when enable, default is false             |


-   Related data type and interface

    [ALGO_DET_SetStrictMode](#211-algo_det_setstrictmode)

### 3.8 DetectionInfo2_t

-   Description

    Structure that uses a model memory block pointer to initialize the algorithm

-   Definition

        typedef struct
        {
            void *model_buffer;
            MI_U32 model_buffer_len;
            MI_FLOAT threshold;                     // confidence
            Rect_t disp_size;
            MI_BOOL had_create_device; // set true to handle ipu device outside algo lib
        } DetectionInfo2_t;

-   Member

    | Member Name                          | Description                                                                                                              |
    | --------------------------------- | ----------------------------------------------------------------------------------------------------------------- |
    | model_buffer | Pointer to the model memory block                                                                                           |
    | model_buffer_len  | Points to the model memory block length                                                                                                    |
    | threshold                         | Detection threshold(0~1)                                                                                                   |
    | disp_size                         | Display the resolution of the code stream (to map the detection box position)                                                                            |
    | had_create_device                 | Set whether to create or destroy IPU_Device in the outer layer. The default value is false. When using multiple algorithms at the same time, it can be set to true to avoid repeated creation or destruction of IPU_Device |

-   Related data type and interface

    [ALGO_DET_InitHandle2](#212-algo_det_inithandle2)


### 3.9 Sort_Input_t

-   Description

    Sort input related structure

-   Definition

        typedef struct
        {
            MI_S32 class_indexs[MAX_DET_CLASSES];
            MI_S32 class_num;
            SORT_TYPE_e sort_type;
        }Sort_Input_t;

-   Member

    | Member Name   | Description                 |
    | ---------- | -------------------- |
    | class_indexs | Index of the classes to be sorted |
    | class_num   | The number of classes to sort |
    | sort_type   | Sorting method type    |

-   Related data type and interface

    [ALGO_DET_SortResult](#213-algo_det_run)


## 4. Enumeration Type

The detection related data types are defined as follows:

| Data Type                                   | Definition                                     |
| ------------------------------------------ | ---------------------------------------- |
| [Label_FD_Face_e](#41-label_fd_face_e)     | Correspondence between model type and class_id of face detection     |
| [Label_FSD_e](#42-label_fsd_e)             | Correspondence between model type and class_id of fire and smoke detection |
| [Label_SD_e](#43-label_sd_e)               | Correspondence between model type and class_id of person + vehicle + pet + head + face detection |
| [Label_SPD_e](#44-label_spd_e)              | Correspondence between model type and class_id of person + head + face detection |
| [Label_BD_e](#45-label_bd_e)              | Correspondence between model type and class_id of bag detection |


### 4.1 Label_FD_Face_e

- Description

    Correspondence between model type and class_id of face detection

- Definition

        typedef enum
        {
            E_FD_FACE = 0,
        }Label_FD_Face_e;


- Member

    | Member Name  | Description                 |
    | --------- | -------------------- |
    | E_FD_FACE | Face(class_id=0) |

- Related data type and interface

    [Box_t](#33-box_t)

### 4.2 Label_FSD_e

- Description

    Correspondence between model type and class_id of fire and smoke detection

- Definition

        typedef enum
        {
            E_FSD_FIRE,
            E_FSD_SMOKE
        }Label_FSD_e;

- Member

   | Member Name     | Description                 |
   | ----------- | -------------------- |
   | E_FSD_FIRE  | Fire(class_id=0) |
   | E_FSD_SMOKE | Smoke(class_id=1) |

- Related data type and interface

    [Box_t](#33-box_t)


### 4.3 Label_SD_e

- Description

    Correspondence between model type and class_id of  person + vehicle + pet + head + face detection

- Definition

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
            E_SD_FACE,
        }LABEL_SD_e;

- Member

    | Member Name         | Description                   |
    | --------------- | ---------------------- |
    | E_SD_PERSON    | Person(class_id=0)   |
    | E_SD_BICYCLE   | Bicycle(class_id=1) |
    | E_SD_CAR       | Car (class_id=2)   |
    | E_SD_MOTOCYCLE | Motocyle(class_id=3) |
    | E_SD_BUS       | Bus(class_id=4) |
    | E_SD_TRUCK     | Truck(class_id=5)   |
    | E_SD_CAT       | Cat(class_id=6)   |
    | E_SD_DOG       | Dog(class_id=7)   |
    | E_SD_HEAD      | Head(class_id=8)   |
    | E_SD_FACE      | Face(class_id=9)   |

- Related data type and interface

    [Box_t](#33-box_t)


### 4.4 Label_SPD_e

- Description

    Correspondence between model type and class_id of person + head + face detection

- Definition

        typedef enum
        {
            E_SPD_PERSON = 0,
            E_SPD_HEAD,
            E_SPD_FACE
        }LABEL_SPD_e;

- Member

    | Member Name        | Description                   |
    | --------------- | ---------------------- |
    | E_SPD_PERSON    | Person(class_id=0)   |
    | E_SPD_HEAD      | Head(class_id=1)   |
    | E_SPD_FACE      | Face(class_id=2)   |


- Related data type and interface

    [Box_t](#33-box_t)

### 4.5 Label_BD_e

- Description

    Correspondence between model type and class_id of bag detection

- Definition

        typedef enum
        {
            E_BD_BAG = 0,
        }LABEL_BD_e;

- Member

    | Member Name         | Description                   |
    | --------------- | ---------------------- |
    | E_BD_BAG        | Bag(class_id=0)   |


- Related data type and interface

    [Box_t](#33-box_t)

## <span id=errorcode>5. Error code </span>

| Error Code                 | Value | Description                             |
| -------------------------- | ----- | --------------------------------------- |
| E_ALGO_SUCCESS             | 0     | Operation successful                    |
| E_ALGO_HANDLE_NULL         | 1     | Algorithm handle is null                |
| E_ALGO_INVALID_PARAM       | 2     | Invalid input parameter                 |
| E_ALGO_DEVICE_FAULT        | 3     | Hardware error                          |
| E_ALGO_LOADMODEL_FAIL      | 4     | Failed to load model                    |
| E_ALGO_INIT_FAIL           | 5     | Algorithm initialization failed         |
| E_ALGO_NOT_INIT            | 6     | Algorithm not initialized               |
| E_ALGO_INPUT_DATA_NULL     | 7     | Algorithm input data is null            |
| E_ALGO_INVALID_INPUT_SIZE  | 8     | Invalid algorithm input data dimension  |
| E_ALGO_INVALID_LICENSE     | 9     | Invalid license                         |
| E_ALGO_MEMORY_OUT          | 10    | Out of memory                           |
| E_ALGO_FILEIO_ERROR        | 11    | File read and write operation error     |
| E_ALGO_INVALID_OUTPUT_SIZE | 12    | Invalid algorithm output data dimension |
| E_ALGO_INVALID_DECODE_MODE | 13   | Invalid decode mode      |
| E_ALGO_MODEL_INVOKE_ERROR  | 14   | Invoke fail |

