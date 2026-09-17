commondir = $(VICE_DATADIR)/common

GRESOURCE_TARGET = $(builddir)/vice.gresource
GRESOURCE_XML = $(srcdir)/vice.gresource.xml

icons_16x16 = vice-vsid_16.png \
	      vice-x128_16.png \
	      vice-x64_16.png \
	      vice-x64dtv_16.png \
	      vice-xcbm2_16.png \
	      vice-xpet_16.png \
	      vice-xplus4_16.png \
	      vice-xscpu64_16.png \
	      vice-xvic_16.png

icons_24x24 = vice-vsid_24.png \
	      vice-x128_24.png \
	      vice-x64_24.png \
	      vice-x64dtv_24.png \
	      vice-xcbm2_24.png \
	      vice-xpet_24.png \
	      vice-xplus4_24.png \
	      vice-xscpu64_24.png \
	      vice-xvic_24.png

icons_32x32 = vice-vsid_32.png \
	      vice-x128_32.png \
	      vice-x64_32.png \
	      vice-x64dtv_32.png \
	      vice-xcbm2_32.png \
	      vice-xpet_32.png \
	      vice-xplus4_32.png \
	      vice-xscpu64_32.png \
	      vice-xvic_32.png

icons_48x48 = vice-vsid_48.png \
	      vice-x128_48.png \
	      vice-x64_48.png \
	      vice-x64dtv_48.png \
	      vice-xcbm2_48.png \
	      vice-xpet_48.png \
	      vice-xplus4_48.png \
	      vice-xscpu64_48.png \
	      vice-xvic_48.png

icons_64x64 = vice-vsid_64.png \
	      vice-x128_64.png \
	      vice-x64_64.png \
	      vice-x64dtv_64.png \
	      vice-xcbm2_64.png \
	      vice-xpet_64.png \
	      vice-xplus4_64.png \
	      vice-xscpu64_64.png \
	      vice-xvic_64.png

icons_256x256 = vice-vsid_256.png \
	        vice-x128_256.png \
	        vice-x64_256.png \
	        vice-x64dtv_256.png \
	        vice-xcbm2_256.png \
	        vice-xpet_256.png \
	        vice-xplus4_256.png \
	        vice-xscpu64_256.png \
	        vice-xvic_256.png

icons_scalable = vice-vsid_1024.svg \
	         vice-x128_1024.svg \
	         vice-x64_1024.svg \
	         vice-x64dtv_1024.svg \
	         vice-xcbm2_1024.svg \
	         vice-xpet_1024.svg \
	         vice-xplus4_1024.svg \
	         vice-xscpu64_1024.svg \
	         vice-xvic_1024.svg

icon_files = $(icons_16x16) \
	     $(icons_24x24) \
	     $(icons_32x32) \
	     $(icons_48x48) \
	     $(icons_64x64) \
	     $(icons_256x256) \
	     $(icons_scalable)

icons_win32_vsid = vice-vsid_16.png \
		   vice-vsid_24.png \
		   vice-vsid_32.png \
		   vice-vsid_48.png \
		   vice-vsid_64.png \
		   vice-vsid_256.png

icons_win32_x128 = vice-x128_16.png \
		   vice-x128_24.png \
		   vice-x128_32.png \
		   vice-x128_48.png \
		   vice-x128_64.png \
		   vice-x128_256.png

icons_win32_x64 = vice-x64_16.png \
		  vice-x64_24.png \
		  vice-x64_32.png \
		  vice-x64_48.png \
		  vice-x64_64.png \
		  vice-x64_256.png

icons_win32_x64dtv = vice-x64dtv_16.png \
		     vice-x64dtv_24.png \
		     vice-x64dtv_32.png \
		     vice-x64dtv_48.png \
		     vice-x64dtv_64.png \
		     vice-x64dtv_256.png

icons_win32_x64sc = vice-x64sc_16.png \
		    vice-x64sc_24.png \
		    vice-x64sc_32.png \
		    vice-x64sc_48.png \
		    vice-x64sc_64.png \
		    vice-x64sc_256.png

icons_win32_xcbm2 = vice-xcbm2_16.png \
		    vice-xcbm2_24.png \
		    vice-xcbm2_32.png \
		    vice-xcbm2_48.png \
		    vice-xcbm2_64.png \
		    vice-xcbm2_256.png

icons_win32_xpet = vice-xpet_16.png \
		   vice-xpet_24.png \
		   vice-xpet_32.png \
		   vice-xpet_48.png \
		   vice-xpet_64.png \
		   vice-xpet_256.png

