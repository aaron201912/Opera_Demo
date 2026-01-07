#TOOLCHAIN_VERSION=10.2.1
TOOLCHAIN_VERSION ?=6.4.0
CHIP ?=SSD2386

CC  = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
AR  = $(CROSS_COMPILE)ar

ALKAID_PATH ?= ..
INTERFACE_CUS3A = enable
SSTAR_VDF = disable

dirs := ./ ./common
dirs += ./sensor ./rgn ./algo ./rtsp ./scl ./venc
OUTPUT_DIR := ./out
$(info mysdk=$(ALKAID_PATH))
COM_FLAGS = -Wall -g -fPIC
#COM_FLAGS += -mfloat-abi=hard
C_FLAGS  = $(COM_FLAGS) -std=gnu11
CPP_FLAGS  = $(COM_FLAGS) -std=gnu++11

INCLUDES  := -I$(ALKAID_PATH)/project/release/include -I$(ALKAID_PATH)/kernel/drivers/sstar/include -I./include -I$(ALKAID_PATH)/project/release/include/isp -I$(ALKAID_PATH)/project/release/include/isp/opera -I./include -I$(ALKAID_PATH)/project/release/include/ipu/opera
INCLUDES  += -I./common -I./sensor -I./rgn -I./algo -I./rtsp -I./scl -I./venc
INCLUDES  += -I./rtsp/live555/UsageEnvironment/include
INCLUDES  += -I./rtsp/live555/groupsock/include
INCLUDES  += -I./rtsp/live555/liveMedia/include
INCLUDES  += -I./rtsp/live555/BasicUsageEnvironment/include
INCLUDES  += -I./rtsp/live555/mediaServer/include

ifeq ($(SSTAR_VDF),enable)
dirs += ./vdf
INCLUDES  += -I./vdf
C_FLAGS += -DSSTAR_VDF
CPP_FLAGS += -DSSTAR_VDF
endif

TARGET_NAME  := Rtsp_sensor_demo

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

LIB_PATH  += -L$(ALKAID_PATH)/project/release/chip/p5/dispcam/common/glibc/$(TOOLCHAIN_VERSION)/mi_libs/dynamic
LIB_PATH  += -L$(ALKAID_PATH)/project/release/chip/p5/sigma_common_libs/glibc/$(TOOLCHAIN_VERSION)/dynamic/
LIB_NAME := -lm -llive555 -lmi_venc -lmi_isp -lmi_vif -lmi_sensor -lmi_iqserver -lmi_scl -lmi_sys -lmi_rgn -lmi_ipu -lmi_common -lsstaralgo_det -ldl -lcam_os_wrapper  -lcam_fs_wrapper -lpthread
ifeq ($(SSTAR_VDF),enable)
LIB_NAME += -lmi_vdf -lmi_shadow -lMD_LINUX -lOD_LINUX -lVG_LINUX -lmi_ive
endif

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
	@$(CC) $(C_FLAGS) $(INCLUDES) $(DEFINES) -g -c $< -o $@

%.cpp.o : %.cpp
	@echo "compile $@"
	@$(CPP) $(CPP_FLAGS) $(INCLUDES) $(DEFINES) -g -c $< -o $@