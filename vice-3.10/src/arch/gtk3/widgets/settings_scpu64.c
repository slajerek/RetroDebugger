/** \file   settings_scpu64.c
 * \brief   Settings widget controlling SCPU64-specific settings
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES SIMMSize    xscpu64
 * $VICERES JiffySwitch xscpu64
 * $VICERES SpeedSwitch xscpu64
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

#include "ui.h"
#include "uistatusbar.h"
#include "vice_gtk3.h"

#include "settings_scpu64.h"


/** \brief  List of SCPU64 SIMM sizes
 */
static const vice_gtk3_combo_entry_int_t simm_sizes[] = {
    { "Not installed",  0 },
    { "1MiB",           1 },
    { "4MiB",           4 },
    { "8MiB",           8 },
    { "16MiB",         16 },
    { NULL,            -1 }
};


/** \brief  Callback for the SuperCPU Speed switch
 *
 * Update statusbar "turbo" LED when the switch is toggled.
 *
 * \param[in]   self    resource switch (ignored)
 * \param[in]   active  new state of switch
 */
static void speed_switch_callback(GtkWidget *self, gboolean active)
{
    supercpu_turbo_led_set_active(PRIMARY_WINDOW, active);
}

/** \brief  Callback for the SuperCPU JiffyDOS switch
 *
 * Update statusbar "JiffyDOS" LED when the switch is toggled.
 *
 * \param[in]   self    resource switch (ignored)
 * \param[in]   active  new state of switch
 */
static void jiffy_switch_callback(GtkWidget *self, gboolean active)
{
    supercpu_jiffy_led_set_active(PRIMARY_WINDOW, active);
}

/** \brief  Create left-aligned label
 *
 * \param[in]   text    text for the label
 *
 * \return  GtkLabel
 */
static GtkWidget *create_label(const char *text)
{
    GtkWidget *label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}


/** \brief  Create widgets for SCPU64-specific settings
 *
 * \param[in]   parent  parent widget (ignored)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_scpu64_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *simm;
    GtkWidget *jiffy;
    GtkWidget *speed;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 16);

    label = create_label("Speed switch");
    speed = vice_gtk3_resource_switch_new("SpeedSwitch");
    vice_gtk3_resource_switch_add_callback(speed, speed_switch_callback);
    gtk_widget_set_halign(speed, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), speed, 1, 0, 1, 1);

    label = create_label("JiffyDOS switch");
    jiffy = vice_gtk3_resource_switch_new("JiffySwitch");
    vice_gtk3_resource_switch_add_callback(jiffy, jiffy_switch_callback);
    gtk_widget_set_halign(jiffy, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), jiffy, 1, 1, 1, 1);

    label = create_label("SuperRAM expansion");
    simm  = vice_gtk3_resource_combo_int_new("SIMMsize",
                                             simm_sizes);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), simm,  1, 2, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
