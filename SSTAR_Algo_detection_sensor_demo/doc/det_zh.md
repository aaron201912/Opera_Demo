# Detection算法说明

---

## REVISION HISTORY

| Revision No. | Description   | Date       |
| ------------ | ------------- | ---------- |
| 1.0          | First version | 04/25/2023 |

## 1. 概述

### 1.1. 算法说明

检测算法包括：行人检测（SYPD），人非车检测（SYPCN）以及人宠物检测（SYPCD）三大类，具体的检测类别说明如下：

- 行人检测（SYPD）输出的类别共1类，为行人(class_id=0);

- 人非车检测（SYPCN）输出的类别共6类，分别为行人(class_id=0)、自行车(class_id=1)、轿车(class_id=2) 、摩托车(class_id=3)、公交车(class_id=4)、卡车(class_id=5) ；

- 人宠物检测（SYPCD）输出的类别共3类，分别为行人(class_id=0)、猫(class_id=1)、狗(class_id=2)；


### 1.2. 算法规格

- 行人检测（SYPD)

    - 板端资源(在377平台（IPU clock@800MHz）下测得)

        | Model Name | Model Version | Resolution | Input Format | Rom   | Ram    | Inference Time | PostProcess Time |
        | ---------- | ------------- | ---------- | ------------ | ----- | ------ | -------------- | ---------------- |
        | SYPD36     | 310           | 640x352    | yuv_nv12     | 772KB | 1780KB | 6.5ms          | 约1.0ms           |
        | SYPD48     | 310           | 800x480    | yuv_nv12     | 828KB | 2540KB | 10.9ms         | 约1.2ms           |
        | SYPD58     | 310           | 896x512    | yuv_nv12     | 848KB | 3048KB | 13.0ms         | 约1.5ms           |

- 人非车检测（SYPCN）

    - 板端资源(在377平台下测得)

        | Model Name | Model Version | Resolution | Input Format | Rom    | Ram    | Inference Time | PostProcess Time |
        | ---------- | ------------- | ---------- | ------------ | ------ | ------ | -------------- | ---------------- |
        | SYPCN36s   | 310           | 640x352    | yuv_nv12     | 880KB  | 1928KB | 6.6ms          | 约1.2ms           |
        | SYPCN48s   | 310           | 800x480    | yuv_nv12     | 936KB  | 2720KB | 11.1ms         | 约1.6ms           |
        | SYPCN58s   | 310           | 896x512    | yuv_nv12     | 960KB  | 3256KB | 13.3ms         | 约2.0ms           |
        | SYPCN36l   | 310           | 800x480    | yuv_nv12     | 3216KB | 5176KB | 15.7ms         | 约1.2ms           |
        | SYPCN48l   | 310           | 896x512    | yuv_nv12     | 3340KB | 6660KB | 26.4ms         | 约1.6ms           |

- 人宠物检测（SYPCD）

    - 板端资源(在377平台下测得)

        | Model Name | Model Version | Resolution | Input Format | Rom    | Ram    | Inference Time | PostProcess Time |
        | ---------- | ------------- | ---------- | ------------ | ------ | ------ | -------------- | ---------------- |
        | SYPCD36s   | 310           | 640x352    | yuv_nv12     | 772KB  | 1796KB | 6.5ms          | 约1.1ms           |
        | SYPCD48s   | 310           | 800x480    | yuv_nv12     | 828KB  | 2572KB | 11.0ms         | 约1.5ms           |
        | SYPCD58s   | 310           | 896x512    | yuv_nv12     | 852KB  | 3092KB | 13.1ms         | 约1.8ms           |

## 2. API参考

该功能模块提供以下API:

