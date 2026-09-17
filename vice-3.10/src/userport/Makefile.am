AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/c64/cart \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/core/rtc \
	-I$(top_srcdir)/src/joyport \
	-I$(top_srcdir)/src/samplerdrv \
	-I$(top_srcdir)/src/diag \
	-I$(top_srcdir)/src/printerdrv \
	-I$(top_srcdir)/src/rs232drv \
	-I$(top_srcdir)/src/c64 \
	-I$(top_srcdir)/src/plus4 \
	-I$(top_srcdir)/src/drive

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libuserport.a

libuserport_a_SOURCES = \
	userport.c \
	userport.h \
	userport_4bit_sampler.c \
	userport_4bit_sampler.h \
	userport_8bss.c \
	userport_8bss.h \
	userport_dac.c \
	userport_dac.h \
	userport_diag_586220_harness.c \
	userport_diag_586220_harness.h \
	userport_diag_pin.c \
	userport_diag_pin.h \
	userport_digimax.c \
	userport_digimax.h \
	userport_hks_joystick.c \
	userport_hks_joystick.h \
	userport_hummer_joystick.c \
	userport_hummer_joystick.h \
	userport_io_sim.c \
	userport_io_sim.h \
	userport_joystick.c \
	userport_joystick.h \
	userport_petscii_snespad.c \
	userport_petscii_snespad.h \
	userport_ps2mouse.c \
	userport_ps2mouse.h \
	userport_rtc_58321a.c \
	userport_rtc_58321a.h \
	userport_rtc_ds1307.c \
	userport_rtc_ds1307.h \
	userport_spt_joystick.c \
	userport_spt_joystick.h \
	userport_superpad64.c \
	userport_superpad64.h \
	userport_synergy_joystick.c \
	userport_synergy_joystick.h \
	userport_wic64.c \
	userport_wic64.h \
	userport_funmp3.c \
	userport_funmp3.h \
	userport_woj_joystick.c \
	userport_woj_joystick.h

