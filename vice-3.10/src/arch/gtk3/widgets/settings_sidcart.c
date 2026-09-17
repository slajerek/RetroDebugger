/** \file   settings_sidcart.c
 * \brief   Settings widget for SID Cart (Plus4/PET)
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES SidCart     xvic xplus4 xpet
 * $VICERES SidAddress  xvic xplus4 xpet
 * $VICERES SidClock    xvic xplus4 xpet
 * $VICERES SIDCartJoy  xplus4
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

#include "archdep.h"
#include "machine.h"
#include "sidcart.h"
#include "sidmodelwidget.h"
#include "vice_gtk3.h"

#include "settings_sidcart.h"


/* References to widgets to enable/disable depending on the "SidCart" resource
 */

/** \brief  SID cart enable toggle button */
static GtkWidget *sidcart_enable = NULL;

/** \brief  SID model radiogroup */
static GtkWidget *sid_model = NULL;

/** \brief  SID I/O address combobox */
static GtkWidget *sid_address = NULL;

/** \brief  SID clock radiogroup */
static GtkWidget *sid_clock = NULL;

/** \brief  SID cart joyport emulation checkbox
 *
 * Plus4 only: emulate the joyport on the SID cart.
 */
static GtkWidget *sid_joy = NULL;


/** \brief  SID cart I/O base addresses for VIC-20
 */
static const vice_gtk3_radiogroup_entry_t sid_base_vic20[] = {
    { "$9800", 0x9800 },
    { "$9C00", 0x9c00 },
    { NULL,        -1 }
};

/** \brief  SID cart I/O base addresses for Plus4
 */
static const vice_gtk3_radiogroup_entry_t sid_base_plus4[] = {
    { "$FD40", 0xfd40 },
    { "$FE80", 0xfe80 },
    { NULL,        -1 }
};

/** \brief  SID cart I/O base addresses for PET
 */
static const vice_gtk3_radiogroup_entry_t sid_base_pet[] = {
    { "$8F00", 0x8f00 },
    { "$E900", 0xe900 },
    { NULL,        -1 }
};

/** \brief  SID cart clock for VIC-20
 */
static const vice_gtk3_radiogroup_entry_t sid_clock_vic20[] = {
    { "C64",    SIDCART_CLOCK_C64 },
    { "VIC-20", SIDCART_CLOCK_NATIVE },
    { NULL,     -1 }
};

/** \brief  SID cart clock for Plus4
 */
static const vice_gtk3_radiogroup_entry_t sid_clock_plus4[] = {
    { "C64",   SIDCART_CLOCK_C64 },
    { "Plus4", SIDCART_CLOCK_NATIVE },
    { NULL,    -1 }
};

/** \brief  SID cart clock for PET
 */
static const vice_gtk3_radiogroup_entry_t sid_clock_pet[] = {
    { "C64", SIDCART_CLOCK_C64 },
    { "PET", SIDCART_CLOCK_NATIVE },
    { NULL,  -1 }
};


/** \brief  Set sensitivity of widgets
 *
 * \param[in]   sensitive   new sensitivity
 */
static void set_widgets_sensitivity(gboolean sensitive)
{
    gtk_widget_set_sensitive(sid_model,   sensitive);
    gtk_widget_set_sensitive(sid_address, sensitive);
    gtk_widget_set_sensitive(sid_clock,   sensitive);
    if (machine_class == VICE_MACHINE_PLUS4) {
        gtk_widget_set_sensitive(sid_joy, sensitive);
    }
}

/** \brief  Handler for the 'toggled' event of the SidCart enable widget
 *
 * Enables/disables the model, address and clock widgets depending on the
 * SidCart enabled state.
 *
 * \param[in]   widget      SidCart toggle button
 * \param[in]   user_data   unused
 */
static void on_sidcart_enable_toggled(GtkWidget *widget, gpointer user_data)
{
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    set_widgets_sensitivity(active);
}

/** \brief  Create left-aligned label using Pango markup
 *
 * \param[in]   text    text for label
 *
 * \return  GtkLabel
 */
static GtkWidget *label_helper(const char *text)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}

/** \brief  Create toggle button to switch the "SidCart" resource
 *
 * \return  GtkGrid
 */
static GtkWidget *create_sidcart_enable_widget(void)
{
    return vice_gtk3_resource_check_button_new("SidCart",
                                               "Enable SID Card emulation");
}

/** \brief  Create widget to set SID I/O base address
 *
 * \return  GtkGrid
 */
static GtkWidget *create_sidcart_address_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *group;
    const vice_gtk3_radiogroup_entry_t *list;

    switch (machine_class) {
        case VICE_MACHINE_VIC20:
            list = sid_base_vic20;
            break;
        case VICE_MACHINE_PLUS4:
            list = sid_base_plus4;
            break;
        case VICE_MACHINE_PET:
            list = sid_base_pet;
            break;
        default:
            list = NULL;
            archdep_vice_exit(1);
            break;
    }

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = label_helper("<b>SID address</b>");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    group = vice_gtk3_resource_radiogroup_new("SidAddress",
                                              list,
                                              GTK_ORIENTATION_VERTICAL);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create widget to set SID clock
 *
 * \return  GtkGrid
 */
static GtkWidget *create_sidcart_clock_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *group;
    const vice_gtk3_radiogroup_entry_t *list;

    switch (machine_class) {
        case VICE_MACHINE_VIC20:
            list = sid_clock_vic20;
            break;
        case VICE_MACHINE_PLUS4:
            list = sid_clock_plus4;
            break;
        case VICE_MACHINE_PET:
            list = sid_clock_pet;
            break;
        default:
            list = NULL;
            archdep_vice_exit(1);
            break;
    }

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = label_helper("<b>SID clock</b>");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    group = vice_gtk3_resource_radiogroup_new("SidClock",
                                              list,
                                              GTK_ORIENTATION_VERTICAL);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create SidCart joyport emulation widget (plus4 only)
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_sidcart_joy_widget(void)
{
    return vice_gtk3_resource_check_button_new("SIDCartJoy",
                                               "Enable joystick port emulation");
}


/** \brief  Create widget to conrol SID cartridge settings
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 */
GtkWidget *settings_sidcart_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    gboolean   active;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    sidcart_enable = create_sidcart_enable_widget();
    gtk_widget_set_margin_bottom(sidcart_enable, 16);
    gtk_grid_attach(GTK_GRID(grid), sidcart_enable, 0, 0, 3, 1);

    sid_model = sid_model_widget_create(NULL);
    gtk_grid_attach(GTK_GRID(grid), sid_model, 0, 1, 1, 1);

    sid_address = create_sidcart_address_widget();
    gtk_grid_attach(GTK_GRID(grid), sid_address, 1, 1, 1, 1);

    sid_clock = create_sidcart_clock_widget();
    gtk_grid_attach(GTK_GRID(grid), sid_clock, 2, 1, 1, 1);

    if (machine_class == VICE_MACHINE_PLUS4) {
        sid_joy = create_sidcart_joy_widget();
        gtk_widget_set_margin_top(sid_joy, 16);
        gtk_grid_attach(GTK_GRID(grid), sid_joy, 0, 2, 3, 1);
    }

    /* doesn't touch any resources, so can connected unlocked */
    g_signal_connect_unlocked(sidcart_enable,
                              "toggled",
                              G_CALLBACK(on_sidcart_enable_toggled),
                              NULL);

    /* set senstitivity of widgets */
    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sidcart_enable));
    set_widgets_sensitivity(active);

    gtk_widget_show_all(grid);
    return grid;
}
