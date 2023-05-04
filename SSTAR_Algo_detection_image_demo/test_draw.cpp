/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
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
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>
#include <dirent.h>
#include <getopt.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/time.h>
#include <chrono>

#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_ipu.h"
#include "mi_sys.h"

#include "sstar_detection_api.h"

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
#define INNER_MOST_ALIGNMENT (8)
#define DIVP_ALIGNMENT (4)
#define DIVP_YUV_ALIGNMENT (16)
#define SRC_IMAGE_SIZE (1920*1080*4)
#define DST_IMAGE_SIZE (1920*1080*4)
#define ST_DEFAULT_SOC_ID 0


void * manager;

struct PreProcessedData {
    std::string pImagePath;
    int iResizeH;
    int iResizeW;
    int iResizeC;
    std::string mFormat;
    MI_U8* pdata;
};

struct DetectionBBoxInfo {
    float xmin;
    float ymin;
    float xmax;
    float ymax;
    float score;
    int classID;
};

typedef struct{
    MI_S32 x;
    MI_S32 y;
    MI_S32 width;
    MI_S32 height;
    MI_FLOAT prob;
    MI_U32 class_id;
}ipu_rect;

//global 
ipu_rect rectA;
std::vector<ipu_rect> bboxes;

MI_S32 create_device_callback(MI_IPU_DevAttr_t* stDevAttr, char *pFirmwarePath)
{
	return MI_IPU_CreateDevice(stDevAttr, NULL, pFirmwarePath, 0);
}

void destory_device_callback()
{
	MI_IPU_DestroyDevice();
}

void detect_callback(std::vector<Box_t> results)
{
	bboxes.clear();
	if(results.size())
	{	
		for(int i = 0; i < results.size(); i++)
		{
			printf("--->[%d] %d %f [x %d,y %d,w %d,h %d]\n", i, results[i].class_id, results[i].score, results[i].x, results[i].y, results[i].width, results[i].height);
			
			rectA.x = results[i].x;
			rectA.y = results[i].y;
			rectA.width = results[i].width;
			rectA.height = results[i].height;
            rectA.prob = results[i].score;
            rectA.class_id = results[i].class_id;
            bboxes.push_back(rectA);
		}
	}
	else
	{
		std::cout<< "*********no results*******:\n" << std::endl;
	}

}


void parse_images_dir(const std::string& base_path, std::vector<std::string>& file_path)
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

DetectionInfo_t detection_info_yuv = 
{
    "/config/dla/ipu_lfw.bin",
    "./model2/sypcd2480509_fixed.sim_sgsimg.img",
    0.5, //threshold
    {800, 480}, //转成显示的坐标
    create_device_callback,
    detect_callback,
	destory_device_callback,
};

DetectionInfo_t detection_info_argb = 
{
    "/config/dla/ipu_lfw.bin",
    "./sypfa5.480302_fixed.sim_sgsimg.img",
    0.4, //threshold
    {800, 480}, //转成显示的坐标
    create_device_callback,
    detect_callback,
	destory_device_callback,
};

static float calcIOU(ipu_rect rectA1, ipu_rect rectB)
{

	if (rectA1.x > rectB.x + rectB.width) { return 0.; }

	if (rectA1.y > rectB.y + rectB.height) { return 0.; }

	if ((rectA1.x + rectA1.width) < rectB.x) { return 0.; }

	if ((rectA1.y + rectA1.height) < rectB.y) { return 0.; }

	float colInt = MIN(rectA1.x + rectA1.width, rectB.x + rectB.width) - MAX(rectA1.x, rectB.x);

	float rowInt = MIN(rectA1.y + rectA1.height, rectB.y + rectB.height) - MAX(rectA1.y, rectB.y);

	float intersection = colInt * rowInt;

	float areaA = rectA1.width * rectA1.height;

	float areaB = rectB.width * rectB.height;

	float intersectionPercent = intersection / (areaA + areaB - intersection);

	return intersectionPercent;
}

