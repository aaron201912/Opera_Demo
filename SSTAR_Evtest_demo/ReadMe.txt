本Demo主要是查看设备信息，获取/模拟发送 触控节点/dev/input/eventX的EVENT事件


一、编译说明：
	本Demo不依赖任何MI相关，直接在本目录执行make clean;make即可(也可以直接gcc evtest.c -o Evtest)

二、生成bin文件：
    --> out/Evtest
	
三、参数说明:
	./Evtest [设备节点] [Read/Write/Dump_Info] [x] [y]  

Eg:
	读坐标：
	./evtest /dev/input/event0 0       //设备节点选择的试 event0   后面一个0是读cmd

	写坐标：
	./evtest /dev/input/event0 1 90 90   //设备节点选择的试 event0   后面一个1是写cmd，写cmd要带x,y坐标

	dump dev info:
	./evtest /dev/input/event0 2       //设备节点选择的试 event0   后面一个2是Dump Dev相关信息