#依赖本地sdk路径

#Toolchain设置
ifeq ($(ARCH),arm)
CROSS_COMPILE ?=arm-linux-gnueabihf-
endif
ifeq ($(ARCH),arm64)
CROSS_COMPILE ?=aarch64-linux-gnu-
endif

CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

#配置源文件目录
PROJ_ROOT_PATH :=./

#源文件后缀名
PROJ_SRC_SUFFIX := %.cpp %.c
PROJ_HEAD_SUFFIX := %.h

#递归遍历目录下的所有的文件
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

#获取相应的源文件
PROJ_ALL_FILES := $(foreach src_path,$(PROJ_ROOT_PATH), $(call rwildcard,$(src_path),*.*) )
PROJ_SRC_FILES := $(filter $(PROJ_SRC_SUFFIX),$(PROJ_ALL_FILES))
PROJ_HEAD_FILES := $(filter $(PROJ_HEAD_SUFFIX),$(PROJ_ALL_FILES))

# 获取相应的源文件
LOCAL_SRC_FILES  := $(PROJ_SRC_FILES)
LOCAL_HEAD_FILES := $(PROJ_HEAD_FILES)

#源文件目录结构
PROJ_DIR_TREE := $(dir $(foreach src_path, $(LOCAL_SRC_FILES), $(call rwildcard,$(src_path),*/) ) )
PROJ_DIR_TREE := $(sort $(PROJ_DIR_TREE))

#obj文件目录结构
OBJS_OUTPUT_DIR= $(PROJ_ROOT_PATH)obj/
OBJS_1 = $(patsubst %.cpp,%.o,$(LOCAL_SRC_FILES))
OBJS_2 = $(patsubst %.c,%.o,$(OBJS_1))
OBJS   = $(addprefix $(OBJS_OUTPUT_DIR),$(OBJS_2))
OBJS_DIR_TREE=$(addprefix $(OBJS_OUTPUT_DIR),$(PROJ_DIR_TREE))

#bin文件目录
BIN_OUTPUT_DIR= $(PROJ_ROOT_PATH)bin

TARGET = mp3Player
CFLAGS = -Wno-write-strings
LDFLAGS =
CONFIG_PREFIX =.

ifeq ($(ARCH),arm)
LIB_VERSION ?= lib_32
endif
ifeq ($(ARCH),arm64)
LIB_VERSION ?= lib_64
endif

LOCAL_CFLAGS = -I./ffmpeg/include -I./sstar/include
LOCAL_LDFLAGS = -L./ffmpeg/$(LIB_VERSION) -L./sstar/$(LIB_VERSION)
LOCAL_LDFLAGS += -lpthread -ldl -lasound -lcam_os_wrapper -lavcodec -lavutil -lswresample -lavformat -lrt -fPIC

all: prepare $(TARGET) install
	@echo genrate $(TARGET) done!

$(TARGET):$(OBJS)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(LOCAL_CFLAGS) $(LOCAL_LDFLAGS) -o $@ $^

$(OBJS_OUTPUT_DIR)%.o: %.cpp $(LOCAL_HEAD_FILES)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(LOCAL_CFLAGS) $(LOCAL_LDFLAGS) -c $< -o $@

$(OBJS_OUTPUT_DIR)%.o: %.c $(LOCAL_HEAD_FILES)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(LOCAL_CFLAGS) $(LOCAL_LDFLAGS) -c $< -o $@

prepare:
	@echo generating target ...
	@-mkdir -p $(OBJS_DIR_TREE)
	@-mkdir -p $(BIN_OUTPUT_DIR)

install:
	@-mv $(TARGET) $(BIN_OUTPUT_DIR)

clean:
	@echo clean ...
	@-rm -rf $(OBJS_OUTPUT_DIR)
	@-rm -rf $(BIN_OUTPUT_DIR)
	@echo clean done!

.PHONY:all prepare clean $(TARGET)
