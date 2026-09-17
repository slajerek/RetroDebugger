AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/core/rtc

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libdriveiec128dcr.a

libdriveiec128dcr_a_SOURCES = \
	iec128dcr-cmdline-options.c \
	iec128dcr-cmdline-options.h \
	iec128dcr-resources.c \
	iec128dcr-resources.h \
	iec128dcr.c \
	iec128dcrrom.c \
	iec128dcrrom.h