icons_win32_xplus4 = vice-xplus4_16.png \
		     vice-xplus4_24.png \
		     vice-xplus4_32.png \
		     vice-xplus4_48.png \
		     vice-xplus4_64.png \
		     vice-xplus4_256.png

icons_win32_xscpu64 = vice-xscpu64_16.png \
		      vice-xscpu64_24.png \
		      vice-xscpu64_32.png \
		      vice-xscpu64_48.png \
		      vice-xscpu64_64.png \
		      vice-xscpu64_256.png

icons_win32_xvic = vice-xvic_16.png \
		   vice-xvic_24.png \
		   vice-xvic_32.png \
		   vice-xvic_48.png \
		   vice-xvic_64.png \
		   vice-xvic_256.png

logo_files = CBM_Logo.svg \
	     vice-logo-black.svg

font_files = C64_Pro_Mono-STYLE.ttf \
             PetMe1282Y.ttf \
             PetMe128.ttf \
             PetMe2X.ttf \
             PetMe2Y.ttf \
             PetMe642Y.ttf \
             PetMe64.ttf \
             PetMe.ttf

desktop_files = x64.desktop \
		x64sc.desktop \
		x64dtv.desktop \
		xscpu64.desktop \
		x128.desktop \
		xvic.desktop \
		xplus4.desktop \
		xpet.desktop \
		xcbm2.desktop \
		xcbm5x0.desktop \
		vsid.desktop


if USE_SDLUI
if WINDOWS_COMPILE
# SDL 1.2 says to use 32x32 icons on Windows
common_DATA = $(icons_32x32)
else
common_DATA = $(icons_256x256)
endif
endif

if USE_SDL2UI
# 256x256 icons are fine for SDL 2.x for all OSes
common_DATA = $(icons_256x256)
endif

if USE_GTK3UI
if INSTALL_DESKTOP_FILES
# We install icons via xdg-icon-resource, so don't install any icons directly
common_DATA = $(logo_files) \
	      $(font_files)
else
common_DATA = $(icons_32x32) \
	      $(icons_48x48) \
	      $(icons_64x64) \
	      $(icons_256x256) \
	      $(icons_scalable) \
	      $(logo_files) \
	      $(font_files)
endif
endif

# Windows .res and .ico files (for Gtk3 and SDL)
if WINDOWS_COMPILE
if !USE_HEADLESSUI

if SUPPORT_X64
X64_RES=x64.res
else
X64_RES=
endif

win32_res_files: vsid.res x128.res $(X64_RES) x64dtv.res x64sc.res xcbm2.res \
		 xcbm5x0.res xpet.res xplus4.res xscpu64.res xvic.res

# Create Windows .ico files
%.ico:
	$(ICOTOOL) -c -o $@ $^

# Create Windows .res files from .ico and .rc files
%.res: $(top_srcdir)/data/common/%.rc %.ico
	$(WINDRES) $< -O coff -o $@

vsid.ico: $(icons_win32_vsid)
x128.ico: $(icons_win32_x128)
if SUPPORT_X64
x64.ico: $(icons_win32_x64)
endif
x64dtv.ico: $(icons_win32_x64dtv)
x64sc.ico: $(icons_win32_x64)
xcbm2.ico: $(icons_win32_xcbm2)
xcbm5x0.ico: $(icons_win32_xcbm2)
xpet.ico: $(icons_win32_xpet)
xplus4.ico: $(icons_win32_xplus4)
xscpu64.ico: $(icons_win32_xscpu64)
xvic.ico: $(icons_win32_xvic)

# if !USE_HEADLESSUI
endif
# if WINDOWS_COMPILE
endif


if USE_GTK3UI
# Generate gresource binary file, required by all Gtk3 emus
$(GRESOURCE_TARGET): $(GRESOURCE_XML)
	glib-compile-resources --sourcedir $(top_srcdir) $< --target $@

common_DATA += $(GRESOURCE_TARGET)

# NOTE: It is important to consider the following cases:
# a) regular install (system wide, by root)
# b) regular install (by user)
# c) staged install (by a package maintainer)
#
# A staged install can be done (by a user) by giving DESTDIR on the commandline:
#
# make install DESTDIR=/home/paul/vice-staged
#
# and should result in the same directory structure a regular system wide install
# would use, but with DESTDIR prepended.
#
# For a regular install, i tricky case to consider is when the desktop environment
# uses a different install prefix than it was used for VICE. a common case would
# be when the system uses /usr and VICE uses /usr/local - but it can be even more
# exotic, for example Gentoo installs emulators in /usr/games.
#
# That means we can NOT use PREFIX in any of the pathes below, we should rather
# let the respective utilities figure out the correct (default) path and only in
# the case of a staged install use whatever mechanism is available to prepend
# DESTDIR.

