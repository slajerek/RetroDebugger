/** \file   driverpmwidget.c
 * \brief   Drive RPM settings widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES Drive8RPM                -vsid
 * $VICERES Drive9RPM                -vsid
 * $VICERES Drive10RPM               -vsid
 * $VICERES Drive11RPM               -vsid
 * $VICERES Drive8WobbleFrequency    -vsid
 * $VICERES Drive9WobbleFrequency    -vsid
 * $VICERES Drive10WobbleFrequency   -vsid
 * $VICERES Drive11WobbleFrequency   -vsid
 * $VICERES Drive8WobbleAmplitude    -vsid
 * $VICERES Drive9WobbleAmplitude    -vsid
 * $VICERES Drive10WobbleAmplitude   -vsid
 * $VICERES Drive11WobbleAmplitude   -vsid
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

#include "drivewidgethelpers.h"
#include "vice_gtk3.h"

#include "drive.h"

#include "driverpmwidget.h"


/** \brief  Spin button declaration
 *
 * Data used for the 'fake float' resource spin buttons: spin buttons are
 * presented as controlling float values while the internal resource values
 * are integers.
 */
typedef struct spin_s {
    const char *label;      /**< label to put next to the spin button */
    const char *format;     /**< resource name format string */
    const char *valfmt;     /**< display format string */
    int         min;        /**< resource minimum value */
    int         max;        /**< resource maximum value */
    float       valmin;     /**< spin button minimum value */
    float       valmax;     /**< spin button maximum value */
    float       step;       /**< spin button stepping */
} spin_t;

/** \brief  Spin button declarations for the RPM resources
 */
static const spin_t spinners[] = {
    { "Drive RPM",        "Drive%dRPM",             "%6.2f",
      DRIVE_RPM_MIN,                 DRIVE_RPM_MAX,
      DRIVE_RPM_MIN / DRIVE_RPM_ONE, DRIVE_RPM_MAX / DRIVE_RPM_ONE,                           0.5f },
    { "Wobble frequency", "Drive%dWobbleFrequency", "%6.3fHz",
      0,                             DRIVE_WOBBLE_FREQ_MAX,
      0.0f,                          DRIVE_WOBBLE_FREQ_MAX / DRIVE_WOBBLE_FREQ_ONE,           0.1f },
    { "Wobble Amplitude", "Drive%dWobbleAmplitude", "%4.2fRPM",
      0,                             DRIVE_WOBBLE_AMPLITUDE_MAX,
      0.0f,                          DRIVE_WOBBLE_AMPLITUDE_MAX / DRIVE_WOBBLE_AMPLITUDE_ONE, 0.1f }
};

/** \brief  Create widget to control drive RPM and wobble
 *
 * \param[in]   unit    unit number
 *
 * \return  GtkGrid
 */
GtkWidget *drive_rpm_widget_create(int unit)
{
    GtkWidget *grid;
    int        i;

    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, "RPM settings", 2);
    g_object_set_data(G_OBJECT(grid), "UnitNumber", GINT_TO_POINTER(unit));

    for (i = 0; i < G_N_ELEMENTS(spinners); i++) {
        GtkWidget *label;
        GtkWidget *spin;

        label = gtk_label_new(spinners[i].label);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        spin = vice_gtk3_resource_spin_custom_new_sprintf(spinners[i].format,
                                                          spinners[i].min, spinners[i].max,
                                                          spinners[i].valmin, spinners[i].valmax, spinners[i].step,
                                                          spinners[i].valfmt,
                                                          unit);
        gtk_grid_attach(GTK_GRID(grid), label, 0, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), spin,  1, i + 1, 1, 1);
    }

    gtk_widget_show_all(grid);
    return grid;
}
