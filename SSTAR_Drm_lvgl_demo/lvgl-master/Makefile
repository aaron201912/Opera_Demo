#
# Makefile
#
#CC := gcc
#CC := aarch64-linux-gnu-gcc
TOOLCHAIN_VERSION ?= 6.4.0
ifeq ($(TOOLCHAIN_VERSION),6.4.0)
CC := arm-linux-gnueabihf-gcc
CPP := arm-linux-gnueabihf-g++
endif
ifeq ($(TOOLCHAIN_VERSION),10.2.1)
CC := aarch64-linux-gnu-gcc
CPP := aarch64-linux-gnu-g++
endif
#CC := aarch64-linux-android30-clang
LVGL_DIR ?= .
LVGL_DIR_NAME ?= lvgl
CHIP ?= SSU9383

#WARNINGS = -Werror -Wall -Wextra \
#           -Wshadow -Wundef -Wmaybe-uninitialized -Wmissing-prototypes -Wpointer-arith -Wuninitialized \
#           -Wunreachable-code -Wreturn-type -Wmultichar -Wformat-security -Wdouble-promotion -Wclobbered -Wdeprecated  \
#           -Wempty-body -Wstack-usage=2048 \
#           -Wtype-limits -Wsizeof-pointer-memaccess

#-Wno-unused-value -Wno-unused-parameter 
ifeq ($(TOOLCHAIN_VERSION),6.4.0)
OPTIMIZATION ?= -O3 -mfpu=neon #-g -fno-omit-frame-pointer
endif
ifeq ($(TOOLCHAIN_VERSION),10.2.1)
OPTIMIZATION ?= -O3 #-g -fno-omit-frame-pointer
endif
#-OPTIMIZATION ?= Wall -g -O2 -fPIC -DUSE_DRM -std=gnu11 
#ALKAID_PROJ := /home/alan.ma/P3/project
#ALKAID_PROJ := /home/alan.ma/P5_TEST_CODE_1024/project
ALKAID_PROJ := /home/peng.xiong/P5_BGA19_CODE_MERGE_01060921/project
#include $(ALKAID_PROJ)/configs/current.configs

CFLAGS ?= -I$(LVGL_DIR)/ $(DEFINES) $(WARNINGS) $(OPTIMIZATION)
CFLAGS += -I$(LVGL_DIR)/$(LVGL_DIR_NAME)
CFLAGS += -I$(LVGL_DIR)/lv_drivers/indev/
CFLAGS += -I$(LVGL_DIR)/lvgl/demos/
CFLAGS += -I$(LVGL_DIR)/lv_porting_sstar/
CFLAGS += -I$(LVGL_DIR)/squareline_proj

#CFLAGS += -I$(PROJ_ROOT)/release/include/
CFLAGS += -I$(ALKAID_PROJ)/release/include -I../include/ -I../include/libdrm -I../include/ffmpeg/include -I./lvgl/demos/sstar_test/Algo_detection_sensor_demo/src
INCLUDES := -I$(ALKAID_PROJ)/release/include  -I../include/ -I../include/libdrm  -I../include/algo -I./lvgl/demos/sstar_test/Algo_detection_sensor_demo/internal/rgn -I $(ALKAID_PROJ)/kbuild/5.10/drivers/sstar/cam_os_wrapper/pub

#LDFLAGS += -L $(ALKAID_PROJ)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/mi_libs/dynamic
#LDFLAGS += -L $(ALKAID_PROJ)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/dynamic

ifeq ($(TOOLCHAIN_VERSION),6.4.0)
LIB_PATH ?= lib_32
endif
ifeq ($(TOOLCHAIN_VERSION),10.2.1)
LIB_PATH ?= lib_64
endif

LDFLAGS  := -L ../$(LIB_PATH) -L ../$(LIB_PATH)/alsalib -L ../$(LIB_PATH)/ffmpeglib -L ../$(LIB_PATH)/algolib -L $(ALKAID_PROJ)/release/chip/p5/sigma_common_libs/glibc/$(TOOLCHAIN_VERSION)/dynamic -L $(ALKAID_PROJ)/release/chip/p5/dispcam/common/glibc/$(TOOLCHAIN_VERSION)/mi_libs/dynamic 
#LDFLAGS  += -L$(ALKAID_PROJ)/release/chip/i7/sigma_common_libs/llvm/12.0.5/dynamic/

