1.编译demo：arm-linux-gnueabihf-gcc uart.c -lpthread -o uart_test 或 aarch64-linux-gnu-gcc uart.c -lpthread -o uart_test
2.运行，将uart的rx和tx短接
 Usage:./uart_test dev_name baudrate databit stopbit parity ctsrts [test type] 
 ./uart_test 设备名  波特率  数据位（5/6/7/8） 停止位(1.1.5,2)  奇偶校验（O：奇校验 E：偶校验 N：无校验）ctsrts：硬件流控（1/2）  [test type] 0或者2：把RX和TX短接，发送1024字
 节数据（type 0和1 能对比接送和发送的数据是否相同，单线程执行）    1或者3：把RX和TX短接，发送      1024*1024*50字节数据   4：接受rx数据再tx回去
 3.uart_test_uart1.sh为测试不同波特率，不同奇偶校验将tx和rx短接，确认发送和接收数据是否相同的脚本
