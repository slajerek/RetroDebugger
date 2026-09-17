/** \file   settings_autofire.c
 * \brief   Widget to control autofire settings for joysticks
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/* See widgets/joystickautofirewidget.c for resources used. */

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

#include "joyport.h"
#include "joystickautofirewidget.h"
#include "machine.h"
#include "vice_gtk3.h"

#include "settings_autofire.h"


/** \brief  Add userport joystick widgets to layout
 *
 * Add userport joystick widgets, starting at resource 'JoyStick3*'.
 *
 * \param[in]   layout  GtkGrid to add widgets to
 * \param[in]   row     row in \a layout to start
 * \param[in]   count   number of widgets to add
 *
 * \return  row in \a layout for additional widgets
 */
static int add_userport_widgets(GtkWidget *layout, int row, int count)
{
    int joy;
    int column = 0;

    /* Userport 1-$count (joy 3...) */
    for (joy = 3; joy < 3 + count; joy++) {
        GtkWidget *widget;
        char title[256];

        if (joyport_has_mapping(joy - 1)) {
            g_snprintf(title, sizeof title, "Extra Joystick #%d", joy - 2);
            widget = joystick_autofire_widget_create(joy, title);
            gtk_grid_attach(GTK_GRID(layout), widget, column, row, 1, 1);
        }
        column ^= 1;
        if (column == 0) {
            row++;
        }
    }
    /* adjust row in case of odd number of widgets */
    if (column != 0) {
        row++;
    }
    return row;
}

/** \brief  Create layout for x64, x64sc, x64dtv, xscpu64, x128 and xcbm5x0
 *
 * \param[in]   layout  GtkGrid to add widgets to
 * \param[in]   row     row in \a layout to start adding widgets
 *
 * \return  row in \a layout for additional widgets
 */
static int create_c64_layout(GtkWidget *layout, int row)
{
    /* Controlport 1 & 2 */
    gtk_grid_attach(GTK_GRID(layout),
                    joystick_autofire_widget_create(1, "Control Port #1"),
                    0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(layout),
                    joystick_autofire_widget_create(2, "Control Port #2"),
                    1, row, 1, 1);
    row++;

    /* Userport 1-8 (joy 3-10) */
    return add_userport_widgets(layout, row, 8);
}

/** \brief  Create layout for xvic
 *
 * \param[in]   layout  GtkGrid to add widgets to
 * \param[in]   row     row in \a layout to start adding widgets
 *
 * \return  row in \a layout for additional widgets
 */
static int create_vic20_layout(GtkWidget *layout, int row)
{
    /* Controlport 1 only */
    gtk_grid_attach(GTK_GRID(layout),
                    joystick_autofire_widget_create(1, "Control Port #1"),
                    0, row, 1, 1);
    row++;

    /* Userport 1-8 (joy 3-10) */
    return add_userport_widgets(layout, row, 8);
}

/** \brief  Create layout for xcbm2
 *
 * \param[in]   layout  GtkGrid to add widgets to
 * \param[in]   row     row in \a layout to start adding widgets
 *
 * \return  row in \a layout for additional widgets
 */
static int create_cbm2_layout(GtkWidget *layout, int row)
{
    /* Userport 1-8 (joy 3-10) */
    return add_userport_widgets(layout, row, 8);
}

/** \brief  Create layout for xpet
 *
 * \param[in]   layout  GtkGrid to add widgets to
 * \param[in]   row     row in \a layout to start adding widgets
 *
 * \return  row in \a layout for additional widgets
 */
static int create_pet_layout(GtkWidget *layout, int row)
{
    /* Userport 1-2 (joy 3-4) */
    return add_userport_widgets(layout, row, 2);
}

/** \brief  Create layout for xplus4
 *
 * \param[in]   layout  GtkGrid to add widgets to
 * \param[in]   row     row in \a layout to start adding widgets
 *
 * \return  row in \a layout for additional widgets
 *
 * \note    Contains support for the 3-way userport joystick adapter from
 *          Synergy, which isn't emulated yet.
 *          See https://plus4world.powweb.com/hardware/3fach_Joystickadapter_Synergy
 *          for details. This moves the SIDCard joystick port to JoyStick6.
 */
static int create_plus4_layout(GtkWidget *layout, int row)
{
    /* Controlport 1 & 2 */
    gtk_grid_attach(GTK_GRID(layout),
                    joystick_autofire_widget_create(1, "Control Port #1"),
                    0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(layout),
                    joystick_autofire_widget_create(2, "Control Port #2"),
                    1, row, 1, 1);
    row++;

    /* Userport 1-8 (joy 3-10) */
    row = add_userport_widgets(layout, row, 8);

    /* SID Card joyport (joy 11) */
    if (joyport_has_mapping(10)) {
        gtk_grid_attach(GTK_GRID(layout),
                        joystick_autofire_widget_create(11, "SIDCard Joystick"),
                        0, row, 1, 1);
    }
    return row;
}

/** \brief  Create settings 'page' widget for autofire settings
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_autofire_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    int row;

    grid = vice_gtk3_grid_new_spaced(32, 16);
    row = 0;

    /* add machine-specific layout */
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_C64DTV:   /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_CBM5x0:
            row = create_c64_layout(grid, row);
            break;
        case VICE_MACHINE_CBM6x0:
            row = create_cbm2_layout(grid, row);
            break;
        case VICE_MACHINE_VIC20:
            row = create_vic20_layout(grid, row);
            break;
        case VICE_MACHINE_PET:
            row = create_pet_layout(grid, row);
            break;
        case VICE_MACHINE_PLUS4:
            row = create_plus4_layout(grid, row);
            break;
        default:
            /* NOP */
            break;
    }
    gtk_widget_show_all(grid);
    return grid;
}
