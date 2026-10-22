CC=m68k-atari-mintelf-gcc
CMINI=../../../../libcmini
CFLAGS=-Os -Wall -fno-strict-aliasing -mshort -mfastcall -m68000 -fpack-struct=2 -I../../../.. -I$(CMINI)/include
all: WIDE.TOS
WIDE.TOS: verify.c ../../../libgod.a
	$(CC) $(CFLAGS) -nostdlib -nostartfiles -nodefaultlibs -static -L$(CMINI)/lib/mshort/mfastcall -L../../.. -o $@ $(CMINI)/lib/mshort/mfastcall/crt0.o verify.c -Wl,--start-group -lgod -lcmini -lgcc -Wl,--end-group
