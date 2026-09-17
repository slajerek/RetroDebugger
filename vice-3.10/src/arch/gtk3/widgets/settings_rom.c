/** \file   settings_rom.c
 * \brief   Settings dialog to manage ROMs
 *
 * Presents a GtkListBox with expandable rows for machine, drive and drive
 * expansion ROMs, using a resource file chooser widget for each ROM.
 * Drag-n-drop should also work, though I've only tested that on my Debian
 * box with Gnome so far.
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

/* Resources manipulated in this file:
 *
 * $VICERES Basic64Name             x128
 * $VICERES BasicHiName             x128
 * $VICERES BasicLoName             x128
 * $VICERES BasicName               -vsid
 * $VICERES ChargenCHName           x128
 * $VICERES ChargenDEName           x128
 * $VICERES ChargenFIName           x128
 * $VICERES ChargenFIName           x128
 * $VICERES ChargenFRName           x128
 * $VICERES ChargenITName           x128
 * $VICERES ChargenIntName          x128
 * $VICERES ChargenNOName           x128
 * $VICERES ChargenName             -xplus4 -vsid
 * $VICERES ChargenSEName           x128
 * $VICERES DosName1540             x64 x64sc x64dtv xscpu64 x128 xplus4 xvic
 * $VICERES DosName1541             x64 x64sc x64dtv xscpu64 x128 xplus4 xvic
 * $VICERES DosName1541ii           x64 x64sc x64dtv xscpu64 x128 xplus4 xvic
 * $VICERES DosName1551             xplus4
 * $VICERES DosName1570             x64 x64sc x64dtv xscpu64 x128 xplus4 xvic
 * $VICERES DosName1571             x64 x64sc x64dtv xscpu64 x128 xplus4 xvic
 * $VICERES DosName1571cr           x128
 * $VICERES DosName1581             x64 x64sc x64dtv xscpu64 x128 xplus4 xvic
 * $VICERES DosName2031             -vsid
 * $VICERES DosName2040             -vsid
 * $VICERES DosName3040             -vsid
 * $VICERES DosName4040             -vsid
 * $VICERES DosName9000             -vsid
 * $VICERES DosNameCMDHD            x64 x64sc x64dtv xscpu64 x128 xplus4 xvic
 * $VICERES DosName1001             -vsid
 * $VICERES DosName2000             x64 x64sc x64dtv xscpu64 x128 xplus4 xvic
 * $VICERES DosName4000             x64 x64sc x64dtv xscpu64 x128 xplus4 xvic
 * $VICERES DriveProfDOS1571Name    x64 x64sc x64dtv xscpu64 x128
 * $VICERES DriveStarDosName        x64 x64sc x64dtv xscpu64 x128
 * $VICERES DriveSuperCardName      x64 x64sc x64dtv xscpu64 x128
 * $VICERES Basic1                  xpet
 * $VICERES EditorName              xpet
 * $VICERES FunctionHighName        xplus4
 * $VICERES FunctionLowName         xplus4
 * $VICERES H6809RomAName           xpet
 * $VICERES H6809RomBName           xpet
 * $VICERES H6809RomCName           xpet
 * $VICERES H6809RomDName           xpet
 * $VICERES H6809RomEName           xpet
 * $VICERES H6809RomFName           xpet
 * $VICERES Kernal64Name            x128
 * $VICERES KernalCHName            x128
 * $VICERES KernalDEName            x128
 * $VICERES KernalFIName            x128
 * $VICERES KernalFRName            x128
 * $VICERES KernalITName            x128
 * $VICERES KernalIntName           x128
 * $VICERES KernalNOName            x128
 * $VICERES KernalName              -vsid
 * $VICERES KernalSEName            x128
 * $VICERES RomModule9Name          xpet
 * $VICERES RomModuleAName          xpet
 * $VICERES RomModuleBName          xpet
 * $VICERES c1hiName                xplus4
 * $VICERES c1loName                xplus4
 * $VICERES c2hiName                xplus4
 * $VICERES c2loName                xplus4
 * $VICERES SCPU64Name              xscpu64
 */

#include <gtk/gtk.h>
#include <stdbool.h>

#include "debug_gtk3.h"
#include "drive.h"
#include "lastdir.h"
#include "lib.h"
#include "petrom.h"
#include "resources.h"
#include "resourcewidgetmediator.h"
#include "romset.h"
#include "uiapi.h"
#include "vice_gtk3.h"

