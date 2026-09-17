## Vice Automake file
# Makefile.am for `libusbsiddrv.a'.
# if HAVE_USBSID
AM_CPPFLAGS = \
  @VICE_CPPFLAGS@ \
  @ARCH_INCLUDES@ \
  -I$(top_builddir)/src \
  -I$(top_srcdir)/src \
  -I$(top_srcdir)/src/lib/libusbsiddrv
AM_CFLAGS = @VICE_CFLAGS@
AM_CXXFLAGS = \
  @VICE_CXXFLAGS@ \
  -Wno-unused-variable \
  -I$(top_srcdir)/src/lib/libusbsiddrv
AM_LDFLAGS = @VICE_LDFLAGS@

noinst_LIBRARIES = libusbsiddrv.a

USBSIDDRIVER=

USBSIDDRIVER += \
  USBSID.o \
  USBSIDInterface.o

# Built depending on configure options:
EXTRA_libusbsiddrv_a_SOURCES = \
  USBSIDInterface.cpp \
  USBSID.cpp

EXTRA_DIST = \
  USBSID.h \
  USBSIDInterface.h

# Always built:
# (needs to be declared empty to avoid `make` to try to build libusbsiddrv.o
# from libusbsiddrv.c)
libusbsiddrv_a_SOURCES =
noinst_HEADERS = USBSID.h USBSIDInterface.h

# Make sure we actually link the generated object files into the library:
libusbsiddrv_a_LIBADD = $(USBSIDDRIVER)
# endif HAVE_USBSID
