/** \file   drivesoundwidget.c
 * \brief   Drive sound emulation settings widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES DriveSoundEmulation         -vsid
 * $VICERES DriveSoundEmulationVolume   -vsid
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

#include "drive.h"
#include "drivesoundwidget.h"


/** \brief  Format the drive emulation volume value
 *
 * Turns the volume scale into 0-100%.
 *
 * \param[in]   scale   drive volume widget
 * \param[in]   value   value of widget to format
 *
 * \return  heap-allocated string representing the current value
 *
 * \note    the string returned here appears to be deallocated by Gtk3. if
 *          I assume the code example of Gtk3 is correct
 */
static gchar *on_drive_volume_format_value(GtkScale *scale, gdouble value)
{
    return g_strdup_printf("%d%%", (int)(value / 40));
}


/** \brief  Create drive sound emulation settings widget
 *
 * Create grid with a check button for sound emulation and a scale for volume.
 *
 * \return  GtkGrid
 */
GtkWidget *drive_sound_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *check;
    GtkWidget *label;
    GtkWidget *scale;

    grid  = vice_gtk3_grid_new_spaced(8, 0);
    check = vice_gtk3_resource_check_button_new("DriveSoundEmulation",
                                                "Drive sound emulation");
    label = gtk_label_new("Drive sound volume");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_widget_set_margin_start(label, 32);
    scale = vice_gtk3_resource_scale_int_new("DriveSoundEmulationVolume",
                                             GTK_ORIENTATION_HORIZONTAL,
                                             0, DRIVE_SOUND_VOLUME_MAX, 200);
    gtk_widget_set_hexpand(scale, TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
    g_signal_connect_unlocked(scale,
                              "format-value",
                              G_CALLBACK(on_drive_volume_format_value),
                              NULL);

    gtk_grid_attach(GTK_GRID(grid), check, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), scale, 2, 0, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}
