/** \file   petram9widget.c
 * \brief   Widget to set the RAM9 area type (PET 8296 only)
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 * \author  Andre Fachat <fachat@web.de>
 */

/*
 * $VICERES Ram9  xpet
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
#include "resources.h"
#include "petiosizewidget.h"

#include "petram9widget.h"


/** \brief  Defines values for jumper JU2/JU4
 *
 * For details see http://www.6502.org/users/andre/petindex/local/8296desc.txt
 */
static const vice_gtk3_radiogroup_entry_t area_types[] = {
    { "ROM", 0 },
    { "RAM", 1 },
    { NULL, -1 }
};


/** \brief  Callback function for radio button toggles
 */
static void (*user_callback)(int) = NULL;


/** \brief  Handler for the 'toggled' event of the radio buttons
 *
 * \param[in]   widget  radio button (unused)
 * \param[in]   id      radio button ID
 */
static void on_ram9_changed(GtkWidget *widget, int id)
{
    if (user_callback != NULL) {
        user_callback(id);
    }
}


/** \brief  Create PET RAM9 widget
 *
 * \return  GtkGrid
 */
GtkWidget *pet_ram9_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *group;

    user_callback = NULL;

    grid = vice_gtk3_grid_new_spaced_with_label(-1, -1, "$9xxx area type", 1);
    group = vice_gtk3_resource_radiogroup_new("Ram9", area_types,
            GTK_ORIENTATION_VERTICAL);
    vice_gtk3_resource_radiogroup_add_callback(group, on_ram9_changed);
    gtk_widget_set_margin_start(group, 16);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set custom callback function for radio button toggle events
 *
 * \param[in]       func    callback function
 */
void pet_ram9_widget_set_callback(void (*func)(int))
{
    user_callback = func;
}


/** \brief  Synchronize \a widget with its resource
 *
 * \param[in,out]   widget  PET RAM9 widget
 */
void pet_ram9_widget_sync(GtkWidget *widget)
{
    int size;
    GtkWidget *group;

    resources_get_int("Ram9", &size);
    group = gtk_grid_get_child_at(GTK_GRID(widget), 0, 1);
    vice_gtk3_resource_radiogroup_set(group, size);
}
