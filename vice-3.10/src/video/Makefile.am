AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/raster \
	-I$(top_srcdir)/src/joyport

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libvideo.a

libvideo_a_SOURCES = \
	render-common.h \
	render1x1.c \
	render1x1.h \
	render1x1rgbi.c \
	render1x1rgbi.h \
	render1x1ntsc.c \
	render1x1ntsc.h \
	render1x1pal.c \
	render1x1pal.h \
	render1x2.c \
	render1x2.h \
	render1x2rgbi.c \
	render1x2rgbi.h \
	render2x2.c \
	render2x2.h \
	render2x2rgbi.c \
	render2x2rgbi.h \
	render2x2ntsc.c \
	render2x2ntsc.h \
	render2x2pal.c \
	render2x2pal.h \
	render2x2palu.c \
	render2x2palu.h \
	render2x4.c \
	render2x4.h \
	render2x4rgbi.c \
	render2x4rgbi.h \
	renderscale2x.c \
	renderscale2x.h \
	video-canvas.c \
	video-canvas.h \
	video-cmdline-options.c \
	video-color.c \
	video-color.h \
	video-render-crtmono.c \
	video-render-palntsc.c \
	video-render-rgbi.c \
	video-render.c \
	video-render.h \
	video-resources.c \
	video-resources.h \
	video-sound.c \
	video-sound.h \
	video-viewport.c

EXTRA_DIST = render-common.c
