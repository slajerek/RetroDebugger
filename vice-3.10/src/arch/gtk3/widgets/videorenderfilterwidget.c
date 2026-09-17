/** \file   videorenderfilterwidget.c
 * \brief   GTK3 widget to select renderer filter
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES CrtcFilter      xpet xcbm2
 * $VICERES TEDFilter       xplus4
 * $VICERES VDCFilter       x128
 * $VICERES VICFilter       xvic
 * $VICERES VICIIFilter     x64 x64sc xscpu64 xdtv64 x128 xcbm5x0
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

#include "debug_gtk3.h"
#include "resources.h"
#include "vice_gtk3.h"
#include "video.h"

#include "videorenderfilterwidget.h"



/** \brief  List of radio buttons
 */
static const vice_gtk3_radiogroup_entry_t filters[] = {
    { "Unfiltered",     VIDEO_FILTER_NONE },
    { "CRT emulation",  VIDEO_FILTER_CRT },
    { "Scale2x",        VIDEO_FILTER_SCALE2X },
    { NULL,             -1 }
};


/** \brief  Create widget to control render filter resources
 *
 * \param[in]   chip    video chip prefix
 *
 * \return  GtkGrid
 */
GtkWidget *video_render_filter_widget_create(const char *chip)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *render;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Render filter</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    render = vice_gtk3_resource_radiogroup_new_sprintf("%sFilter",
                                                       filters,
                                                       GTK_ORIENTATION_VERTICAL,
                                                       chip);
    gtk_grid_attach(GTK_GRID(grid), render, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set callback function to trigger on radio button toggles
 *
 * \param[in,out]   widget      render filter widget
 * \param[in]       callback    function accepting the radio button and its value
 */
void video_render_filter_widget_add_callback(GtkWidget *widget,
                                             void (*callback)(GtkWidget *, int))
{
    GtkWidget *group;

    group = gtk_grid_get_child_at(GTK_GRID(widget), 0, 1);
    vice_gtk3_resource_radiogroup_add_callback(group, callback);
}
