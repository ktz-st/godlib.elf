HOST_BIN_DIR ?= ../../bin/$(shell uname -s)/$(shell uname -m)
all:
	mkdir -p $(HOST_BIN_DIR)
	cp launcher.sh $(HOST_BIN_DIR)/febuild
	chmod +x $(HOST_BIN_DIR)/febuild
check:
	python3 tests/test_febuild.py
schema:
	python3 generate_schema.py
clean:
	rm -f $(HOST_BIN_DIR)/febuild tests/fixture.o tests/fixture.fed tests/FEDTEST.PRG
.PHONY: all check schema clean

ABI = -mshort -mfastcall -m68000 -fpack-struct=2
CC = m68k-atari-mintelf-gcc
CMINI = ../../../libcmini
CRT = $(CMINI)/lib/mshort/mfastcall/crt0.o
tests/fixture.fed: tests/fixture.txt febuild.py schema.json
	python3 febuild.py $< $@
tests/fixture.o: tests/fixture.s tests/fixture.fed
	cd tests && vasmm68k_mot -Felf -devpac fixture.s -o fixture.o
tests/FEDTEST.PRG: tests/native.c tests/fixture.o ../../libgod.a
	$(CC) $(ABI) -Os -Wall -I../../.. -I$(CMINI)/include -nostdlib -nostartfiles -nodefaultlibs -static -L../../ -L$(CMINI)/lib/mshort/mfastcall -Wl,--gc-sections -Wl,--relax -o $@ $(CRT) $< tests/fixture.o -Wl,--start-group -lgod -lcmini -lgcc -Wl,--end-group
check-native: check tests/FEDTEST.PRG
	python3 tests/verify-native.py
.PHONY: check-native
