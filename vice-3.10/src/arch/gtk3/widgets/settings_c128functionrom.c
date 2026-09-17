/** \file   settings_c128functionrom.c
 * \brief   Settings widget to control C128 Function ROMs
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES InternalFunctionROM         x128
 * $VICERES InternalFunctionName        x128
 * $VICERES InternalFunctionROMRTCSave  x128
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

#include "vice.h"
#include <gtk/gtk.h>

#include "vice_gtk3.h"
#include "functionrom.h"

#include "settings_c128functionrom.h"


/** \brief  List of possible ROM bla things
 *
 * Seems to be the same for ext ROMS
 */
static const vice_gtk3_radiogroup_entry_t rom_types[] = {
    { "None",   INT_FUNCTION_NONE },    /* this one probably requires the
                                           text entry/browse button to be
                                           disabled */
    { "ROM",    INT_FUNCTION_ROM },
    { "RAM",    INT_FUNCTION_RAM },
    { "RTC",    INT_FUNCTION_RTC },
    { NULL, - 1 },
};


/** \brief  Create ROM type widget
 *
 * \param[in]   prefix  resource prefix
 *
 * \return  GtkGrid
 */
static GtkWidget *create_rom_type_widget(const char *prefix)
{
    GtkWidget *widget;

    widget = vice_gtk3_resource_radiogroup_new_sprintf("%sFunctionROM",
            rom_types, GTK_ORIENTATION_HORIZONTAL, prefix);
    gtk_grid_set_column_spacing(GTK_GRID(widget), 16);
    return widget;
}

/** \brief  Create ROM file selection widget
 *
 * \param[in]   prefix  resource prefix
 *
 * \return  GtkGrid
 */
static GtkWidget *create_rom_file_widget(const char *prefix)
{
    gchar buffer[256];

    g_snprintf(buffer, sizeof(buffer), "%sFunctionName", prefix);
    return vice_gtk3_resource_browser_new(buffer,
                                          NULL,
                                          NULL,
                                          "Select Function ROM image",
                                          NULL,
                                          NULL);
}

/** \brief  Create External/Internal ROM widget
 *
 * \param[in]   prefix  resource prefix
 *
 * \return GtkGrid
 */
static GtkWidget *create_rom_widget(const char *prefix)
{
    GtkWidget *grid;
    GtkWidget *type;
    GtkWidget *label;
    GtkWidget *rtc;
    char buffer[256];

    g_snprintf(buffer, sizeof(buffer), "%s Function ROM", prefix);
    grid = vice_gtk3_grid_new_spaced_with_label(-1, -1, buffer, 2);

    /* row 1: ROM type */
    label = gtk_label_new("ROM type");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 16);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    type = create_rom_type_widget(prefix);
    gtk_grid_attach(GTK_GRID(grid), type, 1, 1, 1, 1);

    /* row 2: ROM image browser */
    label = gtk_label_new("ROM file");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 16);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), create_rom_file_widget(prefix), 1, 2, 1, 1);

    rtc = vice_gtk3_resource_check_button_new_sprintf("%sFunctionROMRTCSave",
            "Save RTC data", prefix);
    gtk_widget_set_margin_start(rtc, 16);
    gtk_grid_attach(GTK_GRID(grid), rtc, 0, 3, 2, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create widget to select Internal/External function ROMs
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_c128functionrom_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *internal_widget;

    grid = vice_gtk3_grid_new_spaced(16, 32);

    internal_widget = create_rom_widget("Internal");
    gtk_grid_attach(GTK_GRID(grid), internal_widget, 0, 0, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
