/** \file   v364speechwidget.c
 * \brief   V364 Speech widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES SpeechEnabled   xplus4
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

#include "v364speechwidget.h"


/** \brief  Reference to the Plus4 v364 speech widget
 *
 * We can get away with this since there will only ever be a single Plus4
 * Speech widget active in the UI.
 */
static GtkWidget *instance = NULL;

/** \brief  User callback to trigger on widget value changes
 *
 * Optional, set via v364_speech_widget_add_callback()
 */
static void (*user_callback)(GtkWidget *, int value) = NULL;


/** \brief  Handler for the "toggled" event of the Enable check button
 *
 * \param[in]   button      check button
 * \param[in]   data        unused
 */
static void on_enable_toggled(GtkToggleButton *button, gpointer data)
{
    if (user_callback != NULL) {
        user_callback(GTK_WIDGET(button),
                      gtk_toggle_button_get_active(button));
    }
}


/** \brief  Create V364 Speech widget
 *
 * \return  GtkGrid
 */
GtkWidget *v364_speech_widget_create(void)
{
    instance = vice_gtk3_resource_check_button_new("SpeechEnabled",
                                                   "Enable V364 Speech");

    /* set up extra signal handler to trigger the optional callback */
    user_callback = NULL;
    g_signal_connect(instance, "toggled", G_CALLBACK(on_enable_toggled), NULL);

    return instance;
}


/** \brief  Synchronize v364 widget's state with its resource
 */
void v364_speech_widget_sync(void)
{
    vice_gtk3_resource_check_button_sync(instance);
}


/** \brief  Add user callback to trigger on widget changes
 *
 * When the state of the v364 widget changes, the callback is called with two
 * arguments: the v364 widget and the new value (bool).
 *
 * \param[in]   callback    user callback function
 */
void v364_speech_widget_add_callback(void (*callback)(GtkWidget *, int value))
{
    user_callback = callback;
}
