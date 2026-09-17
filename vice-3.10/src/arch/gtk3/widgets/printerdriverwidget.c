/** \file   printerdriverwidget.c
 * \brief   Widget to control printer drivers
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Allows selecting drivers for printers \#4, \#5, \#6 and userport printer.
 */

/*
 * $VICERES Printer4Driver          -vsid
 * $VICERES Printer5Driver          -vsid
 * $VICERES Printer6Driver          -vsid
 * $VICERES PrinterUserportDriver   x64 x64sc xscpu64 x128 xvic xpet xcbm2
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

#include "driver-select.h"
#include "log.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "printerdriverwidget.h"


/** \brief  Resources names per device
 *
 * \note    Index 0 is device 3/userport, index 1-2 are printer 4 and 5 and
 *          index 3 is plotter 6.
 */
static const char *driver_resource_names[4] = {
    "PrinterUserportDriver",
    "Printer4Driver",
    "Printer5Driver",
    "Printer6Driver"
};

/** \brief  Function to call when a driver radio button is selected */
static void (*extra_radio_callback)(GtkWidget *, int, const char *) = NULL;


/** \brief  Handler for the "toggled" event of the radio buttons
 *
 * \param[in]   radio       radio button
 * \param[in]   user_data   new value for the resource (`const char *`)
 */
static void on_radio_toggled(GtkWidget *radio, gpointer user_data)
{

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio))) {
        GtkWidget  *parent;
        const char *resource;
        const char *driver;
        int         device;

        parent   = gtk_widget_get_parent(radio);
        resource = resource_widget_get_resource_name(parent);
        driver   = g_object_get_data(G_OBJECT(radio), "Driver");
        device   = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(radio), "Device"));
#if 0
        debug_gtk3("Setting resource '%s' to '%s'", resource, driver);
#endif
        resources_set_string(resource, driver);
        if (extra_radio_callback != NULL) {
            extra_radio_callback(radio, device, driver);
        }
    }
}


/** \brief  Create printer driver selection widget
 *
 * Creates a group of radio buttons to select the driver of printer # \a device.
 *
 * Uses a custom property "Driver" for the radio buttons and stores the
 * resource name for \a device as the "ResourceName" property.
 *
 * The following device lists are obtained dynamically:
 * Printer 4/5  : [ascii, mps 801/802/803, cbm 2022/4023/8023, nl10, raw]
 * Printer 6    : [1520, raw]
 * Userport (3) : [ascii, nl10, raw]
 *
 * \param[in]   device      device number (4-6) or 3 for userport
 * \param[in]   callback    function to call on radio button selection (optional)
 *
 * \return  GtkGrid
 */
GtkWidget *printer_driver_widget_create(int device,
                                        void (*callback)(GtkWidget  *radio,
                                                         int         device,
                                                         const char *drv_name))
{
    GtkWidget                  *grid;
    GtkWidget                  *last;
    GSList                     *group;
    const driver_select_list_t *drv_node;
    const char                 *current = NULL;
    int                         index;
    int                         row;

    extra_radio_callback = callback;

    /* sanity check */
    if (device < 3 || device > 6) {
        log_error(LOG_DEFAULT,
                  "%s(): invalid device number %d, valid device numbers are"
                  " 4-6 or 3 for userport",
                  __func__, device);
        return NULL;
    }
    index = device - 3; /* index in the resource names array */

    /* build grid */
    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, "Driver", 1);
    vice_gtk3_grid_set_title_margin(grid, 8);

    /* get current resource value */
    resources_get_string(driver_resource_names[index], &current);
    /* set resource name to allow the update function to work */
    resource_widget_set_resource_name(grid, driver_resource_names[index]);

    /* create radio buttons */
    last     = NULL;
    group    = NULL;
    row      = 1;
    drv_node = driver_select_get_drivers();

    while (drv_node != NULL) {
        GtkWidget  *radio = NULL;
        char       *drv_name;   /* cannot be const since we cast this to gpointer */
        const char *ui_name;

        drv_name = drv_node->driver_select.drv_name;
        ui_name  = drv_node->driver_select.ui_name;

        if ((device == 4 || device == 5) && driver_select_is_printer(drv_name)) {
            radio = gtk_radio_button_new_with_label(group, ui_name);
        } else if ((device == 6) && driver_select_is_plotter(drv_name)) {
            radio = gtk_radio_button_new_with_label(group, ui_name);
        } else if ((device == 3) && driver_select_has_userport(drv_name)) {
            radio = gtk_radio_button_new_with_label(group, ui_name);
        }

        if (radio != NULL) {
            /* No need to use g_strdup() since the string is always available.
             * We could use the signal handler's `data` argument to pass this
             * value, but we also need the value for the update() function to work
             */
            g_object_set_data(G_OBJECT(radio), "Driver", (gpointer)drv_name);
            g_object_set_data(G_OBJECT(radio), "Device", GINT_TO_POINTER(device));

            if (last != NULL) {
                gtk_radio_button_join_group(GTK_RADIO_BUTTON(radio),
                                            GTK_RADIO_BUTTON(last));
            }
            if (g_strcmp0(current, drv_name) == 0) {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
            }

            g_signal_connect(G_OBJECT(radio),
                             "toggled",
                             G_CALLBACK(on_radio_toggled),
                             NULL);

            gtk_grid_attach(GTK_GRID(grid), radio, 0, row, 1, 1);
            row++;
            last = radio;
        }

        drv_node = drv_node->next;
    }

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Update the printer driver widget
 *
 * \param[in,out]   widget  printer driver widget
 * \param[in]       driver  driver name
 */
void printer_driver_widget_update(GtkWidget *widget, const char *driver)
{
    int row = 1;

    while (TRUE) {
        GtkWidget *radio = gtk_grid_get_child_at(GTK_GRID(widget), 0, row);

        if (radio == NULL) {
            break;  /* end of grid, give up */
        } else if (GTK_IS_RADIO_BUTTON(radio)) {
            const char *value = g_object_get_data(G_OBJECT(radio), "Driver");
            if (g_strcmp0(driver, value) == 0) {
                /* this also triggers the signal handler, updating the resource */
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
                break;
            }
        }
        row++;
    }
}
