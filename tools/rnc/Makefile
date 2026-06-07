CC ?= cc

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)
HOST_STUB ?= $(UNAME_S)/$(UNAME_M)
HOST_BIN_DIR ?= ../../bin/$(HOST_STUB)
OUT ?= $(HOST_BIN_DIR)/rnc64

CFLAGS ?= -O3 -Wall -Wextra -fno-strict-aliasing

$(OUT): main.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OUT)
