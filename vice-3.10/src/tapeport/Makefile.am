AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/datasette \
	-I$(top_srcdir)/src/core/rtc \
	-I$(top_srcdir)/src/diag

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libtapeport.a

libtapeport_a_SOURCES = \
	cp-clockf83.c \
	cp-clockf83.h \
	dtl-basic-dongle.c \
	dtl-basic-dongle.h \
	sense-dongle.c \
	sense-dongle.h \
	wordcraft-dongle.c \
	wordcraft-dongle.h \
	tape_diag_586220_harness.c \
	tape_diag_586220_harness.h \
	tapecart.c \
	tapecart.h \
	tapecart-loader.h \
	tapeport.c \
	tapeport.h
