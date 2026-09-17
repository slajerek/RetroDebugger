AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/cbm2 \
	-I$(top_srcdir)/src/arch/shared

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libcbm2cart.a

libcbm2cart_a_SOURCES = \
	cbm2cart.h \
	cbm2cart.c \
	cbm2-generic.h \
	cbm2-generic.c \
	debugcart.h \
	debugcart.c
