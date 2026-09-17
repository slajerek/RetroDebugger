AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/crtc \
	-I$(top_srcdir)/src/datasette \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/printerdrv \
	-I$(top_srcdir)/src/sid \
	-I$(top_srcdir)/src/raster \
	-I$(top_srcdir)/src/userport \
	-I$(top_srcdir)/src/video \
	-I$(top_srcdir)/src/tape \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/joyport \
	-I$(top_srcdir)/src/samplerdrv \
	-I$(top_srcdir)/src/rs232drv \
	-I$(top_srcdir)/src/tapeport \
	-I$(top_srcdir)/src/core/rtc

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libpet.a libpetstubs.a

libpetstubs_a_SOURCES = \
	pet-stubs.c

libpet_a_SOURCES = \
	6809.c \
	6809.h \
	debugcart.c \
	debugcart.h \
	pet-cmdline-options.c \
	pet-cmdline-options.h \
	pet-resources.c \
	pet-resources.h \
	pet-sidcart.c \
	pet-snapshot.c \
	pet-snapshot.h \
	pet.c \
	pet.h \
	petacia.h \
	petacia1.c \
	petbus.c \
	petcolour.c \
	petcolour.h \
	petcpu.c \
	petdatasette.c \
	petdrive.c \
	petdww.c \
	petdww.h \
	pethre.c \
	pethre.h \
	petiec.c \
	petiec.h \
	petio.c \
	petmem.c \
	petmem.h \
	petmemsnapshot.c \
	petmemsnapshot.h \
	petmodel.c \
	petmodel.h \
	petpia.h \
	petpia1.c \
	petpia2.c \
	petprinter.c \
	petreu.c \
	petreu.h \
	petrom.c \
	petrom.h \
	petromset.c \
	pets.h \
	petsound.c \
	petsound.h \
	petvia.c \
	petvia.h \
	petvideo.c
