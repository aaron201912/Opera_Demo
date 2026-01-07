# 检测算法

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

## 1. 概述

### 1.1. 算法说明

主要检测算法的检测类别说明如下：

- 人脸检测(SFD/SYFL)输出的类别共1类，分别人脸(class_id=0)，其中SYFL为带关键点的人脸检测模型;

- 火焰烟雾(SFSD)输出的类别共2类，分别为火焰(class_id=0)、烟雾(class_id=1);

- 人+车+宠物+人头+人脸检测(SD)输出的类别共10类，分别为行人(class_id=0)、自行车(class_id=1)、轿车(class_id=2) 、摩托车(class_id=3)、公交车(class_id=4)、卡车(class_id=5)、猫(class_id=6)、狗(class_id=7)、人头(class_id=8)、人脸(class_id=9);

- 人+人头+人脸检测(SPD)输出的类别共3类，分别为行人(class_id=0)、人头(class_id=1)、人脸(class_id=2);

- 包裹检测(SBD)输出的类别共1类，分别为包裹(class_id=0);

- 牛羊检测（COWSHEEP）输出的类别共2类，分别为牛(class_id=0)、羊(class_id=1)。

- 人+车+宠物+人头人脸+牛羊检测（S12D）输出的类别共12类，分别为行人(class_id=0)、自行车(class_id=1)、轿车(class_id=2) 、摩托车(class_id=3)、公交车(class_id=4)、卡车(class_id=5)、猫(class_id=6)、狗(class_id=7)、人头(class_id=8)、人脸(class_id=9)、牛(class_id=10)、羊(class_id=11)。

- 做作业/玩手机检测（HOMEWORK）输出的类别共3类，分别为读书(class_id=0)、写字(class_id=1)、玩手机(class_id=2)。

