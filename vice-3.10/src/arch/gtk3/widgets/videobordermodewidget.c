/** \file   videobordermodewidget.c
 * \brief   GTK3 widget to select border mode
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES TEDBorderMode       xplus4
 * $VICERES VICBorderMode       xvic
 * $VICERES VICIIBorderMode     x64 x64sc xscpu64 x64dtv x128 cbm5x0
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
#include "basewidgets.h"
#include "widgethelpers.h"
#include "resources.h"
#include "video.h"

#include "videobordermodewidget.h"


/** \brief  List of radio buttons
 *
 * Since all ${CHIP}_[NORMAL|FULL|DEBUG|NO]_BORDERS constants are the same,
 * I've decided to use simple numeric constants to avoid having multiple lists
 * for each $CHIP with the same values.
 */
static const vice_gtk3_radiogroup_entry_t modes[] = {
    { "Normal",  0 },   /* chip_NORMAL_BORDERS */
    { "Full",    1 },   /* chip_FULL_BORDERS */
    { "Debug",   2 },   /* chip_DEBUG_BORDERS */
    { "None",    3 },   /* chip_NO_BORDERS */
    { NULL,     -1 }
};


/** \brief  Create widget to set border mode
 *
 * \param[in]   chip    video chip prefix
 *
 * \return  GtkGrid
 */
GtkWidget *video_border_mode_widget_create(const char *chip)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *mode;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Border mode</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    mode = vice_gtk3_resource_radiogroup_new_sprintf("%sBorderMode",
                                                     modes,
                                                     GTK_ORIENTATION_VERTICAL,
                                                     chip);
    gtk_grid_attach(GTK_GRID(grid), mode, 0, 1, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}
