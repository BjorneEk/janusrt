# Author: Gustaf Franzen <gustaffranzen@icloud.com>

include ../config.mk

CROSS_COMPILE ?= aarch64-none-elf-

ROOTFS_DIR ?= ../rootfs
SHARED_DIR ?= ../shared

SRC := rtprog.c
OBJ := $(patsubst %.c,%.o,$(SRC))
ELF := $(patsubst %.c,%.elf,$(SRC))
BIN := $(ROOTFS_DIR)/rt.bin
LDS := linker.ld

GEN_LD := gen.ld

CFLAGS +=		\
	-nostdlib	\
	-ffreestanding	\
	-Wall		\
	-Werror		\
	-O3		\
	-DJRT_MEM_PHYS=$(JRT_MEM_PHYS) \
	-DJRT_MEM_SIZE=$(JRT_MEM_SIZE) \
	-I$(SHARED_DIR)

JRT_MEM_HEX:=$(shell printf "0x%x" $(JRT_MEM_PHYS))

.PHONY: clean debug

all: debug $(BIN)

%.o: %.c
	$(CROSS_COMPILE)gcc -c $(CFLAGS) $@ $<

$(GEN_LD): $(LDS)
	sed 's/JRT_MEM_PHYS/$(JRT_MEM_HEX)/g' $< > $@

$(ELF): $(OBJ) $(GEN_LD)
	$(CROSS_COMPILE)ld -T $(GEN_LD) -o $@ $(OBJ)

$(BIN): $(ELF)
	$(CROSS_COMPILE)objcopy -O binary $< $@

debug:
	@echo "SRC: $(SRC)"
	@echo "OBJ: $(OBJ)"
	@echo "ELF: $(ELF)"
	@echo "BIN: $(BIN)"
	@echo "LDS: $(LDS)"
	@echo "GEN_LD: $(GEN_LD)"
	@echo "CFLAGS	$(CFLAGS)"

clean:
	$(RM) -f $(GEN_LD)
	$(RM) -f *.o *.elf $(BIN)

