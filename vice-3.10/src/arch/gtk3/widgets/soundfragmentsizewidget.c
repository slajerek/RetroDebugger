/** \file   soundfragmentsizewidget.c
 * \brief   GTK3 sound fragment size widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES SoundFragmentSize       all
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

#include "sound.h"
#include "vice_gtk3.h"

#include "soundfragmentsizewidget.h"


/** \brief  Sound buffer fragment sizes table
 */
static const vice_gtk3_radiogroup_entry_t fragment_sizes[] = {
    { "Very small", SOUND_FRAGMENT_VERY_SMALL },
    { "Small",      SOUND_FRAGMENT_SMALL },
    { "Medium",     SOUND_FRAGMENT_MEDIUM },
    { "Large",      SOUND_FRAGMENT_LARGE },
    { "Very large", SOUND_FRAGMENT_VERY_LARGE },
    { NULL,         -1 }
};


/** \brief  Create widget to set the "SoundFragmentSize" resource
 *
 * \return  GtkGrid
 */
GtkWidget *sound_fragment_size_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *group;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Fragment size</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);

    group = vice_gtk3_resource_radiogroup_new("SoundFragmentSize",
                                              fragment_sizes,
                                              GTK_ORIENTATION_VERTICAL);

    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}
