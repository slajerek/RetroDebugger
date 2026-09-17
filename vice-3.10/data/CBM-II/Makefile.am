cbm2dir = $(VICE_DATADIR)/CBM-II

roms = \
	basic-901242+3-04a.bin \
	basic-901240+1-03.bin \
	basic-901235+6-02.bin \
	chargen-901225-01.bin \
	chargen-901237-01.bin \
	chargen-901232-01.bin \
	kernal-901244-04a.bin \
	kernal-901234-02.bin

romsets = \
	rom500.vrs \
	rom128l.vrs \
	rom128h.vrs \
	rom256l.vrs \
	rom256h.vrs

palettes = \
	c64hq.vpl \
	c64s.vpl \
	ccs64.vpl \
	frodo.vpl \
	godot.vpl \
	pc64.vpl \
	pepto-ntsc-sony.vpl \
	pepto-ntsc.vpl \
	pepto-pal.vpl \
	pepto-palold.vpl \
	community-colors.vpl \
	colodore.vpl \
	ptoing.vpl \
	deekay.vpl \
	cjam.vpl \
	rgb.vpl \
	the64.vpl \
	palette_6569R1_v1r.vpl \
	palette_6569R5_v1r.vpl \
	palette_8565R2_v1r.vpl \
	palette_C64_amber.vpl \
	palette_C64_cyan.vpl \
	palette_C64_green.vpl \
	vice.vpl \
	palette.vpl \
	pixcen.vpl \
	amber.vpl \
	green.vpl \
	white.vpl

gtk3_keymaps = \
	gtk3_pos.vkm \
	gtk3_pos_de.vkm \
	gtk3_sym.vkm \
	gtk3_sym_de.vkm

sdl_keymaps = \
	sdl_sym.vkm \
	sdl_sym_de.vkm \
	sdl_pos.vkm

if USE_SDLUI
cbm2_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps)
EXTRA_DIST = $(cbm2_DATA) $(gtk3_keymaps)
endif

if USE_SDL2UI
cbm2_DATA = $(roms) $(romsets) $(palettes) $(sdl_keymaps)
EXTRA_DIST = $(cbm2_DATA) $(gtk3_keymaps)
endif

if USE_GTK3UI
cbm2_DATA = $(roms) $(romsets) $(palettes) $(gtk3_keymaps)
EXTRA_DIST = $(cbm2_DATA) $(sdl_keymaps)
endif

if USE_HEADLESSUI
cbm2_DATA = $(roms) $(romsets) $(palettes)
EXTRA_DIST = $(cbm2_DATA) $(sdl_keymaps) $(gtk3_keymaps)
endif
