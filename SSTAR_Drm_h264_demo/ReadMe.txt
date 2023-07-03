Demo说明：
	本Demo主要针对的是DRM架构的使用介绍

	需要SDK支持DRM && Dma_heap

一、编译说明：
	a.修改makefile中的编译链为系统对应编译链
	
    b.如果将demo放到跟project同级目录   （默认方式）
    --> make clean; make

    c.demo放置到任意路径，只需在make的时候指定ALKAID_PATH到工程的目录（需要链接SDK头文件）
    --> declare -x ALKAID_PATH=~/sdk/source_code/${mysdkrootpath}
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 或 make TOOLCHAIN_VERSION=10.2.1

二、生成bin文件：
    --> out/Drm_player

三、Demo运行参数说明

************************* usage *************************

--vpath  : 使能vdec,指定video播放文件路径
-g : 选填,指定显示GOP图标(默认值为0)
-y : 选填,指定显示NV12图像(默认值为0)
-c : 必填,指定输出panel类型,ttl / mipi


eg:
	./Drm_player --vpath 720P25.h264 -g 1 -y 1 -c ttl


