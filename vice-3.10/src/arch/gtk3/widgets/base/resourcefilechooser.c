/** \file   resourcefilechooser.c
 * \brief   Resource file chooser widget
 *
 * Presents a GtkEntry with an "open document" icon that serves as the "browse"
 * button to open a file chooser dialog.
 * By default no file matching pattern is used and no overwrite confirmation
 * dialog is shown in case of the `GTK_FILE_CHOOSER_ACTION_SAVE` action.
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
#include <stdarg.h>

#include "debug_gtk3.h"
#include "lib.h"
#include "resourcewidgetmediator.h"
#include "ui.h"
#include "util.h"

#include "resourcefilechooser.h"


/** \brief  Additional state for the widget
 */
typedef struct fc_state_s {
    GtkFileChooserAction  action;       /**< chooser type */
    char                 *directory;    /**< default directory */
    char                 *custom_title; /**< custom title for dialog */
    GtkFileFilter        *filter;       /**< file filter for dialog */
    GtkFileFilter        *all_files;    /**< filter for '*' */
    gboolean              confirm;      /**< confirm before overwriting in case
                                             of GTK_FILE_CHOOSER_ACTION_SAVE */
    /**< brief  User-defined function to call on succesfull resource update */
    void                (*callback)(GtkEntry *, gchar *);
} fc_state_t;


/** \brief  Create file chooser state object
 *
 * This object is freed on widget destruction
 */
static fc_state_t *fc_state_new(void)
{
    fc_state_t *state = g_malloc(sizeof *state);
    state->action       = GTK_FILE_CHOOSER_ACTION_OPEN;
    state->directory    = NULL;
    state->confirm      = FALSE;
    state->filter       = NULL;
    state->all_files    = NULL;
    state->custom_title = NULL;
    state->callback     = NULL;
    return state;
}

/** \brief  Free state object and its members
 *
 * \param[in]   state   file chooser state object
 */
static void fc_state_free(void *state)
{
    fc_state_t *st = state;
    g_free(st->directory);
    g_free(st->custom_title);
    if (st->filter != NULL) {
        g_object_unref(st->filter);
    }
    if (st->all_files != NULL) {
        g_object_unref(st->all_files);
    }
    g_free(st);
}

/** \brief  Set default directory
 *
 * Set default directory in \a state, or unset when using `NULL` or an empty
 * string string for \a directory.
 *
 * \param[in]   directory   default directory
 */
static void fc_state_set_directory(fc_state_t *state, const char *directory)
{
    if (state->directory != NULL) {
        g_free(state->directory);
    }
    if (directory != NULL && *directory != '\0') {
        state->directory = g_strdup(directory);
    } else {
        state->directory = NULL;
    }
}

/** \brief  Set resource value and update entry
 *
 * \param[in]   entry   text entry
 * \param[in]   text    new text for resource and entry
 *
 * \return  `TRUE` when the resource was succesfully set
 */
static gboolean set_resource_and_entry(GtkEntry *entry, const char *text)
{
    mediator_t *mediator;
    const char *res_text;
    gboolean    result = TRUE;

    mediator = mediator_for_widget(GTK_WIDGET(entry));
    res_text = mediator_get_resource_string(mediator);
    if (g_strcmp0(res_text, text) != 0) {
        if (!mediator_update_string(mediator, text)) {
            /* set old resource value */
            gtk_entry_set_text(entry, res_text ? res_text : "");
            result = FALSE;
        } else {
            fc_state_t *state = mediator_get_data(mediator);

            /* update entry */
            gtk_entry_set_text(entry, text ? text : "");
            /* call user-defined function, if present */
            if (state->callback != NULL) {
                state->callback(entry, g_strdup(text));
            }
        }
        /* move cursor to the end, the final part of a path is more significant */
        gtk_editable_set_position(GTK_EDITABLE(entry), -1);
    }
    return result;
}

/** \brief  Get title for file chooser dialog/icon tooltip based on action
 *
 * \param[in]   action  file chooser action
 *
 * \return  title for dialog/tooltip
 */
static const char *title_for_action(GtkFileChooserAction action)
{
    switch (action) {
        case GTK_FILE_CHOOSER_ACTION_OPEN:
            return "Select file";
        case GTK_FILE_CHOOSER_ACTION_SAVE:
            return "Select or create file";
        case GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
            return "Select directory";
        case GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER:
            return "Select or create directory";
        default:
            /* shouldn't get here */
            return "Invalid GtkFileChooserAction specified";
    }
}

