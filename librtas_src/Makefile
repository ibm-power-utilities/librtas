#
# Makefile for librtas
#
# Copyright (C) 2005 IBM Corporation
# Common Public License Version 1.0 (see COPYRIGHT)
#

include ../rules.mk

LIBRTAS_SONAME = librtas.so
LIBRTAS_STATIC = librtas.a
LIBRTAS = $(LIBRTAS_SONAME).$(VERSION)

LIBRTAS_OBJS = common.o ofdt.o syscall_calls.o syscall_rmo.o procfs_calls.o

LIBRTAS_HDR = librtas.h
HEADERS	= $(LIBRTAS_HDR) common.h

CFLAGS += -fPIC -DPIC
LDFLAGS += -shared -Wl,-soname -Wl,$(LIBRTAS_SONAME).$(MAJOR_NO)

all: librtas_shared librtas_static

librtas_shared: $(LIBRTAS_OBJS) $(HEADERS)
	@echo "LD $(WORK_DIR)/$(LIBRTAS)..."
	@$(CC) $(LIBRTAS_OBJS) $(LDFLAGS) -o $(LIBRTAS)

librtas_static: $(LIBRTAS_OBJS) $(HEADERS)
	@echo "AR $(LIBRTAS_STATIC)..."
	@ar rcs $(LIBRTAS_STATIC) $(LIBRTAS_OBJS)

install:
	@$(call install_lib,$(LIBRTAS),$(DESTDIR))
	@$(call install_inc,$(HEADERS),$(DESTDIR))
	@ln -sf $(LIBRTAS) $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBRTAS))/$(LIBRTAS_SONAME)

uninstall:
	@$(call uninstall_lib,$(LIBRTAS),$(DESTDIR))
	@$(call uninstall_inc,$(LIBRTAS_HDR),$(DESTDIR))
	@rm -f $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBRTAS))/$(LIBRTAS_SONAME)

clean:
	@echo "Cleaning up $(WORK_DIR) files..."
	@rm -f $(LIBRTAS_OBJS) $(LIBRTAS) $(LIBRTAS_STATIC)
