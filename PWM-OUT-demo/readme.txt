编译
arm-linux-gnueabihf-gcc -o demo-pwm-out demo-pwm-out.c

或aarch64-linux-gnu-gcc -o demo-pwm-out demo-pwm-out.c


使用方法：
1. ./demo-pwm-out -r [pwmId]
2. ./demo-pwm-out -w [pwmId] -p [period] -d [duty_cycle] -o [polarity] -e [enable]
    polarity:
        0 - normal
        1 - inversed
    enable:
        0 - disable
        1 - enable



例如：
1、读取 PAD_PWM_OUT1 引脚的PWM-OUT配置参数
    ./demo-pwm-out -r 1
2、设置 PAD_PWM_OUT0 引脚的PWM-OUT配置参数如下：
    ①周期500000ns
    ②高电平100000ns
    ③反转极性（高低电平转换，实际波形为周期500000ns，高电平400000ns）
    ④启动PWM输出
    ./demo-pwm-out -w 0 -p 500000 -d 100000 -o 1 -e 1



测试：
通过阅读原理图将对应的PAD_PWM_OUT0引脚与示波器或者逻辑分析仪连接，然后观察波形是否符合设置

注：PWM-OUT6在demo板上还复用为SDIO WIFI的电源控制引脚，所以示波器看到的波形会有“失真”，可以去掉R228电阻后问题解决，但是会导致SDIO_WIFI无法使用