/** \brief  Handler for 'response' event of a file chooser dialog
 *
 * \param[in]   dialog      dialog emitting the event
 * \param[in]   response_id response ID
 * \param[in]   data        resource file chooser to receive the dialog result
 */
static void on_response(GtkDialog *dialog, gint response_id, gpointer data)
{
    GtkEntry   *entry;
    char       *filename;

    entry = data;
    switch (response_id) {

        case GTK_RESPONSE_ACCEPT:
            filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            set_resource_and_entry(entry, filename);
            g_free(filename);
            break;

        default:
            break;
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/** \brief  Handler for the 'icon-press' event of the widget
 *
 * Show a file choose dialog to select or create a file or directory.
 *
 * \param[in]   self        resource file chooser
 * \param[in]   icon_pos    icon position in \a self (ignored)
 * \param[in]   event       event information (ignored)
 * \param[in]   data        extra event data (unused)
 */
static void on_icon_press(GtkEntry             *self,
                          GtkEntryIconPosition  icon_pos,
                          GdkEvent             *event,
                          gpointer              data)
{
    GtkWindow  *window;
    GtkWidget  *dialog;
    mediator_t *mediator;
    fc_state_t *state;
    const char *title;
    const char *respath;    /* path in the resource */
    char       *dirpart;    /* dirname */
    char       *filepart;   /* basename */

    mediator = mediator_for_widget(GTK_WIDGET(self));
    state    = mediator_get_data(mediator);
    window   = ui_get_active_window();
    if (state->custom_title != NULL) {
        /* use custom title set by user */
        title = state->custom_title;
    } else {
        title = title_for_action(state->action);
    }

    dialog = gtk_file_chooser_dialog_new(title,
                                         window,
                                         state->action,
                                         "Accept", GTK_RESPONSE_ACCEPT,
                                         "Cancel", GTK_RESPONSE_CANCEL,
                                         NULL);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), window);

    /* set user-defined file filter, if set */
    if (state->filter != NULL) {
        /* user-defined filter */
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), state->filter);
        /* add '*' */
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), state->all_files);
    }

    respath  = mediator_get_resource_string(mediator);
    dirpart  = g_path_get_dirname(respath);
    filepart = g_path_get_basename(respath);
#if 0
    g_print("%s(): respath  = \"%s\"\n", __func__, respath);
    g_print("%s(): dirpart  = \"%s\"\n", __func__, dirpart);
    g_print("%s(): filepart = \"%s\"\n", __func__, filepart);
#endif
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), respath);

    /* set action-specific dialog properties */
    switch (state->action) {
        case GTK_FILE_CHOOSER_ACTION_SAVE:
            gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
                                                           state->confirm);
            /* we don't want a period if the path in the resource is empty:
             * (g_path_get_basename("") returns ".")
             */
            if (g_strcmp0(filepart, ".") != 0) {
                gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),
                                                  filepart);
            }
            /* fall through */
        case GTK_FILE_CHOOSER_ACTION_OPEN:
            if ((g_strcmp0(dirpart, ".") == 0) && (state->directory != NULL)) {
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                                    state->directory);
            } else {
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                                    dirpart);
            }
            break;
        case GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER: /* fall through */
        case GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER:
            if (respath == NULL || *respath == '\0') {
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                                    state->directory);
            } else {
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                                    respath);
            }
            break;
        default:
            debug_gtk3("Should never get here!");
            break;
    }

    g_free(dirpart);
    g_free(filepart);

    g_signal_connect(G_OBJECT(dialog),
                     "response",
                     G_CALLBACK(on_response),
                     (gpointer)self);

    gtk_widget_show(dialog);
}

/** \brief  Handler for the 'focus-out' event of the resource file chooser
 *
 * Update resource when the resource file chooser widget looses focus.
 *
 * \param[in]   self    resource file chooser
 * \param[in]   event   event information (ignored)
 * \param[in]   data    extra event data (unused)
 *
 * \return  `GDK_EVENT_PROPAGATE` to keep propagating (required by Gdk)
 */
static gboolean on_focus_out_event(GtkEntry *self,
                                   GdkEvent *event,
                                   gpointer  data)
{
    set_resource_and_entry(self, gtk_entry_get_text(self));
    return GDK_EVENT_PROPAGATE;
}

