#��ʾ
1.  �û���������SSU9383 DRM�ܹ�ƽ̨��
2.  ����:
    (1)���뵽lvgl-master¼�£�������Ŀ������� `Makefile` �� `ALKAID_PROJ` �� `CC` ��ֵ�����磺
        ```mk
        CC := arm-linux-gnueabihf-gcc
        ALKAID_PROJ := /path/to/alkaid/project
        ```
     (2) ��lvgl-masterĿ¼��ִ�� ��make CHIP=SSD2386 TOOLCHAIN_VERSION=6.4.0 -j8�����ߡ�make CHIP=SSU9383 TOOLCHAIN_VERSION=6.4.0 -j8��
          ���ߡ�make CHIP=SSU9383 TOOLCHAIN_VERSION=10.2.1 -j8�����ߡ�make CHIP=SSU9383 TOOLCHAIN_VERSION=10.2.1 -j8��;
     (3)���� buildĿ¼����build/bin/demo��Ϊ���տ�ִ�еĳ���
         ### ����
	Ĭ������£����ȥΪ�Ͳ�Ĳ���demo,ֱ��ִ�� ./demo�����С�
3.   ��Ŀ�ṹ��
     ```
	 include							   // �ⲿ��ͷ�ļ�
	 lib								   // �ⲿ��  ---sdk��û�е�lib, alsa, ffmpeg, drm �ȿ�
     lvgl-master/build/                    // �������ɵ��ļ�
     lvgl-master/lv_drivers/               // �ٷ� driver��������Ҫʹ�� indev ����
     lvgl-master/lv_porting_sstar/         // ��ֲ���Ż���ش���
     lvgl-master/lvgl/                     // lvgl ���������
	 lvgl-master/lvgl/demos/sstar_test     // sstar test source code
     lvgl-master/squareline_proj/          // Ϊ squareline studio ׼����Ŀ¼�����Ԥ��� makefile
     lvgl-master/lv_conf.h                 // lvgl �����ļ�
     lvgl-master/lv_drv_conf.h             // lv_drivers �����ļ�
     main.c                    // �������ļ�

      ```  		
4.  main.c ��Ҫ�ӿڽ��ܣ�
	sstar_lv_init; //drm��ʼ��,lvgl��ʼ��,���س�ʼ��,drm��lvgl�Խӳ�ʼ��;
	lv_demo_menu; //���ԵĲ˵�ʵ�ֿؼ�;
	
5.  ����drm FB ��cma�����룬 alsa����MP3����Ҳ��Ҫ��cma�������ڴ棬��boottargs��������������ã�
bootargs=ubi.mtd=ubia,2048 root=/dev/mtdblock6 rootfstype=squashfs ro init=/linuxrc LX_MEM=0x20000000 mma_heap=mma_heap_name0,miu=0,sz=0x10000000,max_start_off=0x1000000000 mma_heap=mma_heap_fb,miu=0,sz=0x1000000,max_end_off=0x1100000000 mma_memblock_remove=1 cma=14M nohz=off mmap_reserved=smf,miu=0,sz=0x40000,max_start_off=0x1FFC0000,max_end_off=0x20000000 mtdparts=nand0:1792k@1280k(BOOT),1792k(BOOT_BAK),256k(ENV),256k(ENV1),5m(KERNEL),5m(RECOVERY),6m(rootfs),1m(MISC),108288k(ubia)

