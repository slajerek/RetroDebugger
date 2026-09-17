SUBDIRS = p64 linenoise-ng libzmbv md5

if HAVE_USBSID
SUBDIRS += libusbsiddrv
endif

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

noinst_HEADERS = \
	opencbm.h

.PHONY: libp64

libp64:
	(cd p64; $(MAKE))

.PHONY: libzmbv

libzmbv:
	(cd libzmbv; $(MAKE))

.PHONY: libusbsiddrv
if HAVE_USBSID
libusbsiddrv:
	(cd libusbsiddrv; $(MAKE))
endif
