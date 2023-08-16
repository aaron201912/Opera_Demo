dla_yolov8_det用于推演yolov8检测模型。

编译说明：
    a.修改makefile中的编译链为系统对应编译链，修改PROJ_ROOT路径	
    b. make clean; make 

运行: ./prog_dla_yolov8_det: -m <xxxsgsimg.img> -i <picture>

其中，
xxxsgsimg.img -> 为离线img模型路径
picture -> 为图片路径

Ex:
./prog_dla_yolov8_det -m yolov8s_y248_P5_20230607_fixed.sim_sgsimg.img -i 009962.bmp 
demo_args: in_images=009962.bmp; model_path=yolov8s_y248_P5_20230607_fixed.sim_sgsimg.img; threshold=0.5
found 1 images!
[0] processing ./009962.bmp...
processing ./009962.bmp...
model invoke time: 34.114000 ms
post process time: 11.634000 ms
out_image_path: ./output/470/images/009962.png
client [1096] connected, module:ipu

------shutdown IPU0------
client [1096] disconnected, module:ipu
