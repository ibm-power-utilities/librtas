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

AM_CFLAGS = @cc_std_flags@ @cc_opt_flags@ @cc_warn_flags@
AM_CPPFLAGS = -I$(top_srcdir)/librtas_src/ -I$(top_srcdir)/librtasevent_src/

library_includedir=$(includedir)

# See "Updating library version information" in the libtool manual for
# how to maintain these values. They are *not* tied to the release
# number.
LIBRTAS_CURRENT = 2
LIBRTAS_REVISION = 0
LIBRTAS_AGE = 0
LIBRTAS_LIBRARY_VERSION = $(LIBRTAS_CURRENT):$(LIBRTAS_REVISION):$(LIBRTAS_AGE)

lib_LTLIBRARIES += librtas.la
librtas_la_LDFLAGS = -version-info $(LIBRTAS_LIBRARY_VERSION) -lpthread
librtas_la_SOURCES = \
	librtas_src/vpd.c \
	librtas_src/ofdt.c \
	librtas_src/syscall_calls.c \
	librtas_src/syscall_rmo.c \
	librtas_src/sysparm.c

library_include_HEADERS += librtas_src/librtas.h
noinst_HEADERS += \
	librtas_src/internal.h \
	librtas_src/papr-miscdev.h \
	librtas_src/papr-sysparm.h \
	librtas_src/papr-vpd.h

# See "Updating library version information" in the libtool manual for
# how to maintain these values. They are *not* tied to the release
# number.
LIBRTASEVENT_CURRENT = 2
LIBRTASEVENT_REVISION = 0
LIBRTASEVENT_AGE = 0
LIBRTASEVENT_LIBRARY_VERSION = $(LIBRTASEVENT_CURRENT):$(LIBRTASEVENT_REVISION):$(LIBRTASEVENT_AGE)

lib_LTLIBRARIES += librtasevent.la
librtasevent_la_LDFLAGS = -version-info $(LIBRTASEVENT_LIBRARY_VERSION)
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

if ENABLE_TESTS

check_PROGRAMS = tests/link_librtas tests/dlopen_librtas tests/rtas_set_debug

tests_link_librtas_LDADD = librtas.la $(CMOCKA_LIBS)

tests_dlopen_librtas_LDADD = $(CMOCKA_LIBS)

tests_rtas_set_debug_LDADD = librtas.la $(CMOCKA_LIBS)

TESTS = $(check_PROGRAMS)

endif # ENABLE_TESTS

abixml_dir = $(srcdir)/data/abixml
librtas_abi_xml = $(abixml_dir)/librtas/$(host_cpu)-$(host_os).xml
librtasevent_abi_xml = $(abixml_dir)/librtasevent/$(host_cpu)-$(host_os).xml

librtas_dso = $(builddir)/.libs/librtas.so
librtasevent_dso = $(builddir)/.libs/librtasevent.so

# Automake allows Make conditionals through if they're not in the first column.
 ifdef cmd_abidiff
abidiff_opts = --no-default-suppression
.PHONY: abi-check librtas-abi-check librtasevent-abi-check
abi-check: librtas-abi-check librtasevent-abi-check
librtas-abi-check: librtas.la
	if test -f $(librtas_abi_xml) ; then \
		$(cmd_abidiff) $(abidiff_opts) $(librtas_abi_xml) $(librtas_dso) ; \
	else \
		printf "librtas ABI check skipped, %s not present\n" $(librtas_abi_xml) ; \
	fi
librtasevent-abi-check: librtasevent.la
	if test -f $(librtasevent_abi_xml) ; then \
		$(cmd_abidiff) $(abidiff_opts) $(librtasevent_abi_xml) $(librtasevent_dso) ; \
	else \
		printf "librtasevent ABI check skipped, %s not present\n" $(librtasevent_abi_xml) ; \
	fi
 endif # cmd_abidiff
