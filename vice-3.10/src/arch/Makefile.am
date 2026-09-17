.PHONY: libarch

if DUMMY_COMPILE
SUBDIRS = shared
endif

if USE_GTK3UI
SUBDIRS = shared gtk3
libarch:
	(cd gtk3; $(MAKE) libarch.a)
endif

if USE_SDLUI
SUBDIRS = shared sdl
libarch:
	(cd sdl; $(MAKE) libarch.a)
endif

if USE_SDL2UI
SUBDIRS = shared sdl
libarch:
	(cd sdl; $(MAKE) libarch.a)
endif

if USE_HEADLESSUI
SUBDIRS = shared headless
libarch:
	(cd headless; $(MAKE) libarch.a)
endif
