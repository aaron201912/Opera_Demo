dla_show_img_info用于生成IPU Log，结合IPU SDK中的分析工具使用，可以看到每层的性能耗时占比。

编译说明：
    a.修改makefile中的编译链为系统对应编译链，修改PROJ_ROOT路径	
    b. make clean; make TOOLCHAIN_VERSION=6.4.0 或 make TOOLCHAIN_VERSION=10.2.1

运行可获得使用方法提示
 ./prog_dla_dla_show_img_info 
Usage: ./prog_dla_show_img_info  [-m PATH] ([--details_info])
 -h, --help      show this help message and exit
 -m, --model     path to model file
                 Mulitply models use comma-separated
     --details_info
                 show details model info (Default: false)
     --ipu_log
                 path to save ipu log
     --ipu_log_size
                 size of ipu log (Default: 0x800000)
	
Ex:
./prog_dla_dla_show_img_info -m sypfa5.480302_fixed.sim_sgsimg.img --ipu_log ./
mi_ipu_datatype.h info:
Max Input:      60
Max Output:     60
sypfa5.480302_fixed.sim_sgsimg.img(0):
Invoke Time: 8.72117 ms
Input(0):
    name:       images
    dtype:      BGRA
    shape:      [1, 480, 800, 3]
    size:       1536000
Output(0):
    name:       output
    dtype:      FLOAT32
    shape:      [1, 3, 60, 100, 7]
    size:       504000
Output(1):
    name:       435
    dtype:      FLOAT32
    shape:      [1, 3, 30, 50, 7]
    size:       126016
Output(2):
    name:       455
    dtype:      FLOAT32
    shape:      [1, 3, 15, 25, 7]
    size:       31552


[ipu0] ctrl addr=400aea000 total_size=8388608 used_size=130816 corectrl addr=4012ea000 total_size=8388608 used_size=32 ctrl=ffffff cctrl=1fff

ctrl addr=800000, total_size=11444224 used_size=4 corectrl addr=20, total_size=19832832 used_size=4
ctrl addr=0, total_size=0 used_size=0 corectrl addr=0, total_size=0 used_size=0
save ctrl0 log to .//sypfa5.480302_fixed.sim_sgsimg.img_log_core0.bin
save corectrl0 log to .//sypfa5.480302_fixed.sim_sgsimg.img_log_corectrl0.bin
