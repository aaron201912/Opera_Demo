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
#include <cstdlib>
#include <cstdint>
#include <getopt.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_ipu.h"
#include "mi_sys.h"

#define ST_DEFAULT_SOC_ID 0
#define DIVP_YUV_ALIGNMENT (16)
#ifdef ALIGN_UP
#undef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#else
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(val, alignment) (((val) / (alignment)) * (alignment))
#endif

#define MAX_PATH_STRLEN 256

#define MAX_NUM_OUT_TENSOR 3

#define INNER_MOST_ALIGNMENT 1

typedef struct
{
    MI_U32 width;
    MI_U32 height;
    MI_IPU_ELEMENT_FORMAT format;
} InputAttr_t;

typedef struct
{
    void *p_vir_addr;
    MI_PHY phy_addr;
    MI_U32 buf_size;
    MI_U64 pts;
} ALGO_Input_t;

typedef struct
{
    void *data[MAX_NUM_OUT_TENSOR];
    MI_FLOAT scalar[MAX_NUM_OUT_TENSOR];
    MI_IPU_ELEMENT_FORMAT out_format;
    MI_U32 net_in_w;
    MI_U32 net_in_h;
    MI_U32 net_out_w[MAX_NUM_OUT_TENSOR];
    MI_U32 net_out_h[MAX_NUM_OUT_TENSOR];
    MI_U32 net_out_c[MAX_NUM_OUT_TENSOR];
    MI_U32 num_out_layers;
    MI_S32 num_classes;
    MI_FLOAT conf_threshold;
    MI_FLOAT nms_threshold;
    MI_S32 reg_max;
} DetDecodeInfo;

typedef struct BoxInfo
{
    float x1;
    float y1;
    float x2;
    float y2;
    float score;
    int label;
    unsigned long long pts;
} BoxInfo;

std::vector<char *> coco_classes =
    {
        "person",
        "bicycle",
        "car",
        "motorcycle",
        "airplane",
        "bus",
        "train",
        "truck",
        "boat",
        "traffic light",
        "fire hydrant",
        "stop sign",
        "parking meter",
        "bench",
        "bird",
        "cat",
        "dog",
        "horse",
        "sheep",
        "cow",
        "elephant",
        "bear",
        "zebra",
        "giraffe",
        "backpack",
        "umbrella",
        "handbag",
        "tie",
        "suitcase",
        "frisbee",
        "skis",
        "snowboard",
        "sports ball",
        "kite",
        "baseball bat",
        "baseball glove",
        "skateboard",
        "surfboard",
        "tennis racket",
        "bottle",
        "wine glass",
        "cup",
        "fork",
        "knife",
        "spoon",
        "bowl",
        "banana",
        "apple",
        "sandwich",
        "orange",
        "broccoli",
        "carrot",
        "hot dog",
        "pizza",
        "donut",
        "cake",
        "chair",
        "couch",
        "potted plant",
        "bed",
        "dining table",
        "toilet",
        "tv",
        "laptop",
        "mouse",
        "remote",
        "keyboard",
        "cell phone",
        "microwave",
        "oven",
        "toaster",
        "sink",
        "refrigerator",
        "book",
        "clock",
        "vase",
        "scissors",
        "teddy bear",
        "hair drier",
        "toothbrush"};

void showUsage(char *exec_name, float default_threshold)
{
    std::cout << "Usage: ./" << exec_name << " [-i/--images] ([-h/--help] [-t/--threshold]) " << std::endl;
    std::cout << "-i, --images    : required; path to input images (glob expression supported)" << std::endl;
    std::cout << "-m, --model     : optional; path to model img, ignore while using builtin model" << std::endl;
    std::cout << "-h, --help      : optional; show this help message and exit" << std::endl;
    std::cout << "-t, --threshold : optional; set detection threshold(0.0~1.0), default=" << default_threshold << std::endl;
    std::cout << std::endl;
}

void parseCmdOptions(
    int argc, char *argv[],
    std::string &in_images,
    char *model_path,
    float *threshold)
{

    struct option options_cfg[] = {
        {"help", no_argument, NULL, 'h'},
        {"images", required_argument, NULL, 'i'},
        {"threshold", optional_argument, NULL, 't'},
        {"model", optional_argument, NULL, 'm'},
    };

    int current_opt_index = 0;
    int current_opt_id;

    float default_threshold = *threshold;
    while (true)
    {
        current_opt_id = getopt_long(argc, argv, "hi:t:m:", options_cfg, &current_opt_index);
        if (current_opt_id == -1)
        {
            break;
        }

        switch (current_opt_id)
        {
        case 'i':
            in_images = optarg;
            break;
        case 'm':
            if (strlen(optarg) >= MAX_PATH_STRLEN)
            {
                std::cout << optarg << std::endl;
                std::cerr << "model path is too long (max=" << MAX_PATH_STRLEN << ")" << std::endl;
                exit(__LINE__);
            }
            strcpy(model_path, optarg);
            break;
        case 't':
            std::cout << "-t= " << optarg << std::endl;
            *threshold = atof(optarg);
            break;
        case 'h':
            showUsage(argv[0], default_threshold);
            exit(0);
        default:
            showUsage(argv[0], default_threshold);
            exit(1);
        }
    }

    if (in_images.size() != 0 && *threshold >= 0.0 && *threshold <= 1.0)
    {
        std::cout << "demo_args: in_images=" << in_images << "; model_path=" << model_path << "; threshold=" << *threshold << std::endl;
    }
    else
    {
        showUsage(argv[0], default_threshold);
        exit(1);
    }
}

