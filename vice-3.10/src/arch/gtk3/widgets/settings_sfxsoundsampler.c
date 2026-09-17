/** \file   settings_sfxsoundsampler.c
 * \brief   Settings widget controlling SFX Sound Sampler resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES SFXSoundSampler         x64 x64sc xscpu64 x128 xvic
 * $VICERES SFXSoundSamplerIOSwap   xvic
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

#include "cartridge.h"
#include "machine.h"
#include "vice_gtk3.h"

#include "settings_sfxsoundsampler.h"


/** \brief  Handler for the "toggled" event of the Enable check button
 *
 * \param[in]   widget  SFX SS enable check button
 * \param[in]   io_swap I/O swap check button
 */
static void on_enable_toggled(GtkWidget *widget, gpointer io_swap)
{
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    gtk_widget_set_sensitive(GTK_WIDGET(io_swap), active);
}


/** \brief  Create SFX Sound Sampler widget
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_sfxsoundsampler_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *enable;
    int        cart_id;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    if (machine_class == VICE_MACHINE_VIC20) {
        cart_id = CARTRIDGE_VIC20_SFX_SOUND_SAMPLER;
    } else {
        cart_id = CARTRIDGE_SFX_SOUND_SAMPLER;
    }
    enable = carthelpers_create_enable_check_button(CARTRIDGE_NAME_SFX_SOUND_SAMPLER,
                                                    cart_id);
    gtk_grid_attach(GTK_GRID(grid), enable, 0, 0, 1, 1);

    if (machine_class == VICE_MACHINE_VIC20) {
        GtkWidget *io_swap;
        gboolean   active;

        io_swap = vice_gtk3_resource_check_button_new("SFXSoundSamplerIOSwap",
                                                      "Enable MasC=uerade I/O swap");
        gtk_grid_attach(GTK_GRID(grid), io_swap, 0, 1, 1, 1);

        /* connect handler to the enable check button to set sensitivity of the
         * I/O swap check button */
        g_signal_connect_unlocked(G_OBJECT(enable),
                                  "toggled",
                                  G_CALLBACK(on_enable_toggled),
                                  (gpointer)io_swap);

        /* set initial sensitivity of I/O swap */
        active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enable));
        gtk_widget_set_sensitive(io_swap, active);
    }

    gtk_widget_show_all(grid);
    return grid;
}
