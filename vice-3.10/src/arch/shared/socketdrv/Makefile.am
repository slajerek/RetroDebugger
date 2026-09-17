# Makefile.am for `libsocketdrv.a'.

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libsocketdrv.a

libsocketdrv_a_SOURCES = \
	socketdrv.c

EXTRA_DIST = \
	socket-beos-impl.h \
	socket-unix-impl.h \
	socket-win32-drv.c \
	socket-win32-impl.h \
	socketimpl.h
