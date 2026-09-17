/** \file   filechooserhelpers.c
 * \brief   GtkFileChooser helper functions
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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

#include "vice.h"

#include <gtk/gtk.h>

#include "lib.h"
#include "log.h"
#include "util.h"

#include "filechooserhelpers.h"


/*
 * 'Stock' file patterns
 *
 * These look ridiciulous, but Gtk only allows some basic globbing-like patterns,
 * not proper regexes.
 */

/** \brief  Patterns for all files
 */
const char *file_chooser_pattern_all[] = {
    "*", NULL
};


/** \brief  Patterns for cartridge images
 */
const char *file_chooser_pattern_cart[] = {
    "*.[cC][rR][tT]", "*.[bB][iI]nN]", NULL
};

/** \brief  Patterns for disk images
 */
const char *file_chooser_pattern_disk[] = {
    "*.[dD]64",     "*.[dD]67",     "*.[dD]71",     "*.[dD]8[0-2]",
    "*.[dD]1[mM]",  "*.[dD]2[mM]",  "*.[dD]4[mM]",  "*.[dD][hH][dD]",
    "*.[gG]64",     "*.[gG]71",     "*.[gG]41",     "*.[pP]64",
#ifdef HAVE_X64_IMAGE
    "*.[xX]64",
#endif
    NULL
};


/** \brief  Patterns for disk images (non-GCR floppies only)
 *
 * Used in the AutostartPrgDiskImage widget
 */
const char *file_chooser_pattern_floppy[] = {
    "*.[dD]64",     "*.[dD]67",     "*.[dD]71",     "*.[dD]8[0-2]",
    NULL
};



/** \brief  Patterns for tapes
 *
 * T64 is NOT a tape, so probably should be moved to a 'archive' pattern group,
 * together with ZipCode, Lynx, Ark, etc.
 */
const char *file_chooser_pattern_tape[] = {
    "*.[tT]64", "*.[tT][aA][pP]", NULL
};

/** \brief  Patterns for fliplists
 */
const char *file_chooser_pattern_fliplist[] = {
    "*.[vV][fF][lL]", "*.[lL][sS][tT]", NULL
};

/** \brief  Patterns for playlists
 */
const char *file_chooser_pattern_playlist[] = {
    "*.[mM]3[uU]", "*.[mM]3[uU]8", NULL
};

/** \brief  Patterns for program files
 */
const char *file_chooser_pattern_program[] = {
    "*.[pP][rR][gG]", "*.[pP][0-9][0-9]", NULL
};


/** \brief  Patterns for PSID/SID/MUS files
 */
const char *file_chooser_pattern_sid[] = {
    "*.[sD][iI][dD]",
    "*.[pP][sD][iI][dD]",
    "*.[mM][uU][sS]",
    NULL
};

/** \brief  C64 native archives
 *
 * Not all of these are supported, Lynx and ZipCoded disks are supported through
 * calling c1541.
 */
const char *file_chooser_pattern_archive[] = {
    "*.[aA][rR][kK]",    /* ARK archive */
    "*.[lL][nN][xX]",    /* Lynx archive */
    "[1-4]1*",  /* ZipCode disk */
    "[1-6]!!*", /* ZipSix */
    "[a-z]!*",  /* ZipFile*/
    NULL
};


/** \brief  Patterns for host-compressed files
 *
 * XXX: Once we have libarchive implemented, we could probably query libarchive
 *      for the extensions supported
 */
const char *file_chooser_pattern_compressed[] = {
    "*7[zZ]", "*.[bB][zZ]2", "*.[gG][zZ]", ".[rR][aA][rR]",
    "*.[zZ]", "*.[zZ][iI][pP]", NULL
};


/** \brief  Patterns for hotkeys files */
const char *file_chooser_pattern_hotkeys[] = {
    "*.vhk", NULL
};


/** \brief  Patterns for snapshot files
 */
const char *file_chooser_pattern_snapshot [] = {
    "*.[vV][sS][fF]", NULL };


/*
 * 'Stock' filters, for convenience
 */

/** \brief  Filter for all files */
const ui_file_filter_t file_chooser_filter_all = {
    "All files",
    file_chooser_pattern_all
};

/** \brief  Filter for cartridge images */
const ui_file_filter_t file_chooser_filter_cart = {
    "Cartridge images",
    file_chooser_pattern_cart
};

/** \brief  Filter for disk images */
const ui_file_filter_t file_chooser_filter_disk = {
    "Disk images",
    file_chooser_pattern_disk
};

