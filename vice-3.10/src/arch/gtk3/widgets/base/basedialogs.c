/** \file   basedialogs.c
 * \brief   Gtk3 basic dialogs (Info, Yes/No, etc)
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

#include "debug_gtk3.h"
#include "lib.h"
#include "uimachinewindow.h"
#include "ui.h"

#include "basedialogs.h"


/*
 * Forward declarations
 */
static gboolean entry_get_int(GtkWidget *entry, int *value);


/** \brief  Callback function for the integer input dialog
 */
static void (*integer_cb)(GtkDialog *, int, gboolean);



/** \brief  Handler for the 'response' event of the Info dialog
 *
 * \param[in,out]   dialog          Info dialog
 * \param[in]       response_id     response ID (ignored)
 * \param[in]       data            extra event data (ignored)
 */
static void on_response_info(GtkWidget *dialog, gint response_id, gpointer data)
{
    gtk_widget_destroy(dialog);
}


/** \brief  Handler for the 'response' event of the Confirm dialog
 *
 * \param[in,out]   dialog          Info dialog
 * \param[in]       response_id     response ID (ignored)
 * \param[in]       callback        user-defined callback
 */
static void on_response_confirm(GtkDialog *dialog, gint response_id, gpointer callback)
{
    void (*cb)(GtkDialog *, gboolean) = callback;

    cb(dialog, response_id == GTK_RESPONSE_OK);
    gtk_widget_destroy(GTK_WIDGET(dialog));
}


/** \brief  Handler for the 'response' event of the Error dialog
 *
 * \param[in,out]   dialog          error dialog
 * \param[in]       response_id     response ID (ignored)
 * \param[in]       data            extra event data (ignored)
 */
static void on_response_integer(GtkDialog *dialog, gint response_id, gpointer data)
{
    gboolean valid  = FALSE;
    int      result = 0;

    if (response_id == GTK_RESPONSE_ACCEPT) {
        GtkWidget *entry = data;

        /* try to convert entry box contents to integer */
        if (entry_get_int(entry, &result)) {
            /* OK */
            valid = TRUE;
        }
    }
    /* always trigger the callback, UI actions rely on this */
    integer_cb(dialog, result, valid);
    gtk_widget_destroy(GTK_WIDGET(dialog));
}


/** \brief  Handler for the 'response' event of the Integer dialog
 *
 * \param[in,out]   dialog          integer dialog
 * \param[in]       response_id     response ID
 * \param[in]       data            extra event data (ignored)
 */
static void on_response_error(GtkWidget *dialog, gint response_id, gpointer data)
{
    gtk_widget_destroy(dialog);
}

/** \brief  Handler for the 'destroy' event of a dialog
 *
 * Destroys the temporary parent widget \a data
 *
 * \param[in]   dialog      unused
 * \param[in]   data        temporary parent widget
 */
static void on_dialog_destroy(GtkWidget *dialog, gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
}


/** \brief  Create a GtkMessageDialog
 *
 * \param[in]   parent      parent window (can be \c NULL)
 * \param[in]   type        message type
 * \param[in]   buttons     buttons to use
 * \param[in]   title       dialog title
 * \param[in]   text        dialog text, optional marked-up with Pango
 *
 * \return  GtkMessageDialog
 */
static GtkWidget *create_dialog(GtkWindow      *parent,
                                GtkMessageType  type,
                                GtkButtonsType  buttons,
                                const char     *title,
                                const char     *text)
{
    GtkWidget *dialog;
    gboolean   no_parent = FALSE;

    if (parent == NULL) {
        /* set up a temporary parent to avoid Gtk warnings */
        no_parent = TRUE;
        parent = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    }

    dialog = gtk_message_dialog_new(
            parent,
            /* GTK_DIALOG_DESTROY_WITH_PARENT */ 0,
            type, buttons, NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), text);

    /* set up signal handler to destroy the temporary parent window */
    if (no_parent) {
        g_signal_connect_unlocked(G_OBJECT(dialog),
                                  "destroy",
                                  G_CALLBACK(on_dialog_destroy),
                                  (gpointer)parent);
    }

    return dialog;
}


