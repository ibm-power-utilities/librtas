#
# Makefile for librtas
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

LIBRTAS_SONAME = librtas.so
LIBRTAS_STATIC = librtas.a
LIBRTAS = $(LIBRTAS_SONAME).$(VERSION)

LIBRTAS_OBJS = ofdt.o syscall_calls.o syscall_rmo.o

LIBRTAS_HDR = librtas.h
HEADERS	= $(LIBRTAS_HDR)

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
	@ln -sf $(LIBRTAS) $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBRTAS))/$(LIBRTAS_SONAME).$(MAJOR_NO)

uninstall:
	@$(call uninstall_lib,$(LIBRTAS),$(DESTDIR))
	@$(call uninstall_inc,$(HEADERS),$(DESTDIR))
	@rm -f $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBRTAS))/$(LIBRTAS_SONAME)
	@rm -f $(DESTDIR)/$(LIB_DIR)$(call is_lib64,$(LIBRTAS))/$(LIBRTAS_SONAME).$(MAJOR_NO)

clean:
	@echo "Cleaning up $(WORK_DIR) files..."
	@rm -f $(LIBRTAS_OBJS) $(LIBRTAS) $(LIBRTAS_STATIC)
