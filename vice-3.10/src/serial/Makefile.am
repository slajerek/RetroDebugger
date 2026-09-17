AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/printerdrv \
	-I$(top_srcdir)/src/lib/ \
	-I$(top_srcdir)/src/lib/p64

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libserial.a

EXTRA_libserial_a_SOURCES = \
	realdevice.c \
	realdevice.h

libserial_a_SOURCES = \
	fsdrive.c \
	fsdrive.h \
	iec-ieee488-shared.c \
	iec-ieee488-shared.h \
	serial-device.c \
	serial-iec-bus.c \
	serial-iec-bus.h \
	serial-iec-device.c \
	serial-iec-device.h \
	serial-iec-lib.c \
	serial-iec.c \
	serial-iec.h \
	serial-realdevice.c \
	serial-trap.c \
	serial-trap.h \
	serial.c

if HAVE_REALDEVICE
libserial_a_DEPENDENCIES = realdevice.o
libserial_a_LIBADD = realdevice.o
else
libserial_a_DEPENDENCIES =
libserial_a_LIBADD =
endif
