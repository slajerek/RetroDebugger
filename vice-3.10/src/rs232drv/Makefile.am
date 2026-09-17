AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_srcdir)/src/rs232drv \
	-I$(top_srcdir)/src/userport \
	-I$(top_srcdir)/src/joyport \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = librs232drv.a librs232drvpet.a

librs232drv_a_SOURCES = \
	rs232dev.h \
	rs232.c \
	rs232.h \
	rs232drv.c \
	rs232drv.h \
	rs232net.c \
	rs232net.h \
	rsuser.c \
	rsuser.h

# Same as above, but without the userport bits, to avoid registering userport
# resources and command line options with xpet and xcbm2 (these have stubs for
# rsuser_resources_init() and rsuser_cmdline_init()
librs232drvpet_a_SOURCES = \
	rs232dev.h \
	rs232.c \
	rs232.h \
	rs232drv.c \
	rs232drv.h \
	rs232net.c \
	rs232net.h \
	rsuser.h
