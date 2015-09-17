.PHONY: clean
obj-m := fpga_irq_drv.o
KDIR  := /opt/linux-3.0.35-initial
PWD   := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.ko *.o *.mod.* .H* .tm* .*cmd *.order Module.symvers
