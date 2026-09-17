AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_srcdir)/src/arch/shared \
	$(extra_includes) \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/rs232drv \
	-DVICE_DATADIR='"$(datadir)/$(PACKAGE)"' \
	-DVICE_DOCDIR='"$(docdir)"'

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_OBJCFLAGS = @VICE_OBJCFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libhotkeys.a

libhotkeys_a_SOURCES = \
	boolexpr.c \
	ifstack.c \
	parser.c \
	symtab.c \
	textfilereader.c \
	vhkdebug.c \
	vhkkeysyms.c \
	uihotkeys.c

EXTRA_DIST = \
	boolexpr.h \
	hotkeystypes.h \
	ifstack.h \
	parser.h \
	symtab.h \
	textfilereader.h \
	vhkdebug.h \
	vhkkeysyms.h \
	uihotkeys.h

BUILT_SOURCES = keysymtable.h

keysymtable.h: $(top_srcdir)/src/arch/shared/hotkeys/vhkkeysyms.h $(top_srcdir)/src/buildtools/genkeysymtable.awk $(top_srcdir)/src/buildtools/genkeysymtable.sh
	@echo "Generating $(top_builddir)/src/arch/shared/hotkeys/keysymtable.h"
	@$(SHELL) $(top_srcdir)/src/buildtools/genkeysymtable.sh $(top_srcdir)/src/arch/shared/hotkeys/vhkkeysyms.h $(top_builddir)/src/arch/shared/hotkeys/keysymtable.h
