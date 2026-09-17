## Makefile.am for the native GTK3 version of `libarch.a'.

SUBDIRS = data joystickdrv novte widgets

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_srcdir)/src/arch/shared \
	$(extra_includes) \
	-I$(top_srcdir)/src/arch/shared/hotkeys \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/crtc \
	-I$(top_srcdir)/src/datasette \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/joyport \
	-I$(top_srcdir)/src/printerdrv \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/raster \
	-I$(top_srcdir)/src/rs232drv \
	-I$(top_srcdir)/src/sid \
	-I$(top_srcdir)/src/vdc \
	-I$(top_srcdir)/src/arch/gtk3/widgets \
	-I$(top_srcdir)/src/arch/gtk3/widgets/base \
	-I$(top_srcdir)/src/arch/gtk3/novte \
	-I$(top_srcdir)/src/arch/gtk3/joystickdrv \
	-I$(top_builddir)/src/arch/gtk3/novte \
	-I$(top_srcdir)/src/c64 \
	-I$(top_srcdir)/src/c64dtv \
	-I$(top_srcdir)/src/c128 \
	-I$(top_srcdir)/src/cbm2 \
	-I$(top_srcdir)/src/pet \
	-I$(top_srcdir)/src/plus4 \
	-I$(top_srcdir)/src/scpu64 \
	-I$(top_srcdir)/src/vic20 \
	-I$(top_srcdir)/src/samplerdrv \
	-I$(top_srcdir)/src/c64/cart \
	-I$(top_srcdir)/src/imagecontents \
	-I$(top_srcdir)/src/monitor \
	-I$(top_srcdir)/src/tapeport \
	-I$(top_srcdir)/src/core/rtc \
	-I$(top_srcdir)/src/hvsc

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_OBJCFLAGS = @VICE_OBJCFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libarch.a libtoolarch.a

libarch_a_SOURCES = \
	actions-cartridge.c \
	actions-clipboard.c \
	actions-datasette.c \
	actions-debug.c \
	actions-display.c \
	actions-drive.c \
	actions-help.c \
	actions-hotkeys.c \
	actions-joystick.c \
	actions-machine.c \
	actions-media.c \
	actions-printer.c \
	actions-settings.c \
	actions-snapshot.c \
	actions-speed.c \
	actions-vsid.c \
	archdep.c \
	c128ui.c \
	c64dtvui.c \
	c64scui.c \
	c64ui.c \
	cbm2ui.c \
	cbm5x0ui.c \
	debug_gtk3.c \
	gtk3main.c \
	hotkeys.c \
	kbd.c \
	linenoise.c \
	mousedrv.c \
	petui.c \
	plus4ui.c \
	render_queue.c \
	render_thread.c \
	scpu64ui.c \
	ui.c \
	uiabout.c \
	uicart.c \
	uicmdline.c \
	uicompiletimefeatures.c \
	uidata.c \
	uidatasette.c \
	uidebug.c \
	uidiskattach.c \
	uidiskcreate.c \
	uifliplist.c \
	uihotkeysload.c \
	uihotkeyssave.c \
	uimachinemenu.c \
	uimachinewindow.c \
	uimedia.c \
	uimenu.c \
	uimon-fallback.c \
	uimon.c \
	uisettings.c \
	uisidattach.c \
	uismartattach.c \
	uisnapshot.c \
	uistatusbar.c \
	uitapeattach.c \
	uitapecreate.c \
	uivideo.c \
	uivsidmenu.c \
	uivsidwindow.c \
	vic20ui.c \
	video.c \
	vsidstate.c \
	vsidui.c \
	vsyncarch.c

if USE_GTK3UI
libarch_a_SOURCES += vice_gtk3_locking.c
endif

if WINDOWS_COMPILE
libarch_a_SOURCES += directx_renderer.c directx_renderer_impl.cc
else
libarch_a_SOURCES += opengl_renderer.c
if MACOS_COMPILE
libarch_a_SOURCES += opengl_renderer_macos.m
else
libarch_a_SOURCES += opengl_renderer_unix.c
endif
endif

libtoolarch_a_SOURCES = \
	archdep.c

if USE_GTK3UI
libtoolarch_a_SOURCES += debug_gtk3.c
endif

EXTRA_DIST = \
	Resources/VICE.icns \
	actions-cartridge.h \
	actions-clipboard.h \
	actions-datasette.h \
	actions-debug.h \
	actions-display.h \
	actions-drive.h \
	actions-help.h \
	actions-hotkeys.h \
	actions-joystick.h \
	actions-machine.h \
	actions-media.h \
	actions-printer.h \
	actions-settings.h \
	actions-snapshot.h \
	actions-speed.h \
	actions-vsid.h \
	archdep.h \
	debug_gtk3.h \
	directx_renderer.h \
	directx_renderer_impl.h \
	hotkeys.h \
	kbd.h \
	linenoise.h \
	macOS-ui-runtime.sh \
	make-bindist_osx.sh \
	make-bindist_win32.sh \
	mousedrv.h \
	opengl_renderer.h \
	render_queue.h \
	render_thread.h \
	ui.h \
	uiabout.h \
	uicart.h \
	uicmdline.h \
	uicompiletimefeatures.h \
	uidata.h \
	uidatasette.h \
	uidebug.h \
	uidiskattach.h \
	uidiskcreate.h \
	uifliplist.h \
	uihotkeysload.h \
	uihotkeyssave.h \
	uimachinemenu.h \
	uimachinewindow.h \
	uimedia.h \
	uimenu.h \
	uimon-fallback.h \
	uisettings.h \
	uisidattach.h \
	uismartattach.h \
	uisnapshot.h \
	uistatusbar.h \
	uitapeattach.h \
	uitapecreate.h \
	uitypes.h \
	uivideo.h \
	uivsidmenu.h \
	uivsidwindow.h \
	vice_gtk3_locking.h \
	vice-launcher.sh \
	videoarch.h \
	vsidstate.h \
	winjoy.h
