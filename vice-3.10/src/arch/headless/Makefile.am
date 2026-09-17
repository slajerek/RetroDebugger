## Makefile.am for the native headless version of `libarch.a'.

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_srcdir)/src/arch/shared \
	$(extra_includes) \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/crtc \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/joyport \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/raster \
	-I$(top_srcdir)/src/rs232drv \
	-I$(top_srcdir)/src/sid \
	-I$(top_srcdir)/src/vdc \
	-I$(top_srcdir)/src/c64 \
	-I$(top_srcdir)/src/c64dtv \
	-I$(top_srcdir)/src/c128 \
	-I$(top_srcdir)/src/cbm2 \
	-I$(top_srcdir)/src/pet \
	-I$(top_srcdir)/src/plus4 \
	-I$(top_srcdir)/src/scpu64 \
	-I$(top_srcdir)/src/vic20 \
	-I$(top_srcdir)/src/samplerdrv \
	-I$(top_srcdir)/src/c64/cart \
	-I$(top_srcdir)/src/imagecontents \
	-I$(top_srcdir)/src/monitor \
	-I$(top_srcdir)/src/tapeport \
	-I$(top_srcdir)/src/core/rtc \
	-I$(top_srcdir)/src/hvsc \
	-I$(top_srcdir)/src/datasette

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_OBJCFLAGS = @VICE_OBJCFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@

noinst_LIBRARIES = libarch.a libtoolarch.a

libarch_a_SOURCES = \
	archdep.c \
	kbd.c \
	console.c \
	ui.c \
	uimon.c \
	uistatusbar.c \
	main.c \
	video.c \
	vsidui.c \
	vsyncarch.c \
	c64scui.c \
	mousedrv.c \
	c64dtvui.c \
	scpu64ui.c \
	c128ui.c \
	vic20ui.c \
	petui.c \
	plus4ui.c \
	cbm2ui.c \
	cbm5x0ui.c \
	c64ui.c

libtoolarch_a_SOURCES = \
	archdep.c

EXTRA_DIST = \
	archdep.h \
	debug_headless.h \
	kbd.h \
	mousedrv.h \
	ui.h \
	uistatusbar.h \
	videoarch.h \
	make-bindist_win32.sh
