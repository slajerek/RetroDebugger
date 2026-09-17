/** \file   jamdialog.c
 * \brief   Gtk3 CPU jam dialog
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
 */


#include "vice.h"

#include <gtk/gtk.h>

#include "archdep.h"
#include "debug_gtk3.h"
#include "uiapi.h"
#include "uidata.h"

#include "jamdialog.h"


/** \brief  Custom dialog response codes
 *
 * These are used to enumerate the response codes for the custom buttons of the
 * dialog, in turn determining which UI_JAM_ACTION to return to the emulator.
 */
enum {
    RESPONSE_NONE = 1,      /**< don't do anything */
    RESPONSE_RESET_CPU,     /**< reset machine CPU */
    RESPONSE_POWER_CYCLE,   /**< power cycle machine */
    RESPONSE_MONITOR,       /**< open monitor */
    RESPONSE_QUIT           /**< quit emulator */
};


/** \brief  Create CPU JAM dialog
 *
 * \param[in]   parent  parent widget
 * \param[in]   msg     message to display
 *
 * \return  JAM action response
 */
ui_jam_action_t jam_dialog(GtkWidget *parent, const char *msg)
{
    GtkWidget *dialog;
    GtkWidget *content;
    GtkWidget *label;
    ui_jam_action_t result = UI_JAM_NONE;

    /*
     * No point in making this asynchronous I think, the emulation is paused
     * anyway due to the CPU jam.
     */
    dialog = gtk_dialog_new_with_buttons("D'OH!", GTK_WINDOW(parent),
            GTK_DIALOG_MODAL,
            "Continue",             RESPONSE_NONE,
            "Reset machine CPU",    RESPONSE_RESET_CPU,
            "Power cycle machine",  RESPONSE_POWER_CYCLE,
            "Activate monitor",     RESPONSE_MONITOR,
            "Quit",                 RESPONSE_QUIT,
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
        case RESPONSE_NONE:             /* fall through */
        case GTK_RESPONSE_DELETE_EVENT:
            result = UI_JAM_NONE;
            break;
        case RESPONSE_RESET_CPU:
            result = UI_JAM_RESET_CPU;
            break;
        case RESPONSE_POWER_CYCLE:
            result = UI_JAM_POWER_CYCLE;
            break;
        case RESPONSE_MONITOR:
            result = UI_JAM_MONITOR;
            break;
        case RESPONSE_QUIT:
            gtk_widget_destroy(dialog);
            archdep_vice_exit(0);
            break;
        default:
            /* shouldn't get here */
            break;
    }

    gtk_widget_destroy(dialog);
    return result;
}
