AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/raster

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libvdc.a

libvdc_a_SOURCES = \
	vdc-cmdline-options.c \
	vdc-cmdline-options.h \
	vdc-color.c \
	vdc-color.h \
	vdc-draw.c \
	vdc-draw.h \
	vdc-mem.c \
	vdc-mem.h \
	vdc-resources.c \
	vdc-resources.h \
	vdc-snapshot.c \
	vdc-snapshot.h \
	vdc.c \
	vdc.h \
	vdctypes.h