#include "settings_rom.h"


/** \brief  CSS for the GtkListBoxes used for the ROMs
 *
 * We use the theme background color to avoid ugly white (light theme) or
 * black areas (dark theme) areas around the widgets and in the unused space
 * of the GtkScrolledWindow.
 */
#define LISTBOX_CSS "list { background-color: @theme_bg_color; }"


/** \brief  Array length helper
 *
 * \param[in]   arr array
 *
 * \return  length of \a arr in number of elements
 */
#define ARRAY_LEN(arr)  (sizeof arr / sizeof arr[0])

/** \brief  Machine ROM information */
typedef struct machine_rom_s {
    const char   *name;     /**< ROM name for the UI */
    const char   *resource; /**< resource name for the ROM file */
} machine_rom_t;

/** \brief  Drive ROM information */
typedef struct drive_rom_s {
    int         type;       /**< drive type */
    const char *name;       /**< drive name for the UI */
    const char *resource;   /**< resource name for the ROM file */
} drive_rom_t;

/** \brief  Drive expansion ROM information */
typedef struct drive_exp_rom_s {
    const char   *name;     /**< ROM name for the UI */
    const char   *resource; /**< resource name for the ROM file */
} drive_exp_rom_t;


/* Forward declarations */
static const char **string_list_append_list(const char **list1, const char **list2);
static const char **get_all_resource_names(void);
static const char **expandable_list_get_resources(GtkWidget *list);
static void         expandable_list_sync_resources(GtkWidget *list);
static GtkWidget   *expandable_list_get_chooser_by_resource(GtkWidget  *list,
                                                            const char *resource);


/** \brief  List of machine ROM names and resources */
static const machine_rom_t machine_rom_list[] = {
    /* Kernals */

    /* C64, C64DTV, VIC20, Plus/4, PET, CBM II */
    { "Kernal",                 "KernalName" },
    /* C128 kernals in UI description order */
    { "International kernal",   "KernalIntName" },
    { "Finish kernal",          "KernalFIName" },
    { "French kernal",          "KernalFRName" },
    { "German kernal",          "KernalDEName" },
    { "Italian kernal",         "KernalITName" },
    { "Norwegian kernal",       "KernalNOName" },
    { "Swedish kernal",         "KernalSEName" },
    { "Swiss kernal",           "KernalCHName" },
    { "C64 mode kernal",        "Kernal64Name" },
    /* SCPU64 */
    { "SCPU64",                 "SCPU64Name" },

    /* Basics */

    /* C64, C64DTV, VIC20, Plus/4. PET, CBM II */
    { "Basic",                  "BasicName" },
    /* C128 */
    { "Basic low",              "BasicLoName" },
    { "Basic high",             "BasicHiName" },
    { "C64 mode Basic",         "Basic64Name" },

    /* Chargen, others */

    /* C64, C64DTV, VIC20, PET, CBM II */
    { "Chargen",                "ChargenName" },
    { "International chargen",  "ChargenIntName" },     /* C128 */
    { "Finish chargen",         "ChargenFIName" },      /* C128 */
    { "French chargen",         "ChargenFRName" },      /* C128 */
    { "German chargen",         "ChargenDEName" },      /* C128 */
    { "Italian chargen",        "ChargenITName" },      /* C128 */
    { "Norwegian chargen",      "ChargenNOName" },      /* C128 */
    { "Swedish chargen",        "ChargenSEName" },      /* C128 */
    { "Swiss chargen",          "ChargenCHName" },      /* C128 */
    { "Function low",           "FunctionLowName" },    /* Plus/4 */
    { "Function high",          "FunctionHighName" },   /* Plus/4 */
    { "C1 low",                 "c1loName" },           /* Plus/4 */
    { "C1 high",                "c1hiName" },           /* Plus/4 */
    { "C2 low",                 "c2loName" },           /* Plus/4 */
    { "C2 high",                "c2hiName" },           /* Plus/4 */
    { "Editor",                 "EditorName" },         /* PET */
    { "$9000-$9FFF ROM",        "RomModule9Name" },     /* PET */
    { "$A000-$AFFF ROM",        "RomModuleAName" },     /* PET */
    { "$B000-$BFFF ROM",        "RomModuleBName" },     /* PET */
    { "H6809 ROM A",            "H6809RomAName" },      /* PET */
    { "H6809 ROM B",            "H6809RomBName" },      /* PET */
    { "H6809 ROM C",            "H6809RomCName" },      /* PET */
    { "H6809 ROM D",            "H6809RomDName" },      /* PET */
    { "H6809 ROM E",            "H6809RomEName" },      /* PET */
    { "H6809 ROM F",            "H6809RomFName" },      /* PET */
};

