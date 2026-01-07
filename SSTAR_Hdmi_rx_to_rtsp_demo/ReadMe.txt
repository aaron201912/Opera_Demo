Demo说明：
    本Demo主要针对的是HDMIRX->HVP->SCL->VENC->RTSP的使用介绍
	如需要支持hdcp，需要额外定义HDCP_KEY_PATH
    需要SDK支持HDMIRX

一、编译说明：
    a.修改makefile中的编译链为系统对应编译链
    
    b.如果将demo放到跟project同级目录（默认方式）
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

    c.demo放置到任意路径，只需在make的时候指定ALKAID_PATH到工程的目录（需要链接SDK头文件）
    --> declare -x ALKAID_PATH=~/sdk/source_code/${mysdkrootpath}
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

二、生成bin文件：
    --> out/sstar_hdmi_rx_to_rtsp

三、Demo运行参数说明

    无参数，直接./sstar_hdmi_rx_to_rtsp