/** \brief  Filter for tape images */
const ui_file_filter_t file_chooser_filter_tape = {
    "Tape images",
    file_chooser_pattern_tape
};

/** \brief  Filter for SID files */
const ui_file_filter_t file_chooser_filter_sid = {
    "PSID/SID files",
    file_chooser_pattern_sid
};

/** \brief  Filter for fliplist files */
const ui_file_filter_t file_chooser_filter_fliplist = {
    "Flip lists",
    file_chooser_pattern_fliplist
};

/** \brief  Filter for playlist files */
const ui_file_filter_t file_chooser_filter_playlist = {
    "Playlists",
    file_chooser_pattern_playlist
};

/** \brief  Filter for program files */
const ui_file_filter_t file_chooser_filter_program = {
    "Program files",
    file_chooser_pattern_program
};

/** \brief  Filter for archives */
const ui_file_filter_t file_chooser_filter_archive = {
    "Archive files",
    file_chooser_pattern_archive
};

/** \brief  Filter for compressed files */
const ui_file_filter_t file_chooser_filter_compressed = {
    "Compressed files",
    file_chooser_pattern_compressed
};

/** \brief  Filter for snapshot files */
const ui_file_filter_t file_chooser_filter_snapshot = {
    "Snapshot files",
    file_chooser_pattern_snapshot
};

/** \brief  Filter for hotkeys files */
const ui_file_filter_t file_chooser_filter_hotkeys = {
    "Hotkeys files",
    file_chooser_pattern_hotkeys
};

/** \brief  Create a GtkFileFilter instance from \a filter
 *
 * \param[in]   filter      name and patterns for the filter
 * \param[in]   show_globs  show file globbing pattern in the filter description
 *
 * Example:
 * \code{.c}
 *  const ui_file_filter_t data = {
 *      "disk image",
 *      { "*.d64", "*.d71", "*.d8[0-2]", "*.dhd", NULL }
 *  };
 *  GtkFileFilter *filter = create_file_chooser_filter(data);
 * \endcode
 *
 * \return  GtkFileFilter
 */
GtkFileFilter *create_file_chooser_filter(const ui_file_filter_t filter,
                                          gboolean show_globs)
{
    GtkFileFilter *ff;
    size_t i;
    char *globs;
    char *name;

    if (show_globs) {
        globs = util_strjoin(filter.patterns, ";");
        name = util_concat(filter.name, " (", globs, ")", NULL);
        lib_free(globs);
    } else {
        name = lib_strdup(filter.name);
    }

    ff = gtk_file_filter_new();
    gtk_file_filter_set_name(ff, name);
    for (i = 0; filter.patterns[i] != NULL; i++) {
        gtk_file_filter_add_pattern(ff, filter.patterns[i]);
    }

    /* gtk_file_filter_set_name() makes a copy, so we can free name here
     * (according to the Git repo on 2017-09-09)
     */
    lib_free(name);
    return ff;
}


/** \brief  Convert UTF-8 encoded string \a text to the current locale
 *
 * \param[in]   text    UTF-8 encoded string
 *
 * \return  \a text encoded to the locale, or the original string on failure
 *
 * \note    the result must be freed after use with g_free()
 */
gchar *file_chooser_convert_to_locale(const gchar *text)
{
    GError *err = NULL;
    gsize br;
    gsize bw;
    gchar *result;

    result = g_locale_from_utf8(text, -1, &br, &bw, &err);
    if (result == NULL) {
        log_warning(LOG_DEFAULT,
                "warning: failed to convert string to locale: %s",
                err->message);
        result = g_strdup(text);
        if (err != NULL) {
            g_error_free(err);
        }
    }
    return result;
}


/** \brief  Convert locale encoded string \a text to UTF-8
 *
 * \param[in]   text    string in the current locale
 *
 * \return  \a text encoded to UTF-8, or the original string on failure
 *
 * \note    the result must be freed after use with g_free()
 */
gchar *file_chooser_convert_from_locale(const gchar *text)
{
    GError *err = NULL;
    gsize br;
    gsize bw;
    gchar *result;

    result = g_locale_to_utf8(text, -1, &br, &bw, &err);
    if (result == NULL) {
        log_warning(LOG_DEFAULT,
                "warning: failed to convert string to UTF-8: %s",
                err->message);
        result = g_strdup(text);
        if (err != NULL) {
            g_error_free(err);
        }
    }
    return result;
}
