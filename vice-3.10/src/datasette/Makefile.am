AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/tape \
	-I$(top_srcdir)/src/tapeport


AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libdatasette.a

libdatasette_a_SOURCES = \
	datasette.c \
	datasette.h \
	datasette-sound.c \
	datasette-sound.h

