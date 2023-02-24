KDIR     := /lib/modules/$(shell uname -r)/build
MDIR     := $(shell pwd)
ARCH     := arm64
C_FLAGS  := -Wall -g
DEV_NAME := neil-dev

ccflags-y += $(C_FLAGS)

obj-m += $(DEV_NAME).o

all:
	make -C $(KDIR) M=$(MDIR) modules

install:
	insmod $(DEV_NAME).ko

uninstall:
	rmmod $(DEV_NAME).ko

clean:
	make -C $(KDIR) M=$(MDIR) clean
