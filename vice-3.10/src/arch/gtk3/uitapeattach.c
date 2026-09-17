/** \file   uitapeattach.c
 * \brief   GTK3 tape-attach dialog
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 * \author  Michael C. Martin <mcmartin@gmail.com>
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

#include "attach.h"
#include "autostart.h"
#include "log.h"
#include "tape.h"
#include "debug_gtk3.h"
#include "basedialogs.h"
#include "contentpreviewwidget.h"
#include "filechooserhelpers.h"
#include "imagecontents.h"
#include "lastdir.h"
#include "resources.h"
#include "tapecontents.h"
#include "tapeport.h"
#include "ui.h"
#include "uiactions.h"
#include "uiapi.h"
#include "uimachinewindow.h"

#include "uitapeattach.h"


/** \brief  File type filters for the dialog
 */
static ui_file_filter_t filters[] = {
    { "Tape images", file_chooser_pattern_tape },
    { "All files", file_chooser_pattern_all },
    { NULL, NULL }
};

/** \brief  Reference to the preview widget
 */
static GtkWidget *preview_widget = NULL;


/** \brief  Last directory used
 *
 * When a tape is attached, this is set to the directory of that file. Since
 * it's heap-allocated by Gtk3, it must be freed with a call to
 * ui_tape_attach_shutdown() on emulator shutdown.
 */
static gchar *last_dir = NULL;

/** \brief  Last filename used */
static gchar *last_file = NULL;

/** \brief  Reference to the custom 'Autostart' button
 */
static GtkWidget *autostart_button;


/** \brief  Handler for the "update-preview" event
 *
 * \param[in]   chooser file chooser dialog
 * \param[in]   data    extra event data (unused)
 */
static void on_update_preview(GtkFileChooser *chooser, gpointer data)
{
    GFile *file;
    gchar *path;

    file = gtk_file_chooser_get_preview_file(chooser);
    if (file != NULL) {
        path = g_file_get_path(file);
        if (path != NULL) {
            gchar *path_locale = file_chooser_convert_to_locale(path);

            content_preview_widget_set_image(preview_widget, path_locale);
            g_free(path);
            g_free(path_locale);
        }
        g_object_unref(file);
    }
}


/** \brief  Handler for the 'toggled' event of the 'show hidden files' checkbox
 *
 * \param[in]   widget      checkbox triggering the event
 * \param[in]   user_data   data for the event (the dialog)
 */
static void on_hidden_toggled(GtkWidget *widget, gpointer user_data)
{
    int state;

    state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(user_data), state);
}


/** \brief  Trigger autostart
 *
 * \param[in]   widget      dialog
 * \param[in]   port        tape port number (1 or 2)
 * \param[in]   index       file index in the directory preview
 * \param[in]   autostart   issue "RUN:" after loading
 */
static void do_autostart(GtkWidget *widget, int port, int index, int autostart)
{
    gchar *filename;
    gchar *filename_locale;
#if 0
    debug_gtk3("Got port %d, index %d, autostart %d.", port, index, autostart);
#endif
    lastdir_update(widget, &last_dir, &last_file);
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    filename_locale = file_chooser_convert_to_locale(filename);
    if (autostart_tape(
                filename_locale,
                NULL,   /* program name */
                index,
                autostart ? AUTOSTART_MODE_RUN : AUTOSTART_MODE_LOAD,
                port - 1    /* function uses 0/1 */) < 0) {
        /* oeps */
        log_error(LOG_DEFAULT, "autostarting tape '%s' failed.", filename_locale);
        vice_gtk3_message_error(GTK_WINDOW(widget),
                               "Autostart error",
                               "Autostarting tape '%s' failed.",
                               filename_locale);
    }
    g_free(filename_locale);
}


/** \brief  Attach image to datasette
 *
 * \param[in]   widget  dialog
 * \param[in]   port    datasette port (1 or 2 (xpet))
 */
static void do_attach(GtkWidget *widget, int port)
{
    gchar *filename;
    gchar *filename_locale;

    lastdir_update(widget, &last_dir, &last_file);
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    /* ui_message("Opening file '%s' ...", filename); */

    filename_locale = file_chooser_convert_to_locale(filename);

    if (tape_image_attach(TAPEPORT_PORT_1 + port, filename_locale) < 0) {
        /* failed */
        log_error(LOG_DEFAULT,
                  "attaching tape '%s' to port #%d failed.",
                  filename_locale, port);
        vice_gtk3_message_error(GTK_WINDOW(widget),
                                "Attach error",
                                "Attaching tape '%s' to port #%d failed.",
                                filename_locale, port);
    }
    g_free(filename_locale);
}


