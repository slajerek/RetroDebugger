/** \file   aciawidget.c
 * \brief   Widget to control various ACIA related resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES Acia1Dev        -x64dtv -vsid
 * $VICERES RsDevice1       -vsid
 * $VICERES RsDevice2       -vsid
 * $VICERES RsDevice1Baud   -vsid
 * $VICERES RsDevice2Baud   -vsid
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

#include "lib.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "aciawidget.h"


/** \brief  Reference to baud rates list
 */
static int *acia_baud_rates;

/** \brief  List of baud rates
 *
 * Set in acia_widget_create()
 */
static vice_gtk3_combo_entry_int_t *baud_rate_list = NULL;


/** \brief  List of ACIA devices
 */
static const vice_gtk3_radiogroup_entry_t acia_device_list[] = {
    { "Serial 1",      0 },
    { "Serial 2",      1 },
    { "Dump to file",  2 },
    { "Exec process",  3 },
    { NULL,           -1 }
};


/** \brief  Generate heap-allocated list to use in a resourcecombobox
 *
 * Creates a list of ui_combo_box_int_t entries from the `acia_baud_rates` list
 */
static void generate_baud_rate_list(void)
{
    unsigned int i;

    /* count number of baud rates */
    for (i = 0; acia_baud_rates[i] > 0; i++) {
        /* NOP */
    }

    baud_rate_list = lib_malloc((i + 1) * sizeof *baud_rate_list);
    for (i = 0; acia_baud_rates[i] > 0; i++) {
        baud_rate_list[i].name = lib_msprintf("%d", acia_baud_rates[i]);
        baud_rate_list[i].id   = acia_baud_rates[i];
    }
    /* terminate list */
    baud_rate_list[i].name = NULL;
    baud_rate_list[i].id   = -1;
}

/** \brief  Free memory used by `baud_rate_list`
 */
static void free_baud_rate_list(void)
{
    int i;

    for (i = 0; baud_rate_list[i].name != NULL; i++) {
        lib_free(baud_rate_list[i].name);
    }
    lib_free(baud_rate_list);
    baud_rate_list = NULL;
}

/** \brief  Handler for the 'destroy' event of the main widget
 *
 * Frees memory used by the baud rate list
 *
 * \param[in]   widget      main widget (unused)
 * \param[in]   user_data   extra event data (unused)
 */
static void on_destroy(GtkWidget *widget, gpointer user_data)
{
    free_baud_rate_list();
}

/** \brief  Create left-aligned label using Pango markup
 *
 * \param[in]   markup  text of the label using Pango markup
 *
 * \return  GtkLabel
 */
static GtkWidget *label_helper(const char *markup)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), markup);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}

/** \brief  Create an ACIA device widget
 *
 * Creates a widget to select an ACIA device.
 *
 * \return  GtkGrid
 */
static GtkWidget *create_acia_device_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *group;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = label_helper("<b>Acia device</b>");
    group = vice_gtk3_resource_radiogroup_new("Acia1Dev",
                                              acia_device_list,
                                              GTK_ORIENTATION_VERTICAL);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create a widget to set an ACIA serial device (path + baud rate)
 *
 * \param[in]   num     serial device number
 *
 * \return  GtkGrid
 */
static GtkWidget *create_acia_serial_device_widget(int num)
{
    GtkWidget  *grid;
    GtkWidget  *chooser;
    GtkWidget  *label;
    GtkWidget  *combo;
    char        buffer[256];

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    g_snprintf(buffer, sizeof buffer, "Serial %d device", num);
    label = label_helper(buffer);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    chooser = vice_gtk3_resource_filechooser_new_sprintf("RsDevice%d",
                                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                                         num);
    gtk_grid_attach(GTK_GRID(grid), chooser, 1, 0, 1, 1);

    label = gtk_label_new("Baud");
    gtk_widget_set_halign(label, GTK_ALIGN_END);

    g_snprintf(buffer, sizeof buffer, "RsDevice%dBaud", num);
    combo = vice_gtk3_resource_combo_int_new(buffer, baud_rate_list);

    gtk_grid_attach(GTK_GRID(grid), label, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), combo, 3, 0, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create ACIA settings widget
 *
 * XXX: currently designed for PET, might need updating when used in other UIs
 *
 * \param[in]   baud    list of baud rates (list of `int`'s, terminated by -1)
 *
 * \return  GtkGrid
 */
GtkWidget *acia_widget_create(int *baud)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *device_widget;
    GtkWidget *serial1_widget;
    GtkWidget *serial2_widget;

    acia_baud_rates = baud;
    generate_baud_rate_list();

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label          = label_helper("<b>ACIA settings</b>");
    device_widget  = create_acia_device_widget();
    serial1_widget = create_acia_serial_device_widget(1);
    serial2_widget = create_acia_serial_device_widget(2);

    gtk_grid_attach(GTK_GRID(grid), label,          0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), device_widget,  0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), serial1_widget, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), serial2_widget, 0, 3, 1, 1);

    g_signal_connect_unlocked(grid, "destroy", G_CALLBACK(on_destroy), NULL);
    gtk_widget_show_all(grid);
    return grid;
}
