c64dir = $(VICE_DATADIR)/C64

roms = \
	basic-901226-01.bin \
	chargen-901225-01.bin \
	chargen-906143-02.bin \
	kernal-901227-01.bin \
	kernal-901227-02.bin \
	kernal-901227-03.bin \
	kernal-251104-04.bin \
	kernal-901246-01.bin \
	kernal-390852-01.bin \
	kernal-906145-02.bin

romsets = \
	default.vrs

palettes = \
	c64hq.vpl \
	c64s.vpl \
	ccs64.vpl \
	frodo.vpl \
	godot.vpl \
	pc64.vpl \
	vice.vpl \
	pepto-ntsc-sony.vpl \
	pepto-ntsc.vpl \
	pepto-pal.vpl \
	pepto-palold.vpl \
	community-colors.vpl \
	colodore.vpl \
	ptoing.vpl \
	deekay.vpl \
	cjam.vpl \
	palette.vpl \
	pixcen.vpl \
	rgb.vpl \
	the64.vpl \
	palette_6569R1_v1r.vpl \
	palette_6569R5_v1r.vpl \
	palette_8565R2_v1r.vpl \
	palette_C64_amber.vpl \
	palette_C64_cyan.vpl \
	palette_C64_green.vpl \
	lemon64.vpl

gtk3_keymaps = \
	gtk3_sym.vkm \
	gtk3_pos.vkm \
	gtk3_pos_de.vkm \
	gtk3_pos_se.vkm \
	gtk3_pos_it.vkm \
	gtk3_pos_es.vkm \
	gtk3_pos_uk.vkm \
	gtk3_pos_tr.vkm \
	gtk3_sym_de.vkm \
	gtk3_sym_be.vkm \
	gtk3_sym_nl.vkm \
	gtk3_sym_da.vkm \
	gtk3_sym_uk.vkm \
	gtk3_sym_tr.vkm \
	gtk3_keyrah.vkm \
	gtk3_keyrah_de.vkm \
	gtk3_the64_de.vkm \
	gtk3_sym_it.vkm \
	gtk3_sym_se.vkm \
	gtk3_sym_fr.vkm \
	gtk3_pos_bmc64.vkm

sdl_keymaps = \
	sdl_joymap_ps3.vjm \
	sdl_joymap_thec64.vjm \
	sdl_joymap_keyrah_v3.vjm \
	sdl_joymap_xbox360.vjm \
	sdl_joymap_8bitdo_6b.vjm \
	sdl_pos.vkm \
	sdl_pos_uk.vkm \
	sdl_pos_ch.vkm \
	sdl_pos_da.vkm \
	sdl_pos_de.vkm \
	sdl_pos_fi.vkm \
	sdl_pos_no.vkm \
	sdl_pos_tr.vkm \
	sdl_keyrah.vkm \
	sdl_keyrah_de.vkm \
	sdl_keyrah_retropie.vkm \
	sdl_keyrah_retropie_de.vkm \
	sdl_keyrah_combian.vkm \
	sdl_sym.vkm \
	sdl_sym_uk.vkm \
	sdl_sym_de.vkm \
	sdl_sym_nl.vkm \
	sdl_sym_it.vkm \
	sdl_sym_fr.vkm \
	sdl_sym_tr.vkm \
	sdl_sym_da.vkm \
	sdl_sym_us_pi_4-500_bmc64.vkm \
	sdl_sym_uk_pi_4-500_bmc64.vkm \
	sdl_sym_no_pi_4-500_bmc64.vkm \
	sdl_c64p.vkm

sym_file = \
	c64mem.sym

if USE_SDLUI
c64_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps) $(sym_file)
EXTRA_DIST = $(c64_DATA) $(gtk3_keymaps)
endif

if USE_SDL2UI
c64_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps) $(sym_file)
EXTRA_DIST = $(c64_DATA) $(gtk3_keymaps)
endif

if USE_GTK3UI
c64_DATA = $(roms) $(romsets) $(palettes) $(gtk3_keymaps) $(sym_file)
EXTRA_DIST = $(c64_DATA) $(sdl_keymaps)
endif

if USE_HEADLESSUI
c64_DATA = $(roms) $(romsets) $(palettes) $(sym_file)
EXTRA_DIST = $(c64_DATA) $(sdl_keymaps) $(gtk3_keymaps)
endif
