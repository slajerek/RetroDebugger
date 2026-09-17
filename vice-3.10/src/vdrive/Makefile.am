AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/core/rtc \
	-I$(top_srcdir)/src

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libvdrive.a

libvdrive_a_SOURCES = \
	vdrive-bam.c \
	vdrive-bam.h \
	vdrive-command.c \
	vdrive-command.h \
	vdrive-dir.c \
	vdrive-dir.h \
	vdrive-iec.c \
	vdrive-iec.h \
	vdrive-internal.c \
	vdrive-internal.h \
	vdrive-rel.c \
	vdrive-rel.h \
	vdrive-snapshot.c \
	vdrive-snapshot.h \
	vdrive.c \
	vdrive.h