MI_S32 vpe_OpenSourceFile(const char *pFileName, int *pSrcFd)
{
    int src_fd = open(pFileName, O_RDONLY);
    if (src_fd < 0)
    {
        return src_fd;
    }
    *pSrcFd = src_fd;

    return MI_SUCCESS;
}

MI_S32 vpe_GetOneFrame(int srcFd, char *pData, MI_U32 yuvSize)
{
    off_t end = lseek(srcFd, 0, SEEK_END);
    off_t start = lseek(srcFd, 0, SEEK_SET);

    if (end - start >= yuvSize)
    {
        if (end - start > yuvSize)
        {
            printf("file content(%d) is more than buffer size(%d)\n", end - start, yuvSize);
        }

        int read_bytes = read(srcFd, pData, yuvSize);

        return read_bytes;
    }

    return -1;
}

void initDetBuffer(MI_U16 u16SocId, const InputAttr_t *input_attr, ALGO_Input_t *algo_input)
{

    memset(algo_input, 0, sizeof(ALGO_Input_t));

    MI_U32 in_height = input_attr->height;
    MI_U32 in_width = input_attr->width;
    MI_IPU_ELEMENT_FORMAT element_format = input_attr->format;
    MI_U32 buffer_size = 0;
    MI_U32 row_stride = 0;

    if (element_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_U8 ||
        element_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_INT8 ||
        element_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_GRAY)
    {
        row_stride = in_width;
        buffer_size = row_stride * in_height;
    }
    else if (element_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_NV12)
    {
        row_stride = ALIGN_UP(in_width, DIVP_YUV_ALIGNMENT);
        buffer_size = (MI_U32)(row_stride * in_height * 1.5);
    }
    else if (element_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_INT16)
    {
        row_stride = in_width * 2;
        buffer_size = row_stride * in_height;
    }
    else if (element_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_INT32 ||
             element_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_FP32 ||
             element_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_ARGB8888 ||
             element_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_ABGR8888)
    {
        row_stride = in_width * 4;
        buffer_size = row_stride * in_height;
    }
    else
    {
        std::cout << "unknown element format" << element_format << std::endl;
        exit(__LINE__);
    }

    MI_PHY phySrcBufAddr = 0;
    void *pVirSrcBufAddr = NULL;

    if (algo_input->buf_size != 0)
    {
        std::cout << "algo_input should refer to an uninitialized buffer !" << std::endl;
    }

    MI_S32 ret = MI_SYS_MMA_Alloc(u16SocId, NULL, buffer_size, &phySrcBufAddr);
    if (ret != MI_SUCCESS)
    {
        printf("alloc src buff failed\n");
        exit(__LINE__);
    }

    ret = MI_SYS_Mmap(phySrcBufAddr, buffer_size, &pVirSrcBufAddr, TRUE);
    if (ret != MI_SUCCESS)
    {
        printf("mmap src buff failed\n");
        exit(__LINE__);
    }

    memset(pVirSrcBufAddr, 0, buffer_size);

    algo_input->buf_size = buffer_size;
    algo_input->p_vir_addr = pVirSrcBufAddr;
    algo_input->phy_addr = phySrcBufAddr;
}

bool endsWith(std::string const &full_str, std::string const &ending)
{
    if (full_str.length() >= ending.length())
    {
        return (0 == full_str.compare(full_str.length() - ending.length(), ending.length(), ending));
    }
    else
    {
        return false;
    }
}

void YV12toNV12(const cv::Mat &input, cv::Mat &output)
{
    int width = input.cols;
    int height = input.rows * 2 / 3;
    int stride = (int)input.step[0]; // Rows bytes stride - in most cases equal to width

    input.copyTo(output);

    // Y Channel
    //  YYYYYYYYYYYYYYYY
    //  YYYYYYYYYYYYYYYY
    //  YYYYYYYYYYYYYYYY
    //  YYYYYYYYYYYYYYYY
    //  YYYYYYYYYYYYYYYY
    //  YYYYYYYYYYYYYYYY

    // V Input channel
    //  VVVVVVVV
    //  VVVVVVVV
    //  VVVVVVVV
    cv::Mat inV = cv::Mat(cv::Size(width / 2, height / 2), CV_8UC1, (unsigned char *)input.data + stride * height, stride / 2); // Input V color channel (in YV12 V is above U).

    // U Input channel
    //  UUUUUUUU
    //  UUUUUUUU
    //  UUUUUUUU
    cv::Mat inU = cv::Mat(cv::Size(width / 2, height / 2), CV_8UC1, (unsigned char *)input.data + stride * height + (stride / 2) * (height / 2), stride / 2); // Input V color channel (in YV12 U is below V).

    for (int row = 0; row < height / 2; row++)
    {
        for (int col = 0; col < width / 2; col++)
        {
            output.at<uchar>(height + row, 2 * col) = inU.at<uchar>(row, col);
            output.at<uchar>(height + row, 2 * col + 1) = inV.at<uchar>(row, col);
        }
    }
}

