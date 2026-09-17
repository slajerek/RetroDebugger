vic20dir = $(VICE_DATADIR)/VIC20

roms = \
	basic-901486-01.bin \
	chargen-901460-03.bin \
	chargen-901460-02.bin \
	kernal.901486-02.bin \
	kernal.901486-06.bin \
	kernal.901486-07.bin

romsets = \
	default.vrs

palettes = \
	vice.vpl \
	mike-pal.vpl \
	mike-ntsc.vpl \
	colodore_vic.vpl \
	PALette.vpl

gtk3_keymaps = \
	gtk3_pos_it.vkm \
	gtk3_sym_it.vkm \
	gtk3_pos_de.vkm \
	gtk3_sym_de.vkm \
	gtk3_sym_fr.vkm \
	gtk3_pos.vkm \
	gtk3_sym.vkm

sdl_keymaps = \
	sdl_sym.vkm \
	sdl_sym_de.vkm \
	sdl_sym_uk.vkm \
	sdl_pos.vkm \
	sdl_pos_de.vkm \
	sdl_pos_uk.vkm \
	sdl_pos_fr.vkm \
	sdl_pos_fi.vkm

if USE_SDLUI
vic20_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps)
EXTRA_DIST = $(vic20_DATA) $(gtk3_keymaps)
endif

if USE_SDL2UI
vic20_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps)
EXTRA_DIST = $(vic20_DATA) $(gtk3_keymaps)
endif

if USE_GTK3UI
vic20_DATA = $(roms) $(romsets) $(palettes) $(gtk3_keymaps)
EXTRA_DIST = $(vic20_DATA) $(sdl_keymaps)
endif

if USE_HEADLESSUI
vic20_DATA = $(roms) $(romsets) $(palettes)
EXTRA_DIST = $(vic20_DATA) $(sdl_keymaps) $(gtk3_keymaps)
endif
