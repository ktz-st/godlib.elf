CC=m68k-atari-mintelf-gcc
AS=vasmm68k_mot
CMINI=../../../../libcmini
CFLAGS=-Os -Wall -mshort -mfastcall -m68000 -fpack-struct=2 -I../../../.. -I$(CMINI)/include
all: MULU.TOS
MULU.TOS: verify.c baseline.o shift.o ../../../libgod.a
	$(CC) $(CFLAGS) -nostdlib -nostartfiles -nodefaultlibs -static -Wl,--gc-sections -Wl,--relax -L$(CMINI)/lib/mshort/mfastcall -L../../.. -o $@ $(CMINI)/lib/mshort/mfastcall/crt0.o verify.c baseline.o shift.o -Wl,--start-group -lgod -lcmini -lgcc -Wl,--end-group
%.o: %.s
	$(AS) -Felf -devpac -I../.. $< -o $@
