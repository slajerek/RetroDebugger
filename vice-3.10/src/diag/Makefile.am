AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/c64 \
	-I$(top_srcdir)/src/joyport \
	-I$(top_srcdir)/src/datasette \
	-I$(top_srcdir)/src/tapeport

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libdiag.a

libdiag_a_SOURCES = \
	c64_diag_586220_harness.c \
	c64_diag_586220_harness.h
