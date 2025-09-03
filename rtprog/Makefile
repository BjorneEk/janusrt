# Author: Gustaf Franzen <gustaffranzen@icloud.com>

#include ../config.mk

CC := $(NONE_CROSS)gcc
LD := $(NONE_CROSS)gcc
OBJCOPY := $(NONE_CROSS)objcopy

#CROSS_COMPILE ?= aarch64-none-elf-

#ROOTFS_DIR ?= ../rootfs
#SHARED_DIR ?= ../shared

SRC := rtprog.c boot.S psci.S timer_aarch64.S \
	irq.c timer.c uart.c sync.c alloc.c kerror.c \
	sched.c heap.c ctx_switch.S mmu.c mmu_el1.S irq_el1.S \
	gic.c
OBJ := rtprog.o boot.o psci.o timer_aarch64.o \
	irq.o timer.o uart.o sync.o alloc.o kerror.o \
	sched.o heap.o ctx_switch.o mmu.o mmu_el1.o irq_el1.o \
	gic.o
ELF := rtprog.elf
RELOC  := $(ELF:.elf=.o)

#BIN := $(ROOTFS_DIR)/rt.bin

LDS := linker.ld
GEN_LD := gen.ld

LIBC_DIR  := libc
LIBC_NAME := jrtlibc
LIBC      := $(LIBC_DIR)/lib$(LIBC_NAME).a

APP_DIR := app

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
	-fvisibility=default
#	-fpie				\
	-fno-semantic-interposition	\
	-fvisibility=hidden

#LDFLAGS += -Wl,-T,$(GEN_LD) -Wl,-pie -Wl,-z,noexecstack -L$(LIBC_DIR) -l$(LIBC_NAME)
#LDFLAGS += -Wl,-T,$(GEN_LD) -L$(LIBC_DIR) -l$(LIBC_NAME)
LDFLAGS += -no-pie \
	-Wl,-T,$(GEN_LD) \
	-Wl,-e,_start \
	-Wl,--build-id=none \
	-Wl,-Map,rtprog.map -L$(LIBC_DIR) -l$(LIBC_NAME) -fvisibility=default
JRT_CODE_HEX:=$(shell printf "0x%x" $(JRT_CODE_PHYS))

.PHONY: clean debug

all: debug $(RT_BIN) app

$(LIBC):
	$(MAKE) -C $(dir $@) $(notdir $@)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

$(GEN_LD): $(LDS)
	sed 's/JRT_CODE_PHYS/$(JRT_CODE_HEX)/g' $< > $@

#$(ELF): $(OBJ) $(GEN_LD)
#	$(LD) $(LDFLAGS) -T $(GEN_LD) $(OBJ) -o $@

$(ELF): $(OBJ) $(GEN_LD) $(LIBC)
	$(LD) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $@
	#$(LD) -r -o $@ $<


$(RT_BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

app: $(ELF)
	$(MAKE) -C $(APP_DIR)

debug:
	@echo "SRC: $(SRC)"
	@echo "OBJ: $(OBJ)"
	@echo "ELF: $(ELF)"
	@echo "LDS: $(LDS)"
	@echo "GEN_LD: $(GEN_LD)"
	@echo "CFLAGS	$(CFLAGS)"

clean:
	$(MAKE) -C $(LIBC_DIR) clean
	$(MAKE) -C $(APP_DIR) clean
	$(RM) $(GEN_LD)
	$(RM) *.o *.elf

