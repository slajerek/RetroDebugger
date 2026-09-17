AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@

SUBDIRS = rtc

noinst_LIBRARIES = libcore.a

libcore_a_SOURCES = \
	ata.c \
	ata.h \
	ciacore.c \
	ciatimer.c \
	ciatimer.h \
	cs8900.c \
	cs8900.h \
	flash040core.c \
	flash800core.c \
	fmopl.c \
	fmopl.h \
	i8255a.c \
	i8255a.h \
	mc6821core.c \
	mc6821core.h \
	m93c86.c \
	m93c86.h \
	riotcore.c \
	ser-eeprom.c \
	ser-eeprom.h \
	scsi.c \
	scsi.h \
	spi-flash.c \
	spi-flash.h \
	spi-sdcard.c \
	spi-sdcard.h \
	t6721.c \
	t6721.h \
	tpicore.c \
	viacore.c
