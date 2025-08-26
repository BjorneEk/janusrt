

JRT_MEM_PHYS:=0x51000000
JRT_MEM_SIZE:=0x01000000
JRT_CODE_PHYS:=0x50000000
JRT_CODE_SIZE:=0x01000000
# code (0x50000000-0x50FFFFFF)
# data (0x51000000-0x51FFFFFF)

LINUX_CROSS := aarch64-linux-gnu-
NONE_CROSS  := aarch64-none-elf-
ARCH        := arm64
TARGET      := qemu

# Paths
PROJECT_ROOT:= $(abspath $(shell pwd))
KERNEL_DIR  := $(abspath kernel)
BUSYBOX_DIR := $(abspath busybox)
INITRAMFS   := $(abspath initramfs.cpio)
ROOTFS_DIR  := $(abspath rootfs)
SHARED_DIR  := $(abspath shared)
USPACE_DIR  := $(abspath userspace)
KMOD_DIR    := $(abspath kernelmod)
DEVTREE_DIR := $(abspath devtree)
DOCKER_DIR  := $(abspath docker)

# Artifacts
RT_BIN      := $(abspath $(ROOTFS_DIR)/rt.bin)
LOADER_BIN  := $(abspath $(ROOTFS_DIR)/loader)
MODULE_KO   := $(abspath $(ROOTFS_DIR)/rtcore.ko)
KERNEL_IMG  := $(abspath $(KERNEL_DIR)/kernel/arch/$(ARCH)/boot/Image)
BUSYBOX_BIN := $(abspath $(ROOTFS_DIR)/bin/busybox)

DEVTREE_BLOB := $(DEVTREE_DIR)/$(TARGET).dtb

COMMON_CFLAGS:= \
	-DJRT_MEM_PHYS=$(JRT_MEM_PHYS) \
	-DJRT_MEM_SIZE=$(JRT_MEM_SIZE) \
	-DJRT_CODE_PHYS=$(JRT_CODE_PHYS) \
	-DJRT_CODE_SIZE=$(JRT_CODE_SIZE)

PACKAGE_LIST    := $(DOCKER_DIR)/packages.txt
DOCKER_IMG      := jrt-builder
DOCKERFILE      := $(DOCKER_DIR)/Dockerfile
TOOLCHAIN_CACHE := $(DOCKER_DIR)/toolchain-cache
IN_CONTAINER    := $(shell test -f /.dockerenv && echo 1 || echo 0)

export JRT_MEM_PHYS JRT_MEM_SIZE LINUX_CROSS \
	NONE_CROSS ARCH KERNEL_DIR BUSYBOX_DIR \
	INITRAMFS ROOTFS_DIR SHARED_DIR USPACE_DIR \
	KMOD_DIR RT_BIN LOADER_BIN CHECKER_BIN MODULE_KO \
	KERNEL_IMG BUSYBOX_BIN COMMON_CFLAGS PROJECT_ROOT \
	DOCKER_DIR DOCKERFILE DOCKER_IMG DEVTREE_BLOB \
	TOOLCHAIN_CACHE IN_CONTAINER TARGET PACKAGE_LIST \
	JRT_CODE_SIZE JRT_CODE_PHYS




