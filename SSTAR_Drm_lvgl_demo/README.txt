#提示
1.  该环境适用于SSU9383 DRM架构平台；
2.  编译:
    (1)进入到lvgl-master录下，根据项目需求更改 `Makefile` 中 `ALKAID_PROJ` 和 `CC` 的值，例如：
        ```mk
        CC := arm-linux-gnueabihf-gcc
        ALKAID_PROJ := /path/to/alkaid/project
        ```
     (2) 在lvgl-master目录下执行 “make CHIP=SSD2386 TOOLCHAIN_VERSION=6.4.0 -j8“或者“make CHIP=SSU9383 TOOLCHAIN_VERSION=6.4.0 -j8“
          或者“make CHIP=SSU9383 TOOLCHAIN_VERSION=10.2.1 -j8“或者“make CHIP=SSU9383 TOOLCHAIN_VERSION=10.2.1 -j8“;
     (3)生成 build目录，‘build/bin/demo’为最终可执行的程序；
         ### 运行
	默认情况下，编进去为送测的测试demo,直接执行 ./demo后运行。
3.   项目结构：
     ```
	 include							   // 外部库头文件
	 lib								   // 外部库  ---sdk里没有的lib, alsa, ffmpeg, drm 等库
     lvgl-master/build/                    // 编译生成的文件
     lvgl-master/lv_drivers/               // 官方 driver，这里主要使用 indev 部分
     lvgl-master/lv_porting_sstar/         // 移植与优化相关代码
     lvgl-master/lvgl/                     // lvgl 库主体代码
	 lvgl-master/lvgl/demos/sstar_test     // sstar test source code
     lvgl-master/squareline_proj/          // 为 squareline studio 准备的目录，存放预设的 makefile
     lvgl-master/lv_conf.h                 // lvgl 配置文件
     lvgl-master/lv_drv_conf.h             // lv_drivers 配置文件
     main.c                    // 主函数文件

      ```  		
4.  main.c 主要接口介绍：
	sstar_lv_init; //drm初始化,lvgl初始化,触控初始化,drm和lvgl对接初始化;
	lv_demo_menu; //测试的菜单实现控件;
	
5.  由于drm FB 在cma中申请， alsa播放MP3解码也需要从cma中申请内存，故boottargs建议进行如下配置：
bootargs=ubi.mtd=ubia,2048 root=/dev/mtdblock6 rootfstype=squashfs ro init=/linuxrc LX_MEM=0x20000000 mma_heap=mma_heap_name0,miu=0,sz=0x10000000,max_start_off=0x1000000000 mma_heap=mma_heap_fb,miu=0,sz=0x1000000,max_end_off=0x1100000000 mma_memblock_remove=1 cma=14M nohz=off mmap_reserved=smf,miu=0,sz=0x40000,max_start_off=0x1FFC0000,max_end_off=0x20000000 mtdparts=nand0:1792k@1280k(BOOT),1792k(BOOT_BAK),256k(ENV),256k(ENV1),5m(KERNEL),5m(RECOVERY),6m(rootfs),1m(MISC),108288k(ubia)

