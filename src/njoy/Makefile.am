#
# Makefile for the XBMC N7 PVR AddOn
#
# See the README for copyright information and
# how to reach the author.
#

LIBS   =-ldl -lcurl -lpthread
LIBDIR = ../../addons/pvr.njoy
LIB    = ../../addons/pvr.njoy/XBMC_njoy.pvr

lib_LTLIBRARIES = libnjoy.la

libnjoy_la_SOURCES = \
	client.cpp \
	xmlParser.cpp \
	N7Data.cpp

libnjoy_la_LDFLAGS = \
	-avoid-version


CPPFLAGS = \
	$(ARCH_DEFINES)

INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../.. -I$(srcdir)/../../include/xbmc -D__STDC_CONSTANT_MACROS -D_LINUX

EXTRA_DIST = \
	project

XBMC_njoy.pvr: libnjoy.la
	cp -f .libs/libnjoy.so XBMC_njoy.pvr
	$(STRIP) -x XBMC_njoy.pvr

$(LIB): XBMC_njoy.pvr
	cp -f XBMC_njoy.pvr $(LIB)

all: $(LIB)

clean-local:
	-rm -f $(LIB) XBMC_njoy.pvr

install:

