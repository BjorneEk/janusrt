# Author: Gustaf Franzen <gustaffranzen@icloud.com>

include ../config.mk

CROSS_COMPILE ?= aarch64-linux-gnu-

SHARED_DIR ?= ../shared
ROOTFS_DIR ?= ../rootfs

SRC := $(wildcard *.c)
BIN := $(basename $(SRC))

#BIN := $(addprefix $(ROOTFS_DIR)/bin/, $(basename $(SRC)))

CFLAGS := \
	-static \
	-I$(SHARED_DIR) \
	-DJRT_MEM_PHYS=$(JRT_MEM_PHYS) \
	-DJRT_MEM_SIZE=$(JRT_MEM_SIZE)

.PHONY: clean debug

all: debug $(BIN)

$(ROOTFS_DIR)/bin/%: %.c $(SHARED_DEPS)
	$(CROSS_COMPILE)gcc $(CFLAGS) -o $@ $<

debug:
	@echo "SRC: $(SRC)"
	@echo "BIN: $(BIN)"
	@echo "CFLAGS: $(CFLAGS)"

clean:
	$(RM) -rf $(BIN)

