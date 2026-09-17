/** \file   driveparallelcablewidget.c
 * \brief   Drive parallel cable widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES Drive8ParallelCable     x64 x64sc xscpu64 x128 xplus4
 * $VICERES Drive9ParallelCable     x64 x64sc xscpu64 x128 xplus4
 * $VICERES Drive10ParallelCable    x64 x64sc xscpu64 x128 xplus4
 * $VICERES Drive11ParallelCable    x64 x64sc xscpu64 x128 xplus4
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
#include "drive-check.h"
#include "drive.h"
#include "drivewidgethelpers.h"
#include "machine.h"
#include "resources.h"

#include "driveparallelcablewidget.h"


/** \brief  List of possible parallel cables for c64
 */
static const vice_gtk3_combo_entry_int_t parallel_cables_c64[] = {
    { "None",           DRIVE_PC_NONE },
    { "Standard",       DRIVE_PC_STANDARD },
    { "Dolphin DOS 3",  DRIVE_PC_DD3 },
    { "Formel 64",      DRIVE_PC_FORMEL64 },
    { "21sec Backup",   DRIVE_PC_21SEC_BACKUP },
    { NULL,             -1 }
};


/** \brief  List of possible parallel cables for Plus4
 */
static const vice_gtk3_combo_entry_int_t parallel_cables_plus4[] = {
    { "None",       DRIVE_PC_NONE },
    { "Standard",   DRIVE_PC_STANDARD },
    { NULL,         -1 }
};


/** \brief  Create drive parallel cable widget
 *
 * Create combo box to select parallel cable type.
 *
 * \param[in]   unit    drive unit (8-11)
 *
 * \return  GtkComboBox
 */
GtkWidget *drive_parallel_cable_widget_create(int unit)
{
    GtkWidget *combo;
    const vice_gtk3_combo_entry_int_t *list;

    if (machine_class == VICE_MACHINE_PLUS4) {
        list = parallel_cables_plus4;
    } else {
        list = parallel_cables_c64;
    }

    combo = vice_gtk3_resource_combo_int_new_sprintf(
            "Drive%dParallelCable", list, unit);
    g_object_set_data(G_OBJECT(combo), "UnitNumber", GINT_TO_POINTER(unit));
    gtk_widget_set_hexpand(combo, TRUE);
    drive_parallel_cable_widget_update(combo);

    gtk_widget_show_all(combo);
    return combo;
}


/** \brief  Update the widget
 *
 * Enables/disable both the widget and its children depending on the drive type.
 *
 * \param[in,out]   widget  drive parallel cable widget
 */
void drive_parallel_cable_widget_update(GtkWidget *widget)
{
    int unit;
    int drive_type;

    unit = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "UnitNumber"));
    drive_type = ui_get_drive_type(unit);

    gtk_widget_set_sensitive(widget, drive_check_parallel_cable(drive_type));
}
