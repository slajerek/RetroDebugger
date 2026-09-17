/** \file   uisettings.c
 * \brief   GTK3 main settings dialog
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES SaveResourcesOnExit     all
 * $VICERES ConfirmOnExit           all
 * $VICERES PauseOnSettings         all
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 */


/* The settings_grid is supposed to become this:
 *
 * +--------------+---------------------------+
 * | treeview     |                           |
 * |  with        |                           |
 * |   settings   |    central widget,        |
 * |  more        |    depending on which     |
 * |   foo        |    item is selected in    |
 * |   bar        |    the treeview           |
 * |    whatever  |                           |
 * | burp         |                           |
 * +--------------+---------------------------+
 *
 * And this is handled by the dialog itself:
 * +------------------------------------------+
 * | load | save | load... | save... | close  |
 * +------------------------------------------+
 */

#include "vice.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "archdep.h"
#include "cartridge.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uimachinewindow.h"
#include "util.h"
#include "vice_gtk3.h"

#include "settings_autofire.h"
#include "settings_autostart.h"
#include "settings_burstmode.h"
#include "settings_c128fullbanks.h"
#include "settings_c128functionrom.h"
#include "settings_c64memhacks.h"
#include "settings_c64dtvflash.h"
#include "settings_controlport.h"
#include "settings_cpm.h"
#include "settings_crt.h"
#include "settings_default_cart.h"
#include "settings_digimax.h"
#include "settings_dqbb.h"
#include "settings_drive.h"
#include "settings_ds12c887.h"
#include "settings_easyflash.h"
#include "settings_environment.h"
#ifdef HAVE_RAWNET
# include "settings_ethernet.h"
# include "settings_ethernetcart.h"
#endif
#include "settings_expert.h"
#include "settings_finalexpansion.h"
#include "settings_fsdevice.h"
#include "settings_georam.h"
#include "settings_gmod2.h"
#include "settings_gmod2c128.h"
#include "settings_gmod3.h"
#include "settings_host_display.h"
#include "settings_hotkeys.h"
#include "settings_hvsc.h"
#include "settings_ide64.h"
#include "settings_ieee488.h"
#include "settings_ieeeflash64.h"
#include "settings_io.h"
#include "settings_isepic.h"
#include "settings_jam.h"
#include "settings_joystick.h"
#include "settings_keyboard.h"
#include "settings_ltkernal.h"
#include "settings_magicvoice.h"
#ifdef HAVE_MIDI
# include "settings_midi.h"
#endif
#include "settings_misc.h"
#include "settings_minimon.h"
#include "settings_megabyter.h"
#include "settings_megacart.h"
#include "settings_mmc64.h"
#include "settings_mmcr.h"
#include "settings_model.h"
#include "settings_monitor.h"
#include "settings_netplay.h"
#include "settings_petcolourgraphics.h"
#include "settings_petdww.h"
#include "settings_pethre.h"
#include "settings_petreu.h"
#include "settings_plus4digiblaster.h"
#include "settings_printer.h"
#include "settings_ramcart.h"
#include "settings_ramlink.h"
#include "settings_ramreset.h"
#include "settings_retroreplay.h"
#include "settings_reu.h"
#include "settings_rexramfloppy.h"
#include "settings_rom.h"
#ifdef HAVE_RAWNET
# include "settings_rrnetmk3.h"
#endif
#include "settings_rs232.h"
#include "settings_sampler.h"
#include "settings_scpu64.h"
#include "settings_sfxsoundexpander.h"
#include "settings_sfxsoundsampler.h"
#include "settings_sidcart.h"
#include "settings_snapshot.h"
#include "settings_sound.h"
#include "settings_soundchip.h"
#include "settings_speed.h"
#include "settings_supersnapshotv5.h"
#include "settings_tapeport.h"
#include "settings_ultimem.h"
#include "settings_userport.h"
#include "settings_vfli.h"
#include "settings_vicflashplugin.h"
#include "settings_vicieee488.h"
#include "settings_vicioram.h"
#include "settings_video.h"

#include "uisettings.h"


/** \brief  CSS to allow collapsing/expanding tree nodes with crsr keys
 *
 * Note the 'treeview' rule, before Gtk+ 3.20 this was 'GtkTreeView'
 *
 *
 */
#define TREEVIEW_CSS \
    "@binding-set SettingsTreeViewBinding\n" \
    "{\n" \
    "    bind \"Left\"  { \"select-cursor-parent\" ()\n" \
    "                     \"expand-collapse-cursor-row\" (0,0,0) };\n" \
    "    bind \"Right\" { \"expand-collapse-cursor-row\" (0,1,0) };\n" \
    "}\n" \
    "\n" \
    "treeview\n" \
    "{\n" \
    "    -gtk-key-bindings: SettingsTreeViewBinding;\n" \
    "}\n" \
    "treeview .separator\n" \
    "{\n" \
    "    color: darker (@theme_bg_color);\n" \
    "}\n"

/** \brief  Number of columns in the tree model
 */
#define NUM_COLUMNS 3

/** \brief  Column indici for the tree model
 */
enum {
    COLUMN_NAME = 0,    /**< name */
    COLUMN_ID,          /**< id */
    COLUMN_CALLBACK     /**< callback function */
};

/** \brief  Initial dialog width
 *
 * This is not how wide the dialog will actually become, that is determined by
 * the Gtk theme applied. But it's a rough estimate. A little over 900 pixels
 * will still fit the dialog on a 1024 pixels wide display.
 */
#define DIALOG_WIDTH 920

/** \brief  Tolerated extra dialog width added by decorations/theme
 *
 * Gtk has the annoying "feature" to not count any window decorations when we
 * request a size. So when the dialog pops up, we immediately get a 'configure'
 * event of the window triggered by a resize of said window. So in order for
 * the debugging checks on the resizing of the dialog we need to allow some
 * margin.
 *
 * Gnome's "Adwaita-dark" theme appears to add 16 pixels.
 */
#define DIALOG_WIDTH_TOLERANCE 20

/** \brief  Initial dialog height
 *
 * This is not how tall the dialog will actually become, that is determined by
 * the Gtk theme applied. But it's a rough estimate. With a 1280x720 resolution
 * in Gnome (with the default top bar and a bottom application selector bar)
 * a height of about 560-580 is the maximum that'll make the dialog with still
 * fit.
 */
#define DIALOG_HEIGHT 560

/** \brief  Tolerated extra dialog height added by decorations/theme
 *
 * Gtk has the annoying "feature" to not count any window decorations when we
 * request a size. So when the dialog pops up, we immediately get a 'configure'
 * event of the window triggered by a resize of said window. So in order for
 * the debugging checks on the resizing of the dialog we need to allow some
 * margin.
 *
 * Gnome's "Adwaita-dark" theme appears to add 50 pixels.
 */
#define DIALOG_HEIGHT_TOLERANCE 60

/** \brief  Maximum width the UI can be
 *
 * This again is not a really a fixed value, but more of an indicator when the
 * UI might get too large after any decorations are applied. The idea is to
 * have a UI that works on a 1280x768 resolution without requiring scrollbars.
 */
#define DIALOG_WIDTH_MAX 1024

/** \brief  Maximum height the UI can be
 *
 * This again is not a really a fixed value, but more of an indicator when the
 * UI might get too large after any decorations are applied. The idea is to
 * have a UI that works on a 1280x768 resolution without requiring scrollbars.
 */
#define DIALOG_HEIGHT_MAX 640

/** \brief  Enum used for the "response" callback of the settings dialog
 *
 * All values must be positive since Gtk reserves standard responses in its
 * GtkResponse enum as negative values.
 */
enum {
    RESPONSE_RESET = 1, /**< reset current central widget */
    RESPONSE_FACTORY,   /**< set current central widget's resources to their
                             factory settings */
    RESPONSE_DEFAULT    /**< Restore default settings */
};


/*
 * I/O extensions per emulator
 */

/* Sorting the cartridges is a bit of a challenge, since many of them fall
   into more than one category. So we make a couple groups and put them
   into the one that matches their perceived "primary" function best */

/* {{{ c64_cartridges */
/** \brief  List of C64 cartidge settings (x64, x64sc)
 */
