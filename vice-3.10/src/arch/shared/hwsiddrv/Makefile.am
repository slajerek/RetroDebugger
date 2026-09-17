# Makefile.am for `libhwsiddrv.a'.

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/sid \
	-I$(top_srcdir)/src/platform \
	-I$(top_srcdir)/src/arch/shared \
	-I$(top_srcdir)/src/arch/shared/iodrv

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@

# TESTING
if HAVE_USBSID
AM_CPPFLAGS += -I$(top_srcdir)/src/lib/libusbsiddrv
AM_CXXFLAGS += -I$(top_srcdir)/src/lib/libusbsiddrv
AM_LDFLAGS += -I$(top_srcdir)/src/lib/libusbsiddrv
endif

noinst_LIBRARIES = libhwsiddrv.a

# Determine which drivers need to be built, start with an empty list
# (HardSID is currently enabled by default on system that have drivers for it)
HWSIDDRIVERS=

# Windows drivers
if WINDOWS_COMPILE
HWSIDDRIVERS += \
	hardsid-win32-drv.o \
	hs-win32-dll.o
if HAVE_CATWEASELMKIII
HWSIDDRIVERS += \
	catweaselmkiii-win32-drv.o \
	cw-win32-dll.o
endif HAVE_CATWEASELMKIII
if HAVE_PARSID
HWSIDDRIVERS += \
	parsid-win32-drv.o \
	ps-ieee1284.o
endif HAVE_PARSID
if HAVE_USBSID
HWSIDDRIVERS += \
	usbsid-unixwin-drv.o \
	us-unixwin-device.o
endif HAVE_USBSID
endif WINDOWS_COMPILE

if BEOS_COMPILE
if HAVE_PARSID
HWSIDDRIVERS += \
	ps-ieee1284.o
endif HAVE_PARSID
endif BEOS_COMPILE

if UNIX_COMPILE
HWSIDDRIVERS += \
	hardsid-unix-drv.o
if LINUX_COMPILE
HWSIDDRIVERS += hs-unix-linux.o
endif LINUX_COMPILE
if HAVE_CATWEASELMKIII
HWSIDDRIVERS += \
	catweaselmkiii-unix-drv.o \
	cw-unix-device.o
endif HAVE_CATWEASELMKIII
if HAVE_PARSID
HWSIDDRIVERS += \
	parsid-unix-drv.o \
	ps-ieee1284.o \
	ps-unix-file.o
endif HAVE_PARSID
if HAVE_USBSID
HWSIDDRIVERS += \
	usbsid-unixwin-drv.o \
	us-unixwin-device.o
endif HAVE_USBSID
endif UNIX_COMPILE


# Built depending on configure options:
EXTRA_libhwsiddrv_a_SOURCES = \
	catweaselmkiii-unix-drv.c \
	catweaselmkiii-win32-drv.c \
	cw-unix-device.c \
	cw-win32-dll.c \
	hardsid-unix-drv.c \
	hardsid-win32-drv.c \
	hs-unix-linux.c \
	hs-win32-dll.c \
	parsid-unix-drv.c \
	parsid-win32-drv.c \
	ps-ieee1284.c \
	ps-unix-file.c \
	usbsid-unixwin-drv.c \
	us-unixwin-device.c

EXTRA_DIST = \
	cw-unix.h \
	cw-win32.h \
	hs-unix.h \
	hs-win32.h \
	ps-unix.h \
	ps-win32.h \
	us-unixwin.h

# Always built:
# (needs to be declared empty to avoid `make` to try to build libhwsiddrv.o
# from libhwsiddrv.c)
libhwsiddrv_a_SOURCES =

# Make sure we actually link the generated object files into the library:
libhwsiddrv_a_LIBADD = $(HWSIDDRIVERS)