#LDFLAGS += -lasound  -lavcodec -lavutil -lswresample -lavformat -lrt -fPIC -lmi_iqserver -lmi_venc -lmi_ipu -lmi_rgn -lsstar_algo -lsstar_algo_detection
#LDFLAGS += -lpthread -lm  -lcam_os_wrapper -lcam_fs_wrapper -lmi_vdec -lmi_sys  -lmi_isp -lmi_vif -lmi_sensor -lmi_scl -lmi_common -ldl -ldrm 
#LDFLAGS += -lm  -ldrm -lmi_iqserver  -lmi_venc -lmi_vif  -lmi_scl  -lmi_sensor  -lmi_isp  -lmi_ipu -lmi_rgn  -lmi_vdec  -lmi_sys  -lmi_common -lsstar_algo -lsstar_algo_detection -ldl  -lcam_fs_wrapper 
#LDFLAGS += -lasound  -lavcodec -lavutil -lswresample -lavformat -lrt -fPIC -lpthread 
 
LDFLAGS += -lm  -ldrm  -lcam_fs_wrapper -lmi_iqserver -lmi_venc -lmi_vdec -lmi_vif  -lmi_scl  -lmi_sensor  -lmi_isp  -lmi_ipu -lmi_rgn  -lmi_sys  -lmi_common -lsstar_algo -lsstar_algo_detection 
LDFLAGS += -lasound  -lavcodec -lavutil -lswresample -lavformat -lrt -fPIC -ldl -lpthread  -lcus3a -lispalgo

#LDFLAGS += -lpthread
#LDFLAGS += -lcam_os_wrapper -lcam_fs_wrapper -lmi_sys -lmi_common -lmi_panel -lmi_disp -lmi_gfx -lm 

COM_FLAGS = -Wall -g -fPIC 
CPP_FLAGS  = $(COM_FLAGS) -std=gnu++11

ifeq ($(CHIP),SSU9383)
CFLAGS   += -DCHIP_IS_SSU9383
CPP_FLAGS += -DCHIP_IS_SSU9383
else ifeq ($(CHIP),SSD2386)
CFLAGS   += -DCHIP_IS_SSD2386
CPP_FLAGS += -DCHIP_IS_SSD2386
endif

BIN ?= demo

#Collect the files to compile

include lvgl/lvgl.mk
include lv_drivers/lv_drivers.mk
include lv_porting_sstar/lv_porting_sstar.mk
include squareline_proj/squareline_proj.mk

CSRCS += main.c

BUILD_DIR := $(LVGL_DIR)/build/
OBJ_DIR = $(BUILD_DIR)/obj/
BIN_DIR = $(BUILD_DIR)/bin/

OBJEXT ?= .o
AOBJS = $(addprefix $(OBJ_DIR),$(ASRCS:.S=$(OBJEXT)))
COBJS = $(addprefix $(OBJ_DIR),$(CSRCS:.c=$(OBJEXT)))
CPPOBJS = $(addprefix $(OBJ_DIR),$(CPP_SRCS:.cpp=$(OBJEXT)))
DEPS = $(addprefix $(OBJ_DIR),$(CSRCS:.c=.d))


SRCS = $(ASRCS) $(CSRCS)
OBJS = $(AOBJS) $(COBJS) $(CPPOBJS)

.PHONY: all env clean default
all: env default

-include $(DEPS)

env:
	@echo "Building env......."
	@if [ ! -d "$(BUILD_DIR)" ]; then mkdir -p $(BUILD_DIR); fi
	@if [ ! -d "$(OBJ_DIR)" ]; then mkdir -p $(OBJ_DIR); fi
	@if [ ! -d "$(BIN_DIR)" ]; then mkdir -p $(BIN_DIR); fi

$(OBJ_DIR)main.o: main.c
	@echo "Compiling $(LVGL_DIR)/$<"
	@$(CC)  $(CFLAGS) -c $< -o $@ 

$(OBJ_DIR)%.o: %.c
	@echo "$(CC) Compiling $<"
	@mkdir -p $(OBJ_DIR)$(dir $<)
	@$(CC)  $(CFLAGS) -MMD -c $< -o $@

$(OBJ_DIR)%.o: %.cpp
	@echo "$(CPP) Compiling $<"
	@mkdir -p $(OBJ_DIR)$(dir $<)
	@$(CPP) -DENABLE_HDMI=$(ENABLE_HDMI) $(CPP_FLAGS) $(INCLUDES) -c $< -o $@
	
default: $(OBJS)
	@echo "Linking $(BIN)"
	@$(CC) -o $(BIN_DIR)$(BIN) $(OBJS) $(LDFLAGS)  $(ALKAID_PROJ)/release/chip/p5/sigma_common_libs/glibc/$(TOOLCHAIN_VERSION)/static/libcam_os_wrapper.a

clean:
	rm build -r

