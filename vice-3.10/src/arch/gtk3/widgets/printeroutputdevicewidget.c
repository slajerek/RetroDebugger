/** \file   printeroutputdevicewidget.c
 * \brief   Widget to control printer output device settings
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES Printer4TextDevice          -vsid
 * $VICERES Printer5TextDevice          -vsid
 * $VICERES Printer6TextDevice          -vsid
 * $VICERES PrinterUserportTextDevice   x64 x64sc xscpu64 x128 xvic xpet xcbm2
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

#include "printeroutputdevicewidget.h"


/** \brief  List of text output devices
 */
static const vice_gtk3_radiogroup_entry_t device_list[] = {
    { "Device 1",  PRINTER_TEXT_DEVICE_1 },
    { "Device 2",  PRINTER_TEXT_DEVICE_2 },
    { "Device 3",  PRINTER_TEXT_DEVICE_3 },
    { NULL,       -1 }
};


/** \brief  Create widget for the "Printer[4-6|Userport]TextDevice resource
 *
 * \param[in]   device number (4-6 or 3 for userport)
 *
 * \return  GtkGrid
 */
GtkWidget *printer_output_device_widget_create(int device)
{
    GtkWidget *grid;
    GtkWidget *group;
    char       resource[32];

    if (device >= 4 && device <= 6) {
        g_snprintf(resource, sizeof resource, "Printer%dTextDevice", device);
    } else if (device == 3) {
        strncpy(resource, "PrinterUserportTextDevice", sizeof resource - 1u);
        resource[sizeof resource - 1u] = '\0';
    } else {
        log_error(LOG_DEFAULT,
                  "%s(): invalid device number %d.",
                  __func__, device);
        return NULL;
    }

    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, "Output device", 1);
    vice_gtk3_grid_set_title_margin(grid, 8);
    group = vice_gtk3_resource_radiogroup_new(resource,
                                              device_list,
                                              GTK_ORIENTATION_VERTICAL);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}