/** \brief  Handler for the 'key-press-event' event of the resource file chooser
 *
 * Update resource when the user presses Return or Enter.
 *
 * \param[in]   self    resource file chooser
 * \param[in]   event   event information
 * \param[in]   data    extra event data (unused)
 *
 * \return  `GDK_EVENT_PROPAGATE` to keep propagating or `GDK_EVENT_STOP` when
 *          the user pressed Return or Enter
 */
static gboolean on_key_press_event(GtkEntry *self,
                                   GdkEvent *event,
                                   gpointer  data)
{
    guint keyval = 0;

    if (gdk_event_get_event_type(event) == GDK_KEY_PRESS) {
        if (gdk_event_get_keyval(event, &keyval)) {
            if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
                set_resource_and_entry(self, gtk_entry_get_text(self));
                return GDK_EVENT_STOP;
            }
        }
    }
    return GDK_EVENT_PROPAGATE;
}


/** \brief  Create file chooser widget set select or create file or directory
 *
 * \param[in]   resource    resource name
 * \param[in]   action      file chooser action
 *
 * \return  GtkEntry with icon to pop up file chooser dialog
 *
 * \see https://docs.gtk.org/gtk3/enum.FileChooserAction.html
 */
GtkWidget *vice_gtk3_resource_filechooser_new(const char           *resource,
                                              GtkFileChooserAction  action)
{
    GtkWidget  *entry;
    mediator_t *mediator;
    fc_state_t *state;
    const char *path;

    entry  = gtk_entry_new();

    /* create and initialize state object */
    state = fc_state_new();
    state->action = action;

    /* create and set up mediator */
    mediator = mediator_new(entry, resource, G_TYPE_STRING);
    mediator_set_data(mediator, state, fc_state_free);

    /* set up entry */
    path = mediator_get_resource_string(mediator);
    gtk_entry_set_text(GTK_ENTRY(entry), path ? path : "");
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                      "document-open-symbolic");
    gtk_entry_set_icon_tooltip_markup(GTK_ENTRY(entry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                      title_for_action(action));
    g_signal_connect(G_OBJECT(entry),
                     "focus-out-event",
                     G_CALLBACK(on_focus_out_event),
                     NULL);
    g_signal_connect(G_OBJECT(entry),
                     "key-press-event",
                     G_CALLBACK(on_key_press_event),
                     NULL);

    /* set up icon to pop up file chooser dialog */
    g_signal_connect(G_OBJECT(entry),
                     "icon-press",
                     G_CALLBACK(on_icon_press),
                     NULL);

    return entry;
}


/** \brief  Create file chooser widget set select or create file or directory
 *
 * \param[in]   format  resource name format string
 * \param[in]   action  file chooser action
 * \param[in]   ...     arguments for \a format
 *
 * \return  GtkEntry with icon to pop up file chooser dialog
 */
GtkWidget *vice_gtk3_resource_filechooser_new_sprintf(const char           *format,
                                                      GtkFileChooserAction  action,
                                                      ...)
{
    char    resource[256];
    va_list args;

    va_start(args, action);
    g_vsnprintf(resource, sizeof resource, format, args);
    va_end(args);
    return vice_gtk3_resource_filechooser_new(resource, action);
}


/** \brief  Set new pathname for the resource file chooser and its resource
 *
 * \param[in]   widget      resource file chooser
 * \param[in]   pathname    new value
 *
 * \return  `TRUE` on success
 */
gboolean vice_gtk3_resource_filechooser_set(GtkWidget  *widget,
                                            const char *pathname)
{
    return set_resource_and_entry(GTK_ENTRY(widget), pathname);
}


/** \brief  Set default directory to use in the file chooser dialogs
 *
 * Set default directory to use when the resource is empty or only contains
 * a filename.
 *
 * \param[in]   widget      resource file chooser
 * \param[in]   directory   default directory
 */
void vice_gtk3_resource_filechooser_set_directory(GtkWidget  *widget,
                                                  const char *directory)
{
    mediator_t *mediator;
    fc_state_t *state;

    mediator = mediator_for_widget(widget);
    state    = mediator_get_data(mediator);
    fc_state_set_directory(state, directory);
}


