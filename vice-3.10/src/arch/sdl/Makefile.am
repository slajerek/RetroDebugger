# Makefile.am for the SDL version of `libarch.a'.

sdldatadir = $(VICE_DATADIR)/common

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_srcdir)/src/arch/shared \
	-I$(top_srcdir)/src/arch/shared/hotkeys \
	$(extra_includes) \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/printerdrv \
	-I$(top_srcdir)/src/c64 \
	-I$(top_srcdir)/src/c64/cart \
	-I$(top_srcdir)/src/c64dtv \
	-I$(top_srcdir)/src/scpu64 \
	-I$(top_srcdir)/src/c128 \
	-I$(top_srcdir)/src/vic20 \
	-I$(top_srcdir)/src/pet \
	-I$(top_srcdir)/src/cbm2 \
	-I$(top_srcdir)/src/plus4 \
	-I$(top_srcdir)/src/plus4/cart \
	-I$(top_srcdir)/src/raster \
	-I$(top_srcdir)/src/rs232drv \
	-I$(top_srcdir)/src/samplerdrv \
	-I$(top_srcdir)/src/sid \
	-I$(top_srcdir)/src/userport \
	-I$(top_srcdir)/src/vdrive \
	-I$(top_srcdir)/src/vdc \
	-I$(top_srcdir)/src/video \
	-I$(top_srcdir)/src/lib/ \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/platform \
	-I$(top_srcdir)/src/joyport \
	-I$(top_srcdir)/src/gfxoutputdrv \
	-I$(top_srcdir)/src/tapeport \
	-I$(top_srcdir)/src/imagecontents \
	-I$(top_srcdir)/src/datasette \
	-I$(top_srcdir)/src/core/rtc \
	-I$(top_srcdir)/src

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libarch.a libtoolarch.a

if USE_SDL2UI

libarch_a_VERDEP_SOURCES = \
	video_sdl2.c

else

libarch_a_VERDEP_SOURCES = \
	video_sdl1.c

endif

libarch_a_SOURCES = \
	actions-cartridge.c \
	actions-cartridge.h \
	actions-debug.c \
	actions-debug.h \
	actions-display.c \
	actions-display.h \
	actions-drive.c \
	actions-drive.h \
	actions-edit.c \
	actions-edit.h \
	actions-help.c \
	actions-help.h \
	actions-hotkeys.c \
	actions-hotkeys.h \
	actions-joystick.c \
	actions-joystick.h \
	actions-machine.c \
	actions-machine.h \
	actions-media.c \
	actions-media.h \
	actions-settings.c \
	actions-settings.h \
	actions-snapshot.c \
	actions-snapshot.h \
	actions-speed.c \
	actions-speed.h \
	actions-tape.c \
	actions-tape.h \
	actions-vsid.c \
	actions-vsid.h \
	archdep.c \
	archdep.h \
	fullscreen.c \
	fullscreenarch.h \
	hotkeys.c \
	hotkeys.h \
	icon.c \
	icon.h \
	joy.c \
	joy.h \
	kbd.c \
	kbd.h \
	lightpendrv.c \
	lightpendrv.h \
	menu_c128hw.c \
	menu_c128hw.h \
	menu_c64cart.c \
	menu_c64cart.h \
	menu_c64_common_expansions.c \
	menu_c64_common_expansions.h \
	menu_c64_expansions.c \
	menu_c64_expansions.h \
	menu_c64dtvhw.c \
	menu_c64dtvhw.h \
	menu_c64hw.c \
	menu_c64hw.h \
	menu_c64model.c \
	menu_c64model.h \
	menu_cbm2cart.c \
	menu_cbm2cart.h \
	menu_cbm2hw.c \
	menu_cbm2hw.h \
	menu_cbm5x0hw.c \
	menu_cbm5x0hw.h \
	menu_common.c \
	menu_common.h \
	menu_debug.c \
	menu_debug.h \
	menu_drive.c \
	menu_drive.h \
	menu_drive_rom.c \
	menu_drive_rom.h \
	menu_edit.c \
	menu_edit.h \
	menu_ethernet.c \
	menu_ethernet.h \
	menu_ethernetcart.c \
	menu_ethernetcart.h \
	menu_ffmpeg.c \
	menu_ffmpeg.h \
	menu_help.c \
	menu_help.h \
	menu_jam.c \
	menu_jam.h \
	menu_joyport.c \
	menu_joyport.h \
	menu_joystick.c \
	menu_joystick.h \
	menu_log.c \
	menu_log.h \
	menu_media.c \
	menu_media.h \
	menu_midi.c \
	menu_midi.h \
	menu_monitor.c \
	menu_monitor.h \
	menu_mouse.c \
	menu_mouse.h \
	menu_network.c \
	menu_network.h \
	menu_petcart.c \
	menu_petcart.h \
	menu_pethw.c \
	menu_pethw.h \
	menu_plus4cart.c \
	menu_plus4cart.h \
	menu_plus4hw.c \
	menu_plus4hw.h \
	menu_printer.c \
	menu_printer.h \
	menu_ram.c \
	menu_ram.h \
	menu_reset.c \
	menu_reset.h \
	menu_rom.c \
	menu_rom.h \
	menu_rs232.c \
	menu_rs232.h \
	menu_sampler.c \
	menu_sampler.h \
	menu_scpu64hw.c \
	menu_scpu64hw.h \
	menu_screenshot.c \
	menu_screenshot.h \
	menu_settings.c \
	menu_settings.h \
	menu_sid.c \
	menu_sid.h \
	menu_snapshot.c \
	menu_snapshot.h \
	menu_sound.c \
	menu_sound.h \
	menu_speed.c \
	menu_speed.h \
	menu_tape.c \
	menu_tape.h \
	menu_userport.c \
	menu_userport.h \
	menu_vic20cart.c \
	menu_vic20cart.h \
	menu_vic20hw.c \
	menu_vic20hw.h \
	menu_video.c \
	menu_video.h \
	menu_wic64.c \
	menu_wic64.h \
	mousedrv.c \
	mousedrv.h \
	sdlmain.c \
	ui.c \
	ui.h \
	uifonts.c \
	uifonts.h \
	uifilereq.c \
	uiimagefilereq.c \
	uifilereq.h \
	uihotkey.c \
	uihotkey.h \
	uimenu.c \
	uimenu.h \
	uimon.c \
	uimsgbox.c \
	uimsgbox.h \
	uipause.c \
	uipoll.c \
	uipoll.h \
	uistatusbar.c \
	uistatusbar.h \
	$(libarch_a_VERDEP_SOURCES) \
	videoarch.h \
	vkbd.c \
	vkbd.h \
	vsidui.c \
	vsidui_sdl.h \
	vsyncarch.c \
	x128_ui.c \
	x64_ui.c \
	x64dtv_ui.c \
	x64sc_ui.c \
	xcbm2_ui.c \
	xcbm5x0_ui.c \
	xpet_ui.c \
	xplus4_ui.c \
	xscpu64_ui.c \
	xvic_ui.c

libtoolarch_a_SOURCES = \
	archdep.c

EXTRA_DIST = \
	make-bindist_beos.sh \
	make-bindist_osx.sh \
	make-bindist_win32.sh \
	make_bindist_win32_sdl.sh \
	macOS-ui-runtime.sh \
	Resources/VICE.icns \
	vice-launcher.sh \
	vice_sdl.h

if WINDOWS_COMPILE

# Generate icons
all:
	(cd $(top_builddir)/data/common && $(MAKE) win32_res_files)

endif

