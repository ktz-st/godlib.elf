CC ?= cc
HOST_STUB ?= $(shell uname -s)/$(shell uname -m)
HOST_BIN_DIR ?= ../../bin/$(HOST_STUB)
OUT ?= $(HOST_BIN_DIR)/imploder
CFLAGS ?= -O2 -Wall -Wextra -fno-strict-aliasing
SOURCES = src/imploder.c src/endian.c src/amiga_parser.c src/x68000_parser.c
GODLIB_ROOT := $(abspath ../..)
LIBCMINI ?= $(GODLIB_ROOT)/../libcmini
MINT_CC ?= m68k-atari-mintelf-gcc
NATIVE_DIR ?= /tmp/godlib-imploder
NATIVE_FLAGS = -Os -Wall -mshort -mfastcall -m68000 -fpack-struct=2 \
	-I$(GODLIB_ROOT)/.. -I$(LIBCMINI)/include

$(OUT): $(SOURCES) $(wildcard src/*.h)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(SOURCES) -o $@

test: $(OUT)
	python3 tests/verify.py $(abspath $(OUT))

native: $(OUT)
	python3 tests/verify.py $(abspath $(OUT)) --target-dir $(NATIVE_DIR) $(if $(MICRO_ROOT),--micro-root $(MICRO_ROOT))
	$(MINT_CC) $(NATIVE_FLAGS) -nostdlib -nostartfiles -nodefaultlibs -static \
		-Wl,--gc-sections -Wl,--relax -L$(LIBCMINI)/lib/mshort/mfastcall -L$(GODLIB_ROOT) \
		-o $(NATIVE_DIR)/IMPTEST.PRG $(LIBCMINI)/lib/mshort/mfastcall/crt0.o tests/native.c \
		-Wl,--start-group -lgod -lcmini -lgcc -Wl,--end-group

clean:
	rm -f $(OUT)

.PHONY: test native clean