- 模型命名中的y表示YUV输入; 数字表示分辨率，具体分辨率可以通过[ALGO_DET_GetInputAttr](#25-algo_det_getinputattr)获取;


## 2. API参考

该功能模块提供以下API:

| API名称                                              | 功能                         |
| ---------------------------------------------------- | ---------------------------- |
| [ALGO_DET_CreateHandle](#21-algo_det_createhandle)   | 创建句柄                     |
| [ALGO_DET_InitHandle](#22-algo_det_inithandle)       | 初始化句柄                   |
| [ALGO_DET_SetTracker](#23-algo_det_settracker)       | 设置后处理跟踪算法开关       |
| [ALGO_DET_SetStableBox](#24-algo_det_setstablebox)   | 设置后处理稳框的开关         |
| [ALGO_DET_GetInputAttr](#25-algo_det_getinputattr)   | 获取模型的属性信息           |
| [ALGO_DET_SetThreshold](#26-algo_det_setthreshold)   | 设置阈值                     |
| [ALGO_DET_Run](#27-algo_det_run)                     | 运行检测算法                 |
| [ALGO_DET_DeinitHandle](#28-algo_det_deinithandle)   | 停止检测算法                 |
| [ALGO_DET_ReleaseHandle](#29-algo_det_releasehandle) | 释放句柄                     |
| [ALGO_DET_SetTracker2](#210-algo_det_settracker2)    | 设置后处理跟踪算法 |
| [ALGO_DET_SetStrictMode](#211-algo_det_setstrictmode)    | 设置严格检测模式参数 |
| [ALGO_DET_InitHandle2](#212-algo_det_inithandle2)       | 使用模型内存块指针进行算法初始化  |
| [ALGO_DET_SortResult](#213-algo_det_sortresult)       | 对检测结果进行排序  |

### 2.1. ALGO_DET_CreateHandle

- 功能

    创建句柄

- 语法

        MI_S32 ALGO_DET_CreateHandle(void **handle);

- 形参

    | 参数名称 | 描述 | 输入/输出 |
    | -------- | ---- | --------- |
    | handle   | 句柄 | 输入      |

- 返回值

    | 返回值 | 描述                            |
    | ------ | ------------------------------- |
    | 0      | 成功                            |
    | 其它   | 失败(详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so


### 2.2. ALGO_DET_InitHandle

- 功能

    初始化句柄

- 语法

        MI_S32 ALGO_DET_InitHandle(void *handle, DetectionInfo_t *init_info);

- 形参

    | 参数名称  | 描述                                                      | 输入/输出 |
    | --------- | --------------------------------------------------------- | --------- |
    | handle    | 句柄                                                      | 输入      |
    | init_info | 检测算法配置项,详见[DetectionInfo_t](#34-detectioninfo_t) | 输入      |

- 返回值

    | 返回值 | 描述                            |
    | ------ | ------------------------------- |
    | 0      | 成功                            |
    | 其它   | 失败(详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so


### 2.3. ALGO_DET_SetTracker

- 功能

    设置后处理跟踪算法参数

- 语法

        MI_S32 ALGO_DET_SetTracker(void *handle, MI_S32 tk_type, MI_S32 md_type);

- 形参

    | 参数名称 | 描述                                                                              | 输入/输出 |
    | -------- | --------------------------------------------------------------------------------- | --------- |
    | handle   | 句柄                                                                              | 输入      |
    | tk_type  | Tracker算法开关(设置0为关闭，1为开启，默认为关闭)                               | 输入      |
    | md_type  | 运动目标检测开关(设置为0，运动/静止目标均检测，设置为1，仅检测运动目标， 默认为0) | 输入      |

- 返回值

    | 返回值 | 描述                            |
    | ------ | ------------------------------- |
    | 0      | 成功                            |
    | 其它   | 失败(详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so

### 2.4. ALGO_DET_SetStableBox

- 功能

    设置后处理稳框算法开关

- 语法

        MI_S32 ALGO_DET_SetStableBox(void *handle, bool stable);

- 形参

    | 参数名称 | 描述                                                    | 输入/输出 |
    | -------- | ------------------------------------------------------- | --------- |
    | handle   | 句柄                                                    | 输入      |
    | stable   | 稳框算法开关(设置false为关闭，true为开启，默认为关闭) | 输入      |

- 返回值

    | 返回值 | 描述                            |
    | ------ | ------------------------------- |
    | 0      | 成功                            |
    | 其它   | 失败(详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so


### 2.5. ALGO_DET_GetInputAttr

- 功能

    获取模型的属性信息，包括模型输入分辨率以及输入数据的类型

- 语法

        MI_S32 ALGO_DET_GetInputAttr(void *handle, InputAttr_t *input_attr);

- 形参

    | 参数名称   | 描述                                                 | 输入/输出 |
    | ---------- | ---------------------------------------------------- | --------- |
    | handle     | 句柄                                                 | 输入      |
    | input_attr | 保存属性信息指针，详见[InputAttr_t](#32-inputattr_t) | 输入      |

- 返回值

    | 返回值 | 描述                            |
    | ------ | ------------------------------- |
    | 0      | 成功                            |
    | 其它   | 失败(详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h

    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so

### 2.6. ALGO_DET_SetThreshold

- 功能

    设置检测阈值

- 语法

        MI_S32 ALGO_DET_SetThreshold(void *handle, MI_FLOAT threshold);

- 形参

    | 参数名称  | 描述 | 输入/输出 |
    | --------- | ---- | --------- |
    | handle    | 句柄 | 输入      |
    | threshold | 阈值 | 输入      |

- 返回值

    | 返回值 | 描述                            |
    | ------ | ------------------------------- |
    | 0      | 成功                            |
    | 其它   | 失败(详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so


### 2.7. ALGO_DET_Run

- 功能

    运行检测算法

- 语法

        MI_S32 ALGO_DET_Run(void *handle, const ALGO_Input_t *algo_input, Box_t bboxes[MAX_DET_OBJECT], MI_S32 *num_bboxes);

- 形参

    | 参数名称   | 描述                         | 输入/输出 |
    | ---------- | ---------------------------- | --------- |
    | handle     | 句柄                         | 输入      |
    | algo_input | 输入图像的buffer信息         | 输入      |
    | bboxes     | 用于保存检测结果框的数组     | 输入/输出    |
    | num_bboxes | 用于保存检测结果框个数的指针 | 输入/输出      |

- 返回值

    | 返回值 | 描述                            |
    | ------ | ------------------------------- |
    | 0      | 成功                            |
    | 其它   | 失败(详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so


### 2.8. ALGO_DET_DeinitHandle

- 功能

    停止检测算法

- 语法

        MI_S32 ALGO_DET_DeinitHandle(void *handle);

- 形参

    | 参数名称 | 描述 | 输入/输出 |
    | -------- | ---- | --------- |
    | handle   | 句柄 | 输入      |

- 返回值

    | 返回值 | 描述                            |
    | ------ | ------------------------------- |
    | 0      | 成功                            |
    | 其它   | 失败(详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so


### 2.9. ALGO_DET_ReleaseHandle

- 功能

    释放句柄占用资源

- 语法

        MI_S32 ALGO_DET_ReleaseHandle(void *handle);

- 形参

    | 参数名称 | 描述 | 输入/输出 |
    | -------- | ---- | --------- |
    | handle   | 句柄 | 输入      |

- 返回值

    | 返回值 | 描述                            |
    | ------ | ------------------------------- |
    | 0      | 成功                            |
    | 其它   | 失败(详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so


### 2.10. ALGO_DET_SetTracker2

- 功能

    设置后处理跟踪相关参数

- 语法

        MI_S32 ALGO_DET_SetTracker2(void *handle, DetTrackParams_t track_params);

- 形参

    | 参数名称     | 描述           | 输入/输出 |
    | ------------ | -------------- | --------- |
    | handle       | 句柄           | 输入      |
    | track_params | 跟踪参数结构体 | 输入      |

- 返回值

    | 返回值 | 描述                            |
    | ------ | ------------------------------- |
    | 0      | 成功                            |
    | 其它   | 失败(详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so


### 2.11. ALGO_DET_SetStrictMode

- 功能

    设置strict检测模式参数

- 语法

        MI_S32 ALGO_DET_SetStrictMode(void *handle, DetStrictModeParams_t params);

- 形参

    | 参数名称     | 描述           | 输入/输出 |
    | ------------ | -------------- | --------- |
    | handle       | 句柄           | 输入      |
    | params |strict检测模式参数结构体，(详见[DetStrictModeParams_t](#37-detstrictmodeparams_t)) | 输入      |

- 返回值

    | 返回值 | 描述                            |
    | ------ | ------------------------------- |
    | 0      | 成功                            |
    | 其它   | 失败(详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so


### 2.12. ALGO_DET_InitHandle2

- 功能

    使用模型内存块指针进行算法初始化

- 语法

        MI_S32 ALGO_DET_InitHandle2(void *handle, DetectionInfo2_t *init_info);


- 形参

    | 参数名称  | 描述                                                      | 输入/输出 |
    | --------- | --------------------------------------------------------- | --------- |
    | handle    | 句柄                                                      | 输入      |
    | init_info | 检测算法配置项,详见[DetectionInfo2_t](#38-detectioninfo2_t) | 输入      |

- 返回值

    | 返回值 | 描述                            |
    | ------ | ------------------------------- |
    | 0      | 成功                            |
    | 其它   | 失败(详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so


### 2.13. ALGO_DET_SortResult

- 功能

    对检测结果进行排序

- 语法

        MI_S32 ALGO_DET_SortResult(void *handle, const Sort_Input_t *sort_input, Box_t bboxes[MAX_DET_OBJECT], MI_S32 *num_bboxes);


- 形参

    | 参数名称   | 描述                         | 输入/输出 |
    | ---------- | ---------------------------- | --------- |
    | handle     | 句柄                         | 输入      |
    | sort_input | 排序输入结构体         | 输入      |
    | bboxes     | 用于保存排序结果框的数组     | 输入      |
    | num_bboxes | 用于保存排序结果框个数的指针 | 输入      |

- 返回值

    | 返回值 | 描述                            |
    | ------ | ------------------------------- |
    | 0      | 成功。注意：该接口会修改输入bboxes 和 num_bboxes |
    | 其它   | 失败(详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so



## 3. 结构体说明

检测相关结构体定义如下：

| 数据类型                               | 定义                 |
| -------------------------------------- | -------------------- |
| [Rect_t](#31-rect_t)                   | 定义分辨率大小       |
| [InputAttr_t](#32-inputattr_t)         | 算法输入结构体       |
| [Box_t](#33-box_t)                     | 算法输出结构体       |
| [DetectionInfo_t](#34-detectioninfo_t) | 算法参数相关结构体   |
| [ALGO_Input_t](#35-algo_input_t)       | 算法输入图像数据信息 |
| [DetTrackParams_t](#36-dettrackparams_t) | 跟踪算法参数结构体 |
| [DetStrictModeParams_t](#37-detstrictmodeparams_t) | strict检测模式参数结构体 |
| [DetectionInfo2_t](#38-detectioninfo2_t) | 使用模型内存块进行初始化的算法参数结构体   |
| [Sort_Input_t](#39-sort_input_t)       | 结果排序输入结构体 |

### 3.1 Rect_t

-   说明

    定义分辨率大小

-   定义

        typedef struct{
            MI_U32 width;
            MI_U32 height;
        }Rect_t;


-   成员

    | 成员名称 | 描述             |
    | -------- | ---------------- |
    | width    | 模型输入数据的宽 |
    | height   | 模型输入数据的高 |

-   相关数据类型及接口

    [DetectionInfo_t](#34-detectioninfo_t)

    [ALGO_DET_InitHandle](#22-algo_det_inithandle)

### 3.2 InputAttr_t

-   说明

    定义分辨率大小和模型类型

-   定义

        typedef struct{
            MI_U32 width;
            MI_U32 height;
            MI_IPU_ELEMENT_FORMAT format;
        }InputAttr_t;


-   成员

    | 成员名称 | 描述               |
    | -------- | ------------------ |
    | width    | 模型输入数据的宽   |
    | height   | 模型输入数据的高   |
    | format   | 模型输入数据的类型 |

-   相关数据类型及接口

    [ALGO_DET_GetInputAttr](#25-algo_det_getinputattr)

### 3.3 Box_t

-   说明

    算法输出结构体

-   定义

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

-   成员

    | 成员名称            | 描述               |
    | ------------------- | ------------------ |
    | x，y, width, height | 检测结果框的位置   |
    | class_id            | 检测结果框的类别ID |
    | score               | 检测结果框的分值   |
    | pts                 | 送入检测帧的时间戳 |

-   相关数据类型及接口

    [ALGO_DET_Run](#27-algo_det_run)

### 3.4 DetectionInfo_t

-   说明

    检测算法相关配置项

-   定义

        typedef struct
        {
            char ipu_firmware_path[MAX_DET_STRLEN]; // ipu_firmware.bin path
            char model[MAX_DET_STRLEN];             // detect model path
            MI_FLOAT threshold;                     // confidence
            Rect_t disp_size;
            MI_BOOL had_create_device; // set true to handle ipu device outside algo lib
        } DetectionInfo_t;

-   成员

    | 成员名称                          | 描述                                                                                                              |
    | --------------------------------- | ----------------------------------------------------------------------------------------------------------------- |
    | ipu_firmware_path[MAX_DET_STRLEN] | ipu_firmware_path路径                                                                                             |
    | Model[MAX_DET_STRLEN]             | 模型文件夹路径                                                                                                    |
    | threshold                         | 检测阈值(0~1)                                                                                                   |
    | disp_size                         | 显示码流的分辨率(用于映射检测框位置)                                                                            |
    | had_create_device                 | 设置是否在外层创建和销毁IPU_Device，默认为false，当同时使用多个算法时，可设置为true以避免重复创建或销毁IPU_Device |

-   相关数据类型及接口

    [ALGO_DET_InitHandle](#22-algo_det_inithandle)

### 3.5 ALGO_Input_t

-   说明

    检测算法相关配置项

-   定义

        typedef struct
        {
            void *p_vir_addr;
            MI_PHY phy_addr;
            MI_U32 buf_size;
            MI_U64 pts;
        } ALGO_Input_t;

-   成员

    | 成员名称   | 描述                 |
    | ---------- | -------------------- |
    | p_vir_addr | 输入buffer的虚拟地址 |
    | phy_addr   | 输入buffer的物理地址 |
    | buf_size   | 输入buffer的长度     |
    | pts        | 输入buffer时间戳     |

-   相关数据类型及接口

    [ALGO_DET_Run](#27-algo_det_run)

### 3.6 DetTrackParams_t

-   说明

    检测算法跟踪配置项

-   定义

        typedef struct
        {
            MI_BOOL enable; // false
            MI_BOOL ignore_static_objects;   //false
            MI_FLOAT static_sensitive;    //0.85
            MI_BOOL stable_bbox;     //false
            MI_FLOAT stable_sensitive;    //0.63
            MI_S32 ignore_frame_number; //[0-5] ignore the id first number box of detect,default=0
        } DetTrackParams_t;

-   成员

    | 成员名称              | 描述                                                                              |
    | --------------------- | --------------------------------------------------------------------------------- |
    | enable                | 是否启用跟踪功能，默认值为false，即不启用                                         |
    | ignore_static_objects | 忽略静态目标，默认值为false                                                       |
    | static_sensitive      | 判断物体运动的灵敏度，取值0.0~1.0，默认值为0.85，取值越大越倾向于保留小幅运动的框 |
    | stable_bbox           | 是否开启稳框，默认值为false，即不开启                                             |
    | stable_sensitive      | 稳框算法的灵敏度，取值0.0~1.0，默认值为0.85，取值越小稳框效果越强                 |
    | ignore_frame_number   | 设置忽略一个目标的检测到的前n帧，用于过滤偶尔闪现的误检，取值0~5                   |


-   相关数据类型及接口

    [ALGO_DET_SetTracker2](#210-algo_det_settracker2)


### 3.7 DetStrictModeParams_t

-   说明

    Strict检测模式参数结构体

-   定义

        typedef struct
        {
            MI_BOOL enable; // false
        }
        DetStrictModeParams_t;

-   成员

    | 成员名称              | 描述                                                                              |
    | --------------------- | --------------------------------------------------------------------------------- |
    | enable                | 是否启用strict检测模式，默认值为false，即不启用                                        |


-   相关数据类型及接口

    [ALGO_DET_SetStrictMode](#211-algo_det_setstrictmode)

### 3.8 DetectionInfo2_t

-   说明

    使用模型内存块进行初始化的算法参数结构体

-   定义

        typedef struct
        {
            void *model_buffer;
            MI_U32 model_buffer_len;
            MI_FLOAT threshold;                     // confidence
            Rect_t disp_size;
            MI_BOOL had_create_device; // set true to handle ipu device outside algo lib
        } DetectionInfo2_t;

-   成员

    | 成员名称                          | 描述                                                                                                              |
    | --------------------------------- | ----------------------------------------------------------------------------------------------------------------- |
    | model_buffer | 指向模型的内存块指针                                                                                            |
    | model_buffer_len  | 指向模型的内存块长度                                                                                                    |
    | threshold                         | 检测阈值(0~1)                                                                                                   |
    | disp_size                         | 显示码流的分辨率(用于映射检测框位置)                                                                            |
    | had_create_device                 | 设置是否在外层创建和销毁IPU_Device, 默认为false，当同时使用多个算法时，可设置为true以避免重复创建或销毁IPU_Device |

-   相关数据类型及接口

    [ALGO_DET_InitHandle2](#212-algo_det_inithandle2)


### 3.9 Sort_Input_t

-   说明

    检测算法相关配置项

-   定义

        typedef struct
        {
            MI_S32 class_indexs[MAX_DET_CLASSES];
            MI_S32 class_num;
            SORT_TYPE_e sort_type;
        }Sort_Input_t;

-   成员

    | 成员名称   | 描述                 |
    | ---------- | -------------------- |
    | class_indexs | 需要排序的类别索引 |
    | class_num   | 需要排序的类别数目 |
    | sort_type   | 排序方法类型    |

-   相关数据类型及接口

    [ALGO_DET_SortResult](#213-algo_det_run)


## 4. 枚举类型说明

检测枚举类型定义如下：

| 数据类型                                   | 定义                                     |
| ------------------------------------------ | ---------------------------------------- |
| [Label_FD_Face_e](#41-label_fd_face_e)     | 人脸检测算法class_id和类别名称的对应     |
| [Label_FSD_e](#42-label_fsd_e)             | 烟火检测算法class_id和类别名称的对应 |
| [Label_SD_e](#43-label_sd_e)               | 人+车+宠物+人头+人脸检测算法class_id和类别名称的对应 |
| [Label_SPD_e](#44-label_spd_e)              | 人+人头+人脸检测算法class_id和类别名称的对应 |
| [Label_BD_e](#45-label_bd_e)              | 包裹检测算法class_id和类别名称的对应 |
| [Label_S12D_e](#46-label_s12d_e)            | 人+车+宠物+人头/脸+牛羊检测算法class_id和类别名称的对应 |
| [Label_COWSHEEP_e](#47-label_cowsheep_e)    | 牛羊检测算法class_id和类别名称的对应 |
| [Label_HOMEWORK_e](#48-label_homework_e)    | 做作业/玩手机检测算法class_id和类别名称的对应 |

### 4.1 Label_FD_Face_e

- 说明

    人脸检测模型类别和class_id的对应关系

- 定义

        typedef enum
        {
            E_FD_FACE = 0,
        }Label_FD_Face_e;


- 成员

    | 成员名称  | 描述                 |
    | --------- | -------------------- |
    | E_FD_FACE | 人脸类别(class_id=0) |

- 相关数据类型及接口

    [Box_t](#33-box_t)


### 4.2 Label_FSD_e

- 说明

    定义火焰烟雾检测模型类别和class_id的对应关系

- 定义

        typedef enum
        {
            E_FSD_FIRE,
            E_FSD_SMOKE
        }Label_FSD_e;

- 成员

   | 成员名称    | 描述                 |
   | ----------- | -------------------- |
   | E_FSD_FIRE  | 火焰类别(class_id=0) |
   | E_FSD_SMOKE | 烟雾类别(class_id=1) |

- 相关数据类型及接口

    [Box_t](#33-box_t)


### 4.3 Label_SD_e

- 说明

    定义人+车+宠物+人头+人脸检测模型类别和class_id的对应关系

- 定义

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

- 成员

    | 成员名称        | 描述                   |
    | --------------- | ---------------------- |
    | E_SD_PERSON    | 行人类别(class_id=0)   |
    | E_SD_BICYCLE   | 自行车类别(class_id=1) |
    | E_SD_CAR       | 轿车类别(class_id=2)   |
    | E_SD_MOTOCYCLE | 摩托车类别(class_id=3) |
    | E_SD_BUS       | 公交车类别(class_id=4) |
    | E_SD_TRUCK     | 卡车类别(class_id=5)   |
    | E_SD_CAT       | 猫类别(class_id=6)   |
    | E_SD_DOG       | 狗类别(class_id=7)   |
    | E_SD_HEAD      | 人头类别(class_id=8)   |
    | E_SD_FACE      | 人脸类别(class_id=9)   |


- 相关数据类型及接口

    [Box_t](#33-box_t)

### 4.4 Label_SPD_e

- 说明

    定义人+人头+人脸检测模型类别和class_id的对应关系

- 定义

        typedef enum
        {
            E_SPD_PERSON = 0,
            E_SPD_HEAD,
            E_SPD_FACE
        }LABEL_SPD_e;

- 成员

    | 成员名称        | 描述                   |
    | --------------- | ---------------------- |
    | E_SPD_PERSON    | 行人类别(class_id=0)   |
    | E_SPD_HEAD      | 人头类别(class_id=1)   |
    | E_SPD_FACE      | 人脸类别(class_id=2)   |


- 相关数据类型及接口

    [Box_t](#33-box_t)

### 4.5 Label_BD_e

- 说明

    定义包裹检测模型类别和class_id的对应关系

- 定义

        typedef enum
        {
            E_BD_BAG = 0,
        }LABEL_BD_e;

- 成员

    | 成员名称        | 描述                   |
    | --------------- | ---------------------- |
    | E_BD_BAG        | 包裹类别(class_id=0)   |


- 相关数据类型及接口

    [Box_t](#33-box_t)

### 4.6 Label_S12D_e

- 说明

    定义人+车+宠物+人头/脸+牛羊检测模型类别和class_id的对应关系

- 定义

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


- 成员

    | 成员名称        | 描述                   |
    | --------------- | ---------------------- |
    | E_S12D_PERSON    | 行人类别(class_id=0)   |
    | E_S12D_BICYCLE   | 自行车类别(class_id=1) |
    | E_S12D_CAR       | 轿车类别(class_id=2)   |
    | E_S12D_MOTOCYCLE | 摩托车类别(class_id=3) |
    | E_S12D_BUS       | 公交车类别(class_id=4) |
    | E_S12D_TRUCK     | 卡车类别(class_id=5)   |
    | E_S12D_CAT       | 猫类别(class_id=6)   |
    | E_S12D_DOG       | 狗类别(class_id=7)   |
    | E_S12D_HEAD      | 人头类别(class_id=8)   |
    | E_S12D_FACE      | 人脸类别(class_id=9)   |
    | E_S12D_COW       | 牛类别(class_id=10)   |
    | E_S12D_SHEEP     | 羊类别(class_id=11)   |

- 相关数据类型及接口

    [Box_t](#33-box_t)


### 4.7 Label_COWSHEEP_e

- 说明

    定义牛羊检测模型类别和class_id的对应关系

- 定义

        typedef enum
        {
            E_COWSHEEP_COW = 0,
            E_COWSHEEP_SHEEP,
        } LABEL_COWSHEEP_e;

- 成员

    | 成员名称        | 描述                   |
    | --------------- | ---------------------- |
    | E_COWSHEEP_COW    | 牛类别(class_id=0)   |
    | E_COWSHEEP_SHEEP  | 羊类别(class_id=1)   |



- 相关数据类型及接口

    [Box_t](#33-box_t)


### 4.8 Label_HOMEWORK_e

- 说明

    定义做作业/玩手机类别和class_id的对应关系

- 定义

        typedef enum
        {
            E_HOMEWORK_READ= 0,
            E_HOMEWORK_WRITE,
            E_HOMEWORK_PHONE
        } LABEL_HOMEWORK_e;

- 成员

    | 成员名称        | 描述                   |
    | --------------- | ---------------------- |
    | E_HOMEWORK_READ  | 读书类别(class_id=0)   |
    | E_HOMEWORK_WRITE | 写字类别(class_id=1)   |
    | E_HOMEWORK_PHONE | 玩手机类别(class_id=2)   |

- 相关数据类型及接口

    [Box_t](#33-box_t)


## <span id=errorcode>5. 错误码 </span>

| 错误码                     | 数值 | 描述                   |
| -------------------------- | ---- | ---------------------- |
| E_ALGO_SUCCESS             | 0    | 操作成功               |
| E_ALGO_HANDLE_NULL         | 1    | 算法句柄为空           |
| E_ALGO_INVALID_PARAM       | 2    | 无效的输入参数         |
| E_ALGO_DEVICE_FAULT        | 3    | 硬件错误               |
| E_ALGO_LOADMODEL_FAIL      | 4    | 加载模型失败           |
| E_ALGO_INIT_FAIL           | 5    | 算法初始化失败         |
| E_ALGO_NOT_INIT            | 6    | 算法尚未初始化         |
| E_ALGO_INPUT_DATA_NULL     | 7    | 算法输入数据为空       |
| E_ALGO_INVALID_INPUT_SIZE  | 8    | 无效的算法输入数据维度 |
| E_ALGO_INVALID_LICENSE     | 9    | 无效的license许可      |
| E_ALGO_MEMORY_OUT          | 10   | 内存不足               |
| E_ALGO_FILEIO_ERROR        | 11   | 文件读写操作错误       |
| E_ALGO_INVALID_OUTPUT_SIZE | 12   | 无效的算法输出数据维度 |
| E_ALGO_INVALID_DECODE_MODE | 13   | 无效的解码模式       |
| E_ALGO_MODEL_INVOKE_ERROR  | 14   | 模型invoke错误 |
