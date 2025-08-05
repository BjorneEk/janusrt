# Author: Gustaf Franzen <gustaffranzen@icloud.com>

include config.mk

CROSS    := aarch64-linux-gnu-
RT_CROSS := aarch64-none-elf-
ARCH     := arm64

# Repos
KERNEL_SRC  := https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
BUSYBOX_SRC := https://git.busybox.net/busybox

# Dependency versions
KERNEL_VERSION  := v6.9
BUSYBOX_VERSION := 1_36_1

# Paths
KERNEL_DIR  := kernel
BUSYBOX_DIR := busybox
INITRAMFS   := initramfs.cpio
ROOTFS_DIR  := rootfs
SHARED_DIR  := shared
USPACE_DIR  := userspace

# Artifacts
RT_BIN      := $(ROOTFS_DIR)/rt.bin
LOADER_BIN  := $(ROOTFS_DIR)/loader
CHECKER_BIN := $(ROOTFS_DIR)/checker
MODULE_KO   := $(ROOTFS_DIR)/rtcore.ko
KERNEL_IMG  := $(KERNEL_DIR)/arch/$(ARCH)/boot/Image
BUSYBOX_BIN := $(ROOTFS_DIR)/bin/busybox

.PHONY: all clean rtprog kernelmod userspace rootfs initramfs run

all: $(KERNEL_IMG) $(BUSYBOX_BIN) rtprog kernelmod userspace rootfs initramfs

# ---------------------------- KERNEL -----------------------------
#KMAKE_FLAGS := V=1 -j1
KMAKE_FLAGS := -j$$(nproc)
#KMAKE_FLAGS := -j1


$(KERNEL_DIR):
	@echo " [*]	- cloning kernel"
	git clone --depth=1 $(KERNEL_SRC) $@
#	cd $@ && git checkout $(KERNEL_VERSION)

kernel_prepare: $(KERNEL_DIR)
	@echo " [*]	- preparing kernel"
	cd $< && make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS) $(KMAKE_FLAGS) defconfig
	cd $< && make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS) $(KMAKE_FLAGS) modules_prepare
	cd $< && make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS) $(KMAKE_FLAGS) modules

kernel_config: $(KERNEL_DIR) kernel_prepare
	@echo " [*]	- configuring kernel"
	cd $< && \
		scripts/config --enable CONFIG_SERIAL_AMBA_PL011 && \
		scripts/config --enable CONFIG_SERIAL_AMBA_PL011_CONSOLE && \
		scripts/config --disable CONFIG_CPU_IDLE && \
		scripts/config --disable CONFIG_HOTPLUG_CPU

$(KERNEL_IMG): $(KERNEL_DIR)
	@echo " [*]	- building kernel"
	cd $< && \
		make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS) $(KMAKE_FLAGS)

# ---------------------------- BUSYBOX ----------------------------
$(BUSYBOX_DIR):
	@echo " [*]	- cloning busybox"
	git clone $(BUSYBOX_SRC) $@
#	cd $@ && git checkout $(BUSYBOX_VERSION)

$(BUSYBOX_BIN): $(BUSYBOX_DIR)
	@echo " [*]	- building busybox (static)"
	cd $< && make distclean && make defconfig
	cd $< && sed -i 's/^# CONFIG_STATIC is not set/CONFIG_STATIC=y/' .config
	cd $< && make oldconfig
	cd $< && make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS) -j$$(nproc)
	mkdir -p $(ROOTFS_DIR)/bin
	cp $</busybox $@
	cd $(ROOTFS_DIR)/bin && for cmd in sh mount echo insmod sleep; do ln -sf busybox $$cmd; done

# ---------------------------- MODULES ----------------------------
MAKE_MOD = $(MAKE) -C $(1)		\
	CROSS_COMPILE=$(2)		\
	ARCH=$(ARCH)			\
	KERNEL_DIR=../$(KERNEL_DIR)	\
	SHARED_DIR=../$(SHARED_DIR)	\
	ROOTFS_DIR=../$(ROOTFS_DIR)

CLEAN_MOD = $(MAKE) -C $(1)		\
	KERNEL_DIR=../$(KERNEL_DIR)	\
	clean

# ---------------------------- RT-PROG ----------------------------
rtprog:
	@echo " [*]	- building $@"
	$(call MAKE_MOD,$@,$(RT_CROSS))

# ---------------------------- USERSPACE --------------------------
userspace:
	@echo " [*]	- building $@"
	$(call MAKE_MOD,$@,$(CROSS))

# ---------------------------- KERNEL-MOD -------------------------
kernelmod: $(KERNEL_IMG)
	@echo " [*]	- building $@"
	$(call MAKE_MOD,$@,$(CROSS))

# ---------------------------- ROOTFS SETUP -----------------------
rootfs: rtprog userspace kernelmod
	@echo " [*]	- rootfs setup"
	mkdir -p $(addprefix $(ROOTFS_DIR)/,bin sbin etc proc sys dev)
	chmod +x $(ROOTFS_DIR)/init
	cp $(USPACE_DIR)/loader $(LOADER_BIN)
	cp kernelmod/rtcore.ko $(MODULE_KO)


# ---------------------------- INITRAMFS --------------------------
initramfs: rootfs  $(BUSYBOX_BIN)
	@echo " [*]	- creating initramfs"
	cd $(ROOTFS_DIR) && find . | cpio -H newc -o > ../$(INITRAMFS)

# ---------------------------- RUN-QEMU  --------------------------
run:
	./run.sh $(JRT_MEM_PHYS) $(JRT_MEM_SIZE)

clean:
	$(call CLEAN_MOD,rtprog)
	$(call CLEAN_MOD,userspace)
	$(call CLEAN_MOD,kernelmod)
	cd $(BUSYBOX_DIR) && make distclean || true
	$(RM) -rf $(addprefix $(ROOTFS_DIR)/,bin sbin etc proc sys dev)
	$(RM) -rf $(RT_BIN) $(LOADER_BIN) $(CHECKER_BIN) $(MODULE_KO) $(INITRAMFS)

