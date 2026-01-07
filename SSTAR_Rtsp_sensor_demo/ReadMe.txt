Demo说明：
    本Demo主要针对的是Sensor -> vif -> isp -> scl -> venc ->rtsp的使用介绍

一、编译说明：
    a.修改makefile中的编译链为系统对应编译链

    b.如果将demo放到跟project同级目录   （默认方式）
    --> make clean; make

    c.demo放置到任意路径，只需在make的时候指定ALKAID_PATH到工程的目录（需要链接SDK头文件）
    --> declare -x ALKAID_PATH=~/sdk/source_code/${mysdkrootpath}
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

二、生成bin文件：
    --> out/Rtsp_sensor_demo

三、Demo运行参数说明

************************* usage *************************
-s : [0/1/2]select sensor pad when chose single sensor
-i : chose iqbin path for the fisrt pipeline
-I : chose iqbin path for the second pipeline
-n : [1:2]enable multi sensor and select pipeline
-v : first pipeline select vdf or venc, 0 is vdf, 1 is venc
-m : chose detect model path for the fisrt pipeline
-a : enable face detect funtion for the fisrt pipeline
-r : set rotate mode,default value is 0. [0,1,2,3] = [0,90,180,270]
-h : [0/1]enable hdr

eg:
    单路sensor，从sensor_pad 0出流, isp旋转180度，支持人型检测：
    ./Rtsp_sensor_demo -s 0 -v 1 -i /customer/xxx.bin -r 2 -m /module_path -a 1

    双Sensor PIP,主路支持人型检测
    ./Rtsp_sensor_demo -n 2 -i /customer/xxx.bin -I /customer/xxx.bin -m/module_path -a 1 -v 1

    单sensor, 从sensor_pad 0出流, scl输出到vdf做md运动侦测算法，输入vdf的分辨率宽高需要16x2对齐，打印算法结果,
    按像素块(4x4、8x8、16x16)输出结果, 0表示该对应像素块没有运动，255表示应像素块运动，最高支持600K@15帧分辨率，
    跑vdf时ko需要加载mi_shadow.ko、库文件需要加载libmi_vdf.so、libmi_shadow.so、libMD_LINUX.so、libOD_LINUX.so、libVG_LINUX.so，makefile需要设置SSTAR_VDF = enable
    ./Rtsp_sensor_demo -n 1 -v 0