void fillDetBuffer(std::string input_path, InputAttr_t *input_attr, const ALGO_Input_t *algo_input)
{
    MI_U8 *pVirsrc = (MI_U8 *)algo_input->p_vir_addr;
    MI_U32 in_width = input_attr->width;
    MI_U32 in_height = input_attr->height;
    MI_IPU_ELEMENT_FORMAT in_format = input_attr->format;
    std::cout << "processing " << input_path << "..." << std::endl;

    if (endsWith(input_path, std::string(".jpg")) || endsWith(input_path, std::string(".png")) ||
        endsWith(input_path, std::string(".bmp")))
    {
        cv::Mat raw_bgr = cv::imread(input_path);

        cv::Mat resized_bgr;

        if (in_width != raw_bgr.cols || in_height != raw_bgr.rows)
        {
            cv::resize(raw_bgr, resized_bgr, cv::Size(in_width, in_height), 0, 0, cv::INTER_LINEAR);
        }
        else
        {
            resized_bgr = raw_bgr;
        }

        if (in_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_ARGB8888)
        {
            cv::Mat resized_bgra;
            cv::cvtColor(resized_bgr, resized_bgra, cv::COLOR_BGR2BGRA);

            MI_S16 row_stride = ALIGN_UP(in_width * 4, DIVP_YUV_ALIGNMENT);
            MI_S16 buffer_size = in_height * row_stride;
            for (int i_row = 0; i_row < in_height; i_row++)
            {
                memcpy(pVirsrc, resized_bgra.ptr<uchar>(i_row), in_width * 4);
                pVirsrc += row_stride;
            }
        }
        else if (in_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_ABGR8888)
        {
            cv::Mat resized_rgba;
            cv::cvtColor(resized_bgr, resized_rgba, cv::COLOR_BGR2RGBA);

            MI_S16 row_stride = ALIGN_UP(in_width * 4, DIVP_YUV_ALIGNMENT);
            MI_S16 buffer_size = in_height * row_stride;
            for (int i_row = 0; i_row < in_height; i_row++)
            {
                memcpy(pVirsrc, resized_rgba.ptr<uchar>(i_row), in_width * 4);
                pVirsrc += row_stride;
            }
        }
        else if (in_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_NV12)
        {

            MI_S16 align_height = ALIGN_DOWN(resized_bgr.rows, 2);
            MI_S16 align_width = ALIGN_DOWN(resized_bgr.cols, 2);
            MI_S16 num_cells = align_height * align_width * 3 / 2;
            cv::Mat align_bgr = resized_bgr(cv::Range(0, align_height), cv::Range(0, align_width)).clone();
            cv::Mat align_yv12;
            cv::cvtColor(align_bgr, align_yv12, cv::COLOR_BGR2YUV_YV12);
            cv::Mat align_yuv = align_yv12.clone();
            YV12toNV12(align_yv12, align_yuv);

            // cv::Mat v = align_yuv(cv::Range(align_height, align_height + align_height / 4), cv::Range(0, align_width)).clone();
            // cv::Mat u = align_yuv(cv::Range(align_height + align_height / 4, align_height + align_height / 2), cv::Range(0, align_width)).clone();

            // std::cout << "u[0][0]=" << int(align_yuv.at<u_char>(481, 0)) << std::endl;
            // std::cout << "u[0][1]=" << int(align_yuv.at<u_char>(481, 1)) << std::endl;

            // std::cout << "v[0][0]=" << int(align_yuv.at<u_char>(481+120, 0)) << std::endl;
            // std::cout << "v[0][1]=" << int(align_yuv.at<u_char>(481+120, 1)) << std::endl;

            // for (int j = align_height * align_width, i = 0; j < num_cells; j += 2, i++) {
            //     align_yuv.at<uchar>(j / align_width, j % align_width) = u.at<uchar>(i / align_width, i % align_width);
            //     align_yuv.at<uchar>((j + 1) / align_width, (j + 1) % align_width) = v.at<uchar>(i / align_width, i % align_width);
            // }

            // std::cout << "u[0][0]=" << int(align_yuv.at<u_char>(481, 0)) << std::endl;
            // std::cout << "u[0][1]=" << int(align_yuv.at<u_char>(481, 1)) << std::endl;

            // std::cout << "v[0][0]=" << int(align_yuv.at<u_char>(481+120, 0)) << std::endl;
            // std::cout << "v[0][1]=" << int(align_yuv.at<u_char>(481+120, 1)) << std::endl;

            MI_U8 *pSrc = align_yuv.data;
            int row_stride = ALIGN_UP(in_width, DIVP_YUV_ALIGNMENT);
            for (int i = 0; i < align_yuv.rows; i++)
            {
                memcpy(pVirsrc, pSrc, in_width);
                pVirsrc += row_stride;
                pSrc += in_width;
            }
        }
        else
        {
            std::cerr << "not supported element format " << in_format
                      << " for input " << input_path << std::endl;
            exit(__LINE__);
        }
    }
    else if (endsWith(input_path, std::string(".argb")) || endsWith(input_path, std::string(".bgra")) ||
             (endsWith(input_path, std::string(".bin")) && (in_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_ARGB8888)))
    {
        if (in_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_ARGB8888)
        {
            int src_fd;
            MI_S32 s32Ret = vpe_OpenSourceFile(input_path.c_str(), &src_fd);
            if (s32Ret != MI_SUCCESS)
            {
                printf("open file[%s] failed\n", input_path.c_str());
                exit(__LINE__);
            }

            MI_S32 row_stride = ALIGN_UP(in_width * 4, DIVP_YUV_ALIGNMENT);
            MI_S32 buffer_size = in_height * row_stride;

            int num_write_bytes = vpe_GetOneFrame(src_fd, (char *)pVirsrc, buffer_size);
            if (num_write_bytes == buffer_size)
            {
                close(src_fd);
                MI_SYS_FlushInvCache(pVirsrc, buffer_size);
            }
            else
            {
                printf("file2buffer failed, expected %d bytes, %d bytes written\n", buffer_size, num_write_bytes);
                exit(__LINE__);
            }
        }
        else
        {
            std::cout << "not supported element format " << in_format
                      << " for input " << input_path << std::endl;
            exit(__LINE__);
        }
    }
    else if (endsWith(input_path, std::string(".yuv")) ||
             (endsWith(input_path, std::string(".bin")) && (in_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_NV12)))
    {
        if (in_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_NV12)
        {
            int src_fd;
            MI_S32 s32Ret = vpe_OpenSourceFile(input_path.c_str(), &src_fd);
            if (s32Ret < 0)
            {
                printf("open file[%s] fail\n", input_path.c_str());
                exit(__LINE__);
            }

            MI_S32 row_stride = ALIGN_UP(in_width, DIVP_YUV_ALIGNMENT);
            MI_S32 buffer_size = in_height * row_stride * 3 / 2;
            // MI_S16 buffer_size = in_height * in_width * 4;

            int num_write_bytes = vpe_GetOneFrame(src_fd, (char *)pVirsrc, buffer_size);
            if (num_write_bytes == buffer_size)
            {
                close(src_fd);
                MI_SYS_FlushInvCache(pVirsrc, buffer_size);
            }
            else
            {
                printf("file2buffer failed, expected %d bytes, %d bytes written\n", buffer_size, num_write_bytes);
                exit(__LINE__);
            }
        }
        else
        {
            std::cout << "not supported element format " << in_format
                      << " for input " << input_path << std::endl;
            exit(__LINE__);
        }
    }
}

