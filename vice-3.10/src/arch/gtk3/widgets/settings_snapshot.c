/** \file   settings_snapshot.c
 * \brief   Snapshot/recording settings widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES EventSnapshotDir    all
 * $VICERES EventStartMode      all
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
 *
 */

#include "vice.h"
#include <gtk/gtk.h>

#include "gfxoutput.h"
#include "vice-event.h"
#include "vice_gtk3.h"
#include "uimedia.h"

#include "settings_snapshot.h"


/** \brief  List of Event start modes
 */
static const vice_gtk3_radiogroup_entry_t recstart_modes[] = {
    { "Save new snapshot",          EVENT_START_MODE_FILE_SAVE },
    { "Load existing snapshot",     EVENT_START_MODE_FILE_LOAD },
    { "Start with reset",           EVENT_START_MODE_RESET },
    { "Overwrite running playback", EVENT_START_MODE_PLAYBACK },
    { NULL, -1 }
};


/** \brief  Create settings widget for snapshot/event recording
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 *
 * \todo    Use resourcebrowser to control "EventSnapshotDir" resource
 */
GtkWidget *settings_snapshot_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *histdir;
    GtkWidget *recmode;
    GtkWidget *quickformat;
    gfxoutputdrv_t *driver;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new("History/snapshot directory");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    histdir = vice_gtk3_resource_filechooser_new("EventSnapshotDir",
                                                 GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    vice_gtk3_resource_filechooser_set_custom_title(histdir,
                                                    "Select history/snapshot directory");
    gtk_grid_attach(GTK_GRID(grid), label,   0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), histdir, 1, 0, 1, 1);

    label = gtk_label_new("Recording start mode");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_START);
    recmode = vice_gtk3_resource_radiogroup_new("EventStartMode",
                                                recstart_modes,
                                                GTK_ORIENTATION_VERTICAL);

    gtk_grid_attach(GTK_GRID(grid), label,   0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), recmode, 1, 1, 2, 1);


    label = gtk_label_new("Quicksave screenshot format");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_START);
    quickformat = vice_gtk3_resource_combo_str_new("QuicksaveScreenshotFormat", NULL);

    driver = gfxoutput_drivers_iter_init();
    while (driver != NULL) {
        if (driver->type == GFXOUTPUTDRV_TYPE_SCREENSHOT_NATIVE ||
            driver->type == GFXOUTPUTDRV_TYPE_SCREENSHOT_IMAGE) {
            vice_gtk3_resource_combo_str_append(quickformat, driver->name, driver->displayname);
        }
        driver = gfxoutput_drivers_iter_next();
    }
    vice_gtk3_resource_combo_str_sync(quickformat);
    gtk_grid_attach(GTK_GRID(grid), label,       0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), quickformat, 1, 2, 2, 1);

    gtk_widget_show_all(grid);
    return grid;
}
