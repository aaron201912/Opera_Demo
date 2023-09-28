Demo说明：
	本Demo主要针对的是使用ALSA AUDIO的场景。
	
一、编译说明：
    a.修改makefile中的编译链为系统对应编译链
	
    b. make clean; make TOOLCHAIN_VERSION=6.4.0 或 make clean; make TOOLCHAIN_VERSION=10.2.1
	
二、生成bin文件：
    --> out/alsa_demo 将bin文件、lib_32/或lib_64/和res/alsa/拷贝或打包到image中,指定lib和alsa.conf路径, 如以下示例
    export LD_LIBRARY_PATH=/customer/lib_32:$LD_LIBRARY_PATH
    export ALSA_CONFIG_DIR=/customer/res/alsa

四、Demo运行参数说明

************************* usage *************************
./alsa_demo -h
usage: $ [usecase] -i [interface] [options]
[usecase name]                                  [support interface]
<playback>                                      <dac/echo_tx/spdif/i2s_a/i2s_b>
<capture>                                       <adc_a/adc_b/dmic/echo_rx/hdmi_rx/i2s_a/i2s_b>
<passthrough>                                   <adc_a-spdif/adc_a-dac>
options:
-A | --card <capture card number>               The card to capture audio
-a | --card <playback card number>              The card to playback audio
-D | --device <capture device number>           The device to capture audio
-d | --device <playback device number>          The device to playback audio
-C | --Channel <capture channel>                The channel to capture audio
-c | --Channel <playback channel>               The channel to playback audio
-R | --Rate <capture sample rate>               The sample rate to capture audio
-r | --Rate <playback sample rate>              The sample rate to playback audio
-F | --file <capture file>                      The file name to capture audio
-f | --file <playback file>                     The file name to playback audio
-T | --time <capture time>                      The time to capture audio
-t | --time <playback time>                     The time to playback audio
-V | --volume <capture volume>                  The volume to capture audio
-v | --volume <playback volume>                 The volume to playback audio

eg:
    //使用AMIC录音10秒, 录音增益设置60%, 音频保存在test.wav中
    ./alsa_demo capture -i adc_a -F test.wav -A 0 -D 0 -R 8000 -C 1 -T 10 -V 60
    //使用DMIC录音10秒, 录音增益设置60%, 音频保存在test.wav中
    ./alsa_demo capture -i dmic -F test.wav -A 0 -D 0 -R 8000 -C 1 -T 10 -V 60
    //使用SPEAK播放音频10秒, 播放音量设置40%
    ./alsa_demo playback -i dac -f ./test.wav -a 0 -d 0 -c 2 -t 10 -v 40
    //passthrough AI硬件直连AO边录边播
    ./alsa_demo passthrough -i adc_a-dac -a 0 -d 0 -r 8000 -c 2 -t 10 -v 60 -A 0 -D 0 -R 8000 -C 1 -T 10 -V 60

参数说明：
[usecase]:选择需要使用的功能, playback播放; capture录音;passthrough硬件直连边录边播
[interface]:选择需要使用的外设接口，adc_a:AMIC0/AMIC1;adc_b:AMIC2;dmic:DMIC;dac:SPEAK L/R, 使用DMIC需要配置DMIC padmux
options:
-A:选择捕获音频用的声卡ID, 可通过cat /proc/asound/cards查看, 一般使用0
-a:选择播放音频用的声卡ID, 可通过cat /proc/asound/cards查看, 一般使用0
-D:选择捕获音频的设备ID，一般使用0选择DMA
-d:选择播放音频的设备ID，一般使用0选择DMA
-C:设置捕获音频时的通道数
-c:设置播放音频时的通道数
-F:选择捕获音频的保存文件，文件格式使用wav
-f:选择需要播放的音频文件，文件格式使用wav
-T:设置捕获音频的时间，单位秒
-t:设置播放音频的时间，单位秒
-V:设置捕获音频的增益的百分比，0-100
-v:设置播放音频的音量的百分比，0-100



