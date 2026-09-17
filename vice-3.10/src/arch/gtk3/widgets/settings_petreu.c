/** \file   settings_petreu.c
 * \brief   Settings widget for PET RAM Expansion Unit
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES PETREU          xpet
 * $VICERES PETREUsize      xpet
 * $VICERES PETREUfilename  xpet
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

#include "vice_gtk3.h"

#include "settings_petreu.h"


/** \brief  List of REU sizes in KiB */
static const int reu_sizes[] = { 128, 512, 1024, 2048, -1 };


/* References to widgets to be able to toggle sensitive state depending on the
 * REU Enable check button.
 */

/** \brief  REU image file chooser */
static GtkWidget *chooser = NULL;

/** \brief  REU size radio group */
static GtkWidget *group = NULL;


/** \brief  Set sensitivity of RAM size and image file chooser widgets
 *
 * \param[in]   sensitive   new sensitivity
 */
static void set_widgets_sensitivity(gboolean sensitive)
{
    gtk_widget_set_sensitive(group,   sensitive);
    gtk_widget_set_sensitive(chooser, sensitive);
}

/** \brief  Handler for the "toggled" event of the REU Enable check button
 *
 * Toggles sensitive state of other widgets
 *
 * \param[in]   widget      REU check button
 * \param[in]   user_data   extra event data (unused)
 */
static void on_reu_toggled(GtkWidget *widget, gpointer user_data)
{
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    set_widgets_sensitivity(active);
}

/** \brief  Create left-aligned label using Pango markup
 *
 * \param[in]   text    text for label
 *
 * \return  GtkLabel
 */
static GtkWidget *label_helper(const char *text)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}


/** \brief  Create widget to control PET REU settings
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 */
GtkWidget *settings_petreu_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *enable;
    GtkWidget *label;
    gboolean   active;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 16);

    /* REU enable */
    enable = vice_gtk3_resource_check_button_new("PETREU",
                                                 "Enable PET RAM Expansion Unit");
    g_signal_connect_unlocked(G_OBJECT(enable),
                              "toggled",
                              G_CALLBACK(on_reu_toggled),
                              NULL);
    gtk_grid_attach(GTK_GRID(grid), enable, 0, 0, 3, 1);

    /* REU size */
    label = label_helper("REU size");
    group = ram_size_radiogroup_new("PETREUsize", NULL, reu_sizes);
    gtk_widget_set_valign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), group, 1, 1, 2, 1);

    /* REU image file */
    label   = label_helper("REU image file");
    chooser = vice_gtk3_resource_filechooser_new("PETREUfilename",
                                                 GTK_FILE_CHOOSER_ACTION_SAVE);
    vice_gtk3_resource_filechooser_set_custom_title(chooser,
                                                    "Select PET REU image file");
    gtk_grid_attach(GTK_GRID(grid), label,   0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), chooser, 1, 2, 2, 1);

    /* set initial sensitive state of widgets */
    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enable));
    set_widgets_sensitivity(active);

    gtk_widget_show_all(grid);
    return grid;
}
