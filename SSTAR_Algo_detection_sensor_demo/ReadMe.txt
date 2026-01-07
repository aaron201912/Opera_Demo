Demo说明：

    本Demo主要针对的是IPU运算人脸、人形检测的使用介绍，sensor图像实时送给算法运算并显示在屏幕的场景。注：需要SDK支持DRM && Dma_heap。
    注：libsstaralgo_det.a\libsstaralgo_det.so为SSTAR研发算法，有体验次数限制。
    运行超过次数后会报Error: This is a trial version. The limited trials of detection is reached.
    如需生产使用，请联系SSTAR商务。

模型说明：
    参考doc/det_zh.md中1.1的算法说明

一、编译说明：
    a.修改makefile中的编译链为系统对应编译链

    b.如果将demo放到跟project同级目录   （默认方式）
    --> make clean; make

    c.demo放置到任意路径，只需在make的时候指定ALKAID_PATH到工程的目录（需要链接SDK头文件）
    --> declare -x ALKAID_PATH=~/sdk/source_code/${mysdkrootpath}
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

二、code修改说明：
    根据不同的模型和输入格式修改g_detection_info，包括ipu firmware路径、模型路径、分辨率、检测阈值等。
    可根据doc目录下文档了解该算法api使用。

三、生成bin文件：
    --> out/Algo_detect_sensor  sensor图像实时送给算法，在图像中用osd框出人脸/人形并实时Drm显示在屏幕上

四、Demo运行参数说明

************************* usage *************************
 ./Algo_detect_sensor -h
-p : select sensor pad
-c : select panel type, ttl, mipi, lvds or hdmi
-b : select iq file
-r : select scl rotate, 0:NONE, 1:90,2:180, 3:270
eg:./Algo_detect_sensor -p 0 -c ttl -b iqfile -r 1


举例:
1.硬件接好sensor和panel，并设置好kernel相关配置以支持sensor和panel, panel配置和选择的显示类型对不上时会core dump
2.把sdy48.img拷贝到code中指定目录/customer/res/
3.运行./Algo_detect_sensor -p 0 -c ttl
4.在屏幕上看到人脸和人形被框出，并随人移动


