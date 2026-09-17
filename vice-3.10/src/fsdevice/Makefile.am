AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/vdrive \
	-I$(top_srcdir)/src/lib/p64

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libfsdevice.a

libfsdevice_a_SOURCES = \
	fsdevice-close.c \
	fsdevice-close.h \
	fsdevice-cmdline-options.c \
	fsdevice-cmdline-options.h \
	fsdevice-flush.c \
	fsdevice-flush.h \
	fsdevice-filename.c \
	fsdevice-filename.h \
	fsdevice-open.c \
	fsdevice-open.h \
	fsdevice-read.c \
	fsdevice-read.h \
	fsdevice-resources.c \
	fsdevice-resources.h \
	fsdevice-write.c \
	fsdevice-write.h \
	fsdevice.c \
	fsdevicetypes.h

