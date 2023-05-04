CROSS_COMPILE ?=arm-linux-gnueabihf-
CC  = arm-linux-gnueabihf-gcc
CPP = arm-linux-gnueabihf-g++
AR  = $(CROSS_COMPILE)ar

ALKAID_PATH ?= /home/zekun.zhang/cust/P5_alkaid/dev_p5_bringup_new
dirs := ./src
dirs += ./internal/rgn

OUTPUT_DIR := ./out
$(info mysdk=$(ALKAID_PATH))
COM_FLAGS = -Wall -g -fPIC 
#COM_FLAGS += -mfloat-abi=hard
C_FLAGS  = $(COM_FLAGS) -std=gnu11
CPP_FLAGS  = $(COM_FLAGS) -std=gnu++11

INCLUDES  := -I./include -I$(ALKAID_PATH)/project/release/include   -I./include/libdrm  -I./include/algo -I./internal/rgn
INCLUDES  += -I./include/live555/UsageEnvironment/include
INCLUDES  += -I./include/live555/groupsock/include
INCLUDES  += -I./include/live555/liveMedia/include
INCLUDES  += -I./include/live555/BasicUsageEnvironment/include
INCLUDES  += -I./include/live555/mediaServer/include

TARGET_NAME  := Sensor_rtsp

CPP_SRCS := $(foreach dir,$(dirs),$(wildcard $(dir)/*.cpp))
CPP_OBJS := $(foreach n,$(CPP_SRCS),$(addsuffix .cpp.o,$(basename ${n})))

C_SRCS := $(foreach dir,$(dirs),$(wildcard $(dir)/*.c))
C_OBJS := $(foreach n,$(C_SRCS),$(addsuffix .c.o,$(basename ${n})))

LIB_PATH  := -L./lib -L./lib/algo -L$(ALKAID_PATH)/project/release/chip/p5/dispcam/common/glibc/6.4.0/mi_libs/dynamic  
LIB_PATH  += -L$(ALKAID_PATH)/project/release/chip/p5/sigma_common_libs/glibc/6.4.0/dynamic/

LIB_NAME := -lm  -llive555 -ldrm -lcus3a -lispalgo -lmi_iqserver -lmi_venc  -lmi_vdec -lmi_vif  -lmi_scl  -lmi_sensor  -lmi_isp  -lmi_ipu -lmi_rgn  -lmi_sys  -lmi_common -lsstar_algo -lsstar_algo_detection -ldl -lcam_os_wrapper -lcam_fs_wrapper -lpthread


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
	@$(CPP) -o $@ $(C_OBJS) $(CPP_OBJS) $(LIB_PATH) $(LIB_NAME) 

%.c.o : %.c
	@echo "compile $@"
	@$(CC) -DENABLE_HDMI=$(ENABLE_HDMI)  $(C_FLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

%.cpp.o : %.cpp
	@echo "compile $@"
	@$(CPP) -DENABLE_HDMI=$(ENABLE_HDMI)  $(CPP_FLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@