/** \brief  Handler for 'selection-changed' event of the preview widget
 *
 * Checks if a proper selection was made and activates the 'Autostart' button
 * if so, disabled it otherwise
 *
 * \param[in]   chooser Parent dialog
 * \param[in]   data    Extra event data (unused)
 */
static void on_selection_changed(GtkFileChooser *chooser, gpointer data)
{
    gchar *filename;

    filename = gtk_file_chooser_get_filename(chooser);
    if (filename != NULL) {
        gtk_widget_set_sensitive(autostart_button, TRUE);
        g_free(filename);
    } else {
        gtk_widget_set_sensitive(autostart_button, FALSE);
    }
}


/** \brief  Handler for the 'destroy' event of the dialog
 *
 * Marks the appropriate UI action as finished.
 *
 * \param[in]   dialog  dialog (unused)
 * \param[in]   port    port number
 */
static void on_destroy(GtkWidget *dialog, gpointer port)
{
    if (GPOINTER_TO_INT(port) == 1) {
        ui_action_finish(ACTION_TAPE_ATTACH_1);
    } else {
        ui_action_finish(ACTION_TAPE_ATTACH_2);
    }
}


/** \brief  Handler for 'response' event of the dialog
 *
 * This handler is called when the user clicks a button in the dialog.
 *
 * \param[in]   widget      the dialog
 * \param[in]   response_id response ID
 * \param[in]   user_data   unit number
 */
static void on_response(GtkWidget *widget, gint response_id, gpointer user_data)
{
    gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    int index = content_preview_widget_get_index(preview_widget);
    int autostart = 0;
    int unit;

    resources_get_int("AutostartOnDoubleClick", &autostart);
    unit = GPOINTER_TO_INT(user_data);
#if 0
    debug_gtk3("Got port %d.", port);
    debug_gtk3("Index %d.", index);
#endif
    /* first, to make the following logic less funky, map some events to others,
       depending on whether autostart-on-doubleclick is enabled or not, and
       depending on the event coming from the preview window or not. */
    switch (response_id) {
        /* double-click on file in the preview widget when autostart-on-doubleclick is NOT enabled */
        case VICE_RESPONSE_AUTOLOAD_INDEX:
        /* double-click on file in the preview widget when autostart-on-doubleclick is enabled */
        case VICE_RESPONSE_AUTOSTART_INDEX:
            if ((index < 0) || (filename == NULL)) {
                response_id = VICE_RESPONSE_INVALID;   /* drop this event */
            }
            break;
        /* 'Open' button clicked when autostart-on-doubleclick is enabled */
        case VICE_RESPONSE_CUSTOM_OPEN:
            if (filename == NULL) {
                response_id = VICE_RESPONSE_INVALID;   /* drop this event */
            } else if (index >= 0) {
                response_id = VICE_RESPONSE_AUTOLOAD_INDEX;
            }
            break;
        /* double-click on file in the main file chooser,
           'Autostart' button when autostart-on-doubleclick is enabled
           'Open' button when autostart-on-doubleclick is NOT enabled */
        case GTK_RESPONSE_ACCEPT:
            if (filename == NULL) {
                response_id = VICE_RESPONSE_INVALID;   /* drop this event */
            } else if ((index >= 0) && (autostart == 0)) {
                response_id = VICE_RESPONSE_AUTOLOAD_INDEX;
            } else if ((index >= 0) && (autostart == 1)) {
                response_id = VICE_RESPONSE_AUTOSTART_INDEX;
            } else if (autostart == 0) {
                response_id = VICE_RESPONSE_CUSTOM_OPEN;
            } else {
                response_id = VICE_RESPONSE_AUTOSTART;
            }
            break;
        default:
            break;
    }

    switch (response_id) {
        /* 'Open' button clicked when autostart-on-doubleclick is enabled */
        case VICE_RESPONSE_CUSTOM_OPEN:
            do_attach(widget, unit);

            gtk_widget_destroy(widget);
            break;

        /* 'Autostart' button clicked when autostart-on-doubleclick is NOT enabled */
        case VICE_RESPONSE_AUTOSTART:
        /* double-click on file in the preview widget when autostart-on-doubleclick is enabled */
        case VICE_RESPONSE_AUTOSTART_INDEX:
            do_autostart(widget, unit, index + 1, 1);

            gtk_widget_destroy(widget);
            break;

        /* double-click on file in the preview widget when autostart-on-doubleclick is NOT enabled */
        case VICE_RESPONSE_AUTOLOAD_INDEX:
            do_autostart(widget, unit, index + 1, 0);

            gtk_widget_destroy(widget);
            break;

        /* 'Close'/'X' button */
        case GTK_RESPONSE_REJECT:
            gtk_widget_destroy(widget);
            break;
        default:
            break;
    }

    if (filename != NULL) {
        g_free(filename);
    }
}



