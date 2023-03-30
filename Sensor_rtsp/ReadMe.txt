Demo说明：
	
	1、sensor->vif->isp->scl->ttl
                                                      ->ipu
                                                      ->rtsp
模型说明：
	以 syfd5.360302_fixed.sim_sgsimg.img 为例
	sy：算法代号
	fd：face detection，标示人脸检测，其他情况：
		pcn（表示person、bicycle、car、motorcycle、bus、truck的检测）
		pcd（表示person，cat，dog的检测）
		pf（表示person，face的检测）
		pc（表示person、car、bus、truck的检测）
	5：算法代号，高内存版本，其他情况：
		2（低内存版）
	36: 模型输入 352*640，其他情况：
		48（表示480*800）
	0302：算法代号

一、编译说明：
    a.修改makefile中的编译链为系统对应编译链，修改PROJ_ROOT路径
	
    b. make clean; make 

二、code修改说明：
    根据不同的模型和输入格式修改g_detection_info，包括ipu firmware路径、模型路径、分辨率等。
	
三、生成bin文件：
    --> out/Sensor_rtsp  sensor图像实时送给算法，在图像中用osd框出人脸/人形并实时显示在屏幕上

四、Demo运行参数说明

************************* usage *************************
 ./Sensor_rtsp -h
-p : select sensor pad
-c : select panel type: ttl / mipi
-b : select iq file
eg:./Sensor_rtsp  -p 0 -c ttl 


