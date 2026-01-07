dla_simulator用于推演模型，功能与SGS_IPU_SDK中simulator.py类似

编译说明：
    a.修改makefile中的编译链为系统对应编译链

    b.如果将demo放到跟project同级目录   （默认方式）
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

    c.demo放置到任意路径，只需在make的时候指定PROJ_ROOT到SDK工程project的目录（需要链接SDK头文件）
    --> declare -x PROJ_ROOT=~/sdk/source_code/${myprojectpath}
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

运行可获得使用方法提示, 将resource和prog_dla_simulator拷贝到customer下
cd /customer
./prog_dla_simulator
Usage: ./prog_dla_dla_simulator [-i] [-m] ([-c] [-f])
 -h, --help      show this help message and exit
 -i, --images    path to validation image or images' folder
 -m, --model     path to model file
 -c, --category  indicate model category (Default is Unknown):
                 Classification / Detection / Unknown
 -f, --training_formats
                 model training input formats (Default is BGR):
                 BGR / RGB / GRAY / RAWDATA_NHWC / DUMP_RAWDATA


其中，
-i 参数为图片路径或图片文件夹路径。(多输入模型传入多个图片文件夹，只支持每个图片文件夹中仅有一张图片)
-m 参数为离线img模型路径
-c 为可选参数，默认为Unknown，参数为对模型后处理方法（与SGS_IPU_SDK中simulator.py相同）
-f 为可选参数，默认为BGR，用于指定模型的training_input_formats（以input_config.ini中配置为准）
    `BGR / RGB / GRAY` 这些参数为图片输入的模型，input_config.ini中training_input_formats配置为这些参数时，
            可以使用对应的参数。input_config.ini中input_formats已存如模型，会将training_input_formats转到
            模型需要的格式后输入。
    `RAWDATA_NHWC` 为input_config.ini中training_input_formats配置为
            RAWDATA_S16_NHWC / RAWDATA_F32_NHWC / RAWDATA_COMPLEX64 时使用，使用该参数时，
            -i 参数为使用numpy.ndarray.tofile方法保存模型的输入数据，
            RAWDATA_S16_NHWC / RAWDATA_F32_NHWC 务必以float32的data type保存，
            RAWDATA_COMPLEX64务必以complex64的data type保存。
    `DUMP_RAWDATA` 为使用SGS_IPU_SDK中simulator.py时，增加--dump_rawdata后保存下的.bin文件作为-i 参数的输入，
            可以作为比对simulator.py和prog_dla_dla_simulator结果的方法。

Ex:
单输入RGB分类模型, invoke单张图片
./prog_dla_simulator -m resource/ssd_mobilenet_v1_fixed.sim_sgsimg.img   -i resource/009962.bmp -c Detection -f RGB

单输入RGB分类模型，遍历invoke图片目录100张图片
./prog_dla_simulator -i ./100_imgs_folder -m test_fixed.sim_sgsimg.img -c Classification --format RGB

双输入RGB/YUV分类模型，invoke单张图片
./prog_dla_simulator -i ./img1.jpg,img2.jpg -m test_fixed.sim_sgsimg.img -c Classification --format RGB,YUV