Demo说明：
    本Demo主要针对的是使用ALSA + ffmpeg播放mp3音频的场景。

一、编译说明：
    a.指定编译链为系统对应编译链
    10.2.1:
    export PATH=/tools/toolchain/gcc-10.2.1-20210303-sigmastar-glibc-x86_64_aarch64-linux-gnu/bin:$PATH
    export CROSS_COMPILE=aarch64-linux-gnu-
    export ARCH=arm64
    6.4.0:
    export PATH=/tools/toolchain/gcc-6.4.0-20220722-sigmastar-glibc-x86_64_arm-linux-gnueabihf/bin/:$PATH
    export CROSS_COMPILE=arm-linux-gnueabihf-
    export ARCH=arm

    b. cd source;make clean; make

二、生成bin文件：
    --> source/bin/mp3Player
    根据系统版本(32位和64位)将source/ffmpeg和source/sstar下lib_32或lib_64的lib拷贝或打包到/customer/lib下, 将bin文件，runEnv下的res和对应版本的amixer、run.sh拷贝到/customer下
    ./run.sh
    
    ex:
    1.set env 
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/config/lib/:/customer/lib
    export ALSA_CONFIG_DIR=/customer/res/alsa
    (./amixer cset name='AMP_CTL' 1)&
    2.run ./mp3Player -i /customer/res/start.mp3 -v  50