/** \brief  List of drive ROM IDs, names and resources
 *
 * FIXME: shouldn't be here but somewhere in core code, but alas.
 */
static const drive_rom_t drive_rom_list[] = {
    { DRIVE_TYPE_1540,      DRIVE_NAME_1540,    "DosName1540" },
    { DRIVE_TYPE_1541,      DRIVE_NAME_1541,    "DosName1541" },
    { DRIVE_TYPE_1541II,    DRIVE_NAME_1541II,  "DosName1541ii" },
    { DRIVE_TYPE_1551,      DRIVE_NAME_1551,    "DosName1551" },
    { DRIVE_TYPE_1571,      DRIVE_NAME_1571,    "DosName1571" },
    { DRIVE_TYPE_1571CR,    DRIVE_NAME_1571CR,  "DosName1571cr" },
    { DRIVE_TYPE_1581,      DRIVE_NAME_1581,    "DosName1581" },
    { DRIVE_TYPE_2000,      DRIVE_NAME_2000,    "DosName2000" },
    { DRIVE_TYPE_4000,      DRIVE_NAME_4000,    "DosName4000" },
    { DRIVE_TYPE_2031,      DRIVE_NAME_2031,    "DosName2031" },
    { DRIVE_TYPE_2040,      DRIVE_NAME_2040,    "DosName2040" },
    { DRIVE_TYPE_3040,      DRIVE_NAME_3040,    "DosName3040" },
    { DRIVE_TYPE_4040,      DRIVE_NAME_4040,    "DosName4040" },
    { DRIVE_TYPE_1001,      DRIVE_NAME_1001,    "DosName1001" },
    { DRIVE_TYPE_8050,      DRIVE_NAME_8050,    NULL },
    { DRIVE_TYPE_8250,      DRIVE_NAME_8250,    NULL },
    { DRIVE_TYPE_9000,      DRIVE_NAME_9000,    "DosName9000" },
    { DRIVE_TYPE_CMDHD,     DRIVE_NAME_CMDHD,   "DosNameCMDHD" }
};

/** \brief  List of drive expansion ROMs names and resources */
static const drive_exp_rom_t drive_exp_rom_list[] = {
    { "Professional DOS 1571",  "DriveProfDOS1571Name" },
    { "Supercard+",             "DriveSuperCardName" },
    { "StarDOS",                "DriveStarDosName" },
};

/** \brief  Patterns for the ROM file chooser widgets */
static const char *rom_patterns[] = { "*.bin", "*.rom", NULL };

/** \brief  Patterns for the ROM set file dialogs */
static const char *romset_patterns[] = { "*.vrs", NULL };

/** \brief  Main GtkListBox */
static GtkWidget *root_list;

/** \brief  Machine ROMs widget
 *
 * GtkListBoxRow containing a GtkExpander containing a GtkListBox.
 */
static GtkWidget *machine_roms;

/** \brief  Drive ROMs widget
 *
 * GtkListBoxRow containing a GtkExpander containing a GtkListBox.
 */
static GtkWidget *drive_roms;

/** \brief  Drive expansion ROMs widget
 *
 * GtkListBoxRow containing a GtkExpander containing a GtkListBox.
 */
static GtkWidget *drive_exp_roms;

/** \brief  Last directory used in a ROM set file dialog */
static gchar *last_directory;

/** \brief  Last filename used in a ROM set file dialog */
static gchar *last_filename;

/** \brief  CSS provided used for the GtkListBox widgets */
static GtkCssProvider *listbox_css_provider;


/* {{{ Event handlers */
/** \brief  Callback for the ROM set open dialog
 *
 * \param[in]   dialog      ROM set load dialog
 * \param[in]   filename    filename (or \a NULL when canceled)
 * \param[in]   data        extra callback data (ignored)
 */