/** \brief  Create 'info' dialog
 *
 * \param[in]   parent  parent window (can be \c NULL)
 * \param[in]   title   dialog title
 * \param[in]   fmt     message format string and arguments
 *
 * \return  dialog
 */
GtkWidget *vice_gtk3_message_info(GtkWindow  *parent,
                                  const char *title,
                                  const char *fmt, ...)
{
    GtkWindow *active_window = NULL;
    GtkWidget *dialog;
    va_list    args;
    char      *buffer;

    va_start(args, fmt);
    buffer = lib_mvsprintf(fmt, args);
    va_end(args);

    dialog = create_dialog(parent, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, title, buffer);

    lib_free(buffer);

    if (parent == NULL) {
        /* no parent provided, assume active emulator window */
        active_window = ui_get_active_window();
    } else {
        active_window = parent;
    }

    if (active_window != NULL) {
        gtk_window_set_transient_for(GTK_WINDOW(dialog), active_window);
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    } else {
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    }

    g_signal_connect_unlocked(G_OBJECT(dialog),
                              "response",
                              G_CALLBACK(on_response_info),
                              NULL);

    gtk_widget_show(dialog);
    return dialog;
}


/** \brief  Create 'confirm' dialog
 *
 * \param[in]   parent      parent window (can be \c NULL)
 * \param[in]   callback    callback function to accept the dialog's result
 * \param[in]   title       dialog title
 * \param[in]   fmt         message format string and arguments
 *
 * \return  dialog
 */
GtkWidget *vice_gtk3_message_confirm(GtkWindow *parent,
                                     void (*callback)(GtkDialog *, gboolean),
                                     const char *title,
                                     const char *fmt, ...)
{
    GtkWindow *active_window;
    GtkWidget *dialog;
    va_list    args;
    char      *buffer;

    va_start(args, fmt);
    buffer = lib_mvsprintf(fmt, args);
    va_end(args);

    dialog = create_dialog(parent, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
            title, buffer);
    lib_free(buffer);

    if (parent == NULL) {
        active_window = ui_get_active_window();
    } else {
        active_window = parent;
    }

    if (active_window != NULL) {
        gtk_window_set_transient_for(GTK_WINDOW(dialog), active_window);
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    } else {
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    }

    /* cannot connect unlocked: the user's callback might set a resource */
    g_signal_connect(G_OBJECT(dialog),
                     "response",
                     G_CALLBACK(on_response_confirm),
                     (gpointer)callback);
    gtk_widget_show(dialog);
    return dialog;
}


/** \brief  Create 'error' dialog
 *
 * Create modal error dialog with either \a parent as parentm or if \a parent
 * is \c NULL the active emu window as parent or if that one isn't created yet,
 * a temporary empty (non-visible) parent window.
 *
 * \param[in]   parent  parent window/dialog
 * \param[in]   title   dialog title
 * \param[in]   fmt     message format string and arguments
 *
 * \return  dialog
 */
GtkWidget *vice_gtk3_message_error(GtkWindow  *parent,
                                   const char *title,
                                   const char *fmt, ...)
{
    GtkWindow *active_window = NULL;
    GtkWidget *dialog;
    va_list    args;
    char      *buffer;

    va_start(args, fmt);
    buffer = lib_mvsprintf(fmt, args);
    va_end(args);

    dialog = create_dialog(parent, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, title, buffer);

    lib_free(buffer);

    if (parent == NULL) {
        /* no parent: assume active emu window */
        active_window = ui_get_active_window();
    } else {
        /* we have a proper parent: */
        active_window = parent;
    }
    if (active_window != NULL) {
        gtk_window_set_transient_for(GTK_WINDOW(dialog), active_window);
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    } else {
        /* no emu window present yet (UI hasn't  properly started), center on
         * screen (if the WM accept this) */
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    }

    g_signal_connect_unlocked(G_OBJECT(dialog),
                              "response",
                              G_CALLBACK(on_response_error),
                              NULL);

    gtk_widget_show(dialog);
    return dialog;
}


