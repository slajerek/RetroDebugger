## Makefile.am for the native GTK3 version of `libwidgetsbase.a'.


AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_srcdir)/src/arch/shared \
	-I$(top_srcdir)/src/arch/shared/hotkeys \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/joyport \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/platform \
	-I$(top_srcdir)/src/raster \
	-I$(top_srcdir)/src/rs232drv \
	-I$(top_srcdir)/src/sid \
	-I$(top_srcdir)/src/c64 \
	-I$(top_srcdir)/src/c64dtv \
	-I$(top_srcdir)/src/c128 \
	-I$(top_srcdir)/src/cbm2 \
	-I$(top_srcdir)/src/pet \
	-I$(top_srcdir)/src/plus4 \
	-I$(top_srcdir)/src/scpu64 \
	-I$(top_srcdir)/src/vic20 \
	-I$(top_srcdir)/src/userport \
	-I$(top_srcdir)/src/c64/cart

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libbasewidgets.a

libbasewidgets_a_SOURCES = \
	basedialogs.c \
	carthelpers.c \
	cartridgewidgets.c \
	clockportdevicewidget.c \
	csshelpers.c \
	filechooserhelpers.c \
	lastdir.c \
	openfiledialog.c \
	popupmenu.c \
	ramsizeradiogroup.c \
	resourcebrowser.c \
	resourcecheckbutton.c \
	resourcecombobox.c \
	resourceentry.c \
	resourcefilechooser.c \
	resourcehelpers.c \
	resourcenumericstring.c \
	resourceradiogroup.c \
	resourcescale.c \
	resourcespinbutton.c \
	resourceswitch.c \
	resourcewidgetmediator.c \
	savefiledialog.c \
	selectdirectorydialog.c \
	unicodehelpers.c \
	uiactionbutton.c \
	widgethelpers.c

EXTRA_DIST = \
	basedialogs.h \
	basewidget_types.h \
	basewidgets.h \
	carthelpers.h \
	cartridgewidgets.h \
	clockportdevicewidget.h \
	csshelpers.h \
	filechooserhelpers.h \
	lastdir.h \
	openfiledialog.h \
	popupmenu.h \
	ramsizeradiogroup.h \
	resourcebrowser.h \
	resourcecheckbutton.h \
	resourcecombobox.h \
	resourceentry.h \
	resourcefilechooser.h \
	resourcehelpers.h \
	resourcenumericstring.h \
	resourceradiogroup.h \
	resourcescale.h \
	resourcespinbutton.h \
	resourceswitch.h \
	resourcewidgetmediator.h \
	savefiledialog.h \
	selectdirectorydialog.h \
	unicodehelpers.h \
	uiactionbutton.h \
	vice_gtk3.h \
	vice_gtk3_settings.h \
	widgethelpers.h
