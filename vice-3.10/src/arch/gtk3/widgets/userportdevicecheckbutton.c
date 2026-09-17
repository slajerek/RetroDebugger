/** \file   userportdevicecheckbutton.c
 * \brief   Toggles UserportDevice between none and a single other type
 *
 * Simple GtkCheckButton that toggles the "UserportDevice" between
 * `USERPORT_DEVICE_NONE` and a single other `USERPORT_DEVICE_foo` type.
 *
 * This widget provides a shortcut for setting the UserportDevice to a specific
 * type without having to go to the Userport setting node in the settings
 * dialog.
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES UserportDevice  -vsid -xcbm5x0
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

#include "resources.h"
#include "userport.h"
#include "vice_gtk3.h"

#include "userportdevicecheckbutton.h"


/** \brief  Handler for the 'toggled' event of the check button
 *
 * Set "UserportDevice" resource to \a type if toggled on, set resource to
 * `USERPORT_DEVICE_NONE` when toggled off.
 */
static void on_toggled(GtkToggleButton *self, gpointer type)
{
    if (gtk_toggle_button_get_active(self)) {
        resources_set_int("UserportDevice", GPOINTER_TO_INT(type));
    } else {
        resources_set_int("UserportDevice", USERPORT_DEVICE_NONE);
    }
}

/** \brief  Create check button to toggle "UserportDevice" between two states
 *
 * Create check button to toggle the "UserportDevice" resource between
 * `USERPORT_DEVICE_NONE` and \a type.
 * If the current state of "UserportDevice" doesn't match \a type the check
 * button is initially off.
 *
 * \param[in]   label   label for the check button
 * \param[in]   type    the userport device type for the ON state
 *
 * \return  GtkCheckButton
 */
GtkWidget *userport_device_check_button_new(const char *label, int type)
{
    GtkWidget *check;
    int        current = USERPORT_DEVICE_NONE;

    resources_get_int("UserportDevice", &current);
    check = gtk_check_button_new_with_label(label);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                 (gboolean)(current == type));
    g_signal_connect(check,
                     "toggled",
                     G_CALLBACK(on_toggled),
                     GINT_TO_POINTER(type));
    return check;
}

