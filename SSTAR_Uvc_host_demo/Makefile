#TOOLCHAIN_VERSION=10.2.1
TOOLCHAIN_VERSION ?=6.4.0
CHIP ?=SSD2386
ENABLE_LDC=0
ENABLE_RTSP ?=0
ENABLE_DRM ?=0
ENABLE_UVC_HOST ?=0

CC  = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
AR  = $(CROSS_COMPILE)ar

ALKAID_PATH ?= ..
INTERFACE_CUS3A = enable
dirs := ./ ./common
dirs += ./drm ./scl ./vdec ./uvc_host
OUTPUT_DIR := ./out
$(info mysdk=$(ALKAID_PATH))
COM_FLAGS = -Wall -g -fPIC
#COM_FLAGS += -mfloat-abi=hard
C_FLAGS  = $(COM_FLAGS) -std=gnu11
CPP_FLAGS  = $(COM_FLAGS) -std=gnu++11

INCLUDES  := -I./common -I./drm -I./drm/libdrm -I./scl -I./vdec -I./uvc_host
INCLUDES  += -I$(ALKAID_PATH)/project/release/include -I$(ALKAID_PATH)/kernel/drivers/sstar/include -I./include -I$(ALKAID_PATH)/project/release/include/isp -I$(ALKAID_PATH)/project/release/include/isp/opera -I./include -I$(ALKAID_PATH)/project/release/include/ipu/opera

TARGET_NAME  := uvc_host_demo

CPP_SRCS := $(foreach dir,$(dirs),$(wildcard $(dir)/*.cpp))
CPP_OBJS := $(foreach n,$(CPP_SRCS),$(addsuffix .cpp.o,$(basename ${n})))

C_SRCS := $(foreach dir,$(dirs),$(wildcard $(dir)/*.c))
C_OBJS := $(foreach n,$(C_SRCS),$(addsuffix .c.o,$(basename ${n})))

ifeq ($(TOOLCHAIN_VERSION),10.2.1)
LIB_PATH  := -L./lib/lib_64 -L./lib/lib_64/algolib
endif

ifeq ($(TOOLCHAIN_VERSION),6.4.0)
LIB_PATH  := -L./lib/lib_32 -L./lib/lib_32/algolib
endif

ifeq ($(TOOLCHAIN_VERSION),10.2.1)
CROSS_COMPILE ?=aarch64-linux-gnu-
endif

ifeq ($(TOOLCHAIN_VERSION),6.4.0)
CROSS_COMPILE ?=arm-linux-gnueabihf-
endif

ifeq ($(CHIP),SSD2386)
C_FLAGS += -DCHIP_IS_SSD2386
CPP_FLAGS += -DCHIP_IS_SSD2386
else ifeq  ($(CHIP),SSU9383)
C_FLAGS += -DCHIP_IS_SSU9383
CPP_FLAGS += -DCHIP_IS_SSU9383
else ifeq  ($(CHIP),SSD2381)
C_FLAGS += -DCHIP_IS_SSD2381
CPP_FLAGS += -DCHIP_IS_SSD2381
endif

ifeq ($(ENABLE_RTSP),1)
C_FLAGS += -DRTSP_ENABLE
CPP_FLAGS += -DRTSP_ENABLE
endif

ifeq ($(ENABLE_DRM),1)
C_FLAGS += -DDRM_ENABLE
CPP_FLAGS += -DDRM_ENABLE
endif

ifeq ($(ENABLE_UVC_HOST),1)
C_FLAGS += -DUVC_HOST_ENABLE
CPP_FLAGS += -DUVC_HOST_ENABLE
endif

LIB_PATH  += -L$(ALKAID_PATH)/project/release/chip/p5/dispcam/common/glibc/$(TOOLCHAIN_VERSION)/mi_libs/dynamic
LIB_PATH  += -L$(ALKAID_PATH)/project/release/chip/p5/sigma_common_libs/glibc/$(TOOLCHAIN_VERSION)/dynamic/
LIB_NAME := -lm -ldrm -lmi_vdec -lmi_isp -lmi_vif -lmi_sensor -lmi_scl -lmi_sys -lmi_jpd -lmi_ipu -lmi_common -ldl -lcam_os_wrapper -lcam_fs_wrapper -lpthread
#-lmi_hdmirx
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
	@$(CPP) -o $@ $(C_OBJS) $(CPP_OBJS) $(LIB_PATH) $(LIB_NAME) -lm

%.c.o : %.c
	@echo "compile $@"
	@$(CC) -DENABLE_HDMI=$(ENABLE_HDMI)  $(C_FLAGS) $(INCLUDES) $(DEFINES) -g -c $< -o $@

%.cpp.o : %.cpp
	@echo "compile $@"
	@$(CPP) -DENABLE_HDMI=$(ENABLE_HDMI)  $(CPP_FLAGS) $(INCLUDES) $(DEFINES) -g -c $< -o $@
