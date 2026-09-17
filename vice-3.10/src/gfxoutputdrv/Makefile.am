# Makefile.am for `libgfxoutputdrv.a'.

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/lib/libzmbv \
	-I$(top_srcdir)/src/arch/shared/sounddrv \
	@ARCH_INCLUDES@

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libgfxoutputdrv.a

# These sources are linked only under certain circumstances.
EXTRA_libgfxoutputdrv_a_SOURCES = \
	gifdrv.c \
	gifdrv.h \
	pngdrv.c \
	pngdrv.h \
	zmbvdrv.c \
	zmbvdrv.h

# These sources are always built.
libgfxoutputdrv_a_SOURCES = \
	bmpdrv.c \
	bmpdrv.h \
	artstudiodrv.c \
	ffmpegexedrv.c \
	ffmpegexedrv.h \
	gfxoutput.c \
	godotdrv.c \
	godotdrv.h \
	iffdrv.c \
	iffdrv.h \
	koaladrv.c \
	minipaintdrv.c \
	nativedrv.c \
	nativedrv.h \
	pcxdrv.c \
	pcxdrv.h \
	ppmdrv.c \
	ppmdrv.h \
	zmbvdrv.c \
	zmbvdrv.h

libgfxoutputdrv_a_DEPENDENCIES = @GFXOUTPUT_DRIVERS@
libgfxoutputdrv_a_LIBADD = @GFXOUTPUT_DRIVERS@