MI_S32 vpe_OpenSourceFile(const char *pFileName, int *pSrcFd)
{
    int src_fd = open(pFileName, O_RDONLY);
    printf("----filename %s\n", pFileName);
    if (src_fd < 0)
    {
        perror("open");
        return -1;
    }
    *pSrcFd = src_fd;

    return TRUE;
}

MI_S32 vpe_GetOneFrame(int srcFd, char *pData, int yuvSize)
{
    off_t current = lseek(srcFd,0L, SEEK_CUR);
    off_t end = lseek(srcFd,0L, SEEK_END);

    if((end - current) == 0 || (end - current) < yuvSize)
    {
        lseek(srcFd,0,SEEK_SET);
        current = lseek(srcFd,0,SEEK_CUR);
    }

    lseek(srcFd, current, SEEK_SET);
    if (read(srcFd, pData, yuvSize) == yuvSize)
    {
        return 1;
    }

    return 0;
}

static void Dump_Rawdata_Input(MI_IPU_TensorDesc_t* desc, PreProcessedData* pstPreProcessedData) {
    std::ifstream ifs(pstPreProcessedData->pImagePath, std::ios::binary);
    
    ifs.seekg(0, std::ios_base::end);
    if (ifs.tellg() != desc->s32AlignedBufSize)
    {
        std::cerr << "[Error] Got length of " << pstPreProcessedData->pImagePath << ": " << ifs.tellg()
                  << " != s32AlignedBufSize: " << desc->s32AlignedBufSize << std::endl;
        ifs.clear();
        ifs.close();
        return;
    }
    ifs.seekg(0, std::ios_base::beg);
    while (!ifs.eof()) {
        ifs.read(reinterpret_cast<char*>(pstPreProcessedData->pdata), desc->s32AlignedBufSize);
    }
    std::cout << " " << desc->s32AlignedBufSize << std::endl;
    MI_SYS_FlushInvCache(pstPreProcessedData->pdata, desc->s32AlignedBufSize);
    ifs.clear();
    ifs.close();
}