void dumpDetBuffer(InputAttr_t *input_attr, const ALGO_Input_t *algo_input, cv::Mat &dst_mat)
{
    MI_U8 *pVirsrc = (MI_U8 *)algo_input->p_vir_addr;
    MI_U32 in_width = input_attr->width;
    MI_U32 in_height = input_attr->height;
    MI_IPU_ELEMENT_FORMAT in_format = input_attr->format;

    if (dst_mat.rows == in_height && dst_mat.cols == in_width && dst_mat.channels() == 3)
    {
        // do nothing
    }
    else
    {
        if (dst_mat.empty() == false)
        {
            dst_mat.release();
        }

        dst_mat.create(in_height, in_width, CV_8UC3);
    }

    if (in_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_ARGB8888)
    {
        cv::Mat bgra(in_height, in_width, CV_8UC4);
        memcpy(bgra.data, (unsigned char *)pVirsrc, in_height * in_width * 4 * sizeof(unsigned char));
        cv::cvtColor(bgra, dst_mat, cv::COLOR_BGRA2BGR);
    }
    else if (in_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_ABGR8888)
    {
        cv::Mat rgba(in_height, in_width, CV_8UC4);
        memcpy(rgba.data, (unsigned char *)pVirsrc, in_height * in_width * 4 * sizeof(unsigned char));
        cv::cvtColor(rgba, dst_mat, cv::COLOR_RGBA2BGR);
    }
    else if (in_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_NV12)
    {
        cv::Mat yuv(in_height * 3 / 2, in_width, CV_8UC1);
        memcpy(yuv.data, (unsigned char *)pVirsrc, in_height * 3 / 2 * in_width * sizeof(unsigned char));
        cv::cvtColor(yuv, dst_mat, cv::COLOR_YUV2BGR_NV12);
    }
    else
    {
        std::cout << "not supported element format " << in_format
                  << " for dump " << std::endl;
        exit(__LINE__);
    }
}

