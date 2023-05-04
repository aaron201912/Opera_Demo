#!/bin/sh

for i in 2400 4800 9600 19200 38400 57600 115200 230400 460800 921600
do
    echo $i
    ./uart_test /dev/ttyS1 $i 8 1 N 0 0
    if [ $? -ne 0 ];then
        echo "./uart_test /dev/ttyS1 $i 8 1 N 0 0 fail"
        return
    fi
    ./uart_test /dev/ttyS1 $i 8 1.5 N 0 0
    if [ $? -ne 0 ];then
        echo "./uart_test /dev/ttyS1 $i 8 1.5 N 0 0 fail"
        return
    fi
    ./uart_test /dev/ttyS1 $i 8 2 N 0 0
    if [ $? -ne 0 ];then
        echo "./uart_test /dev/ttyS1 $i 8 2 N 0 0 fail"
        return
    fi

    ./uart_test /dev/ttyS1 $i 8 1 O 0 0
    if [ $? -ne 0 ];then
        echo "./uart_test /dev/ttyS1 $i 8 1 O 0 0 fail"
        return
    fi
    ./uart_test /dev/ttyS1 $i 8 1.5 O 0 0
    if [ $? -ne 0 ];then
        echo "./uart_test /dev/ttyS1 $i 8 1 O 0 0 fail"
        return
    fi
    ./uart_test /dev/ttyS1 $i 8 2 O 0 0
    if [ $? -ne 0 ];then
       echo "./uart_test /dev/ttyS1 $i 8 1 O 0 0 fail"
       return
    fi


    ./uart_test /dev/ttyS1 $i 8 1 E 0 0
    if [ $? -ne 0 ];then
        echo "./uart_test /dev/ttyS1 $i 8 1 E 0 0 fail"
        return
    fi
    ./uart_test /dev/ttyS1 $i 8 1.5 E 0 0
    if [ $? -ne 0 ];then
        echo "./uart_test /dev/ttyS1 $i 8 1.5 E 0 0 fail"
        return
    fi
    ./uart_test /dev/ttyS1 $i 8 2 E 0 0
    if [ $? -ne 0 ];then
        echo "./uart_test /dev/ttyS1 $i 8 2 E 0 0 fail"
        return
    fi
done
