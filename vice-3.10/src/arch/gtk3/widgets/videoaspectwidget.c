/** \file   videoaspectwidget.c
 * \brief   GTK3 widget to select renderer filter
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 * \author  groepaz <groepaz@gmx.net>
 */

/*
 * $VICERES CrtcAspectMode      xpet xcbm2
 * $VICERES TEDAspectMode       xplus4
 * $VICERES VDCAspectMode       x128
 * $VICERES VICAspectMode       xvic
 * $VICERES VICIIAspectMode     x64 x64sc xscpu64 xdtv64 x128 xcbm5x0
 * $VICERES CrtcAspectRatio     xpet xcbm2
 * $VICERES TEDAspectRatio      xplus4
 * $VICERES VDCAspectRatio      x128
 * $VICERES VICAspectRatio      xvic
 * $VICERES VICIIAspectRatio    x64 x64sc xscpu64 xdtv64 x128 xcbm5x0
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
#include "debug_gtk3.h"
#include "resources.h"
#include "video.h"

#include "videoaspectwidget.h"

/** \brief  Aspect ratio spin button lower bound */
#define RATIO_LOWER     0.5

/** \brief  Aspect ratio spin button upper bound */
#define RATIO_UPPER     2.0

/** \brief  Aspect ratio spin button stepping */
#define RATIO_STEP      0.0069

/** \brief  Aspect ratio spin button displayed digits
 *
 * A GtkSpinButton is limited to 20 digits, should be enough.
 */
#define RATIO_DIGITS    4


/** \brief  List of radio buttons
 */
static const vice_gtk3_radiogroup_entry_t aspect_modes[] = {
    { "Off",        VIDEO_ASPECT_MODE_NONE },
    { "Custom:",    VIDEO_ASPECT_MODE_CUSTOM },
    { "True",       VIDEO_ASPECT_MODE_TRUE },
    { NULL,         -1 }
};


/** \brief  Create widget to control render filter resources
 *
 * \param[in]   chip    video chip prefix
 *
 * \return  GtkGrid
 */
GtkWidget *video_aspect_widget_create(const char *chip)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *group;
    GtkWidget *aspect_ratio;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Aspect Mode</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    group = vice_gtk3_resource_radiogroup_new_sprintf("%sAspectMode",
                                                      aspect_modes,
                                                      GTK_ORIENTATION_VERTICAL,
                                                      chip);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);

    /* custom aspect ratio spin button */
    aspect_ratio = vice_gtk3_resource_spin_double_new_sprintf("%sAspectRatio",
                                                              RATIO_LOWER,
                                                              RATIO_UPPER,
                                                              RATIO_STEP,
                                                              chip);
    vice_gtk3_resource_spin_double_set_digits(aspect_ratio, RATIO_DIGITS);
    gtk_widget_set_margin_start(aspect_ratio, 8);
    gtk_widget_set_hexpand(aspect_ratio, FALSE);
    /* put spin button in the radio button group grid, next to "Custom" */
    gtk_grid_attach(GTK_GRID(group), aspect_ratio, 1, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set callback function to trigger on radio button toggles
 *
 * \param[in,out]   widget      render filter widget
 * \param[in]       callback    function accepting the radio button and its value
 */
void video_aspect_widget_add_callback(GtkWidget  *widget,
                                      void      (*callback)(GtkWidget *, int))
{
    GtkWidget *group;

    group = gtk_grid_get_child_at(GTK_GRID(widget), 0, 1);
    vice_gtk3_resource_radiogroup_add_callback(group, callback);
}