void SaveResultImage(
    std::vector<BoxInfo> bboxes,
    std::string in_path,
    std::string out_dir,
    cv::Mat &buffer_mat)
{

    std::string image_base_name = in_path.substr(in_path.rfind("/") + 1);

    size_t last_dot_pos = image_base_name.rfind(".");
    image_base_name.replace(last_dot_pos, image_base_name.size() - last_dot_pos, ".png");

    if (access(out_dir.c_str(), F_OK) != 0)
    {
        mkdir(out_dir.c_str(), 0777);
    }

    std::string output_image_dir = out_dir + "images/";
    if (access(output_image_dir.c_str(), F_OK) != 0)
    {
        mkdir(output_image_dir.c_str(), 0777);
    }

    std::string out_image_path = output_image_dir + image_base_name;

    for (int i = 0; i < bboxes.size(); i++)
    {
        if (bboxes[i].label >= coco_classes.size())
        {
            printf("invalid class id %d ignored!\n", bboxes[i].label);
            continue;
        }

        cv::Rect rect(
            bboxes[i].x1,
            bboxes[i].y1,
            bboxes[i].x2 - bboxes[i].x1,
            bboxes[i].y2 - bboxes[i].y1);

        int32_t r_class_id = int32_t(bboxes[i].label);

        cv::Scalar r_box_color = cv::Scalar(0, 0, 255);

        std::string r_class_name(coco_classes[r_class_id]);

        cv::rectangle(buffer_mat, rect, r_box_color, 1, cv::LINE_8, 0);
        cv::putText(
            buffer_mat,
            r_class_name + "_0." + std::to_string(int(bboxes[i].score * 100)),
            cv::Point(bboxes[i].x1,
                      bboxes[i].y1),
            cv::FONT_HERSHEY_SIMPLEX,
            0.4,
            r_box_color,
            1);
    }

    std::cout << "out_image_path: " << out_image_path << std::endl;
    cv::imwrite(out_image_path, buffer_mat);
}

void Checked(MI_S32 ret_code)
{
    if (ret_code != MI_SUCCESS)
    {
        std::cerr << "exit with error code " << ret_code << std::endl;
        exit(ret_code);
    }
}

void CheckBoxes(std::vector<BoxInfo> bboxes, MI_S32 width, MI_S32 height)
{
    for (size_t i = 0; i < bboxes.size(); i++)
    {
        BoxInfo *r_box = &(bboxes[i]);

        if (r_box->x1 < 0)
        {
            r_box->x1 = 0;
        }
        else if (r_box->x1 > width - 1)
        {
            r_box->x1 = width - 1;
        }
        if (r_box->x2 < 0)
        {
            r_box->x2 = 0;
        }
        else if (r_box->x2 > width - 1)
        {
            r_box->x2 = width - 1;
        }
        if (r_box->x1 > r_box->x2)
        {
            r_box->x1 = r_box->x2;
        }

        if (r_box->y1 < 0)
        {
            r_box->y1 = 0;
        }
        else if (r_box->y1 > height - 1)
        {
            r_box->y1 = height - 1;
        }
        if (r_box->y2 < 0)
        {
            r_box->y2 = 0;
        }
        else if (r_box->y2 > height - 1)
        {
            r_box->y2 = height - 1;
        }
        if (r_box->y1 > r_box->y2)
        {
            r_box->y1 = r_box->y2;
        }

        if (r_box->score < 0.0)
        {
            r_box->score = 0.0;
        }
        else if (r_box->score > 1.0)
        {
            r_box->score = 1.0;
        }
    }
}

float sigmoid(float x)
{
    // return 1.0f / (1.0f + fast_exp(-x));
    return (1 / (1 + exp(-x)));
}

void softmax_(const float *x, float *y, int length)
{
    float sum = 0;
    int i = 0;
    for (i = 0; i < length; i++)
    {
        y[i] = exp(x[i]);
        sum += y[i];
    }
    for (i = 0; i < length; i++)
    {
        y[i] /= sum;
    }
}

void nms(std::vector<BoxInfo> &input_boxes, float NMS_THRESH)
{
    std::sort(input_boxes.begin(), input_boxes.end(), [](BoxInfo a, BoxInfo b)
              { return a.score > b.score; });
    std::vector<float> vArea(input_boxes.size());
    for (int i = 0; i < int(input_boxes.size()); ++i)
    {
        vArea[i] = (input_boxes[i].x2 - input_boxes[i].x1 + 1) * (input_boxes[i].y2 - input_boxes[i].y1 + 1);
    }
    for (int i = 0; i < int(input_boxes.size()); ++i)
    {
        for (int j = i + 1; j < int(input_boxes.size());)
        {
            float xx1 = std::max(input_boxes[i].x1, input_boxes[j].x1);
            float yy1 = std::max(input_boxes[i].y1, input_boxes[j].y1);
            float xx2 = std::min(input_boxes[i].x2, input_boxes[j].x2);
            float yy2 = std::min(input_boxes[i].y2, input_boxes[j].y2);
            float w = std::max(float(0), xx2 - xx1 + 1);
            float h = std::max(float(0), yy2 - yy1 + 1);
            float inter = w * h;
            float ovr = inter / (vArea[i] + vArea[j] - inter);
            if (ovr >= NMS_THRESH)
            {
                input_boxes.erase(input_boxes.begin() + j);
                vArea.erase(vArea.begin() + j);
            }
            else
            {
                j++;
            }
        }
    }
}

