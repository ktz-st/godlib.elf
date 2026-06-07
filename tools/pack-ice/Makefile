CC ?= cc
AR ?= ar
RANLIB ?= ranlib

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)
HOST_STUB ?= $(UNAME_S)/$(UNAME_M)
HOST_BIN_DIR ?= ../../bin/$(HOST_STUB)
BUILD_DIR ?= build/$(HOST_STUB)

CFLAGS += -Wall -g

PROGS = icecat freeze melt
BIN_PROGS = $(addprefix $(HOST_BIN_DIR)/,$(PROGS)) $(HOST_BIN_DIR)/pack-ice
ICELIB = $(BUILD_DIR)/libice.a
ICELIB_OBJS = $(addprefix $(BUILD_DIR)/,ice_info.o ice_crunch.o ice_decrunch.o)
PROG_OBJS = $(addprefix $(BUILD_DIR)/,icecat.o freeze.o melt.o)

all: $(BIN_PROGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(HOST_BIN_DIR)/icecat: $(BUILD_DIR)/icecat.o $(ICELIB)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $(BUILD_DIR)/icecat.o $(ICELIB)

$(HOST_BIN_DIR)/freeze: $(BUILD_DIR)/freeze.o $(ICELIB)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $(BUILD_DIR)/freeze.o $(ICELIB)

$(HOST_BIN_DIR)/melt: $(BUILD_DIR)/melt.o $(ICELIB)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $(BUILD_DIR)/melt.o $(ICELIB)

$(HOST_BIN_DIR)/pack-ice: $(HOST_BIN_DIR)/freeze
	cp $(HOST_BIN_DIR)/freeze $@

$(ICELIB): $(ICELIB_OBJS)
	rm -f $(ICELIB)
	$(AR) cr $(ICELIB) $(ICELIB_OBJS)
	$(RANLIB) $(ICELIB)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(BIN_PROGS)

check: $(BIN_PROGS)
	cd testcases; FREEZE=../$(HOST_BIN_DIR)/freeze MELT=../$(HOST_BIN_DIR)/melt ./check
