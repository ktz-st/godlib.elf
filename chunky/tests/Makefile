CC = m68k-atari-mintelf-gcc
CFLAGS = -Os -Wall -mshort -mfastcall -m68000 -fpack-struct=2 -I../../.. -I../../../libcmini/include
LDFLAGS = -nostdlib -nostartfiles -nodefaultlibs -static -mshort -mfastcall -m68000 -L../../../libcmini/lib/mshort/mfastcall -L../..
verify.tos: verify.o ../../libgod.a
	$(CC) $(LDFLAGS) -o $@ ../../../libcmini/lib/crt0.o verify.o -Wl,--start-group -lgod -lcmini -lgcc -Wl,--end-group
clean:
	rm -f verify.o verify.tos
.PHONY: clean