MI_S32 DetDecodeOneLayer(DetDecodeInfo &info, int i_layer, std::vector<BoxInfo> &out_boxes)
{
    if (i_layer >= info.num_out_layers)
    {
        printf("illegal access to out layer%d \n", i_layer);
        exit(__LINE__);
    }

    if (info.net_in_w % info.net_out_w[i_layer] != 0 ||
        info.net_in_h % info.net_out_h[i_layer] != 0 ||
        info.net_in_w / info.net_out_w[i_layer] != info.net_in_h / info.net_out_h[i_layer])
    {
        printf("Mismatch between input shape(%ux%u) and output shape(%ux%u)\n",
               info.net_in_w, info.net_in_h,
               info.net_out_w[i_layer], info.net_out_h[i_layer]);
        exit(__LINE__);
    }

    int stride = info.net_in_w / info.net_out_w[i_layer];

    if (info.num_classes < 0) {
        info.num_classes = info.net_out_c[i_layer] - 4 * info.reg_max;
        // printf("guess num_classes = %d\n", info.num_classes);
    }

    const int len = info.num_classes + 4 * info.reg_max;

    // y = sigmoid(x) = 1 / (1 + e^-x) ==> x = ln(y/(1-y))
    float threshold_before_sigmoid = log(info.conf_threshold / (1.0 - info.conf_threshold));
    float *scores = new float[info.num_classes];
    float *distribution = new float[info.reg_max];
    float ltrb[4];

    if (info.out_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_FP32)
    {
        // printf("executing FP32 decode...\n");
        const float *preds_float = (float *)info.data[i_layer];
        for (int i = 0; i < info.net_out_h[i_layer]; i++)
        {
            for (int j = 0; j < info.net_out_w[i_layer]; j++)
            {
                int max_ind = 0;
                float max_score = -INFINITY; // -inf

                for (int k = 0; k < info.num_classes; k++)
                {
                    if (preds_float[4 * info.reg_max + k] > max_score)
                    {
                        max_score = preds_float[4 * info.reg_max + k];
                        max_ind = k;
                    }
                }
                if (max_score >= threshold_before_sigmoid)
                {
                    max_score = sigmoid(max_score);

                    const float *pbox = preds_float;

                    for (int k = 0; k < 4; k++)
                    {
                        softmax_(pbox + k * info.reg_max, distribution, info.reg_max);

                        ltrb[k] = 0.0;
                        for (int l = 0; l < info.reg_max; l++)
                        {
                            ltrb[k] += l * distribution[l];
                        }
                        ltrb[k] = ltrb[k] * stride;
                    }
                    

                    float pb_cx = (j + 0.5) * stride;
                    float pb_cy = (i + 0.5) * stride;

                    BoxInfo r_box_info;
                    r_box_info.x1 = pb_cx - ltrb[0];
                    r_box_info.x2 = pb_cx + ltrb[2];
                    r_box_info.y1 = pb_cy - ltrb[1];
                    r_box_info.y2 = pb_cy + ltrb[3];
                    r_box_info.score = max_score;
                    r_box_info.label = max_ind;

                    out_boxes.push_back(r_box_info);
                }
                preds_float += ALIGN_UP(len, INNER_MOST_ALIGNMENT);
            }
        }
    }
    else if (info.out_format == MI_IPU_ELEMENT_FORMAT::MI_IPU_FORMAT_INT16)
    {
        // printf("executing INT16 decode...\n");
        const MI_S16 *preds_int16 = (MI_S16 *)info.data[i_layer];
        float r_score;
        float *dist_before_softmax = new float[info.reg_max];
        for (int i = 0; i < info.net_out_h[i_layer]; i++)
        {
            for (int j = 0; j < info.net_out_w[i_layer]; j++)
            {
                int max_ind = 0;
                float max_score = -INFINITY; // -inf

                for (int k = 0; k < info.num_classes; k++)
                {
                    r_score = preds_int16[4 * info.reg_max + k] * info.scalar[i_layer];
                    if (r_score > max_score)
                    {
                        max_score = r_score;
                        max_ind = k;
                    }
                }

                if (max_score >= threshold_before_sigmoid)
                {
                    max_score = sigmoid(max_score);

                    const int16_t *pbox = preds_int16;

                
                    for (int k = 0; k < 4; k++)
                    {
                        for (int i_reg = 0; i_reg < info.reg_max; i_reg++)
                        {
                            dist_before_softmax[i_reg] = *(pbox + k * info.reg_max + i_reg) * info.scalar[i_layer];
                        }

                        softmax_(dist_before_softmax, distribution, info.reg_max);

                        ltrb[k] = 0.0;
                        for (int l = 0; l < info.reg_max; l++)
                        {
                            ltrb[k] += l * distribution[l];
                        }
                        ltrb[k] = ltrb[k] * stride;
                    }

                    float pb_cx = (j + 0.5) * stride;
                    float pb_cy = (i + 0.5) * stride;

                    BoxInfo r_box_info;
                    r_box_info.x1 = pb_cx - ltrb[0];
                    r_box_info.x2 = pb_cx + ltrb[2];
                    r_box_info.y1 = pb_cy - ltrb[1];
                    r_box_info.y2 = pb_cy + ltrb[3];
                    r_box_info.score = max_score;
                    r_box_info.label = max_ind;

                    out_boxes.push_back(r_box_info);
                }
                preds_int16 += ALIGN_UP(len, INNER_MOST_ALIGNMENT);
            }
        }
        delete[] dist_before_softmax;
    }
    else
    {
        printf("%d\n", __LINE__);
        printf("Not implemented out format: %s !\n", info.out_format);
        delete[] scores;
        delete[] distribution;
        exit(__LINE__);
    }

    delete[] scores;
    delete[] distribution;

    return MI_SUCCESS;
}

