TOOLCHAIN_VERSION ?=6.4.0
CROSS_COMPILE ?=arm-linux-gnueabihf-

CC  = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
AR  = $(CROSS_COMPILE)ar

ALKAID_PATH ?= ..
$(info mysdk=$(ALKAID_PATH))
INTERFACE_CUS3A = enable

dirs := ./source
OUTPUT_DIR := ./out
COM_FLAGS = -Wall -g -fPIC -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES -DEGL_DUMMY

C_FLAGS  = $(COM_FLAGS) -std=gnu11
CPP_FLAGS  = $(COM_FLAGS) -std=gnu++11

INCLUDES := -I./inc -I./inc/drm -I./inc/include
INCLUDES += -I$(ALKAID_PATH)/release/include


TARGET_NAME  := sstar_keystone

CPP_SRCS := $(foreach dir,$(dirs),$(wildcard $(dir)/*.cpp))
CPP_OBJS := $(foreach n,$(CPP_SRCS),$(addsuffix .cpp.o,$(basename ${n})))

C_SRCS := $(foreach dir,$(dirs),$(wildcard $(dir)/*.c))
C_OBJS := $(foreach n,$(C_SRCS),$(addsuffix .c.o,$(basename ${n})))

LIBS := -L$(ALKAID_PATH)/release/chip/p5/sigma_common_libs/glibc/6.4.0/dynamic
LIBS += -lcam_fs_wrapper -lmi_common -lcam_os_wrapper

LIBS += -L./lib/ffmpeg/arm/lib/glibc/6.4.0/static
LIBS += -lssplayer

LIBS += -L$(ALKAID_PATH)/release/chip/p5/dispcam/common/glibc/6.4.0/mi_libs/dynamic
LIBS += -lmi_sys
LIBS += -lmi_scl
LIBS += -lmi_venc
LIBS += -lmi_vdec
LIBS += -lmi_disp
LIBS += -lmi_hdmi
LIBS += -lmi_hvp
LIBS += -lmi_hdmirx  -lmi_isp -lmi_vif -lmi_sensor -lmi_iqserver

ifeq ($(INTERFACE_CUS3A), enable)
LIBS += -lcus3a -lispalgo
endif

LIBS += -L$(ALKAID_PATH)/release/chip/p5/dispcam/common/glibc/6.4.0/3rd_party_libs/dynamic
LIBS += -lmali -lEGL -lGLESv1_CM -lGLESv2

LIBS += -L./lib
LIBS += -lgpugfx
LIBS += -ldrm


LIBS += -L./lib/ffmpeg/arm/lib/glibc/6.4.0/dynamic
LIBS += -lavformat -lavcodec -lavutil -lswscale -lswresample



LIBS += -L./lib/alsa/arm/lib/glibc/6.4.0/dynamic
LIBS += -lswresample -lasound

LIBS += -L./lib/openssl/arm/lib/glibc/6.4.0/dynamic
LIBS += -lssl -lcrypto




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
	@echo "$(CPP) -o $@ $(C_OBJS) $(CPP_OBJS) $(LIBS) -lpthread"
	@echo
	@$(CPP) -o $@ $(C_OBJS) $(CPP_OBJS) $(LIBS) -lpthread



%.c.o : %.c
	@echo "compile $@"
	@$(CC) $(C_FLAGS) $(INCLUDES) -g -c $< -o $@

%.cpp.o : %.cpp
	@echo "compile $@"
	@$(CPP) $(CPP_FLAGS) $(INCLUDES) -g -c $< -o $@
