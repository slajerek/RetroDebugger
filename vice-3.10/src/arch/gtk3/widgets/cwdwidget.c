/** \file   cwdwidget.c
 * \brief   Widget to set the current working directory
 *
 * GtkEntry with clickable icon to pop up a directory selection dialog to set
 * the current working directory.
 * Failures of g_chdir() will be marked by squiggly crimson lines under the text.
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
#include <glib/gstdio.h>

#include "log.h"
#include "ui.h"
#include "vice_gtk3.h"

#include "cwdwidget.h"


/** \brief  CSS used to mark invalid paths */
#define INVALID_PATH_CSS "entry { text-decoration: crimson wavy underline; }"


/** \brief  CSS provider reference
 *
 * Use to add and remove the style for invalid paths.
 */
static GtkCssProvider *provider = NULL;


/** \brief  Create CSS provider to mark invalid paths
 */
static void create_css_provider(void)
{
    GError *error = NULL;

    provider = gtk_css_provider_new();
    if (!gtk_css_provider_load_from_data(provider, INVALID_PATH_CSS, -1, &error)) {
        log_error(LOG_DEFAULT, "%s(): %s", __func__, error->message);
        g_error_free(error);
    }
}

/** \brief  Add CSS provider marking invalid paths
 *
 * \param[in]   entry   entry widget
 */
static void add_css_provider(GtkEditable *entry)
{
    GtkStyleContext *context;

    context = gtk_widget_get_style_context(GTK_WIDGET(entry));
    gtk_style_context_add_provider(context,
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
}

/** \brief  Remove CSS provider marking invalid paths
 *
 * \param[in]   entry   entry widget
 */
static void remove_css_provider(GtkEditable *entry)
{
    GtkStyleContext *context;

    context = gtk_widget_get_style_context(GTK_WIDGET(entry));
    gtk_style_context_remove_provider(context, GTK_STYLE_PROVIDER(provider));
}

/** \brief  Handler for the "changed" event of the text entry box
 *
 * Try to set the current working directory with g_chdir(), on failure CSS is
 * added to the entry marking the text invalid.
 *
 * \param[in]   self    entry widget
 * \param[in]   data    event data (NULL)
 */
static void on_changed(GtkEditable *self, gpointer data)
{
    const char *cwd = gtk_entry_get_text(GTK_ENTRY(self));

    if (g_chdir(cwd) == 0) {
        remove_css_provider(self);
    } else {
        add_css_provider(self);
    }
}

/** \brief  Handler for the 'destroy' event of the entry
 *
 * Unrefs the CSS provider.
 *
 * \param[in]   self    entry widget (ignored)
 * \param[in]   data    event data (NULL)
 */
static void on_destroy(GtkWidget *self, gpointer data)
{
    if (provider != NULL) {
        g_object_unref(provider);
        provider = NULL;
    }
}

/** \brief  Handler for the 'response' event of the file chooser dialog
 *
 * In case of `GTK_RESPONSE_ACCEPT` the path of the dialog will be used to set
 * the current working directory.
 *
 * \param[in]   self        file chooser dialog
 * \param[in]   response_id response ID
 * \param[in]   data        event data (entry widget)
 */
static void on_response(GtkDialog *self, gint response_id, gpointer data)
{
    GtkEntry *entry = data;
    char     *path  = NULL;

    switch (response_id) {
        case GTK_RESPONSE_ACCEPT:
            path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(self));
            if (path != NULL) {
                /* the 'changed' signal handler will call g_chdir() and add or
                 * remove the CSS provider for errors */
                gtk_entry_set_text(entry, path);
                g_free(path);
            }
            break;
        default:
            break;
    }
    gtk_widget_destroy(GTK_WIDGET(self));
}

/** \brief  Handler for the 'icon-press' event of the entry
 *
 * Show file chooser dialog to select an existing directory.
 *
 * \param[in]   self        entry widget
 * \param[in]   icon_pos    icon position in \a self (ignored)
 * \param[in]   event       event object (ignored)
 * \param[in]   data        event data (NULL)
 */
static void on_icon_press(GtkEntry             *self,
                          GtkEntryIconPosition  icon_pos,
                          GdkEvent             *event,
                          gpointer              data)
{
    GtkWindow *window;
    GtkWidget *dialog;
    char      *curdir;

    window = ui_get_active_window();
    curdir = g_get_current_dir();
    dialog = gtk_file_chooser_dialog_new("Select working directory",
                                         window,
                                         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                         "Accept", GTK_RESPONSE_ACCEPT,
                                         "Cancel", GTK_RESPONSE_CANCEL,
                                         NULL);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), curdir);
    g_free(curdir);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), window);

    g_signal_connect_unlocked(G_OBJECT(dialog),
                              "response",
                              G_CALLBACK(on_response),
                              (gpointer)self);
    gtk_widget_show(dialog);
}


/** \brief  Create widget to change the current working directory
 *
 * Create a GtkEntry with a clickable icon to pop up a file chooser to select
 * an existing directory to use for the current working directory.
 *
 * \return  GtkEntry
 */
GtkWidget *cwd_widget_create(void)
{
    GtkWidget *entry;
    char      *curdir;

    create_css_provider();

    entry  = gtk_entry_new();
    curdir = g_get_current_dir();
    gtk_entry_set_text(GTK_ENTRY(entry), curdir);
    g_free(curdir);
    gtk_widget_set_hexpand(entry, TRUE);
    /* add clickable icon to pop up a directory selector dialog */
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                      "document-open-symbolic");
    gtk_entry_set_icon_tooltip_markup(GTK_ENTRY(entry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                      "Select working directory");
    /* since we're not touching any resources or other VICE internals we can
     * avoid obtaining the VICE lock: */
    g_signal_connect_unlocked(G_OBJECT(entry),
                              "changed",
                              G_CALLBACK(on_changed),
                              NULL);
    g_signal_connect_unlocked(G_OBJECT(entry),
                              "icon-press",
                              G_CALLBACK(on_icon_press),
                              NULL);
    g_signal_connect_unlocked(G_OBJECT(entry),
                              "destroy",
                              G_CALLBACK(on_destroy),
                              NULL);
    return entry;
}
