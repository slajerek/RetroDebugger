SUBDIRS = cart

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/c64/cart \
	-I$(top_srcdir)/src/datasette \
	-I$(top_srcdir)/src/printerdrv \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/drive/iec/c64exp \
	-I$(top_srcdir)/src/raster \
	-I$(top_srcdir)/src/sid \
	-I$(top_srcdir)/src/tape \
	-I$(top_srcdir)/src/userport \
	-I$(top_srcdir)/src/viciisc \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/video \
	-I$(top_srcdir)/src/core/rtc \
	-I$(top_srcdir)/src/joyport \
	-I$(top_srcdir)/src/samplerdrv \
	-I$(top_srcdir)/src/tapeport \
	-I$(top_srcdir)/src/core \
	-I$(top_srcdir)/src/rs232drv \
	-I$(top_srcdir)/src/hvsc \
	-I$(top_srcdir)/src/vdrive \
	-I$(top_srcdir)/src/fsdevice \
	-I$(top_srcdir)/src/diag \
	-I$(top_srcdir)/src/imagecontents

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libvsid.a libvsidstubs.a libc64.a libc64stubs.a libc64sc.a libc64scstubs.a libc64c128.a libc64c64dtv.a libc64scpu64.a

libvsidstubs_a_SOURCES = \
	vsid-stubs.c

libvsid_a_SOURCES = \
	vsid-cmdline-options.c \
	vsid-cmdline-options.h \
	vsid-resources.c \
	vsid-snapshot.c \
	c64gluelogic.c \
	c64gluelogic.h \
	c64keyboard.c \
	c64keyboard.h \
	c64meminit.h \
	c64memlimit.c \
	c64memlimit.h \
	c64memrom.c \
	c64memrom.h \
	c64memsnapshot.h \
	c64model.c \
	c64model.h \
	c64parallel.h \
	c64pla.h \
	c64rom.c \
	c64rom.h \
	c64romset.c \
	c64rsuser.c \
	c64rsuser.h \
	c64video.c \
	vsid-debugcart.c \
	vsid-debugcart.h \
	musdrv.h \
	psid.c \
	psid.h \
	psiddrv.a65 \
	psiddrv.h \
	reloc65.c \
	vsid.c \
	vsidcia1.c \
	vsidcia2.c \
	vsidcpu.c \
	vsidmem.c \
	vsidmeminit.c \
	vsidmemsnapshot.c \
	vsidpla.c \
	vsidsound.c

libc64stubs_a_SOURCES = \
	c64-stubs.c

libc64_a_SOURCES = \
	c64-cmdline-options.c \
	c64-cmdline-options.h \
	c64-memory-hacks.c \
	c64-memory-hacks.h \
	c64-resources.c \
	c64-resources.h \
	c64-snapshot.c \
	c64-snapshot.h \
	c64.c \
	c64.h \
	c64_256k.c \
	c64_256k.h \
	c64bus.c \
	c64cart.h \
	c64cia.h \
	c64cia1.c \
	c64cia2.c \
	c64cpu.c \
	c64datasette.c \
	c64drive.c \
	c64export.c \
	c64fastiec.c \
	c64fastiec.h \
	c64gluelogic.c \
	c64gluelogic.h \
	c64iec.c \
	c64iec.h \
	c64io.c \
	c64keyboard.c \
	c64keyboard.h \
	c64mem.c \
	c64mem.h \
	c64meminit.c \
	c64meminit.h \
	c64memlimit.c \
	c64memlimit.h \
	c64memrom.c \
	c64memrom.h \
	c64memsnapshot.c \
	c64memsnapshot.h \
	c64model.c \
	c64model.h \
	c64parallel.c \
	c64parallel.h \
	c64pla.c \
	c64pla.h \
	c64printer.c \
	c64rom.c \
	c64rom.h \
	c64romset.c \
	c64rsuser.c \
	c64rsuser.h \
	c64sound.c \
	c64video.c \
	plus256k.c \
	plus256k.h \
	plus60k.c \
	plus60k.h

