# Author: Gustaf Franzen <gustaffranzen@icloud.com>

CROSS_COMPILE ?= aarch64-none-elf-

ROOTFS_DIR ?= ../rootfs
SHARED_DIR ?= ../shared

SRC := rtprog.c
OBJ := $(patsubst %.c,%.o,$(SRC))
ELF := $(patsubst %.c,%.elf,$(SRC))
BIN := $(ROOTFS_DIR)/rt.bin
LDS := linker.ld

CFLAGS +=		\
	-nostdlib	\
	-ffreestanding	\
	-Wall		\
	-Werror		\
	-O3		\
	-I$(SHARED_DIR)

.PHONY: clean debug

all: debug $(BIN)

%.o: %.c
	$(CROSS_COMPILE)gcc -c $(CFLAGS) $@ $<

$(ELF): $(OBJ)
	$(CROSS_COMPILE)ld -T $(LDS) -o $@ $^

$(BIN): $(ELF)
	$(CROSS_COMPILE)objcopy -O binary $< $@

debug:
	@echo "SRC: $(SRC)"
	@echo "OBJ: $(OBJ)"
	@echo "ELF: $(ELF)"
	@echo "BIN: $(BIN)"
	@echo "LDS: $(LDS)"
	@echo "CFLAGS	$(CFLAGS)"
	
clean:
	$(RM) -f *.o *.elf $(BIN)
