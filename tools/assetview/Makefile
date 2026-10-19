HOST_BIN_DIR ?= ../../bin/$(shell uname -s)/$(shell uname -m)
APP = $(HOST_BIN_DIR)/GodLib AssetView.app
CC = clang
all:
ifeq ($(shell uname -s),Darwin)
	$(MAKE) build
else
	@echo "AssetView Cocoa application is available on macOS only."
endif
build:
	python3 prepare_demo.py
	mkdir -p "$(APP)/Contents/MacOS"
	$(CC) -fobjc-arc -Wall -Wextra -Wno-unused-parameter -O2 -framework Cocoa AssetView.m -o "$(APP)/Contents/MacOS/assetview"
	cp Info.plist "$(APP)/Contents/Info.plist"
	mkdir -p "$(APP)/Contents/Resources/Demo"
	cp demo/* "$(APP)/Contents/Resources/Demo/"
	codesign --force --sign - "$(APP)"
	cp launcher.sh "$(HOST_BIN_DIR)/assetview"
	chmod +x "$(HOST_BIN_DIR)/assetview"
check: all
	python3 tests.py
clean:
	rm -rf "$(APP)"
	rm -f "$(HOST_BIN_DIR)/assetview"
.PHONY: all build check clean
