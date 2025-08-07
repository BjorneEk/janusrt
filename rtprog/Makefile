# Author: Gustaf Franzen <gustaffranzen@icloud.com>

#include ../config.mk

CC := $(NONE_CROSS)gcc
LD := $(NONE_CROSS)ld
OBJCOPY := $(NONE_CROSS)objcopy

#CROSS_COMPILE ?= aarch64-none-elf-

#ROOTFS_DIR ?= ../rootfs
#SHARED_DIR ?= ../shared

SRC := rtprog.c boot.S psci.S
OBJ := rtprog.o boot.o psci.o
ELF := rtprog.elf

#BIN := $(ROOTFS_DIR)/rt.bin

LDS := linker.ld
GEN_LD := gen.ld

CFLAGS +=			\
	-nostdlib		\
	-ffreestanding		\
	-Wall			\
	-Werror			\
	-O0			\
	-g			\
	$(COMMON_CFLAGS)	\
	-I$(SHARED_DIR)

JRT_MEM_HEX:=$(shell printf "0x%x" $(JRT_MEM_PHYS))

.PHONY: clean debug

all: debug $(RT_BIN)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

$(GEN_LD): $(LDS)
	sed 's/JRT_MEM_PHYS/$(JRT_MEM_HEX)/g' $< > $@

$(ELF): $(OBJ) $(GEN_LD)
	$(LD) -T $(GEN_LD) $(OBJ) -o $@

$(RT_BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

debug:
	@echo "SRC: $(SRC)"
	@echo "OBJ: $(OBJ)"
	@echo "ELF: $(ELF)"
	@echo "LDS: $(LDS)"
	@echo "GEN_LD: $(GEN_LD)"
	@echo "CFLAGS	$(CFLAGS)"

clean:
	$(RM) -f $(GEN_LD)
	$(RM) -f *.o *.elf

