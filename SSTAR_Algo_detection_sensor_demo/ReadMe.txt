Demo说明：
	
	本Demo主要针对的是IPU运算人脸、人形检测的使用介绍，sensor图像实时送给算法运算并显示在屏幕的场景。注：需要SDK支持DRM && Dma_heap。
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
    a.修改makefile中的编译链为系统对应编译链，修改ALKAID_PATH路径
	
    b. make clean; make TOOLCHAIN_VERSION=6.4.0 或 make TOOLCHAIN_VERSION=10.2.1

二、code修改说明：
    根据不同的模型和输入格式修改g_detection_info，包括ipu firmware路径、模型路径、分辨率、检测阈值等。
    可根据doc目录下文档了解该算法api使用。
	
三、生成bin文件：
    --> out/Algo_detect_sensor  sensor图像实时送给算法，在图像中用osd框出人脸/人形并实时Drm显示在屏幕上

四、Demo运行参数说明

************************* usage *************************
 ./Algo_detect_sensor -h
-p : select sensor pad
-c : select panel type: ttl / mipi
-b : select iq file		default：NULL
-r : rotate 0：NONE    1：90    2：180    3：270	default：0
eg:./Algo_detect_sensor -p 0 -c ttl/mipi -b iqfile -r 1


举例:
1.硬件接好sensor和panel，并设置好kernel相关配置以支持sensor和panel
2.把sypfa5.480302_fixed.sim_sgsimg.img拷贝到code中指导目录
3.运行./Algo_detect_sensor -p 0 -c ttl
4.在屏幕上看到人脸和人形被框出，并随人移动


