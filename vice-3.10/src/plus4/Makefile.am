SUBDIRS = cart

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/plus4/cart \
	-I$(top_srcdir)/src/core \
	-I$(top_srcdir)/src/datasette \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/printerdrv \
	-I$(top_srcdir)/src/sid \
	-I$(top_srcdir)/src/tape \
	-I$(top_srcdir)/src/monitor \
	-I$(top_srcdir)/src/video \
	-I$(top_srcdir)/src/samplerdrv \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/joyport \
	-I$(top_srcdir)/src/userport \
	-I$(top_srcdir)/src/raster \
	-I$(top_srcdir)/src/rs232drv \
	-I$(top_srcdir)/src/tapeport \
	-I$(top_srcdir)/src/core/rtc

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libplus4.a libplus4stubs.a

libplus4stubs_a_SOURCES = \
	plus4-stubs.c

libplus4_a_SOURCES = \
	plus4-cmdline-options.c \
	plus4-cmdline-options.h \
	plus4-resources.c \
	plus4-resources.h \
	plus4-snapshot.c \
	plus4-snapshot.h \
	plus4.c \
	plus4.h \
	plus4acia.c \
	plus4acia.h \
	plus4bus.c \
	plus4cpu.c \
	plus4datasette.c \
	plus4drive.c \
	plus4export.c \
	plus4iec.c \
	plus4iec.h \
	plus4io.c \
	plus4mem.c \
	plus4mem.h \
	plus4memcsory256k.c \
	plus4memcsory256k.h \
	plus4memhacks.c \
	plus4memhacks.h \
	plus4memhannes256k.c \
	plus4memhannes256k.h \
	plus4memlimit.c \
	plus4memlimit.h \
	plus4memrom.c \
	plus4memrom.h \
	plus4memsnapshot.c \
	plus4memsnapshot.h \
	plus4model.c \
	plus4model.h \
	plus4parallel.c \
	plus4parallel.h \
	plus4pio1.c \
	plus4pio1.h \
	plus4pio2.c \
	plus4pio2.h \
	plus4printer.c \
	plus4rom.c \
	plus4rom.h \
	plus4romset.c \
	plus4speech.c \
	plus4speech.h \
	plus4tcbm.c \
	plus4tcbm.h \
	plus4video.c \
	ted-badline.c \
	ted-badline.h \
	ted-cmdline-options.c \
	ted-cmdline-options.h \
	ted-color.c \
	ted-color.h \
	ted-draw.c \
	ted-draw.h \
	ted-fetch.c \
	ted-fetch.h \
	ted-irq.c \
	ted-irq.h \
	ted-mem.c \
	ted-mem.h \
	ted-resources.c \
	ted-resources.h \
	ted-snapshot.c \
	ted-snapshot.h \
	ted-sound.c \
	ted-sound.h \
	ted-timer.c \
	ted-timer.h \
	ted-timing.c \
	ted-timing.h \
	ted.c \
	ted.h \
	tedtypes.h

.PHONY: libplus4cart

libplus4cart:
	(cd cart; $(MAKE))
