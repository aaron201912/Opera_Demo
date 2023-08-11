YEAR := $(shell date '+%Y')
DATE := $(shell date '+%m%d')
TOOLCHAIN_VERSION = 10.2.1
ifeq ($(TOOLCHAIN_VERSION),6.4.0)
CROSS_COMPILE ?=arm-linux-gnueabihf-
endif
ifeq ($(TOOLCHAIN_VERSION),10.2.1)
CROSS_COMPILE ?=aarch64-linux-gnu-
endif
PROJ_ROOT=/users/peng.xiong/Opera_V3.0.0/project

#test
TEST_OBJECT:= ./dla_classifyNBatch.cpp
INCLUDE_TEST += -I ./inc/opencv4
INCLUDE_TEST += -I $(PROJ_ROOT)/release/include
INCLUDE_TEST += -I $(PROJ_ROOT)/release/include/ipu/opera
INCLUDE_TEST += -I $(PROJ_ROOT)/../kernel/drivers/sstar/include   

ifeq ($(TOOLCHAIN_VERSION),6.4.0)
LIB_VERSION ?= libs_32
endif
ifeq ($(TOOLCHAIN_VERSION),10.2.1)
LIB_VERSION ?= libs_64
endif

#dynamic
LIBS_SHARED_TEST += -L $(PROJ_ROOT)/release/chip/p5/dispcam/common/glibc/$(TOOLCHAIN_VERSION)/mi_libs/dynamic -lmi_ipu  -lmi_sys -lmi_common
LIBS_SHARED_TEST += -L $(PROJ_ROOT)/release/chip/p5/sigma_common_libs/glibc/$(TOOLCHAIN_VERSION)/dynamic -lcam_fs_wrapper -lcam_os_wrapper
LIBS_SHARED_TEST += -L ./$(LIB_VERSION)/shared_opencv
LIBS_SHARED_TEST += -lopencv_imgcodecs -lopencv_imgproc -lopencv_core 
#static
LIBS_STATICT_TEST += -L $(PROJ_ROOT)/release/chip/p5/dispcam/common/glibc/$(TOOLCHAIN_VERSION)/mi_libs/static -lmi_ipu -lmi_sys -lmi_common
LIBS_STATICT_TEST += -L $(PROJ_ROOT)/release/chip/p5/sigma_common_libs/glibc/$(TOOLCHAIN_VERSION)/static -lcam_fs_wrapper -lcam_os_wrapper
LIBS_STATICT_TEST += -L ./$(LIB_VERSION)/static_opencv
LIBS_STATICT_TEST += -L ./$(LIB_VERSION)/static_opencv/opencv4/3rdparty
LIBS_STATICT_TEST += -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -littnotify -llibjasper  -llibjpeg-turbo  -llibpng -llibtiff  -llibwebp -ltegra_hal -lzlib
CXXFLAGS := -Wno-psabi -std=c++11 
all:
	rm -rf out
	mkdir out

ifeq ($(TOOLCHAIN_VERSION),6.4.0)
	#dynamic
	#$(CROSS_COMPILE)g++ $(CXXFLAGS) $(INCLUDE_TEST) $(TEST_OBJECT) -O3 -g -Wall -o ./out/prog_dla_classifyNBatch $(LIBS_SHARED_TEST) -Wl,-rpath=./out/ -lm -lpthread -ldl -march=armv7-a -mfpu=neon -ffast-math
	#static
	$(CROSS_COMPILE)g++ $(CXXFLAGS) $(INCLUDE_TEST) $(TEST_OBJECT) -O3 -g -Wall -o ./out/prog_dla_classifyNBatch $(LIBS_STATICT_TEST) -static -lm -lpthread -ldl -march=armv7-a -mfpu=neon -ffast-math
endif
ifeq ($(TOOLCHAIN_VERSION),10.2.1)
	#dynamic
	#$(CROSS_COMPILE)g++ $(CXXFLAGS) $(INCLUDE_TEST) $(TEST_OBJECT) -O3 -g -Wall -o ./out/prog_dla_classifyNBatch $(LIBS_SHARED_TEST) -Wl,-rpath=./out/ -lm -lpthread -ldl -march=armv8-a -ffast-math
	#static
	$(CROSS_COMPILE)g++ $(CXXFLAGS) $(INCLUDE_TEST) $(TEST_OBJECT) -O3 -g -Wall -o ./out/prog_dla_classifyNBatch $(LIBS_STATICT_TEST) -static -lm -lpthread -ldl -march=armv8-a -ffast-math
endif	
	
.PHONY: clean
clean:
	rm -rf out/*
	rm -rf *.o


