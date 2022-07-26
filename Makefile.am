ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = README COPYING.LESSER
EXTRA_DIST += doc/doxygen.rtas doc/doxygen.rtasevent Changelog

docdir = $(datadir)/doc/@PACKAGE@
doc_DATA = README COPYING.LESSER

bin_PROGRAMS=
lib_LTLIBRARIES=
man_MANS=
noinst_HEADERS=
library_include_HEADERS=

AM_CPPFLAGS = -I$(top_srcdir)/librtas_src/ -I$(top_srcdir)/librtasevent_src/

library_includedir=$(includedir)

LIBRTAS_LIBRARY_VERSION = 2:0:0

lib_LTLIBRARIES += librtas.la
librtas_la_LDFLAGS = -version-info $(LIBRTAS_LIBRARY_VERSION)
librtas_la_SOURCES = \
	librtas_src/ofdt.c \
	librtas_src/syscall_calls.c \
	librtas_src/syscall_rmo.c

library_include_HEADERS += librtas_src/librtas.h
noinst_HEADERS += librtas_src/lparctl.h librtas_src/syscall.h

lib_LTLIBRARIES += librtasevent.la
librtasevent_la_LDFLAGS = -version-info $(LIBRTAS_LIBRARY_VERSION)
librtasevent_la_SOURCES = \
	librtasevent_src/get_rtas_event.c \
	librtasevent_src/print_rtas_event.c \
	librtasevent_src/rtas_cpu.c \
	librtasevent_src/rtas_dump.c \
	librtasevent_src/rtas_epow.c \
	librtasevent_src/rtas_io.c \
	librtasevent_src/rtas_lri.c \
	librtasevent_src/rtas_mem.c \
	librtasevent_src/rtas_post.c \
	librtasevent_src/rtas_sp.c \
	librtasevent_src/rtas_srcfru.c \
	librtasevent_src/rtas_v6_misc.c \
	librtasevent_src/rtas_vend.c \
	librtasevent_src/rtas_hotplug.c

library_include_HEADERS += \
	librtasevent_src/librtasevent.h \
	librtasevent_src/librtasevent_v4.h \
	librtasevent_src/librtasevent_v6.h

noinst_HEADERS += librtasevent_src/rtas_event.h \
	librtasevent_src/rtas_src_codes.c
