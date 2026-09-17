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


noinst_LIBRARIES = libimagecontents.a

libimagecontents_a_SOURCES = \
	diskcontents-block.c \
	diskcontents-block.h \
	diskcontents-iec.c \
	diskcontents-iec.h \
	diskcontents.c \
	diskcontents.h \
	imagecontents.c \
	tapecontents.c \
	tapecontents.h

