
# Author: Gustaf Franzen <gustaffranzen@icloud.com>

CC := $(NONE_CROSS)gcc
AR := $(NONE_CROSS)ar

SRC := memcpy.c memset.c memcmp.c snprintf.c
OBJ := $(patsubst %.c,%.o,$(patsubst %.S,%.o,$(SRC)))


CFLAGS +=				\
	-nostdlib			\
	-ffreestanding			\
	-Wall				\
	-Werror				\
	-O3				\
	-g				\
	$(COMMON_CFLAGS)		\
	-I$(SHARED_DIR)			\
	-I$(LIBC_DIR)			\
	-fpie				\
	-fno-semantic-interposition	\
	-fvisibility=hidden

ARFLAGS := rcs

.PHONY: clean debug

all: debug

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.a: $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

debug:
	@echo "SRC: $(SRC)"
	@echo "OBJ: $(OBJ)"
	@echo "CFLAGS	$(CFLAGS)"

clean:
	$(RM) $(OBJ)
	$(RM) *.a

