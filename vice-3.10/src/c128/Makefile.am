SUBDIRS = cart

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/datasette \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/printerdrv \
	-I$(top_srcdir)/src/c64 \
	-I$(top_srcdir)/src/c64/cart \
	-I$(top_srcdir)/src/c128 \
	-I$(top_srcdir)/src/c128/cart \
	-I$(top_srcdir)/src/raster \
	-I$(top_srcdir)/src/sid \
	-I$(top_srcdir)/src/tape \
	-I$(top_srcdir)/src/userport \
	-I$(top_srcdir)/src/vicii \
	-I$(top_srcdir)/src/video \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/vdc \
	-I$(top_srcdir)/src/core/rtc \
	-I$(top_srcdir)/src/joyport \
	-I$(top_srcdir)/src/samplerdrv \
	-I$(top_srcdir)/src/tapeport \
	-I$(top_srcdir)/src/rs232drv \
	-I$(top_srcdir)/src/core

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libc128.a libc128stubs.a

libc128stubs_a_SOURCES = \
	c128-stubs.c

libc128_a_SOURCES = \
	c128-cmdline-options.c \
	c128-cmdline-options.h \
	c128-resources.c \
	c128-resources.h \
	c128-snapshot.c \
	c128-snapshot.h \
	c128.c \
	c128.h \
	c128cia1.c \
	c128cpu.c \
	c128drive.c \
	c128fastiec.c \
	c128fastiec.h \
	c128mem.c \
	c128mem.h \
	c128meminit.c \
	c128meminit.h \
	c128memlimit.c \
	c128memlimit.h \
	c128memrom.c \
	c128memrom.h \
	c128memsnapshot.c \
	c128memsnapshot.h \
	c128mmu.c \
	c128mmu.h \
	c128model.c \
	c128model.h \
	c128rom.c \
	c128rom.h \
	c128romset.c \
	c128video.c \
	functionrom.c \
	functionrom.h \
	z80.c \
	z80.h \
	z80mem.c \
	z80mem.h
	
EXTRA_DIST = \
	c128cart.h

.PHONY: libc128cartsystem

libc128cartsystem:
	(cd cart; $(MAKE))
