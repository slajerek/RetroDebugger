/** \file   plus4aciawidget.c
 * \brief   Widget to control Plus 4 ACIA
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES Acia1Enable     xplus4
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

#include "machine.h"
#include "resources.h"
#include "debug_gtk3.h"
#include "widgethelpers.h"
#include "basewidgets.h"

#include "plus4aciawidget.h"


/** \brief  Reference to the Plus4 ACIA widget
 *
 * We can get away with this since there will only ever be a single Plus4 ACIA
 * widget active in the UI.
 */
static GtkWidget *instance = NULL;

/** \brief  User callback to trigger on widget value changes
 *
 * Optional, set via plus4_acia_widget_add_callback()
 */
static void (*user_callback)(GtkWidget*, int) = NULL;


/** \brief  Extra handler for the 'toggled' event of the ACIA widget
 *
 * Calls the user callback function if that is defined.
 *
 * \param[in]   button  ACIA widget
 * \param[in]   data    extra event data (unused)
 */
static void on_acia_widget_toggled(GtkToggleButton *button,
                                   gpointer         data)
{
    int active = gtk_toggle_button_get_active(button);
    if (user_callback != NULL) {
        user_callback(GTK_WIDGET(button), active);
    }
}


/** \brief  Create widget to control Plus4 ACIA
 *
 * \return  GtkCheckButton
 */
GtkWidget *plus4_acia_widget_create(void)
{
    instance = vice_gtk3_resource_check_button_new("Acia1Enable",
                                                   "Enable ACIA");
    g_signal_connect(instance,
                     "toggled",
                     G_CALLBACK(on_acia_widget_toggled),
                     NULL);

    user_callback = NULL;
    return instance;
}


/** \brief  Synchronize ACIA widget's state with its resource
 */
void plus4_acia_widget_sync(void)
{
    vice_gtk3_resource_check_button_sync(instance);
}


/** \brief  Add user callback to trigger on widget changes
 *
 * When the state of the ACIA widget changes, the callback is called with two
 * arguments: the ACIA widget and the new value (bool).
 *
 * \param[in]   callback    user callback function
 */
void plus4_acia_widget_add_callback(void (*callback)(GtkWidget *, int value))
{
    user_callback = callback;
}
