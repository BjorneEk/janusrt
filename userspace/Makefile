# Author: Gustaf Franzen <gustaffranzen@icloud.com>

CROSS_COMPILE ?= aarch64-linux-gnu-

SHARED_DIR ?= ../shared
ROOTFS_DIR ?= ../rootfs

SRC := $(wildcard *.c)
BIN := $(basename $(SRC))

#BIN := $(addprefix $(ROOTFS_DIR)/bin/, $(basename $(SRC)))

CFLAGS := -static -I$(SHARED_DIR)

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

