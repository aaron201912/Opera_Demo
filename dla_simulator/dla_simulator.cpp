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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <map>
#include <dirent.h>
#include <getopt.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/time.h>
#include <chrono>
#include <random>

#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_ipu.h"
#include "mi_sys.h"

#ifdef ALIGN_UP
#undef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#else
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(val, alignment) (((val)/(alignment))*(alignment))
#endif

#define LABEL_IMAGE_FUNC_INFO(fmt, args...) do {printf("[Info ] [%-4d] [%10s] ", __LINE__, __func__); printf(fmt, ##args);} while(0)
#define CLIP3(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
static std::string IPU_FIRMWARE;


struct PreProcessedData {
    std::string pImagePath;
    int iResizeH;
    int iResizeW;
    int iResizeC;
    std::string TrainingFormat;
    MI_IPU_ELEMENT_FORMAT eInputFormat;
    MI_U8* pdata;
    MI_U32 u32InputWidthAlignment;
    MI_U32 u32InputHeightAlignment;
    MI_IPU_LayoutType_e eLayoutType;
};


struct DetectionBBoxInfo {
    float xmin;
    float ymin;
    float xmax;
    float ymax;
    float score;
    int classID;
};


static void parse_images_dir(const std::vector<std::string>& base_path, std::vector<std::vector<std::string>>& file_path)
{
    for(int idx = 0; idx < base_path.size(); idx++)
    {
        std::vector<std::string> subFilePath;
        DIR* dir;
        struct dirent* ptr;
        std::string base_path_str {base_path[idx]};
        if ((dir = opendir(base_path_str.c_str())) == NULL)
        {
            subFilePath.push_back(base_path_str);
            file_path.push_back(subFilePath);
            continue;
        }
        if (base_path_str.back() != '/') {
            base_path_str.append("/");
        }
        while ((ptr = readdir(dir)) != NULL)
        {
            if (ptr->d_type == 8) {
                std::string path = base_path_str + ptr->d_name;
                subFilePath.push_back(path);
            }
        }
        closedir(dir);
        file_path.push_back(subFilePath);
    }
}


MI_S32 IPUCreateDevice(MI_U32 u32VarBufSize)
{
    MI_S32 s32Ret = MI_SUCCESS;
    char* pFirmwarePath = NULL;

    MI_IPU_DevAttr_t stDevAttr;
    memset(&stDevAttr, 0, sizeof(MI_IPU_DevAttr_t));
    stDevAttr.u32MaxVariableBufSize = u32VarBufSize;

    s32Ret = MI_IPU_CreateDevice(&stDevAttr, NULL, pFirmwarePath, 0);
    return s32Ret;
}


MI_S32 IPUCreateChannel(MI_U32* u32Channel, char* pModelImage)
{
    MI_SYS_GlobalPrivPoolConfig_t stGlobalPrivPoolConf;
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 1;
    stChnAttr.u32OutputBufDepth = 1;
    return MI_IPU_CreateCHN(u32Channel, &stChnAttr, NULL, pModelImage);
}

