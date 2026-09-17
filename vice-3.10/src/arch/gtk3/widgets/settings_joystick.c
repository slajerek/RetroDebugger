/** \file   settings_joystick.c
 * \brief   Widget to control settings for joysticks
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 */

/*
 * $VICERES JoyDevice1      -xcbm2 -xpet -vsid
 * $VICERES JoyDevice2      -xcbm2 -xpet -xvic -vsid
 * $VICERES JoyDevice3      -vsid
 * $VICERES JoyDevice4      -vsid
 * $VICERES JoyDevice5      -vsid -xpet
 * $VICERES JoyDevice6      -xplus4 -xpet -vsid
 * $VICERES JoyDevice7      -xplus4 -xpet -vsid
 * $VICERES JoyDevice8      -xplus4 -xpet -vsid
 * $VICERES JoyDevice9      -xplus4 -xpet -vsid
 * $VICERES JoyDevice10     -xplus4 -xpet -vsid
 * $VICERES JoyDevice11     -xplus4
 *
 * Only altered when two control ports are available, so not for xvic:
 * $VICERES JoyPort1Device  -xcbm2 -xpet -xvic -vsid
 * $VICERES JoyPort2Device  -xcbm2 -xpet -xvic -vsid
 *
 * $VICERES JoyOpposite     -vsid
 * $VICERES KeySetEnable    -vsid
 *
 *  (see used external widgets for more)
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
#include <stdlib.h>

#include "actions-joystick.h"
#include "debug_gtk3.h"
#include "joyport.h"
#include "joystick.h"
#include "joystickdevicewidget.h"
#include "keysetdialog.h"
#include "lib.h"
#include "machine.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "vice_gtk3.h"

#include "settings_joystick.h"


/** \brief  Number of joystick adapter ports for C64
 */
#define ADAPTER_PORT_COUNT_C64      8

/** \brief  Number of joystick adapter ports for C64 DTV
 */
#define ADAPTER_PORT_COUNT_C64DTV   8

/** \brief  Number of joystick adapter ports for SCPU64
 */
#define ADAPTER_PORT_COUNT_SCPU64   8

/** \brief  Number of joystick adapter ports for C128
 */
#define ADAPTER_PORT_COUNT_C128     8

/** \brief  Number of joystick adapter ports for VIC-20
 */
#define ADAPTER_PORT_COUNT_VIC20    8

/** \brief  Number of joystick adapter ports for Plus4
 */
#define ADAPTER_PORT_COUNT_PLUS4    8

/** \brief  Number of joystick adapter ports for CBM-II 5x0/P
 */
#define ADAPTER_PORT_COUNT_CBM5x0   8

/** \brief  Number of joystick adapter ports for CBM-II 6x0/7x0/B
 */
#define ADAPTER_PORT_COUNT_CBM6x0   8

/** \brief  Number of joystick adapter ports for PET
 */
#define ADAPTER_PORT_COUNT_PET   3

/*
 * Static data
 */

/** \brief  References to the joystick device widgets
 *
 * These references are used to update the UI when swapping joysticks.
 */
static GtkWidget *device_widgets[JOYPORT_MAX_PORTS];


/*****************************************************************************
 *                                Event handlers                             *
 ****************************************************************************/

/** \brief  Handler for the 'toggled' event of the "Swap joysticks" button
 *
 * Swaps resources JoyDevice1/JoyDevice2 and JoyPort1Device/JoyPort2Device and
 * updates the UI accordingly.
 *
 * \param[in]   button  toggle button
 * \param[in]   data    extra event data (unused)
 */
static void on_swap_joysticks_toggled(GtkWidget *button, gpointer data)
{
    int joy1 = -1;
    int joy2 = -1;

    ui_action_trigger(ACTION_SWAP_CONTROLPORT_TOGGLE);

    /* make sure to set the correct state, swapping might fail due to certain
     * devices not being allowed on certain ports */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                 ui_get_controlport_swapped());

    /* get current values */
    resources_get_int_sprintf("JoyDevice%d", &joy1, 1);
    resources_get_int_sprintf("JoyDevice%d", &joy2, 2);

    /* updating the widgets (triggers updating the resources but since they're
     * the same nothing will happen) */
    joystick_device_widget_update(device_widgets[JOYPORT_1], joy1);
    joystick_device_widget_update(device_widgets[JOYPORT_2], joy2);
}

/** \brief  Handler for the 'clicked' event of the "Configure keysets" button
 *
 * \param[in]   widget  button (unused)
 * \param[in]   data    keyset number (`int`)
 */
static void on_keyset_configure_button_clicked(GtkWidget *widget, gpointer data)
{
    keyset_dialog_show(GPOINTER_TO_INT(data));
}


/*****************************************************************************
 *                      Functions to create simple widgets                   *
 ****************************************************************************/

/** \brief  Create a button to swap joysticks #1 and #2
 *
 * \return  GtkButton
 */
