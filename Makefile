#
# Makefile for librtas
#

include rules.mk

.SILENT:

FILES = README COPYRIGHT
SUBDIRS = librtas_src librtasevent_src

TARBALL_FILES += $(FILES) doc/doxygen.rtas doc/doxygen.rtasevent Changelog

all:
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d;) 
	# Update spec file for build type
	@sed "s|\@LIB_DIR\@|$(LIB_DIR)$(call is_lib64,librtas_src/librtas.so.$(VERSION))|g" $(SPECFILE_IN) > $(SPECFILE)

install:
	@$(call install_doc,$(FILES),$(DESTDIR))
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d install;) 

uninstall:
	@$(call uninstall_doc,$(FILES),$(DESTDIR))
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d uninstall;) 

rpm: all
	@echo "Creating rpm..."
	@export DESTDIR=$(SHIPDIR); $(MAKE) install
	@rm $(SHIPDIR)$(LIB_DIR)$(call is_lib64,librtas_src/librtas.so.$(VERSION))/librtas.so
	@rm $(SHIPDIR)$(LIB_DIR)$(call is_lib64,librtasevent_src/librtasevent.so.$(VERSION))/librtasevent.so
	@$(RPM) -bb $(SPECFILE)
	@rm -rf $(SHIPDIR)

docs:	
	@echo "Creating doxygen documents..."
	@mkdir -p doc/librtasevent
	@mkdir -p doc/librtas
	@$(DOXYGEN) doc/doxygen.rtas
	@$(DOXYGEN) doc/doxygen.rtasevent

tarball: clean
	@echo "Creating release tarball..."
	@$(BUILD_TARBALL)

devball: clean
	@echo "Creating snapshot tarball..."
	@$(BUILD_DEVBALL)

clean:
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d clean;) 
	@echo "Cleaning up doxygen files..."
	@rm -rf doc/librtas doc/librtasevent
	@$(CLEAN) $(SHIPDIR) $(TARBALL) $(DEVBALL) $(SPECFILE)
