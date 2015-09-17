.PHONY: clean
obj-m := hello.o
#KDIR  := /lib/modules/$(shell uname -r)/build
KDIR  := /opt/linux-3.0.35-initial
PWD   := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.ko *.o *.mod.* .H* .tm* .*cmd Module.symvers *.order
