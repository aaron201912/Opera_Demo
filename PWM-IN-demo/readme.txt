编译
arm-linux-gnueabihf-gcc -o demo-pwm-in demo-pwm-in.c

或aarch64-linux-gnu-gcc -o demo-pwm-in demo-pwm-in.c

使用方法：
1. ./demo-pwm-in



测试：
通过阅读原理图将对应的OUTP_TX1_CH3引脚与信号发生器的PWM波形进行连接，然后检查PWM-IN的log是否与信号发生器内容相符合