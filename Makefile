# Author: Gustaf Franzen <gustaffranzen@icloud.com>

include config.mk

# Repos
KERNEL_SRC  := https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
BUSYBOX_SRC := https://git.busybox.net/busybox

# Dependency versions
KERNEL_VERSION  := v6.9
BUSYBOX_VERSION := 1_36_1

.PHONY: all clean kernel busybox rtprog kernelmod userspace rootfs initramfs run

all: compile
#all: kernel busybox rtprog kernelmod userspace rootfs initramfs $(DEVTREE_BLOB)

# ---------------------------- KERNEL -----------------------------
KMAKE_FLAGS :=

kernel:
	@echo " [*]	- make kernel"
	$(MAKE) -C $@ $(KMAKE_FLAGS)

# ---------------------------- BUSYBOX ----------------------------
busybox:
	@echo " [*]	- make busybox"
	$(MAKE) -C $@

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
kernelmod: kernel rootfs
	@echo " [*]	- building $@"
	$(MAKE) -C $@

# ---------------------------- ROOTFS SETUP -----------------------
rootfs:
	@echo " [*]	- rootfs setup"
	mkdir -p $(addprefix $(ROOTFS_DIR)/,bin sbin etc proc sys dev)
	chmod +x $(ROOTFS_DIR)/init

# ---------------------------- INITRAMFS --------------------------
initramfs: rtprog userspace kernelmod rootfs busybox
	@echo " [*]	- creating initramfs"
	cd $(ROOTFS_DIR) && find . | cpio -H newc -o > $(INITRAMFS)

# ---------------------------- DEVICE-TREE -------------------------
%.dtb: %.dts
	@echo " [*]	- compiling device tree blob ($@)"
	dtc -I dts -O dtb -o $@ $<

# ---------------------------- BUILD-CONTAINER ---------------------
DOCKER_FLAGS:=
.docker-image.stamp: $(DOCKERFILE) $(PACKAGE_LIST)
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
compile: .docker-image.stamp
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

run-container: .docker-image.stamp
	docker run --rm -it \
		-v $(PROJECT_ROOT):/project \
		-w /project \
		$(DOCKER_IMG) \
		bash

# ---------------------------- RUN-QEMU  --------------------------
run:
	./run.sh $(JRT_CODE_PHYS) $(JRT_CODE_SIZE) $(DEVTREE_BLOB)

softclean:
	$(RM) -rf $(RT_BIN) $(LOADER_BIN) $(CHECKER_BIN) $(MODULE_KO) $(INITRAMFS)
	$(RM) -rf $(addprefix $(ROOTFS_DIR)/,bin sbin etc proc sys dev)
	$(RM) -rf $(DEVTREE_BLOB)
	$(MAKE) -C kernelmod softclean
	$(MAKE) -C rtprog clean
	$(MAKE) -C userspace clean

clean:
	$(RM) -rf .docker-image.stamp
	$(RM) -rf $(RT_BIN) $(LOADER_BIN) $(CHECKER_BIN) $(MODULE_KO) $(INITRAMFS)
	$(RM) -rf $(addprefix $(ROOTFS_DIR)/,bin sbin etc proc sys dev)
	$(RM) -rf $(DEVTREE_BLOB)
	$(MAKE) -C $(KERNEL_DIR) $(KMAKE_FLAGS) clean
	$(MAKE) -C $(BUSYBOX_DIR) clean
	$(MAKE) -C kernelmod clean
	$(MAKE) -C rtprog clean
	$(MAKE) -C userspace clean

