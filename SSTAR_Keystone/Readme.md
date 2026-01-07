# keystone使用说明

---

## 一、demo场景

**场景一：基于HDMI_RX为视频源的梯形校正**

```
  HDMI_RX -> HVP -> SCL -> GPUGFX -> DRM
                                      ^
                     Ui -> GPUGFX-----|


  参数说明： HDMI_RX->HVP->SCL 均为1920x1080@60帧，SCL output为1024x600@60帧，DRM屏参为1024x600；

            HVP输出格式为YUV444，SCL输出格式为YUV420

            其中目前UI素材为ARGB8888的512x300图像，在DRM中的显示位置为左上角（50，50）坐标位置


  绑定模式：hvp->scl为realtime，SCL使用Dev8 Chn0 port0，其他均为为framemode，且SCLoutputport后均为DmaBuffer。
```

**场景二：基于ffmpeg解码为视频源的梯形校正**

```
  File ->FFMPEG -> SCL -> GPUGFX -> DRM
                                     ^
                    Ui -> GPUGFX-----|


  参数说明： File采用1080P@60帧的H264编码格式，通过FFMPEG接口将获得的图像Buffer输入给SCL Dev1 chn0 port0

            SCL output为1024x600@60帧，DRM屏参为1024x600；

            FFMPEG输出格式为YUV444，SCL输出格式为YUV420

            其中目前UI素材为ARGB8888的512x300图像，在DRM中的显示位置为左上角（50，50）坐标位置


  绑定模式：均为为framemode，且SCLoutputport后均为DmaBuffer。
```

---

## 二、编译环境说明

1. 本demo必须使用6.4.0版本的toolchain进行编译（依赖的lib均为6.4.0）

2. 如果将demo放到跟project同级目录   （默认方式）  
    --> make clean; make

3. demo放置到任意路径，只需在make的时候指定ALKAID_PATH到工程的目录（需要链接SDK头文件）  
    --> declare -x ALKAID_PATH=~/sdk/source_code/${mysdkrootpath}  
    --> make clean; make
ex：
make TOOLCHAIN_VERSION=6.4.0 ALKAID_PATH=/home/jackson.pan/customer/stable_p5/project CHIP=SSD2381
cp out/sstar_keystone ~/customer/stable_p5/sdk/verify/application/lvgl_demo_bga16_2/prog_keystone 
---

## 三、运行环境说明

**板端环境：**

SPK_L1/L2 分别插上扬声器，具体型号由FAE提供，HDMI_RX通过HDMI线连接PC(测试的HDMI源)，TTL888链接1024x600的TTL屏。

**dts配置：**

默认dts已配好，无需修改

**输入文件：**

```
    由于该demo依赖的so库较多，需统一打包后将对应的so库连接至板子内`LD_LIBRARY_PATH`

    两场景均需要输入UI图像，场景二需要输入对应的源视频流。

```

---

## 四、运行说明

将可执行文件sstar_keystone放到板子上，修改权限777

1. 按`./sstar_keystone -m 1 -r 0 -i xxx -u xxx -w xxx -h xxx`运行；

   > -m: 0 为场景一, 1 为场景二

   > -i: 场景二需要输入的源视频流路径, 若-m 1的情况下为必填项

   > -r: 使能video旋转 0=rot_0 / 1=rot_90 / 2=rot_180 / 3=rot_270 
r
   > -u：需要输入的ui图像的路径，格式要求为ABGR8888

   > -w/-h: 分别为ui输入图像的宽高,若与输入ui图的宽高不符，显示出来的ui会异常


运行demo后，TTL屏会播放对应场景的图像

2. 在运行中，可通过 `a/b/c/d/e/f/g/h` 8个字符来控制对应位置的梯形校正程度，分别为的

   > a：左上+10 / b: 左上-10

   > c：左下+10 / d: 左下-10

   > e：右上+10 / f: 右上-10

   > g：右下+10 / h: 右下-10

---

**退出命令**

    输入`q`即可退出demo
