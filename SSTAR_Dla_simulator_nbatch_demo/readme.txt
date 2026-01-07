dla_simulator用于推演模型，功能与SGS_IPU_SDK中simulator.py类似

编译说明：
    a.修改makefile中的编译链为系统对应编译链

    b.如果将demo放到跟project同级目录   （默认方式）
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

    c.demo放置到任意路径，只需在make的时候指定PROJ_ROOT到SDK工程project的目录（需要链接SDK头文件）
    --> declare -x PROJ_ROOT=~/sdk/source_code/${myprojectpath}
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

运行可获得使用方法提示, 将resource和prog_dla_simulator_nbatch拷贝到customer下
cd /customer
./prog_dla_simulator_nbatch -i resource/photo -m resource/mobilenet_v2_fixed.sim_sgsimg.img -n 1 -c Classification --format RGB

./prog_dla_simulator
Usage: ./prog_dla_dla_simulator [-i] [-m] [-c] ([--format] [--ipu_firmware])
 -h, --help      show this help message and exit
 -i, --images    path to validation image or images' folder
 -m, --model     path to model file
 -n, --number    max number of images when invoke once
 -c, --category  indicate model category:
                 Classification / Detection / Unknown
     --format    model input format (Default is BGR):
                 BGR / RGB / BGRA / RGBA / YUV_NV12 / GRAY / RAWDATA_S16_NHWC / DUMP_RAWDATA

其中，
-i 参数为图片路径或图片文件夹路径
-m 参数为离线img模型路径
-n 参数为一次推理执行的最大图片张数
-c 参数为对模型后处理方法（与SGS_IPU_SDK中simulator.py相同）
--format 为可选参数，用于指定模型的input_formats（以input_config.ini中配置为准）
    BGR / RGB / BGRA / RGBA / YUV_NV12 / GRAY 这些参数为图片输入的模型，input_config.ini中input_formats配置为这些参数时，可以使用对应的参数，会将图片转换为对应的格式后送入模型。
    RAWDATA_S16_NHWC 为input_config.ini中input_formats配置为RAWDATA_S16_NHWC时使用，使用该参数时，-i 参数为使用float32格式存下的rawdata文件，可以使用numpy.ndarray.tofile方法保存float模型的输入数据，务必以float32的data type保存。
    DUMP_RAWDATA 为使用SGS_IPU_SDK中simulator.py时，增加--dump_rawdata后保存下的.bin文件作为-i 参数的输入，可以作为比对simulator.py和prog_dla_simulator结果的方法。
