/** \file   superpetwidget.c
 * \brief   Widget to control various SuperPET related resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES SuperPET        xpet
 * $VICERES CPUswitch       xpet
 *
 * See the widgets/aciawidget.c file for additional resources.
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
#include "aciawidget.h"
#include "resources.h"
#include "ui.h"

#include "superpetwidget.h"


/** \brief  List of baud rates for the ACIA widget
 */
static int baud_rates[] = { 300, 1200, 2400, 9600, 19200, -1 };

/** \brief  List of CPU types
 */
static const vice_gtk3_radiogroup_entry_t cpu_types[] = {
    { "MOS 6502", 0 },
    { "Motorola 6809", 1 },
    { "Programmable", 2 },
    { NULL, -1 },
};


/** \brief  SuperPET enable toggle button */
static GtkWidget *enable_widget = NULL;

/** \brief  ACIA widget */
static GtkWidget *acia1_widget = NULL;

/** \brief  CPU widget */
static GtkWidget *cpu_widget = NULL;

/** \brief  User-defined callback function
 *
 * Set with pet_superpet_widget_set_superpet_enable_callback()
 */
static void (*user_callback_enable)(int);


/** \brief  Callback for the check button widget
 *
 * \param[in]   widget   check button widget
 * \param[in]   enabled  enabled or not
 */
static void superpet_enable_callback(GtkWidget *widget, int enabled)
{
    if (user_callback_enable != NULL) {
        user_callback_enable(enabled);
    }
}

/** \brief  Create check button for the SuperPET resource
 *
 * Create a grid with a check button and some informative text under it.
 *
 * \return  GtkGrid
 */
static GtkWidget *create_superpet_enable_widget(void)
{
    GtkWidget *check;

    check = vice_gtk3_resource_check_button_new("SuperPET", "I/O Enable (disables 8x96)");
    vice_gtk3_resource_check_button_add_callback(check, superpet_enable_callback);
    gtk_widget_set_valign(check, GTK_ALIGN_START);

    return check;
}

/** \brief  Create SuperPET CPU selection widget
 *
 * Select between 'MOS 6502, 'Motorola 6809' or 'programmable'
 *
 * \return  GtkGrid
 */
static GtkWidget *create_superpet_cpu_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *group;

    grid  = gtk_grid_new();
    label = gtk_label_new(NULL);
    group = vice_gtk3_resource_radiogroup_new("CPUswitch",
                                              cpu_types,
                                              GTK_ORIENTATION_VERTICAL);

    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_label_set_markup(GTK_LABEL(label), "<b>CPU type</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);

    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create a SuperPET-specific widget to be used in the PET model widget
 *
 * \return  GtkGrid
 */
GtkWidget *superpet_widget_create(void)
{
    GtkWidget *grid;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 16);

    acia1_widget  = acia_widget_create(baud_rates);
    cpu_widget    = create_superpet_cpu_widget();
    enable_widget = create_superpet_enable_widget();
    gtk_grid_attach(GTK_GRID(grid), acia1_widget,  0, 2, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), cpu_widget,    0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), enable_widget, 1, 3, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set function to trigger on SuperPET enable checkbox toggle
 *
 * \param[in]   func    callback function
 */
void pet_superpet_widget_set_superpet_enable_callback(void (*func)(int))
{
    user_callback_enable = func;
}


/** \brief  Synchronize \a widget with its resource
 *
 * \param[in,out]   widget  SuperPET I/O enabled widget
 */
void pet_superpet_enable_widget_sync(void)
{
    vice_gtk3_resource_check_button_sync(enable_widget);
}
