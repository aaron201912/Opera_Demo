TOOLCHAIN_VERSION ?= 10.2.1
ifeq ($(TOOLCHAIN_VERSION),6.4.0)
CC := arm-linux-gnueabihf-gcc
CPP := arm-linux-gnueabihf-g++
endif
ifeq ($(TOOLCHAIN_VERSION),10.2.1)
CC := aarch64-linux-gnu-gcc
CPP := aarch64-linux-gnu-g++
endif
rpmsg_demo:./src/rpmsg_demo.c
	$(CC) -o ./bin/rpmsg_demo ./src/rpmsg_demo.c -I./inc -lpthread
clean:
	rm -rf ./bin/rpmsg_demo
