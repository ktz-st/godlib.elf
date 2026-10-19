CC = m68k-atari-mintelf-gcc
CMINI = ../../../libcmini
CFLAGS = -Os -Wall -mshort -mfastcall -m68000 -fpack-struct=2 -I../../.. -I$(CMINI)/include
LDFLAGS = -nostdlib -nostartfiles -nodefaultlibs -static -mshort -mfastcall -m68000 -Wl,--gc-sections -Wl,--relax -L$(CMINI)/lib/mshort/mfastcall -L../..
all: REFTEST.PRG
REFTEST.PRG: verify.c ../reflect.h ../../libgod.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(CMINI)/lib/mshort/mfastcall/crt0.o $< -Wl,--start-group -lgod -lcmini -lgcc -Wl,--end-group
check: REFTEST.PRG
	python3 verify.py
clean:
	rm -f REFTEST.PRG RESULT.BIN
.PHONY: all check clean
