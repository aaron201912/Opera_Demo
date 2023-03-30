KERNAL_DIR ?= ../kernel
PWD := $(shell pwd)
obj-m := mdrv_timer.o
EXTRA_CFLAGS      +=  -Idrivers/sstar/include
EXTRA_CFLAGS      +=  -Idrivers/sstar/include/pioneer5
modules:
	$(MAKE) -C $(KERNAL_DIR) M=$(PWD) modules
	@make clear
clear:
	rm -f *.o *.cmd *.mod.c
	rm -rf *~ core .depend .tmp_versions Module.symvers modules.order -f
	rm -f .*ko.cmd .*.o.cmd .*.o.d
	rm -f *.unsigned

clean:
	rm -f mdrv_timer.ko