libc64scstubs_a_SOURCES = \
	c64sc-stubs.c

libc64sc_a_SOURCES = \
	c64-cmdline-options.c \
	c64-cmdline-options.h \
	c64-memory-hacks.c \
	c64-memory-hacks.h \
	c64-resources.c \
	c64-resources.h \
	c64-snapshot.c \
	c64-snapshot.h \
	c64.c \
	c64.h \
	c64_256k.c \
	c64_256k.h \
	c64bus.c \
	c64cart.h \
	c64cia.h \
	c64cia1.c \
	c64cia2.c \
	c64cpusc.c \
	c64datasette.c \
	c64drive.c \
	c64export.c \
	c64fastiec.c \
	c64fastiec.h \
	c64gluelogic.c \
	c64gluelogic.h \
	c64iec.c \
	c64iec.h \
	c64io.c \
	c64keyboard.c \
	c64keyboard.h \
	c64memsc.c \
	c64mem.h \
	c64meminit.c \
	c64meminit.h \
	c64memlimit.c \
	c64memlimit.h \
	c64memrom.c \
	c64memrom.h \
	c64memsnapshot.c \
	c64memsnapshot.h \
	c64scmodel.c \
	c64model.h \
	c64parallel.c \
	c64parallel.h \
	c64pla.c \
	c64pla.h \
	c64printer.c \
	c64rom.c \
	c64rom.h \
	c64romset.c \
	c64rsuser.c \
	c64rsuser.h \
	c64sound.c \
	c64video.c \
	plus256k.c \
	plus256k.h \
	plus60k.c \
	plus60k.h

libc64c64dtv_a_SOURCES = \
	c64bus.c \
	c64drive.c \
	c64fastiec.c \
	c64fastiec.h \
	c64keyboard.c \
	c64keyboard.h \
	c64rom.c \
	c64rom.h \
	c64romset.c \
	c64rsuser.c \
	c64rsuser.h \
	c64video.c

libc64c128_a_SOURCES = \
	c64bus.c \
	c64cart.h \
	c64cia.h \
	c64cia2.c \
	c64datasette.c \
	c64export.c \
	c64gluelogic.c \
	c64gluelogic.h \
	c64iec.c \
	c64iec.h \
	c64io.c \
	c64keyboard.c \
	c64keyboard.h \
	c64meminit.c \
	c64meminit.h \
	c64memlimit.c \
	c64memlimit.h \
	c64memrom.c \
	c64memrom.h \
	c64printer.c \
	c64pla.c \
	c64pla.h \
	c64parallel.c \
	c64parallel.h \
	c64rsuser.c \
	c64rsuser.h \
	c64sound.c

libc64scpu64_a_SOURCES = \
	c64bus.c \
	c64cart.h \
	c64cia.h \
	c64cia1.c \
	c64cia2.c \
	c64datasette.c \
	c64drive.c \
	c64export.c \
	c64fastiec.c \
	c64fastiec.h \
	c64iec.c \
	c64iec.h \
	c64io.c \
	c64keyboard.c \
	c64keyboard.h \
	c64memsnapshot.c \
	c64memsnapshot.h \
	c64model.h \
	c64parallel.c \
	c64parallel.h \
	c64printer.c \
	c64romset.c \
	c64rsuser.c \
	c64rsuser.h \
	c64sound.c \
	c64video.c

BUILT_SOURCES = psiddrv.h

EXTRA_DIST =

SUFFIXES = .a65

.a65.h:
	$(XA) -R -G $< -o $*.o65 2>/dev/null
	od -v -An -tx1 $*.o65 | sed 's/\([0-9a-f][0-9a-f]\)/0x\1,/g' > $@

.PHONY: libc64cart libc64cartsystem libc64commoncart

libc64cart:
libc64cartsystem:
libc64commoncart:
	(cd cart; $(MAKE))