# FIXME: we need a mechanism to figure out the correct PREFIX used by the desktop
# installation, to use with the cases below that do not offer a good mechanism
# to do this for the staged install case. (/usr should work for most practical
# purposes anyway)
DESKTOP_PREFIX=/usr

if INSTALL_DESKTOP_FILES
.PHONY: install-icons install-desktop-files uinstall-icons

# Install icon files in their proper place. The default context is 'apps' and
# the default mode is 'system' when running as root and 'user' when not running
# as root, both defaults are what we want.
install-icons:
	if test -n "$(DESTDIR)"; then \
		export XDG_DATA_HOME=$(DESTDIR)$(DESKTOP_PREFIX)/share ;\
	fi ;\
	xdg-icon-resource install --size 32 $(top_srcdir)/data/common/vice-x64_32.png vice-x64 ;\
	xdg-icon-resource install --size 48 $(top_srcdir)/data/common/vice-x64_48.png vice-x64 ;\
	xdg-icon-resource install --size 64 $(top_srcdir)/data/common/vice-x64_64.png vice-x64 ;\
	xdg-icon-resource install --size 256 $(top_srcdir)/data/common/vice-x64_256.png vice-x64 ;\
	xdg-icon-resource install --size 32 $(top_srcdir)/data/common/vice-x64dtv_32.png vice-x64dtv ;\
	xdg-icon-resource install --size 48 $(top_srcdir)/data/common/vice-x64dtv_48.png vice-x64dtv ;\
	xdg-icon-resource install --size 64 $(top_srcdir)/data/common/vice-x64dtv_64.png vice-x64dtv ;\
	xdg-icon-resource install --size 256 $(top_srcdir)/data/common/vice-x64dtv_256.png vice-x64dtv ;\
	xdg-icon-resource install --size 32 $(top_srcdir)/data/common/vice-xscpu64_32.png vice-xscpu64 ;\
	xdg-icon-resource install --size 48 $(top_srcdir)/data/common/vice-xscpu64_48.png vice-xscpu64 ;\
	xdg-icon-resource install --size 64 $(top_srcdir)/data/common/vice-xscpu64_64.png vice-xscpu64 ;\
	xdg-icon-resource install --size 256 $(top_srcdir)/data/common/vice-xscpu64_256.png vice-xscpu64 ;\
	xdg-icon-resource install --size 32 $(top_srcdir)/data/common/vice-x128_32.png vice-x128 ;\
	xdg-icon-resource install --size 48 $(top_srcdir)/data/common/vice-x128_48.png vice-x128 ;\
	xdg-icon-resource install --size 64 $(top_srcdir)/data/common/vice-x128_64.png vice-x128 ;\
	xdg-icon-resource install --size 256 $(top_srcdir)/data/common/vice-x128_256.png vice-x128 ;\
	xdg-icon-resource install --size 32 $(top_srcdir)/data/common/vice-xplus4_32.png vice-xplus4 ;\
	xdg-icon-resource install --size 48 $(top_srcdir)/data/common/vice-xplus4_48.png vice-xplus4 ;\
	xdg-icon-resource install --size 64 $(top_srcdir)/data/common/vice-xplus4_64.png vice-xplus4 ;\
	xdg-icon-resource install --size 256 $(top_srcdir)/data/common/vice-xplus4_256.png vice-xplus4 ;\
	xdg-icon-resource install --size 32 $(top_srcdir)/data/common/vice-xvic_32.png vice-xvic ;\
	xdg-icon-resource install --size 48 $(top_srcdir)/data/common/vice-xvic_48.png vice-xvic ;\
	xdg-icon-resource install --size 64 $(top_srcdir)/data/common/vice-xvic_64.png vice-xvic ;\
	xdg-icon-resource install --size 256 $(top_srcdir)/data/common/vice-xvic_256.png vice-xvic ;\
	xdg-icon-resource install --size 32 $(top_srcdir)/data/common/vice-xpet_32.png vice-xpet ;\
	xdg-icon-resource install --size 48 $(top_srcdir)/data/common/vice-xpet_48.png vice-xpet ;\
	xdg-icon-resource install --size 64 $(top_srcdir)/data/common/vice-xpet_64.png vice-xpet ;\
	xdg-icon-resource install --size 256 $(top_srcdir)/data/common/vice-xpet_256.png vice-xpet ;\
	xdg-icon-resource install --size 32 $(top_srcdir)/data/common/vice-xcbm2_32.png vice-xcbm2 ;\
	xdg-icon-resource install --size 48 $(top_srcdir)/data/common/vice-xcbm2_48.png vice-xcbm2 ;\
	xdg-icon-resource install --size 64 $(top_srcdir)/data/common/vice-xcbm2_64.png vice-xcbm2 ;\
	xdg-icon-resource install --size 256 $(top_srcdir)/data/common/vice-xcbm2_256.png vice-xcbm2 ;\
	xdg-icon-resource install --size 32 $(top_srcdir)/data/common/vice-vsid_32.png vice-vsid ;\
	xdg-icon-resource install --size 48 $(top_srcdir)/data/common/vice-vsid_48.png vice-vsid ;\
	xdg-icon-resource install --size 64 $(top_srcdir)/data/common/vice-vsid_64.png vice-vsid ;\
	xdg-icon-resource install --size 256 $(top_srcdir)/data/common/vice-vsid_256.png vice-vsid

