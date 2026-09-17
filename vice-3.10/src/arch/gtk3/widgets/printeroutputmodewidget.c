/** \file   printeroutputmodewidget.c
 * \brief   Widget to control printer output mode settings
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES Printer4Output          -vsid
 * $VICERES Printer5Output          -vsid
 * $VICERES Printer6Output          -vsid
 * $VICERES PrinterUserportOutput   x64 x64sc xscpu64 x128 xvic xpet xcbm2
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
#include <string.h>

#include "log.h"
#include "printer.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "printeroutputmodewidget.h"


/* Output mode radio button rows */
enum {
    ROW_TEXT_MODE     = 1,
    ROW_GRAPHICS_MODE = 2
};


/** \brief  Handler for the 'toggled' event of the radio buttons
 *
 * \param[in]   radio   radio button
 * \param[in]   mode    new output mode
 */
static void on_radio_toggled(GtkWidget *radio, gpointer mode)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio))) {

        GtkWidget  *parent;
        const char *new_val;
        const char *old_val = NULL;
        const char *resource;

        parent   = gtk_widget_get_parent(radio);
        resource = resource_widget_get_resource_name(parent);
        new_val  = (const char *)mode;
        resources_get_string(resource, &old_val);

        if (g_strcmp0(new_val, old_val) != 0) {
            resources_set_string(resource, new_val);
        }
    }
}


/** \brief  Create widget to control Printer[device]TextDevice resource
 *
 * \param[in]   device  device number (4-6 or 3 for userport)
 *
 * \return  GtkGrid
 */
GtkWidget *printer_output_mode_widget_create(int device)
{
    GtkWidget  *grid;
    GtkWidget  *text;
    GtkWidget  *gfx;
    GSList     *group = NULL;
    const char *value = NULL;
    char        resource[32];

    /* can't use the resource base widgets here, since for some reason this
     * resource is a string with two possible values: "text" and "graphics"
     */

    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, "Output mode", 1);
    vice_gtk3_grid_set_title_margin(grid, 8);

    if (device >= 4 && device <= 6) {
        g_snprintf(resource, sizeof resource , "Printer%dOutput", device);
    } else if (device == 3) {
        /* userport printer */
        strncpy(resource, "PrinterUserportOutput", sizeof resource - 1u);
        resource[sizeof resource - 1u] = '\0';
    } else {
        log_error(LOG_DEFAULT,
                  "%s(): invalid device number %d.",
                  __func__, device);
        return NULL;
    }
    resource_widget_set_resource_name(grid, resource);

    text = gtk_radio_button_new_with_label(group, "Text");
    gfx  = gtk_radio_button_new_with_label(group, "Graphics");
    gtk_radio_button_join_group(GTK_RADIO_BUTTON(gfx),
                                GTK_RADIO_BUTTON(text));

    resources_get_string(resource, &value);
    if (g_strcmp0(value, "text") == 0) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(text), TRUE);
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gfx), TRUE);
    }

    gtk_grid_attach(GTK_GRID(grid), text, 0, ROW_TEXT_MODE,     1, 1);
    gtk_grid_attach(GTK_GRID(grid), gfx,  0, ROW_GRAPHICS_MODE, 1, 1);

    g_signal_connect(text,
                     "toggled",
                     G_CALLBACK(on_radio_toggled),
                     (gpointer)"text");
    g_signal_connect(gfx,
                     "toggled",
                     G_CALLBACK(on_radio_toggled),
                     (gpointer)"graphics");

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Set printer output mode
 *
 * \param[in]   widget  printer output mode widget
 * \param[in]   mode    mode ("text or "graphics")
 */
void printer_output_mode_widget_set_mode(GtkWidget *widget, const char *mode)
{
    GtkWidget *radio;

    if (g_strcmp0(mode, "text") == 0) {
        radio = gtk_grid_get_child_at(GTK_GRID(widget), 0, ROW_TEXT_MODE);
    } else {
        radio = gtk_grid_get_child_at(GTK_GRID(widget), 0, ROW_GRAPHICS_MODE);
    }
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
}
