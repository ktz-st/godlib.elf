CC=m68k-atari-mintelf-gcc
CMINI=../../../libcmini
CFLAGS=-Os -Wall -mshort -mfastcall -m68000 -fpack-struct=2 -I../../.. -I$(CMINI)/include
verify.tos: verify.c ../blitter.h ../../libgod.a
	$(CC) $(CFLAGS) -nostdlib -nostartfiles -nodefaultlibs -static -Wl,--gc-sections -Wl,--relax -L$(CMINI)/lib/mshort/mfastcall -L../.. -o $@ $(CMINI)/lib/mshort/mfastcall/crt0.o $< -Wl,--start-group -lgod -lcmini -lgcc -Wl,--end-group
