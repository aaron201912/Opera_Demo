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
    std::string mFormat;
    MI_U8* pdata;
    MI_U32 u32InputWidthAlignment;
    MI_U32 u32InputHeightAlignment;
};


struct DetectionBBoxInfo {
    float xmin;
    float ymin;
    float xmax;
    float ymax;
    float score;
    int classID;
};


static void parse_images_dir(const std::string& base_path, std::vector<std::string>& file_path)
{
    DIR* dir;
    struct dirent* ptr;
    std::string base_path_str {base_path};
    if ((dir = opendir(base_path_str.c_str())) == NULL)
    {
        file_path.push_back(base_path_str);
        return;
    }
    if (base_path_str.back() != '/') {
        base_path_str.append("/");
    }
    while ((ptr = readdir(dir)) != NULL)
    {
        if (ptr->d_type == 8) {
            std::string path = base_path_str + ptr->d_name;
            file_path.push_back(path);
        }
    }
    closedir(dir);
}


MI_S32 IPUCreateDevice(MI_U32 u32VarBufSize)
{
    MI_S32 s32Ret = MI_SUCCESS;
    char* pFirmwarePath = NULL;

    MI_IPU_DevAttr_t stDevAttr;
    stDevAttr.u32MaxVariableBufSize = u32VarBufSize;

    s32Ret = MI_IPU_CreateDevice(&stDevAttr, NULL, pFirmwarePath, 0);
    return s32Ret;
}


