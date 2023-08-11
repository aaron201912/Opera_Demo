Demo说明：
    本Demo主要针对的是UVC device的使用介绍

    需要SDK支持UVC device

一、编译说明：
    a.修改makefile中的编译链为系统对应编译链
    
    b.如果将demo放到跟project同级目录（默认方式）
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

    c.demo放置到任意路径，只需在make的时候指定ALKAID_PATH到工程的目录（需要链接SDK头文件）
    --> declare -x ALKAID_PATH=~/sdk/source_code/${mysdkrootpath}
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

二、生成bin文件：
    --> out/uvc_demo

三、Demo运行参数说明

-a: set sensor pad
-A: set sensor resolution index
-b: bitrate
-B: burst
-m: mult
-p: maxpacket
-M: 0:mmap, 1:userptr
-N: num of uvc stream
-i: set iq api.bin,ex: -i /customer/imx415_api.bin
-I: [c_intf/s_intf], c_intf: control interface;s_intf: streaming interface
-t: Trace level (0-6)
-q: open iqserver
-T: 0:Isoc, 1:Bulk
-Q: qfactor
-f: use file instead of MI,ex: -f /customer/resource
-V: 0:Disable video, 1:Enable video(Default)
-h: help message

eg:
    ./uvc_demo -a 0 -A 0 -t 1