static void on_load_romset_callback(GtkDialog *dialog,
                                    gchar     *filename,
                                    gpointer   data)
{
    debug_gtk3("filename = %s", filename != NULL ? filename : "NULL");

    if (filename != NULL) {
        lastdir_update(GTK_WIDGET(dialog), &last_directory, &last_filename);

        if (romset_file_load(filename) != 0) {
            vice_gtk3_message_error(GTK_WINDOW(dialog),
                                    "VICE error",
                                    "Failed to load ROM set file %s.",
                                    filename);
        }
        /* ROM set loading is not atomic, it can fail after having already
         * set one or more resources, so we need to sync unconditionally */
        expandable_list_sync_resources(machine_roms);
        expandable_list_sync_resources(drive_roms);
        expandable_list_sync_resources(drive_exp_roms);
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/** \brief  Handler for the 'clicked' event of the "Load ROM set" button
 *
 * Shows dialog to open a ROM set file.
 *
 * \param[in]   self    button (ignored)
 * \param[in]   data    extra event data (unused)
 */
static void on_load_romset_clicked(GtkButton *self, gpointer data)
{
    GtkWidget *dialog;

    dialog = vice_gtk3_open_file_dialog("Select a ROM set file to load",
                                        "ROM set files",
                                        romset_patterns,
                                        NULL,
                                        on_load_romset_callback,
                                        NULL);
    lastdir_set(dialog, &last_directory, &last_filename);
    gtk_widget_show(dialog);
}

/** \brief  Callback for the ROM set save dialog
 *
 * \param[in]   dialog      ROM set save dialog
 * \param[in]   filename    filename (or \a NULL when canceled)
 * \param[in]   data        extra callback data (ignored)
 */
static void on_romset_save_callback(GtkDialog *dialog,
                                    gchar     *filename,
                                    gpointer   data)
{
    if (filename != NULL) {
        const char **roms = get_all_resource_names();

        if (romset_file_save(filename, roms) != 0) {
            vice_gtk3_message_error(GTK_WINDOW(dialog),
                                    "VICE error",
                                    "Failed to save ROM set file %s.",
                                    filename);
        }
        lib_free(roms);
        g_free(filename);
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/** \brief  Handler for the 'clicked' event of the "Save ROM set" button
 *
 * Shows dialog to save a ROM set file.
 *
 * \param[in]   self    button (ignored)
 * \param[in]   data    extra event data (unused)
 */
static void on_save_romset_clicked(GtkButton *self, gpointer data)
{
    GtkWidget *dialog;

    dialog = vice_gtk3_save_file_dialog("Enter or select ROM set file to save",
                                        "romset.vrs",
                                        TRUE,
                                        NULL,
                                        on_romset_save_callback,
                                        NULL);
    lastdir_set(dialog, &last_directory, &last_filename);
    gtk_widget_show(dialog);
}

/** \brief  Handler for the 'clicked' event of the "Reset to defaults" button
 *
 * Resets all ROM resources to factory values.
 *
 * \param[in]   self    button (ignored)
 * \param[in]   data    extra event data (unused)
 */
static void on_reset_to_defaults_clicked(GtkButton *self, gpointer data)
{
    const char **roms = get_all_resource_names();

    if (roms != NULL) {
        int i;

        for (i = 0; roms[i] != NULL; i++) {
            const char *factory = NULL;

            if (resources_get_default_value(roms[i], &factory) == 0) {
#if 0
                debug_gtk3("resetting resource %s to factory value \"%s\"",
                           roms[i], factory);
#endif
                resources_set_string(roms[i], factory);
            }
        }
        expandable_list_sync_resources(machine_roms);
        expandable_list_sync_resources(drive_roms);
        expandable_list_sync_resources(drive_exp_roms);
    }
}

/** \brief  Handler for the 'clicked' event of the reset-to-default icon button
 *
 * Reset a ROM resource to its default value
 *
 * \param[in]   self    button (ignored)
 * \param[in]   data    resource file chooser for the ROM
 */
static void on_reset_icon_clicked(GtkWidget *self, gpointer data)
{
    GtkWidget  *chooser;
    const char *resource;
    const char *factory = NULL;

    chooser  = data;
    resource = mediator_get_name_w(chooser);
    resources_get_default_value(resource, (void*)&factory);
    if (factory != NULL) {
        resources_set_string(resource, factory);
        vice_gtk3_resource_filechooser_sync(chooser);
    }
}

/** \brief  Handler for the 'clicked' event of the eject icon button
 *
 * "Eject" a ROM by clearing a resource.
 *
 * \param[in]   self    button (ignored)
 * \param[in]   data    resource file chooser for the ROM
 */
static void on_eject_icon_clicked(GtkWidget *self, gpointer data)
{
    GtkWidget  *chooser;
    const char *resource;

    chooser  = data;
    resource = mediator_get_name_w(chooser);
    resources_set_string(resource, "");
    vice_gtk3_resource_filechooser_sync(chooser);
}

/** \brief  Handler for the 'drag-data-received' event of a resource file chooser
 *
 * \param[in]   self        resource file chooser
 * \param[in]   context     drag context
 * \param[in]   x           ignored
 * \param[in]   y           ignored
 * \param[in]   data        drag data
 * \param[in]   info        ignored
 * \param[in]   time        timestamp of drag event
 * \param[in]   user_data   extra event data (ignored)
 */
static void on_chooser_drag_data_received(GtkWidget        *self,
                                          GdkDragContext   *context,
                                          gint              x,
                                          gint              y,
                                          GtkSelectionData *data,
                                          guint             info,
                                          guint             time,
                                          gpointer          user_data)
{
    if (gtk_selection_data_get_length(data) > 0) {
        const char *filename;

        filename = (const char *)gtk_selection_data_get_data(data);
#ifdef HAVE_DEBUG_GTK3UI
        debug_gtk3("Setting resource \"%s\" to \"%s\"",
                   mediator_get_name_w(self), filename);
#endif
        vice_gtk3_resource_filechooser_set(self, filename);
        /* mark the drop finish so we don't end up with weird data like
         * "org.ibus.bla" in the entry */
        gtk_drag_finish(context, TRUE, FALSE /* don't delete source */, time);
    }
}

/** \brief  Handler for the 'clicked' event of a PET charset selection button
 *
 * Load a chargen ROM by setting the resource "ChargenName" to \a data.
 *
 * \param[in]   self    button (ignored)
 * \param[in]   data    new value for the "ChargenName" resource
 */
static void on_pet_charset_load_clicked(GtkWidget *self, gpointer data)
{
    GtkWidget  *chooser;
    const char *charset;

    chooser = expandable_list_get_chooser_by_resource(machine_roms, "ChargenName");
    charset = data;
    debug_gtk3("resource value = %s, chooser = %p", charset, (const void*)chooser);
    resources_set_string("ChargenName", charset);
    if (chooser != NULL) {
        vice_gtk3_resource_filechooser_sync(chooser);
    }
}
/* }}} */

/* {{{ Helper functions */
/** \brief  Append list of strings to another list of strings
 *
 * Reallocate \a to to make space for all elements of \a from and append
 * all elements from \a from to \a to.
 *
 * \param[in]   to      list of strings to append to
 * \param[in]   from    list of strings to append
 *
 * \return  \a to, possibly relocated
 */
static const char **string_list_append_list(const char **to, const char **from)
{
    if (to == NULL && from != NULL) {
        /* make copy of `from` so calling code can unconditionally free `from`
         * after calling this function */
        size_t n;

        for (n = 0; from[n] != NULL; n++) { /* NOP */ }
        to = lib_malloc((n + 1u) * sizeof *to);
        for (n = 0; from[n] != NULL; n++) {
            to[n] = from[n];
        }
        to[n] = NULL;
    } else if (to != NULL && from != NULL) {
        size_t nt;
        size_t nf;
        size_t i;

        /* count elements in lists */
        for (nt = 0; to[nt]   != NULL; nt++) { /* NOP */ }
        for (nf = 0; from[nf] != NULL; nf++) { /* NOP */ }

        /* append elements in `from` to `to` */
        to = lib_realloc(to, (nt + nf + 1u) * sizeof *to);
        /* copy elements *including* terminating NULL */
        for (i = 0; i <= nf; i++) {
            to[nt + i] = from[i];
        }
    }
    return to;
}

/** \brief  Get list of all resource names for valid ROMs
 *
 * \return  list of resource names, \a NULL terminated
 * \note    free list after use with \a lib_free()
 */
static const char **get_all_resource_names(void)
{
    const char **allroms   = expandable_list_get_resources(machine_roms);
    const char **driveroms = expandable_list_get_resources(drive_roms);
    const char **drexproms = expandable_list_get_resources(drive_exp_roms);

    allroms = string_list_append_list(allroms, driveroms);
    lib_free(driveroms);
    allroms = string_list_append_list(allroms, drexproms);
    lib_free(drexproms);

    return allroms;
}

/** \brief  Create horizontally aligned label using Pango markup
 *
 * \param[in]   markup  Pango markup text for the label
 * \param[in]   halign  horizontal alignment for the label
 *
 * \return  GtkLabel
 */
static GtkWidget *label_helper(const char *markup, GtkAlign halign)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), markup);
    gtk_widget_set_halign(label, halign);
    return label;
}

/** \brief  Create expandable list of file choosers
 *
 * Create a GtkListBoxRow with a GtkExpander that contains a GtkListBox for
 * GtkListBoxRows with file chooser widgets (and labels).
 *
 * \param[in]   title   title for the expander
 *
 * \return  GtkListBoxRow
 */
static GtkWidget *expandable_list_new(const char *title)
{
    GtkWidget *listrow;
    GtkWidget *expander;
    GtkWidget *list;

    listrow  = gtk_list_box_row_new();
    expander = gtk_expander_new(title);
    list     = gtk_list_box_new();

    vice_gtk3_css_provider_add(list, listbox_css_provider);

    gtk_container_add(GTK_CONTAINER(expander), list);
    gtk_container_add(GTK_CONTAINER(listrow), expander);
    return listrow;
}

/** \brief  Get list of GtkListBoxRows for a given ROM widget list
 *
 * \param[in]   list    ROM widget list
 *
 * \return  all children of \a list
 * \note    free after use with \a g_list_free()
 */
static GList *expandable_list_get_children(GtkWidget *list)
{
    GtkWidget *expander;
    GtkWidget *listbox;

    expander = gtk_bin_get_child(GTK_BIN(list));
    listbox  = gtk_bin_get_child(GTK_BIN(expander));
    return gtk_container_get_children(GTK_CONTAINER(listbox));
}

/** \brief  Get list of resource name and value pairs for a ROM list
 *
 * \param[in]   list    ROM list (GtkListBoxRow)
 *
 * \return  list of ROM resources in the list, \c NULL terminated
 *
 * \note    Free the list after use with \c lib_free()
 */
static const char **expandable_list_get_resources(GtkWidget *list)
{
    GList       *rows;
    GList       *row;
    const char **roms;
    guint        num_rows;
    guint        i;

    rows = expandable_list_get_children(list);
    if (rows == NULL) {
        return NULL;
    }
    num_rows = g_list_length(rows);
    roms     = lib_malloc((num_rows + 1u) * sizeof *roms);

    i = 0;
    for (row = rows; row != NULL; row = row->next) {
        GtkWidget  *grid;
        GtkWidget  *chooser;

        grid      = gtk_bin_get_child(GTK_BIN(row->data));
        chooser   = gtk_grid_get_child_at(GTK_GRID(grid), 1, 0);
        roms[i++] = mediator_get_name_w(chooser);
    }
    roms[i] = NULL;
    g_list_free(rows);

    return roms;
}

/** \brief  Synchronize all widgets in a ROM widget list with their resources
 *
 * \param[in]   list    ROM widget list
 */
static void expandable_list_sync_resources(GtkWidget *list)
{
    GList *rows;
    GList *row;

    rows = expandable_list_get_children(list);
    for (row = rows; row != NULL; row = row->next) {
        GtkWidget *grid;
        GtkWidget *chooser;

        grid    = gtk_bin_get_child(GTK_BIN(row->data));
        chooser = gtk_grid_get_child_at(GTK_GRID(grid), 1, 0);
        vice_gtk3_resource_filechooser_sync(chooser);
    }
    g_list_free(rows);
}

/** \brief  Find a resource filechooser in a ROMs list widget by resource name
 *
 * \param[in]   list        ROMs list
 * \param[in]   resource    resource name
 *
 * \return  resource filechooser widget or \c NULL if not found
 */
static GtkWidget *expandable_list_get_chooser_by_resource(GtkWidget  *list,
                                                          const char *resource)
{
    GtkWidget *chooser;
    GList     *rows;
    GList     *row;

    rows    = expandable_list_get_children(list);
    chooser = NULL;
    for (row = rows; row != NULL; row = row->next) {
        GtkWidget *grid;
        GtkWidget *widget;

        grid   = gtk_bin_get_child(GTK_BIN(row->data));
        widget = gtk_grid_get_child_at(GTK_GRID(grid), 1, 0);
        /* debug_gtk3("g_strcmp0(%s, %s)", resource, mediator_get_name_w(widget)); */
        if (g_strcmp0(resource, mediator_get_name_w(widget)) == 0) {
            chooser = widget;
            break;
        }
    }
    g_list_free(rows);
    return chooser;
}

/** \brief  Add resource file chooser with label to a ROMs section
 *
 * \param[in]   list            ROMs section
 * \param[in]   label_text      text for the label
 * \param[in]   resource_name   resource name for the resource file chooser
 */
static void add_rom_chooser(GtkWidget  *list,
                            const char *label_text,
                            const char *resource_name)
{
    GtkWidget *listrow;
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *chooser;
    GtkWidget *reset;
    GtkWidget *eject;
    GtkWidget *expander;
    GtkWidget *rom_list;

    listrow = gtk_list_box_row_new();
    grid    = gtk_grid_new();
    label   = label_helper(label_text, GTK_ALIGN_START);
    chooser = vice_gtk3_resource_filechooser_new(resource_name,
                                                 GTK_FILE_CHOOSER_ACTION_OPEN);

    /* set up the label: we need to use xalign here to force left alignment
     * since we set a fixed size and the normal alignment is ignored */
    gtk_widget_set_size_request(label, 150, -1);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_widget_set_margin_start(label, 8);

    /* set up the file chooser widget */
    vice_gtk3_resource_filechooser_set_filter(chooser,
                                             "ROM files",
                                             rom_patterns,
                                             TRUE);
    vice_gtk3_resource_filechooser_set_custom_title(chooser, "Select ROM file");
    gtk_widget_set_halign(chooser, GTK_ALIGN_FILL);
    g_signal_connect(G_OBJECT(chooser),
                     "drag-data-received",
                     G_CALLBACK(on_chooser_drag_data_received),
                     NULL);

    /* set up reset-to-default button */
    reset = gtk_button_new_from_icon_name("view-refresh-symbolic",
                                          GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_set_tooltip_text(reset, "Reset to default value");
    g_signal_connect(G_OBJECT(reset),
                     "clicked",
                     G_CALLBACK(on_reset_icon_clicked),
                     (gpointer)chooser);

    /* set up unload button */
    eject = gtk_button_new_from_icon_name("media-eject-symbolic",
                                          GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_set_tooltip_text(eject, "Unload ROM");
    g_signal_connect(G_OBJECT(eject),
                     "clicked",
                     G_CALLBACK(on_eject_icon_clicked),
                     (gpointer)chooser);

    /* put everthing together */
    expander = gtk_bin_get_child(GTK_BIN(list));
    rom_list = gtk_bin_get_child(GTK_BIN(expander));
    gtk_grid_attach(GTK_GRID(grid), label,   0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), chooser, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), reset,   2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), eject,   3, 0, 1, 1);
    gtk_container_add(GTK_CONTAINER(listrow), grid);
    gtk_list_box_insert(GTK_LIST_BOX(rom_list), listrow, -1);
}
/* }}} */


