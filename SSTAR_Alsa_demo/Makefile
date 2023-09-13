TARGET = ./out/alsa_demo
TOOLCHAIN_VERSION ?= 10.2.1
ifeq ($(TOOLCHAIN_VERSION),6.4.0)
CROSS_COMPILE =arm-linux-gnueabihf-
endif
ifeq ($(TOOLCHAIN_VERSION),10.2.1)
CROSS_COMPILE =aarch64-linux-gnu-
endif

TEST_OBJECT:= ./aio.c
INCLUDE_TEST += -I./include
INCLUDE_TEST += -I./include/alsa

ifeq ($(TOOLCHAIN_VERSION),6.4.0)
LIB_VERSION = lib_32
endif
ifeq ($(TOOLCHAIN_VERSION),10.2.1)
LIB_VERSION = lib_64
endif

LIBS_SHARED_TEST += -L./$(LIB_VERSION)
LIBS_SHARED_TEST += -lrt -lasound -lm -ldl -lpthread
CFLAGS := 

all:
	rm -rf out
	mkdir out	
	$(CROSS_COMPILE)gcc $(CFLAGS) $(INCLUDE_TEST) $(TEST_OBJECT) $(LIBS_SHARED_TEST) -D__ALSA__=1 -DCONFIG_SIGMASTAR_CHIP_P5=1 -o $(TARGET)
	$(CROSS_COMPILE)strip $(TARGET) --strip-unneeded
	
.PHONY: clean
clean:
	rm -rf out/*
	rm -rf *.o


