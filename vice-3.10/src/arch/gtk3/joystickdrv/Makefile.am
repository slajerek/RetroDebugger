# Makefile.am for `libjoystickdrv.a'.

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_srcdir)/src/joyport \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src

if HAVE_LINUX_EVDEV
AM_CPPFLAGS += -D_XOPEN_SOURCE=700
endif

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libjoystickdrv.a

if WINDOWS_COMPILE
libjoystickdrv_a_SOURCES = joystick_win32_directinput.c
EXTRA_libjoystickdrv_a_SOURCES = \
	joystick_bsd.c \
	joystick_linux_evdev.c \
	joystick_osx.c
endif

if MACOS_COMPILE
libjoystickdrv_a_SOURCES = joystick_osx.c
EXTRA_libjoystickdrv_a_SOURCES = \
	joystick_bsd.c \
	joystick_linux_evdev.c \
	joystick_win32_directinput.c
endif

if LINUX_COMPILE
libjoystickdrv_a_SOURCES = joystick_linux_evdev.c
EXTRA_libjoystickdrv_a_SOURCES = \
	joystick_bsd.c \
	joystick_osx.c \
	joystick_win32_directinput.c
endif

if BSD_COMPILE
libjoystickdrv_a_SOURCES = joystick_bsd.c
EXTRA_libjoystickdrv_a_SOURCES = \
	joystick_linux_evdev.c \
	joystick_osx.c \
	joystick_win32_directinput.c
endif

libjoystickdrv_a_DEPENDENCIES = \
	@JOYSTICK_DRIVERS@

libjoystickdrv_a_LIBADD = \
        @JOYSTICK_DRIVERS@