static std::string GetTensorTypeName(MI_IPU_ELEMENT_FORMAT eIPUFormat) {
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

static void Show_Model_Info(MI_IPU_SubNet_InputOutputDesc_t& desc) {
    for (MI_U32 idx = 0; idx < desc.u32InputTensorCount; idx++) {
        std::cout << "Input(" << idx << "):" << std::endl;
        std::cout << "    name:\t" << desc.astMI_InputTensorDescs[idx].name << std::endl;
        std::cout << "    dtype:\t" << GetTensorTypeName(desc.astMI_InputTensorDescs[idx].eElmFormat) << std::endl;
        std::cout << "    shape:\t[";
        for (MI_U32 jdx = 0; jdx < desc.astMI_InputTensorDescs[idx].u32TensorDim; jdx++) {
            std::cout << desc.astMI_InputTensorDescs[idx].u32TensorShape[jdx];
            if (jdx < desc.astMI_InputTensorDescs[idx].u32TensorDim - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;
        std::cout << "    size:\t" << desc.astMI_InputTensorDescs[idx].s32AlignedBufSize << std::endl;
        if (desc.astMI_InputTensorDescs[idx].eLayoutType == E_IPU_LAYOUT_TYPE_NCHW) {
            std::cout << "    layout:\t" << "NCHW" << std::endl;
        }
        if (desc.astMI_InputTensorDescs[idx].eElmFormat == MI_IPU_FORMAT_INT16) {
            std::cout << "    quantization:\t(" << desc.astMI_InputTensorDescs[idx].fScalar << ", ";
            std::cout << desc.astMI_InputTensorDescs[idx].s64ZeroPoint << ")" << std::endl;
        }
    }
    for (MI_U32 idx = 0; idx < desc.u32OutputTensorCount; idx++) {
        std::cout << "Output(" << idx << "):" << std::endl;
        std::cout << "    name:\t" << desc.astMI_OutputTensorDescs[idx].name << std::endl;
        std::cout << "    dtype:\t" << GetTensorTypeName(desc.astMI_OutputTensorDescs[idx].eElmFormat) << std::endl;
        std::cout << "    shape:\t[";
        for (MI_U32 jdx = 0; jdx < desc.astMI_OutputTensorDescs[idx].u32TensorDim; jdx++) {
            std::cout << desc.astMI_OutputTensorDescs[idx].u32TensorShape[jdx];
            if (jdx < desc.astMI_OutputTensorDescs[idx].u32TensorDim - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;
        std::cout << "    size:\t" << desc.astMI_OutputTensorDescs[idx].s32AlignedBufSize << std::endl;
        if (desc.astMI_OutputTensorDescs[idx].eLayoutType == E_IPU_LAYOUT_TYPE_NCHW) {
            std::cout << "    layout:\t" << "NCHW" << std::endl;
        }
        if (desc.astMI_OutputTensorDescs[idx].eElmFormat == MI_IPU_FORMAT_INT16) {
            std::cout << "    quantization:\t(" << desc.astMI_OutputTensorDescs[idx].fScalar << ", ";
            std::cout << desc.astMI_OutputTensorDescs[idx].s64ZeroPoint << ")" << std::endl;
        }
    }

}

static void get_hwPitch_Paras(PreProcessedData* pstPreProcessedData, MI_U32 *pu32YUV420_H_Pitch, MI_U32 *pu32YUV420_V_Pitch, MI_U32 *pu32XRGB_H_Pitch)
{
    // Input alignment set by user has been stored in model.

    // Get yuv420 hotizontal alignment from model.
    *pu32YUV420_H_Pitch = pstPreProcessedData->u32InputWidthAlignment;
    // Get yuv420 vertical alignment from model.
    *pu32YUV420_V_Pitch = pstPreProcessedData->u32InputHeightAlignment;
    // Get XRGB hotizontal alignment from model.
    *pu32XRGB_H_Pitch = pstPreProcessedData->u32InputWidthAlignment;
}
//Y = (R * 218  + G * 732  + B * 74) / 1024
//U = (R * -117 + G * -395 + B * 512)/ 1024
//V = (R * 512  + G * -465 + B * -47)/ 1024
static float rgb2yuv_covert_matrix[9] = {218.0/1024, 732.0/1024, 74.0/1024, -117.0/1024, -395.0/1024, 512.0/1024, 512.0/1024, -465.0/1024, -47.0/1024};

static void Transpose_NHWC_to_NCHW(void* dst, void* src, int H, int W, int C, int ElemSize) {
    int HWSize = H * W;
    MI_U8* pstdst = (MI_U8*)dst;
    MI_U8* pstsrc = (MI_U8*)src;
    for (int idx = 0; idx < HWSize; idx++) {
        for (int jdx = 0; jdx < C; jdx++) {
            memcpy((pstdst + (jdx * HWSize + idx) * ElemSize),
                   (pstsrc + (idx * C + jdx) * ElemSize), ElemSize);
        }
    }
}

static void OpenCV_Image(PreProcessedData* pstPreProcessedData)
{
    std::string filename = pstPreProcessedData->pImagePath;
    cv::Mat img = cv::imread(filename, cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "Error! Can not read picture: " << filename << std::endl;
        exit(1);
    }

    if (pstPreProcessedData->iResizeW <= 0 ||
        pstPreProcessedData->iResizeH <= 0 ||
        !(pstPreProcessedData->iResizeC == 3 || pstPreProcessedData->iResizeC == 1)) {
        std::cerr << "Error! This model training_formats cannot be set to BGR / RGB / GRAY!" << std::endl;
        exit(1);
    }

    cv::Mat sample_resized;
    cv::Size inputSize = cv::Size(pstPreProcessedData->iResizeW, pstPreProcessedData->iResizeH);
    if (img.size() != inputSize) {
        cv::resize(img, sample_resized, inputSize, 0, 0, cv::INTER_LINEAR);
    }
    else {
        sample_resized = img;
    }

    cv::Mat sample;
    int imageSize;
    int imageStride;
    MI_U32 u32YUV420_H_Pitch = 0, u32YUV420_V_Pitch = 0, u32XRGB_H_Pitch = 0;
    get_hwPitch_Paras(pstPreProcessedData, &u32YUV420_H_Pitch, &u32YUV420_V_Pitch, &u32XRGB_H_Pitch);

    if (pstPreProcessedData->eInputFormat == MI_IPU_FORMAT_U8) {
        if (pstPreProcessedData->TrainingFormat == "RGB") {
            cv::cvtColor(sample_resized, sample, cv::COLOR_BGR2RGB);
        }
        else {
            sample = sample_resized;
        }
        imageSize = pstPreProcessedData->iResizeH * pstPreProcessedData->iResizeW * pstPreProcessedData->iResizeC;
        MI_U8* pSrc = sample.data;
        if (pstPreProcessedData->eLayoutType == E_IPU_LAYOUT_TYPE_NCHW) {
            Transpose_NHWC_to_NCHW(pstPreProcessedData->pdata, pSrc, pstPreProcessedData->iResizeH,
                pstPreProcessedData->iResizeW, pstPreProcessedData->iResizeC, sizeof(MI_U8));
        }
        else {
            memcpy(pstPreProcessedData->pdata, pSrc, imageSize);
        }
    }
    else if (pstPreProcessedData->eInputFormat == MI_IPU_FORMAT_ARGB8888) {
        cv::cvtColor(sample_resized, sample, cv::COLOR_BGR2BGRA);
        MI_U8* pSrc = sample.data;
        MI_U8* pVirsrc = pstPreProcessedData->pdata;
        imageStride = ALIGN_UP(pstPreProcessedData->iResizeW, u32XRGB_H_Pitch) * 4;
        imageSize = pstPreProcessedData->iResizeH * imageStride;
        for (int i = 0; i < pstPreProcessedData->iResizeH; i++) {
            memcpy(pVirsrc, pSrc, pstPreProcessedData->iResizeW * 4);
            pVirsrc += imageStride;
            pSrc += pstPreProcessedData->iResizeW * 4;
        }
    }
    else if (pstPreProcessedData->eInputFormat == MI_IPU_FORMAT_ABGR8888) {
        cv::cvtColor(sample_resized, sample, cv::COLOR_BGR2RGBA);
        MI_U8* pSrc = sample.data;
        MI_U8* pVirsrc = pstPreProcessedData->pdata;
        imageStride = ALIGN_UP(pstPreProcessedData->iResizeW, u32XRGB_H_Pitch) * 4;
        imageSize = pstPreProcessedData->iResizeH * imageStride;
        for (int i = 0; i < pstPreProcessedData->iResizeH; i++) {
            memcpy(pVirsrc, pSrc, pstPreProcessedData->iResizeW * 4);
            pVirsrc += imageStride;
            pSrc += pstPreProcessedData->iResizeW * 4;
        }
    }
    else if (pstPreProcessedData->eInputFormat == MI_IPU_FORMAT_NV12) {
        cv::cvtColor(sample_resized, sample, cv::COLOR_BGR2RGB);
        int s32c = 3;
        int w = sample.cols;
        int h = sample.rows;

        float * pfSrc = (float *)malloc(w * h * s32c * sizeof(*pfSrc));

        for(int j = 0; j < w * h * 3; j++)
        {
            *(pfSrc + j) = *(sample.data + j);
        }

        int w_with_pitch = ALIGN_UP(w, u32YUV420_H_Pitch);
        int h_with_pitch = ALIGN_UP(h, u32YUV420_V_Pitch);

        float *yuv444_packet_buf = (float *)malloc(w_with_pitch * h_with_pitch * s32c * sizeof(*yuv444_packet_buf));
        if (yuv444_packet_buf == NULL)
        {
            std::cerr << "alloc yuv444_packet_buf fail " << std::endl;
            exit(-1);
        }
        for (int i = 0; i < w_with_pitch * h_with_pitch * s32c; ++i)
        {
            yuv444_packet_buf[i] = 0.0;
        }

        float *yuv444_semiplanar_buf = (float *)malloc(w_with_pitch * h_with_pitch * s32c * sizeof(*yuv444_semiplanar_buf));
        if (yuv444_semiplanar_buf == NULL)
        {
            std::cerr << "alloc yuv444_semiplanar_buf fail " << std::endl;
            free(yuv444_packet_buf);
            exit(-1);
        }

        imageSize = w_with_pitch * h_with_pitch * 3 / 2;
        float *trans_yuv_buf = (float *)malloc(imageSize * sizeof(*trans_yuv_buf));
        if (trans_yuv_buf == NULL)
        {
            std::cerr << "alloc trans_yuv_buf fail " << std::endl;
            free(pfSrc);
            free(yuv444_packet_buf);
            free(yuv444_semiplanar_buf);
            exit(-1);
        }

        float *yuv420_buf = (float *)trans_yuv_buf;

        //convert from rgb to YUV444 packed
        for (int index_h = 0; index_h < h; ++index_h)
        {
            for (int index_w = 0; index_w < w; ++index_w)
            {
                float *in_rgb = (float *)pfSrc + (index_h*w + index_w)*3;
                float *out_yuv444 = yuv444_packet_buf + (index_h*w_with_pitch + index_w)*3;

                float in_rgb0 = 0, in_rgb1 = 0, in_rgb2 = 0;
                in_rgb0 = in_rgb[2];
                in_rgb1 = in_rgb[1];
                in_rgb2 = in_rgb[0];

                out_yuv444[0] = rgb2yuv_covert_matrix[0] * in_rgb0 + rgb2yuv_covert_matrix[1] * in_rgb1 + rgb2yuv_covert_matrix[2] * in_rgb2; //Y
                out_yuv444[1] = rgb2yuv_covert_matrix[3] * in_rgb0 + rgb2yuv_covert_matrix[4] * in_rgb1 + rgb2yuv_covert_matrix[5] * in_rgb2 + 128; //U
                out_yuv444[2] = rgb2yuv_covert_matrix[6] * in_rgb0 + rgb2yuv_covert_matrix[7] * in_rgb1 + rgb2yuv_covert_matrix[8] * in_rgb2 + 128; //V

                //for the horizontal pitch, copy w-1 into w position
                if (((w&1) == 1) && (w_with_pitch > w) && (index_w == (w-1)))
                {
                    memcpy(&out_yuv444[3],  &out_yuv444[0], 3*sizeof(*yuv444_packet_buf));
                }
            }

            //for the vertical pitch, copy row h-1 into row h
            if (h_with_pitch == (h+1) && index_h == (h-1))
            {
                float *out_yuv444_last_row = yuv444_packet_buf + index_h*w_with_pitch*3;
                float *out_yuv444_last_row_pitch = yuv444_packet_buf + (index_h+1)*w_with_pitch*3;

                memcpy(out_yuv444_last_row_pitch, out_yuv444_last_row, 3*w_with_pitch*sizeof(*yuv444_packet_buf));
            }
        }

        //convert from YUV444_packet to YUV444 semiplanar
        for (int index_group = 0; index_group < h_with_pitch*w_with_pitch; ++index_group)
        {
            float *in_yuv444_packed = yuv444_packet_buf + 3*index_group;
            float *out_y_yuv444_planar = yuv444_semiplanar_buf + index_group;
            float *out_uv_yuv444_planar = yuv444_semiplanar_buf + h_with_pitch*w_with_pitch + index_group*2;

            out_y_yuv444_planar[0] =  in_yuv444_packed[0]; //Y
            out_uv_yuv444_planar[0] = in_yuv444_packed[1]; //U
            out_uv_yuv444_planar[1] = in_yuv444_packed[2]; //V
        }

        //convert from YUV444_planar to YUV420 planar
        //all Y
        memcpy(yuv420_buf, yuv444_semiplanar_buf, w_with_pitch*h_with_pitch*sizeof(*yuv420_buf));
        //for UV
        float *uv_yuv420_offset = yuv420_buf + w_with_pitch*h_with_pitch;
        float *uv_yuv444_offset = yuv444_semiplanar_buf + w_with_pitch*h_with_pitch;
        for (int index_h = 0; index_h < h_with_pitch/2; ++index_h)
        {
            for (int index_w = 0; index_w < w_with_pitch; index_w+=2)
            {
                float *in_row1_uv_yuv444_planar = uv_yuv444_offset + (index_h*2) * (w_with_pitch*2) + index_w * 2;
                float *in_row2_uv_yuv444_planar = uv_yuv444_offset + (index_h*2+1) * (w_with_pitch*2) + index_w * 2;
                float *out_uv_yuv420_planar = uv_yuv420_offset + index_h*w_with_pitch + index_w;

                float u = in_row1_uv_yuv444_planar[0] + in_row2_uv_yuv444_planar[0] + in_row1_uv_yuv444_planar[2] + in_row2_uv_yuv444_planar[2];
                float v = in_row1_uv_yuv444_planar[1] + in_row2_uv_yuv444_planar[1] + in_row1_uv_yuv444_planar[3] + in_row2_uv_yuv444_planar[3];

                out_uv_yuv420_planar[0] = u/4.0; //u
                out_uv_yuv420_planar[1] = v/4.0; //v
            }
        }

        for (int k = 0; k < imageSize; k++)
        {
            *((MI_U8 *)pstPreProcessedData->pdata + k) = (MI_U8)round(*(trans_yuv_buf + k));
        }

        free(yuv444_packet_buf);
        free(yuv444_semiplanar_buf);
        free(pfSrc);
        free(trans_yuv_buf);
    }
    else if (pstPreProcessedData->eInputFormat == MI_IPU_FORMAT_GRAY) {
        if (pstPreProcessedData->TrainingFormat == "GRAY") {
            cv::cvtColor(sample_resized, sample, cv::COLOR_BGR2GRAY);
        }
        else {
            std::cerr << "Error! training_input_formats is GRAY, input_formats must also be GRAY!" << std::endl;
            exit(1);
        }
        MI_U8* pSrc = sample.data;
        MI_U8* pVirsrc = pstPreProcessedData->pdata;
        int w_with_pitch = ALIGN_UP(pstPreProcessedData->iResizeW, u32YUV420_H_Pitch);
        imageSize = pstPreProcessedData->iResizeH * w_with_pitch;
        for (int idx = 0; idx < pstPreProcessedData->iResizeH; idx++) {
            for (int jdx = 0; jdx < pstPreProcessedData->iResizeW; jdx++) {
                pVirsrc[idx * w_with_pitch + jdx] = pSrc[idx * pstPreProcessedData->iResizeW + jdx];
            }
        }
    }
    else {
        std::cerr << "Error! Not support input formats: " << std::to_string(pstPreProcessedData->eInputFormat) << std::endl;
        exit(1);
    }

    MI_SYS_FlushInvCache(pstPreProcessedData->pdata, imageSize);
}


static void RAWDATA_Input(MI_IPU_TensorDesc_t* desc, PreProcessedData* pstPreProcessedData) {
    int inputSize = 1;
    for (int i = 0; i < desc->u32TensorDim; i++) {
        inputSize *= desc->u32TensorShape[i];
    }
    std::ifstream ifs(pstPreProcessedData->pImagePath, std::ios::binary);
    if (desc->eElmFormat == MI_IPU_FORMAT_FP32)
    {
        MI_FLOAT* inputBuf = new MI_FLOAT[inputSize];
        while (!ifs.eof()) {
            ifs.read(reinterpret_cast<char*>(inputBuf), inputSize * sizeof(MI_FLOAT));
        }
        MI_FLOAT* pfSrc = (MI_FLOAT*)(pstPreProcessedData->pdata);
        memcpy(pfSrc, inputBuf, inputSize * sizeof(MI_FLOAT));
        MI_SYS_FlushInvCache(pstPreProcessedData->pdata, inputSize * sizeof(MI_FLOAT));
        delete[] inputBuf;
    }
    else if (desc->eElmFormat == MI_IPU_FORMAT_INT16)
    {
        MI_FLOAT* inputBuf = new MI_FLOAT[inputSize];
        while (!ifs.eof()) {
            ifs.read(reinterpret_cast<char*>(inputBuf), inputSize * sizeof(MI_FLOAT));
        }
        MI_FLOAT fScalar = desc->fScalar;
        MI_S16* ps16Src = (MI_S16*)(pstPreProcessedData->pdata);
        for (int i = 0; i < inputSize; i++) {
            *(ps16Src + i) = (MI_S16)CLIP3(*(inputBuf + i) / fScalar, -32767 , 32767);
        }
        MI_SYS_FlushInvCache(pstPreProcessedData->pdata, inputSize * sizeof(MI_S16));
        delete[] inputBuf;
    }
    else {
        std::cerr << "Error! Not support input formats: " << std::to_string(pstPreProcessedData->eInputFormat) << std::endl;
        exit(1);
    }
    ifs.clear();
    ifs.close();
}


static void Dump_Rawdata_Input(MI_IPU_TensorDesc_t* desc, PreProcessedData* pstPreProcessedData) {
    std::ifstream ifs(pstPreProcessedData->pImagePath, std::ios::binary);
    ifs.seekg(0, std::ios_base::end);
    size_t raw_size = ifs.tellg();
    ifs.seekg(0, std::ios_base::beg);
    while (!ifs.eof()) {
        ifs.read(reinterpret_cast<char*>(pstPreProcessedData->pdata), raw_size);
    }
    MI_SYS_FlushInvCache(pstPreProcessedData->pdata, desc->s32AlignedBufSize);
    ifs.clear();
    ifs.close();
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


static void renew_output() {
    const char* output = "output";
    if (access(output, 0) != 0) {
        if (mkdir(output, 0777) != 0) {
            std::cerr << "mkdir error!" << std::endl;
            exit(1);
        }
    }
    else {
        std::string cmd = "rm -rf " + std::string(output);
        system(cmd.c_str());
        if (mkdir(output, 0777) != 0) {
            std::cerr << "mkdir error!" << std::endl;
            exit(1);
        }
    }
}


static std::vector<cv::Scalar> GetColors(const int n) {
    std::vector<cv::Scalar> colors;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    for (int i = 0; i < n; ++i) {
        colors.push_back(cv::Scalar(dis(gen) * 255, dis(gen) * 255, dis(gen) * 255));
    }
    return colors;
}

static void VisualizeBBox(const cv::Mat& image, const std::vector<DetectionBBoxInfo>& detections,
                   const float threshold, const std::vector<cv::Scalar>& colors,
                   const std::string& save_file) {
    // Retrieve detections.
    const int num_det = detections.size();
    if (num_det == 0) {
        return;
    }
    int fontface = cv::FONT_HERSHEY_SIMPLEX;
    double scale = 0.6;
    int thickness = 1;
    int baseline = 0;
    char buffer[50];
    const int width = image.cols;
    const int height = image.rows;
    // Draw bboxes.
    for (auto it = detections.begin(); it != detections.end(); ++it) {
        if (it->score > threshold) {
            int label = it->classID;
            const cv::Scalar& color = colors[label];
            cv::Point top_left_pt(it->xmin, it->ymin);
            cv::Point bottom_right_pt(it->xmax, it->ymax);
            cv::rectangle(image, top_left_pt, bottom_right_pt, color, 4);
            cv::Point bottom_left_pt(it->xmin, it->ymax);
            sprintf(buffer, "%d: %.3f", label, it->score);
            cv::Size text = cv::getTextSize(buffer, fontface, scale, thickness,
                                            &baseline);
            cv::rectangle(
                image, bottom_left_pt + cv::Point(0, 0),
                bottom_left_pt + cv::Point(text.width, -text.height-baseline),
                color, -1);
            cv::putText(image, buffer, bottom_left_pt - cv::Point(0, baseline),
                        fontface, scale, CV_RGB(0, 0, 0), thickness, 8);
        }
    }
    cv::imwrite(save_file.c_str(), image);
}


inline void GetTopN(float aData[], int dataSize, int aResult[], int TopN) {
    int i, j, k;
    float data = 0;
    MI_BOOL bSkip = FALSE;
    for (i=0; i < TopN; i++) {
        data = -0.1f;
        for (j = 0; j < dataSize; j++) {
            if (aData[j] > data) {
                bSkip = FALSE;
                for (k = 0; k < i; k++) {
                    if (aResult[k] == j) {
                        bSkip = TRUE;
                    }
                }
                if (bSkip == FALSE) {
                    aResult[i] = j;
                    data = aData[j];
                }
            }
        }
    }
}

static void DumpUnknown(std::string outputName, MI_IPU_TensorDesc_t* desc, MI_IPU_Tensor_t* OutputTensor, int num) {
    std::ofstream ofs;
    if (num == 0) {
        ofs.open(outputName, std::ios::trunc);
    }
    else {
        ofs.open(outputName, std::ios::app);
    }
    ofs.setf(std::ios::fixed, std::ios::floatfield);
    ofs.precision(6);
    if (desc->eElmFormat == MI_IPU_FORMAT_FP32) {
        MI_U32 shape_size = 1;
        ofs << desc->name << " Tensor:\n{\ntensor dim:" << desc->u32TensorDim << ",\tOriginal shape:[";
        for (int i = 0; i < desc->u32TensorDim; i++) {
            ofs << *((MI_U32*)desc->u32TensorShape + i) << " ";
            shape_size *= *((MI_U32*)desc->u32TensorShape + i);
        }
        ofs << "]\ntensor data:" << std::endl;
        float* data = (float*)(OutputTensor->ptTensorData[0]);
        for (int i = 0; i < shape_size; i++) {
            if (i > 0 && i % 16 == 0) {
                ofs << std::endl;
            }
            ofs << *(data + i) << "  ";
        }
    }
    else {
        float fscale = desc->fScalar;
        MI_U32 shape_size = 1;
        ofs << desc->name << " Tensor:\n{\ntensor dim:" << desc->u32TensorDim << ",\tOriginal shape:[";
        for (int i = 0; i < desc->u32TensorDim; i++) {
            ofs << *((MI_U32*)desc->u32TensorShape + i) << " ";
            shape_size *= *((MI_U32*)desc->u32TensorShape + i);
        }
        ofs << "]\ntensor data:" << std::endl;
        MI_S16* ps16data = (MI_S16*)(OutputTensor->ptTensorData[0]);
        for (int i = 0; i < shape_size; i++) {
            if (i > 0 && i % 16 == 0) {
                ofs << std::endl;
            }
            ofs << *(ps16data + i) * fscale << "  ";
        }
    }
    ofs << std::endl << "}" << std::endl << std::endl;
    ofs.close();
}


static void postClassification(MI_IPU_SubNet_InputOutputDesc_t* desc, MI_IPU_TensorVector_t* OutputTensorVector, std::string image_path, std::string modelFile) {
    std::vector<std::string> paths = split(image_path, "/");
    std::string imageName = paths.back();
    std::vector<std::string> model = split(modelFile, "/");
    std::string modelName = model.back();
    std::string outputName = "./output/classification_" + modelName + "_" + imageName + ".txt";
    int s32TopN[5];
    int iDimCount = desc->astMI_OutputTensorDescs[0].u32TensorDim;
    int s32ClassCount = 1;
    for(int i = 0; i < iDimCount; i++)
    {
        s32ClassCount *= desc->astMI_OutputTensorDescs[0].u32TensorShape[i];
    }
    MI_FLOAT* pfData = new MI_FLOAT[s32ClassCount];
    if (desc->astMI_OutputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_FP32)
    {
        memcpy(pfData, OutputTensorVector->astArrayTensors[0].ptTensorData[0], s32ClassCount * sizeof(MI_FLOAT));
    }
    else
    {
        for (int i = 0; i < s32ClassCount; i++)
        {
            MI_FLOAT fScalar = desc->astMI_OutputTensorDescs[0].fScalar;
            pfData[i] = *((MI_S16*)OutputTensorVector->astArrayTensors[0].ptTensorData[0] + i) * fScalar;
        }
    }
    GetTopN(pfData, s32ClassCount, s32TopN, 5);
    std::ofstream ofs(outputName, std::ios::trunc);
    ofs.setf(std::ios::fixed, std::ios::floatfield);
    ofs.precision(6);
    ofs << imageName << std::endl;
    for (int i = 0; i < 5; i++) {
        ofs << s32TopN[i] << " " << pfData[s32TopN[i]] << std::endl;
    }
    ofs.close();
    delete[] pfData;
}


static void postDetection(MI_IPU_SubNet_InputOutputDesc_t* desc, MI_IPU_TensorVector_t* OutputTensorVector, std::string image_path, std::string modelFile) {
    std::vector<std::string> paths = split(image_path, "/");
    std::string imageName = paths.back();
    std::vector<std::string> model = split(modelFile, "/");
    std::string modelName = model.back();
    std::string outputName = "./output/detection_" + modelName + "_" + imageName + ".txt";
    std::string outputImg = "./output/" + imageName;
    std::vector<std::string> imageNums = split(imageName, ".");
    cv::Mat img = cv::imread(image_path, -1);
    if (img.empty()) {
        std::cerr << "[Error] Can't open " << image_path << " for image, use `Unknown` for -c/--category parameter!" << std::endl;
        return;
    }
    float* pfBBox = NULL;
    float* pfClass = NULL;
    float* pfScore = NULL;
    float* pfDetect = NULL;
    MI_S16* ps16BBox = NULL;
    MI_S16* ps16Class = NULL;
    MI_S16* ps16Score = NULL;
    MI_S16* ps16Detect = NULL;
    int s32DetectCount = 0;
    if (desc->astMI_OutputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_FP32) {
        pfBBox   = (float *)(OutputTensorVector->astArrayTensors[0].ptTensorData[0]);
    } else {
        ps16BBox = (MI_S16*)(OutputTensorVector->astArrayTensors[0].ptTensorData[0]);
    }
    if (desc->astMI_OutputTensorDescs[1].eElmFormat == MI_IPU_FORMAT_FP32) {
        pfClass  = (float *)(OutputTensorVector->astArrayTensors[1].ptTensorData[0]);
    } else {
        ps16Class = (MI_S16*)(OutputTensorVector->astArrayTensors[1].ptTensorData[0]);
    }
    if (desc->astMI_OutputTensorDescs[2].eElmFormat == MI_IPU_FORMAT_FP32) {
        pfScore  = (float *)(OutputTensorVector->astArrayTensors[2].ptTensorData[0]);
    } else {
        ps16Score = (MI_S16*)(OutputTensorVector->astArrayTensors[2].ptTensorData[0]);
    }
    if (desc->astMI_OutputTensorDescs[3].eElmFormat == MI_IPU_FORMAT_FP32) {
        pfDetect  = (float *)(OutputTensorVector->astArrayTensors[3].ptTensorData[0]);
        s32DetectCount = int(*pfDetect);
    } else {
        ps16Detect = (MI_S16*)(OutputTensorVector->astArrayTensors[3].ptTensorData[0]);
        s32DetectCount = int(*ps16Detect);
    }
    std::vector<DetectionBBoxInfo> detections;
    detections.reserve(s32DetectCount);
    for(int i = 0; i < s32DetectCount; i++)
    {
        DetectionBBoxInfo detection;
        //box coordinate
        if (pfBBox != NULL) {
            detection.ymin = *(pfBBox + i * 4 + 0) * img.rows;
            detection.xmin = *(pfBBox + i * 4 + 1) * img.cols;
            detection.ymax = *(pfBBox + i * 4 + 2) * img.rows;
            detection.xmax = *(pfBBox + i * 4 + 3) * img.cols;
        } else {
            float fscale = desc->astMI_OutputTensorDescs[0].fScalar;
            detection.ymin = *(ps16BBox + i * 4 + 0) * img.rows * fscale;
            detection.xmin = *(ps16BBox + i * 4 + 1) * img.cols * fscale;
            detection.ymax = *(ps16BBox + i * 4 + 2) * img.rows * fscale;
            detection.xmax = *(ps16BBox + i * 4 + 3) * img.cols * fscale;
        }
        //box class
        if (pfClass != NULL) {
            detection.classID = int(*(pfClass + i)) + 1;
        } else {
            detection.classID = int(*(ps16Class + i)) + 1;
        }
        //score
        if (pfScore != NULL) {
            detection.score = *(pfScore + i);
        } else {
            float fscale = desc->astMI_OutputTensorDescs[2].fScalar;
            detection.score = *(ps16Score + i) * fscale;
        }
        detections.push_back(detection);
    }
    VisualizeBBox(img, detections, 0.7, GetColors(100), outputImg);
    std::ofstream ofs(outputName, std::ios::trunc);
    ofs.setf(std::ios::fixed, std::ios::floatfield);
    ofs.precision(6);
    for (int i = 0; i < detections.size(); i++) {
        ofs << "{\"image_id\":" << imageNums[0] << ", \"category_id\":" << detections[i].classID << ", \"bbox\":[";
        ofs << detections[i].xmin << "," << detections[i].ymin << "," << detections[i].xmax - detections[i].xmin;
        ofs << "," << detections[i].ymax - detections[i].ymin << "], \"score\": " << detections[i].score << "}," << std::endl;
    }
    ofs.close();
}


static void postUnknown(MI_IPU_SubNet_InputOutputDesc_t* desc, MI_IPU_TensorVector_t* OutputTensorVector, std::string image_path, std::string modelFile) {
    std::vector<std::string> paths = split(image_path, "/");
    std::string imageName = paths.back();
    std::vector<std::string> model = split(modelFile, "/");
    std::string modelName = model.back();
    std::string outputName = "./output/unknown_" + modelName + "_" + imageName + ".txt";
    for (int i = 0; i < desc->u32OutputTensorCount; i++) {
        DumpUnknown(outputName, &desc->astMI_OutputTensorDescs[i], &OutputTensorVector->astArrayTensors[i], i);
    }
}

MI_U32 _MI_IPU_GetTensorUnitDataSize(MI_IPU_ELEMENT_FORMAT eElmFormat)
{
    switch (eElmFormat) {
        case MI_IPU_FORMAT_INT16:
            return sizeof(short);
        case MI_IPU_FORMAT_INT32:
            return sizeof(int);
        case MI_IPU_FORMAT_INT8:
            return sizeof(char);
        case MI_IPU_FORMAT_FP32:
            return sizeof(float);
        case MI_IPU_FORMAT_UNKNOWN:
        default:
            return 1;
    }
}

MI_U32 IPU_CalcTensorSize(MI_IPU_TensorDesc_t stTensorDescs)
{
    MI_U32 u32Size = 1;
    MI_U32 u32UnitSize = 1;

    u32UnitSize = _MI_IPU_GetTensorUnitDataSize(stTensorDescs.eElmFormat);

    for (int i = 0; i < stTensorDescs.u32TensorDim; i++)
    {
        u32Size *= stTensorDescs.u32TensorShape[i];
    }
    u32Size *= u32UnitSize;

    return u32Size;
}

static void IPU_PrintOutputXOR(MI_IPU_SubNet_InputOutputDesc_t desc, MI_IPU_TensorVector_t OutputTensorVector)
{
    MI_U32 u32InputNum = desc.u32InputTensorCount;
    MI_U32 u32OutputNum = desc.u32OutputTensorCount;

    volatile MI_U32 u32XORValue = 0;
    MI_U8 *pu8XORValue[4]= {(MI_U8 *)&u32XORValue,(MI_U8 *)&u32XORValue+1,(MI_U8 *)&u32XORValue+2,(MI_U8 *)&u32XORValue+3 };
    MI_U32 u32Count = 0;

    for (MI_U32 idxOutputNum = 0; idxOutputNum < desc.u32OutputTensorCount; idxOutputNum++)
    {
        MI_U8 u8Data = 0;
        MI_U8 *pu8Data = (MI_U8 *)OutputTensorVector.astArrayTensors[idxOutputNum].ptTensorData[0];
        for(int i = 0; i < IPU_CalcTensorSize(desc.astMI_OutputTensorDescs[idxOutputNum]); i++)
        {
            u8Data = *(pu8Data + i);
            *pu8XORValue[u32Count%4] ^= u8Data;
            u32Count++;
        }
    }
    printf("All outputs XOR = 0x%08x\n", u32XORValue);
}


inline void tf_testInterpreterLabelImage(std::vector<std::vector<std::string>> images, std::string modelFile, std::string category, std::vector<std::string> vecFormats) {
    MI_U32 u32ChannelID;
    MI_IPU_OfflineModelStaticInfo_t OfflineModelInfo;
    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_IPU_TensorVector_t InputTensorVector {0, 0, 0, 0};
    MI_IPU_TensorVector_t OutputTensorVector;
    MI_S32 ret;
    std::string defaultFormat = "BGR";

    // 1. create device
    MI_SYS_Init(0);
    ret = MI_IPU_GetOfflineModeStaticInfo(NULL, const_cast<char *>(modelFile.c_str()), &OfflineModelInfo);
    if (ret != MI_SUCCESS)
    {
        std::cerr << "Get model variable buffer size failed! Error Code: " << ret << std::endl;
        return;
    }
    ret = IPUCreateDevice(OfflineModelInfo.u32VariableBufferSize);
    if (ret != MI_SUCCESS)
    {
        std::cerr << "Create IPU Device failed! Error Code: " << ret << std::endl;
        return;
    }

    // 2. create channel
    ret = IPUCreateChannel(&u32ChannelID, const_cast<char *>(modelFile.c_str()));
    if (ret != MI_SUCCESS)
    {
        MI_IPU_DestroyDevice();
        std::cerr << "Create IPU Channel failed! Error Code: " << ret << std::endl;
        return;
    }

    // 3. get input/output tensor
    MI_IPU_GetInOutTensorDesc(u32ChannelID, &desc);
    MI_IPU_GetInputTensors(u32ChannelID, &InputTensorVector);
    MI_IPU_GetOutputTensors(u32ChannelID, &OutputTensorVector);
    Show_Model_Info(desc);

    if(vecFormats.empty())
    {
        for(int inputIdx = 0; inputIdx < desc.u32InputTensorCount; inputIdx++)
        {
            vecFormats.push_back(defaultFormat);
        }
    }

    if((vecFormats.size() != images.size()) || (vecFormats.size() != desc.u32InputTensorCount))
    {
        MI_IPU_PutInputTensors(u32ChannelID, &InputTensorVector);
        MI_IPU_PutOutputTensors(u32ChannelID, &OutputTensorVector);
        MI_IPU_DestroyCHN(u32ChannelID);
        MI_IPU_DestroyDevice();
        std::cerr << "Size of format list, size of image list and num of model inputs should be same! Pls check demo command!" << std::endl;
        return;
    }

    for(int idx = 0; idx < images[0].size(); idx++)
    {
        std::cout << idx + 1 << " / " << images[0].size() << '\t';

        for(int inputIdx = 0; inputIdx < desc.u32InputTensorCount; inputIdx++)
        {
            PreProcessedData stProcessedData = PreProcessedData();
            stProcessedData.eLayoutType = desc.astMI_InputTensorDescs[inputIdx].eLayoutType;
            if (stProcessedData.eLayoutType == E_IPU_LAYOUT_TYPE_NCHW) {
                stProcessedData.iResizeH = desc.astMI_InputTensorDescs[inputIdx].u32TensorShape[2];
                stProcessedData.iResizeW = desc.astMI_InputTensorDescs[inputIdx].u32TensorShape[3];
                stProcessedData.iResizeC = desc.astMI_InputTensorDescs[inputIdx].u32TensorShape[1];
            }
            else {
                stProcessedData.iResizeH = desc.astMI_InputTensorDescs[inputIdx].u32TensorShape[1];
                stProcessedData.iResizeW = desc.astMI_InputTensorDescs[inputIdx].u32TensorShape[2];
                stProcessedData.iResizeC = desc.astMI_InputTensorDescs[inputIdx].u32TensorShape[3];
            }
            stProcessedData.TrainingFormat = vecFormats[inputIdx];
            stProcessedData.eInputFormat = desc.astMI_InputTensorDescs[inputIdx].eElmFormat;
            stProcessedData.u32InputHeightAlignment = desc.astMI_InputTensorDescs[inputIdx].u32InputHeightAlignment;
            stProcessedData.u32InputWidthAlignment = desc.astMI_InputTensorDescs[inputIdx].u32InputWidthAlignment;
            stProcessedData.pdata = (MI_U8*)InputTensorVector.astArrayTensors[inputIdx].ptTensorData[0];
            stProcessedData.pImagePath = images[inputIdx][idx];

            if (vecFormats[inputIdx] == "RAWDATA_NHWC") {
                RAWDATA_Input(&desc.astMI_InputTensorDescs[inputIdx], &stProcessedData);
            }
            else if (vecFormats[inputIdx] == "DUMP_RAWDATA") {
                Dump_Rawdata_Input(&desc.astMI_InputTensorDescs[inputIdx], &stProcessedData);
            }
            else {
                OpenCV_Image(&stProcessedData);
            }
        }


        // 4. invoke
        auto time_0 = std::chrono::high_resolution_clock::now();
        ret = MI_IPU_Invoke(u32ChannelID, &InputTensorVector, &OutputTensorVector);
        auto time_1 = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(time_1 - time_0).count() / (1000.0 * 1000) << " ms" << std::endl;
        if (ret != MI_SUCCESS)
        {
            MI_IPU_DestroyCHN(u32ChannelID);
            MI_IPU_DestroyDevice();
            std::cerr << "IPU invoke failed! Error Code: " << ret << std::endl;
            return;
        }
        // post-process
        IPU_PrintOutputXOR(desc, OutputTensorVector);

        if (category == "Classification") {
            postClassification(&desc, &OutputTensorVector, images[0][idx], modelFile);
        }
        else if (category == "Detection") {
            postDetection(&desc, &OutputTensorVector, images[0][idx], modelFile);
        }
        else {
            postUnknown(&desc, &OutputTensorVector, images[0][idx], modelFile);
        }
    }

    // 5. put intput tensor
    MI_IPU_PutInputTensors(u32ChannelID, &InputTensorVector);
    MI_IPU_PutOutputTensors(u32ChannelID, &OutputTensorVector);

    // 6. destroy channel/device
    MI_IPU_DestroyCHN(u32ChannelID);
    MI_IPU_DestroyDevice();
    MI_SYS_Exit(0);

    std::cout << "Done! Results in `output` folder." << std::endl;
}


static void showUsage() {
    std::cout << "Usage: ./prog_dla_dla_simulator [-i] [-m] ([-c] [-f]) " << std::endl;
    std::cout << " -h, --help      show this help message and exit" << std::endl;
    std::cout << " -i, --images    path to validation image or images' folder" << std::endl;
    std::cout << " -m, --model     path to model file" << std::endl;
    std::cout << " -c, --category  indicate model category (Default is Unknown):" << std::endl;
    std::cout << "                 Classification / Detection / Unknown" << std::endl;
    std::cout << " -f, --training_formats" << std::endl;
    std::cout << "                 model training input formats (Default is BGR):" << std::endl;
    std::cout << "                 BGR / RGB / GRAY / RAWDATA_NHWC / DUMP_RAWDATA" << std::endl;
}


int main(int argc, char* argv[]) {
    char strUnknown[16] = "Unknown\0";
    std::string imagesPath;
    char* modelFile = NULL;
    char* category = strUnknown;
    std::string mFormat;
    int opt;
    while (true) {
        int option_index = 0;
        static struct option long_options[] =
        {
            {"images",  required_argument, 0,  'i' },
            {"model",   required_argument, 0,  'm' },
            {"category",required_argument, 0,  'c' },
            {"training_formats",  required_argument, 0,  'f' },
            {"help",    no_argument,       0,  'h' },
        };
        opt = getopt_long(argc, argv, "hi:m:c:f:", long_options, &option_index);
        if (opt == -1) {
            break;
        }
        switch (opt) {
            case 'i':
                imagesPath = std::string(optarg);
                break;
            case 'm':
                modelFile = optarg;
                break;
            case 'c':
                category = optarg;
                break;
            case 'f':
                mFormat = std::string(optarg);
                break;
            case 'h':
                showUsage();
                return 0;
            default:
                showUsage();
                return 0;
        }
    }
    if ((imagesPath.empty()) || (modelFile == NULL)) {
        showUsage();
        return 0;
    }
    if (strcmp(category, "Classification") && strcmp(category, "Detection") && strcmp(category, "Unknown")) {
        std::cout << "model category only support `Classification / Detection / Unknown`" << std::endl;
        return 0;
    }

    for(int idx = 0; idx < mFormat.size(); idx++)
    {
        if(mFormat[idx] == ',')
        {
            mFormat[idx] = ' ';
        }
    }

    std::istringstream formats(mFormat);
    std::string tmpStr;
    std::vector<std::string> vecFormats;

    while(formats >> tmpStr)
    {
        vecFormats.push_back(tmpStr);
    }

    if (!vecFormats.empty())
    {
        for(int idx = 0; idx < vecFormats.size(); idx++)
        {
            if ((vecFormats[idx] != "BGR") && (vecFormats[idx] != "RGB") && (vecFormats[idx] != "GRAY") &&
                (vecFormats[idx] != "RAWDATA_NHWC") && (vecFormats[idx] != "DUMP_RAWDATA"))
            {
                std::cout << "model training input format only support `BGR / RGB / GRAY / RAWDATA_NHWC / DUMP_RAWDATA`" << std::endl;
                return 0;
            }
        }
    }

    for(int idx = 0; idx < imagesPath.size(); idx++)
    {
        if(imagesPath[idx] == ',')
        {
            imagesPath[idx] = ' ';
        }
    }

    std::istringstream inputs(imagesPath);
    std::vector<std::string> foldernames;
    std::vector<std::vector<std::string>> images;

    while(inputs >> tmpStr)
    {
        foldernames.push_back(tmpStr);
    }

    parse_images_dir(foldernames, images);

    // For single-input model, you can put several images in one folder to invoke several times.
    // For multi-input model, you can only use one image for each input to invoke one time.
    if(images.size() > 1)
    {
        for(int idx = 0; idx < images.size(); idx++)
        {
            if(images[idx].size() != 1)
            {
                std::cout << "Wrong image numbers! For multi-input models, only support one image each input!" << std::endl;
                return 0;
            }
        }
    }
    renew_output();
    tf_testInterpreterLabelImage(images, std::string(modelFile), std::string(category), vecFormats);

    return 0;
}
