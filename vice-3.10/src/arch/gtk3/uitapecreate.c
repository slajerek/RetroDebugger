/** \file   uitapecreate.c
 * \brief   Gtk3 dialog to create and attach a new tape image
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
#include <string.h>

#include "basewidgets.h"
#include "basedialogs.h"
#include "widgethelpers.h"
#include "filechooserhelpers.h"
#include "util.h"
#include "lib.h"
#include "attach.h"
#include "diskimage.h"  /* for DISK_IMAGE_TYPE_TAP*/
#include "cbmimage.h"
#include "resources.h"
#include "tape.h"
#include "ui.h"
#include "uiapi.h"
#include "uiactions.h"

#include "uitapecreate.h"


/* forward declarations of functions */
static gboolean create_tape_image(GtkWindow *parent, const char *filename, int port);


/** \brief  Reference to the 'auto-attach' check button
 */
static GtkWidget *auto_attach = NULL;


/** \brief  Handler for the 'destroy' event of the dialog
 *
 * \param[in]   dialog  dialog (unused)
 * \param[in]   port    port number
 */
static void on_destroy(GtkWidget *dialog, gpointer port)
{
    if (GPOINTER_TO_INT(port) == 1) {
        ui_action_finish(ACTION_TAPE_CREATE_1);
    } else {
        ui_action_finish(ACTION_TAPE_CREATE_2);
    }
}


/** \brief  Handler for 'response' event of the dialog
 *
 * This handler is called when the user clicks a button in the dialog.
 *
 * \param[in]   dialog      the dialog
 * \param[in]   response_id response ID
 * \param[in]   data        port number (integer, 1 or 2)
 */
static void on_response(GtkDialog *dialog, gint response_id, gpointer data)
{
    gchar    *filename;
    gboolean  status = TRUE;
    int       port = GPOINTER_TO_INT(data);

    switch (response_id) {

        case GTK_RESPONSE_ACCEPT:
            filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            if (filename != NULL) {
                gchar *filename_locale = file_chooser_convert_to_locale(filename);

                /* create tape */
                status = create_tape_image(GTK_WINDOW(dialog), filename_locale, port);
                g_free(filename_locale);
            }
            g_free(filename);
            if (status) {
                /* image creation and attaching was succesful, exit dialog */
                gtk_widget_destroy(GTK_WIDGET(dialog));
            }
            break;

        case GTK_RESPONSE_REJECT:
            gtk_widget_destroy(GTK_WIDGET(dialog));
            break;

        default:
            break;
    }
}


/** \brief  Actually create the tape image and attach it
 *
 * \param[in]   parent      parent dialog
 * \param[in]   filename    filename of the new image
 * \param[in]   port        datasette port number (1 or 2)
 *
 * \return  bool
 */
static gboolean create_tape_image(GtkWindow  *parent,
                                  const char *filename,
                                  int         port)
{
    char     *fname_copy;
    gboolean  status = TRUE;

    /* fix extension */
    fname_copy = util_add_extension_const(filename, "tap");

    /* try to create the image */
    if (cbmimage_create_image(fname_copy, DISK_IMAGE_TYPE_TAP) < 0) {
        vice_gtk3_message_error(parent,
                                "VICE error",
                                "Failed to create tape image '%s'",
                                fname_copy);
        status = FALSE;
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_attach))) {
        /* try to attach the image */
        if (tape_image_attach(port, fname_copy) < 0) {
            vice_gtk3_message_error(parent,
                                    "Failed to attach tape image '%s' to port #%d",
                                    fname_copy, port);
            status = FALSE;
        }
    }

    lib_free(fname_copy);
    return status;
}


/** \brief  Create the 'extra' widget for the dialog
 *
 * \return  GtkGrid
 */
static GtkWidget *create_extra_widget(void)
{
    GtkWidget *grid;

    /* create a grid with some spacing and margins */
    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);
    gtk_widget_set_margin_start(grid, 16);
    gtk_widget_set_margin_end(grid, 16);

    auto_attach = gtk_check_button_new_with_label("Auto-attach tape image");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_attach), TRUE);
    gtk_grid_attach(GTK_GRID(grid), auto_attach, 0, 0, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create and show 'attach new tape image' dialog
 *
 * \param[in]   port    port number (1 or 2)
 *
 * \return  TRUE
 */
void ui_tape_create_dialog_show(int port)
{
    GtkWidget *dialog;
    GtkFileFilter *filter;

    dialog = gtk_file_chooser_dialog_new(
            "Create and attach a new tape image",
            ui_get_active_window(),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            /* buttons */
            "Save", GTK_RESPONSE_ACCEPT,
            "Close", GTK_RESPONSE_REJECT,
            NULL, NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
            TRUE);

    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog),
            create_extra_widget());

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Tape images (*.tap)");
    gtk_file_filter_add_pattern(filter, "*.tap");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    g_signal_connect(dialog,
                     "response",
                     G_CALLBACK(on_response),
                     GINT_TO_POINTER(port));
    g_signal_connect(dialog,
                     "destroy",
                     G_CALLBACK(on_destroy),
                     GINT_TO_POINTER(port));

    gtk_widget_show(dialog);
}
