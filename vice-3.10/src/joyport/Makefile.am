# Makefile.am for `libjoyport.a'.

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_srcdir)/src/arch/shared/ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/samplerdrv \
	-I$(top_srcdir)/src/core/rtc \
	-I$(top_srcdir)/src/userport \
	-I$(top_srcdir)/src/diag

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@

noinst_LIBRARIES = libjoyport.a

# These sources are always built.
libjoyport_a_SOURCES = \
	bbrtc.c \
	bbrtc.h \
	cardkey.c \
	cardkey.h \
	coplin_keypad.c \
	coplin_keypad.h \
	cx21.c \
	cx21.h \
	cx85.c \
	cx85.h \
	inception.c \
	inception.h \
	joyport.c \
	joyport.h \
	joyport_diag_586220_harness.c \
	joyport_diag_586220_harness.h \
	joyport_io_sim.c \
	joyport_io_sim.h \
	joystick.c \
	joystick.h \
	lightpen.c \
	lightpen.h \
	mouse.c \
	mouse.h \
	mouse_1351.c \
	mouse_1351.h \
	mouse_neos.c \
	mouse_neos.h \
	mouse_paddle.c \
	mouse_paddle.h \
	mouse_quadrature.c \
	mouse_quadrature.h \
	multijoy.c \
	multijoy.h \
	ninja_snespad.c \
	ninja_snespad.h \
	paperclip64.c \
	paperclip64.h \
	paperclip64e.c \
	paperclip64e.h \
	paperclip64sc.c \
	paperclip64sc.h \
	paperclip2.c \
	paperclip2.h \
	powerpad.c \
	powerpad.h \
	protopad.c \
	protopad.h \
	rushware_keypad.c \
	rushware_keypad.h \
	sampler2bit.c \
	sampler2bit.h \
	sampler4bit.c \
	sampler4bit.h \
	script64_dongle.c \
	script64_dongle.h \
	spaceballs.c \
	spaceballs.h \
	trapthem_snespad.c \
	trapthem_snespad.h \
	vizawrite64_dongle.c \
	vizawrite64_dongle.h \
	waasoft_dongle.c \
	waasoft_dongle.h