/** \brief  Set whether the dialog should ask for confirmation before overwriting
 *
 * Set if the file chooser dialog for save file actions should show a confirmation
 * dialog if the selected file should cause it to be overwritten.
 * This is `FALSE` by default and only has effect for `GTK_FILE_CHOOSER_ACTION_SAVE`.
 *
 * \param[in]   widget  resource file chooser
 * \param[in]   confirm_overwrite   show confirmation dialog when selecting an
 *                                  existing file
 */
void vice_gtk3_resource_filechooser_set_confirm(GtkWidget *widget,
                                                gboolean   confirm_overwrite)
{
    mediator_t *mediator;
    fc_state_t *state;

    mediator = mediator_for_widget(widget);
    if (mediator != NULL) {
        state = mediator_get_data(mediator);
        state->confirm = confirm_overwrite;
    }
}


/** \brief  Add file filter for the file chooser dialog
 *
 * Add glob-style patterns to filter displayed files in the dialog.
 * If \a show_patterns is true, the \a name will be combined with the patterns
 * into "name (pattern1,pattern2,patter3)".
 *
 * \param[in]   widget          resource file chooser
 * \param[in]   name            name of filter
 * \param[in]   patterns        shell glob-style patterns, terminated with `NULL`
 * \param[in]   show_patterns   display list of patterns after the \a name
 */
void vice_gtk3_resource_filechooser_set_filter(GtkWidget   *widget,
                                               const char  *name,
                                               const char **patterns,
                                               gboolean     show_patterns)
{
    GtkFileFilter *filter;
    mediator_t    *mediator;
    fc_state_t    *state;
    int            index;

    mediator = mediator_for_widget(widget);
    state    = mediator_get_data(mediator);
    filter   = gtk_file_filter_new();

    for (index = 0; patterns[index] != NULL; index++) {
        gtk_file_filter_add_pattern(filter, patterns[index]);
    }

    if (show_patterns) {
        char  buffer[256];
        char *joined;

        /* can't use g_strjoinv() here since the list is a `gchar**` (no const) */
        joined = util_strjoin(patterns, ",");
        g_snprintf(buffer, sizeof buffer, "%s (%s)", name, joined);
        lib_free(joined);
        gtk_file_filter_set_name(filter, buffer);
    } else {
        gtk_file_filter_set_name(filter, name);
    }

    /* Sink reference, we'll be using this object every time we create a
     * file chooser dialog. If we don't sink the reference the dialog will
     * take ownership of this floating reference and free it when the dialog
     * is destroyed. */
    g_object_ref_sink(filter);
    state->filter = filter;

    /* Pattern for '*', used to allow selecting all files when the user sets
     * a filter using this method. */
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "All files (*)");
    gtk_file_filter_add_pattern(filter, "*");
    /* Again sink ref for reuse */
    g_object_ref_sink(filter);
    state->all_files = filter;
}


/** \brief  Set custom title to use for file chooser dialogs
 *
 * \param[in]   widget  resource file chooser
 * \param[in]   title   custom title for dialog of the icon
 */
void vice_gtk3_resource_filechooser_set_custom_title(GtkWidget  *widget,
                                                     const char *title)
{
    mediator_t *mediator = mediator_for_widget(widget);
    if (mediator != NULL) {
        fc_state_t *state = mediator_get_data(mediator);
        state->custom_title = g_strdup(title);
    }
}

/** \brief  Set function to be called on succesfull update of the resource
 *
 * Set function that will be called with the new resource value and a reference
 * to the \a widget when the bound resource is successfully updated.
 *
 * The string passed to the \a callback is allocated using g_strdup() and should
 * be freed with g_free().
 *
 * \param[in]   widget      resource filechooser widget
 * \param[in]   callback    function to call on succesfull resource update
 */
void vice_gtk3_resource_filechooser_set_callback(GtkWidget *widget,
                                                 void (*callback)(GtkEntry *, gchar *))
{
    mediator_t *mediator = mediator_for_widget(widget);
    if (mediator != NULL) {
        fc_state_t *state = mediator_get_data(mediator);
        state->callback = callback;
    }
}


/** \brief  Synchronize resource file chooser with its resource
 *
 * \param[in]   widget  resource file chooser
 */
void vice_gtk3_resource_filechooser_sync(GtkWidget *widget)
{
    mediator_t *mediator = mediator_for_widget(widget);
    if (mediator != NULL) {
        const char *value = mediator_get_resource_string(mediator);

        gtk_entry_set_text(GTK_ENTRY(widget), value != NULL ? value : "");
    }
}
