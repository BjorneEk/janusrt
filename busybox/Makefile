BUSYBOX_SRC:=https://git.busybox.net/busybox
BUSYBOX_STAMP:=$(BUSYBOX_DIR)/.busybox.stamp

BUSYBOX_REPO:=$(BUSYBOX_DIR)/busybox

BUSYBOX_CONFIGURED:=$(BUSYBOX_REPO)/.config
BUSYBOX_BUILT:=$(BUSYBOX_REPO)/busybox

.PHONY: all clean

all: copy_tools

$(BUSYBOX_REPO):
	@echo " [*]	- cloning busybox"
	git clone $(BUSYBOX_SRC) $@

$(BUSYBOX_CONFIGURED): | $(BUSYBOX_REPO)
	@echo " [*] - configuring busybox (static)"
	cd $(BUSYBOX_REPO) && \
		make distclean && make defconfig -j$(shell nproc)
	cd $(BUSYBOX_REPO) && \
		sed -i 's/^# CONFIG_STATIC is not set/CONFIG_STATIC=y/' .config
	cd $(BUSYBOX_REPO) && \
		make oldconfig -j$(shell nproc)
	touch $@

$(BUSYBOX_BUILT): $(BUSYBOX_CONFIGURED)
	@echo " [*] - building busybox"
	cd $(BUSYBOX_REPO) && \
		make ARCH=$(ARCH) CROSS_COMPILE=$(LINUX_CROSS) -j$(shell nproc)

BUSYBOX_TOOLS:= \
	sh mount echo insmod sleep ps \
	top kill dmesg mount umount \
	grep sed awk poweroff shutdown \
	ls cp mv rm mkdir rmdir touch \
	cat head tail cut sort uniq wc mknod

$(BUSYBOX_STAMP): $(BUSYBOX_BUILT)
	@echo " [*] - installing busybox"

copy_tools: $(BUSYBOX_STAMP)
	mkdir -p $(ROOTFS_DIR)/bin
	cp $(BUSYBOX_BUILT) $(BUSYBOX_BIN)
	cd $(ROOTFS_DIR)/bin && \
		for cmd in $(BUSYBOX_TOOLS); do \
			ln -sf busybox $$cmd; \
		done && \
		touch $@

clean:
	$(RM) -rf $(BUSYBOX_STAMP)
	cd $(BUSYBOX_REPO) && make distclean


