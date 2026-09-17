/** \file   driveidlemethodwidget.c
 * \brief   Drive Idle method widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES Drive8IdleMethod    -vsid
 * $VICERES Drive9IdleMethod    -vsid
 * $VICERES Drive10IdleMethod   -vsid
 * $VICERES Drive11IdleMethod   -vsid
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
#include "resources.h"

#include "driveidlemethodwidget.h"



/** \brief  Idle method (name,id) tuples
 */
static const vice_gtk3_combo_entry_int_t idle_methods[] = {
    { "None",           DRIVE_IDLE_NO_IDLE },
    { "Skip cycles",    DRIVE_IDLE_SKIP_CYCLES },
    { "Trap idle",      DRIVE_IDLE_TRAP_IDLE },
    { NULL,             -1 }
};


/** \brief  Create widget to set the drive idle method
 *
 * \param[in]   unit    current drive unit number
 *
 * \return  GtkGrid
 */
GtkWidget *drive_idle_method_widget_create(int unit)
{
    GtkWidget *combo;

    combo = vice_gtk3_resource_combo_int_new_sprintf(
            "Drive%dIdleMethod", idle_methods, unit);
    g_object_set_data(G_OBJECT(combo), "UnitNumber", GINT_TO_POINTER(unit));
    gtk_widget_set_hexpand(combo, TRUE);
    gtk_widget_show_all(combo);
    return combo;
}
