ACLOCAL_AMFLAGS=-Im4

# Install HTML docs?
if ENABLE_HTML_DOCS
HTML_DOCS=yes
else
HTML_DOCS=no
endif


if VICE_QUIET
.SILENT:
endif

# Process this file with automake to produce Makefile.in

SUBDIRS = src build data doc

# COMMON_EXTRA_DIST is what used to be the EXTRA_DIST, when adding dist
# files, please add them to the COMMON_EXTRA_DIST list.

COMMON_EXTRA_DIST = \
	autogen.sh \
	config.rpath \
	configure.ac \
	cmake-bootstrap.sh \
	COPYING \
	NEWS

EXTRA_DIST = $(COMMON_EXTRA_DIST)

if SUPPORT_X64
  X64_INCLUDED = yes
  X64_ENABLE = "--enable-x64"
else
  X64_INCLUDED = no
  X64_ENABLE = ""
endif

.PHONY: stylecheck
stylecheck:
	@echo "Doing some style checks..."
	@cd $(top_srcdir) && $(SHELL) ./build/github-actions/check-lastline.sh
	@cd $(top_srcdir) && $(SHELL) ./build/github-actions/check-spaces.sh
	@cd $(top_srcdir) && $(SHELL) ./build/github-actions/check-tabs.sh

.PHONY: vsid x64 x64sc x128 x64dtv xvic xpet xplus4 xcbm2 xcbm5x0 xscpu64 c1541 petcat cartconv

vsid:
	(cd src; $(MAKE) vsid-all)

if SUPPORT_X64
x64:
	(cd src; $(MAKE) x64-all)
endif

x64sc:
	(cd src; $(MAKE) x64sc-all)

x128:
	(cd src; $(MAKE) x128-all)

x64dtv:
	(cd src; $(MAKE) x64dtv-all)

xscpu64:
	(cd src; $(MAKE) xscpu64-all)

xvic:
	(cd src; $(MAKE) xvic-all)

xpet:
	(cd src; $(MAKE) xpet-all)

xplus4:
	(cd src; $(MAKE) xplus4-all)

xcbm2:
	(cd src; $(MAKE) xcbm2-all)

xcbm5x0:
	(cd src; $(MAKE) xcbm5x0-all)

c1541:
	(cd src; $(MAKE) c1541-all)

petcat:
	(cd src/tools/petcat; $(MAKE))

cartconv:
	(cd src/tools/cartconv; $(MAKE))

install: installvice



if MAKE_BINDIST

if BEOS_COMPILE

if USE_SDLUI
bindist:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_beos.sh $(STRIP) $(VICE_VERSION) $(host_cpu) $(host_os) @ENABLE_ARCH@ nozip $(top_srcdir) 1

bindistzip:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_beos.sh $(STRIP) $(VICE_VERSION) $(host_cpu) $(host_os) @ENABLE_ARCH@ zip $(XSCPU64_INCLUDED) $(top_srcdir) 1
endif USE_SDLUI

if USE_SDL2UI
bindist:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_beos.sh $(STRIP) $(VICE_VERSION) $(host_cpu) $(host_os) @ENABLE_ARCH@ nozip $(top_srcdir) 2

bindistzip:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_beos.sh $(STRIP) $(VICE_VERSION) $(host_cpu) $(host_os) @ENABLE_ARCH@ zip $(XSCPU64_INCLUDED) $(top_srcdir) 2
endif USE_SDL2UI
endif BEOS_COMPILE


if MACOS_COMPILE

if USE_GTK3UI
bindist:
	/bin/bash @ARCH_SRC_DIR@/../gtk3/make-bindist_osx.sh $(top_srcdir) $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ nozip

bindistzip:
	/bin/bash @ARCH_SRC_DIR@/../gtk3/make-bindist_osx.sh $(top_srcdir) $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ zip
endif USE_GTK3UI

if USE_SDLUI
bindist:
	FW_DIR=@FW_DIR@ /bin/bash @ARCH_SRC_DIR@/../sdl/make-bindist_osx.sh $(top_srcdir) $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ nozip 1

bindistzip:
	FW_DIR=@FW_DIR@ /bin/bash @ARCH_SRC_DIR@/../sdl/make-bindist_osx.sh $(top_srcdir) $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ zip 1
endif USE_SDLUI

if USE_SDL2UI
bindist:
	FW_DIR=@FW_DIR@ /bin/bash @ARCH_SRC_DIR@/../sdl/make-bindist_osx.sh $(top_srcdir) $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ nozip 2

bindistzip:
	FW_DIR=@FW_DIR@ /bin/bash @ARCH_SRC_DIR@/../sdl/make-bindist_osx.sh $(top_srcdir) $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ zip 2
endif USE_SDL2UI

endif MACOS_COMPILE


if WINDOWS_COMPILE
if USE_GTK3UI
bindist:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_win32.sh $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ nozip @UNZIPBIN@ $(X64_INCLUDED) $(top_srcdir) $(host_cpu) $(abs_top_builddir) $(objdump) $(CC) $(HTMLDOCS) $(SVN_REVISION_OVERRIDE)

bindistzip:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_win32.sh $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ zip @UNZIPBIN@ $(X64_INCLUDED) $(top_srcdir) $(host_cpu) $(abs_top_builddir) $(objdump) $(CC) $(HTML_DOCS) $(SVN_REVISION_OVERRIDE)

