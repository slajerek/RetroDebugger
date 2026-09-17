AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/raster

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libcrtc.a

libcrtc_a_SOURCES = \
	crtc-cmdline-options.c \
	crtc-cmdline-options.h \
	crtc-color.c \
	crtc-color.h \
	crtc-draw.c \
	crtc-draw.h \
	crtc-mem.c \
	crtc-mem.h \
	crtc-resources.c \
	crtc-resources.h \
	crtc-snapshot.c \
	crtc.c \
	crtc.h \
	crtctypes.h