/** \brief  Create ROM settings widget
 *
 * \param[in]   parent  unused
 *
 * \return  GtkGrid
 */
GtkWidget *settings_rom_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *scrolled;
    GtkWidget *expander;
    GtkWidget *button_box;
    GtkWidget *button;
    size_t     i;
    int        row = 0;

    listbox_css_provider  = vice_gtk3_css_provider_new(LISTBOX_CSS);

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    root_list = gtk_list_box_new();
    gtk_widget_set_vexpand(root_list, TRUE);
    scrolled  = gtk_scrolled_window_new(NULL, NULL);
    vice_gtk3_css_provider_add(root_list, listbox_css_provider);

    if (machine_class == VICE_MACHINE_PET) {
        /* We add a check button and buttons to load a chargen to xpet, so the
         * scrolled window must be slightly less tall */
        gtk_widget_set_size_request(scrolled, 400, 370);
    } else {
        gtk_widget_set_size_request(scrolled, 400, 420);
    }
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), root_list);
    gtk_grid_attach(GTK_GRID(grid), scrolled, 0, row, 1, 1);
    row++;

    machine_roms   = expandable_list_new("Machine ROMs");
    drive_roms     = expandable_list_new("Drive ROMs");
    drive_exp_roms = expandable_list_new("Drive expansion ROMs");
    gtk_container_add(GTK_CONTAINER(root_list), machine_roms);
    gtk_container_add(GTK_CONTAINER(root_list), drive_roms);
    gtk_container_add(GTK_CONTAINER(root_list), drive_exp_roms);

    /* add machine ROMs */
    for (i = 0; i < ARRAY_LEN(machine_rom_list); i++) {
        if (resources_exists(machine_rom_list[i].resource)) {
            add_rom_chooser(machine_roms,
                            machine_rom_list[i].name,
                            machine_rom_list[i].resource);
        }
    }

    /* add drive ROMs */
    for (i = 0; i < ARRAY_LEN(drive_rom_list); i++) {
        if (resources_exists(drive_rom_list[i].resource)) {
            add_rom_chooser(drive_roms,
                            drive_rom_list[i].name,
                            drive_rom_list[i].resource);
        }
    }

    /* add drive expansion ROMs */
    for (i = 0; i < ARRAY_LEN(drive_exp_rom_list); i++) {
        if (resources_exists(drive_exp_rom_list[i].resource)) {
            add_rom_chooser(drive_exp_roms,
                            drive_exp_rom_list[i].name,
                            drive_exp_rom_list[i].resource);
        }
    }

    /* expand the machine ROMs by default */
    expander = gtk_bin_get_child(GTK_BIN(machine_roms));
    gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);

    if (machine_class == VICE_MACHINE_PET) {
        /* add PET v1 kernal IEEE-488 patch */
        GtkWidget *check = vice_gtk3_resource_check_button_new("Basic1",
                "Patch kernal v1 to make the IEEE-488 interface work");

        gtk_grid_attach(GTK_GRID(grid), check, 0, row, 1, 1);
        row++;
    }

    /* add normal buttons */
    button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(button_box), 8);
    button = gtk_button_new_with_label("Load ROM set");
    g_signal_connect(G_OBJECT(button),
                     "clicked",
                     G_CALLBACK(on_load_romset_clicked),
                     NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, FALSE, 0);
    button = gtk_button_new_with_label("Save ROM set");
    g_signal_connect(G_OBJECT(button),
                     "clicked",
                     G_CALLBACK(on_save_romset_clicked),
                     NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, FALSE, 0);
    button = gtk_button_new_with_label("Reset to defaults");
    g_signal_connect(G_OBJECT(button),
                     "clicked",
                     G_CALLBACK(on_reset_to_defaults_clicked),
                     NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, FALSE, 0);
    gtk_grid_attach(GTK_GRID(grid), button_box, 0, row, 1, 1);
    row++;

    /* add PET-specific buttons */
    if (machine_class == VICE_MACHINE_PET) {
        button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 8);

        /* load original charset button */
        button = gtk_button_new_with_label("Load original charset");
        g_signal_connect(G_OBJECT(button),
                         "clicked",
                         G_CALLBACK(on_pet_charset_load_clicked),
                         (gpointer)PET_CHARGEN2_NAME);
        gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, FALSE, 0);
        /* load DE charset button */
        button = gtk_button_new_with_label("Load German charset");
        g_signal_connect(G_OBJECT(button),
                         "clicked",
                         G_CALLBACK(on_pet_charset_load_clicked),
                         (gpointer)PET_CHARGEN_DE_NAME);
        gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, FALSE, 0);
        gtk_grid_attach(GTK_GRID(grid), button_box, 0, row, 1, 1);
    }

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Free resources used by the ROM settings
 *
 * Frees the last used directory and filename of the file dialogs.
 */
void settings_rom_widget_shutdown(void)
{
    g_free(last_directory);
    g_free(last_filename);
    last_directory = NULL;
    last_filename  = NULL;
    if (listbox_css_provider != NULL) {
        g_object_unref(listbox_css_provider);
        listbox_css_provider  = NULL;
    }
}
