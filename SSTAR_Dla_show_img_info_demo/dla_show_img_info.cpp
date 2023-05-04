/* SigmaStar trade secret */
/* Copyright (c) [2019~2021] SigmaStar Technology.
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


#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <getopt.h>
#include <unistd.h>
#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_ipu.h"
#include "mi_sys.h"
#include "show_ipu_log.h"


static bool DETAILS = false;
static std::string IPU_LOG_PATH;


std::string GetTensorTypeName(MI_IPU_ELEMENT_FORMAT eIPUFormat) {
    switch (eIPUFormat) {
        case MI_IPU_FORMAT_U8:
            return "UINT8";
        case MI_IPU_FORMAT_NV12:
            return "YUV_NV12";
        case MI_IPU_FORMAT_INT16:
            return "INT16";
        case MI_IPU_FORMAT_INT32:
            return "INT32";
        case MI_IPU_FORMAT_INT8:
            return "INT8";
        case MI_IPU_FORMAT_FP32:
            return "FLOAT32";
        case MI_IPU_FORMAT_UNKNOWN:
            return "UNKNOWN";
        case MI_IPU_FORMAT_ARGB8888:
            return "BGRA";
        case MI_IPU_FORMAT_ABGR8888:
            return "RGBA";
        case MI_IPU_FORMAT_GRAY:
            return "GRAY";
    }
    return "NOTYPE";
}


static std::vector<std::string> split(const std::string& str, const std::string& pattern) {
    char* strc = new char[strlen(str.c_str()) + 1];
    strcpy(strc, str.c_str());
    std::vector<std::string> resultVec;
    char* tmpStr = strtok(strc, pattern.c_str());
    while (tmpStr) {
        resultVec.push_back(std::string(tmpStr));
        tmpStr = strtok(NULL, pattern.c_str());
    }
    delete[] strc;
    return resultVec;
}


class IPUModel {
public:
    IPUModel(std::string model_path) {
        MI_S32 ret = MI_SUCCESS;
        Model_path = model_path;
        memset(&stChnAttr,0,sizeof(MI_IPUChnAttr_t));
        stChnAttr.u32InputBufDepth =  1;
        stChnAttr.u32OutputBufDepth = 1;
        ret = MI_IPU_CreateCHN(&u32ChannelID, &stChnAttr, NULL, const_cast<char*>(model_path.c_str()));
        if (ret != MI_SUCCESS) {
            std::string msg = "MI_IPU_CreateCHN(" + model_path + ") failed: " + std::to_string(ret) + "\n";
            throw std::runtime_error(msg);
        }
        ret = MI_IPU_GetInputTensors(u32ChannelID, &InputTensorVector);
        if (ret != MI_SUCCESS) {
            std::string msg = "MI_IPU_GetInputTensors(" + model_path + ", " + std::to_string(u32ChannelID) + ") failed: " + std::to_string(ret) + "\n";
            throw std::runtime_error(msg);
        }
        ret = MI_IPU_GetOutputTensors(u32ChannelID, &OutputTensorVector);
        if (ret != MI_SUCCESS) {
            std::string msg = "MI_IPU_GetOutputTensors(" + model_path + ", " + std::to_string(u32ChannelID) + ") failed: " + std::to_string(ret) + "\n";
            throw std::runtime_error(msg);
        }
        ret = MI_IPU_GetInOutTensorDesc(u32ChannelID, &desc);
        if (ret != MI_SUCCESS) {
            std::string msg = "MI_IPU_GetInOutTensorDesc(" + model_path + ", " + std::to_string(u32ChannelID) + ") failed: " + std::to_string(ret) + "\n";
            throw std::runtime_error(msg);
        }
        sleep(1);
    }

    ~IPUModel() noexcept {
        MI_IPU_PutInputTensors(u32ChannelID, &InputTensorVector);
        MI_IPU_PutOutputTensors(u32ChannelID, &OutputTensorVector);
        MI_IPU_DestroyCHN(u32ChannelID);
    }

    MI_IPU_SubNet_InputOutputDesc_t get_model_details() {
        return desc;
    }

    void invoke() {
        MI_S32 ret = MI_SUCCESS;
        ret = MI_IPU_Invoke(u32ChannelID, &InputTensorVector, &OutputTensorVector);
        if (ret != MI_SUCCESS) {
            std::string msg = "MI_IPU_Invoke(" + Model_path + ", " + std::to_string(u32ChannelID) + ") failed: " + std::to_string(ret) + "\n";
            throw std::runtime_error(msg);
        }
    }

    void set_input(MI_U32 index, void* inbuf, MI_S32 len) {
        if (index >= desc.u32InputTensorCount) {
            std::string msg = "Can not set_input for Input(" + std::to_string(index) + "), exceed MaxTensorIndex: " + std::to_string(desc.u32InputTensorCount - 1) + "\n";
            throw std::invalid_argument(msg);
        }
        if (len > desc.astMI_InputTensorDescs[index].s32AlignedBufSize) {
            std::string msg = "Can not set_input for Input(" + std::to_string(index) + "), exceed BufSize: " + std::to_string(desc.astMI_InputTensorDescs[index].s32AlignedBufSize) + "\n";
            throw std::invalid_argument(msg);
        }
        memcpy(InputTensorVector.astArrayTensors[index].ptTensorData[0], inbuf, len);
        if (desc.astMI_InputTensorDescs[index].eElmFormat != MI_IPU_FORMAT_FP32) {
            MI_SYS_FlushInvCache(InputTensorVector.astArrayTensors[index].ptTensorData[0], len);
        }
    }

    void get_output(MI_U32 index, void** outbuf, MI_S32* len) {
        if (index >= desc.u32OutputTensorCount) {
            std::string msg = "Can not get_output for Output(" + std::to_string(index) + "), exceed MaxTensorIndex: " + std::to_string(desc.u32OutputTensorCount - 1) + "\n";
            throw std::invalid_argument(msg);
        }
        *len = desc.astMI_OutputTensorDescs[index].s32AlignedBufSize;
        *outbuf = OutputTensorVector.astArrayTensors[index].ptTensorData[0];
    }

    friend std::ostream& operator <<(std::ostream& os, IPUModel& m) {
        auto time_0 = std::chrono::high_resolution_clock::now();
        m.invoke();
        auto time_1 = std::chrono::high_resolution_clock::now();
        auto Time = std::chrono::duration_cast<std::chrono::nanoseconds>(time_1 - time_0).count();
        os << m.Model_path << "(" << m.u32ChannelID << "):" << std::endl;
        os << "Invoke Time: " << Time / (1000.0 * 1000) << " ms" << std::endl;
        for (MI_U32 idx = 0; idx < m.desc.u32InputTensorCount; idx++) {
            os << "Input(" << idx << "):" << std::endl;
            os << "    name:\t" << m.desc.astMI_InputTensorDescs[idx].name << std::endl;
            os << "    dtype:\t" << GetTensorTypeName(m.desc.astMI_InputTensorDescs[idx].eElmFormat) << std::endl;
            os << "    shape:\t[";
            for (MI_U32 jdx = 0; jdx < m.desc.astMI_InputTensorDescs[idx].u32TensorDim; jdx++) {
                os << m.desc.astMI_InputTensorDescs[idx].u32TensorShape[jdx];
                if (jdx < m.desc.astMI_InputTensorDescs[idx].u32TensorDim - 1) {
                    os << ", ";
                }
            }
            os << "]" << std::endl;
            os << "    size:\t" << m.desc.astMI_InputTensorDescs[idx].s32AlignedBufSize << std::endl;
            if (m.desc.astMI_InputTensorDescs[idx].eLayoutType == E_IPU_LAYOUT_TYPE_NCHW) {
                os << "    layout:\t" << "NCHW" << std::endl;
            }
            if (m.desc.astMI_InputTensorDescs[idx].eElmFormat == MI_IPU_FORMAT_INT16) {
                os << "    quantization:\t(" << m.desc.astMI_InputTensorDescs[idx].fScalar << ", ";
                os << m.desc.astMI_InputTensorDescs[idx].s64ZeroPoint << ")" << std::endl;
            }
        }
        for (MI_U32 idx = 0; idx < m.desc.u32OutputTensorCount; idx++) {
            os << "Output(" << idx << "):" << std::endl;
            os << "    name:\t" << m.desc.astMI_OutputTensorDescs[idx].name << std::endl;
            os << "    dtype:\t" << GetTensorTypeName(m.desc.astMI_OutputTensorDescs[idx].eElmFormat) << std::endl;
            os << "    shape:\t[";
            for (MI_U32 jdx = 0; jdx < m.desc.astMI_OutputTensorDescs[idx].u32TensorDim; jdx++) {
                os << m.desc.astMI_OutputTensorDescs[idx].u32TensorShape[jdx];
                if (jdx < m.desc.astMI_OutputTensorDescs[idx].u32TensorDim - 1) {
                    os << ", ";
                }
            }
            os << "]" << std::endl;
            os << "    size:\t" << m.desc.astMI_OutputTensorDescs[idx].s32AlignedBufSize << std::endl;
            if (m.desc.astMI_OutputTensorDescs[idx].eLayoutType == E_IPU_LAYOUT_TYPE_NCHW) {
                os << "    layout:\t" << "NCHW" << std::endl;
            }
            if (m.desc.astMI_OutputTensorDescs[idx].eElmFormat == MI_IPU_FORMAT_INT16) {
                os << "    quantization:\t(" << m.desc.astMI_OutputTensorDescs[idx].fScalar << ", ";
                os << m.desc.astMI_OutputTensorDescs[idx].s64ZeroPoint << ")" << std::endl;
            }
        }
        if (DETAILS) {
            os << std::endl << "Details info:" << std::endl;
            std::ifstream fipu("/proc/mi_modules/mi_ipu/mi_ipu0");
            std::istreambuf_iterator<char> it{fipu}, end;
            std::string ss{it, end};
            os << ss;
            fipu.close();
        }
        return os;
    }

private:
    std::string Model_path;
    MI_IPUChnAttr_t stChnAttr;
    MI_U32 u32ChannelID;
    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_IPU_TensorVector_t InputTensorVector;
    MI_IPU_TensorVector_t OutputTensorVector;
};


static MI_U32 GetMaxVaribleSize(std::vector<std::string>& models) {
    MI_U32 u32MaxSize = 0;
    for (auto& m: models) {
        MI_IPU_OfflineModelStaticInfo_t ModelInfo;
        MI_S32 ret = MI_IPU_GetOfflineModeStaticInfo(NULL, const_cast<char*>(m.c_str()), &ModelInfo);
        if (ret != MI_SUCCESS)
        {
            std::string msg = "MI_IPU_GetOfflineModeStaticInfo(" + m + ") failed: " + std::to_string(ret) + "\n";
            throw std::runtime_error(msg);
        }
        u32MaxSize = std::max<MI_U32>(u32MaxSize, ModelInfo.u32VariableBufferSize);
    }
    return u32MaxSize;
}


void IPUCreateDevice(MI_U32 u32VarBufSize)
{
    std::cout << "mi_ipu_datatype.h info:" << std::endl;
    std::cout << "Max Input:\t" << MI_IPU_MAX_INPUT_TENSOR_CNT << std::endl;
    std::cout << "Max Output:\t" << MI_IPU_MAX_OUTPUT_TENSOR_CNT << std::endl;
    sleep(1);

    MI_IPU_DevAttr_t stDevAttr;
    memset(&stDevAttr, 0, sizeof(MI_IPU_DevAttr_t));
    stDevAttr.u32MaxVariableBufSize = u32VarBufSize;

    MI_S32 ret = MI_IPU_CreateDevice(&stDevAttr, NULL, NULL, 0);
    if (ret != MI_SUCCESS)
    {
        std::string msg = "MI_IPU_CreateDevice(" + std::to_string(u32VarBufSize) + ") failed: " + std::to_string(ret) + "\n";
        throw std::runtime_error(msg);
    }
}


int DumpIPULog(std::string& strModel) {
    int fd, ret, i;
    char str[512];
    struct ipu_log_info ipu_log_info;

    fd = open(IPU_LOG, O_RDONLY);
    if (fd < 0) {
        printf("fail to open %s, error=%s\n", IPU_LOG, strerror(errno));
        return -1;
    }

    ret = read(fd, str, sizeof(str) -1);
    if (ret < 0) {
        printf("fail to read %s, error=%s\n", IPU_LOG, strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);
    str[ret] = 0;
    if (strstr(str, IPU_LOG_OFF)) {
        printf("ipu log is off, no need to save ipu log\n");
        return 0;
    }
    printf("%s\n", str);

    memset(&ipu_log_info, 0, sizeof(ipu_log_info));
    ret = parse_log_info(&ipu_log_info, str);
    if (!ret) {
        for (i = 0; i < MAX_IPU_CORE_NUM; i++) {
            printf("ctrl addr=%llx, total_size=%u used_size=%u ",
                    ipu_log_info.ctrl[i].addr,
                    ipu_log_info.ctrl[i].total_size,
                    ipu_log_info.ctrl[i].used_size);
            printf("corectrl addr=%llx, total_size=%u used_size=%u\n",
                    ipu_log_info.corectrl[i].addr,
                    ipu_log_info.corectrl[i].total_size,
                    ipu_log_info.corectrl[i].used_size);
        }
    }

    ret = save_ipu_log(&ipu_log_info, IPU_LOG_PATH.c_str(), strModel.c_str());

    return ret;
}


void ShowIPUModelInfo(std::vector<std::string>& models) {
    for (auto& m: models) {
        IPUModel Model(m);
        std::cout << Model;
        std::cout << std::endl << std::endl;
        if (!IPU_LOG_PATH.empty()) {
            auto model_paths = split(m, "/");
            int ret = DumpIPULog(model_paths[model_paths.size() - 1]);
            if (ret) {
                throw std::runtime_error("Fail to Dump IPU LOG!\n");
            }
        }
    }
    sleep(1);
}


static void showUsage() {
    std::cout << "Usage: ./prog_dla_show_img_info  [-m PATH] ([--details_info])" << std::endl;
    std::cout << " -h, --help      show this help message and exit" << std::endl;
    std::cout << " -m, --model     path to model file" << std::endl;
    std::cout << "                 Mulitply models use comma-separated" << std::endl;
    std::cout << "     --details_info" << std::endl;
    std::cout << "                 show details model info (Default: false)" << std::endl;
    std::cout << "     --ipu_log" << std::endl;
    std::cout << "                 path to save ipu log" << std::endl;
    std::cout << "     --ipu_log_size" << std::endl;
    std::cout << "                 size of ipu log (Default: 0x800000)" << std::endl;
}


int main(int argc, char* argv[]) {
    char* modelFile = NULL;
    int opt;
    int lopt {0};
    size_t ipu_log_size = 0x800000;
    while (true) {
        int option_index = 0;
        static struct option long_options[] =
        {
            {"model",       required_argument, 0,  'm'},
            {"details_info",no_argument      ,&lopt,1 },
            {"ipu_log",     required_argument,&lopt,2 },
            {"ipu_log_size",required_argument,&lopt,3 },
        };
        opt = getopt_long(argc, argv, "hm:", long_options, &option_index);
        if (opt == -1) {
            break;
        }
        switch (opt) {
            case 'm':
                modelFile = optarg;
                break;
            case 0:
                switch (lopt) {
                    case 1:
                        DETAILS = true;
                        break;
                    case 2:
                        IPU_LOG_PATH = std::string(optarg);
                        break;
                    case 3:
                        ipu_log_size = std::stoi(optarg, nullptr, 0);
                        break;
                }
                break;
            case 'h':
                showUsage();
                return 0;
            default:
                showUsage();
                return 0;
        }
    }
    if (modelFile == NULL) {
        showUsage();
        return 0;
    }

    auto models = split(std::string(modelFile), ",");
    MI_SYS_Init(0);
    if (!IPU_LOG_PATH.empty()) {
        char IPU_LOG_CMD[256] = {0};
        sprintf(IPU_LOG_CMD, "echo ctrl_size=0x%x corectrl_size=0x%x ctrl=0xffffff corectrl=0x1fff > /sys/dla/ipu_log", ipu_log_size, ipu_log_size);
        system(IPU_LOG_CMD);
    }
    try {
        IPUCreateDevice(GetMaxVaribleSize(models));
        ShowIPUModelInfo(models);
    }
    catch(const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    if (!IPU_LOG_PATH.empty()) {
        system("echo ctrl_size=0x000000 corectrl_size=0x000000 ctrl=0xffffff corectrl=0x1fff > /sys/dla/ipu_log");
    }
    MI_IPU_DestroyDevice();
    MI_SYS_Exit(0);

    return 0;
}