MI_S32 IPUCreateChannel(MI_U32* u32Channel, char* pModelImage, int number)
{
    MI_SYS_GlobalPrivPoolConfig_t stGlobalPrivPoolConf;
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 1;
    stChnAttr.u32OutputBufDepth = 1;
    stChnAttr.u32BatchMax = number;
    return MI_IPU_CreateCHN(u32Channel, &stChnAttr, NULL, pModelImage);
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

static void OpenCV_Image(PreProcessedData* pstPreProcessedData)
{
    std::string filename = pstPreProcessedData->pImagePath;
    cv::Mat img = cv::imread(filename, cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "Error! Image doesn't exist!" << std::endl;
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

    if (pstPreProcessedData->mFormat == "RGB") {
        cv::cvtColor(sample_resized, sample, cv::COLOR_BGR2RGB);
        imageSize = pstPreProcessedData->iResizeH * pstPreProcessedData->iResizeW * 3;
        MI_U8* pSrc = sample.data;
        for(int i = 0; i < imageSize; i++)
        {
            *(pstPreProcessedData->pdata + i) = *(pSrc + i);
        }
    }
    else if (pstPreProcessedData->mFormat == "BGRA") {
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
    else if (pstPreProcessedData->mFormat == "RGBA") {
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
    else if ((pstPreProcessedData->mFormat == "YUV_NV12") || (pstPreProcessedData->mFormat == "GRAY")) {
        int s32c = 3;
        int w = sample_resized.cols;
        int h = sample_resized.rows;

        float * pfSrc = (float *)malloc(w * h * s32c * sizeof(*pfSrc));

        for(int j = 0; j < w * h * 3; j++)
        {
            *(pfSrc + j) = *(sample_resized.data + j);
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
    else {
        sample = sample_resized;
        imageSize = pstPreProcessedData->iResizeH * pstPreProcessedData->iResizeW * 3;
        MI_U8* pSrc = sample.data;
        for(int i = 0; i < imageSize; i++)
        {
            *(pstPreProcessedData->pdata + i) = *(pSrc + i);
        }
    }

    MI_SYS_FlushInvCache(pstPreProcessedData->pdata, imageSize);
}


static void RAWDATA_S16_Input(MI_IPU_TensorDesc_t* desc, PreProcessedData* pstPreProcessedData) {
    int inputSize = 1;
    for (int i = 0; i < desc->u32TensorDim; i++) {
        inputSize *= desc->u32TensorShape[i];
    }
    MI_FLOAT* inputBuf = new MI_FLOAT[inputSize];
    std::ifstream ifs(pstPreProcessedData->pImagePath, std::ios::binary);
    while (!ifs.eof()) {
        ifs.read(reinterpret_cast<char*>(inputBuf), inputSize * sizeof(MI_FLOAT));
    }
    if (desc->eElmFormat == MI_IPU_FORMAT_FP32)
    {
        MI_FLOAT* pfSrc = (MI_FLOAT*)(pstPreProcessedData->pdata);
        memcpy(pfSrc, inputBuf, inputSize * sizeof(MI_FLOAT));
    }
    else
    {
        MI_FLOAT fScalar = desc->fScalar;
        MI_S16* ps16Src = (MI_S16*)(pstPreProcessedData->pdata);
        for (int i = 0; i < inputSize; i++) {
            *(ps16Src + i) = (MI_S16)CLIP3(*(inputBuf + i) / fScalar, -32768 , 32767);
        }
        MI_SYS_FlushInvCache(pstPreProcessedData->pdata, inputSize * sizeof(MI_S16));
    }
    delete[] inputBuf;
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


static void postClassification(MI_IPU_SubNet_InputOutputDesc_t* desc, MI_IPU_Tensor_t* OutputTensors, std::string image_path, std::string modelFile) {
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
        memcpy(pfData, OutputTensors->ptTensorData[0], s32ClassCount * sizeof(MI_FLOAT));
    }
    else
    {
        for (int i = 0; i < s32ClassCount; i++)
        {
            MI_FLOAT fScalar = desc->astMI_OutputTensorDescs[0].fScalar;
            pfData[i] = *((MI_S16*)OutputTensors->ptTensorData[0] + i) * fScalar;
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


static void postDetection(MI_IPU_SubNet_InputOutputDesc_t* desc, MI_IPU_Tensor_t* OutputTensors, std::string image_path, std::string modelFile) {
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
    MI_IPU_Tensor_t* pOutputTensors = OutputTensors;
    if (desc->astMI_OutputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_FP32) {
        pfBBox   = (float *)(pOutputTensors->ptTensorData[0]);
    } else {
        ps16BBox = (MI_S16*)(pOutputTensors->ptTensorData[0]);
    }
    pOutputTensors++;
    if (desc->astMI_OutputTensorDescs[1].eElmFormat == MI_IPU_FORMAT_FP32) {
        pfClass  = (float *)(pOutputTensors->ptTensorData[0]);
    } else {
        ps16Class = (MI_S16*)(pOutputTensors->ptTensorData[0]);
    }
    pOutputTensors++;
    if (desc->astMI_OutputTensorDescs[2].eElmFormat == MI_IPU_FORMAT_FP32) {
        pfScore  = (float *)(pOutputTensors->ptTensorData[0]);
    } else {
        ps16Score = (MI_S16*)(pOutputTensors->ptTensorData[0]);
    }
    pOutputTensors++;
    if (desc->astMI_OutputTensorDescs[3].eElmFormat == MI_IPU_FORMAT_FP32) {
        pfDetect  = (float *)(pOutputTensors->ptTensorData[0]);
        s32DetectCount = int(*pfDetect);
    } else {
        ps16Detect = (MI_S16*)(pOutputTensors->ptTensorData[0]);
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


static void postUnknown(MI_IPU_SubNet_InputOutputDesc_t* desc, MI_IPU_Tensor_t* OutputTensors, std::string image_path, std::string modelFile) {
    std::vector<std::string> paths = split(image_path, "/");
    std::string imageName = paths.back();
    std::vector<std::string> model = split(modelFile, "/");
    std::string modelName = model.back();
    std::string outputName = "./output/unknown_" + modelName + "_" + imageName + ".txt";
    MI_IPU_Tensor_t* pOutputTensors;
    for (int i = 0; i < desc->u32OutputTensorCount; i++) {
        pOutputTensors = OutputTensors + i;
        DumpUnknown(outputName, &desc->astMI_OutputTensorDescs[i], pOutputTensors, i);
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

static void IPU_PrintOutputXOR(MI_IPU_SubNet_InputOutputDesc_t desc, MI_IPU_BatchInvokeParam_t stInvokeParam, MI_BOOL bBatchOneBuf)
{
    MI_U32 u32InputNum = desc.u32InputTensorCount;
    MI_U32 u32OutputNum = desc.u32OutputTensorCount;
    MI_U32 u32BatchNum = stInvokeParam.u32BatchN;

    if(bBatchOneBuf)
    {
        volatile MI_U32 u32XORValue = 0;
        MI_U8 *pu8XORValue[4]= {(MI_U8 *)&u32XORValue,(MI_U8 *)&u32XORValue+1,(MI_U8 *)&u32XORValue+2,(MI_U8 *)&u32XORValue+3 };
        MI_U32 u32Count = 0;

        for (MI_U32 idxOutputNum = 0; idxOutputNum < desc.u32OutputTensorCount; idxOutputNum++)
        {
            for (MI_U32 idxBatchNum = 0; idxBatchNum < stInvokeParam.u32BatchN; idxBatchNum++)
            {
                MI_U8 u8Data = 0;
                MI_U8 *pu8Data = (MI_U8 *)stInvokeParam.astArrayTensors[idxOutputNum + u32InputNum*u32BatchNum].ptTensorData[0] + idxBatchNum * IPU_CalcTensorSize(desc.astMI_OutputTensorDescs[idxOutputNum]);
                for(int i = 0; i < IPU_CalcTensorSize(desc.astMI_OutputTensorDescs[idxOutputNum]); i++)
                {
                    u8Data = *(pu8Data + i);
                    *pu8XORValue[u32Count%4] ^= u8Data;
                    u32Count++;
                }
            }
        }
        printf("All outputs XOR = 0x%08x\n", u32XORValue);
    }
    else
    {
        for (MI_U32 idxBatchNum = 0; idxBatchNum < stInvokeParam.u32BatchN; idxBatchNum++)
        {
            volatile MI_U32 u32XORValue = 0;
            MI_U8 *pu8XORValue[4]= {(MI_U8 *)&u32XORValue,(MI_U8 *)&u32XORValue+1,(MI_U8 *)&u32XORValue+2,(MI_U8 *)&u32XORValue+3 };
            MI_U32 u32Count = 0;

            for (MI_U32 idxOutputNum = 0; idxOutputNum < desc.u32OutputTensorCount; idxOutputNum++)
            {
                MI_U8 u8Data = 0;
                MI_U8 *pu8Data = (MI_U8 *)stInvokeParam.astArrayTensors[idxBatchNum * desc.u32OutputTensorCount + idxOutputNum + u32InputNum*u32BatchNum].ptTensorData[0];
                for(int i = 0; i < IPU_CalcTensorSize(desc.astMI_OutputTensorDescs[idxOutputNum]); i++)
                {
                    u8Data = *(pu8Data + i);
                    *pu8XORValue[u32Count%4] ^= u8Data;
                    u32Count++;
                }
            }

            printf("All outputs XOR Batch[%d] = 0x%08x\n", idxBatchNum, u32XORValue);
        }
    }
}


inline void tf_testInterpreterLabelImage(std::vector<std::string> images, std::string modelFile, std::string category, std::string mFormat, int number) {
    MI_U32 u32ChannelID;
    MI_IPU_OfflineModelStaticInfo_t OfflineModelInfo;
    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_IPU_BatchInvokeParam_t stInvokeParam;
    MI_IPU_RuntimeInfo_t stRuntimeInfo;
    int startIdx;
    MI_S32 ret;

    // 1. create device
    MI_SYS_Init(0);
    memset(&OfflineModelInfo, 0, sizeof(MI_IPU_OfflineModelStaticInfo_t));
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
    ret = IPUCreateChannel(&u32ChannelID, const_cast<char *>(modelFile.c_str()), number);
    if (ret != MI_SUCCESS)
    {
        MI_IPU_DestroyDevice();
        std::cerr << "Create IPU Channel failed! Error Code: " << ret << std::endl;
        return;
    }

    // 3. get input/output tensor
    memset(&desc, 0, sizeof(MI_IPU_SubNet_InputOutputDesc_t));
    memset(&stInvokeParam, 0, sizeof(MI_IPU_BatchInvokeParam_t));
    MI_IPU_GetInOutTensorDesc(u32ChannelID, &desc);
    PreProcessedData stProcessedData;
    stProcessedData.iResizeH = desc.astMI_InputTensorDescs[0].u32TensorShape[1];
    stProcessedData.iResizeW = desc.astMI_InputTensorDescs[0].u32TensorShape[2];
    stProcessedData.iResizeC = desc.astMI_InputTensorDescs[0].u32TensorShape[3];
    stProcessedData.mFormat = mFormat;
    stProcessedData.u32InputHeightAlignment = desc.astMI_InputTensorDescs[0].u32InputHeightAlignment;
    stProcessedData.u32InputWidthAlignment = desc.astMI_InputTensorDescs[0].u32InputWidthAlignment;

    for (int idx = 0; idx < images.size(); idx++) {
        //batch number pictures or last several pictures invoke together
        if (((idx % number) == (number - 1)) || (idx == (int)(images.size() -1))) {
            stInvokeParam.u32BatchN = idx % number + 1;
            MI_IPU_GetInputTensors2(u32ChannelID, &stInvokeParam);
            MI_IPU_GetOutputTensors2(u32ChannelID, &stInvokeParam);

            startIdx = (idx / number) * number;
            for (int cnt = startIdx; cnt <= idx; cnt++) {
                stProcessedData.pImagePath = images[cnt];
                stProcessedData.pdata = (MI_U8 *)stInvokeParam.astArrayTensors[cnt - startIdx].ptTensorData[0];
                if (mFormat == "RAWDATA_S16_NHWC") {
                    RAWDATA_S16_Input(&desc.astMI_InputTensorDescs[0], &stProcessedData);
                }
                else if (mFormat == "DUMP_RAWDATA") {
                    Dump_Rawdata_Input(&desc.astMI_InputTensorDescs[0], &stProcessedData);
                }
                else {
                    OpenCV_Image(&stProcessedData);
                }
            }

            auto time_0 = std::chrono::high_resolution_clock::now();
            ret = MI_IPU_Invoke2(u32ChannelID, &stInvokeParam, &stRuntimeInfo);
            auto time_1 = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(time_1 - time_0).count() / (1000.0 * 1000) << " ms" << std::endl;
            if (ret != MI_SUCCESS)
            {
                MI_IPU_DestroyCHN(u32ChannelID);
                MI_IPU_DestroyDevice();
                std::cerr << "IPU invoke failed! Error Code: " << ret << std::endl;
                return;
            }

            int outBase = stInvokeParam.u32BatchN * desc.u32InputTensorCount;
            IPU_PrintOutputXOR(desc, stInvokeParam, FALSE);
            for (int cnt = startIdx; cnt <= idx; cnt++) {
                // post-process
                int outBufStartIdx = outBase + (cnt - startIdx) * desc.u32OutputTensorCount;
                if (category == "Classification") {
                    postClassification(&desc, &stInvokeParam.astArrayTensors[outBufStartIdx], images[cnt], modelFile);
                }
                else if (category == "Detection") {
                    postDetection(&desc, &stInvokeParam.astArrayTensors[outBufStartIdx], images[cnt], modelFile);
                }
                else {
                    postUnknown(&desc, &stInvokeParam.astArrayTensors[outBufStartIdx], images[cnt], modelFile);
                }
            }

            // 5. put intput tensor
            MI_IPU_PutInputTensors2(u32ChannelID, &stInvokeParam);
            MI_IPU_PutOutputTensors2(u32ChannelID, &stInvokeParam);
        }
    }

    // 6. destroy channel/device
    MI_IPU_DestroyCHN(u32ChannelID);
    MI_IPU_DestroyDevice();
    MI_SYS_Exit(0);

    std::cout << "Done! Results in `output` folder." << std::endl;
}


static void showUsage() {
    std::cout << "Usage: ./prog_dla_dla_simulator [-i] [-m] [-c] ([--format] ) " << std::endl;
    std::cout << " -h, --help      show this help message and exit" << std::endl;
    std::cout << " -i, --images    path to validation image or images' folder" << std::endl;
    std::cout << " -m, --model     path to model file" << std::endl;
    std::cout << " -n, --number    max number of images when invoke once" << std::endl;
    std::cout << " -c, --category  indicate model category:" << std::endl;
    std::cout << "                 Classification / Detection / Unknown" << std::endl;
    std::cout << "     --format    model input format (Default is BGR):" << std::endl;
    std::cout << "                 BGR / RGB / BGRA / RGBA / YUV_NV12 / GRAY / RAWDATA_S16_NHWC / DUMP_RAWDATA" << std::endl;
}


int main(int argc, char* argv[]) {

    char* imagesPath = NULL;
    char* modelFile = NULL;
    char* category = NULL;
    int number = 0;
    std::string mFormat;
    int opt;
    int lopt {0};
    while (true) {
        int option_index = 0;
        static struct option long_options[] =
        {
            {"images",  required_argument, 0,  'i' },
            {"model",   required_argument, 0,  'm' },
            {"number",  required_argument, 0,  'n' },
            {"category",required_argument, 0,  'c' },
            {"help",    no_argument,       0,  'h' },
            {"format",  required_argument,&lopt,1  },
        };
        opt = getopt_long(argc, argv, "hi:m:c:n:", long_options, &option_index);
        if (opt == -1) {
            break;
        }
        switch (opt) {
            case 'i':
                imagesPath = optarg;
                break;
            case 'm':
                modelFile = optarg;
                break;
            case 'n':
                number = atoi(optarg);
                break;
            case 'c':
                category = optarg;
                break;
            case 0:
                switch (lopt) {
                    case 1:
                        mFormat = std::string(optarg);
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
    if ((imagesPath == NULL) || (modelFile == NULL) || (category == NULL)) {
        showUsage();
        return 0;
    }
    if (strcmp(category, "Classification") && strcmp(category, "Detection") && strcmp(category, "Unknown")) {
        std::cout << "model category only support `Classification / Detection / Unknown`" << std::endl;
        return 0;
    }
    if (mFormat.empty()) {
        mFormat = "BGR";
    }
    if (number < 1) {
        number = 1;
    }
    else if ((mFormat != "BGR") && (mFormat != "RGB") && (mFormat != "BGRA") && (mFormat != "RGBA") &&
            (mFormat != "YUV_NV12") && (mFormat != "GRAY") && (mFormat != "RAWDATA_S16_NHWC") &&
            (mFormat != "DUMP_RAWDATA")) {
        std::cout << "model input format only support `BGR / RGB / BGRA / RGBA / YUV_NV12 / GRAY / RAWDATA_S16_NHWC / DUMP_RAWDATA`" << std::endl;
        return 0;
    }
    std::string foldername {imagesPath};
    std::vector<std::string> images;
    parse_images_dir(foldername, images);
    renew_output();
    tf_testInterpreterLabelImage(images, std::string(modelFile), std::string(category), mFormat, number);

    return 0;
}
