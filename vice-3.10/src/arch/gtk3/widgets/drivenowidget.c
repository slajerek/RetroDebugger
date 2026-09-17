/** \file   drivenowidget.c
 * \brief   GTK3 drive number selection widget
 *
 * \author  Andre Fachat
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
 */


#include "vice.h"

#include <gtk/gtk.h>

#include "widgethelpers.h"
#include "debug_gtk3.h"
#include "drive.h"

#include "drivenowidget.h"


/** \brief  Destination of the drive number when changed
 *
 * Set when the drive number in the widget changes, pass `NULL` as the `target`
 * argument to disable this.
 */
static int *number_target;


/** \brief  Callback triggered on drive number change
 *
 * This is an optional callback to allow updating other widgets depending on
 * the selected drive number. Pass `NULL` as the `callback` argument to
 * create_drive_no_widget() to disable this.
 */
static void (*number_callback)(int) = NULL;


/** \brief  Handler for the "toggled" events of the radio buttons
 *
 * \param[in]   widget      radio button triggering the event
 * \param[in]   user_data   drive number (int)
 */
static void on_radio_toggled(GtkWidget *widget, gpointer user_data)
{
    int number = GPOINTER_TO_INT(user_data);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
#if 0
        debug_gtk3("setting drive number to %d.", number);
#endif
        if (number_target != NULL) {
            *number_target = number;
        }
        if (number_callback != NULL) {
            number_callback(number);
        }
    }
}


/** \brief  Create a GtkGrid with radio buttons in a group for drive numbers
 *
 * Sets up event handler to change the `number_target` variable passed in
 * create_drive_no_widget().
 *
 * \param[in]   number    default drive number
 *
 * \return GtkGrid
 */
static GtkWidget *create_radio_group(int number)
{
    GtkWidget *grid;
    GtkRadioButton *last;
    GSList *group = NULL;
    int i;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

    last = NULL;
    for (i = DRIVE_NUMBER_MIN; i <= DRIVE_NUMBER_MAX; i++) {
        gchar buffer[16];
        GtkWidget *radio;

        g_snprintf(buffer, 16, "%d", i);

        radio = gtk_radio_button_new_with_label(group, buffer);
        gtk_radio_button_join_group(GTK_RADIO_BUTTON(radio), last);
        gtk_grid_attach(GTK_GRID(grid), radio, i, 0, 1, 1);

        if (number == i) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
        }

        /* don't connect signal handlers here, since that can trigger the
         * handlers through the previous statement */

        last = GTK_RADIO_BUTTON(radio);
    }
    gtk_widget_show_all(grid);
    return grid;
}



/** \brief  Create a drive#-selection widget
 *
 * \param[in]   number      currently selected drive number (0/1)
 * \param[out]  target      destination of the drive number on radio button
 *                          clicks
 * \param[in]   callback    user callback function to receieve drive number
 *
 * \return  GtkGrid
 */
GtkWidget *drive_no_widget_create(int number,
                                  int *target,
                                  void (*callback)(int))
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *group;
    int i;

    number_target = target;
    number_callback = callback;

    if (target != NULL) {
        *target = number; /* make sure we don't end up with unintialized data */
    }

    grid = vice_gtk3_grid_new_spaced(8, 0);

    label = gtk_label_new("Drive #:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    group = create_radio_group(number);
    gtk_grid_attach(GTK_GRID(grid), group, 1, 0, 1, 1);

    /* connect signal handlers */

    /* The limit used to be NUM_DISK_UNITS, but that indicates the number of
     * units, not drives, so we may have to rename a few defines to cope with
     * having both units and drives in a unit. And also apply these constants
     * in the other code the patch fixes (ie all the 0-2 loops and drive > 1
     * checks)
     *
     * -- compyx
     */
    for (i = 0; i < 2; i++) {
        GtkWidget *radio = gtk_grid_get_child_at(GTK_GRID(group), i, 0);
        if (radio == NULL) {
            debug_gtk3("Got NULL for radio button");
        } else {
            g_signal_connect(radio, "toggled", G_CALLBACK(on_radio_toggled),
                GINT_TO_POINTER(i));
        }
    }

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Update widget by setting a \a drive number
 *
 * \param[in,out]   widget  drive number widget
 * \param[in]       drive   drive number
 */
void drive_no_widget_update(GtkWidget *widget, int drive)
{
    GtkWidget *group;

    /* XXX: change this if we ever find drive units with more than two drives,
     *      which seems highly unlikely
     */
    if (drive != 0) {
        drive = 1;
    }

    group = gtk_grid_get_child_at(GTK_GRID(widget), 1, 0);
    if (group != NULL) {
        GtkWidget *radio;

        radio = gtk_grid_get_child_at(GTK_GRID(group), drive, 0);
        if (radio != NULL) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
        }
    }
}


