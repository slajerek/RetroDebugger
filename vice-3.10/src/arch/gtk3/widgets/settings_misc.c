/** \file   settings_misc.c
 * \brief   Widget to control resources that are hard to place properly
 *
 * Currently doesn't contain a single widget, which is nice. Keeping this around
 * for future hard-to-place widgets. In the end this should go.
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
 *
 */

#include "vice.h"
#include <gtk/gtk.h>
#include "vice_gtk3.h"

#include "settings_misc.h"


/** \brief  Create miscellaneous settings widget
 *
 * Basically a widget to contain (hopefully temporarily) widgets controlling
 * resources that can't (yet) be placed in a more logical location.
 *
 * \param[in]   widget  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_misc_widget_create(GtkWidget *widget)
{
    GtkWidget *grid;
    GtkWidget *label;

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);

    label = gtk_label_new(NULL);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_markup(GTK_LABEL(label),
            "Placeholder for settings where we can't think of a better place.\n\n"
            "Ideally this is empty, but it can <b>temporarily</b> be used to store "
            "some widgets until we figure out a proper place.");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
