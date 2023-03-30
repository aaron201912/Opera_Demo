Demo说明：
	本Demo主要针对的是DRM架构的使用介绍

	需要SDK支持DRM && Dma_heap

一、编译说明：
	a.修改makefile中的编译链为系统对应编译链
	
    b.如果将demo放到跟project同级目录   （默认方式）
    --> make clean; make

    c.demo放置到任意路径，只需在make的时候指定ALKAID_PATH到工程的目录（需要链接SDK头文件）
    --> declare -x ALKAID_PATH=~/sdk/source_code/${mysdkrootpath}
    --> make clean; make 

二、生成bin文件：
    --> out/Drm_sensor

三、Demo运行参数说明

************************* usage *************************

-s: [0/2]选择sensorpadid,只支持选0/2（默认是0）
-i：指定需要load得iqxx.bin,只作用于第一路pipline
-r：选择是否需要旋转（1/2/3分别对应是90/180/270度旋转）,只作用于第一路pipline
-m：指定人形检测的算法模型路径,只作用于第一路pipline
-n：[1/2]选择单路播放还是pip双sensor（默认单路）
-a：[0/1]选择是否打开人形检查,只作用于第一路pipline(默认关闭)
-v：[0/1]选择是否打开Venc（同时会关闭DRM显示）,只作用于第一路（注意没有对接rtsp，仅为了测试）

eg:
	单路sensor，从sensor_pad 0出流：
	./Drm_sensor -s 0
	
	单路sensor，从sensor_pad 2出流：
	./Drm_sensor -s 2
	
	双Sensor PIP：
	./Drm_sensor -n 2
	
	双Sensor PIP,主路支持人型检测
	./Drm_sensor -n 2 -a 1
	
	单sensor出流并指定sensor iqbin路径
	./Drm_sensor -s0 -i /customer/xxx.bin