int main(int argc, char *argv[])
{

    float threshold = 0.5;
    char model_path[MAX_PATH_STRLEN] = {0};
    char pFirmwarePath[MAX_PATH_STRLEN] = "/config/dla/ipu_firmware.bin";
    MI_S32 s32Ret;

    struct timeval tstart, tend;

    std::string in_images;
    parseCmdOptions(argc, argv, in_images, model_path, &threshold);

    ALGO_Input_t algo_input; // buffer info for det model

    MI_S32 ret = MI_SUCCESS;
    MI_U16 u16SocId = ST_DEFAULT_SOC_ID;
    MI_SYS_Init(u16SocId);

    MI_IPUChnAttr_t stChnAttr;
    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_IPU_OfflineModelStaticInfo_t OfflineModelInfo;
    memset(&stChnAttr, 0, sizeof(MI_IPUChnAttr_t));
    memset(&desc, 0, sizeof(MI_IPU_SubNet_InputOutputDesc_t));

    // 1. create device
    s32Ret = MI_IPU_GetOfflineModeStaticInfo(NULL, model_path, &OfflineModelInfo);
    if (s32Ret != MI_SUCCESS)
    {
        printf("get model variable buffer size failed (%d)!\n", s32Ret);
        exit(__LINE__);
    }

    MI_IPU_DevAttr_t stDevAttr;
    stDevAttr.u32MaxVariableBufSize = OfflineModelInfo.u32VariableBufferSize;
    stDevAttr.u32YUV420_W_Pitch_Alignment = 16;
    stDevAttr.u32YUV420_H_Pitch_Alignment = 2;
    stDevAttr.u32XRGB_W_Pitch_Alignment = 16;
    s32Ret = MI_IPU_CreateDevice(&stDevAttr, NULL, pFirmwarePath, 0);

    if (s32Ret != MI_SUCCESS)
    {
        printf("create ipu device failed(%d)!\n", s32Ret);
        exit(__LINE__);
    }

    // 2. create channel
    stChnAttr.u32InputBufDepth = 0;
    stChnAttr.u32OutputBufDepth = 1;
    MI_U32 u32ChannelID;
    s32Ret = MI_IPU_CreateCHN(&(u32ChannelID), &stChnAttr, NULL, model_path);
    if (s32Ret != MI_SUCCESS)
    {
        MI_IPU_DestroyDevice();
        printf("create detect ipu channel failed(%d)!\n", s32Ret);
        exit(__LINE__);
    }

    // 3. get input/output tensor
    MI_IPU_SubNet_InputOutputDesc_t model_desc;

    s32Ret = MI_IPU_GetInOutTensorDesc(u32ChannelID, &(model_desc));
    if (s32Ret != MI_SUCCESS)
    {
        MI_IPU_DestroyDevice();
        printf("get tensor desc failed(%d)!\n", s32Ret);
        exit(__LINE__);
    }

    // 4. init input buffer
    InputAttr_t input_attr;
    input_attr.width = model_desc.astMI_InputTensorDescs[0].u32TensorShape[2];
    input_attr.height = model_desc.astMI_InputTensorDescs[0].u32TensorShape[1];
    input_attr.format = model_desc.astMI_InputTensorDescs[0].eElmFormat;
    initDetBuffer(u16SocId, &input_attr, &algo_input);

    cv::Mat buffer_mat(input_attr.height, input_attr.width, CV_8UC3);

    std::vector<cv::String> input_paths;

    cv::glob(cv::String(in_images.c_str()), input_paths, false);
    std::cout << "found " << input_paths.size() << " images!" << std::endl;

    std::string time_str = std::to_string(long(time(0)));

    std::string out_dir = "./output/" + time_str + "/";
    if (access(out_dir.c_str(), F_OK) != 0)
    {
        mkdir(out_dir.c_str(), 0777);
    }

    for (int i = 0; i < input_paths.size(); i++)
    {

        std::cout << "[" << i << "] "
                  << "processing " << input_paths[i] << "..." << std::endl;

        // 5. fill input buffer

        fillDetBuffer(input_paths[i], &input_attr, &algo_input);
        // dump input buffer into an opencv mat
        dumpDetBuffer(&input_attr, &algo_input, buffer_mat);

        gettimeofday(&tstart, NULL);

        // 6. invoke model
        MI_IPU_TensorVector_t InputTensorVector;
        MI_IPU_TensorVector_t OutputTensorVector;
        memset(&InputTensorVector, 0, sizeof(MI_IPU_TensorVector_t));
        InputTensorVector.u32TensorCount = model_desc.u32InputTensorCount;
        s32Ret = MI_IPU_GetOutputTensors(u32ChannelID, &OutputTensorVector);
        if (s32Ret != MI_SUCCESS)
        {
            printf("get output tensor failed,ret:%d\n", s32Ret);
            exit(__LINE__);
        }

        InputTensorVector.astArrayTensors[0].phyTensorAddr[0] = algo_input.phy_addr;
        InputTensorVector.astArrayTensors[0].ptTensorData[0] = algo_input.p_vir_addr;

        if (MI_SUCCESS != (s32Ret = MI_IPU_Invoke(u32ChannelID, &InputTensorVector, &OutputTensorVector)))
        {
            printf("MI_IPU_Invoke error, ret %d", s32Ret);
            MI_IPU_PutOutputTensors(u32ChannelID, &OutputTensorVector);
            MI_IPU_PutInputTensors(u32ChannelID, &InputTensorVector);
            MI_IPU_DestroyCHN(u32ChannelID);
            MI_IPU_DestroyDevice();
            exit(__LINE__);
        }

        s32Ret = MI_IPU_PutOutputTensors(u32ChannelID, &OutputTensorVector);
        if (s32Ret != MI_SUCCESS)
        {
            printf("failed to put output tensor, return %d\n", s32Ret);
            exit(__LINE__);
        }

        gettimeofday(&tend, NULL);

        printf("model invoke time: %f ms\n",
               (tend.tv_sec - tstart.tv_sec) * 1000.0 + (tend.tv_usec - tstart.tv_usec) / 1000.0);

        tstart = tend;
        // 7. output decode
        DetDecodeInfo decode_info;
        memset(&decode_info, 0, sizeof(decode_info));

        if (model_desc.u32OutputTensorCount > MAX_NUM_OUT_TENSOR)
        {
            printf(
                "num output tensor channel should not be larger than %d\n",
                MAX_NUM_OUT_TENSOR);
            exit(__LINE__);
        }

        decode_info.num_out_layers = model_desc.u32OutputTensorCount;
        decode_info.net_in_w = input_attr.width;
        decode_info.net_in_h = input_attr.height;
        decode_info.out_format = model_desc.astMI_OutputTensorDescs[0].eElmFormat;;
        decode_info.nms_threshold = 0.5;
        decode_info.conf_threshold = threshold;
        decode_info.reg_max = 16;
        decode_info.num_classes = -1; // auto fill num_classes
      

        for (int i = 0; i < model_desc.u32OutputTensorCount && i < MAX_NUM_OUT_TENSOR; i++)
        {
            decode_info.data[i] = (void *)(OutputTensorVector.astArrayTensors[i].ptTensorData[0]);
            decode_info.scalar[i] = model_desc.astMI_OutputTensorDescs[i].fScalar;
            decode_info.net_out_h[i] = model_desc.astMI_OutputTensorDescs[i].u32TensorShape[1];
            decode_info.net_out_w[i] = model_desc.astMI_OutputTensorDescs[i].u32TensorShape[2];
            decode_info.net_out_c[i] = model_desc.astMI_OutputTensorDescs[i].u32TensorShape[3];

            // printf("layer %d w x h x c = %u x %u x %u \n",
            //          i, decode_info.net_out_w[i], decode_info.net_out_h[i], decode_info.net_out_c[i]);
        }

        

        std::vector<BoxInfo> out_boxes;

        for (int i_layer = 0; i_layer < decode_info.num_out_layers; i_layer++)
        {
            // printf("decoding layer %d...\n", i_layer);
            DetDecodeOneLayer(
                decode_info,
                i_layer,
                out_boxes);
        }

        CheckBoxes(out_boxes, decode_info.net_in_w, decode_info.net_in_h);

        nms(out_boxes, decode_info.nms_threshold);

        gettimeofday(&tend, NULL);

        printf("post process time: %f ms\n",
               (tend.tv_sec - tstart.tv_sec) * 1000.0 + (tend.tv_usec - tstart.tv_usec) / 1000.0);

        SaveResultImage(out_boxes, input_paths[i], out_dir, buffer_mat);
    }

    MI_SYS_FlushInvCache(algo_input.p_vir_addr, algo_input.buf_size);
    MI_SYS_Munmap(algo_input.p_vir_addr, algo_input.buf_size);
    MI_SYS_MMA_Free(u16SocId, algo_input.phy_addr);
    MI_IPU_DestroyCHN(u32ChannelID);
    MI_IPU_DestroyDevice();
    MI_SYS_Exit(u16SocId);
}