
#TOOLCHAIN_VERSION=10.2.1
TOOLCHAIN_VERSION=6.4.0
ENABLE_LDC=0

ifeq ($(TOOLCHAIN_VERSION),6.4.0)
CROSS_COMPILE ?=arm-linux-gnueabihf-
endif

ifeq ($(TOOLCHAIN_VERSION),10.2.1)
CROSS_COMPILE ?=aarch64-linux-gnu-
endif

CC  = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
AR  = $(CROSS_COMPILE)ar

ALKAID_PATH ?= /home/xinhua.ye/alkaid_p5/
INTERFACE_CUS3A = enable
dirs := ./src
dirs += ./src/internal/rgn
OUTPUT_DIR := ./out
$(info mysdk=$(ALKAID_PATH))
COM_FLAGS = -Wall -g -fPIC
#COM_FLAGS += -mfloat-abi=hard
C_FLAGS  = $(COM_FLAGS) -std=gnu11
CPP_FLAGS  = $(COM_FLAGS) -std=gnu++11

INCLUDES  := -I./include -I$(ALKAID_PATH)/project/release/include   -I./include/libdrm  -I./include/internal -I$(ALKAID_PATH)/kernel/drivers/sstar/include -I./include -I$(ALKAID_PATH)/project/release/include/isp -I$(ALKAID_PATH)/project/release/include/isp/opera -I./include -I$(ALKAID_PATH)/project/release/include/ipu/opera

TARGET_NAME  := Drm_sensor

CPP_SRCS := $(foreach dir,$(dirs),$(wildcard $(dir)/*.cpp))
CPP_OBJS := $(foreach n,$(CPP_SRCS),$(addsuffix .cpp.o,$(basename ${n})))

C_SRCS := $(foreach dir,$(dirs),$(wildcard $(dir)/*.c))
C_OBJS := $(foreach n,$(C_SRCS),$(addsuffix .c.o,$(basename ${n})))

ifeq ($(TOOLCHAIN_VERSION),10.2.1)
LIB_PATH  := -L./lib_64 -L./lib_64/algolib
endif

ifeq ($(TOOLCHAIN_VERSION),6.4.0)
LIB_PATH  := -L./lib -L./lib/algo
endif

LIB_PATH  += -L$(ALKAID_PATH)/project/release/chip/p5/dispcam/common/glibc/$(TOOLCHAIN_VERSION)/mi_libs/dynamic
LIB_PATH  += -L$(ALKAID_PATH)/project/release/chip/p5/sigma_common_libs/glibc/$(TOOLCHAIN_VERSION)/dynamic/
LIB_NAME := -lm  -ldrm -lmi_venc -lmi_isp -lmi_vif -lmi_sensor -lmi_iqserver -lmi_scl -lmi_sys -lmi_rgn -lmi_ipu  -lmi_common -lsstar_algo -lsstar_algo_detection  -ldl -lcam_os_wrapper  -lcam_fs_wrapper -lpthread


ifeq ($(ENABLE_LDC),1)
C_FLAGS += -DENABLE_LDC
CPP_FLAGS += -DENABLE_LDC
LIB_NAME += -lgles_ldc -lmali -lEGL -lGLESv1_CM -lGLESv2
INCLUDES  += -I$(ALKAID_PATH)/sdk/osdk/gpu/opengles/ldc
LIB_PATH   +=-L$(ALKAID_PATH)/sdk/osdk/gpu/opengles/ldc -L$(ALKAID_PATH)/project/release/chip/p5/dispcam/common/glibc/$(TOOLCHAIN_VERSION)/3rd_party_libs/dynamic
endif

ifeq ($(INTERFACE_CUS3A), enable)
LIB_NAME += -lcus3a -lispalgo
endif

.PHONY: all prepare clean

all: prepare $(TARGET_NAME) finish

prepare:
	@echo
	@echo ">>>>========================================================"
	@echo "TARGET_NAME = $(TARGET_NAME)"
	@echo

clean:
	@rm -Rf $(CPP_OBJS)
	@rm -f $(C_OBJS)
	@rm -Rf $(OUTPUT_DIR)

finish: $(TARGET_NAME)
	@echo "<<<<========================================================"
	@rm -Rf $(CPP_OBJS)
	@rm -f $(C_OBJS)
	@mkdir -p $(OUTPUT_DIR)
	@mv $(TARGET_NAME) $(OUTPUT_DIR) -v
	@echo "make Done"
	@echo

$(TARGET_NAME): $(CPP_OBJS) $(CPP_SRCS) $(C_OBJS) $(C_SRCS)
	@echo "generate $@"
	@$(CC) -o $@ $(C_OBJS) $(CPP_OBJS) $(LIB_PATH) $(LIB_NAME) -lm

%.c.o : %.c
	@echo "compile $@"
	@$(CC) -DENABLE_HDMI=$(ENABLE_HDMI)  $(C_FLAGS) $(INCLUDES) $(DEFINES) -g -c $< -o $@

%.cpp.o : %.cpp
	@echo "compile $@"
	@$(CPP) -DENABLE_HDMI=$(ENABLE_HDMI)  $(CPP_FLAGS) $(INCLUDES) $(DEFINES) -g -c $< -o $@
