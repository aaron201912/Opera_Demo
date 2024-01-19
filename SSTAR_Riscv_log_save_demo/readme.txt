编译
 arm-linux-gnueabihf-gcc riscv_log_save_demo.c -o riscv_log_save_demo

 或aarch64-linux-gnu-gcc riscv_log_save_demo.c -o riscv_log_save_demo


使用方法：
riscv固件需要打开日志功能，参考https://dev.comake.online/home/article/2335?sid=37579
运行./riscv_log_save_demo
此时riscv打印的log将会存在/tmp/save_log_file.txt文件中