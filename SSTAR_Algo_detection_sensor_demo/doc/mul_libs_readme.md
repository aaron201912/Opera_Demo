# 多库和多handle使用说明书

-   多库和多handle调用算法流程如下：
        1，首先将所有用到的模型进行遍历读取，并创建device,调用下面MulModelsCreateDevice函数
        2，创建对应算法的handle，并初始化对应的handle,初始化参数的had_create_device设置为true
        3, 调用算法对应的功能接口
        4，释放handle,调用对应的deinit和release handle
        5, 最后调用MI_IPU_DestroyDevice()

## 多库和多handle使用示例

-   多库和多handle使用示例：以dt和det库为例进行伪代码示例（多handle类似调用）

    #include <stdio.h>
    #include <stdlib.h>
    #include <string>
    #include "mi_ipu.h"
    #include "mi_sys_datatype.h"
    #include "mi_sys.h"
    #include "algo_dt_api.h"
    #include "sstar_det_api.h"
    int MulModelsCreateDevice(char** model_path, int number,char* ipuPath)
    {
        uint32_t maxSize = 0;
        for (int i = 0; i < number; ++i)
        {
            MI_IPU_OfflineModelStaticInfo_t offline_model_info;
            memset(&offline_model_info, 0, sizeof(MI_IPU_OfflineModelStaticInfo_t));
            if (MI_SUCCESS != MI_IPU_GetOfflineModeStaticInfo(NULL, (char*)model_path[i], &offline_model_info))
            {
                printf("get Model variable buffer size failed! modelpath:%s", model_path[i]);
                return -1;
            }
            if (offline_model_info.u32VariableBufferSize > maxSize)
            {
                maxSize = offline_model_info.u32VariableBufferSize;
            }
        }

        MI_IPU_DevAttr_t stDevAttr;
        memset(&stDevAttr, 0, sizeof(MI_IPU_DevAttr_t));
        stDevAttr.u32MaxVariableBufSize = maxSize;
        int ret = MI_IPU_CreateDevice(&stDevAttr, NULL, ipuPath, 0);
        if (ret != MI_SUCCESS)
        {
            printf("create device failed %d\n",ret);
            return ret;
        }
        return MI_SUCCESS;
    }

    int main()
    {
        MI_SYS_Init(0);
        void* dthandle;
        void* detHandle;
        std::string ipuPath = "/config/dla/ipu_firmware.bin";
        std::string model1 = "./models/sypd36.img";
        std::string model2 = "./models/sypd48.img";
        char* models[3];
        models[0] = new char[128];
        models[1] = new char[128];
        strcpy(models[0], model1.c_str());
        strcpy(models[1], model1.c_str());
        MulModelsCreateDevice(models,2,(char*)ipuPath.c_str());

        //det和dt可以写到各自的线程中去
        {
            //创建det和init det
            ALGO_DET_CreateHandle(&detHandle);
            std::string ipuPath = "/config/dla/ipu_firmware.bin";
            DetectionInfo_t info;
            memset(&info, 0, sizeof(info));
            info.had_create_device=true;
            ...
            memcpy(info.model, model1.c_str(), model1.length() + 1);
            ALGO_DET_InitHandle(detHandle, &info);

            //创建dt和init dt
            ALGO_DT_CreateHandle(&dthandle);
            DtInfo_t dt_info;
            memset(&(dt_info), 0, sizeof(DtInfo_t));
            memcpy(dt_info.model, model2.c_str(), model2.length() + 1);
            ...
            dt_info.had_create_device=true;
            ALGO_DT_InitHandle(dthandle, &dt_info);

            //dt和det功能使用
            ...

            //释放dt和det
            ALGO_DET_DeinitHandle(detHandle)
            ALGO_DET_ReleaseHandle(detHandle)
            ALGO_DT_DeinitHandle(dtHandle)
            ALGO_DT_ReleaseHandle(dtHandle)
            //
        }

        delete[] models[0];
        delete[] models[1];
        MI_IPU_DestroyDevice();
        MI_SYS_Exit(0);
        return 0;
    }

