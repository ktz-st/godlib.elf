ROOT = ../..
ABI = -mshort -mfastcall -m68000 -fpack-struct=2
CC = m68k-atari-mintelf-gcc
CMINI = ../../../libcmini
HOST_BIN_DIR ?= ../../bin/$(shell uname -s)/$(shell uname -m)
all: $(HOST_BIN_DIR)/cutbuild
$(HOST_BIN_DIR)/cutbuild: launcher.sh cutbuild.py
	mkdir -p $(HOST_BIN_DIR)
	cp launcher.sh $@
	chmod +x $@
check:
	python3 tests/test_cutbuild.py
tests/fixture.cut: tests/fixture.txt cutbuild.py $(ROOT)/cutscene/cutscene.c $(ROOT)/cutscene/cutscene.h
	python3 cutbuild.py $< $@
tests/assets.o: tests/assets.s tests/fixture.cut
	cd tests && vasmm68k_mot -Felf -devpac assets.s -o assets.o
tests/CUTBUILD.PRG: tests/native.c tests/assets.o $(ROOT)/libgod.a
	$(CC) $(ABI) -Wall -Os -I../../.. -I$(CMINI)/include -nostdlib -nostartfiles -nodefaultlibs -static -Wl,--gc-sections -Wl,--relax -L$(CMINI)/lib/mshort/mfastcall -L../.. -o $@ $(CMINI)/lib/mshort/mfastcall/crt0.o $< tests/assets.o -Wl,--start-group -lgod -lcmini -lgcc -Wl,--end-group
check-native: check tests/CUTBUILD.PRG
	python3 tests/verify-native.py
clean:
	rm -f $(HOST_BIN_DIR)/cutbuild tests/assets.o tests/CUTBUILD.PRG tests/fixture.cut
.PHONY: all check check-native clean
