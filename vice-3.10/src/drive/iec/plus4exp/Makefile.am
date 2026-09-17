AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/drive/iec \
	-I$(top_srcdir)/src/userport \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/core/rtc

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libdriveiecplus4exp.a

libdriveiecplus4exp_a_SOURCES = \
	iec-plus4exp.c \
	plus4exp-cmdline-options.c \
	plus4exp-cmdline-options.h \
	plus4exp-resources.c \
	plus4exp-resources.h
