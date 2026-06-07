CC ?= cc

GODLIB_DIR ?= ../..
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)
HOST_STUB ?= $(UNAME_S)/$(UNAME_M)
HOST_BIN_DIR ?= ../../bin/$(HOST_STUB)
OUT ?= $(HOST_BIN_DIR)/godpack

CPPFLAGS += -I$(GODLIB_DIR)/..
CFLAGS ?= -O2 -Wall -g -fno-strict-aliasing
LDFLAGS +=
LDLIBS += $(GODLIB_DIR)/libgod_host.a

$(OUT): main_host.c $(GODLIB_DIR)/libgod_host.a
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) main_host.c $(LDFLAGS) $(LDLIBS) -o $@

$(GODLIB_DIR)/libgod_host.a:
	$(MAKE) -C $(GODLIB_DIR) -f Makefile.host

clean:
	rm -f $(OUT)
