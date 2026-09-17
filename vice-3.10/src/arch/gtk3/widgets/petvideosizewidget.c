/** \file   petvideosizewidget.c
 * \brief   Widget to set the PET video size
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES VideoSize   xpet
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

#include "basewidgets.h"
#include "widgethelpers.h"
#include "debug_gtk3.h"
#include "resources.h"

#include "petvideosizewidget.h"


/** \brief  Data for the radio buttons group
 */
static const vice_gtk3_radiogroup_entry_t video_sizes[] = {
    { "Auto (from ROM)",  0 },
    { "40 Columns",      40 },
    { "80 Columns",      80 },
    { NULL,              -1 }
};


/** \brief  User-defined extra callback
 */
static void (*user_callback)(int) = NULL;


/** \brief  Handler for the 'toggled' event of the radio buttons
 *
 * Sets the VideoSize resource when it has been changed.
 *
 * \param[in]   widget  radio button triggering the event (unused)
 * \param[in]   size    value for the resource (`int`)
 */
static void video_size_callback(GtkWidget *widget, int size)
{
    if (user_callback != NULL) {
        user_callback(size);
    }
}


/** \brief  Create PET video size widget
 *
 * Creates a widget to control the PET's video display number of columns
 *
 * \return  GtkGrid
 */
GtkWidget *pet_video_size_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *group;

    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, "Display width", 1);
    vice_gtk3_grid_set_title_margin(grid, 8);

    group = vice_gtk3_resource_radiogroup_new("VideoSize",
                                              video_sizes,
                                              GTK_ORIENTATION_VERTICAL);
    vice_gtk3_resource_radiogroup_add_callback(group, video_size_callback);
    gtk_widget_set_margin_start(group, 8);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set user-defined callback to be triggered when the widget changes
 *
 * \param[in]   func    user-defined callback
 */
void pet_video_size_widget_set_callback(void (*func)(int))
{
    user_callback = func;
}


/** \brief  Synchronize \a widget with its current resource value
 *
 * \param[in,out]   widget  PET video size widget
 */
void pet_video_size_widget_sync(GtkWidget *widget)
{
    GtkWidget *group;

    group = gtk_grid_get_child_at(GTK_GRID(widget), 0, 1);
    if (group != NULL && GTK_IS_GRID(group)) {
        vice_gtk3_resource_radiogroup_sync(group);
    }
}
