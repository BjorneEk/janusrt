# Author: Gustaf Franzen <gustaffranzen@icloud.com>

#include ../config.mk

#ROOTFS_DIR    ?= ../rootfs
#KERNEL_DIR    ?= ../kernel
#SHARED_DIR    ?= ../shared
#ARCH          ?= arm64
#CROSS_COMPILE ?= aarch64-linux-gnu-

MOD         := rtcore.ko
obj-m       := rtcore.o
rtcore-objs := rtmain.o psci.o psci_arm64.o

ccflags-y += \
	-I$(abspath $(SHARED_DIR)) \
	$(COMMON_CFLAGS)

KMAKE_FLAGS := \
	ARCH=$(ARCH) \
	CROSS_COMPILE=$(LINUX_CROSS) \
	M=$(shell pwd)

all: $(MODULE_KO)

$(MOD):
	$(MAKE) -C $(KERNEL_DIR)/kernel $(KMAKE_FLAGS) modules

$(MODULE_KO): $(MOD)
	@cp $< $@

clean:
	$(MAKE) -C $(KERNEL_DIR)/kernel $(KMAKE_FLAGS) clean

