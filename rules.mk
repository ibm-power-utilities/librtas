#
# Common Makefile definitions
#

PWD = $(shell echo `pwd`)
INSTALL = /usr/bin/install

DOXYGEN = `which doxygen`

# Name of the rpm spec file
SPECFILENAME = librtas.spec
SPECFILENAME_IN = librtas.spec.in

# resolves the root directory at which this build is occuring
ROOT_DIR =                                              \
        $(shell                                         \
        while [ `pwd` != "/" ];                         \
        do                                              \
                if [ -f `pwd`/$(SPECFILENAME_IN) ];     \
                then                                    \
                        echo `pwd`;                     \
                        break;                          \
                fi;                                     \
                cd ..;                                  \
        done;)

# Full path to the rpm specfile
SPECFILE = $(ROOT_DIR)/$(SPECFILENAME)
SPECFILE_IN = $(ROOT_DIR)/$(SPECFILENAME_IN)

# Find the correct command to build RPM's
RPM =							\
	$(shell						\
	if [ -a /bin/rpmbuild ]; then			\
		echo "/bin/rpmbuild";			\
	elif [ -a /usr/bin/rpmbuild ]; then		\
		echo "/usr/bin/rpmbuild";		\
	elif [ -a /bin/rpm ]; then			\
		echo "/bin/rpm";			\
	elif [ -a /usr/bin/rpm ]; then			\
		echo "/usr/bin/rpmbuild";		\
	else						\
		echo "rpm seems to be non-existant";	\
	fi;)

# Pull the version, release and project name info from the spec file
VERSION	:= $(shell echo `grep "define version" $(SPECFILE_IN) | cut -d' ' -f3`)
RELEASE	:= $(shell echo `grep "define release" $(SPECFILE_IN) | cut -d' ' -f3`)
PROJECT := $(shell echo `grep "define name" $(SPECFILE_IN) | cut -d' ' -f3`)

# Generate the Major, Minor and Revision numbers from the version
VERSION_LIST := $(subst ., ,$(VERSION))
MAJOR_NO := $(word 1,$(VERSION_LIST))
MINOR_NO := $(word 2,$(VERSION_LIST))
REVISION_NO := $(word 3,$(VERSION_LIST))

# Set this if you want to install to a base directory other than /
DESTDIR ?= 

# Standard base directory names
BIN_DIR = /usr/bin
SBIN_DIR = /usr/sbin
LIB_DIR = /usr/lib
INC_DIR = /usr/include/
DOC_DIR = /usr/share/doc/packages/$(PROJECT)
MAN_DIR = /usr/share/man/man8

# Shipdir is where we put all the files to build an rpm
SHIPDIR = /tmp/$(PROJECT)-buildroot

# Source tarball name and build directory
TARBALL = $(PROJECT)-$(VERSION).tar.gz
TARBALL_FILES = Makefile rules.mk $(SPECFILENAME_IN)
TB_DIR = $(PROJECT)-$(VERSION)

# Build a tarball of the source code
BUILD_TARBALL =						\
	$(shell						\
	echo CVS > ./ignore;				\
	mkdir $(TB_DIR);				\
	cp -R $(SUBDIRS) $(TARBALL_FILES) $(TB_DIR);	\
	tar -zcf $(TARBALL) -X ./ignore $(TB_DIR);	\
	rm -rf ./ignore $(TB_DIR);)

# Development snapshot builds
DEVBALL = $(PROJECT)-$(VERSION)-`date +%Y%m%d`.tar.gz

# Build a development snapshot of the source code
BUILD_DEVBALL =						\
	$(shell						\
	echo CVS > ./ignore;				\
	mkdir $(TB_DIR);				\
	cp -R $(SUBDIRS) $(TARBALL_FILES) $(TB_DIR);	\
	tar -zcf $(DEVBALL) -X ./ignore $(TB_DIR);	\
	rm -rf ./ignore $(TB_DIR);)

# Current build directory
WORK_DIR = $(patsubst /%,%,$(subst $(ROOT_DIR),,$(PWD)))

# You should always build with -Wall
CFLAGS += -Wall

# Uncomment this for debug builds
CFLAGS += -g -DDEBUG

# If you wish to have a log of installed files, define the file here
INSTALL_LOG ?= $(ROOT_DIR)/install.log

#
# is_lib64 - determine if a library is 64-bit
#
# $1 library to examine
define is_lib64
$(findstring 64,$(shell file $(firstword $1)))
endef

#
# install_files - Install file(s) in the given location
#
#  $1 - files to be installed
#  $2 - permissions to install file with
#  $3 - directory to install file to
define install_files
	$(INSTALL) -d -m 755 $3;
	$(foreach f,$1,							\
		echo "Installing $(patsubst /%,%,$(WORK_DIR)/$f)";	\
		$(INSTALL) -m $2 $f $3;					\
		$(if $(INSTALL_LOG),echo $3/$f >> $(INSTALL_LOG);,))
endef

#
# The following are wrappers for calls to install_files for
# installing files in known locations (i.e. /usr/bin).  The args
# to each of the wrappers are the same.
#
#  $1 - files to be installed
#  $2 - prefix to install path for the files
#
define install_bin
	$(call install_files,$1,755,$2/$(BIN_DIR))
endef

define install_sbin
	$(call install_files,$1,744,$2/$(SBIN_DIR))
endef

define install_lib
	$(call install_files,$1,755,$2/$(LIB_DIR)$(call is_lib64,$1))
endef

define install_inc
	$(call install_files,$1,644,$2/$(INC_DIR))
endef

define install_doc
	$(call install_files,$1,644,$2/$(DOC_DIR))
endef

define install_man
	$(call install_files,$1,644,$2/$(MAN_DIR))
endef

#
# uninstall_files - Uninstall file(s)
#
#  $1 - files to be uninstalled
#  $2 - the directory the files to uninstall live in
define uninstall_files
	$(foreach f,$1,							\
		echo "Un-installing $(patsubst /%,%,$(WORK_DIR)/$f)";	\
		rm -f $2/$f;) 
endef

#
# The following are wrappers for calls to uninstall_files for
# removing files in known locations (i.e. /usr/bin).  The args
# to each of the wrappers are the same.
#
#  $1 - files to be uninstalled
#  $2 - prefix to uninstall path for the files
#
define uninstall_bin
	$(call uninstall_files,$1,$2/$(BIN_DIR))
endef

define uninstall_sbin
	$(call uninstall_files,$1,$2/$(SBIN_DIR))
endef

define uninstall_lib
	$(call uninstall_files,$1,$2/$(LIB_DIR)$(call is_lib64,$1))
endef

define uninstall_inc
	$(call uninstall_files,$1,$2/$(INC_DIR))
endef

define uninstall_doc
	$(call uninstall_files,$1,$2/$(DOC_DIR))
endef

define uninstall_man
	$(call uninstall_files,$1,$2/$(MAN_DIR))
endef


# Define "CLEAN" as rm plus any files defined in this file that
# the actual Makefile may not (or have to) know about
CLEAN = /bin/rm -rf $(INSTALL_LOG) $(TB_DIR) *~

# Default target for building object files
%.o: %.c
	@echo "CC $(WORK_DIR)/$@"
	@$(CC) -c $(CFLAGS) $<


