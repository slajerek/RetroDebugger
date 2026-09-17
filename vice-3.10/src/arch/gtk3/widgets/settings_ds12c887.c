/** \file   settings_ds12c887.c
 * \brief   Settings widget controlling DS12C887 RTC resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES DS12C887RTC         x64 x64sc xscpu64 x128 xvic
 * $VICERES DS12C887RTCbase     x64 x64sc xscpu64 x128 xvic
 * $VICERES DS12C887RTCSave     x64 x64sc xscpu64 x128 xvic
 * $VICERES DS12C887RTCRunMode  x64 x64sc xscpu64 x128 xvic
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
#include "debug_gtk3.h"
#include "machine.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "settings_ds12c887.h"


/** \brief  Values for I/O base on C64/C128
 *
 * The hardware appears to allow $d100, $d200 and $d300 as I/O-base but that's
 * not emulated and thus not in this list.
 */
static const int io_base_c64[] = { 0xd500, 0xd600, 0xd700, 0xde00, 0xdf00, -1 };

/** \brief  Values for I/O base on VIC-20
 */
static const int io_base_vic20[] = { 0x9800, 0x9c00, -1 };


/** \brief  Reference to the oscilattor checkbox */
static GtkWidget *oscil_widget = NULL;
/** \brief  Reference to the base widget */
static GtkWidget *base_widget = NULL;
/** \brief  Reference to the 'RTC' widget */
static GtkWidget *rtc_widget = NULL;


/** \brief  Set sensitivity of widgets
 *
 * \param[in]   sensitive   sensitivity
 */
static void set_widgets_sensitivity(gboolean sensitive)
{
    gtk_widget_set_sensitive(oscil_widget, sensitive);
    gtk_widget_set_sensitive(base_widget,  sensitive);
    gtk_widget_set_sensitive(rtc_widget,   sensitive);
}

/** \brief  Handler for the "toggled" event of the enable widget
 *
 * \param[in,out]   widget      "enable" toggle button
 * \param[in]       user_data   extra event data (unused)
 */
static void on_enable_toggled(GtkWidget *widget, gpointer user_data)
{
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    set_widgets_sensitivity(active);
}

/** \brief  Create widget to set I/O base for the RTC thing
 *
 * Sets the correct I/O base list for the current machine
 *
 * \return  GktComboBox
 */
static GtkWidget *create_base_widget(void)
{
    return vice_gtk3_resource_combo_hex_new_list("DS12C887RTCbase",
                                                 machine_class == VICE_MACHINE_VIC20
                                                 ? io_base_vic20 : io_base_c64);
}


/** \brief  Create widget to control the DS12C887 RTC
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 */
GtkWidget *settings_ds12c887_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *enable;
    GtkWidget *label;
    gboolean   active;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    enable = carthelpers_create_enable_check_button(CARTRIDGE_NAME_DS12C887RTC,
                                                    CARTRIDGE_DS12C887RTC);
    oscil_widget = vice_gtk3_resource_check_button_new("DS12C887RTCRunMode",
                                                       "Start with running oscillator");
    rtc_widget = vice_gtk3_resource_check_button_new("DS12C887RTCSave",
                                                     "Enable RTC Saving");
    gtk_grid_attach(GTK_GRID(grid), enable,       0, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), oscil_widget, 0, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), rtc_widget,   0, 2, 2, 1);

    label = gtk_label_new("I/O base address");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    base_widget = create_base_widget();
    gtk_grid_attach(GTK_GRID(grid), label,       0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), base_widget, 1, 3, 1, 1);

    g_signal_connect_unlocked(G_OBJECT(enable),
                              "toggled",
                              G_CALLBACK(on_enable_toggled),
                              NULL);
    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enable));
    set_widgets_sensitivity(active);

    gtk_widget_show_all(grid);
    return grid;
}
