petdir = $(VICE_DATADIR)/PET

roms = \
	basic-1.901439-09-05-02-06.bin \
	basic-2.901465-01-02.bin \
	basic-4.901465-23-20-21.bin \
	kernal-1.901439-04-07.bin \
	kernal-2.901465-03.bin \
	kernal-4.901465-22.bin \
	edit-1-n.901439-03.bin \
	edit-2-n.901447-24.bin \
	edit-2-b.901474-01.bin \
	edit-4-40-b-50Hz.ts.bin \
	edit-4-40-b-60Hz.ts.bin \
	edit-4-40-n-50Hz.901498-01.bin \
	edit-4-40-n-60Hz.901499-01.bin \
	edit-4-80-b-50Hz.901474-04.bin \
	edit-4-80-b-50Hz.901474-04_.bin \
	edit-4-80-b-60Hz.901474-03.bin \
	edit-4-b-noCRTC.901474-02.bin \
	chargen.de \
	characters-1.901447-08.bin \
	characters-2.901447-10.bin \
	characters.901640-01.bin \
	waterloo-a000.901898-01.bin \
	waterloo-b000.901898-02.bin \
	waterloo-c000.901898-03.bin \
	waterloo-d000.901898-04.bin \
	waterloo-e000.901897-01.bin \
	waterloo-f000.901898-05.bin \
	hre-9000.324992-02.bin \
	hre-a000.324993-02.bin

romsets = \
	rom1g.vrs \
	rom2b.vrs \
	rom2g.vrs \
	rom4b40.vrs \
	rom4b80.vrs \
	rom4g40.vrs \
	romsuperpet.vrs \
	hre.vrs

palettes = \
	2001-blueish.vpl \
	amber.vpl \
	green.vpl \
	white.vpl

gtk3_keymaps = \
	gtk3_sym.vkm \
	gtk3_pos.vkm \
	gtk3_bude_pos.vkm \
	gtk3_bude_pos_de.vkm \
	gtk3_bude_sym.vkm \
	gtk3_bude_sym_de.vkm \
	gtk3_grus_pos.vkm \
	gtk3_grus_pos_de.vkm \
	gtk3_grus_sym.vkm \
	gtk3_grus_sym_de.vkm \
	gtk3_buuk_pos.vkm \
	gtk3_buuk_pos_de.vkm \
	gtk3_buuk_sym.vkm \
	gtk3_buuk_sym_de.vkm

sdl_keymaps = \
	sdl_sym.vkm \
	sdl_pos.vkm \
	sdl_buuk_sym.vkm \
	sdl_buuk_sym_de.vkm \
	sdl_buuk_pos.vkm \
	sdl_buuk_pos_de.vkm \
	sdl_grus_sym.vkm \
	sdl_grus_sym_de.vkm \
	sdl_grus_pos.vkm \
	sdl_grus_pos_de.vkm

if USE_SDLUI
pet_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps)
EXTRA_DIST = $(pet_DATA) $(gtk3_keymaps)
endif

if USE_SDL2UI
pet_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps)
EXTRA_DIST = $(pet_DATA) $(gtk3_keymaps)
endif

if USE_GTK3UI
pet_DATA = $(roms) $(romsets) $(palettes) $(gtk3_keymaps)
EXTRA_DIST = $(pet_DATA) $(sdl_keymaps)
endif

if USE_HEADLESSUI
pet_DATA = $(roms) $(romsets) $(palettes)
EXTRA_DIST = $(pet_DATA) $(sdl_keymaps) $(gtk3_keymaps)
endif
