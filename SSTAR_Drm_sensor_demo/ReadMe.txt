Demo说明：
	本Demo主要针对的是DRM架构的使用介绍

	需要SDK支持DRM && Dma_heap

一、编译说明：
	a.修改makefile中的编译链为系统对应编译链
	
    b.如果将demo放到跟project同级目录   （默认方式）
    --> make clean; make

    c.demo放置到任意路径，只需在make的时候指定ALKAID_PATH到工程的目录（需要链接SDK头文件）
    --> declare -x ALKAID_PATH=~/sdk/source_code/${mysdkrootpath}
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

二、生成bin文件：
    --> out/Drm_sensor_demo

三、Demo运行参数说明

************************* usage *************************

-s: [0/1/2]选择sensorpadid,SSU9383只支持选0/2, SSU9386只支持选0/1（默认是0）
-i：指定需要load得iqxx.bin,只作用于第一路pipline
-I：指定需要load得iqxx.bin,只作用于第二路pipline
-r：选择是否需要旋转（1/2/3分别对应是90/180/270度旋转）,只作用于第一路pipline
-m：指定人形检测的算法模型路径,只作用于第一路pipline
-n：[1/2]选择单路播放还是pip双sensor（默认单路）
-a：[0/1]选择是否打开人形检查,只作用于第一路pipline(默认关闭)
-H: [0/1]是否开启HDR功能，只作用于第一路pipline
-c: 指定需要显示的屏接口类型，需要提前配置好屏参否则会Segmentation fault

eg:
	单路sensor，从sensor_pad 0出流, 支持人型检测：
	./Drm_sensor_demo -s 0 -c ttl -i /customer/xxx.bin -m/module_path -a 1 -r 2
	
	双Sensor PIP,主路支持人型检测
               ./Drm_sensor_demo -n 2 -c ttl -i /customer/xxx.bin -I /customer/xxx.bin -m/module_path -a 1 -r 2

注：1.运行demo后，如sensor driver支持多res选择，择需要选择完成才会出图。双sensor时择要选择两次（第二路sensor的res打印会被第一路起来时的log刷掉，但输入对应index加回车即可显示第二路）
      2.由于Opera系列支持的sensor最大分辨率是2688x2564，所以选择的输入分辨率如果大于此会报错