static ui_settings_tree_node_t c64_cartridges[] = {

    { "Default cartridge",
      "default-cart",
      settings_default_cart_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* RAM Expansions and "RAM Disk" */

    { CARTRIDGE_NAME_DQBB,
      "dqbb",
      settings_dqbb_widget_create, NULL },
    { CARTRIDGE_NAME_GEORAM,
      "geo-ram",
      settings_georam_widget_create, NULL },
    { CARTRIDGE_NAME_REU,
      "reu",
      settings_reu_widget_create, NULL },
    { CARTRIDGE_NAME_RAMCART,
      "ramcart",
      settings_ramcart_widget_create, NULL },
    { CARTRIDGE_NAME_RAMLINK,
      "ramlink",
      settings_ramlink_widget_create, NULL },
    { CARTRIDGE_NAME_REX_RAMFLOPPY,
      "rexramfloppy",
      settings_rexramfloppy_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Flash+EPROM Cartridges */

    { CARTRIDGE_NAME_EASYFLASH,
      "easyflash",
      settings_easyflash_widget_create, NULL },
    { CARTRIDGE_NAME_GMOD2,
      "gmod2",
      settings_gmod2_widget_create, NULL },
    { CARTRIDGE_NAME_GMOD3,
      "gmod3",
      settings_gmod3_widget_create, NULL },
    { CARTRIDGE_NAME_MEGABYTER,
      "megabyter",
      settings_megabyter_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Freezer+Utility Cartridges */

    { CARTRIDGE_NAME_EXPERT,
      "expert-cart",
      settings_expert_widget_create, NULL },
    { CARTRIDGE_NAME_ISEPIC,
      "isepic",
      settings_isepic_widget_create, NULL },
    { CARTRIDGE_NAME_RETRO_REPLAY,
      "retro-replay",
      settings_retroreplay_widget_create, NULL },
    { CARTRIDGE_NAME_SUPER_SNAPSHOT_V5,
      "super-snapshot-v5",
      settings_supersnapshotv5_widget_create, NULL },
#ifdef HAVE_RAWNET

    UI_SETTINGS_SEPARATOR,

    /* Network Expansions */

    { CARTRIDGE_NAME_ETHERNETCART,
      "ethernet-cart",
      settings_ethernetcart_widget_create, NULL },
    { CARTRIDGE_NAME_RRNETMK3,
      "rrnetmk3",
      settings_rrnetmk3_widget_create, NULL },

#endif
    UI_SETTINGS_SEPARATOR,

    /* Storage Host Adapters */

    { CARTRIDGE_NAME_IDE64,
      "ide64",
      settings_ide64_widget_create, NULL },
    { CARTRIDGE_NAME_IEEE488,
      "ieee-488",
      settings_ieee488_widget_create, NULL },
    { CARTRIDGE_NAME_IEEEFLASH64,
      "ieee-flash-64",
      settings_ieeeflash64_widget_create, NULL },
    { CARTRIDGE_NAME_LT_KERNAL,
      "ltkernal",
      settings_ltkernal_widget_create, NULL },
    { CARTRIDGE_NAME_MMC64,
      "mmc64",
      settings_mmc64_widget_create, NULL },
    { CARTRIDGE_NAME_MMC_REPLAY,
      "mmc-replay",
      settings_mmcr_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Audio Expansions + Adapters */

    { CARTRIDGE_NAME_DIGIMAX,
      "digimax",
      settings_digimax_widget_create, NULL },
    { CARTRIDGE_NAME_MAGIC_VOICE,
      "magic-voice",
      settings_magicvoice_widget_create, NULL },
#ifdef HAVE_MIDI
    { "MIDI emulation",
      "midi",
      settings_midi_widget_create, NULL },
#endif
    { CARTRIDGE_NAME_SFX_SOUND_EXPANDER,
      "sfx-sound-expander",
      settings_sfxsoundexpander_widget_create, NULL },
    { CARTRIDGE_NAME_SFX_SOUND_SAMPLER,
      "sfx-sound-sampler",
      settings_sfxsoundsampler_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* CPU Expansions */

    { CARTRIDGE_NAME_CPM,
      "cpm-cart",
      settings_cpm_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* misc */

    { CARTRIDGE_NAME_DS12C887RTC,
      "ds12c887-rtc",
      settings_ds12c887_widget_create, NULL },
     UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ scpu64_cartridges */
/** \brief  List of SuperCPU64 extensions (xscpu64)
 */
static ui_settings_tree_node_t scpu64_cartridges[] = {
    { "Default cartridge",
      "default-cart",
      settings_default_cart_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* RAM Expansions and "RAM Disk" */

    { CARTRIDGE_NAME_GEORAM,
      "geo-ram",
      settings_georam_widget_create, NULL },
    { CARTRIDGE_NAME_REU,
      "reu",
      settings_reu_widget_create, NULL },
    { CARTRIDGE_NAME_RAMCART,
      "ramcart",
      settings_ramcart_widget_create, NULL },
    { CARTRIDGE_NAME_REX,
      "rexramfloppy",
      settings_rexramfloppy_widget_create, NULL },
    { CARTRIDGE_NAME_RAMLINK,
      "ramlink",
      settings_ramlink_widget_create, NULL },
    { CARTRIDGE_NAME_DQBB,
      "dqbb",
      settings_dqbb_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Flash+EPROM Cartridges */

    { CARTRIDGE_NAME_EASYFLASH,
      "easyflash",
      settings_easyflash_widget_create, NULL },
    { CARTRIDGE_NAME_GMOD2,
      "gmod2",
      settings_gmod2_widget_create, NULL },
    { CARTRIDGE_NAME_GMOD3,
      "gmod3",
      settings_gmod3_widget_create, NULL },
    { CARTRIDGE_NAME_MEGABYTER,
      "megabyter",
      settings_megabyter_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Freezer+Utility Cartridges */

    { CARTRIDGE_NAME_EXPERT,
      "expert-cart",
      settings_expert_widget_create, NULL },
    { CARTRIDGE_NAME_ISEPIC,
      "isepic",
      settings_isepic_widget_create, NULL },
    { CARTRIDGE_NAME_RETRO_REPLAY,
      "retro-replay",
      settings_retroreplay_widget_create, NULL },
    { CARTRIDGE_NAME_SUPER_SNAPSHOT_V5,
      "super-snapshot-v5",
      settings_supersnapshotv5_widget_create, NULL },
#ifdef HAVE_RAWNET

    UI_SETTINGS_SEPARATOR,

    /* Network Expansions */

    { CARTRIDGE_NAME_ETHERNETCART,
      "ethernet-cart",
      settings_ethernetcart_widget_create, NULL },
    { CARTRIDGE_NAME_RRNETMK3,
      "rrnetmk3",
      settings_rrnetmk3_widget_create, NULL },
#endif

    UI_SETTINGS_SEPARATOR,

    /* Storage Host Adapters */

    { CARTRIDGE_NAME_IDE64,
      "ide64",
      settings_ide64_widget_create, NULL },
    { CARTRIDGE_NAME_IEEE488,
      "ieee-488",
      settings_ieee488_widget_create, NULL },
    { CARTRIDGE_NAME_MMC64,
      "mmc64",
      settings_mmc64_widget_create, NULL },
    { CARTRIDGE_NAME_MMC_REPLAY,
      "mmc-replay",
      settings_mmcr_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Audio Expansions + Adapters */

    { CARTRIDGE_NAME_DIGIMAX,
      "digimax",
      settings_digimax_widget_create, NULL },
    { CARTRIDGE_NAME_MAGIC_VOICE,
      "magic-voice",
      settings_magicvoice_widget_create, NULL },
#ifdef HAVE_MIDI
    { "MIDI emulation",
      "midi",
      settings_midi_widget_create, NULL },
#endif
    { CARTRIDGE_NAME_SFX_SOUND_EXPANDER,
      "sfx-sound-expander",
      settings_sfxsoundexpander_widget_create, NULL },
    { CARTRIDGE_NAME_SFX_SOUND_SAMPLER,
      "sfx-sound-sampler",
      settings_sfxsoundsampler_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* misc */

    { CARTRIDGE_NAME_DS12C887RTC,
      "ds12c887-rtc",
      settings_ds12c887_widget_create, NULL },

    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ c128_cartridges */
/** \brief  I/O extensions for C128
 */
static ui_settings_tree_node_t c128_cartridges[] = {
    { "Default cartridge",
      "default-cart",
      settings_default_cart_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* RAM Expansions and "RAM Disk" */

    { CARTRIDGE_NAME_GEORAM,
      "geo-ram",
      settings_georam_widget_create, NULL },
    { CARTRIDGE_NAME_REU,
      "reu",
      settings_reu_widget_create, NULL },
    { CARTRIDGE_NAME_RAMLINK,
      "ramlink",
      settings_ramlink_widget_create, NULL },
    { CARTRIDGE_NAME_REX_RAMFLOPPY,
      "rexramfloppy",
      settings_rexramfloppy_widget_create, NULL },
    { CARTRIDGE_NAME_RAMCART,
      "ramcart",
      settings_ramcart_widget_create, NULL },
    { CARTRIDGE_NAME_DQBB,
      "dqbb",
      settings_dqbb_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Flash+EPROM Cartridges */

    { CARTRIDGE_NAME_EASYFLASH,
      "easyflash",
      settings_easyflash_widget_create, NULL },
    { CARTRIDGE_NAME_GMOD2,
      "gmod2",
      settings_gmod2_widget_create, NULL },
    { CARTRIDGE_C128_NAME_GMOD2C128,
      "gmod2c128",
      settings_gmod2c128_widget_create, NULL },
    { CARTRIDGE_NAME_GMOD3,
      "gmod3",
      settings_gmod3_widget_create, NULL },
    { CARTRIDGE_NAME_MEGABYTER,
      "megabyter",
      settings_megabyter_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Freezer+Utility Cartridges */

    { CARTRIDGE_NAME_EXPERT,
      "expert-cart",
      settings_expert_widget_create, NULL },
    { CARTRIDGE_NAME_ISEPIC,
      "isepic",
      settings_isepic_widget_create, NULL },
    { CARTRIDGE_NAME_RETRO_REPLAY,
      "retro-replay",
      settings_retroreplay_widget_create, NULL },
    { CARTRIDGE_NAME_SUPER_SNAPSHOT,
      "super-snapshot-v5",
      settings_supersnapshotv5_widget_create, NULL },
#ifdef HAVE_RAWNET
    UI_SETTINGS_SEPARATOR,

    /* Network Expansions */

    { CARTRIDGE_NAME_ETHERNETCART,
      "ethernet-cart",
      settings_ethernetcart_widget_create, NULL },
    { CARTRIDGE_NAME_RRNETMK3,
      "rrnetmk3",
      settings_rrnetmk3_widget_create, NULL },
#endif
    UI_SETTINGS_SEPARATOR,

    /* Storage Host Adapters */

    { CARTRIDGE_NAME_IDE64,
      "ide64",
      settings_ide64_widget_create, NULL },
    { CARTRIDGE_NAME_LT_KERNAL,
      "ltkernal",
      settings_ltkernal_widget_create, NULL },
    { CARTRIDGE_NAME_IEEE488,
      "ieee-488",
      settings_ieee488_widget_create, NULL },
    { CARTRIDGE_NAME_MMC64,
      "mmc64",
      settings_mmc64_widget_create, NULL },
    { CARTRIDGE_NAME_MMC_REPLAY,
      "mmc-replay",
      settings_mmcr_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Audio Expansions + Adapters */

    { CARTRIDGE_NAME_DIGIMAX,
      "digimax",
      settings_digimax_widget_create, NULL },
    { CARTRIDGE_NAME_MAGIC_VOICE,
      "magic-voice",
      settings_magicvoice_widget_create, NULL },
#ifdef HAVE_MIDI
    { "MIDI emulation",
      "midi",
      settings_midi_widget_create, NULL },
#endif
    { CARTRIDGE_NAME_SFX_SOUND_EXPANDER,
      "sfx-sound-expander",
      settings_sfxsoundexpander_widget_create, NULL },
    { CARTRIDGE_NAME_SFX_SOUND_SAMPLER,
      "sfx-sound-sampler",
      settings_sfxsoundsampler_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* CPU Expansions */

    { CARTRIDGE_NAME_CPM,
      "cpm-cart",
      settings_cpm_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* misc */

    { CARTRIDGE_NAME_DS12C887RTC,
      "ds12c887-rtc",
       settings_ds12c887_widget_create, NULL },

    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ vic20_cartridges */
/** \brief  List of VIC-20 I/O extensions
 */
static ui_settings_tree_node_t vic20_cartridges[] = {
    { "Default cartridge",
      "default-cart",
      settings_default_cart_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    { CARTRIDGE_VIC20_NAME_MINIMON,
      "Minimon",
      settings_minimon_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Flash+EPROM Cartridges */

    { CARTRIDGE_VIC20_NAME_FINAL_EXPANSION,
      "final-expansion",
      settings_finalexpansion_widget_create, NULL },
    { CARTRIDGE_VIC20_NAME_MEGACART,
      "mega-cart",
      settings_megacart_widget_create, NULL },
    { CARTRIDGE_VIC20_NAME_UM,
      "ultimem",
      settings_ultimem_widget_create, NULL },
    { CARTRIDGE_VIC20_NAME_FP,
      "vic-flash-plugin",
      settings_vicflashplugin_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Storage Host Adapters */

    { CARTRIDGE_VIC20_NAME_IEEE488,
      "ieee-488",
      settings_vicieee488_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Audio Expansions + Adapters */

#ifdef HAVE_MIDI
    { "MIDI emulation",
      "midi",
      settings_midi_widget_create, NULL },
#endif

    UI_SETTINGS_SEPARATOR,

    /* C64 I/O Expansions connected via "MasC=uerade" adapter */

    { CARTRIDGE_NAME_DIGIMAX " (MasC=uerade)",
      "digimax",
      settings_digimax_widget_create, NULL },
    { CARTRIDGE_NAME_DS12C887RTC " (MasC=uerade)",
      "ds12c887-rtc",
      settings_ds12c887_widget_create, NULL },
#ifdef HAVE_RAWNET
    { CARTRIDGE_NAME_ETHERNETCART " (MasC=uerade)",
      "ethernet-cart",
      settings_ethernetcart_widget_create, NULL },
#endif
    { CARTRIDGE_NAME_GEORAM " (MasC=uerade)",
      "geo-ram",
      settings_georam_widget_create, NULL },
    { CARTRIDGE_NAME_SFX_SOUND_EXPANDER " (MasC=uerade)",
      "sfx-sound-expander",
      settings_sfxsoundexpander_widget_create, NULL },
    { CARTRIDGE_NAME_SFX_SOUND_SAMPLER " (MasC=uerade)",
      "sfx-sound-sampler",
      settings_sfxsoundsampler_widget_create, NULL },

    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ plus4_io_extensions */
/** \brief  List of Plus4 I/O extensions
 */
static ui_settings_tree_node_t plus4_io_extensions[] = {
    { "Default cartridge",
      "default-cart",
      settings_default_cart_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Audio Expansions + Adapters */

    { "Digi-Blaster add-on",
      "digi-blaster",
      settings_plus4_digiblaster_widget_create, NULL },
    { "SID Card",
      "sid-card",
      settings_sidcart_widget_create, NULL },

    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ cbm2_io_extensions */
/** \brief  List of CBM2 I/O extensions
 */
static ui_settings_tree_node_t cbm2_io_extensions[] = {
    { "Default cartridge",
      "default-cart",
      settings_default_cart_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ pet_io_extensions */
/** \brief  List of PET I/O extensions
 */
static ui_settings_tree_node_t pet_io_extensions[] = {
    /* RAM Expansions and "RAM Disk" */

    { "PET RAM Expansion Unit",
      "pet-reu",
      settings_petreu_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Audio Expansions + Adapters */

    { "SID Card",
      "sid-card",
      settings_sidcart_widget_create, NULL },

    UI_SETTINGS_SEPARATOR,

    /* Graphics Expansions + Adapters */

    { "PET Colour graphics",
      "pet-colour",
      settings_petcolourgraphics_widget_create, NULL },
    { "PET DWW hi-res graphics",
      "pet-dww",
      settings_petdww_widget_create, NULL },
    { "PET HRE hi-res graphics",
      "pet-hre",
      settings_pethre_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */


/*
 * Main tree nodes per emulator
 */

/*****************************************************************************
 *                  VSID tree nodes for the settings UI                      *
 ****************************************************************************/

/* {{{ main_nodes_vsid */
/** \brief  Main tree nodes for VSID
 */
static ui_settings_tree_node_t main_nodes_vsid[] = {
    { "Sound driver", "sound-driver", settings_sound_widget_create, NULL },
    { "SID",        "sid",      settings_soundchip_widget_create,   NULL },
    /* XXX: basically a selection between 'PAL'/'NTSC' (50/60Hz) */
    { "Model",      "model",    settings_model_widget_create,       NULL },
    { "Monitor",    "monitor",  settings_monitor_widget_create,     NULL },
    { "HVSC",       "hvsc",     settings_hvsc_widget_create,        NULL },
    { "Hotkeys",    "hotkeys",  settings_hotkeys_widget_create,     NULL },

    UI_SETTINGS_TERMINATOR
};
/* }}} */


/* {{{ host_nodes_generic */
/** \brief  Child nodes for the 'Host' node for all machines
 */
static ui_settings_tree_node_t host_nodes_generic[] = {
    { "Autostart",
      "autostart",
      settings_autostart_widget_create, NULL },
    { "Hotkeys",
      "hotkeys",
      settings_hotkeys_widget_create, NULL },
    { "Monitor",
      "monitor",
      settings_monitor_widget_create, NULL },
    { "Netplay",
      "netplay",
      settings_netplay_widget_create, NULL },
    { "Snapshot/event/media recording",
      "snapshot",
      settings_snapshot_widget_create, NULL },
    { "Environment",
      "environment",
      settings_environment_widget_create, NULL },
    { "CPU JAM action",
      "jam-action",
      settings_jam_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */


/*****************************************************************************
 *                  C64 tree nodes for the settings UI                       *
 ****************************************************************************/

/* {{{ machine_nodes_c64 */
/** \brief  Child nodes for the C64 'Machine' node
 */
static ui_settings_tree_node_t machine_nodes_c64[] = {
    { "Model",
      "model",
      settings_model_widget_create, NULL },
    { "ROM",
      "rom",
      settings_rom_widget_create, NULL },
    { "RAM",
      "ram",
      settings_ramreset_widget_create, NULL },
    { "Memory Expansion Hacks",
      "mem-hacks",
      settings_c64_memhacks_widget_create, NULL },
    { "I/O settings",
      "io",
      settings_io_widget_create, NULL },
    { "Burst Mode Modification",
      "burst-mode",
      settings_burstmode_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ display_nodes_c64 */
/** \brief  Child nodes for the C64 'Display' node
 */
static ui_settings_tree_node_t display_nodes_c64[] = {
    { "Host display",
      "host-display",
      settings_host_display_widget_create, NULL },
    { "VIC-II",
      "vicii",
      settings_video_widget_create, NULL },
    { "CRT",
      "crt",
      settings_crt_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ audio_nodes_c64 */
/** \brief  Child nodes for the C64 'Audio' node
 */
static ui_settings_tree_node_t audio_nodes_c64[] = {
    { "Sound driver",
      "sound-driver",
      settings_sound_widget_create, NULL },
    { "SID",
      "sid",
      settings_soundchip_widget_create, NULL },
    { "Sampler",
      "sampler",
      settings_sampler_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ input_nodes_c64 */
/** \brief  Child nodes for the C64 'Input devices' node
 */
static ui_settings_tree_node_t input_nodes_c64[] = {
    { "Keyboard",
      "keyboard",
      settings_keyboard_widget_create, NULL },
    { "Joystick",
      "joystick",
      settings_joystick_widget_create, NULL },
    { "Control port",
      "control-port",
      settings_controlport_widget_create, NULL },
    { "Autofire",
      "autofire",
      settings_autofire_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ peripheral_nodes_c64 */
/** \brief  Child nodes for the C64 'Peripheral devices' node
 */
static ui_settings_tree_node_t peripheral_nodes_c64[] = {
    /* "Output devices? drive is also input */
    { "Drive",
      "drive",
      settings_drive_widget_create, NULL },
    { "Host file system device",
      "fsdevice",
      settings_fsdevice_widget_create, NULL },
    { "Printer",
      "printer",
      settings_printer_widget_create, NULL },
#ifdef HAVE_RS232DEV
    { "RS232",
      "rs232",
      settings_rs232_widget_create, NULL },
#endif
   { "Userport devices",
      "userport-devices",
      settings_userport_widget_create, NULL },
    { "Tape port devices",
      "tapeport-devices",
      settings_tapeport_widget_create, NULL },
#ifdef HAVE_RAWNET
    { "Ethernet",
      "ethernet",
      settings_ethernet_widget_create, NULL },
#endif
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ main_nodes_c64 */
/** \brief  Main tree nodes for x64/x64sc
 */
static ui_settings_tree_node_t main_nodes_c64[] = {
    { "Host",               "host",         NULL,   host_nodes_generic },
    { "Machine",            "machine",      NULL,   machine_nodes_c64 },
    { "Display",            "display",      NULL,   display_nodes_c64 },
    { "Audio",              "audio",        NULL,   audio_nodes_c64 },
    { "Input devices",      "input",        NULL,   input_nodes_c64 },
    { "Peripheral devices", "peripheral",   NULL,   peripheral_nodes_c64 },
    { "Cartridges",         "cartridges",   NULL,   c64_cartridges },
    UI_SETTINGS_TERMINATOR
};
/* }}} */


/*****************************************************************************
 *                  C64DTV tree nodes for the settings UI                    *
 ****************************************************************************/

/* {{{ machine_nodes_c64dtv */
/** \brief  Child nodes for the C64DTV 'Machine' node
 */
static ui_settings_tree_node_t machine_nodes_c64dtv[] = {
    { "Model",
      "model",
      settings_model_widget_create, NULL },
    { "ROM",
      "rom",
      settings_rom_widget_create, NULL },
    { "RAM",
      "ram",
      settings_ramreset_widget_create, NULL },
    { "Flash",
      "flash",
      settings_c64dtvflash_widget_create, NULL },

    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ display_nodes_c64dtv */
/** \brief  Child nodes for the C64DTV 'Display' node
 */
static ui_settings_tree_node_t display_nodes_c64dtv[] = {
    { "Host display",
      "host-display",
      settings_host_display_widget_create, NULL },
    { "VIC-II",
      "vicii",
      settings_video_widget_create, NULL },
    { "CRT",
      "CRT",
      settings_crt_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ audio_nodes_c64dtv */
/** \brief  Child nodes for the C64DTV 'Audio' node
 */
static ui_settings_tree_node_t audio_nodes_c64dtv[] = {
    { "Sound driver",
      "sound-driver",
      settings_sound_widget_create, NULL },
    { "SID",
      "sid",
      settings_soundchip_widget_create, NULL },
    { "Sampler",
      "sampler",
      settings_sampler_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ input_nodes_c64dtv */
/** \brief  Child nodes for the C64DTV 'Input devices' node
 */
static ui_settings_tree_node_t input_nodes_c64dtv[] = {
    { "Keyboard",
      "keyboard",
      settings_keyboard_widget_create, NULL },
    { "Joystick",
      "joystick",
      settings_joystick_widget_create, NULL },
    { "Control port",
      "control-port",
      settings_controlport_widget_create, NULL },
    { "Autofire",
      "autofire",
      settings_autofire_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ peripheral_nodes_c64dtv */
/** \brief  Child nodes for the C64DTV 'Peripheral devices' node
 */
static ui_settings_tree_node_t peripheral_nodes_c64dtv[] = {
    { "Drive",
      "drive",
      settings_drive_widget_create, NULL },
    { "Host file system device",
      "fsdevice",
      settings_fsdevice_widget_create, NULL },
    { "Printer",
      "printer",
      settings_printer_widget_create, NULL },
    { "Userport devices",
      "userport-devices",
      settings_userport_widget_create, NULL },
      UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ main_nodes_c64dtv */
/** \brief  Main tree nodes for x64dtv
 */
static ui_settings_tree_node_t main_nodes_c64dtv[] = {
    { "Host",               "host",         NULL,   host_nodes_generic },
    { "Machine",            "machine",      NULL,   machine_nodes_c64dtv },
    { "Display",            "display",      NULL,   display_nodes_c64dtv },
    { "Audio",              "audio",        NULL,   audio_nodes_c64dtv },
    { "Input devices",      "input",        NULL,   input_nodes_c64dtv },
    { "Peripheral devices", "peripheral",   NULL,   peripheral_nodes_c64dtv },
    UI_SETTINGS_TERMINATOR
};
/* }}} */


/*****************************************************************************
 *                      C128 tree nodes for the settings UI                  *
 ****************************************************************************/

/* {{{ machine_nodes_c128 */
/** \brief  Child nodes for the C128 'Machine' node
 */
static ui_settings_tree_node_t machine_nodes_c128[] = {
    { "Model",
      "model",
      settings_model_widget_create, NULL },
    { "ROM",
      "rom",
      settings_rom_widget_create, NULL },
    { "RAM",
      "ram",
      settings_ramreset_widget_create, NULL },
    { "Function ROM",
      "function-rom",
      settings_c128functionrom_widget_create, NULL },
    { "Banks 2 & 3",
      "banks-23",
      settings_c128fullbanks_widget_create, NULL },
    { "I/O settings",
      "io",
      settings_io_widget_create, NULL },

    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ display_nodes_c128 */
/** \brief  Child nodes for the C128 'Display' node
 */
static ui_settings_tree_node_t display_nodes_c128[] = {
    { "Host display",
      "host-display",
      settings_host_display_widget_create, NULL },
    { "VIC-II",
      "vicii",
      settings_video_widget_create, NULL },
    { "VDC",
      "vdc",
      settings_video_widget_create_vdc, NULL },
    { "CRT",
      "crt",
      settings_crt_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ audio_nodes_c128 */
/** \brief  Child nodes for the C128 'Audio' node
 */
static ui_settings_tree_node_t audio_nodes_c128[] = {
    { "Sound driver",
      "sound-driver",
      settings_sound_widget_create, NULL },
    { "SID",
      "sid",
      settings_soundchip_widget_create, NULL },
    { "Sampler",
      "sampler",
      settings_sampler_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ input_nodes_c128 */
/** \brief  Child nodes for the C128 'Input devices' node
 */
static ui_settings_tree_node_t input_nodes_c128[] = {
    { "Keyboard",
      "keyboard",
      settings_keyboard_widget_create, NULL },
    { "Joystick",
      "joystick",
      settings_joystick_widget_create, NULL },
    { "Control port",
      "control-port",
      settings_controlport_widget_create, NULL },
    { "Autofire",
      "autofire",
      settings_autofire_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ peripheral_nodes_c128 */
/** \brief  Child nodes for the C128 'Peripheral devices' node
 */
static ui_settings_tree_node_t peripheral_nodes_c128[] = {
    /* "Output devices? drive is also input */
    { "Drive",
      "drive",
      settings_drive_widget_create, NULL },
    { "Host file system device",
      "fsdevice",
      settings_fsdevice_widget_create, NULL },
    { "Printer",
      "printer",
      settings_printer_widget_create, NULL },
#ifdef HAVE_RS232DEV
    { "RS232",
      "rs232",
      settings_rs232_widget_create, NULL },
#endif
    { "Userport devices",
      "userport-devices",
      settings_userport_widget_create, NULL },
    { "Tape port devices",
      "tapeport-devices",
      settings_tapeport_widget_create, NULL },
#ifdef HAVE_RAWNET
    { "Ethernet",
      "ethernet",
      settings_ethernet_widget_create, NULL },
#endif
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ main_nodes_c128 */
/** \brief  Main tree nodes for x128
 */
static ui_settings_tree_node_t main_nodes_c128[] = {
    { "Host",               "host",         NULL,   host_nodes_generic },
    { "Machine",            "machine",      NULL,   machine_nodes_c128 },
    { "Display",            "display",      NULL,   display_nodes_c128 },
    { "Audio",              "audio",        NULL,   audio_nodes_c128 },
    { "Input devices",      "input",        NULL,   input_nodes_c128 },
    { "Peripheral devices", "peripheral",   NULL,   peripheral_nodes_c128 },
    { "Cartridges",         "cartridges",   NULL,   c128_cartridges },
    UI_SETTINGS_TERMINATOR
};
/* }}} */


/*****************************************************************************
 *                  SCPU64 tree nodes for the settings UI                    *
 ****************************************************************************/

/* {{{ machine_nodes_scpu64 */
/** \brief  Child nodes for the SCPU64 'Machine' node
 */
static ui_settings_tree_node_t machine_nodes_scpu64[] = {
    { "Model",
      "model",
      settings_model_widget_create, NULL },
    { "SCPU64",
      "scpu64",
      settings_scpu64_widget_create, NULL },
    { "ROM",
      "rom",
      settings_rom_widget_create, NULL },
    { "RAM",
      "ram",
      settings_ramreset_widget_create, NULL },
    { "I/O settings",
      "io",
      settings_io_widget_create, NULL },
    { "Burst Mode Modification",
      "burst-mode",
      settings_burstmode_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ display_nodes_scpu64 */
/** \brief  Child nodes for the SCPU64 'Display' node
 */
static ui_settings_tree_node_t display_nodes_scpu64[] = {
    { "Host display",
      "host-display",
      settings_host_display_widget_create, NULL },
    { "VIC-II",
      "vicii",
      settings_video_widget_create, NULL },
    { "CRT",
      "CRT",
      settings_crt_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ audio_nodes_scpu64 */
/** \brief  Child nodes for the SCPU64 'Peripheral devices' node
 */
static ui_settings_tree_node_t audio_nodes_scpu64[] = {
    { "Sound driver",
      "sound-driver",
      settings_sound_widget_create, NULL },
    { "SID",
      "sid",
      settings_soundchip_widget_create, NULL },
    { "Sampler",
      "sampler",
      settings_sampler_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ input_nodes_scpu64 */
/** \brief  Child nodes for the SCPU64 'Input devices' node
 */
static ui_settings_tree_node_t input_nodes_scpu64[] = {
    { "Keyboard",
      "keyboard",
      settings_keyboard_widget_create, NULL },
    { "Joystick",
      "joystick",
      settings_joystick_widget_create, NULL },
    { "Control port",
      "control-port",
      settings_controlport_widget_create, NULL },
    { "Autofire",
      "autofire",
      settings_autofire_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ peripheral_nodes_scpu64 */
/** \brief  Child nodes for the SCPU64 'Peripheral devices' node
 */
static ui_settings_tree_node_t peripheral_nodes_scpu64[] = {
    { "Drive",
      "drive",
      settings_drive_widget_create, NULL },
    { "Host file system device",
      "fsdevice",
      settings_fsdevice_widget_create, NULL },
    { "Printer",
      "printer",
      settings_printer_widget_create, NULL },
#ifdef HAVE_RS232DEV
    { "RS232",
      "rs232",
      settings_rs232_widget_create, NULL },
#endif
    { "Userport devices",
      "userport-devices",
      settings_userport_widget_create, NULL },
#ifdef HAVE_RAWNET
    { "Ethernet",
      "ethernet",
      settings_ethernet_widget_create, NULL },
#endif
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ main_nodes_scpu64 */
/** \brief  Main tree nodes for xscpu64
 */
static ui_settings_tree_node_t main_nodes_scpu64[] = {
    { "Host",               "host",         NULL,   host_nodes_generic },
    { "Machine",            "machine",      NULL,   machine_nodes_scpu64 },
    { "Display",            "display",      NULL,   display_nodes_scpu64 },
    { "Audio",              "audio",        NULL,   audio_nodes_scpu64 },
    { "Input devices",      "input",        NULL,   input_nodes_scpu64 },
    { "Peripheral devices", "peripheral",   NULL,   peripheral_nodes_scpu64 },
    { "Cartridges",         "cartridges",   NULL,   scpu64_cartridges },
    UI_SETTINGS_TERMINATOR
};
/* }}} */


/*****************************************************************************
 *                  VIC-20 tree nodes for the settings UI                    *
 ****************************************************************************/

/* {{{ machine_nodes_vic20 */
/** \brief  Child nodes for the VIC20 'Machine' node
 */
static ui_settings_tree_node_t machine_nodes_vic20[] = {
    { "Model",
      "model",
      settings_model_widget_create, NULL },
    { "ROM",
      "rom",
      settings_rom_widget_create, NULL },
    { "RAM",
      "ram",
      settings_ramreset_widget_create, NULL },
    { "I/O settings",
      "io",
      settings_io_widget_create, NULL },
    { "I/O RAM",
      "io-ram",
      settings_vicioram_widget_create, NULL },
    { "VFLI modification",
      "vfli",
      settings_vfli_widget_create, NULL },
    { "SID Card",
      "sid-card",
      settings_sidcart_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ display_nodes_vic20 */
/** \brief  Child nodes for the VIC20 'Display' node
 */
static ui_settings_tree_node_t display_nodes_vic20[] = {
    { "Host display",
      "host-display",
      settings_host_display_widget_create, NULL },
    { "VIC",
      "vic",
      settings_video_widget_create, NULL },
    { "CRT",
      "CRT",
      settings_crt_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ audio_nodes_vic20 */
/** \brief  Child nodes for the VIC20 'Audio' node
 */
static ui_settings_tree_node_t audio_nodes_vic20[] = {
    { "Sound driver",
      "sound-driver",
      settings_sound_widget_create, NULL },
    { "SID",
      "sid",
      settings_soundchip_widget_create, NULL },
    { "Sampler",
      "sampler",
      settings_sampler_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ input_nodes_vic20 */
/** \brief  Child nodes for the VIC20 'Input devices' node
 */
static ui_settings_tree_node_t input_nodes_vic20[] = {
    { "Keyboard",
      "keyboard",
      settings_keyboard_widget_create, NULL },
    { "Joystick",
      "joystick",
      settings_joystick_widget_create, NULL },
    { "Control port",
      "control-port",
      settings_controlport_widget_create, NULL },
    { "Autofire",
      "autofire",
      settings_autofire_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ peripheral_nodes_vic20 */
/** \brief  Child nodes for the VIC20 'Peripheral devices' node
 */
static ui_settings_tree_node_t peripheral_nodes_vic20[] = {
    { "Drive",
      "drive",
      settings_drive_widget_create, NULL },
    { "Host file system device",
      "fsdevice",
      settings_fsdevice_widget_create, NULL },
    { "Printer",
      "printer",
      settings_printer_widget_create, NULL },
#ifdef HAVE_RS232DEV
    { "RS232",
      "rs232",
      settings_rs232_widget_create, NULL },
#endif
    { "Userport devices",
      "userport-devices",
      settings_userport_widget_create, NULL },
    { "Tapeport devices",
      "tapeport-devices",
      settings_tapeport_widget_create, NULL },
#ifdef HAVE_RAWNET
    { "Ethernet",
      "ethernet",
      settings_ethernet_widget_create, NULL },
#endif
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ main_nodes_vic20 */
/** \brief  Main tree nodes for xvic
 */
static ui_settings_tree_node_t main_nodes_vic20[] = {
    { "Host",               "host",         NULL,   host_nodes_generic },
    { "Machine",            "machine",      NULL,   machine_nodes_vic20 },
    { "Display",            "display",      NULL,   display_nodes_vic20 },
    { "Audio",              "audio",        NULL,   audio_nodes_vic20 },
    { "Input devices",      "input",        NULL,   input_nodes_vic20 },
    { "Peripheral devices", "peripheral",   NULL,   peripheral_nodes_vic20 },
    { "Cartridges",         "cartridges",   NULL,   vic20_cartridges },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/*****************************************************************************
 *                  Plus4/C16 tree nodes for the settings UI                 *
 ****************************************************************************/

/* {{{ machine_nodes_plus4 */
/** \brief  Child nodes for the Plus4 'Machine' node
 */
static ui_settings_tree_node_t machine_nodes_plus4[] = {
    { "Model",
      "model",
       settings_model_widget_create, NULL },
    { "ROM",
      "rom",
       settings_rom_widget_create, NULL },
    { "RAM",
      "ram",
      settings_ramreset_widget_create, NULL },
    { "I/O settings",
      "io",
      settings_io_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ display_nodes_plus4 */
/** \brief  Child nodes for the Plus4 'Display' node
 */
static ui_settings_tree_node_t display_nodes_plus4[] = {
    { "Host display",
      "host-display",
      settings_host_display_widget_create, NULL },
    { "TED",
      "ted",
      settings_video_widget_create, NULL },
    { "CRT",
      "CRT",
      settings_crt_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ audio_nodes_plus4 */
/** \brief  Child nodes for the Plus4 'Audio' node
 */
static ui_settings_tree_node_t audio_nodes_plus4[] = {
    { "Sound driver",
      "sound-driver",
      settings_sound_widget_create, NULL },
    { "SID",
      "sid",
      settings_soundchip_widget_create, NULL },
    { "Sampler",
      "sampler",
      settings_sampler_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ input_nodes_plus4 */
/** \brief  Child nodes for the Plus4 'Input devices' node
 */
static ui_settings_tree_node_t input_nodes_plus4[] = {
    { "Keyboard",
      "keyboard",
      settings_keyboard_widget_create, NULL },
    { "Joystick",
      "joystick",
      settings_joystick_widget_create, NULL },
    { "Control port",
      "control-port",
      settings_controlport_widget_create, NULL },
    { "Autofire",
      "autofire",
      settings_autofire_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ peripheral_nodes_plus4 */
/** \brief  Child nodes for the Plus4 'Peripheral devices' node
 */
static ui_settings_tree_node_t peripheral_nodes_plus4[] = {
    { "Drive",
      "drive",
      settings_drive_widget_create, NULL },
    { "Host file system device",
      "fsdevice",
      settings_fsdevice_widget_create, NULL },
    { "Printer",
      "printer",
      settings_printer_widget_create, NULL },
#ifdef HAVE_RS232DEV
    { "RS232",
      "rs232",
      settings_rs232_widget_create, NULL },
#endif
    { "Userport devices",
      "userport-devices",
      settings_userport_widget_create, NULL },
    { "Tape port devices",
      "tapeport-devices",
      settings_tapeport_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ main_nodes_plus4 */
/** \brief  Main tree nodes for xplus4
 */
static ui_settings_tree_node_t main_nodes_plus4[] = {
    { "Host",               "host",         NULL,   host_nodes_generic },
    { "Machine",            "machine",      NULL,   machine_nodes_plus4 },
    { "Display",            "display",      NULL,   display_nodes_plus4 },
    { "Audio",              "audio",        NULL,   audio_nodes_plus4 },
    { "Input devices",      "input",        NULL,   input_nodes_plus4 },
    { "Peripheral devices", "peripheral",   NULL,   peripheral_nodes_plus4 },
    { "Cartridges",         "cartridges",   NULL,   plus4_io_extensions },
    UI_SETTINGS_TERMINATOR
};
/* }}} */


/*****************************************************************************
 *                      PET tree nodes for the settings UI                   *
 ****************************************************************************/

/* {{{ machine_nodes_pet */
/** \brief  Child nodes for the PET 'Machine' node
 */
static ui_settings_tree_node_t machine_nodes_pet[] = {
    { "Model",
      "model",
      settings_model_widget_create, NULL },
    { "ROM",
      "rom",
      settings_rom_widget_create, NULL },
    { "RAM",
      "ram",
      settings_ramreset_widget_create, NULL },
    { "I/O settings",
      "io",
      settings_io_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ display_nodes_pet */
/** \brief  Child nodes for the PET 'Display' node
 */
static ui_settings_tree_node_t display_nodes_pet[] = {
    { "Host display",
      "host-display",
      settings_host_display_widget_create, NULL },
    { "CRTC",
      "crtc",
      settings_video_widget_create, NULL },
    { "CRT",
      "CRT",
      settings_crt_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ audio_nodes_pet */
/** \brief  Child nodes for the PET 'Audio' node
 */
static ui_settings_tree_node_t audio_nodes_pet[] = {
    { "Sound driver",
      "sound-driver",
      settings_sound_widget_create, NULL },
    { "SID",
      "sid",
      settings_soundchip_widget_create, NULL },
    { "Sampler",
      "sampler",
      settings_sampler_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ input_nodes_pet */
/** \brief  Child nodes for the PET 'Input devices' node
 */
static ui_settings_tree_node_t input_nodes_pet[] = {
    { "Keyboard",
      "keyboard",
      settings_keyboard_widget_create, NULL },
    { "Joystick",
      "joystick",
      settings_joystick_widget_create, NULL },
    { "Autofire",
      "autofire",
      settings_autofire_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ peripheral_nodes_pet */
/** \brief  Child nodes for the PET 'Peripheral devices' node
 */
static ui_settings_tree_node_t peripheral_nodes_pet[] = {
    { "Drive",
      "drive",
      settings_drive_widget_create, NULL },
    { "Host file system device",
      "fsdevice",
      settings_fsdevice_widget_create, NULL },
    { "Printer",
      "printer",
      settings_printer_widget_create, NULL },
    /* No RS232 for standard PET, SuperPET has ACIA though */
#if 0
#ifdef HAVE_RS232DEV
    { "RS232",
      "rs232",
      settings_rs232_widget_create, NULL },
#endif
#endif
    { "Userport devices",
      "userport-devices",
      settings_userport_widget_create, NULL },
    { "Tape port devices",
      "tapeport-devices",
      settings_tapeport_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ main_nodes_pet */
/** \brief  Main tree nodes for xpet
 */
static ui_settings_tree_node_t main_nodes_pet[] = {
    { "Host",               "host",             NULL,   host_nodes_generic },
    { "Machine",            "machine",          NULL,   machine_nodes_pet },
    { "Display",            "display",          NULL,   display_nodes_pet },
    { "Audio",              "audio",            NULL,   audio_nodes_pet },
    { "Input devices",      "input",            NULL,   input_nodes_pet },
    { "Peripheral devices", "peripheral",       NULL,   peripheral_nodes_pet },
    { "I/O extensions",     "io-extensions",    NULL,   pet_io_extensions },
    UI_SETTINGS_TERMINATOR
};
/* }}} */


/*****************************************************************************
 *                  CBM5x0 tree nodes for the settings UI                    *
 ****************************************************************************/

/* {{{ machine_nodes_cbm5x0 */
/** \brief  Child nodes for the CBM5x0 'Machine' node
 */
static ui_settings_tree_node_t machine_nodes_cbm5x0[] = {
    { "Model",
      "model",
      settings_model_widget_create, NULL },
    { "ROM",
      "rom",
      settings_rom_widget_create, NULL },
    { "RAM",
      "ram",
      settings_ramreset_widget_create, NULL },
    { "I/O settings",
      "io",
      settings_io_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ display_nodes_cbm5x0 */
/** \brief  Child nodes for the CBM5x0 'Display' node
 */
static ui_settings_tree_node_t display_nodes_cbm5x0[] = {
    { "Host display",
      "host-display",
      settings_host_display_widget_create, NULL },
    { "VIC-II",
      "vicii",
      settings_video_widget_create, NULL },
    { "CRT",
      "CRT",
      settings_crt_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ audio_nodes_cbm5x0 */
/** \brief  Child nodes for the CBM5x0 'Audio' node
 */
static ui_settings_tree_node_t audio_nodes_cbm5x0[] = {
    { "Sound driver",
      "sound-driver",
      settings_sound_widget_create, NULL },
    { "SID",
      "sid",
      settings_soundchip_widget_create, NULL },
    { "Sampler",
      "sampler",
      settings_sampler_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ input_nodes_cbm5x0 */
/** \brief  Child nodes for the CBM5x0 'Input devices' node
 */
static ui_settings_tree_node_t input_nodes_cbm5x0[] = {
    { "Keyboard",
      "keyboard",
      settings_keyboard_widget_create, NULL },
    { "Joystick",
      "joystick",
      settings_joystick_widget_create, NULL },
    { "Control port",
      "control-port",
      settings_controlport_widget_create, NULL },
    { "Autofire",
      "autofire",
      settings_autofire_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ peripheral_nodes_cbm5x0 */
/** \brief  Child nodes for the CBM5x0 'Peripheral devices' node
 */
static ui_settings_tree_node_t peripheral_nodes_cbm5x0[] = {
    { "Drive",
      "drive",
      settings_drive_widget_create, NULL },
    { "Host file system device",
      "fsdevice",
      settings_fsdevice_widget_create, NULL },
    { "Printer",
      "printer",
      settings_printer_widget_create, NULL },
#ifdef HAVE_RS232DEV
    { "RS232",
      "rs232",
      settings_rs232_widget_create, NULL },
#endif
    { "Tape port devices",
      "tapeport-devices",
      settings_tapeport_widget_create, NULL },

    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ main_nodes_cbm5x0 */
/** \brief  Main tree nodes for xcbm5x0
 */
static ui_settings_tree_node_t main_nodes_cbm5x0[] = {
    { "Host",               "host",         NULL,   host_nodes_generic },
    { "Machine",            "machine",      NULL,   machine_nodes_cbm5x0 },
    { "Display",            "display",      NULL,   display_nodes_cbm5x0 },
    { "Audio",              "audio",        NULL,   audio_nodes_cbm5x0 },
    { "Input devices",      "input",        NULL,   input_nodes_cbm5x0 },
    { "Peripheral devices", "peripheral",   NULL,   peripheral_nodes_cbm5x0 },
    { "Cartridges",         "cartridges",   NULL,   cbm2_io_extensions },
    UI_SETTINGS_TERMINATOR
};
/* }}} */


/*****************************************************************************
 *                  CBM-II tree nodes for the settings UI                    *
 ****************************************************************************/

/* {{{ machine_nodes_cbm6x0 */
/** \brief  Child nodes for the CBM6x0 'Machine' node
 */
static ui_settings_tree_node_t machine_nodes_cbm6x0[] = {
    { "Model",
      "model",
      settings_model_widget_create, NULL },
    { "ROM",
      "rom",
      settings_rom_widget_create, NULL },
    { "RAM",
      "ram",
      settings_ramreset_widget_create, NULL },
    { "I/O settings",
      "io",
      settings_io_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ display_nodes_cbm6x0 */
/** \brief  Child nodes for the CBM6x0 'Display' node
 */
static ui_settings_tree_node_t display_nodes_cbm6x0[]= {
    { "Host display",
      "host-display",
      settings_host_display_widget_create, NULL },
    { "CRTC",
      "crtc",
      settings_video_widget_create, NULL },
    { "CRT",
      "CRT",
      settings_crt_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ audio_nodes_cbm6x0 */
/** \brief  Child nodes for the CBM6x0 'Audio' node
 */
static ui_settings_tree_node_t audio_nodes_cbm6x0[] = {
    { "Sound driver",
      "sound-driver",
      settings_sound_widget_create, NULL },
    { "SID",
      "sid",
      settings_soundchip_widget_create, NULL },
    { "Sampler",
      "sampler",
      settings_sampler_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ input_nodes_cbm6x0 */
/** \brief  Child nodes for the CBM6x0 'Input devices' node
 */
static ui_settings_tree_node_t input_nodes_cbm6x0[] = {
    { "Keyboard",
      "keyboard",
      settings_keyboard_widget_create, NULL },
    { "Joystick",
      "joystick",
      settings_joystick_widget_create, NULL },
    { "Control port",
      "control-port",
      settings_controlport_widget_create, NULL },
    { "Autofire",
      "autofire",
      settings_autofire_widget_create, NULL },
    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ peripheral nodes_cbm6x0 */
/** \brief  Child nodes for the CBM6x0 'Peripheral devices' node
 */
static ui_settings_tree_node_t peripheral_nodes_cbm6x0[] = {
    { "Drive",
      "drive",
      settings_drive_widget_create, NULL },
    { "Host file system device",
      "fsdevice",
      settings_fsdevice_widget_create, NULL },
    { "Printer",
      "printer",
      settings_printer_widget_create, NULL },
#ifdef HAVE_RS232DEV
    { "RS232",
      "rs232",
      settings_rs232_widget_create, NULL },
#endif
    { "Userport devices",
      "userport-devices",
      settings_userport_widget_create, NULL },
    { "Tape port devices",
      "tapeport_devices",
      settings_tapeport_widget_create, NULL },

    UI_SETTINGS_TERMINATOR
};
/* }}} */

/* {{{ main_nodes_cbm6x0 */
/** \brief  Main tree nodes for xcbm6x0
 */
static ui_settings_tree_node_t main_nodes_cbm6x0[] = {
    { "Host",               "host",         NULL,   host_nodes_generic },
    { "Machine",            "machine",      NULL,   machine_nodes_cbm6x0 },
    { "Display",            "display",      NULL,   display_nodes_cbm6x0 },
    { "Audio",              "audio",        NULL,   audio_nodes_cbm6x0 },
    { "Input devices",      "input",        NULL,   input_nodes_cbm6x0 },
    { "Peripheral devices", "peripheral",   NULL,   peripheral_nodes_cbm6x0 },
    { "Cartridges",         "cartridges",   NULL,   cbm2_io_extensions },
    UI_SETTINGS_TERMINATOR
};
/* }}} */


/** \brief  Reference to the current 'central' widget in the settings dialog
 */
static void ui_settings_set_central_widget(GtkWidget *widget);

/** \brief  Old pause state when popping up the dialog
 *
 * Used for the PauseOnSettings resource: if true, exiting the dialog will set
 * the emulators pause state to this.
 */
static int settings_old_pause_state;

/** \brief  Reference to the settings dialog
 */
static GtkWidget *settings_window = NULL;

/** \brief  Previous X position of settings dialog
 */
static gint settings_xpos = INT_MIN;

/** \brief  Previous Y position of settings dialog
 */
static gint settings_ypos = INT_MIN;

/** \brief  Reference to the 'content area' widget of the settings dialog
 */
static GtkWidget *settings_grid = NULL;

/** \brief  Reference to the tree model for the settings tree
 */
static GtkTreeStore *settings_model = NULL;

/** \brief  Reference to the tree view for the settings tree
 */
static GtkWidget *settings_tree = NULL;

/** \brief  Scroll window container for the settings treeview
 */
static GtkWidget *scrolled_window = NULL;

/** \brief  Widget containing the treeview and the settings 'page'
 *
 * Allows resizing both the treeview and the setting with a 'grip'.
 */
static GtkWidget *paned_widget = NULL;

/** \brief  Path to the last used settings page
 */
static GtkTreePath *last_node_path = NULL;


/** \brief  Handler for the "destroy" event of the main dialog
 *
 * Stores the position of the dialog so it can be shown again at that
 * position.
 *
 * \param[in]   widget      main dialog
 * \param[in]   data        extra event data (unused)
 */
static void on_settings_dialog_destroy(GtkWidget *widget, gpointer data)
{
    settings_window = NULL;
    ui_action_finish(ACTION_SETTINGS_DIALOG);
}

/** \brief  Handler for the 'clicked' event of our own "Close" button
 *
 * \param[in]   widget  button
 * \param[in]   dialog  settings dialog
 */
static void on_close_clicked(GtkWidget *widget, gpointer dialog)
{
    g_signal_emit_by_name(G_OBJECT(dialog),
                          "response",
                          GTK_RESPONSE_DELETE_EVENT,
                          NULL);
}

/** \brief  Handler for the double click event of a tree node
 *
 * Expands or collapses the node and its children (if any)
 *
 * \param[in,out]   tree_view   tree view instance
 * \param[in]       path        tree view path
 * \param[in]       column      tree view column (unused)
 * \param[in]       user_data   extra event data (unused)
 */
static void on_row_activated(GtkTreeView *tree_view,
                            GtkTreePath *path,
                            GtkTreeViewColumn *column,
                            gpointer user_data)
{
    if (gtk_tree_view_row_expanded(tree_view, path)) {
        gtk_tree_view_collapse_row(tree_view, path);
    } else {
        /*
         * Only expand the immediate children. A no-op at the moment since
         * we only have two levels of nodes in the tree, but perhaps useful
         * for later.
         */
        gtk_tree_view_expand_row(tree_view, path, FALSE);
    }
}

/** \brief  Create the widget that is initially shown in the settings UI
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
static GtkWidget *ui_settings_inital_widget(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;

    grid = vice_gtk3_grid_new_spaced(64, 64);
    label = gtk_label_new(NULL);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_markup(GTK_LABEL(label),
            "This is the first widget/dialog shown when people click on the"
            " settings UI.\n"
            "So perhaps we could show some instructions or something here.");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Handler for the "changed" event of the tree view
 *
 * \param[in]   selection   GtkTreeSelection associated with the tree model
 * \param[in]   user_data   data for the event (unused for now)
 */
static void on_tree_selection_changed(GtkTreeSelection *selection,
                                      gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *name = NULL;
        gchar *parent_name = NULL;
        GtkWidget *(*callback)(void *) = NULL;
        const char *id;

        gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);
        gtk_tree_model_get(model, &iter, COLUMN_CALLBACK, &callback, -1);
        gtk_tree_model_get(model, &iter, COLUMN_ID, &id, -1);

        if (callback != NULL) {
            GtkTreeIter parent;
            char title[256];

            /* try to get parent's name */
            if (gtk_tree_model_iter_parent(model, &parent, &iter)) {
                gtk_tree_model_get(model, &parent, COLUMN_NAME, &parent_name, -1);
            }

            if (parent_name != NULL) {
                g_snprintf(title, sizeof title,
                           "%s settings :: %s :: %s",
                           machine_name, parent_name, name);
            } else {
                g_snprintf(title, sizeof title,
                           "%s settings :: %s",
                           machine_name, name);
            }
            gtk_window_set_title(GTK_WINDOW(settings_window), title);

            /* create new central widget, using settings_window (this dialog)
             * as its parent, this will allow for proper blocking in modal
             * dialogs, while ui_get_active_window() breaks that. */
            if (last_node_path != NULL) {
                gtk_tree_path_free(last_node_path);
            }
            last_node_path = gtk_tree_model_get_path(
                    GTK_TREE_MODEL(settings_model), &iter);
            ui_settings_set_central_widget(callback(settings_window));
        }
        if (name != NULL) {
            g_free(name);
        }
        if (parent_name != NULL) {
            g_free(parent_name);
        }
    }
}

/** \brief  Create the 'Save on exit' checkbox
 *
 * The current position/display of the checkbox is a little lame at the moment
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_save_on_exit_checkbox(void)
{
    return vice_gtk3_resource_check_button_new("SaveResourcesOnExit",
            "Save settings on exit");
}

/** \brief  Create the 'Confirm on exit' checkbox
 *
 * The current position/display of the checkbox is a little lame at the moment
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_confirm_on_exit_checkbox(void)
{
    return vice_gtk3_resource_check_button_new("ConfirmOnExit",
            "Confirm on exit");
}

/** \brief  Create the 'Pause on settings dialog' checkbox
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_pause_on_settings_checkbox(void)
{
    return vice_gtk3_resource_check_button_new(
            "PauseOnSettings",
            "Pause when showing settings");
}

/** \brief  Create empty tree model for the settings tree
 */
static void create_tree_model(void)
{
    settings_model = gtk_tree_store_new(NUM_COLUMNS,
            G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
}

/** \brief  Create tree store containing settings items and children
 *
 * \return  GtkTreeStore
 */
static GtkTreeStore *populate_tree_model(void)
{
    GtkTreeStore *model;
    GtkTreeIter iter;
    GtkTreeIter child;
    ui_settings_tree_node_t *nodes = NULL;
    int i;

    model = settings_model;

    switch (machine_class) {
        case VICE_MACHINE_C64:  /* fall through */
        case VICE_MACHINE_C64SC:
            nodes = main_nodes_c64;
            break;
        case VICE_MACHINE_C64DTV:
            nodes = main_nodes_c64dtv;
            break;
        case VICE_MACHINE_C128:
            nodes = main_nodes_c128;
            break;
        case VICE_MACHINE_SCPU64:
            nodes = main_nodes_scpu64;
            break;
        case VICE_MACHINE_VIC20:
            nodes = main_nodes_vic20;
            break;
        case VICE_MACHINE_PLUS4:
            nodes = main_nodes_plus4;
            break;
        case VICE_MACHINE_PET:
            nodes = main_nodes_pet;
            break;
        case VICE_MACHINE_CBM5x0:
            nodes = main_nodes_cbm5x0;
            break;
        case VICE_MACHINE_CBM6x0:
            nodes = main_nodes_cbm6x0;
            break;
        case VICE_MACHINE_VSID:
            nodes = main_nodes_vsid;
            break;
        default:
            log_error(LOG_DEFAULT,
                      "Error: %s:%d:%s(): unsupported machine_class %d\n",
                      __FILE__, __LINE__, __func__, machine_class);
            archdep_vice_exit(1);
            break;
    }

    for (i = 0; nodes[i].name != NULL; i++) {
        gtk_tree_store_append(model, &iter, NULL);
        gtk_tree_store_set(model, &iter,
                COLUMN_NAME, nodes[i].name,
                COLUMN_ID, nodes[i].id,
                COLUMN_CALLBACK, nodes[i].callback,
                -1);

        /* this bit will need proper recursion if we need more than two
         * levels of subitems */
        if (nodes[i].children != NULL) {
            int c;
            ui_settings_tree_node_t *list = nodes[i].children;

            for (c = 0; list[c].name != NULL; c++) {
                char buffer[256];

                g_snprintf(buffer, 256, "%s", list[c].name);
                gtk_tree_store_append(model, &child, &iter);
                gtk_tree_store_set(model, &child,
                        COLUMN_NAME, buffer,
                        COLUMN_ID, list[c].id,
                        COLUMN_CALLBACK, list[c].callback,
                        -1);
            }
        }
    }
    return model;
}

/** \brief  Determine if the current tree item should be a separator
 *
 * Callback function for the tree view to check item at \a iter in \a model
 * and decide if the item is a separator.
 *
 * \param[in]   model   tree model
 * \param[in]   iter    tree iterator
 * \param[in]   data    extra event data (unused)
 *
 * \return  TRUE if the item is a separator
 */
static gboolean row_separator_func(GtkTreeModel *model,
                                   GtkTreeIter *iter,
                                   gpointer data)
{
    gchar *name = NULL;
    gboolean is_sep = FALSE;

    gtk_tree_model_get(model, iter, COLUMN_NAME, &name, -1);
    if (name != NULL && *name == '-') {
        is_sep = TRUE;
    }
    g_free(name);

    return is_sep;
}

/** \brief  Create treeview for settings side-menu
 *
 * Reads items from `main_nodes` and adds them to the tree view.
 *
 * \return  GtkTreeView
 *
 * TODO:    Handle nested items, and write up somewhere how the hell I finally
 *          got the callbacks working.
 *          Split into a function creating the tree view and functions adding,
 *          altering or removing nodes.
 */
static GtkWidget *create_treeview(void)
{
    GtkWidget *tree;
    GtkCellRenderer *text_renderer;
    GtkTreeViewColumn *text_column;

    create_tree_model();
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(populate_tree_model()));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
    gtk_tree_view_set_row_separator_func(GTK_TREE_VIEW(tree),
                                         row_separator_func,
                                         NULL,
                                         NULL);

    text_renderer = gtk_cell_renderer_text_new();
    text_column = gtk_tree_view_column_new_with_attributes(
            "item-name",
            text_renderer,
            "text", 0,
            NULL);
    /*    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), obj_column); */
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), text_column);

    /* disable search popup when typing */
    g_object_set(G_OBJECT(tree), "enable-search", FALSE, NULL);

    /* apply CSS for keyboard navigation */
    vice_gtk3_css_add(tree, TREEVIEW_CSS);
    return tree;
}

/** \brief  Set the 'central'/action widget for the settings dialog
 *
 * Destroys the old 'central' widget and sets the new one.
 *
 *  \param[in,out]  widget  widget to use as the new 'central' widget
 */
static void ui_settings_set_central_widget(GtkWidget *widget)
{
    GtkWidget *child;

    child = gtk_paned_get_child2(GTK_PANED(paned_widget));
    if (child != NULL) {
        gtk_widget_destroy(child);
    }
    gtk_paned_pack2(GTK_PANED(paned_widget), widget, TRUE, FALSE);
    /* add a little space around the widget */
    gtk_widget_set_margin_top(widget, 8);
    gtk_widget_set_margin_start(widget, 8);
    gtk_widget_set_margin_end(widget, 8);
    gtk_widget_set_margin_bottom(widget, 8);
}

/** \brief  Create the 'content widget' of the settings dialog
 *
 * This creates the widget in the dialog used to display the treeview and room
 * for the widget connected to that tree's currently selected item.
 *
 * \param[in]   dialog  settings dialog
 *
 * \return  GtkGrid
 */
static GtkWidget *create_content_widget(GtkWidget *dialog)
{
    GtkTreeSelection *selection;
    GtkWidget        *extra;
    GtkWidget        *button_box;
    GtkWidget        *close_button;

    settings_grid = gtk_grid_new();
    settings_tree = create_treeview();

    /* pack the tree in a scrolled window to allow scrolling of the tree when
     * it gets too large for the dialog
     */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), settings_tree);
    gtk_widget_set_hexpand(scrolled_window, TRUE);

    /* pack the tree and the settings 'page' into a GtkPaned so we can resize
     * the tree */
    paned_widget = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(paned_widget, TRUE);
    gtk_paned_set_wide_handle(GTK_PANED(paned_widget), TRUE);
    gtk_paned_pack1(GTK_PANED(paned_widget), scrolled_window, FALSE, FALSE);
    gtk_grid_attach(GTK_GRID(settings_grid), paned_widget, 0, 0, 1, 1);

    /* Remember the previously selected setting/widget and set it here */

    /* do we have a previous settings "page"? */
    if (last_node_path == NULL) {
        /* nope, display the default one */
        ui_settings_set_central_widget(ui_settings_inital_widget(dialog));
    } else {
        /* try to restore the page last shown */
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(settings_model), &iter,
                    last_node_path)) {
            GtkWidget *(*callback)(GtkWidget *) = NULL;
            gtk_tree_model_get(
                    GTK_TREE_MODEL(settings_model), &iter,
                    COLUMN_CALLBACK, &callback, -1);
            if (callback != NULL) {

                selection = gtk_tree_view_get_selection(
                        GTK_TREE_VIEW(settings_tree));

                ui_settings_set_central_widget(callback(dialog));
                gtk_tree_view_expand_to_path(
                        GTK_TREE_VIEW(settings_tree),
                        last_node_path);
                gtk_tree_selection_select_path(selection, last_node_path);

            }
        }
    }

    /* create container for generic settings and close button */
    extra = gtk_grid_new();
    gtk_widget_set_hexpand(extra, TRUE);
    gtk_widget_set_margin_top(extra, 8);

    gtk_grid_attach(GTK_GRID(extra),
                    create_save_on_exit_checkbox(),
                    0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(extra),
                    create_confirm_on_exit_checkbox(),
                    0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(extra),
                    create_pause_on_settings_checkbox(),
                    0, 2, 1, 1);

    /* We add our own custom "Close" button here so we can pack the check
     * buttons and the Close button on the same vertical space. We're not
     * allowed to touch the dialog's GtkButtonBox that contains a dialog's
     * buttons, so we create our own here.
     *
     * Using a GtkButtonBox makes the buttons appear more "natural", ie they
     * get some extra width as is default with dialog buttons. And we can add
     * new buttons, should we wish, which will by default be homogeneous in
     * size.
     */
    button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(button_box, FALSE);
    gtk_widget_set_hexpand(button_box, TRUE);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    gtk_widget_set_valign(button_box, GTK_ALIGN_END);

    /* Alt+C closes the dialog, although in Gnome the accelerator isn't
     * visible until pressing Alt */
    close_button = gtk_button_new_with_mnemonic("_Close");
    gtk_box_pack_end(GTK_BOX(button_box), close_button, FALSE, FALSE, 0);

    gtk_grid_attach(GTK_GRID(extra), button_box, 1, 0, 1, 3);

    /* add to main layout */
    gtk_grid_attach(GTK_GRID(settings_grid), extra, 0, 2, 2, 1);

    gtk_widget_show_all(settings_grid);
    gtk_widget_show_all(settings_tree);
    gtk_widget_show_all(extra);

    gtk_widget_set_size_request(scrolled_window, 270, -1);
    gtk_widget_set_size_request(settings_grid, DIALOG_WIDTH, DIALOG_HEIGHT);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(settings_tree));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect_unlocked(G_OBJECT(selection),
                              "changed",
                              G_CALLBACK(on_tree_selection_changed),
                              NULL);
    /* handler for the double click event on a node */
    g_signal_connect_unlocked(settings_tree,
                              "row-activated",
                              G_CALLBACK(on_row_activated),
                              NULL);
    /* our own Close button: emits the GtkDialog::response event on he dialog
     * with a GTK_REPONSE_DELETE_EVENT argument. */
    g_signal_connect_unlocked(close_button,
                              "clicked",
                              G_CALLBACK(on_close_clicked),
                              (gpointer)dialog);

    return settings_grid;
}

/** \brief  Handler for the 'response' event of the settings dialog
 *
 * This determines what to do based on the 'reponse ID' emitted by the dialog.
 *
 * \param[in,out]   widget      settings dialog
 * \param[in]       response_id response ID
 * \param[in]       user_data   extra data (unused)
 */
static void response_callback(GtkWidget *widget,
                              gint       response_id,
                              gpointer   user_data)
{
    if (response_id == GTK_RESPONSE_DELETE_EVENT) {
        /* close dialog */
        int pause_on_settings = 0;

        gtk_widget_destroy(widget);
        settings_window = NULL;

        resources_get_int("PauseOnSettings", &pause_on_settings);
        if (pause_on_settings) {
            if (settings_old_pause_state) {
                ui_pause_enable();
            } else {
                ui_pause_disable();
            }
        }
    }
}

/** \brief  Respond to window size changes
 *
 * This allows for quickly seeing if specific dialog is getting too large.
 * It's also used to store the dialog position so it can be restored when
 * respawning.
 *
 * \param[in]   widget  a GtkWindow
 * \param[in]   event   the GDK event
 * \param[in]   data    extra event data (unused)
 *
 * \return  boolean
 */
static gboolean on_dialog_configure_event(GtkWidget *widget,
                                          GdkEvent *event,
                                          gpointer data)
{
    if (event->type == GDK_CONFIGURE) {
        GdkEventConfigure *cfg = (GdkEventConfigure *)event;
#ifdef HAVE_DEBUG_GTK3UI
        int max_width = DIALOG_WIDTH + DIALOG_WIDTH_TOLERANCE;
        int max_height = DIALOG_HEIGHT + DIALOG_HEIGHT_TOLERANCE;
#endif
        /* Update dialog position, using gtk_window_get_position() doesn't
         * work, it reports the position of the dialog when it was spawned,
         * not the position if it has been moved afterwards. */
        settings_xpos = cfg->x;
        settings_ypos = cfg->y;

        /* check dialog size */
        debug_gtk3("New dialog size is %dx%d (requested %dx%d).",
                   cfg->width, cfg->height, DIALOG_WIDTH, DIALOG_HEIGHT);
#ifdef HAVE_DEBUG_GTK3UI
        if (cfg->width > max_width || cfg->height > max_height) {
            debug_gtk3("New dialog size of %dx%d exceeds tolerance of %dx%d.",
                       cfg->width, cfg->height, max_width, max_height);
            gtk_window_set_title(GTK_WINDOW(widget),
                                 "GODSAMME, M'N DING IS TE GROOT!");
        }
#endif
    }
    return FALSE;
}

/** \brief  Dialog create helper
 *
 * Create the GtkDialog and set it up, connecting signal handlers.
 *
 * \return  Settings dialog
 */
static GtkWidget *dialog_create_helper(void)
{
    GtkWidget *dialog;
    GtkWidget *content;
    gchar      title[256];

    g_snprintf(title, sizeof title, "%s Settings", machine_name);

    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), ui_get_active_window());
    /* XXX: maybe we can set this to FALSE once all dialog nodes fit the
     *      initial size: */
    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);

    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    g_object_set(content, "border-width", 8, NULL);
    gtk_container_add(GTK_CONTAINER(content), create_content_widget(dialog));

    gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                    GTK_RESPONSE_DELETE_EVENT);

    g_signal_connect_unlocked(dialog,
                              "response",
                              G_CALLBACK(response_callback),
                              NULL);
    g_signal_connect_unlocked(dialog,
                              "configure-event",
                              G_CALLBACK(on_dialog_configure_event),
                              NULL);
    g_signal_connect_unlocked(dialog,
                              "destroy",
                              G_CALLBACK(on_settings_dialog_destroy),
                              NULL);
    return dialog;
}

/** \brief  Find and activate node in the tree view via \a path
 *
 * The \a path argument is expected to be in the form 'foo/bar/bah', each
 * path item indicates a node in the tree view/model. For example:
 * "display/vdc" would select the VDC settings dialog on x128, but would fail
 * on any other machine.
 *
 * \param[in]   path    path to the node
 *
 * \return  bool
 */
static gboolean ui_settings_dialog_activate_node(const char *path)
{
    GtkTreeIter iter;
    gchar **parts;
    const gchar *part;
    int column = 0;

    if (settings_window == NULL) {
        log_error(LOG_DEFAULT, "settings dialog node activation requested without"
                " the dialog active.");
        return FALSE;
    }
    if (path == NULL || *path == '\0') {
        log_error(LOG_DEFAULT, "NULL or empty path passed.");
        return FALSE;
    }

    /* split path into parts */
    parts = g_strsplit(path, "/", 0);
    part = parts[0];

    /* get first item in model */
    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(settings_model), &iter);

    /* iterate the parts of the path, trying to find to requested node */
    while (part != NULL) {

        const gchar *node_id = NULL;

        /* iterate nodes until either 'part' is found or the nodes in the
         * current 'column' run out */
        while (TRUE) {
            gtk_tree_model_get(GTK_TREE_MODEL(settings_model), &iter,
                    COLUMN_ID, &node_id, -1);

            /* check node ID against currently sought part of the path */
            if (strcmp(node_id, part) == 0) {
                /* got the requested node */
                if (parts[column + 1] == NULL) {
                    /* got final item */
                    GtkTreeSelection *selection;
                    GtkTreePath *tree_path;

                    selection = gtk_tree_view_get_selection(
                            GTK_TREE_VIEW(settings_tree));
                    tree_path = gtk_tree_model_get_path(
                            GTK_TREE_MODEL(settings_model), &iter);
                    gtk_tree_view_expand_to_path(
                            GTK_TREE_VIEW(settings_tree), tree_path);
                    gtk_tree_selection_select_path(selection, tree_path);

                    gtk_tree_path_free(tree_path);
                    g_strfreev(parts);
                    return TRUE;
                } else {
                    /* continue searching, dive into the children of the
                     * current node, if there are any */
                    if (gtk_tree_model_iter_has_child(
                                GTK_TREE_MODEL(settings_model), &iter)) {
                        /* node has children, iterate those now */
                        GtkTreeIter child;

                        if (!gtk_tree_model_iter_nth_child(
                                    GTK_TREE_MODEL(settings_model),
                                    &child, &iter, 0)) {
                            g_strfreev(parts);
                            return FALSE;
                        }
                        /* set iterator to first child node, update the index
                         * in the path parts */
                        iter = child;
                        part = parts[++column];
                        continue;
                    } else {
                        /* oops */
                        debug_gtk3("error: path '%s' continues into '%s' but"
                                " there are no child nodes.", path, part);
                        g_strfreev(parts);
                        return FALSE;
                    }
                }
            } else {
                /* is there another node to inspect? */
                if (!gtk_tree_model_iter_next(GTK_TREE_MODEL(settings_model),
                                              &iter)) {
                    /* couldn't find the requested node, exit */
                    debug_gtk3("failed to find node at path '%s'.", path);
                    g_strfreev(parts);
                    return FALSE;
                }
            }
        }
    }

    debug_gtk3("warning: should never get here.");
    g_strfreev(parts);
    return FALSE;
}

/** \brief  Threaded UI handler for the settings dialog constructor
 *
 * \param[in]   user_data   path to active node in the treeview
 *
 * \return  FALSE
 */
static gboolean ui_settings_dialog_show_impl(gpointer user_data)
{
    const char *path = (const char *)user_data;
    GtkWidget *dialog;

    dialog = dialog_create_helper();
    settings_window = dialog;

    /* find and activate the node */
    if (path && !ui_settings_dialog_activate_node(path)) {
        debug_gtk3("failed to locate node, showing dialog anyway for now.");
    }

    gtk_widget_show_all(dialog);
        /* XXX: Doesn't work on Wayland, which appears to be a bug since at least
     *      2014 :)
     */
    if (settings_xpos > INT_MIN && settings_ypos > INT_MIN) {
        /* restore previous position, if any */
        debug_gtk3("Restoring previous position: (%d,%d).",
                   settings_xpos, settings_ypos);
        gtk_window_move(GTK_WINDOW(dialog), settings_xpos, settings_ypos);
    }

    return FALSE;
}


/** \brief  Menu callback for the settings dialog
 *
 * Opens the main settings dialog and activates a node, if any.
 *
 * \param[in]   path   path to node
 *
 * \return  TRUE
 */
void ui_settings_dialog_show(const char *path)
{
    int pause_on_settings = 0;

    settings_old_pause_state = ui_pause_active();

    resources_get_int("PauseOnSettings", &pause_on_settings);
    if (pause_on_settings) {
        ui_pause_enable();
    }

    /* call from ui thread without locking - creating the settings dialog is heavy */
    gdk_threads_add_timeout(0, ui_settings_dialog_show_impl, (gpointer)path);
}


/** \brief  Clean up resources used on emu exit
 *
 * This function cleans up the data used to present the user with the last used
 * settings page.
 *
 * \note    Do NOT call this when exiting the settings UI, the event handlers
 *          will take care of cleaning up resources used by the UI.
 */
void ui_settings_shutdown(void)
{
    if (last_node_path != NULL) {
        gtk_tree_path_free(last_node_path);
        last_node_path = NULL;
    }
}