static GtkWidget *create_swap_joysticks_button(void)
{
    GtkWidget *button;
#if 0
    button = gtk_button_new_with_label("Swap Joysticks");
    g_signal_connect(button, "clicked", G_CALLBACK(on_swap_joysticks_clicked),
            NULL);
#endif
    button = gtk_check_button_new_with_label("Swap joysticks");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                 ui_get_controlport_swapped());
    g_signal_connect(button,
                     "toggled",
                     G_CALLBACK(on_swap_joysticks_toggled),
                     NULL);

    gtk_widget_set_vexpand(button, FALSE);
    gtk_widget_set_valign(button, GTK_ALIGN_END);
    gtk_widget_show(button);
    return button;
}

/** \brief  Create a check button to enable "user-defined keysets"
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_keyset_enable_checkbox(void)
{
    return vice_gtk3_resource_check_button_new("KeySetEnable",
                                               "Allow keyset joysticks");
}

/** \brief  Create a check button to enable "Allow opposite directions"
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_opposite_enable_checkbox(void)
{
    return vice_gtk3_resource_check_button_new("JoyOpposite",
                                               "Allow opposite directions");
}

/** \brief  Create button to pop up dialog to configure a keyset
 *
 * \param[in]   keyset  keyset number (1 for 'A', 2 for 'B')
 *
 * \return  GtkButton
 */
static GtkWidget *create_keyset_configure_button(int keyset)
{
    GtkWidget *button;
    char       label[64];

    g_snprintf(label, sizeof label, "Configure keyset _%c", 'A' - 1 + keyset);
    button = gtk_button_new_with_mnemonic(label);
    gtk_widget_set_halign(button, GTK_ALIGN_START);
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_keyset_configure_button_clicked),
                     GINT_TO_POINTER(keyset));
    return button;
}

/*
 * Functions to create machine-specific layouts
 */

/** \brief  Add control port joysticks
 *
 * Add widgets to the \a layout for the control port joysticks.
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     starting row in \a layout
 * \param[in]       count   number of widgets to add to the \a layout
 *
 * \return  row in the \a layout for additional widgets
 */
static int layout_add_control_ports(GtkGrid *layout, int row, int count)
{
    if (count <= 0) {
        return row;
    }

    device_widgets[JOYPORT_1] = joystick_device_widget_create(
            JOYPORT_1, "Joystick #1");
    gtk_grid_attach(GTK_GRID(layout),
                    device_widgets[JOYPORT_1],
                    0, row, 1, 1);
    /* add spacing */
    gtk_widget_set_margin_bottom(device_widgets[JOYPORT_1], 8);

    /* control port #2 */
    if (count > 1) {
        device_widgets[JOYPORT_2] = joystick_device_widget_create(
            JOYPORT_2, "Joystick #2");
        gtk_grid_attach(GTK_GRID(layout),
                        device_widgets[JOYPORT_2],
                        1, row, 1, 1);
    }

    return row + 1;
}

/** \brief  Add Joystick Adapter ports
 *
 * Add widgets to the \a layout for the joystick adapter ports.
 *
 * The ports start numbered at 1, but the internal resources ("JoyDeviceX")
 * start at 3 ("JoyDevice3").
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     starting row in \a layout
 * \param[in]       count   number of widgets to add to the \a layout
 *
 * \return  row in the \a layout for additional widgets
 */
static int layout_add_adapter_ports(GtkGrid *layout, int row, int count)
{
    int i;
    int r = row;
    int c = 0;
    int d = JOYPORT_3;  /* device number*/

    for (i = 0; i < count; i++) {

        char label[256];

        if (joyport_has_mapping(d)) {
            g_snprintf(label, sizeof(label), "Joystick Adapter Port #%d", i + 1);
            device_widgets[i + 2] = joystick_device_widget_create(d, label);
            gtk_grid_attach(layout, device_widgets[i + 2], c, r, 1, 1);
            gtk_widget_set_margin_bottom(device_widgets[i + 2], 8);
        }
        d++;
        c ^= 1; /* switch column position */
        if (c == 0) {
            r++;    /* update row position */
        }
    }

    /* if we have an odd number of ports, the row number would not have been
     * updated when exiting the loop, so we add 1 by using the column index
     * `c` (which happens to be 1 in that case) */
    return r + 1 + c;
}

/** \brief  Add SIDCard joystick port
 *
 * Add widget to the \a layout for the SIDVCard joystick.
 *
 * This device is currently mappend to JOYDEV_5 (resource "JoyDevice5").
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     starting row in \a layout
 *
 * \return  row in the \a layout for additional widgets
 */
static int layout_add_sidcard_port(GtkGrid *layout, int row)
{
    /* not JOYPORT_PLUS4_SIDCART because the API is retarded: JOYPORT_PLUS4_SIDCART == JOYPORT_11 == 10 */
    if (joyport_has_mapping(10)) {
        device_widgets[JOYPORT_PLUS4_SIDCART] = joystick_device_widget_create(
                JOYPORT_PLUS4_SIDCART, "SIDCard Joystick");
        gtk_grid_attach(layout, device_widgets[JOYPORT_PLUS4_SIDCART], 0, row, 1, 1);
    }
    return row + 1;
}