void OpenCV_Image(PreProcessedData* pstPreProcessedData)
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
		if(pVirsrc)
		{
			
		}
		else
		{
			exit(1);
		}
        imageStride = ALIGN_UP(pstPreProcessedData->iResizeW * 4, DIVP_YUV_ALIGNMENT);
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
        imageStride = ALIGN_UP(pstPreProcessedData->iResizeW * 4, DIVP_YUV_ALIGNMENT);
        imageSize = pstPreProcessedData->iResizeH * imageStride;
        for (int i = 0; i < pstPreProcessedData->iResizeH; i++) {
            memcpy(pVirsrc, pSrc, pstPreProcessedData->iResizeW * 4);
            pVirsrc += imageStride;
            pSrc += pstPreProcessedData->iResizeW * 4;
        }
    }
    else if ((pstPreProcessedData->mFormat == "YUV_NV12") || (pstPreProcessedData->mFormat == "GRAY")) {
        int Align_height = ALIGN_DOWN(sample_resized.rows, 2);
        int Align_width = ALIGN_DOWN(sample_resized.cols, 2);
        imageSize = Align_height * Align_width * 3 / 2;
        cv::Mat dst = sample_resized(cv::Range(0, Align_height), cv::Range(0, Align_width)).clone();
        cv::cvtColor(dst, sample, cv::COLOR_BGR2YUV_YV12);
        cv::Mat v = sample(cv::Range(Align_height, Align_height + Align_height / 4), cv::Range(0, Align_width)).clone();
        cv::Mat u = sample(cv::Range(Align_height + Align_height / 4, Align_height + Align_height / 2), cv::Range(0, Align_width)).clone();
        for (int j = Align_height * Align_width, i = 0; j < imageSize; j += 2, i++) {
            sample.at<uchar>(j / Align_width, j % Align_width) = u.at<uchar>(i / Align_width, i % Align_width);
            sample.at<uchar>((j + 1) / Align_width, (j + 1) % Align_width) = v.at<uchar>(i / Align_width, i % Align_width);
        }
        MI_U8* pSrc = sample.data;
        MI_U8* pVirsrc = pstPreProcessedData->pdata;
        imageStride = ALIGN_UP(pstPreProcessedData->iResizeW, DIVP_YUV_ALIGNMENT);
        imageSize = pstPreProcessedData->iResizeH * imageStride;
        for (int i = 0; i < sample.rows; i++) {
            memcpy(pVirsrc, pSrc, pstPreProcessedData->iResizeW);
            pVirsrc += imageStride;
            pSrc += pstPreProcessedData->iResizeW;
        }
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


void renew_output() {
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

void showUsage() {
    std::cout << "Usage: ./prog_dla_label_image --images --input_format" << std::endl;
    std::cout << " -h, --help      show this help message and exit" << std::endl;
    std::cout << " -i, --images    path to validation image or images' folder" << std::endl;
	std::cout << " -s, --input_format      select data input format: argb8888 / yuv, default=argb8888" << std::endl;
    std::cout << "     --format    model input format (Default is BGRA):" << std::endl;
    std::cout << "                 BGR / RGB / BGRA / RGBA / YUV_NV12 / GRAY / RAWDATA_S16_NHWC" << std::endl;
}

int main(int argc, char* argv[]) {
	//run  ./prog_dla_label_image --images ./images --input_format rgb
    char* imagesPath = NULL;
	char* labelsPath = NULL;
	char* input_format = NULL;
    std::string mFormat;
    int opt;
    int lopt {0};
    while (true) {
        int option_index = 0;
        static struct option long_options[] =
        {
            {"images",        required_argument, 0,  'i' },
			{"labels",        required_argument, 0,  'l' },
			{"input_format",  required_argument, 0,  's' },
            {"help",          no_argument,       0,  'h' },
            {"format",        required_argument, 0,  'f' },
        };
        opt = getopt_long(argc, argv, "hi:m:c:", long_options, &option_index);
        if (opt == -1) {
            break;
        }
        switch (opt) {
            case 'i':
                imagesPath = optarg;
                break;
			case 's':
                input_format = optarg;
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
    if (imagesPath == NULL) {
        std::cout <<  "if (imagesPath == NULL) { "<< std::endl;
        showUsage();
        return 0;
    }
	if (input_format == NULL) {
        std::cout <<  "if (input_format == NULL) { "<< std::endl;
        input_format = "argb8888";
    }
	if (mFormat.empty()) {
        mFormat = "BGRA";
    }
    else if ((mFormat != "BGR") && (mFormat != "RGB") && (mFormat != "BGRA") && (mFormat != "RGBA") &&
            (mFormat != "YUV_NV12") && (mFormat != "GRAY") && (mFormat != "RAWDATA_S16_NHWC")) {
        std::cout << "model input format only support `BGR / RGB / BGRA / RGBA / YUV_NV12 / GRAY / RAWDATA_S16_NHWC`" << std::endl;
        return 0;
    }
	MI_SYS_Init(0);
    MI_U32 u32ChannelID;
    MI_IPU_SubNet_InputOutputDesc_t desc;

	// set threshold
	const size_t num_threshold = 11;
    float thresholds[num_threshold];
    for (int i_threshold = 1; i_threshold < num_threshold - 1; i_threshold++)
    {
        thresholds[i_threshold] = 1.0 / (num_threshold - 1) * (i_threshold);
    }
    thresholds[0] = 0.05;
    thresholds[10] = 0.99;

    int all_gnums = 0;
    int g_TP_nums[num_threshold] = {0};
    int g_TN_nums[num_threshold] = {0};
    int g_FP_nums[num_threshold] = {0};
    int g_FN_nums[num_threshold] = {0};
    char c;
	
	// set image input 
	int src_fd;
	MI_S32 s32Ret;
	std::string foldername {imagesPath};
	std::vector<std::string> images;
	parse_images_dir(foldername, images);
    std::cout << "found " << images.size() << " images!" << std::endl;
	
	// set pVirAddr
	MI_SYS_BufInfo_t stBufInfo;
	memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
	
	MI_S32 ret = MI_SUCCESS;
	MI_U16 u16SocId = ST_DEFAULT_SOC_ID;
	MI_U32 srcBuffSize = 800*480*4;
	MI_PHY phySrcBufAddr = 0;
	void *pVirSrcBufAddr = NULL;
	ipu_rect rectC;
	float iou = 0.0;

	std::vector<float> values;
	
	ret = MI_SYS_MMA_Alloc(u16SocId, NULL, srcBuffSize, &phySrcBufAddr);
	if(ret != MI_SUCCESS)
	{
		printf("alloc src buff failed\n");
		return -1;
	}
	ret = MI_SYS_Mmap(phySrcBufAddr, srcBuffSize, &pVirSrcBufAddr, TRUE);
	if(ret != MI_SUCCESS)
	{
		printf("mmap src buff failed\n");
		return -1;
	}
	memset(pVirSrcBufAddr, 0, srcBuffSize);
	
	// enter sdk
	getDetectionManager(&manager);
	if(0 == strcmp(input_format, "rgb")){
		initDetection(manager, &detection_info_argb);
        printf("the input format is BGRA \n");
	}
	else if(0 == strcmp(input_format, "argb8888")){
		initDetection(manager, &detection_info_argb);
        printf("the input format is ARGB8888 \n");
	}
	else{
		initDetection(manager, &detection_info_yuv);
        printf("the input format is YUV_NV12 \n");
	}
	startDetect(manager);
	
	PreProcessedData stProcessedData;
	stProcessedData.iResizeH = 480;
	stProcessedData.iResizeW = 800;
	stProcessedData.iResizeC = 4;
	stProcessedData.mFormat = mFormat;
	stProcessedData.pdata =(MI_U8 *) pVirSrcBufAddr;

    std::string time_str = std::to_string(long(time(0)));
	for (int idx = 0; idx < images.size(); idx++) 
	{
        
            cv::Mat src;
            if(0 == strcmp(input_format, "rgb")){
                stProcessedData.pImagePath = images[idx];
                printf("the image path is %s \n",stProcessedData.pImagePath.c_str());
                OpenCV_Image(&stProcessedData);
                stBufInfo.stFrameData.u32BufSize = srcBuffSize;
                stBufInfo.stFrameData.pVirAddr[0] = pVirSrcBufAddr;
                stBufInfo.stFrameData.phyAddr[0] = phySrcBufAddr;
                stBufInfo.eBufType = E_MI_SYS_BUFDATA_FRAME;
                src = cv::imread(images[idx]);
    
                
            }
            else if(0 == strcmp(input_format, "argb8888")){
                s32Ret = vpe_OpenSourceFile(images[idx].c_str(), &src_fd);
                if(s32Ret < 0)
                {
                    printf("!!!!!!!!!open file[%s] fail\n", images[idx].c_str());
                    continue;
                }

                std::cout  << "[" << idx << "] " << "processing " << images[idx] << std::endl;

                vpe_GetOneFrame(src_fd, (char *)pVirSrcBufAddr, srcBuffSize);
                close(src_fd);
                MI_SYS_FlushInvCache(pVirSrcBufAddr, srcBuffSize);
                stBufInfo.eBufType					= E_MI_SYS_BUFDATA_FRAME;
                stBufInfo.stFrameData.u16Width 		= 800;
                stBufInfo.stFrameData.u16Height		= 480;
                stBufInfo.stFrameData.pVirAddr[0]	= pVirSrcBufAddr;
                stBufInfo.stFrameData.phyAddr[0]	= phySrcBufAddr;
                stBufInfo.stFrameData.u32BufSize	= srcBuffSize;
                
                cv::Mat srcMat(480, 800 , CV_8UC4, (unsigned char *)stBufInfo.stFrameData.pVirAddr[0]);
                src = srcMat;
            }
            else{
                s32Ret = vpe_OpenSourceFile(images[idx].c_str(), &src_fd);
                if(s32Ret < 0)
                {
                    printf("!!!!!!!!!open file[%s] fail\n", images[idx].c_str());
                    continue;
                }
                printf("---enter here\n");
                vpe_GetOneFrame(src_fd, (char *)pVirSrcBufAddr, srcBuffSize);
                stBufInfo.eBufType					= E_MI_SYS_BUFDATA_FRAME;
                stBufInfo.stFrameData.u16Width 		= 800;
                stBufInfo.stFrameData.u16Height		= 480;
                stBufInfo.stFrameData.pVirAddr[0]	= pVirSrcBufAddr;
                stBufInfo.stFrameData.phyAddr[0]	= phySrcBufAddr;
                stBufInfo.stFrameData.u32BufSize	= srcBuffSize;
                close(src_fd);

                int pixel_h = 480;
                int pixel_w = 800;
                cv::Mat yuv;
                yuv.create(pixel_h*1.5,pixel_w,CV_8UC1);
                memcpy(yuv.data,(unsigned char *)stBufInfo.stFrameData.pVirAddr[0], pixel_h*pixel_w*3/2*sizeof(unsigned char));
                cvtColor(yuv, src, cv::COLOR_YUV2BGR_NV12);
                
            }
        // doSnDetect(manager, &stBufInfo);
        //doDetectPerson(manager, &stBufInfo);
		doDetect(manager, &stBufInfo);
		
        printf("---->image path: %s\n",images[idx].c_str());

        for(int i = 0; i < bboxes.size(); i++)
		{

			cv::Rect rect(
                bboxes[i].x,
                bboxes[i].y,
                bboxes[i].width,
                bboxes[i].height);
			
            cv::Scalar frame_color(0,0,255);

            std::string r_label_text = "";
            if (bboxes[i].class_id == 0) {
                r_label_text = "person";
                frame_color = cv::Scalar(0,0,255);
            }
            else if (bboxes[i].class_id == 1) {
                r_label_text = "cat";
                frame_color = cv::Scalar(255,0,0);
            }
            else if (bboxes[i].class_id == 2) {
                r_label_text = "dog";
                frame_color = cv::Scalar(0,255,0);
            }
            else {
                frame_color = cv::Scalar(0,0,0);
                r_label_text = "other";
            }
            
            cv::rectangle(src, rect, frame_color, 1, cv::LINE_8,0);
            // printf("--->do rect x,y,w,h%d,%d,%d,%d\n",bboxes[i].x,bboxes[i].y,bboxes[i].width,bboxes[i].height);

            cv::putText(
                src, 
                r_label_text + "0." + std::__cxx11::to_string(int(bboxes[i].prob * 100)),
                cv::Point(
                    float(bboxes[i].x),
                    float(bboxes[i].y)),
                cv::FONT_HERSHEY_SIMPLEX,
                0.6,
                frame_color,
                2);
		}

        std::string output_dir = "./output/" + time_str + "/";
        mkdir(output_dir.c_str(), 0777);
        std::string output_image_dir = output_dir + "images/";
        mkdir(output_image_dir.c_str(), 0777);
        std::string output_txt_dir = output_dir + "txts/";
        mkdir(output_txt_dir.c_str(), 0777);


        std::string image_base_name = images[idx].substr(images[idx].rfind("/") + 1);

        size_t last_dot_pos = image_base_name.rfind(".");

        std::string result_txt_base_name = image_base_name.replace(last_dot_pos, image_base_name.size() - last_dot_pos, ".txt");

        std::string result_image_base_name = image_base_name.replace(last_dot_pos, image_base_name.size() - last_dot_pos, ".png");

		std::string image_save_path = output_image_dir + image_base_name;
        std::cout << "result_image_save_path: " << result_image_base_name << std::endl;
        std::cout << "image_save_path: " << image_save_path << std::endl;
		cv::imwrite(image_save_path, src);
		src.release();
		
	}

	MI_SYS_FlushInvCache(pVirSrcBufAddr, srcBuffSize);
	MI_SYS_Munmap(pVirSrcBufAddr, srcBuffSize);
	printf("the doDetectPerson is ok \n");

    return 0;
}
