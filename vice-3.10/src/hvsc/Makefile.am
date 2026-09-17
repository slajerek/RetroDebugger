noinst_LIBRARIES = libhvsc.a

libhvsc_a_SOURCES = \
	base.c \
	bugs.c \
	hvsc_defs.h \
	hvsc.h \
	main.c \
	psid.c \
	sldb.c \
	stil.c

EXTRA_DIST = \
	base.h \
	bugs.h \
	hvsc_defs.h \
	hvsc.h \
	main.h \
	psid.h \
	sldb.h \
	stil.c \
	stil.h

AM_CPPFLAGS = @VICE_CPPFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/lib/md5

AM_CFLAGS = -pedantic @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@