| API名称                                              | 功能                   |
| ---------------------------------------------------- | ---------------------- |
| [ALGO_DET_CreateHandle](#21-algo_det_createhandle)   | 创建句柄               |
| [ALGO_DET_InitHandle](#22-algo_det_inithandle)       | 初始化句柄             |
| [ALGO_DET_SetTracker](#23-algo_det_settracker)       | 设置后处理跟踪算法开关 |
| [ALGO_DET_SetStableBox](#24-algo_det_setstablebox)   | 设置后处理稳框的开关   |
| [ALGO_DET_GetInputAttr](#25-algo_det_getinputattr)   | 获取模型的属性信息     |
| [ALGO_DET_SetThreshold](#26-algo_det_setthreshold)   | 设置阈值               |
| [ALGO_DET_Run](#27-algo_det_run)                     | 运行检测算法           |
| [ALGO_DET_DeinitHandle](#28-algo_det_deinithandle)   | 停止检测算法           |
| [ALGO_DET_ReleaseHandle](#29-algo_det_releasehandle) | 释放句柄               |

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

    | 返回值 | 描述                           |
    | ------ | ------------------------------ |
    | 0      | 成功                           |
    | 其它   | 失败（详见[错误码](#errorcode)) |

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

    | 返回值 | 描述                           |
    | ------ | ------------------------------ |
    | 0      | 成功                           |
    | 其它   | 失败（详见[错误码](#errorcode)) |

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
    | tk_type  | tracker算法开关（设置0为关闭，1为开启，默认为关闭）                               | 输入      |
    | md_type  | 运动目标检测开关(设置为0，运动/静止目标均检测，设置为1，仅检测运动目标， 默认为0) | 输入      |

- 返回值

    | 返回值 | 描述                           |
    | ------ | ------------------------------ |
    | 0      | 成功                           |
    | 其它   | 失败（详见[错误码](#errorcode)) |

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
    | stable   | 稳框算法开关（设置false为关闭，true为开启，默认为关闭） | 输入      |

- 返回值

    | 返回值 | 描述                           |
    | ------ | ------------------------------ |
    | 0      | 成功                           |
    | 其它   | 失败（详见[错误码](#errorcode)) |

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

    | 返回值 | 描述                           |
    | ------ | ------------------------------ |
    | 0      | 成功                           |
    | 其它   | 失败（详见[错误码](#errorcode)) |

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

    | 返回值 | 描述                           |
    | ------ | ------------------------------ |
    | 0      | 成功                           |
    | 其它   | 失败（详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so

###

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
    | bboxes     | 用于保存检测结果框的数组     | 输入      |
    | num_bboxes | 用于保存检测结果框个数的指针 | 输入      |

- 返回值

    | 返回值 | 描述                           |
    | ------ | ------------------------------ |
    | 0      | 成功                           |
    | 其它   | 失败（详见[错误码](#errorcode)) |

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

    | 返回值 | 描述                           |
    | ------ | ------------------------------ |
    | 0      | 成功                           |
    | 其它   | 失败（详见[错误码](#errorcode)) |

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

    | 返回值 | 描述                           |
    | ------ | ------------------------------ |
    | 0      | 成功                           |
    | 其它   | 失败（详见[错误码](#errorcode)) |

- 依赖

    - 头文件：sstar_det_api.h
    - 库文件：libsstaralgo_det.a / libsstaralgo_det.so

## 3. 结构体/枚举类型说明

检测相关数据类型定义如下：

| 数据类型                                   | 定义                                   |
| ------------------------------------------ | -------------------------------------- |
| [Rect_t](#31-rect_t)                       | 算法输入结构体                         |
| [InputAttr_t](#32-inputattr_t)             | 算法输入结构体                         |
| [Box_t](#33-box_t)                         | 算法输出结构体                         |
| [DetectionInfo_t](#34-detectioninfo_t)     | 算法参数相关结构体                     |
| [ALGO_Input_t](#35-algo_input_t)           | 算法输入图像数据信息                   |
| [Label_PD_Person_e](#36-label_pd_person_e) | 行人检测算法class_id和类别名称的对应   |
| [Label_FD_Face_e](#37-label_fd_face_e)     | 人脸检测算法class_id和类别名称的对应   |
| [Label_PCN_e](#38-label_pcn_e)             | 人非车检测算法class_id和类别名称的对应 |
| [Label_PCD_e](#39-label_pcd_e)             | 人宠物检测算法class_id和类别名称的对应 |

### 3.1 Rect_t

- 说明

    定义分辨率大小

- 定义

        typedef struct{
            MI_U32 width;
            MI_U32 height;
        }Rect_t;


- 成员

   | 成员名称 | 描述             |
   | -------- | ---------------- |
   | width    | 模型输入数据的宽 |
   | height   | 模型输入数据的高 |

- 相关数据类型及接口

    [DetectionInfo_t](#34-detectioninfo_t)

    [ALGO_DET_InitHandle](#22-algo_det_inithandle)

### 3.2 InputAttr_t

- 说明

    定义分辨率大小和模型类型

- 定义

        typedef struct{
            MI_U32 width;
            MI_U32 height;
            MI_IPU_ELEMENT_FORMAT format;
        }InputAttr_t;


- 成员

    | 成员名称 | 描述               |
    | -------- | ------------------ |
    | width    | 模型输入数据的宽   |
    | height   | 模型输入数据的高   |
    | format   | 模型输入数据的类型 |

- 相关数据类型及接口

    [ALGO_DET_GetInputAttr](#25-algo_det_getinputattr)

### 3.3 Box_t

- 说明

    算法输出结构体

- 定义

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

- 成员

    | 成员名称            | 描述               |
    | ------------------- | ------------------ |
    | x，y, width, height | 检测结果框的位置   |
    | class_id            | 检测结果框的类别ID |
    | score               | 检测结果框的分值   |
    | pts                 | 送入检测帧的时间戳 |

- 相关数据类型及接口

    [ALGO_DET_Run](#27-algo_det_run)

### 3.4 DetectionInfo_t

- 说明

    检测算法相关配置项

- 定义

        typedef struct
        {
            char ipu_firmware_path[MAX_DET_STRLEN]; // ipu_firmware.bin path
            char model[MAX_DET_STRLEN];             // detect model path
            MI_FLOAT threshold;                     // confidence
            Rect_t disp_size;
        } DetectionInfo_t;

- 成员

    | 成员名称                          | 描述                                   |
    | --------------------------------- | -------------------------------------- |
    | ipu_firmware_path[MAX_DET_STRLEN] | ipu_firmware_path路径                  |
    | Model[MAX_DET_STRLEN]             | 模型文件夹路径                         |
    | threshold                         | 检测阈值（0~1）                        |
    | disp_size                         | 显示码流的分辨率（用于映射检测框位置） |

- 相关数据类型及接口

    [ALGO_DET_InitHandle](#22-algo_det_inithandle)

### 3.5 ALGO_Input_t

- 说明

    检测算法相关配置项

- 定义

        typedef struct
        {
            void *p_vir_addr;
            MI_PHY phy_addr;
            MI_U32 buf_size;
            MI_U64 pts;
        } ALGO_Input_t;

- 成员

    | 成员名称   | 描述                 |
    | ---------- | -------------------- |
    | p_vir_addr | 输入buffer的虚拟地址 |
    | phy_addr   | 输入buffer的物理地址 |
    | buf_size   | 输入buffer的长度     |
    | pts        | 输入buffer时间戳     |

- 相关数据类型及接口

    [ALGO_DET_Run](#27-algo_det_run)

### 3.6 Label_PD_Person_e

- 说明

    检测模型类别和class_id的对应关系

- 定义

        typedef enum
        {
            E_PD_PERSON = 0
        } Label_PD_Person_e;


- 成员

    | 成员名称    | 描述                 |
    | ----------- | -------------------- |
    | E_PD_PERSON | 行人类别(class_id=0) |

- 相关数据类型及接口

    [Box_t](#33-box_t)

### 3.7 Label_FD_Face_e

- 说明

    人脸检测模型类别和class_id的对应关系

- 定义

        typedef enum
        {
            E_FD_FACE = 0,
        } Label_FD_Face_e;


- 成员

    | 成员名称  | 描述                 |
    | --------- | -------------------- |
    | E_FD_FACE | 人脸类别(class_id=0) |

- 相关数据类型及接口

    [Box_t](#33-box_t)

### 3.8 Label_PCN_e

- 说明

    非车模型类别和class_id的对应关系

- 定义

        typedef enum
        {
            E_PCN_PERSON = 0,
            E_PCN_BICYCLE,
            E_PCN_CAR,
            E_PCN_MOTOCYCLE,
            E_PCN_BUS,
            E_PCN_TRUCK
        } Label_PCN_e;

- 成员

    | 成员名称        | 描述                   |
    | --------------- | ---------------------- |
    | E_PCN_PERSON    | 行人类别(class_id=0)   |
    | E_PCN_BICYCLE   | 自行车类别(class_id=1) |
    | E_PCN_CAR       | 轿车类别(class_id=2)   |
    | E_PCN_MOTOCYCLE | 摩托车类别(class_id=3) |
    | E_PCN_BUS       | 公交车类别(class_id=4) |
    | E_PCN_TRUCK     | 卡车类别(class_id=5)   |

- 相关数据类型及接口

    [Box_t](#33-box_t)

### 3.9 Label_PCD_e

- 说明

    定义人宠物检测模型类别和class_id的对应关系

- 定义

        typedef enum
        {
            E_PCD_PERSON,
            E_PCD_CAT,
            E_PCD_DOG
        } Label_PCD_e;

- 成员

   | 成员名称     | 描述                 |
   | ------------ | -------------------- |
   | E_PCN_PERSON | 行人类别(class_id=0) |
   | E_PCN_CAT    | 猫类别(class_id=1)   |
   | E_PCN_DOG    | 狗类别(class_id=2)   |

- 相关数据类型及接口

    [Box_t](#33-box_t)

## <span id=errorcode>4. 错误码 </span>

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
