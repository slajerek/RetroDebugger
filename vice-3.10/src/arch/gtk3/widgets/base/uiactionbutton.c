/** \file   uiactionbutton.c
 * \brief   Simple push button to trigger a UI action
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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
#include "uiactions.h"

#include "uiactionbutton.h"


/** \brief  Handler for the 'clicked' event of the button
 *
 * Trigger UI \a action.
 *
 * \param[in]   self    button (unused)
 * \param[in]   action  UI action ID
 */
static void on_clicked(GtkButton *self, gpointer action)
{
    ui_action_trigger(GPOINTER_TO_INT(action));
}


/** \brief  Create new button to trigger a UI action
 *
 * \param[in]   action  UI action ID
 * \param[in]   label   label for the button
 *
 * \return  new GtkButton
 */
GtkWidget *ui_action_button_new(int action, const char *label)
{
    GtkWidget *button = gtk_button_new_with_label(label);

    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_clicked),
                     GINT_TO_POINTER(action));
    gtk_widget_show(button);
    return button;
}
