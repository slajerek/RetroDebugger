/** \file   settings_environment.c
 * \brief   Settings widget to control host environment settings
 *
 * /author  Bas Wassink <b.wassink@ziggo.nl>
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

#include "cwdwidget.h"
#include "logfilewidget.h"
#include "machine.h"
#include "vice_gtk3.h"

#include "settings_environment.h"

/*
 * $VICERES LogColorize     all
 * $VICERES LogLevelANE     all
 * $VICERES LogLevelLXA     all
 * $VICERES LogLimit        all
 * $VICERES LogToFile       all
 * $VICERES LogToMonitor    all
 * $VICERES LogToStdout     all
 */

static const vice_gtk3_radiogroup_entry_t loglevels[] = {
    { "no log",        0 },
    { "unstable only", 1 },
    { "all",           2 },
    { NULL,           -1 }
};


/** \brief  Create widget to change host environment settings
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_environment_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *grid2;
    GtkWidget *cwd_label;
    GtkWidget *cwd_widget;
    GtkWidget *logfile_widget;
    GtkWidget *ane_label;
    GtkWidget *lxa_label;
    GtkWidget *ane_widget;
    GtkWidget *lxa_widget;
    int        row = 0;
    int        row2 = 0;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    /* CWD header */
    cwd_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(cwd_label), "<b>Current working directory</b>");
    gtk_widget_set_halign(cwd_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), cwd_label, 0, row, 1, 1);
    row++;

    /* CWD widget */
    cwd_widget = cwd_widget_create();
    gtk_grid_attach(GTK_GRID(grid), cwd_widget, 0, row, 1, 1);
    row++;

    /* Logfile widget */
    logfile_widget = logfile_widget_create();
    gtk_widget_set_margin_top(logfile_widget, 24);  /* extra space */
    gtk_grid_attach(GTK_GRID(grid), logfile_widget, 0, row, 1, 1);
    row++;

    if ((machine_class != VICE_MACHINE_SCPU64)) {

        grid2 = gtk_grid_new();
        gtk_grid_set_column_spacing(GTK_GRID(grid2), 8);
        gtk_grid_set_row_spacing(GTK_GRID(grid2), 8);

        ane_label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(ane_label), "<b>ANE logging</b>");
        gtk_widget_set_halign(ane_label, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid2), ane_label, 0, row2, 1, 1);

        lxa_label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(lxa_label), "<b>LXA logging</b>");
        gtk_widget_set_halign(lxa_label, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid2), lxa_label, 1, row2, 1, 1);
        row2++;

        ane_widget = vice_gtk3_resource_radiogroup_new("LogLevelANE",
                                                loglevels,
                                                GTK_ORIENTATION_VERTICAL);
        gtk_grid_attach(GTK_GRID(grid2), ane_widget, 0, row2, 1, 1);

        lxa_widget = vice_gtk3_resource_radiogroup_new("LogLevelLXA",
                                                loglevels,
                                                GTK_ORIENTATION_VERTICAL);
        gtk_grid_attach(GTK_GRID(grid2), lxa_widget, 1, row2, 1, 1);
        row2++;

        gtk_widget_show_all(grid2);

        gtk_grid_attach(GTK_GRID(grid), grid2, 0, row, 1, 1);
        row++;
    }

    gtk_widget_show_all(grid);
    return grid;
}
