/** \file   canvasrenderfilterwidget.c
 * \brief   Widget to select the Cario/OpenGL render filter
 *
 * \note    Normally I'd use 'gtkrenderfilterwidget.c' as the filename and
 *          gtk_render_filter_widget_' as the functions prefix, but that
 *          would invade the Gtk3 namespace, so 'canvas' it is for now.
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES CrtcGLFilter      xpet xcbm2
 * $VICERES TEDGLFilter       xplus4
 * $VICERES VDCGLFilter       x128
 * $VICERES VICGLFilter       xvic
 * $VICERES VICIIGLFilter     x64 x64sc xscpu64 xdtv64 x128 xcbm5x0
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
#include "video.h"

#include "canvasrenderfilterwidget.h"


/** \brief  List of Cairo/OpenGL render filters
 */
static const vice_gtk3_radiogroup_entry_t filters[] = {
    { "Nearest neighbor",   VIDEO_GLFILTER_NEAREST  },
    { "Bilinear",           VIDEO_GLFILTER_BILINEAR  },
    { "Bicubic",            VIDEO_GLFILTER_BICUBIC  },
    { NULL,                 -1 }
};

/** \brief  Create widget to select the Gtk render filter method
 *
 * \param[in]   chip    video chip prefix
 *
 * \return  GtkGrid
 */
GtkWidget *canvas_render_filter_widget_create(const char *chip)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *group;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>GL render filter</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    group = vice_gtk3_resource_radiogroup_new_sprintf("%sGLFilter",
                                                      filters,
                                                      GTK_ORIENTATION_VERTICAL,
                                                      chip);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}
