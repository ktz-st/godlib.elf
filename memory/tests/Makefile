CC = m68k-atari-mintelf-gcc
HOST_CC ?= cc
CMINI = ../../../libcmini
HOST_CFLAGS = -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer
ifeq ($(shell uname -s),Darwin)
HOST_CFLAGS += -Wno-pragma-pack
endif
CFLAGS = -Os -Wall -mshort -mfastcall -m68000 -fpack-struct=2 -I../../.. -I$(CMINI)/include
LDFLAGS = -nostdlib -nostartfiles -nodefaultlibs -static -mshort -mfastcall -m68000 -Wl,--gc-sections -Wl,--relax -L$(CMINI)/lib/mshort/mfastcall -L../..
all: HEAPTEST.PRG
HEAPTEST.PRG: heap-test.c ../heap.h ../../libgod.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(CMINI)/lib/mshort/mfastcall/crt0.o $< -Wl,--start-group -lgod -lcmini -lgcc -Wl,--end-group
heap-host: heap-test.c ../heap.c ../heap.h
	$(HOST_CC) $(HOST_CFLAGS) -DHEAP_HOST -I../../.. heap-test.c ../heap.c -o $@
check-host: heap-host
	./heap-host
check: check-host HEAPTEST.PRG
	python3 verify-heap.py
clean:
	rm -f HEAPTEST.PRG heap-host RESULT.BIN
.PHONY: all check check-host clean
