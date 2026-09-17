/** \file   settings_magicvoice.c
 * \brief   Settings widget controlling Magic Voice resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES MagicVoiceCartridgeEnabled  x64 x64sc xscpu64 x128
 * $VICERES MagicVoiceImage             x64 x64sc xscpu64 x128
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
#include "vice_gtk3.h"

#include "settings_magicvoice.h"


/** \brief  Create widget to control MagicVoice cartridge
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_magicvoice_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *enable;
    GtkWidget *label;
    GtkWidget *chooser;
    const char *title;
    const char *patterns[] = { "*.bin", "*.rom", NULL };

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    /* enable checkbutton */
    enable = carthelpers_create_enable_check_button(CARTRIDGE_NAME_MAGIC_VOICE,
                                                    CARTRIDGE_MAGIC_VOICE);
    gtk_grid_attach(GTK_GRID(grid), enable, 0, 0, 2, 1);

    /* image browser widget */
    label = gtk_label_new("ROM image file");
    gtk_widget_set_margin_start(label, GTK_ALIGN_START);
    title   = "Select " CARTRIDGE_NAME_MAGIC_VOICE " ROM image file";
    chooser = vice_gtk3_resource_filechooser_new("MagicVoiceImage",
                                                 GTK_FILE_CHOOSER_ACTION_OPEN);
    vice_gtk3_resource_filechooser_set_custom_title(chooser, title);
    vice_gtk3_resource_filechooser_set_filter(chooser,
                                              "ROM images",
                                              patterns,
                                              TRUE);

    gtk_grid_attach(GTK_GRID(grid), label,   0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), chooser, 1, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
