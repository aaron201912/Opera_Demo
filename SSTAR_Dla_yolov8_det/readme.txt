dla_yolov8_det用于推演yolov8检测模型。

编译说明：
    a.修改makefile中的编译链为系统对应编译链

    b.如果将demo放到跟project同级目录   （默认方式）
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

    c.demo放置到任意路径，只需在make的时候指定PROJ_ROOT到SDK工程project的目录（需要链接SDK头文件）
    --> declare -x PROJ_ROOT=~/sdk/source_code/${myprojectpath}
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

运行: ./prog_dla_yolov8_det: -m <xxxsgsimg.img> -i <picture>

其中，
xxxsgsimg.img -> 为离线img模型路径
picture -> 为图片路径

Ex:
将resource和prog_dla_yolov8_det拷贝到customer下
cd /customer
./prog_dla_yolov8_det -m resource/yolov8s_y248_SSU938X_20230607_fixed.sim_sgsimg.img -i resource/009962.bmp -t 0.5
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