/** \brief  Create the 'extra' widget
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 *
 * TODO: 'grey-out'/disable units without a proper drive attached
 */
static GtkWidget *create_extra_widget(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *hidden_check;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

    hidden_check = gtk_check_button_new_with_label("Show hidden files");
    g_signal_connect(hidden_check, "toggled", G_CALLBACK(on_hidden_toggled),
            (gpointer)(parent));
    gtk_grid_attach(GTK_GRID(grid), hidden_check, 0, 0, 1, 1);

#if 0
    readonly_check = gtk_check_button_new_with_label("Attach read-only");
    gtk_grid_attach(GTK_GRID(grid), readonly_check, 1, 0, 1, 1);
#endif

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create the tape attach dialog
 *
 * \param[in]   port    tape port (1 or 2 (xpet))
 *
 * \return  GtkFileChooserDialog
 */
static GtkWidget *create_tape_attach_dialog(int port)
{
    GtkWidget *dialog;
    size_t i;
    int autostart = 0;
    char title[256];
#if 0
    debug_gtk3("Got port %d.", port);
#endif
    resources_get_int("AutostartOnDoubleClick", &autostart);
    g_snprintf(title, sizeof(title), "Attach a tape image to port #%d", port);

    /* create new dialog */
    dialog = gtk_file_chooser_dialog_new(
            title,
            ui_get_active_window(),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            /* buttons */
            NULL, NULL);

    /* We need to manually add the buttons here, not using the constructor
     * above, to keep the order of the buttons the same as in the other
     * 'attach' dialogs. Gtk doesn't allow getting references to buttons when
     * added via the constructor, meaning we cannot get a reference to the
     * "Autostart" button in order to "grey it out".
     */
    /* to handle the "double click means autostart" option, we need to always
       connect a button to GTK_RESPONSE_ACCEPT, else double clicks will stop
       working */
    if (autostart) {
        gtk_dialog_add_button(GTK_DIALOG(dialog), "Attach / Load", VICE_RESPONSE_CUSTOM_OPEN);
        autostart_button = gtk_dialog_add_button(GTK_DIALOG(dialog),
                                                "Autostart",
                                                GTK_RESPONSE_ACCEPT);
    } else {
        gtk_dialog_add_button(GTK_DIALOG(dialog), "Attach / Load", GTK_RESPONSE_ACCEPT);
        autostart_button = gtk_dialog_add_button(GTK_DIALOG(dialog),
                                                "Autostart",
                                                VICE_RESPONSE_AUTOSTART);
    }

    gtk_widget_set_sensitive(autostart_button, FALSE);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Close", GTK_RESPONSE_REJECT);

    /* set modal so mouse-grab doesn't get triggered */
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    /* set last directory */
    lastdir_set(dialog, &last_dir, &last_file);

    /* add 'extra' widget: 'readonly' and 'show preview' checkboxes */
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog),
                                      create_extra_widget(dialog));

    preview_widget = content_preview_widget_create(dialog,
                                                   tapecontents_read,
                                                   on_response,
                                                   port);
    gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(dialog),
            preview_widget);

    /* add filters */
    for (i = 0; filters[i].name != NULL; i++) {
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),
                create_file_chooser_filter(filters[i], FALSE));
    }

    /* connect "reponse" handler: the `user_data` argument gets filled in when
     * the "response" signal is emitted: a response ID */
    g_signal_connect(dialog,
                     "response",
                     G_CALLBACK(on_response),
                     GINT_TO_POINTER(port));
    g_signal_connect(dialog,
                     "update-preview",
                     G_CALLBACK(on_update_preview),
                     NULL);
    g_signal_connect_unlocked(dialog,
                              "selection-changed",
                              G_CALLBACK(on_selection_changed),
                              NULL);
    g_signal_connect(dialog,
                     "destroy",
                     G_CALLBACK(on_destroy),
                     GINT_TO_POINTER(port));

    return dialog;

}


/** \brief  Show dialog to attach a tape image to a datasette
 *
 * \param[in]   port    tape port (1 or 2)
 *
 * \return  TRUE (signal to Gtk the event was 'consumed')
 */
void ui_tape_attach_show_dialog(int port)
{
    GtkWidget *dialog = create_tape_attach_dialog(port);
    gtk_widget_show(dialog);
}


/** \brief  Clean up the last directory string
 */
void ui_tape_attach_shutdown(void)
{
    lastdir_shutdown(&last_dir, &last_file);
}
