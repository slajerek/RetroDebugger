AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/datasette \
	-I$(top_srcdir)/src/tapeport

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libtape.a

libtape_a_SOURCES = \
	t64.c \
	t64.h \
	tap.c \
	tape-internal.c \
	tape-internal.h \
	tape-snapshot.c \
	tape-snapshot.h \
	tape.c \
	tapeimage.c \
	tapeimage.h

