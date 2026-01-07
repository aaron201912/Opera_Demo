dla_detect用于推演检测模型，模型必须使用SGS的后处理方法。

编译说明：
    a.修改makefile中的编译链为系统对应编译链

    b.如果将demo放到跟project同级目录   （默认方式）
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

    c.demo放置到任意路径，只需在make的时候指定PROJ_ROOT到SDK工程project的目录（需要链接SDK头文件）
    --> declare -x PROJ_ROOT=~/sdk/source_code/${myprojectpath}
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

运行可获得使用方法提示, 将resource和prog_dla_detect拷贝到customer下
./prog_dla_detect
USAGE: ./prog_dla_detect: <xxxsgsimg.img> <picture> <labels> <model intput_format:RGB or BGR>
cd /customer
./prog_dla_detect resource/caffe_yolo_v2_fixed.sim_sgsimg.img resource/009962.bmp resource/mscoco_label.txt RGB

其中，
xxxsgsimg.img -> 为离线img模型路径
picture -> 为图片路径
labels -> 为标签路径
model intput_format:RGB or BGR -> 选择RGB或BGR

注意：
该Demo仅能运行input_config.ini中input_formats设置为RGB或BGR的检测模型。
该Demo示例了使用MI_SYS的接口管理模型Input和Output的Buffer。
