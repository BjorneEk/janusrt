# Author: Gustaf Franzen <gustaffranzen@icloud.com>


CC := $(NONE_CROSS)gcc
LD := $(NONE_CROSS)ld
OBJCOPY := $(NONE_CROSS)objcopy

LIBC_DIR  := ../libc
LIBC_NAME := jrtlibc
LIBC      := $(LIBC_DIR)/lib$(LIBC_NAME).a

CFLAGS +=				\
	-nostdlib			\
	-ffreestanding			\
	-Wall				\
	-Werror				\
	-O0				\
	-g				\
	$(COMMON_CFLAGS)		\
	-I$(SHARED_DIR)			\
	-I$(LIBC_DIR)			\
	-I../ \
	-fpie				\
	-fno-plt \
	-fno-semantic-interposition	\
	-fvisibility=hidden

SRC := main.c
OBJ := main.o
BIN := $(ROOTFS_DIR)/bin/app.bin

LINK_OBJ := ../rtprog.elf
LINKER := linker.ld
ELF := app.elf

LDFLAGS := -T $(LINKER) -e main --gc-sections --build-id=none \
	--just-symbols=$(LINK_OBJ) -pie

.PHONY: clean

all: $(BIN)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

clean:
	$(RM) $(OBJ)
	$(RM) $(ELF)