/** \brief  Add "Swap joystick" button
 *
 * Add widget to the \a layout to swap control port
 *
 * \param[in,out]   layout      main widget grid
 * \param[in]       row         starting row in \a layout
 *
 * \return  row in the \a layout for additional widgets
 */
static int layout_add_swap_button(GtkGrid *layout, int row)
{
    GtkWidget *button = create_swap_joysticks_button();
    gtk_widget_set_margin_top(button, 8);
    gtk_grid_attach(GTK_GRID(layout), button, 0, row, 1, 1);
    return row + 1;
}

/** \brief  Create layout for x64/x64sc/xscpu64/x128
 *
 * \param[in,out]   grid    main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_c64_layout(GtkGrid *grid)
{
    int row = 0;

    row = layout_add_control_ports(grid, row, 2);
    row = layout_add_adapter_ports(grid, row, ADAPTER_PORT_COUNT_C64);
    row = layout_add_swap_button(grid, row);

    return row;
}

/** \brief  Create layout for x64dtv
 *
 * \param[in,out]   grid    main widget grid
 *
 * \return  number of rows added to \a grid
 */
static int create_c64dtv_layout(GtkGrid *grid)
{
    int row = 0;

    row = layout_add_control_ports(grid, row, 2);
    row = layout_add_adapter_ports(grid, row, ADAPTER_PORT_COUNT_C64DTV);
    row = layout_add_swap_button(grid, row);

    return row;
}


/** \brief  Create layout for xvic
 *
 * \param[in,out]   grid    main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_vic20_layout(GtkGrid *grid)
{
    int row = 0;

    row = layout_add_control_ports(grid, row, 1);
    row = layout_add_adapter_ports(grid, row, ADAPTER_PORT_COUNT_VIC20);

    return row;
}

/** \brief  Create layout for xplus4
 *
 * \param[in,out]   grid    main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_plus4_layout(GtkGrid *grid)
{
    int row = 0;

    row = layout_add_control_ports(grid, row, 2);
    row = layout_add_adapter_ports(grid, row, ADAPTER_PORT_COUNT_PLUS4);
    row = layout_add_sidcard_port(grid, row);
    row = layout_add_swap_button(grid, row);

    return row;
}

/** \brief  Create layout for xcbm5x0
 *
 * \param[in,out]   grid    main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_cbm5x0_layout(GtkGrid *grid)
{
    int row = 0;

    row = layout_add_control_ports(grid, row, 2);
    row = layout_add_adapter_ports(grid, row, ADAPTER_PORT_COUNT_CBM5x0);
    row = layout_add_swap_button(grid, row);

    return row;
}

/** \brief  Create layout for xcbm2
 *
 * \param[in,out]   grid    main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_cbm6x0_layout(GtkGrid *grid)
{
    int row = 0;

    row = layout_add_adapter_ports(grid, row, ADAPTER_PORT_COUNT_CBM6x0);

    return row ;
}

/** \brief  Create layout for xpet
 *
 * \param[in,out]   grid    main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_pet_layout(GtkGrid *grid)
{
    int row = 0;

    row = layout_add_adapter_ports(grid, row, ADAPTER_PORT_COUNT_PET);

    return row;
}


/*
 * Public functions
 */

/** \brief  Create joystick settings main widget
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_joystick_widget_create(GtkWidget *parent)
{
    GtkWidget *layout;
    GtkWidget *keyset_enabled;
    GtkWidget *opposite_enabled;
    GtkWidget *keyset_1_button;
    GtkWidget *keyset_2_button;
    int        row = 0;

    layout = vice_gtk3_grid_new_spaced(8, 0);

    /* create basic layout, depending on machine class */
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:
            row = create_c64_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_C64DTV:
            row = create_c64dtv_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_VIC20:
            row = create_vic20_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_PLUS4:
            row = create_plus4_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_CBM5x0:
            row = create_cbm5x0_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_PET:
            row = create_pet_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_CBM6x0:
            row = create_cbm6x0_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_VSID:
            break;  /* no control ports or user ports */
        default:
            break;
    }

    /*
     * These widgets are valid for each emulator (except VSID of course), so
     * not much use wrapping these in functions and calling from the various
     * layout functions.
     */

    /* add check buttons for resources */
    keyset_enabled   = create_keyset_enable_checkbox();
    opposite_enabled = create_opposite_enable_checkbox();
    gtk_grid_attach(GTK_GRID(layout), keyset_enabled,   0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(layout), opposite_enabled, 1, row, 1, 1);
    row++;

    /* add buttons to activate keyset dialog */
    keyset_1_button = create_keyset_configure_button(1);
    keyset_2_button = create_keyset_configure_button(2);
    gtk_widget_set_margin_top(keyset_1_button, 16);
    gtk_widget_set_margin_top(keyset_2_button, 16);
    gtk_grid_attach(GTK_GRID(layout), keyset_1_button, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(layout), keyset_2_button, 1, row, 1, 1);

    gtk_widget_show_all(layout);
    return layout;
}
