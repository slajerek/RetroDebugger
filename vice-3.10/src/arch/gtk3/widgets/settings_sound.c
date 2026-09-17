/** \file   settings_sound.c
 * \brief   Sound settings main widget for the settings dialog
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES Sound                   all
 *
 * See included widgets for more.
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

/*
 * $VICERES SoundEmulateOnWarp
 */

#include "vice.h"
#include <gtk/gtk.h>
#include "sound.h"

#include "soundbuffersizewidget.h"
#include "sounddriverwidget.h"
#include "soundfragmentsizewidget.h"
#include "soundoutputmodewidget.h"
#include "soundsampleratewidget.h"
#include "vice_gtk3.h"

#include "settings_sound.h"


/** \brief  Create the 'inner' grid, the one containing all the widgets
 *
 * \return  GtkGrid
 */
static GtkWidget *create_inner_grid(void)
{
    GtkWidget *grid;
    GtkWidget *driver;

    grid = vice_gtk3_grid_new_spaced(8, 0);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    /* row 0, columns 0 & 1 */
    driver = sound_driver_widget_create();
    gtk_widget_set_margin_bottom(driver, 16);
    gtk_grid_attach(GTK_GRID(grid),
                    driver,
                    0, 0, 4, 1);

    /* row 1, column 0 */
    gtk_grid_attach(GTK_GRID(grid),
                    sound_output_mode_widget_create(),
                    0, 1, 1, 1);

    /* row 1, column 1 */
    gtk_grid_attach(GTK_GRID(grid),
                    sound_sample_rate_widget_create(),
                    1, 1, 1, 1);

    /* row 1, columm 2 */
    gtk_grid_attach(GTK_GRID(grid),
                    sound_buffer_size_widget_create(),
                    2, 1, 1, 1);

    /* row 1, columm 3 */
    gtk_grid_attach(GTK_GRID(grid),
                    sound_fragment_size_widget_create(),
                    3, 1, 1, 1);

    return grid;
}



/** \brief  Create sound settings widget for use as a 'central' settings widget
 *
 * \param[in]   widget  parent widget
 *
 * \return  grid with sound settings widgets
 */
GtkWidget *settings_sound_widget_create(GtkWidget *widget)
{
    GtkWidget *outer;
    GtkWidget *inner;
    GtkWidget *scale;
    GtkWidget *enabled_check;
    GtkWidget *warp_enabled_check;
    GtkWidget *label;

    /* outer grid: contains the checkbox and an 'inner' grid for the widgets */
    outer = gtk_grid_new();

    /* add checkbox for 'sound enabled' */
    enabled_check = vice_gtk3_resource_check_button_new("Sound",
                                                        "Enable sound emulation");
    gtk_grid_attach(GTK_GRID(outer), enabled_check, 0, 0, 2, 1);

    warp_enabled_check = vice_gtk3_resource_check_button_new("SoundEmulateOnWarp",
        "Enable sound emulation in warp mode. (Disabling has a negative impact on compatibility)");
    gtk_grid_attach(GTK_GRID(outer), warp_enabled_check, 0, 1, 2, 1);

    label = gtk_label_new("Master volume");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_end(label, 8);    /* a litte more space to avoid the
                                               slider handle touching the text */
    scale = vice_gtk3_resource_scale_custom_new_printf("%s",
                                                       GTK_ORIENTATION_HORIZONTAL,
                                                       0,
                                                       MASTER_VOLUME_MAX,
                                                       0,
                                                       (MASTER_VOLUME_MAX * 100) / MASTER_VOLUME_ONE,
                                                       5,
                                                       "%3.0f%%",
                                                       "SoundVolume");
    gtk_widget_set_hexpand(scale, TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
    gtk_grid_attach(GTK_GRID(outer), label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(outer), scale, 1, 2, 1, 1);
    /* inner grid: contains widgets and can be enabled/disabled depending on
     * the state of the 'sound enabled' checkbox */
    inner = create_inner_grid();
    gtk_widget_set_margin_top(inner, 16);

    gtk_grid_attach(GTK_GRID(outer), inner, 0, 3, 2, 1);
    gtk_widget_show_all(outer);
    return outer;
}
