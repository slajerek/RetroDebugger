AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/plus4 \
	-I$(top_srcdir)/src/joyport \
	-I$(top_srcdir)/src/samplerdrv \
	-I$(top_srcdir)/src/sid \
	-I$(top_srcdir)/src/arch/shared

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libplus4cart.a

libplus4cart_a_SOURCES = \
	plus4cart.h \
	plus4cart.c \
	debugcart.h \
	debugcart.c \
	digiblaster.c \
	digiblaster.h \
	plus4-generic.h \
	plus4-generic.c \
	plus4-sidcart.c \
	speedy.h \
	speedy.c \
	jacint1mb.h \
	jacint1mb.c \
	magiccart.h \
	magiccart.c \
	multicart.h \
	multicart.c
