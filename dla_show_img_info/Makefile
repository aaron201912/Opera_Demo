YEAR := $(shell date '+%Y')
DATE := $(shell date '+%m%d')
TOOLCHAIN_VERSION ?= 6.4.0
ifeq ($(TOOLCHAIN_VERSION),6.4.0)
CROSS_COMPILE ?=arm-linux-gnueabihf-
endif
ifeq ($(TOOLCHAIN_VERSION),10.2.1)
CROSS_COMPILE ?=aarch64-linux-gnu-
endif
PROJ_ROOT=/home/peng.xiong/Alkaid/project

#test
TEST_OBJECT:= ./dla_show_img_info.cpp ./show_ipu_log.cpp
INCLUDE_TEST += -I $(PROJ_ROOT)/release/include -I ./
#INCLUDE_TEST += -I $(PROJ_ROOT)/../kernel/drivers/sstar/include   

#dynamic
LIBS_SHARED_TEST += -L $(PROJ_ROOT)/release/chip/p5/dispcam/common/glibc/$(TOOLCHAIN_VERSION)/mi_libs/dynamic -lmi_ipu  -lmi_sys -lmi_common
LIBS_SHARED_TEST += -L $(PROJ_ROOT)/release/chip/p5/sigma_common_libs/glibc/$(TOOLCHAIN_VERSION)/dynamic -lcam_fs_wrapper 
#static
LIBS_STATICT_TEST += -L $(PROJ_ROOT)/release/chip/p5/dispcam/common/glibc/$(TOOLCHAIN_VERSION)/mi_libs/static -lmi_ipu -lmi_sys -lmi_common
LIBS_STATICT_TEST += -L $(PROJ_ROOT)/release/chip/p5/sigma_common_libs/glibc/$(TOOLCHAIN_VERSION)/static -lcam_fs_wrapper -lcam_os_wrapper
CXXFLAGS := -Wno-psabi -std=c++11 
all:
	rm -rf out
	mkdir out
	
ifeq ($(TOOLCHAIN_VERSION),6.4.0)
	#dynamic
	#$(CROSS_COMPILE)g++ $(CXXFLAGS) $(INCLUDE_TEST) $(TEST_OBJECT) -O3 -g -Wall -o ./out/prog_dla_dla_show_img_info $(LIBS_STATICT_TEST) -static -Wl,-rpath=./out/ -lm -lpthread -ldl -march=armv7-a -mfpu=neon -ffast-math
	#static
	$(CROSS_COMPILE)g++ $(CXXFLAGS) $(INCLUDE_TEST) $(TEST_OBJECT) -O3 -g -Wall -o ./out/prog_dla_dla_show_img_info $(LIBS_STATICT_TEST) -static -lm -lpthread -ldl -march=armv7-a -mfpu=neon -ffast-math
endif
ifeq ($(TOOLCHAIN_VERSION),10.2.1)
	#dynamic
	#$(CROSS_COMPILE)g++ $(CXXFLAGS) $(INCLUDE_TEST) $(TEST_OBJECT) -O3 -g -Wall -o ./out/prog_dla_dla_show_img_info $(LIBS_STATICT_TEST) -static -Wl,-rpath=./out/ -lm -lpthread -ldl -march=armv8-a -ffast-math
	#static
	$(CROSS_COMPILE)g++ $(CXXFLAGS) $(INCLUDE_TEST) $(TEST_OBJECT) -O3 -g -Wall -o ./out/prog_dla_dla_show_img_info $(LIBS_STATICT_TEST) -static -lm -lpthread -ldl -march=armv8-a -ffast-math
endif

.PHONY: clean
clean:
	rm -rf out/*
	rm -rf *.o


