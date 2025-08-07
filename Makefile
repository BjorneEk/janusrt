# Author: Gustaf Franzen <gustaffranzen@icloud.com>

include config.mk

# Repos
KERNEL_SRC  := https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
BUSYBOX_SRC := https://git.busybox.net/busybox

# Dependency versions
KERNEL_VERSION  := v6.9
BUSYBOX_VERSION := 1_36_1

.PHONY: all clean rtprog kernelmod userspace rootfs initramfs run

all: $(KERNEL_IMG) $(BUSYBOX_BIN) rtprog kernelmod userspace rootfs initramfs

# ---------------------------- KERNEL -----------------------------
#KMAKE_FLAGS := V=1 -j1
KMAKE_FLAGS := -j$(shell  nproc)
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

#kernel_config_options := \
#	--disable CONFIG_CPU_IDLE \
#	--disable CONFIG_HOTPLUG_CPU \
#	--enable  CONFIG_DEBUG_INFO \
#	--enable  CONFIG_ARM_PSCI_FW \
#	--enable  CONFIG_SMP \
#	--enable  CONFIG_ARM64_PSCI
#
#kernel_config: $(KERNEL_DIR) kernel_prepare
#	@echo " [*]	- configuring kernel"
#	cd $< && for opt in $(kernel_config_options); do scripts/config $$opt; done
kernel_config: $(KERNEL_DIR) kernel_prepare
	@echo " [*]	- configuring kernel"
	cd $< && \
		scripts/config --disable CONFIG_CPU_IDLE && \
		scripts/config --disable CONFIG_HOTPLUG_CPU && \
		scripts/config --enable CONFIG_DEBUG_INFO=y && \
		scripts/config --enable CONFIG_ARM_PSCI_FW && \
		scripts/config --enable CONFIG_SMP && \
		scripts/config --enable CONFIG_ARM64_PSCI
#		scripts/config --enable CONFIG_SERIAL_AMBA_PL011 &&
#		scripts/config --enable CONFIG_SERIAL_AMBA_PL011_CONSOLE &&
#
$(KERNEL_IMG): $(KERNEL_DIR)
	@echo " [*]	- building kernel"
	cd $< && \
		make ARCH=$(ARCH) CROSS_COMPILE=$(LINUX_CROSS) $(KMAKE_FLAGS)

# ---------------------------- BUSYBOX ----------------------------
BUSYBOX_TOOLS:= \
	sh mount echo insmod sleep ps \
	top kill dmesg mount umount \
	grep sed awk poweroff shutdown \
	ls cp mv rm mkdir rmdir touch \
	cat head tail cut sort uniq wc mknod

BUSYBOX_CONFIGURED := $(BUSYBOX_DIR)/.config
BUSYBOX_BUILT := $(BUSYBOX_DIR)/busybox

$(BUSYBOX_DIR):
	@echo " [*]	- cloning busybox"
	git clone $(BUSYBOX_SRC) $@

$(BUSYBOX_CONFIGURED): $(BUSYBOX_DIR)
	@echo " [*] - configuring busybox (static)"
	cd $< && make distclean && make defconfig -j$(shell nproc)
	cd $< && sed -i 's/^# CONFIG_STATIC is not set/CONFIG_STATIC=y/' .config
	cd $< && make oldconfig -j$(shell nproc)
	touch $@

$(BUSYBOX_BUILT): $(BUSYBOX_CONFIGURED)
	@echo " [*] - building busybox"
	cd $(BUSYBOX_DIR) && make ARCH=$(ARCH) CROSS_COMPILE=$(LINUX_CROSS) -j$(shell nproc)

$(BUSYBOX_BIN): $(BUSYBOX_BUILT)
	@echo " [*] - installing busybox"
	mkdir -p $(ROOTFS_DIR)/bin
	cp $(BUSYBOX_BUILT) $@
	cd $(ROOTFS_DIR)/bin && for cmd in $(BUSYBOX_TOOLS); do ln -sf busybox $$cmd; done

# ---------------------------- MODULES ----------------------------
# ---------------------------- RT-PROG ----------------------------
rtprog: rootfs
	@echo " [*]	- building $@"
	$(MAKE) -C $@

# ---------------------------- USERSPACE --------------------------
userspace: rootfs
	@echo " [*]	- building $@"
	$(MAKE) -C $@

# ---------------------------- KERNEL-MOD -------------------------
kernelmod: $(KERNEL_IMG) rootfs
	@echo " [*]	- building $@"
	$(MAKE) -C $@

# ---------------------------- ROOTFS SETUP -----------------------
rootfs:
	@echo " [*]	- rootfs setup"
	mkdir -p $(addprefix $(ROOTFS_DIR)/,bin sbin etc proc sys dev)
	chmod +x $(ROOTFS_DIR)/init

# ---------------------------- INITRAMFS --------------------------
initramfs: rtprog userspace kernelmod rootfs $(BUSYBOX_BIN)
	@echo " [*]	- creating initramfs"
	cd $(ROOTFS_DIR) && find . | cpio -H newc -o > $(INITRAMFS)

# ---------------------------- DEVICE-TREE -------------------------
%.dtb: %.dts
	@echo " [*]	- compiling device tree blob ($@)"
	dtc -I dts -O dtb -o $@ $<

# ---------------------------- BUILD-CONTAINER ---------------------
DOCKER_FLAGS:=
.docker-image-stamp: $(DOCKERFILE) $(PACKAGE_LIST)
	@echo " [*]	- checking docker build context"
	@if [ "$(IN_CONTAINER)" = "0" ]; then \
		echo " [*]	- building docker image"; \
		docker build $(DOCKER_FLAGS) -t $(DOCKER_IMG) $(DOCKER_DIR) && \
			touch $@ && \
			docker create --name tc $(DOCKER_IMG) && \
			docker cp tc:/opt/toolchain-cache $(DOCKER_DIR) && \
			docker rm tc; \
	else \
		echo " [X]	- inside container, not building"; \
	fi

# ---------------------------- COMPILE ---------------------------
ifeq ($(IN_CONTAINER),0)
compile: .docker-image-stamp
else
compile: $(KERNEL_BIN) initramfs $(DEVTREE_BLOB)
endif
compile:
	@echo " [*]	- compiling jrt project"
	@if [ "$(IN_CONTAINER)" = "0" ]; then \
		echo " [*]	- invoking make in container..."; \
		docker run --rm -it \
			-v $(PROJECT_ROOT):/project \
			-w /project \
			$(DOCKER_IMG) \
			/bin/bash -c "make $(MAKECMDGOALS) $(MAKEFLAGS)"; \
	else \
		echo " [*]	- building in container"; \
	fi

run-container: .docker-image-stamp
	docker run --rm -it \
		-v $(PROJECT_ROOT):/project \
		-w /project \
		$(DOCKER_IMG) \
		bash

# ---------------------------- RUN-QEMU  --------------------------
run:
	./run.sh $(JRT_MEM_PHYS) $(JRT_MEM_SIZE) $(DEVTREE_BLOB)

clean:
	$(RM) -rf .docker-image-stamp
	$(RM) -rf $(DEVTREE_BLOB)
	cd $(BUSYBOX_DIR) && make distclean || true
	$(MAKE) -C kernelmod clean
	$(RM) -rf $(RT_BIN) $(LOADER_BIN) $(CHECKER_BIN) $(MODULE_KO) $(INITRAMFS)
	$(RM) -rf $(addprefix $(ROOTFS_DIR)/,bin sbin etc proc sys dev)
	$(MAKE) -C rtprog clean
	$(MAKE) -C userspace clean

