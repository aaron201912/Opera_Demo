#提示
1.  该环境适用于SSU9383平台；
2.  编译:
    (1)进入到rpmsg_userspace录下，执行
        ./10.2.1.sh
        make clean;make TOOLCHAIN_VERSION=10.2.1 -j8
        或者
        ./6.4.0.sh
        make clean;make TOOLCHAIN_VERSION=6.4.0 -j8
    (2)进入bin目录，bin/rpmsg_demo为最终可执行的程序；
        ### 运行
        直接执行 ./rpmsg_demo运行。
3.   消息结构：
Linux与MCU通讯消息目前设计为message和command：
message:
    [MESSAGE_TYPE][BSP_TYPE][Description/DATA]
    [Description]: [BSP_TYPE] select 0/1/2
    [DATA]: [BSP_TYPE] select 3/4/5
    [MESSAGE_TYPE]
        0: message
        1: command
    [BSP_TYPE]
        0: GPIO
        1: ADC
        2: PWM out
        3: PWM in
        4: ADC data
        5: PWM data
command:
    [MESSAGE_TYPE][BSP_TYPE][BSP_COMMAND]
    [BSP_COMMAND]:
        [PARAM_LENTH1][PARAM1][PARAM_LENTH2][PARAM2]...
    [MESSAGE_TYPE]
        0: message
        1: command
    [BSP_TYPE]
        0: GPIO
        1: ADC
        2: PWM out
        3: PWM in
        4: ADC data
        5: PWM data