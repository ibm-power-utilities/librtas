#
# Makefile for librtasevent_src
#
# Copyright (C) 2005 IBM Corporation
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

include ../rules.mk

LIBRE_SONAME = librtasevent.so
LIBRE_STATIC = librtasevent.a
LIBRE = $(LIBRE_SONAME).$(VERSION)

LIBRE_OBJS = get_rtas_event.o print_rtas_event.o rtas_cpu.o rtas_dump.o	\
	     rtas_epow.o rtas_io.o rtas_lri.o rtas_mem.o rtas_post.o	\
	     rtas_sp.o rtas_srcfru.o rtas_v6_misc.o rtas_vend.o rtas_hotplug.o

LIBRE_HDRS = librtasevent.h librtasevent_v4.h librtasevent_v6.h
HEADERS = $(LIBRE_HDRS) rtas_event.h

CFLAGS += -fPIC -DPIC -I.
LDFLAGS += -shared -Wl,-soname -Wl,$(LIBRE_SONAME).$(MAJOR_NO)

all: libre_shared libre_static

libre_shared: $(LIBRE_OBJS) $(HEADERS)
	@echo "LD librtasevent_src/$(LIBRE)..."
	@$(CC) $(LIBRE_OBJS) $(LDFLAGS) -o $(LIBRE)

libre_static: $(LIBRE_OBJS) $(HEADERS)
	@echo "AR $(LIBRE_STATIC)..."
	@ar rcs $(LIBRE_STATIC) $(LIBRE_OBJS)

install:
	@$(call install_lib,$(LIBRE),$(DESTDIR))
	@$(call install_inc,$(LIBRE_HDRS),$(DESTDIR))
	@ln -sf $(LIBRE) $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBRE))/$(LIBRE_SONAME)
	@ln -sf $(LIBRE) $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBRE))/$(LIBRE_SONAME).$(MAJOR_NO)

uninstall:
	@$(call uninstall_lib,$(LIBRE),$(DESTDIR))
	@$(call uninstall_inc,$(LIBRE_HDRS),$(DESTDIR))
	@rm -f $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBRE))/$(LIBRE_SONAME)
	@rm -f $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBRE))/$(LIBRE_SONAME).$(MAJOR_NO)

clean:
	@echo "Cleaning up $(WORK_DIR) files..."
	@rm -f $(LIBRE_OBJS) $(LIBRE_SONAME) $(LIBRE) $(LIBRE_STATIC)