uninstall-icons:
	if test -n "$(DESTDIR)"; then \
		export XDG_DATA_HOME=$(DESTDIR)$(DESKTOP_PREFIX)/share ;\
	fi ;\
	xdg-icon-resource uninstall --size 32 vice-x64 ;\
	xdg-icon-resource uninstall --size 48 vice-x64 ;\
	xdg-icon-resource uninstall --size 64 vice-x64 ;\
	xdg-icon-resource uninstall --size 256 vice-x64 ;\
	xdg-icon-resource uninstall --size 32 vice-x64dtv ;\
	xdg-icon-resource uninstall --size 48 vice-x64dtv ;\
	xdg-icon-resource uninstall --size 64 vice-x64dtv ;\
	xdg-icon-resource uninstall --size 256 vice-x64dtv ;\
	xdg-icon-resource uninstall --size 32 vice-xscpu64 ;\
	xdg-icon-resource uninstall --size 48 vice-xscpu64 ;\
	xdg-icon-resource uninstall --size 64 vice-xscpu64 ;\
	xdg-icon-resource uninstall --size 256 vice-xscpu64 ;\
	xdg-icon-resource uninstall --size 32 vice-x128 ;\
	xdg-icon-resource uninstall --size 48 vice-x128 ;\
	xdg-icon-resource uninstall --size 64 vice-x128 ;\
	xdg-icon-resource uninstall --size 256 vice-x128 ;\
	xdg-icon-resource uninstall --size 32 vice-xplus4 ;\
	xdg-icon-resource uninstall --size 48 vice-xplus4 ;\
	xdg-icon-resource uninstall --size 64 vice-xplus4 ;\
	xdg-icon-resource uninstall --size 256 vice-xplus4 ;\
	xdg-icon-resource uninstall --size 32 vice-xvic ;\
	xdg-icon-resource uninstall --size 48 vice-xvic ;\
	xdg-icon-resource uninstall --size 64 vice-xvic ;\
	xdg-icon-resource uninstall --size 256 vice-xvic ;\
	xdg-icon-resource uninstall --size 32 vice-xpet ;\
	xdg-icon-resource uninstall --size 48 vice-xpet ;\
	xdg-icon-resource uninstall --size 64 vice-xpet ;\
	xdg-icon-resource uninstall --size 256 vice-xpet ;\
	xdg-icon-resource uninstall --size 32 vice-xcbm2 ;\
	xdg-icon-resource uninstall --size 48 vice-xcbm2 ;\
	xdg-icon-resource uninstall --size 64 vice-xcbm2 ;\
	xdg-icon-resource uninstall --size 256 vice-xcbm2 ;\
	xdg-icon-resource uninstall --size 32 vice-vsid ;\
	xdg-icon-resource uninstall --size 48 vice-vsid ;\
	xdg-icon-resource uninstall --size 64 vice-vsid ;\
	xdg-icon-resource uninstall --size 256 vice-vsid

