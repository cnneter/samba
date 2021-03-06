# Copyright (c) 2007-2008 Apple Inc. All rights reserved.
#
# @APPLE_LICENSE_HEADER_START@
#
# This file contains Original Code and/or Modifications of Original Code
# as defined in and that are subject to the Apple Public Source License
# Version 2.0 (the 'License'). You may not use this file except in
# compliance with the License. Please obtain a copy of the License at
# http://www.opensource.apple.com/apsl/ and read it before using this
# file.
#
# The Original Code and all software distributed under the License are
# distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
# INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
# Please see the License for the specific language governing rights and
# limitations under the License.
#
# @APPLE_LICENSE_HEADER_END@

#
# Makefile for smb-sync-preferences
#

Project := smb-sync-preferences

ifndef CoreOSMakefiles
CoreOSMakefiles = $(MAKEFILEPATH)/CoreOS
endif

# Common.make does a "rm -f" on $(OBJROOT) without ignoring the exit code,
# which breaks us for doing in-place builds (where OBJROOT="."). This tells
# make to error status for all commands in the "clean" target.
.IGNORE: clean

VERBOSE := YES
ifeq ($(SRCROOT),.)
	COPY_SOURCES = NO
endif

ifeq ($(GnuNoStrip),YES)
STRIP_X	:= true
else
STRIP_X	:= strip -x
endif

# This is approximately what Standard/Standard.make does, but if we use that it
# kill the build. Probably something subtl WRT the default target. This works.
include $(CoreOSMakefiles)/ReleaseControl/Common.make
include $(CoreOSMakefiles)/Standard/Variables.make
include $(CoreOSMakefiles)/Standard/Implicit.make

# Reset the various roots for in-place builds. Common.make sets
# these to subdirectories of /tmp but does not bother to actually
# create the subdirectories. This makes in-place builds fail because
# no-one has got around to doing a mkdir on $(SYMROOT). buildit
# sets these paths in the environment, which overrides the local
# values, making us all OK.

SRCROOT = .
OBJROOT = .
SYMROOT = .
DSTROOT = .

# make.common contains the project-wide GCC options and other useful settings.
# when doing a stand-alone buldit from the current directory, it's not copied
# to $(SRCROOT), so we have to live without it.
-include ../../make.common

SMB_LIBEXEC ?= $(LIBEXECDIR)/samba

DEBUG ?= NO
TESTLEAKS ?= NO

# The CoreOS Makefiles set this to /usr/bin/cc, which makes the link fail.
Cxx := /usr/bin/c++

# NOTE: Adding -frepo makes a smaller executable, but this fails to link when
# building multi-architecture binaries.
Extra_CPP_Includes = -Iinclude
Extra_Cxx_Flags := $(RC_CFLAGS) $(SANE_GCC_WARNINGS)
Extra_LD_Flags := $(RC_CFLAGS) $(SANE_GCC_WARNINGS)

# Turn off optimisation for debug builds. There's so much inlining going
# on that it's just too hard to step through otherwise.
ifeq ($(DEBUG),YES)
Extra_Cxx_Flags += -DDEBUG
CC_Optimize := -O0
endif

ifeq ($(TESTLEAKS),YES)
Extra_Cxx_Flags += -DTESTLEAKS
endif

LIBS := -framework SystemConfiguration -framework CoreServices

SYNC_SOURCES := \
	$(SRCROOT)/sync.cpp \
	$(SRCROOT)/rules.cpp \
	$(SRCROOT)/lib/SmbOption.cpp \
	$(SRCROOT)/lib/SmbConfig.cpp \
	$(SRCROOT)/lib/plist.cpp \
	$(SRCROOT)/lib/options.cpp \
	$(SRCROOT)/lib/utils.cpp \
	$(SRCROOT)/lib/RegularExpression.cpp \
	$(SRCROOT)/lib/launchctl.cpp

