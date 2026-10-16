CC=m68k-atari-mintelf-gcc
CMINI=../../../../libcmini
CFLAGS=-Os -Wall -mshort -mfastcall -m68000 -fpack-struct=2 -I../../../.. -I$(CMINI)/include
PROFILE.TOS: profile.c ../../../libgod.a
	$(CC) $(CFLAGS) -nostdlib -nostartfiles -nodefaultlibs -static -Wl,--gc-sections -L$(CMINI)/lib/mshort/mfastcall -L../../.. -Wl,-Map=profile.map -o $@ $(CMINI)/lib/mshort/mfastcall/crt0.o profile.c -Wl,--start-group -lgod -lcmini -lgcc -Wl,--end-group
