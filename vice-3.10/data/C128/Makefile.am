c128dir = $(VICE_DATADIR)/C128

roms = \
	basic64-901226-01.bin \
	kernal64-901227-03.bin \
	kernal64-325179-01.bin \
	kernal64-325182-01.bin \
	basichi-318019-04.bin \
	basiclo-318018-04.bin \
	chargen-315079-01.bin \
	chargen-390059-01.bin \
	chargen-325181-01.bin \
	chargen-325173-01D.bin \
	chargen-325078-02.bin \
	chargen-325167-02.bin \
	kernal-318020-05.bin \
	kernal-315078-03.bin \
	kernalfi \
	kernalfr \
	kernalit \
	kernalno \
	kernal-318034-01.bin \
	kernal-325172-01.bin

romsets = \
	default.vrs

palettes = \
	c64hq.vpl \
	c64s.vpl \
	ccs64.vpl \
	frodo.vpl \
	godot.vpl \
	pc64.vpl \
	vdc_deft.vpl \
	vdc_comp.vpl \
	vdc_scart.vpl \
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
	gtk3_sym_fr.vkm \
	gtk3_sym_de.vkm \
	gtk3_sym_se.vkm \
	gtk3_pos.vkm \
	gtk3_pos_de.vkm \
	gtk3_pos_se.vkm \
	gtk3_keyrahv3.vkm \
	gtk3_keyrahv3_de.vkm

sdl_keymaps = \
	sdl_sym.vkm \
	sdl_sym_de.vkm \
	sdl_sym_fr.vkm \
	sdl_pos.vkm \
	sdl_pos_de.vkm \
	sdl_v128.vkm

if USE_SDLUI
c128_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps)
EXTRA_DIST = $(c128_DATA) $(gtk3_keymaps)
endif

if USE_SDL2UI
c128_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps)
EXTRA_DIST = $(c128_DATA) $(gtk3_keymaps)
endif

if USE_GTK3UI
c128_DATA = $(roms) $(romsets) $(palettes) $(gtk3_keymaps)
EXTRA_DIST = $(c128_DATA) $(sdl_keymaps)
endif

if USE_HEADLESSUI
c128_DATA = $(roms) $(romsets) $(palettes)
EXTRA_DIST = $(c128_DATA) $(sdl_keymaps) $(gtk3_keymaps)
endif
