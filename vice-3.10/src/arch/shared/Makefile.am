# Makefile.am for the shared archdep code

# Set directory constants here as well. Somehow that seems to be the way
# autotools wants us to handle bin/, data/, doc/ and such paths, not via
# config.h.
#
# To properly handle bindists, we'll probably have to use a few branches here
# depending on target and set some vars to relative paths for at least Windows
# and MacOS dmg things.
#
# -- compyx, 2020-02-08

SUBDIRS = hwsiddrv mididrv socketdrv sounddrv
if !USE_HEADLESSUI
SUBDIRS += hotkeys
endif

# The core/rtc and lib/p64 directories are required in order to be able to
# include drive.h

AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_srcdir)/src/arch/shared \
	-I$(top_srcdir)/src/arch/shared/hotkeys \
	$(extra_includes) \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/core/rtc \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/rs232drv \
	-DVICE_DATADIR='"$(datadir)/$(PACKAGE)"' \
	-DVICE_DOCDIR='"$(docdir)"'

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_OBJCFLAGS = @VICE_OBJCFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libarchdep.a

if MACOS_COMPILE
# Build native wrapper executable used by macOS apps
noinst_PROGRAMS = macOS-launcher
macOS_launcher_SOURCES = macOS-launcher.c
# Don't link this launcher app to the usual deps
macOS-launcher$(EXEEXT): CPPFLAGS =
macOS-launcher$(EXEEXT): CFLAGS =
macOS-launcher$(EXEEXT): LDFLAGS =
endif

libarchdep_a_SOURCES = \
	archdep_access.c \
	archdep_boot_path.c \
	archdep_cbmfont.c \
	archdep_chdir.c \
	archdep_close.c \
	archdep_create_user_cache_dir.c \
	archdep_create_user_config_dir.c \
	archdep_create_user_state_dir.c \
	archdep_current_dir.c \
	archdep_default_autostart_disk_image_file_name.c \
	archdep_default_fliplist_file_name.c \
	archdep_default_joymap_file_name.c \
	archdep_default_logfile.c \
	archdep_default_logger.c \
	archdep_default_portable_resource_file_name.c \
	archdep_default_resource_file_name.c \
	archdep_default_rtc_file_name.c \
	archdep_default_sysfile_pathlist.c \
	archdep_dir.c \
	archdep_ethernet_available.c \
	archdep_exit.c \
	archdep_expand_path.c \
	archdep_extra_title_text.c \
	archdep_fdopen.c \
	archdep_file_exists.c \
	archdep_file_is_blockdev.c \
	archdep_file_is_chardev.c \
	archdep_file_size.c \
	archdep_filename_parameter.c \
	archdep_fix_permissions.c \
	archdep_fix_streams.c \
	archdep_fseeko.c \
	archdep_ftello.c \
	archdep_get_current_drive.c \
	archdep_get_hvsc_dir.c \
	archdep_get_runtime_info.c \
	archdep_get_vice_datadir.c \
	archdep_get_vice_docsdir.c \
	archdep_get_vice_drivesdir.c \
	archdep_get_vice_hotkeysdir.c \
	archdep_get_vice_machinedir.c \
	archdep_getcwd.c \
	archdep_glob.c \
	archdep_home_path.c \
	archdep_icon_path.c \
	archdep_is_haiku.c \
	archdep_is_macos_bindist.c \
	archdep_is_windows_nt.c \
	archdep_kbd_get_host_mapping.c \
	archdep_list_drives.c \
	archdep_make_backup_filename.c \
	archdep_mkdir.c \
	archdep_mkstemp_fd.c \
	archdep_open_default_log_file.c \
	archdep_path_is_relative.c \
	archdep_program_name.c \
	archdep_program_path.c \
	archdep_quote_parameter.c \
	archdep_quote_unzip.c \
	archdep_rawnet_capability.c \
	archdep_real_path.c \
	archdep_remove.c \
	archdep_rename.c \
	archdep_require_vkbd.c \
	archdep_rmdir.c \
	archdep_rtc_get_centisecond.c \
	archdep_sanitize_filename.c \
	archdep_set_current_drive.c \
	archdep_set_openmp_wait_policy.c \
	archdep_signals.c \
	archdep_sleep.c \
	archdep_socketpeek.c \
	archdep_sound.c \
	archdep_spawn.c \
	archdep_startup_log_error.c \
	archdep_stat.c \
	archdep_tick.c \
	archdep_tmpnam.c \
	archdep_user_cache_path.c \
	archdep_user_config_path.c \
	archdep_user_state_path.c \
	archdep_usleep.c \
	archdep_xdg.c \
	console.c \
	coproc.c \
	dynlib.c \
	rawnetarch.c \
	rs232dev.c \
	signals.c