bindist7zip:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_win32.sh $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ 7zip @UNZIPBIN@ $(X64_INCLUDED) $(top_srcdir) $(host_cpu) $(abs_top_builddir) $(objdump) $(CC) $(HTML_DOCS) $(SVN_REVISION_OVERRIDE)
endif USE_GTK3UI

if USE_SDLUI
bindist:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_win32.sh $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ nozip @UNZIPBIN@ $(X64_INCLUDED) $(top_srcdir) $(top_builddir) $(host_cpu) 1 "$(SDLCONFIG)" $(CROSS) $(objdump) $(CC) $(HTML_DOCS) $(SVN_REVISION_OVERRIDE)

bindistzip:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_win32.sh $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ zip @UNZIPBIN@ $(X64_INCLUDED) $(top_srcdir) $(top_builddir) $(host_cpu) 1 "$(SDLCONFIG)" $(CROSS) $(objdump) $(CC) $(HTML_DOCS) $(SVN_REVISION_OVERRIDE)

bindist7zip:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_win32.sh $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ 7zip @UNZIPBIN@ $(X64_INCLUDED) $(top_srcdir) $(top_builddir) $(host_cpu) 1 "$(SDLCONFIG)" $(CROSS) $(objdump) $(CC) $(HTML_DOCS) $(SVN_REVISION_OVERRIDE)
endif USE_SDLUI

if USE_SDL2UI
bindist:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_win32.sh $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ nozip @UNZIPBIN@ $(X64_INCLUDED) $(top_srcdir) $(top_builddir) $(host_cpu) 2 "$(SDLCONFIG)" $(CROSS) $(objdump) $(CC) $(HTML_DOCS) $(SVN_REVISION_OVERRIDE)

bindistzip:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_win32.sh $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ zip @UNZIPBIN@ $(X64_INCLUDED) $(top_srcdir) $(top_builddir) $(host_cpu) 2 "$(SDLCONFIG)" $(CROSS) $(objdump) $(CC) $(HTML_DOCS) $(SVN_REVISION_OVERRIDE)

bindist7zip:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_win32.sh $(STRIP) $(VICE_VERSION) @ENABLE_ARCH@ 7zip @UNZIPBIN@ $(X64_INCLUDED) $(top_srcdir) $(top_builddir) $(host_cpu) 2 "$(SDLCONFIG)" $(CROSS) $(objdump) $(CC) $(HTML_DOCS) $(SVN_REVISION_OVERRIDE)
endif USE_SDL2UI

if USE_HEADLESSUI
bindist:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_win32.sh \
		--verbose \
		--compiler-exe $(CC) \
		--objdump-exe $(objdump) \
		--strip-exe $(STRIP) \
		--vice-version $(VICE_VERSION) \
		--enable-arch @ENABLE_ARCH@ \
		$(X64_ENABLE) \
		--src-dir $(top_srcdir) \
		--build-dir $(top_builddir) \
		--cpu-type $(host_cpu)

bindistzip:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_win32.sh \
		--verbose \
		--compiler-exe $(CC) \
		--objdump-exe $(objdump) \
		--strip-exe $(STRIP) \
		--vice-version $(VICE_VERSION) \
		--enable-arch @ENABLE_ARCH@ \
		$(X64_ENABLE) \
		--src-dir $(top_srcdir) \
		--build-dir $(top_builddir) \
		--cpu-type $(host_cpu) \
		--zip-type zip

bindist7zip:
	$(SHELL) @ARCH_SRC_DIR@/make-bindist_win32.sh \
		--verbose \
		--compiler-exe $(CC) \
		--objdump-exe $(objdump) \
		--strip-exe $(STRIP) \
		--vice-version $(VICE_VERSION) \
		--enable-arch @ENABLE_ARCH@ \
		$(X64_ENABLE) \
		--src-dir $(top_srcdir) \
		--build-dir $(top_builddir) \
		--cpu-type $(host_cpu) \
		--zip-type 7zip

endif USE_HEADLESSUI


endif WINDOWS_COMPILE

else !MAKE_BINDIST
bindist:
	@echo this platform doesn\'t use \"make bindist\", use \"make install\" instead
endif !MAKE_BINDIST

EXTRA_INSTALLS =
EXTRA_CLEANS =


if MAKE_INSTALL
installvice: install-recursive $(EXTRA_INSTALLS)
else
installvice:
	@echo this platform doesn\'t use \"make install\", use \"make bindist\" instead
endif

# Manually define the dist targets so the correct tar usage will be used.
dist-gzip:
	$(DOS2UNIX) `find $(top_srcdir) -name "*.sh"`
	$(DOS2UNIX) $(top_srcdir)/doc/vice.texi
	$(MAKE) distdir
	$(AMTAR) chf - $(distdir) | gzip --best -c >$(distdir).tar.gz
	$(am__remove_distdir)

dist dist-all:
	$(DOS2UNIX) `find $(top_srcdir) -name "*.sh"`
	$(DOS2UNIX) $(top_srcdir)/doc/vice.texi
	$(MAKE) distdir
	$(AMTAR) chf - $(distdir) | gzip --best -c >$(distdir).tar.gz
	$(am__remove_distdir)

clean: $(EXTRA_CLEANS) clean-recursive
