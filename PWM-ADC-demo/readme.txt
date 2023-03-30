编译
arm-linux-gnueabihf-gcc -o demo-sar demo-sar.c

或aarch64-linux-gnu-gcc -o demo-sar demo-sar.c


使用方法：
./demo-sar [channel_id] [channel_id] ... [channel_id]
例如：
读取并显示PAD_SAR_ADC_4 、 PAD_SAR_ADC_8 、 PAD_SAR_ADC_12 、 PAD_SAR_ADC_5 、 PAD_SAR_ADC_7 、 PAD_SAR_ADC_21 的ADC值 
./demo-sar 4 8 12 5 7 21



测试：
通过阅读原理图将对应的PAD_SAR_ADC_0到PAD_SAR_ADC_23引脚与滑动变阻器短接，然后通过滑动滑动变阻器改变电压来测试PWM-ADC功能