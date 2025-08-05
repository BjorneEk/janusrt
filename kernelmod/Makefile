# Author: Gustaf Franzen <gustaffranzen@icloud.com>

include ../config.mk

ROOTFS_DIR    ?= ../rootfs
KERNEL_DIR    ?= ../kernel
SHARED_DIR    ?= ../shared
ARCH          ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-

obj-m := rtcore.o psci.o

ccflags-y += \
	-I$(abspath $(SHARED_DIR)) \
	-DJRT_MEM_PHYS=$(JRT_MEM_PHYS) \
	-DJRT_MEM_SIZE=$(JRT_MEM_SIZE)

all:
	$(MAKE) -C $(KERNEL_DIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(shell pwd) \
		modules

clean:
	$(MAKE) -C $(KERNEL_DIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(PWD) \
		clean

