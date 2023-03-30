function description:
使用opengles绘制一张三角形的图片，通过Drm送到屏端显示

tool chain:
gcc-6.4.0-20221118-sigmastar-glibc-x86_64_arm-linux-gnueabihf/bin:$PATH

编译方法：
export CROSS_COMPILE=arm-linux-gnueabihf-
export ARCH=arm
make

执行方法：
./gles_common
