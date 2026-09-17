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


noinst_LIBRARIES = libdrivetcbm.a

libdrivetcbm_a_SOURCES = \
	glue1551.c \
	glue1551.h \
	mem1551.c \
	mem1551.h \
	tcbm-cmdline-options.c \
	tcbm-cmdline-options.h \
	tcbm-resources.c \
	tcbm-resources.h \
	tcbm.c \
	tcbmrom.c \
	tcbmrom.h \
	tpid.c \
	tpid.h
