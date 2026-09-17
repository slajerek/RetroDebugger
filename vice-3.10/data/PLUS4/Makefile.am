plus4dir = $(VICE_DATADIR)/PLUS4

roms = \
	3plus1-317053-01.bin \
	3plus1-317054-01.bin \
	basic-318006-01.bin \
	kernal-318004-05.bin \
	kernal-318005-05.bin \
	kernal-318004-01.bin \
	kernal-364.bin \
	c2lo-364.bin

romsets = \
	default.vrs

palettes = \
	yape-pal.vpl \
	yape-ntsc.vpl \
	colodore_ted.vpl \
	ITU-R_BT601_CRT.vpl \
	ITU-R_BT709_HDTV.vpl \
	ITU-R_BT2020.vpl

gtk3_keymaps = \
	gtk3_pos.vkm \
	gtk3_pos_de.vkm \
	gtk3_pos_es.vkm \
	gtk3_sym.vkm \
	gtk3_sym_fr.vkm \
	gtk3_sym_de.vkm \
	gtk3_sym_it.vkm \
	gtk3_pos_it.vkm \
	gtk3_c16_pos_it.vkm \
	gtk3_c116_pos_it.vkm

sdl_keymaps = \
	sdl_sym_de.vkm \
	sdl_sym.vkm \
	sdl_pos_fr.vkm \
	sdl_pos.vkm

if USE_SDLUI
plus4_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps)
EXTRA_DIST = $(plus4_DATA) $(gtk3_keymaps)
endif

if USE_SDL2UI
plus4_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps)
EXTRA_DIST = $(plus4_DATA) $(gtk3_keymaps)
endif

if USE_GTK3UI
plus4_DATA = $(roms) $(romsets) $(palettes) $(gtk3_keymaps)
EXTRA_DIST = $(plus4_DATA) $(sdl_keymaps)
endif

if USE_HEADLESSUI
plus4_DATA = $(roms) $(romsets) $(palettes)
EXTRA_DIST = $(plus4_DATA) $(sdl_keymaps) $(gtk3_keymaps)
endif
