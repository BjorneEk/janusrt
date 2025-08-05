# Author: Gustaf Franzen <gustaffranzen@icloud.com>

ROOTFS_DIR    ?= ../rootfs
KERNEL_DIR    ?= ../kernel
SHARED_DIR    ?= ../shared
ARCH          ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-

obj-m := rtcore.o

EXTRA_CFLAGS += -I$(abspath $(SHARED_DIR))
ccflags-y := -I$(abspath $(SHARED_DIR))
all:
	$(MAKE) -C $(KERNEL_DIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(shell pwd) \
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
		modules
install:
	cp rtcore.ko $(ROOTFS_DIR)/

clean:
	$(MAKE) -C $(KERNEL_DIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(PWD) \
		clean

