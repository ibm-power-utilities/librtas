
#
# Makefile for libofdt
#
# Copyright (C) 2005 IBM Corporation
# Common Public License Version 1.0 (see COPYRIGHT)
#

include ../rules.mk

LIBOFDT_SONAME = libofdt.so
LIBOFDT = $(LIBOFDT_SONAME).$(VERSION)

LIBOFDT_OBJS = dtree.o drc.o

LIBOFDT_HDRS = libofdt.h common.h
LIBOFDT_SHIP_HDR = libofdt.h

CFLAGS += -fPIC -DPIC
LDFLAGS += -shared -Wl,-soname -Wl,$(LIBOFDT_SONAME).$(MAJOR_NO)

all: $(LIBOFDT_OBJS) $(LIBOFDT_HDRS)
	@echo "LD $(WORK_DIR)/$(LIBOFDT)..."
	@$(CC) $(LIBOFDT_OBJS) $(LDFLAGS) -o $(LIBOFDT)

install:
	@$(call install_lib,$(LIBOFDT),$(DESTDIR))
	@$(call install_inc,$(LIBOFDT_SHIP_HDR),$(DESTDIR))
	@ln -sf $(LIBOFDT) $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBOFDT))/$(LIBOFDT_SONAME)

uninstall:
	@$(call uninstall_lib,$(LIBOFDT),$(DESTDIR))
	@$(call uninstall_inc,$(LIBOFDT_SHIP_HDR),$(DESTDIR))
	@rm -f $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBOFDT))/$(LIBOFDT_SONAME)

clean:
	@echo "Cleaning up $(WORK_DIR) files..."
	@rm -f $(LIBOFDT_OBJS) $(LIBOFDT)