/** \brief  Try to convert the text in \a entry into an integer value
 *
 * \param[in]   entry   GtkTextEntry
 * \param[out]  value   object to store integer result
 *
 * \return  TRUE when conversion succeeded, FALSE otherwise
 *
 * \note    When FALSE is returned, the value pointed at by \a value is
 *          unchanged
 */
static gboolean entry_get_int(GtkWidget *entry, int *value)
{
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    char *endptr;
    long tmp;

    tmp = strtol(text, &endptr, 0);
    if (*endptr == '\0') {
        *value = (int)tmp;
        return TRUE;
    }
    return FALSE;
}


/** \brief  Handler for the key-press event of the integer input box
 *
 * Signals ACCEPT to the dialog.
 *
 * \param[in]   entry   entry box
 * \param[in]   event   event object
 * \param[in]   data    dialog reference
 *
 * \return  TRUE if Enter was pushed, FALSE otherwise (makes the pushed key
 *          propagate to the entry
 */
static gboolean on_integer_key_press_event(GtkEntry *entry,
                                           GdkEvent *event,
                                           gpointer  data)
{
    GtkWidget   *dialog = data;
    GdkEventKey *keyev  = (GdkEventKey *)event;

    if (keyev->type == GDK_KEY_PRESS && keyev->keyval == GDK_KEY_Return) {
        /* got Enter */
        g_signal_emit_by_name(dialog, "response", GTK_RESPONSE_ACCEPT, NULL);
    }
    return FALSE;
}


/** \brief  Create a dialog to enter an integer value
 *
 * \param[in]   callback    callback function to accept result
 * \param[in]   title       dialog title
 * \param[in]   message     dialog body text
 * \param[in]   old_value   current value of whatever needs to be changed
 * \param[in]   min         minimal valid value
 * \param[in]   max         maximum valid value
 *
 * \return  dialog
 *
 * TODO: check input while entering (marking any invalid value red or so)
 */
GtkWidget *vice_gtk3_integer_input_box(
        void (*callback)(GtkDialog *, int, gboolean),
        const char *title,
        const char *message,
        int old_value,
        int min,
        int max)
{
    GtkWidget *dialog;
    GtkWidget *content;
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *entry;
    char *text;
    char buffer[1024];

    integer_cb = callback;

    dialog = gtk_dialog_new_with_buttons(title, ui_get_active_window(),
            GTK_DIALOG_MODAL,
            "Accept", GTK_RESPONSE_ACCEPT,
            "Cancel", GTK_RESPONSE_REJECT,
            NULL);
    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    gtk_window_set_transient_for(GTK_WINDOW(dialog), ui_get_active_window());

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 16);
    gtk_widget_set_margin_start(grid, 16);
    gtk_widget_set_margin_end(grid, 16);

    /* add body message text */
    label = gtk_label_new(message);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_line_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);

    /* add info on limits */
    text = lib_msprintf("(enter a number between %d and %d)", min, max);
    label = gtk_label_new(text);
    lib_free(text);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 2, 1);

    label = gtk_label_new("Enter new value:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, FALSE);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 1, 1);

    /* add the text entry */
    entry = gtk_entry_new();
    g_snprintf(buffer, sizeof(buffer), "%d", old_value);
    gtk_entry_set_text(GTK_ENTRY(entry), buffer);

    gtk_widget_set_hexpand(entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), entry, 1, 2, 1 ,1);

    gtk_widget_show_all(grid);
    gtk_box_pack_start(GTK_BOX(content), grid, TRUE, TRUE, 8);

    /* cannot connect unlocked: this handler emits the "accept" signal of the
     * dialog and the callback could set a resource */
    g_signal_connect(G_OBJECT(dialog),
                     "key-press-event",
                     G_CALLBACK(on_integer_key_press_event),
                     (gpointer)dialog);
    /* cannot connect unlocked either */
    g_signal_connect(G_OBJECT(dialog),
                     "response",
                     G_CALLBACK(on_response_integer),
                     (gpointer)entry);
    gtk_widget_show(dialog);
    return dialog;
}
