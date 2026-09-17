SUBDIRS = \
	iec \
	iec128dcr \
	iecieee \
	ieee \
	tcbm

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/monitor \
	-I$(top_srcdir)/src/vdrive \
	-I$(top_srcdir)/src/core \
	-I$(top_srcdir)/src/core/rtc \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/drive/iec \
	-I$(top_srcdir)/src/drive/tcbm \
	-I$(top_srcdir)/src/drive/ieee

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libdrive.a

libdrive_a_SOURCES = \
	drive-check.c \
	drive-check.h \
	drive-cmdline-options.c \
	drive-cmdline-options.h \
	drive-resources.c \
	drive-resources.h \
	drive-snapshot.c \
	drive-snapshot.h \
	drive-sound.c \
	drive-sound.h \
	drive-writeprotect.c \
	drive-writeprotect.h \
	drive.c \
	drive.h \
	drivecpu.c \
	drivecpu.h \
	drivecpu65c02.c \
	drivecpu65c02.h \
	driveimage.c \
	driveimage.h \
	drivemem.c \
	drivemem.h \
	driverom.c \
	driverom.h \
	drivesync.c \
	drivesync.h \
	drivetypes.h \
	iec-c64exp.h \
	iec-plus4exp.h \
	iec.h \
	iec128dcr.h \
	iecieee.h \
	ieee.h \
	rotation.c \
	rotation.h \
	tcbm.h \
	viad.h

.PHONY: libdriveiec libdriveiec128dcr libdriveiecieee libdriveieee libdrivetcbm

libdriveiec:
	(cd iec; $(MAKE))
libdriveiec128dcr:
	(cd iec128dcr; $(MAKE))
libdriveiecieee:
	(cd iecieee; $(MAKE))
libdriveieee:
	(cd ieee; $(MAKE))
libdrivetcbm:
	(cd tcbm; $(MAKE))

