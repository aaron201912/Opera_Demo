1.将timer目录放在与kernel同一级目录下，执行make clean;make 就可以生成mdrv_timer.ko
2.在板子上执行insmod mdrv_timer.ko,就可以看到定时中断服务的打印