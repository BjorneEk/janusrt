

JRT_MEM_PHYS:=0x50000000
JRT_MEM_SIZE:=0x01000000

LINUX_CROSS := aarch64-linux-gnu-
NONE_CROSS  := aarch64-none-elf-
ARCH        := arm64

# Paths
KERNEL_DIR  := $(abspath kernel)
BUSYBOX_DIR := $(abspath busybox)
INITRAMFS   := $(abspath initramfs.cpio)
ROOTFS_DIR  := $(abspath rootfs)
SHARED_DIR  := $(abspath shared)
USPACE_DIR  := $(abspath userspace)
KMOD_DIR    := $(abspath kernelmod)

# Artifacts
RT_BIN      := $(abspath $(ROOTFS_DIR)/rt.bin)
LOADER_BIN  := $(abspath $(ROOTFS_DIR)/loader)
CHECKER_BIN := $(abspath $(ROOTFS_DIR)/checker)
MODULE_KO   := $(abspath $(ROOTFS_DIR)/rtcore.ko)
KERNEL_IMG  := $(abspath $(KERNEL_DIR)/arch/$(ARCH)/boot/Image)
BUSYBOX_BIN := $(abspath $(ROOTFS_DIR)/bin/busybox)

COMMON_CFLAGS:= \
	-DJRT_MEM_PHYS=$(JRT_MEM_PHYS) \
	-DJRT_MEM_SIZE=$(JRT_MEM_SIZE)

export JRT_MEM_PHYS JRT_MEM_SIZE LINUX_CROSS \
	NONE_CROSS ARCH KERNEL_DIR BUSYBOX_DIR \
	INITRAMFS ROOTFS_DIR SHARED_DIR USPACE_DIR \
	KMOD_DIR RT_BIN LOADER_BIN CHECKER_BIN MODULE_KO \
	KERNEL_IMG BUSYBOX_BIN COMMON_CFLAGS



