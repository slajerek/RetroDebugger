/** \file   extendimagedialog.c
 * \brief   Gtk3 extend image dialog dialog
 *
 * \author  groepaz <groepaz@gmx.net>
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

#include "archdep.h"
#include "debug_gtk3.h"
#include "ui.h"
#include "uiapi.h"

#include "extendimagedialog.h"


/** \brief  Custom dialog response codes
 *
 * These are used to enumerate the response codes for the custom buttons of the
 * dialog, in turn determining which UI_EXTEND_IMAGE* value to return to the
 * emulator.
 */
enum {
    RESPONSE_NEVER = 0,     /**< don't extend */
    RESPONSE_EXTEND = 1,    /**< extend */
};


/** \brief  Create extend image dialog
 *
 * \param[in]   parent  parent widget
 * \param[in]   msg     message to display
 *
 * \return  extend image action response
 */
ui_extendimage_action_t extendimage_dialog(GtkWidget *parent, const char *msg)
{
    GtkWidget *dialog;
    GtkWidget *content;
    GtkWidget *label;
    ui_extendimage_action_t result = UI_EXTEND_IMAGE_INVALID;

    /*
     * No point in making this asynchronous I think, the emulation is paused
     * anyway due to the CPU jam.
     */
    dialog = gtk_dialog_new_with_buttons(
            "Extend disk image?",
            ui_get_active_window(),
            GTK_DIALOG_MODAL,
            "No, do not extend", RESPONSE_NEVER,
            "Yes, extend", RESPONSE_EXTEND,
            NULL);

    /*
     * Add label with message to the content area and make the label
     * wrap at word boundaries
     */
    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    label = gtk_label_new(msg);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_line_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD);
    gtk_box_pack_start(GTK_BOX(content), label, FALSE, FALSE, 16);
    gtk_widget_show_all(content);


    switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
        case RESPONSE_NEVER:
        case GTK_RESPONSE_DELETE_EVENT:
            result = UI_EXTEND_IMAGE_NEVER;
            break;
        case RESPONSE_EXTEND:
            result = UI_EXTEND_IMAGE_ALWAYS;
            break;
        default:
            /* shouldn't get here */
            break;
    }

    gtk_widget_destroy(dialog);
    return result;
}
