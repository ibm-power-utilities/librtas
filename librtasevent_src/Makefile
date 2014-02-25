#
# Makefile for librtasevent_src
#
# Copyright (C) 2005 IBM Corporation
# Common Public License Version 1.0 (see COPYRIGHT)
#

include ../rules.mk

LIBRE_SONAME = librtasevent.so
LIBRE = $(LIBRE_SONAME).$(VERSION)

LIBRE_OBJS = get_rtas_event.o print_rtas_event.o rtas_cpu.o rtas_dump.o	\
	     rtas_epow.o rtas_io.o rtas_lri.o rtas_mem.o rtas_post.o	\
	     rtas_sp.o rtas_srcfru.o rtas_v6_misc.o rtas_vend.o rtas_hotplug.o

LIBRE_HDRS = librtasevent.h librtasevent_v4.h librtasevent_v6.h
HEADERS = $(LIBRE_HDRS) rtas_event.h

CFLAGS += -fPIC -DPIC -I.
LDFLAGS += -shared -Wl,-soname -Wl,$(LIBRE_SONAME).$(MAJOR_NO)

all: $(LIBRE_OBJS) $(HEADERS)
	@echo "LD librtasevent_src/$(LIBRE)..."
	@$(CC) $(LIBRE_OBJS) $(LDFLAGS) -o $(LIBRE)

install:
	@$(call install_lib,$(LIBRE),$(DESTDIR))
	@$(call install_inc,$(LIBRE_HDRS),$(DESTDIR))
	@ln -sf $(LIBRE) $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBRE))/$(LIBRE_SONAME)
	@ln -sf $(LIBRE) $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBRE))/$(LIBRE_SONAME).1

uninstall:
	@$(call uninstall_lib,$(LIBRE),$(DESTDIR))
	@$(call uninstall_inc,$(LIBRE_HDRS),$(DESTDIR))
	@rm -f $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBRE))/$(LIBRE_SONAME)

clean:
	@echo "Cleaning up $(WORK_DIR) files..."
	@rm -f $(LIBRE_OBJS) $(LIBRE_SONAME) $(LIBRE)

