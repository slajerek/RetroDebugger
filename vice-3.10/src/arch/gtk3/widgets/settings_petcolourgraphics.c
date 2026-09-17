/** \file   settings_petcolourgraphics.c
 * \brief   Settings widget for PET Colour Graphics
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES PETColour       xpet
 * $VICERES PETColourBG     xpet
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

#include "pet.h"
#include "vice_gtk3.h"

#include "settings_petcolourgraphics.h"


/** \brief  List of PET Colour types
 */
static const vice_gtk3_radiogroup_entry_t colour_types[] = {
    { "Off",    PET_COLOUR_TYPE_OFF },
    { "RGBI",   PET_COLOUR_TYPE_RGBI },
    { "Analog", PET_COLOUR_TYPE_ANALOG },
    { NULL,     -1 }
};


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


/** \brief  Create widget to control PET Colour graphics resources
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_petcolourgraphics_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *group;
    GtkWidget *spin;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 16);

    /* PET Colour type */
    label = label_helper("PET Colour type");
    group = vice_gtk3_resource_radiogroup_new("PETColour",
                                              colour_types,
                                              GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_valign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    /* use two columns to avoid the BG spinbutton to stretch very wide */
    gtk_grid_attach(GTK_GRID(grid), group, 1, 0, 2, 1);

    /* PET Colour background */
    label = label_helper("PET Colour background");
    spin  = vice_gtk3_resource_spin_int_new("PETColourBG", 0, 255, 1);
    gtk_widget_set_hexpand(spin, FALSE);    /* still too wide */
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), spin, 1, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
