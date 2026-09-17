# Makefile.am for `libsounddrv.a'.

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/arch/sdl

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libsounddrv.a

# These sources are linked only under certain circumstances.
EXTRA_libsounddrv_a_SOURCES = \
	soundalsa.c \
	soundbeos.cc \
	soundbsp.cc \
	soundcoreaudio.c \
	sounddx.c \
	soundflac.c \
	soundmp3.c \
	soundpulse.c \
	soundsdl.c \
	soundsun.c \
	soundvorbis.c \
	soundwmm.c \
	lamelib.c

# These sources are always built.
libsounddrv_a_SOURCES = \
	soundaiff.c \
	sounddummy.c \
	sounddump.c \
	soundfs.c \
	soundiff.c \
	soundmovie.c \
	soundvoc.c \
	soundwav.c

noinst_HEADERS = \
  soundmovie.h

libsounddrv_a_DEPENDENCIES = \
	@SOUND_DRIVERS@ \
	soundaiff.o \
	sounddummy.o \
	sounddump.o \
	soundfs.o \
	soundiff.o \
	soundvoc.o \
	soundwav.o

libsounddrv_a_LIBADD = @SOUND_DRIVERS@

EXTRA_DIST = \
	lamelib.h
