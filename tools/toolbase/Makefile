HOST_BIN_DIR ?= ../../bin/$(shell uname -s)/$(shell uname -m)
all:
	mkdir -p $(HOST_BIN_DIR)
	cp bsbmaker.sh $(HOST_BIN_DIR)/bsbmaker
	chmod +x $(HOST_BIN_DIR)/bsbmaker
check:
	python3 tests/test_rotation.py
clean:
	rm -f $(HOST_BIN_DIR)/bsbmaker
.PHONY: all check clean
