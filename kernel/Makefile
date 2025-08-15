
KERNEL_SRC  := https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
KERNEL_STAMP:= $(KERNEL_DIR)/.kernel.stamp
KERNEL_CONFIG_STAMP:= $(KERNEL_DIR)/.kernel-config.stamp

KERNEL_REPO :=$(KERNEL_DIR)/kernel

# Build flags (no fragile backslashes)
KMAKE_FLAGS :=
KMAKE_FLAGS += ARCH=$(ARCH)
KMAKE_FLAGS += CROSS_COMPILE=$(CROSS)

# Detect V on the command line
ifeq ($(V),s)
  $(info Verbose mode: silent)
  KMAKE_FLAGS += -j1 V=$(V)
else ifneq ($(origin V),undefined)
  $(info V was passed but not "s": $(V))
  KMAKE_FLAGS += -j1 V=$(V)
else
  $(info V not passed at all, using nproc)
  KMAKE_FLAGS += -j$(shell nproc)
endif

.PHONY: all clean

all: $(KERNEL_IMG)

$(KERNEL_REPO):
	@echo " [*]	- cloning kernel"
	git clone --depth=1 $(KERNEL_SRC) $@
#	cd $@ && git checkout $(KERNEL_VERSION)

# Configure once, recorded by a stamp (repo is order-only)
$(KERNEL_CONFIG_STAMP): | $(KERNEL_REPO)
	@echo " [*]  - configuring kernel"
	$(MAKE) -C $(KERNEL_REPO) $(KMAKE_FLAGS) defconfig
	$(MAKE) -C $(KERNEL_REPO) $(KMAKE_FLAGS) modules_prepare
	cd $(KERNEL_REPO) && scripts/config \
		-d CPU_IDLE \
		-d HOTPLUG_CPU \
		-e DEBUG_INFO \
		-e ARM_PSCI_FW \
		-e SMP \
		-e ARM64_PSCI
	$(MAKE) -C $(KERNEL_REPO) $(KMAKE_FLAGS) olddefconfig
	touch $@

$(KERNEL_STAMP): $(KERNEL_CONFIG_STAMP)
	@echo " [*]  - building kernel"
	$(MAKE) -C $(KERNEL_REPO) $(KMAKE_FLAGS) Image modules && \
		touch $@

$(KERNEL_IMG): $(KERNEL_STAMP)

clean:
	$(RM) $(KERNEL_STAMP)
	$(RM) $(KERNEL_CONFIG_STAMP)
	@if [ -d "$(KERNEL_REPO)" ]; then \
		$(MAKE) -C $(KERNEL_REPO) $(KMAKE_FLAGS) clean; \
	fi

