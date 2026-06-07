CC ?= cc
AR ?= ar

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)
HOST_STUB ?= $(UNAME_S)/$(UNAME_M)
HOST_BIN_DIR ?= ../../bin/$(HOST_STUB)
BUILD_DIR ?= build/$(HOST_STUB)

CFLAGS ?= -O3 -g -fomit-frame-pointer -Isrc/libdivsufsort/include -Isrc
OBJDIR = $(BUILD_DIR)/obj

$(OBJDIR)/%.o: src/../%.c
	@mkdir -p '$(@D)'
	$(CC) $(CFLAGS) -c $< -o $@

APP := $(HOST_BIN_DIR)/apultra
LIB := $(BUILD_DIR)/libapultra.a

LIBOBJS += $(OBJDIR)/src/expand.o
LIBOBJS += $(OBJDIR)/src/matchfinder.o
LIBOBJS += $(OBJDIR)/src/shrink.o
LIBOBJS += $(OBJDIR)/src/libdivsufsort/lib/divsufsort.o
LIBOBJS += $(OBJDIR)/src/libdivsufsort/lib/divsufsort_utils.o
LIBOBJS += $(OBJDIR)/src/libdivsufsort/lib/sssort.o
LIBOBJS += $(OBJDIR)/src/libdivsufsort/lib/trsort.o

APPOBJS := $(OBJDIR)/src/apultra.o

all: $(APP) $(LIB)

$(APP): $(LIB) $(APPOBJS)
	@mkdir -p '$(@D)'
	$(CC) $(APPOBJS) $(LIB) -o $(APP)

$(LIB): $(LIBOBJS)
	$(AR) rcs $(LIB) $(LIBOBJS)

libs: $(LIB)

clean:
	@rm -rf $(BUILD_DIR)
	@rm -f $(APP)