# We can use the program prefix/suffix set via configure's --program-prefix= /
# --program-suffix= switches via:
# --set-key=Exec --set-value=@PROGRAM_PREFIX@x64sc@PROGRAM_SUFFIX@, but icon
# lookup appears to depend on the .desktop file's name matching (the part
# after the optional vendor prefix) the executable name.
#
# So for example using --program-suffix=-gtk3 won't work since "x64sc-gtk3.desktop"
# means the vendor is "x64sc" and the executable "gtk3". For now we'll just make
# this work for the canonical emulator names, alternate names will be a TODO.

install-desktop-files:
if SUPPORT_X64
	if test -n "$(DESTDIR)"; then \
		export RPM_BUILD_ROOT=$(DESTDIR) ;\
	fi ;\
	desktop-file-install $(top_srcdir)/data/common/x64.desktop
endif
	if test -n "$(DESTDIR)"; then \
		export RPM_BUILD_ROOT=$(DESTDIR) ;\
	fi ;\
	desktop-file-install $(top_srcdir)/data/common/x64sc.desktop ;\
	desktop-file-install $(top_srcdir)/data/common/x64dtv.desktop ;\
	desktop-file-install $(top_srcdir)/data/common/xscpu64.desktop ;\
	desktop-file-install $(top_srcdir)/data/common/x128.desktop ;\
	desktop-file-install $(top_srcdir)/data/common/xplus4.desktop ;\
	desktop-file-install $(top_srcdir)/data/common/xvic.desktop ;\
	desktop-file-install $(top_srcdir)/data/common/xpet.desktop ;\
	desktop-file-install $(top_srcdir)/data/common/xcbm2.desktop ;\
	desktop-file-install $(top_srcdir)/data/common/xcbm5x0.desktop ;\
	desktop-file-install $(top_srcdir)/data/common/vsid.desktop

# No `desktop-file-uninstall` exists, nor is there an uninstall option for
# `desktop-file-install` or `desktop-file-uninstall`, so a simple `rm` will
# have to do.
uninstall-desktop-files:
	rm -f $(DESTDIR)$(DESKTOP_PREFIX)/share/applications/x64{,dtv,sc}.desktop
	rm -f $(DESTDIR)$(DESKTOP_PREFIX)/share/applications/xscpu64.desktop
	rm -f $(DESTDIR)$(DESKTOP_PREFIX)/share/applications/x128.desktop
	rm -f $(DESTDIR)$(DESKTOP_PREFIX)/share/applications/xplus4.desktop
	rm -f $(DESTDIR)$(DESKTOP_PREFIX)/share/applications/xpet.desktop
	rm -f $(DESTDIR)$(DESKTOP_PREFIX)/share/applications/xcbm{2,5x0}.desktop
	rm -f $(DESTDIR)$(DESKTOP_PREFIX)/share/applications/xvic.desktop
	rm -f $(DESTDIR)$(DESKTOP_PREFIX)/share/applications/vsid.desktop

install-data-hook: install-icons install-desktop-files
	-update-desktop-database

uninstall-hook: uninstall-icons uninstall-desktop-files
	-update-desktop-database

# if INSTALL_DESKTOP_FILES
endif

# if USE_GTK3UI
endif

# Everything goes into a source dist
EXTRA_DIST = $(icon_files) \
	     $(logo_files) \
	     $(font_files) \
	     $(desktop_files) \
             PetMe-FreeLicense.txt \
	     x64.rc \
	     x64sc.rc \
	     x64dtv.rc \
	     xscpu64.rc \
	     x128.rc \
	     xvic.rc \
	     xplus4.rc \
	     xpet.rc \
	     xcbm2.rc \
	     xcbm5x0.rc \
	     vsid.rc \
	     vice.gresource.xml


if USE_GTK3UI
CLEANFILES = $(GRESOURCE_TARGET)
else
CLEANFILES =
endif

if WINDOWS_COMPILE
# The *.ico and *.res files are currently generated in
# src/arch/gtk3/data/win32/Makefile.am, but the plan is to move that to here
# to also be able to generate the files for linking with SDL binaries on Win32
CLEANFILES += \
	x64.ico \
	x64sc.ico \
	x64dtv.ico \
	xscpu64.ico \
	x128.ico \
	xvic.ico \
	xplus4.ico \
	xpet.ico \
	xcbm2.ico \
	xcbm5x0.ico \
	vsid.ico \
	x64.res \
	x64sc.res \
	x64dtv.res \
	xscpu64.res \
	x128.res \
	xvic.res \
	xplus4.res \
	xpet.res \
	xcbm2.res \
	xcbm5x0.res \
	vsid.res
endif
