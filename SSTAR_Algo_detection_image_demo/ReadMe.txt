﻿Demo说明：
	本Demo主要针对的是IPU运算人脸、人形检测的使用介绍，推单张或多张图给算法运算的场景。
	注：libsstaralgo_det.a\libsstaralgo_det.so为SSTAR研发算法，有体验次数限制。
	运行超过次数后会报Error: This is a trial version. The limited trials of detection is reached.
	如需生产使用，请联系SSTAR商务。

模型说明：
	以 syfdy2.48_V3.10S_20230506_fixed.sim_sgsimg.img 为例
	sy：算法代号
	fd：face detection，标示人脸检测，其他情况：
		pd （表示person的检测）
		pcn（表示person、bicycle、car、motorcycle、bus、truck的检测）
		pcnh（表示person、bicycle、car、motorcycle、bus、truck、head的检测）
		pcnf（表示person、bicycle、car、motorcycle、bus、truck、face的检测）
		pcncd（表示person、bicycle、car、motorcycle、bus、truck、cat、dog的检测）
	y：YUV_NV12，模型输入pixel format：
		a（ARGB8888）
	48: 模型输入h*w为800*480，其他情况：
		48（表示640*352）
	V3.10：模型版本
	S：标准模型，其他情况：
		M （中模型）
		L （大模型）
	20230506：模型日期
	fixed.sim_sgsimg.img：模型后缀

一、编译说明：
    a.修改makefile中的编译链为系统对应编译链，修改PROJ_ROOT路径
	
    b. make clean; make TOOLCHAIN_VERSION=6.4.0 或 make TOOLCHAIN_VERSION=10.2.1

二、code修改说明：
    根据不同的模型和输入格式修改detection_info_argb或detection_info_yuv，包括ipu firmware路径、模型路径、分辨率等。下面均以yuv_nv12格式的模型举例。
    可根据doc目录下文档了解该算法api使用。
	
三、生成bin文件：
    --> out/test_draw  推源图到算法，在图中框出人脸/人形并保存输出图到指定位置

四、Demo运行参数说明

************************* usage *************************
./test_draw -h
Usage: ./test_draw --images --input_format --format
 -h, --help      show this help message and exit
 -i, --images    path to validation image or images' folder
 -s, --input_format      select data input image format: rgb / argb8888 / yuv, default=rgb
     --format    model input format (Default is YUV_NV12):
                 BGR / RGB / BGRA / RGBA / YUV_NV12 / GRAY / RAWDATA_S16_NHWC

举例:
1.在板端test_draw所在目录下创建./output文件夹
2.把syfdy2.48_V3.10S_20230506_fixed.sim_sgsimg.img拷贝到code中指导目录
3.把res目录下的800x480.png拷贝到./src/800x480.png
4.运行./test_draw -i ./src/800x480.png --input_format rgb，会在output下生成结果图片
5.在./src多放几张图，运行./test_draw -i ./src/ --input_format rgb，会在output下生成每张源图推算的结果图片