SHARES_SOURCES := \
	$(SRCROOT)/shares.cpp \
	$(SRCROOT)/lib/SmbConfig.cpp \
	$(SRCROOT)/lib/RegularExpression.cpp \
	$(SRCROOT)/lib/options.cpp \
	$(SRCROOT)/lib/utils.cpp

SYNC_OBJECTS := $(subst $(SRCROOT),$(OBJROOT),$(SYNC_SOURCES:.cpp=.o))
SHARES_OBJECTS := $(subst $(SRCROOT),$(OBJROOT),$(SHARES_SOURCES:.cpp=.o))

TARGETS := $(OBJROOT)/smb-sync-preferences \
	$(OBJROOT)/smb-sync-shares

$(BuildDirectory)/%.o: %.cpp
	@echo "Compiling "$@"..."
	$(_v) $(Cxx) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

all: $(TARGETS)
build:: $(TARGETS)

# Make sure we create the lib subdirectory is $OBJROOT to put objects in.
$(SYNC_OBJECTS): $(OBJROOT)/lib

$(OBJROOT)/lib:
	$(_v) $(MKDIR) $(OBJROOT)/lib

$(OBJROOT)/smb-sync-preferences: $(SYNC_OBJECTS)
	@echo Linking $@ ...
	$(_v) $(Cxx) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJROOT)/smb-sync-shares: $(SHARES_OBJECTS)
	@echo Linking $@ ...
	$(_v) $(Cxx) $(LDFLAGS) -o $@ $^ $(LIBS) -framework OpenDirectory

# Install preferences sync tools to $DSTROOT and copy the symbolicated
# versions to $SYMROOT. This is already done in the Samba project as
# part of the GnuAfterInstall rule. We only need to do this when doing
# a buildit build of the prefsync sub-project.
#
# NOTE: smb-sync-preferences used to be called synchronize-preferences. We
# install a symlink with the old name for compatibility. Thursby software calls
# the old name, so don't remove the symlink.
install:: $(TARGETS)
	$(INSTALL) -d -m 755 $(DSTROOT)$(USRDIR)/local/include
	$(INSTALL) -d -m 755 $(DSTROOT)$(SMB_LIBEXEC)
	$(INSTALL) -c -m 755 -o root -g wheel \
		$(OBJROOT)/smb-sync-preferences \
		$(DSTROOT)$(SMB_LIBEXEC)
	$(INSTALL) -c -m 755 -o root -g wheel \
		$(OBJROOT)/smb-sync-shares \
		$(DSTROOT)$(SMB_LIBEXEC)
	(cd $(DSTROOT)$(SMB_LIBEXEC) && \
	 	ln -s smb-sync-preferences synchronize-preferences)
	(cd $(DSTROOT)$(SMB_LIBEXEC) && \
	 	ln -s smb-sync-shares synchronize-shares)
	$(INSTALL) -c -m 644 \
		$(SRCROOT)/include/smb_server_prefs.h \
		$(DSTROOT)$(USRDIR)/local/include
ifndef $(NoSymRootCopy)
	$(INSTALL) -d -m 755 $(SYMROOT)$(USRDIR)/local/include
	$(INSTALL) -d -m 755 $(SYMROOT)$(SMB_LIBEXEC)
	$(INSTALL) -c -m 755 -o root -g wheel \
		$(OBJROOT)/smb-sync-preferences \
		$(SYMROOT)$(SMB_LIBEXEC)
	$(INSTALL) -c -m 755 -o root -g wheel \
		$(OBJROOT)/smb-sync-shares \
		$(SYMROOT)$(SMB_LIBEXEC)
	dsymutil $(SYMROOT)$(SMB_LIBEXEC)/smb-sync-preferences
	dsymutil $(SYMROOT)$(SMB_LIBEXEC)/smb-sync-shares
endif

clean::
	-$(_v) rm -rf $(SYNC_OBJECTS) $(SHARES_OBJECTS) *.dSYM
	-$(_v) rm -f $(TARGETS)

