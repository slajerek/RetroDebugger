/** \file   drivefsdevicewidget.c
 * \brief   Drive file system device widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES FSDevice8ConvertP00     -vsid
 * $VICERES FSDevice9ConvertP00     -vsid
 * $VICERES FSDevice10ConvertP00    -vsid
 * $VICERES FSDevice11ConvertP00    -vsid
 * $VICERES FSDevice8SaveP00        -vsid
 * $VICERES FSDevice9SaveP00        -vsid
 * $VICERES FSDevice10SaveP00       -vsid
 * $VICERES FSDevice11SaveP00       -vsid
 * $VICERES FSDevice8HideCBMFiles   -vsid
 * $VICERES FSDevice9HideCBMFiles   -vsid
 * $VICERES FSDevice10HideCBMFiles  -vsid
 * $VICERES FSDevice11HideCBMFiles  -vsid
 * $VICERES FSDevice8Dir            -vsid
 * $VICERES FSDevice9Dir            -vsid
 * $VICERES FSDevice10Dir           -vsid
 * $VICERES FSDevice11Dir           -vsid
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

#include "drive.h"
#include "machine.h"
#include "mainlock.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "drivefsdevicewidget.h"


/** \brief  Callback for the directory-select dialog
 *
 * \param[in]   dialog      directory-select dialog
 * \param[in]   filename    filename (NULL if canceled)
 * \param[in]   entry       entry box for the filename
 */
static void fsdir_browse_callback(GtkDialog *dialog, gchar *filename, gpointer entry)
{
    if (filename != NULL) {
        vice_gtk3_resource_entry_set(GTK_WIDGET(entry), filename);
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/** \brief  Handler for the 'clicked' event of the FS directory browse button
 *
 * \param[in]   button  FS directory browse button (unused)
 * \param[in]   entry   FS directory text entry
 */
static void on_fsdir_browse_clicked(GtkWidget *button, gpointer entry)
{
    GtkWidget *dialog;

    mainlock_assert_is_not_vice_thread();

    dialog = vice_gtk3_select_directory_dialog("Select filesystem directory",
                                               NULL,
                                               TRUE,
                                               NULL,
                                               fsdir_browse_callback,
                                               entry);
    gtk_widget_show_all(dialog);
}

/** \brief  Create text entry for file system directory for \a unit
 *
 * \param[in]   unit    unit number (8-11)
 *
 * \return  GtkEntry
 */
static GtkWidget *create_fsdir_entry_widget(int unit)
{
    GtkWidget *entry;
    char       resource[32];

    g_snprintf(resource, sizeof resource, "FSDevice%dDir", unit);
    entry = vice_gtk3_resource_entry_new(resource);
    gtk_widget_set_tooltip_text(entry,
                                "Set the host directory to use as a virtual drive");
    gtk_widget_set_hexpand(entry, TRUE);
    return entry;
}


/** \brief  Create widget to control P00-settings
 *
 * \param[in]   unit    unit number (8-11)
 *
 * \return  GtkGrid
 */
static GtkWidget *create_p00_widget(int unit)
{
    GtkWidget *grid;
    GtkWidget *p00_convert;
    GtkWidget *p00_only;
    GtkWidget *p00_save;

    grid = vice_gtk3_grid_new_spaced(8, 0);

    /* Label texts have been converted to shorter strings, using vice.texi
     * So don't blame me.
     */
    p00_convert = vice_gtk3_resource_check_button_new_sprintf(
            "FSDevice%dConvertP00",
            "Access P00 files with their built-in filename",
            unit);
    gtk_grid_attach(GTK_GRID(grid), p00_convert, 0, 0, 1, 1);

    p00_save = vice_gtk3_resource_check_button_new_sprintf(
            "FSDevice%dSaveP00",
            "Create P00 files on save",
            unit);
    gtk_grid_attach(GTK_GRID(grid), p00_save, 0, 1, 1, 1);

    p00_only = vice_gtk3_resource_check_button_new_sprintf(
            "FSDevice%dHideCBMFiles",
            "Only show P00 files",
            unit);
    gtk_grid_attach(GTK_GRID(grid), p00_only, 0, 2, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create widget to control file system device settings of \a unit
 *
 * \param[in]   unit    unit number
 *
 * \return  GtkGrid
 */
GtkWidget *drive_fsdevice_widget_create(int unit)
{
    GtkWidget *grid;
    GtkWidget *entry;
    GtkWidget *label;
    GtkWidget *browse;
    GtkWidget *p00;

    grid = vice_gtk3_grid_new_spaced(8, 0);

    label  = gtk_label_new("Host directory");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    entry  = create_fsdir_entry_widget(unit);
    browse = gtk_button_new_with_label("Browse ...");
    g_signal_connect_unlocked(browse,
                              "clicked",
                              G_CALLBACK(on_fsdir_browse_clicked),
                              (gpointer)entry);
    gtk_grid_attach(GTK_GRID(grid), label,  0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry,  1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), browse, 2, 1, 1, 1);

    p00 = create_p00_widget(unit);
    gtk_widget_set_margin_top(p00, 16);
    gtk_grid_attach(GTK_GRID(grid), p00, 0, 2, 3, 1);

    gtk_widget_show_all(grid);
    return grid;
}
