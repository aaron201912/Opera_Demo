Demo说明：
    本Demo主要针对的是UVC host的使用介绍

    需要SDK支持UVC host

一、编译说明：
    a.修改makefile中的编译链为系统对应编译链
    
    b.如果将demo放到跟project同级目录（默认方式）
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 ENABLE_UVC_HOST=1 OR make clean; make TOOLCHAIN_VERSION=10.2.1 ENABLE_UVC_HOST=1

    c.demo放置到任意路径，只需在make的时候指定ALKAID_PATH到工程的目录（需要链接SDK头文件）
    --> declare -x ALKAID_PATH=~/sdk/source_code/${mysdkrootpath}
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 ENABLE_UVC_HOST=1 OR make clean; make TOOLCHAIN_VERSION=10.2.1 ENABLE_UVC_HOST=1

二、生成bin文件：
    --> out/uvc_host_demo

三、Demo运行参数说明

-a: 选择对应的UAC device, /dev/snd/pcmC[X]D0C (default 0), 暂未支持
-b: 设置v4l2的buffer数量(default 3)
-m: 运行模式, [1]:Video Stream [2]:Video Control [4]:Audio Stream In [8]:Audio Stream Out [16]:Audio Control (default 1), 目前仅支持Video Stream
-v: 选择对应的UVC device, /dev/video[X] (default 0)
-d: buffer处理模式, [0]:No Handle Mode [1]:File Handle Mode [2]:MI Handle Mode (default 0)
-p: 保存路径(default /mnt)
-t: 调试等级 (default 1)
-h: 帮助信息

eg:
    ./uvc_host_demo -m 1 -v 0 -d 2 -t 2