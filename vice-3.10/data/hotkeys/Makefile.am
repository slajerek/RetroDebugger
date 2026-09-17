hotkeysdir = $(VICE_DATADIR)/hotkeys

vhk_files = hotkeys.vhk \
	    hotkeys-cartridge.vhk \
	    hotkeys-debug.vhk \
	    hotkeys-drive.vhk \
	    hotkeys-datasette.vhk \
	    hotkeys-fliplist.vhk \
	    hotkeys-help.vhk \
	    hotkeys-settings.vhk \
	    hotkeys-snapshot.vhk \
	    hotkeys-vsid.vhk \
	    hotkeys-winvice-legacy.vhk

EXTRA_DIST = $(vhk_files)
# Do not install hotkeys files when configured for headless
if !USE_HEADLESSUI
hotkeys_DATA = $(vhk_files)
endif
