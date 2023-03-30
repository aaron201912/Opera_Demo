编译
 arm-linux-gnueabihf-gcc i2c_demo.c -o i2c_read_write

 或aarch64-linux-gnu-gcc i2c_demo.c -o i2c_read_write


使用方法：
./i2c_read_write i2c r[w] start_addr reg_addr [value]
例如：
选择I2C 4（/dev/i2c-4），写，从设备地址为5d，寄存器8140，数据1234
./i2c_read_write 4 w 5d  8140 1234



测试：
目前测试I2C4 有拉引脚，且有上拉
./i2c_read_write 4 w 5d  8140 1234