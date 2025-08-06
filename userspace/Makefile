# Author: Gustaf Franzen <gustaffranzen@icloud.com>

#include ../config.mk

CC:=$(LINUX_CROSS)gcc

SRC := $(wildcard *.c)
PROG := loader

CFLAGS :=			\
	-static			\
	-I$(SHARED_DIR)		\
	$(COMMON_CFLAGS)

.PHONY: clean debug

all: debug $(LOADER_BIN)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

$(LOADER_BIN): $(PROG)
	@cp $< $@

debug:
	@echo "SRC: $(SRC)"
	@echo "PROG: $(prog)"
	@echo "CFLAGS: $(CFLAGS)"

clean:
	$(RM) -rf $(BIN)

