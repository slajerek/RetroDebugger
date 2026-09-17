# Makefile for cartconv


# Make sure we use Windows' console mode since this is a command line tool
if WINDOWS_COMPILE
cartconv_LDFLAGS = -mconsole
else
cartconv_LDFLAGS =
endif

if HAVE_DEBUG
if MACOS_COMPILE
cartconv_LDFLAGS = @cartconv_LDFLAGS@ -Wl,-map -Wl,cartconv.map
else
cartconv_LDFLAGS = @cartconv_LDFLAGS@ -Wl,-Map=cartconv.map
endif
endif

if USE_SVN_REVISION
# Generate svnversion.h if it doesn't exist yet (for `make cartconv`)
$(top_builddir)/src/svnversion.h:
	(cd ../..; $(MAKE) svnversion.h)

# cartconv.c needs to include a built header
cartconv.$(OBJEXT): $(top_builddir)/src/svnversion.h
endif

# This is the binary we want to create
bin_PROGRAMS = cartconv

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src

LIBS =

# Sources used cartconv
cartconv_SOURCES = \
	cartconv.c \
	c64-cartridges.c \
	c64-saver.c \
	c128-cartridges.c \
	c128-saver.c \
	cbm2-cartridges.c \
	cbm2-saver.c \
	plus4-cartridges.c \
	plus4-saver.c \
	vic20-cartridges.c \
	vic20-saver.c \
	crt.c

# Extra files used for cartconv that should also end up in the `make dist`
EXTRA_DIST = \
	cartconv.h \
	c64-cartridges.h \
	c64-saver.h \
	c128-cartridges.h \
	c128-saver.h \
	cbm2-cartridges.h \
	cbm2-saver.h \
	plus4-cartridges.h \
	plus4-saver.h \
	vic20-cartridges.h \
	vic20-saver.h \
	crt.h