if USE_SDLUI
libarchdep_a_SOURCES += uiactions.c
endif
if USE_SDL2UI
libarchdep_a_SOURCES += uiactions.c
endif
if USE_GTK3UI
libarchdep_a_SOURCES += uiactions.c
endif

if UNIX_COMPILE
libarchdep_a_SOURCES += \
	rawnetarch_tuntap.c \
	rawnetarch_unix.c
endif

if MACOS_COMPILE
libarchdep_a_SOURCES += macOS-util.m
endif

EXTRA_DIST = \
	archapi.h \
	archdep_access.h \
	archdep_beos.h \
	archdep_boot_path.h \
	archdep_cbmfont.h \
	archdep_chdir.h \
	archdep_close.h \
	archdep_create_user_cache_dir.h \
	archdep_create_user_config_dir.h \
	archdep_create_user_state_dir.h \
	archdep_current_dir.h \
	archdep_default_autostart_disk_image_file_name.h \
	archdep_default_fliplist_file_name.h \
	archdep_default_joymap_file_name.h \
	archdep_default_logfile.h \
	archdep_default_logger.h \
	archdep_default_portable_resource_file_name.h \
	archdep_default_resource_file_name.h \
	archdep_default_rtc_file_name.h \
	archdep_default_sysfile_pathlist.h \
	archdep_defs.h \
	archdep_dir.h \
	archdep_ethernet_available.h \
	archdep_exit.h \
	archdep_expand_path.h \
	archdep_extra_title_text.h \
	archdep_fdopen.h \
	archdep_file_exists.h \
	archdep_file_is_blockdev.h \
	archdep_file_is_chardev.h \
	archdep_file_size.h \
	archdep_filename_parameter.h \
	archdep_fix_permissions.h \
	archdep_fix_streams.h \
	archdep_fseeko.h \
	archdep_ftello.h \
	archdep_get_current_drive.h \
	archdep_get_hvsc_dir.h \
	archdep_get_runtime_info.h \
	archdep_get_vice_datadir.h \
	archdep_get_vice_docsdir.h \
	archdep_get_vice_drivesdir.h \
	archdep_get_vice_hotkeysdir.h \
	archdep_get_vice_machinedir.h \
	archdep_getcwd.h \
	archdep_glob.h \
	archdep_home_path.h \
	archdep_icon_path.h \
	archdep_is_haiku.h \
	archdep_is_macos_bindist.h \
	archdep_is_windows_nt.h \
	archdep_kbd_get_host_mapping.h \
	archdep_list_drives.h \
	archdep_make_backup_filename.h \
	archdep_mkdir.h \
	archdep_mkstemp_fd.h \
	archdep_network.h \
	archdep_open_default_log_file.h \
	archdep_path_is_relative.h \
	archdep_program_name.h \
	archdep_program_path.h \
	archdep_quote_parameter.h \
	archdep_quote_unzip.h \
	archdep_rawnet_capability.h \
	archdep_real_path.h \
	archdep_remove.h \
	archdep_rename.h \
	archdep_require_vkbd.h \
	archdep_rmdir.h \
	archdep_rtc_get_centisecond.h \
	archdep_sanitize_filename.h \
	archdep_set_current_drive.h \
	archdep_set_openmp_wait_policy.h \
	archdep_signals.h \
	archdep_sleep.h \
	archdep_socketpeek.h \
	archdep_sound.h \
	archdep_spawn.h \
	archdep_startup_log_error.h \
	archdep_stat.h \
	archdep_tick.h \
	archdep_tmpnam.h \
	archdep_unix.h \
	archdep_unix.h \
	archdep_user_cache_path.h \
	archdep_user_config_path.h \
	archdep_user_state_path.h \
	archdep_usleep.h \
	archdep_win32.h \
	archdep_xdg.h \
	console_none.c \
	console_unix.c \
	coproc.h \
	dynlib-unix.c \
	dynlib-win32.c \
	macOS-common-runtime.sh \
	macOS-launcher.sh \
	macOS-redirect-launcher.applescript \
	macOS-util.h \
	macOS-wrap-ar.sh \
	macOS-wrap-ranlib.sh \
	make-bindist_osx.sh \
	make_bindist_win32.sh \
	rawnetarch.h \
	rawnetarch_win32.c \
	rs232-unix-dev.c \
	rs232-win32-dev.c \
	uiactions.h \
	README-bindist.txt

if !MACOS_COMPILE
# Distribute the native launcher source on non-macOS platforms
EXTRA_DIST += macOS-launcher.c
endif

# FIXME: the Extra MacOS script stuff should live elsewhere
