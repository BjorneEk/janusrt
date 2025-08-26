
# Author: Gustaf Franzen <gustaffranzen@icloud.com>

CC := gcc
CFLAGS := -DAUTOGEN_HEADER $(COMMON_CFLAGS)

HEADERS := memory_layout.h
AUTOGEN_DIR := autogen
AUTOGEN := $(addprefix $(AUTOGEN_DIR)/, $(HEADERS))
AUTOGEN_PROGS := $(patsubst %.h,%_autogen,$(HEADERS))
AUTOGEN_C := $(patsubst %.h,%_autogen.c,$(HEADERS))


.PHONY: all clean
all: $(AUTOGEN)

$(AUTOGEN_DIR):
	@mkdir -p $@

$(AUTOGEN_DIR)/%: % | $(AUTOGEN_DIR)
	@cp $< $(basename $*)_autogen.c
	$(CC) $(CFLAGS) $(basename $*)_autogen.c -o $(basename $*)_autogen
	@./$(basename $*)_autogen > $@
	$(RM) $(basename $*)_autogen
	$(RM) $(basename $*)_autogen.c

clean:
	$(RM) -rf $(AUTOGEN_DIR)
	$(RM) $(AUTOGEN_PROGS)
	$(RM) $(AUTOGEN_C)
