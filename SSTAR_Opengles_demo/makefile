# SigmaStar trade secret
# Copyright (c) [2019~2020] SigmaStar Technology.
# All rights reserved.
#
# Unless otherwise stipulated in writing, any and all information contained
# herein regardless in any format shall remain the sole proprietary of
# SigmaStar and be kept in strict confidence
# (SigmaStar Confidential Information) by the recipient.
# Any unauthorized act including without limitation unauthorized disclosure,
# copying, use, reproduction, sale, distribution, modification, disassembling,
# reverse engineering and compiling of the contents of SigmaStar Confidential
# Information is unlawful and strictly prohibited. SigmaStar hereby reserves the
# rights to any and all damages, losses, costs and expenses resulting therefrom.
#

C_SRCS := $(foreach dir,$(CURDIR),$(wildcard $(CURDIR)/*.c))
C_OBJS := $(foreach n,$(C_SRCS),$(addsuffix .c.o,$(basename ${n})))
TARGET      :=gles_common

#compile and lib parameter
CC          :=$(CROSS_COMPILE)g++
A_LDFLAGS   :=
A_LIBS      :=
S_LDFLAGS   :=-L$(CURDIR)/lib

S_LIBS      :=-lgles_common -lmali -lEGL -lGLESv1_CM -lGLESv2 -ldrm -lpthread


A_INCLUDE   :=
S_INCLUDE   :=-I./include/libdrm -I./include/
CFLAGS      :=
CXXFLAGS    :=



$(TARGET): clean $(C_OBJS) $(C_SRCS)
	@echo "generate $@"
	@$(CC) *.o -o $@ $(S_LDFLAGS) $(S_LIBS) $(S_INCLUDE) -lm

%.c.o : %.c
	@echo "compile $@"
	@$(CC) -DENABLE_HDMI=$(ENABLE_HDMI) $(S_INCLUDE) $(DEFINES) -g -c $< -o $@

clean:
	rm -fr *.o
	rm -fr $(TARGET)
