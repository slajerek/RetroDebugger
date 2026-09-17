c64dtvdir = $(VICE_DATADIR)/C64DTV

roms = \
	basic-901226-01.bin \
	chargen-901225-01.bin \
	kernal-901227-03.bin \
	dtvrom.bin

romsets = \
	default.vrs

palettes = \
	spiff.vpl

gtk3_keymaps = \
	gtk3_keyrah.vkm \
	gtk3_keyrah_de.vkm \
	gtk3_the64_de.vkm \
	gtk3_pos.vkm \
	gtk3_pos_de.vkm \
	gtk3_pos_se.vkm \
	gtk3_pos_it.vkm \
	gtk3_pos_es.vkm \
	gtk3_pos_uk.vkm \
	gtk3_pos_tr.vkm \
	gtk3_sym.vkm \
	gtk3_sym_de.vkm \
	gtk3_sym_be.vkm \
	gtk3_sym_da.vkm \
	gtk3_sym_nl.vkm \
	gtk3_sym_it.vkm \
	gtk3_sym_se.vkm \
	gtk3_sym_fr.vkm \
	gtk3_sym_uk.vkm \
	gtk3_sym_tr.vkm \
	gtk3_pos_bmc64.vkm

sdl_keymaps = \
	sdl_joymap_8bitdo_6b.vjm \
	sdl_keyrah.vkm \
	sdl_keyrah_de.vkm \
	sdl_keyrah_retropie.vkm \
	sdl_keyrah_retropie_de.vkm \
	sdl_keyrah_combian.vkm \
	sdl_pos.vkm \
	sdl_pos_uk.vkm \
	sdl_pos_ch.vkm \
	sdl_pos_da.vkm \
	sdl_pos_de.vkm \
	sdl_pos_fi.vkm \
	sdl_pos_no.vkm \
	sdl_pos_tr.vkm \
	sdl_sym.vkm \
	sdl_sym_uk.vkm \
	sdl_sym_de.vkm \
	sdl_sym_fr.vkm \
	sdl_sym_nl.vkm \
	sdl_sym_tr.vkm \
	sdl_sym_da.vkm \
	sdl_sym_us_pi_4-500_bmc64.vkm \
	sdl_sym_uk_pi_4-500_bmc64.vkm \
	sdl_sym_no_pi_4-500_bmc64.vkm \
	sdl_c64p.vkm

sym_file = \
	c64mem.sym

if USE_SDLUI
c64dtv_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps) $(sym_file)
EXTRA_DIST = $(c64dtv_DATA) $(gtk3_keymaps)
endif

if USE_SDL2UI
c64dtv_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps) $(sym_file)
EXTRA_DIST = $(c64dtv_DATA) $(gtk3_keymaps)
endif

if USE_GTK3UI
c64dtv_DATA = $(roms) $(romsets) $(palettes) $(gtk3_keymaps) $(sym_file)
EXTRA_DIST = $(c64dtv_DATA) $(sdl_keymaps)
endif

if USE_HEADLESSUI
c64dtv_DATA = $(roms) $(romsets) $(palettes) $(sym_file)
EXTRA_DIST = $(c64dtv_DATA) $(sdl_keymaps) $(gtk3_keymaps)
endif
