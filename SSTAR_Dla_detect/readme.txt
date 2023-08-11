dla_detect用于推演检测模型，模型必须使用SGS的后处理方法。

编译说明：
    a.修改makefile中的编译链为系统对应编译链，修改PROJ_ROOT路径	
    b. make clean; make 

运行可获得使用方法提示
./prog_dla_dla_detect
USAGE: ./prog_dla_dla_detect: <xxxsgsimg.img> <picture> <labels> <model intput_format:RGB or BGR>


其中，
xxxsgsimg.img -> 为离线img模型路径
picture -> 为图片路径
labels -> 为标签路径
model intput_format:RGB or BGR -> 选择RGB或BGR

注意：
该Demo仅能运行input_config.ini中input_formats设置为RGB或BGR的检测模型。
该Demo示例了使用MI_SYS的接口管理模型Input和Output的Buffer。
