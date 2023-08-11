﻿dla_classify用于推演分类模型。

编译说明：
    a.修改makefile中的编译链为系统对应编译链，修改PROJ_ROOT路径	
    b. make clean; make 

运行可获得使用方法提示
./prog_dla_dla_classify
USAGE: ./prog_dla_dla_classify: <xxxsgsimg.img><picture> <labels> <model intput_format:RGB or BGR>


其中，
xxxsgsimg.img -> 为离线img模型路径
picture -> 为图片路径
labels -> 为标签路径
model intput_format:RGB or BGR -> 选择RGB或BGR

注意：
该Demo仅能运行input_config.ini中input_formats设置为RGB或BGR的分类模型。
该Demo示例了从内存读取模型的方法。

Ex:
运行caffe分类模型
./prog_dla_classify caffe_resnet50_conv_fixed.sim_sgsimg.img  ILSVRC2012_test_00000002.bmp labels1.txt RGB

model_img:caffe_resnet50_conv_fixed.sim_sgsimg.img
picture:ILSVRC2012_test_00000002.bmp
labels:labels1.txt
model input_format:RGB
get variable size from memory__
read from call back function
read from call back function
create channel from memory__
read from call back function
read from call back function
input tensor[0] name :data
output tensor[0] name :prob
input size should be :3 224 224
now input size is :3 500 447
img is going to resize!
the times is 32 
fps:132.873
All outputs XOR = 0x073e6a03
the class Count :1000


order: 1 index: 18 0.993824 
order: 2 index: 143 0.00115969 
order: 3 index: 912 0.000854509 
order: 4 index: 20 0.000732436 
order: 5 index: 16 0.000579845 
client [1075] connected, module:ipu

------shutdown IPU0------
client [1075] disconnected, module:ipu


