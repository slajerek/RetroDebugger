/** \file   carthelpers.c
 * \brief   Cartridge helper functions
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 *
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

#include "basewidget_types.h"
#include "cartridge.h"

#include "carthelpers.h"


/** \brief  Set toggle button active state while blocking event handler
 *
 * Set \a button to \a active while blocking signal handler \a handler_id.
 *
 * \param[in]   button      toggle button
 * \param[in]   handler_id  ID of signal handler to temporarily block
 * \param[in]   active      new active state for \a button
 */
static void set_active_blocked(GtkToggleButton *button,
                               gulong           handler_id,
                               gboolean         active)
{
    g_signal_handler_block(G_OBJECT(button), handler_id);
    gtk_toggle_button_set_active(button, active);
    g_signal_handler_unblock(G_OBJECT(button), handler_id);
}

/** \brief  Handler for the "toggled" event of the cart enable check button
 *
 * When this function fails to enable/disable the cartridge, it'll revert \a self
 * to its previous state.
 *
 * \param[in,out]   self    check button
 * \param[in]       id      cartridge ID
 */
static void on_cart_enable_check_button_toggled(GtkToggleButton *self,
                                                gpointer         id)
{
    gulong handler_id;
    int    cart_id;

    cart_id = GPOINTER_TO_INT(id);
    handler_id = GPOINTER_TO_ULONG(g_object_get_data(G_OBJECT(self), "HandlerId"));
    if (gtk_toggle_button_get_active(self)) {
        if (cartridge_enable(cart_id) < 0) {
            set_active_blocked(self, handler_id, FALSE);
        }
    } else {
        if (cartridge_disable(cart_id) < 0) {
            set_active_blocked(self, handler_id, TRUE);
        }
    }
}


/** \brief  Create a check button to enable/disable a cartridge
 *
 * Creates a check button that enables/disables a cartridge. The \a cart_name
 * argument is copied to allow for debug/error messages to mention the cart
 * by name, rather than by ID. The name is freed when the check button is
 * destroyed.
 *
 * What the widget basically does is call cartridge_enable(\a cart_id) or
 * cartridge_disable(\a cart_id), using cartridge_type_enabled(\a cart_id) to
 * set the initial state of the widget. But since all Gtk3 widgets are
 * currently linked into a big lib and vsid doesn't like that, we use some
 * function pointer magic is used.
 *
 * \param[in]   cart_name   cartridge name (see cartridge.h)
 * \param[in]   cart_id     cartridge ID (see cartridge.h)
 *
 * \return  GtkCheckButton
 */
GtkWidget *carthelpers_create_enable_check_button(const char *cart_name,
                                                  int         cart_id)
{
    GtkWidget *check;
    gulong     handler_id;
    char       title[256];

    g_snprintf(title, sizeof title, "Enable %s emulation", cart_name);
    check = gtk_check_button_new_with_label(title);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                 (gboolean)cartridge_type_enabled(cart_id));

    /* cannot connect unlocked: the signal handler sets a resource */
    handler_id = g_signal_connect(check,
                                  "toggled",
                                  G_CALLBACK(on_cart_enable_check_button_toggled),
                                  GINT_TO_POINTER(cart_id));
    g_object_set_data(G_OBJECT(check),
                      "HandlerId",
                      GULONG_TO_POINTER(handler_id));
    return check;
}
