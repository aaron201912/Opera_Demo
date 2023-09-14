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
-m : chose detect model path for the fisrt pipeline
-a : enable face detect funtion for the fisrt pipeline
-r : set rotate mode,default value is 0. [0,1,2,3] = [0,90,180,270]
-h : [0/1]enable hdr

eg:
	单路sensor，从sensor_pad 0出流, isp旋转180度，支持人型检测：
	./Rtsp_sensor_demo -s 0 -i /customer/xxx.bin -r 2 -m /module_path -a 1
	
	双Sensor PIP,主路支持人型检测
    	./Rtsp_sensor_demo -n 2 -i /customer/xxx.bin -I /customer/xxx.bin -m/module_path -a 1 


