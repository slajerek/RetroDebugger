/** \file   resourcebrowser.c
 * \brief   Text entry with label and browse button connected to a resource
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * This class presents a text entry box and a "Browse ..." button to update
 * a resource, optionally providing a label before the text entry. It is meant
 * as a widget to set a resource that represents a file, such as a kernal image.
 *
 * Internally this widget is a GtkGrid, so when a dialog/widget needs multiple
 * instances of this class, the best thing to do is to set the label to `NULL`
 * and add the labels manually in another GtkGrid or other container to keep
 * things aligned.
 *
 * The constructor is slightly convoluted, but flexible, see
 * #vice_gtk3_resource_browser_new
 * (todo: figure out how to get Doxygen to print the function prototype here)
 *
 * The first argument is required, it's the VICE resource we wish to change, for
 * example "Kernal".
 * The second argument is a list of strings representing file name globbing
 * patterns, for example <tt>{"*.bin", "*.rom", NULL}</tt>
 *
 *
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
#include <string.h>

#include "archdep.h"
#include "debug_gtk3.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "openfiledialog.h"
#include "savefiledialog.h"
#include "widgethelpers.h"
#include "resourcehelpers.h"
#include "resourceentry.h"
#include "ui.h"
#include "util.h"

#include "resourcebrowser.h"


/** \brief  Object keeping track of the state of the widget
 */
typedef struct rb_state_s {
    char  *res_name;            /**< resource name */
    char  *res_orig;            /**< resource value at widget creation */
    char **patterns;            /**< file matching patterns */
    char  *pattern_name;        /**< name to display for the file patterns */
    char  *title;               /**< title to display for the file browser */
    char  *directory;           /**< directory to use when the resource only
                                     contains a filename and not a path, or
                                     in the 'save' variant to use as default
                                     directory */

    /** optional user-defined callback */
    void (*callback)(GtkWidget*, gpointer);

    GtkWidget *entry;           /**< GtkEntry reference */
    GtkWidget *button;          /**< GtkButton reference */
} rb_state_t;


/** \brief  Allocate new state object and initialize all members to `NULL`.
 *
 * \return  new state object
 */
static rb_state_t *rb_state_new(void)
{
    rb_state_t *state   = lib_malloc(sizeof *state);
    state->res_name     = NULL;
    state->res_orig     = NULL;
    state->patterns     = NULL;
    state->pattern_name = NULL;
    state->title        = NULL;
    state->directory    = NULL;
    state->callback     = NULL;
    state->entry        = NULL;
    state->button       = NULL;
    return state;
}

/** \brief  Set/update directory in state object
 *
 * Frees the old directory string and sets it to \a directory.
 * Passing `NULL` or "" for directory will free the directory string.
 *
 * \param[in]   state       state object
 * \param[in]   directory   new directory to use
 */
static void rb_state_set_directory(rb_state_t *state, const char *directory)
{
    lib_free(state->directory);
    if (directory != NULL && *directory != '\0') {
        state->directory = lib_strdup(directory);
    } else {
        state->directory = NULL;
    }
}

/** \brief  Free memory used by file type patterns
 *
 * \param[in]   state   state object
 */
static void rb_state_free_patterns(rb_state_t *state)
{
    if (state->patterns != NULL) {
        size_t num;

        for (num = 0; state->patterns[num] != NULL; num++) {
            lib_free(state->patterns[num]);
        }
        lib_free(state->patterns);
        state->patterns = NULL;
    }
}

/** \brief  Set file type patterns
 *
 * Make a deep copy of \a patterns in \a state. Calling with \a patterns set
 * to `NULL` will free the patterns in \a state.
 *
 * \param[in]   state       state object
 * \param[in]   patterns    list of file type patterns
 */
static void rb_state_set_patterns(rb_state_t         *state,
                                  const char * const *patterns)
{
    rb_state_free_patterns(state);
    if (patterns != NULL) {
        size_t num;

        /* count number of pattern elements */
        for (num = 0; patterns[num] != NULL; num++) {
            /* NOP */
        }
        if (num == 0) {
            /* free patterns */
            rb_state_free_patterns(state);
            return;
        }
        /* deep copy patterns */
        state->patterns = lib_malloc((num + 1u) * sizeof *(state->patterns));
        for (num = 0; patterns[num] != NULL; num++) {
            state->patterns[num] = lib_strdup(patterns[num]);
        }
        state->patterns[num] = NULL;
    }
}

/** \brief  Free state object and all its members
 *
 * \param[in]   state   state object
 */
static void rb_state_free(rb_state_t *state)
{
    if (state != NULL) {
        lib_free(state->res_name);
        lib_free(state->res_orig);
        rb_state_free_patterns(state);
        lib_free(state->pattern_name);
        lib_free(state->title);
        lib_free(state->directory);
        lib_free(state);
    }
}

/** \brief  Clean up memory used by the main widget
 *
 * \param[in,out]   widget  resource browser widget
 * \param[in]       data    state object pointer
 */
static void on_resource_browser_destroy(GtkWidget *widget, gpointer data)
{
    rb_state_t *state = data;
    rb_state_free(state);
}

/** \brief  Callback for the dialog
 *
 * \param[in,out]   dialog      Open file dialog reference
 * \param[in]       filename    filename or NULL on cancel
 * \param[in,out]   data        browser state
 */
static void browse_filename_callback(GtkDialog *dialog,
                                     char *filename,
                                     gpointer data)
{
    rb_state_t *state = data;

    if (filename != NULL) {
        if (!vice_gtk3_resource_entry_set(state->entry, filename)){
            log_error(LOG_DEFAULT,
                    "failed to set resource %s to '%s', reverting\n",
                    state->res_name, filename);
            /* restore resource to original state */
            resources_set_string(state->res_name, state->res_orig);
            gtk_entry_set_text(GTK_ENTRY(state->entry), state->res_orig);
        } else {
            if (state->callback != NULL) {
                state->callback(GTK_WIDGET(dialog), (gpointer)filename);
            }
        }
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/** \brief  Callback for the save dialog
 *
 * \param[in,out]   dialog      Save file dialog reference
 * \param[in]       filename    filename or NULL on cancel
 * \param[in,out]   data        browser state
 */
static void save_filename_callback(GtkDialog *dialog,
                                     char *filename,
                                     gpointer data)
{
    rb_state_t *state = data;

    if (filename != NULL) {
        if (!vice_gtk3_resource_entry_set(state->entry, filename)){
            log_error(LOG_DEFAULT,
                    "failed to set resource %s to '%s', reverting\n",
                    state->res_name, filename);
            /* restore resource to original state */
            resources_set_string(state->res_name, state->res_orig);
            gtk_entry_set_text(GTK_ENTRY(state->entry), state->res_orig);
        } else {
            if (state->callback != NULL) {
                state->callback(GTK_WIDGET(dialog), (gpointer)filename);
            }
        }
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/** \brief  Handler for the "clicked" event of the browse button
 *
 * Shows a file open dialog to select a file.
 *
 * If the connected resource value contains a valid filename/path, the dialog's
 * directory is set to that file's directory. If only a filename is given, such
 * as the default KERNAL, $datadir/$machine_name is used for the directory.
 * In the dialog's directory listing the current file is selected.
 *
 * \param[in]   widget  browse button
 * \param[in]   data    state object pointer
 *
 */
static void on_resource_browser_browse_clicked(GtkWidget *widget, gpointer data)
{
    GtkWidget  *dialog;
    rb_state_t *state     = data;
    const char *res_value = NULL;

    /* get the filename/path in the resource */
    resources_get_string(state->res_name, &res_value);

    dialog = vice_gtk3_open_file_dialog(state->title,
                                        state->pattern_name,
                                        (const char **)(state->patterns),
                                        NULL,
                                        browse_filename_callback,
                                        state);

    /* set browser directory to directory in resource if available */
    if (res_value != NULL) {
        /* get dirname and basename */
        gchar *dirname  = g_path_get_dirname(res_value);
        gchar *basename = g_path_get_basename(res_value);

        /* if no path is present in the resource value, set the directory to
         * state->directory, if present and the file to the current filename */
        if (g_strcmp0(dirname, ".") == 0 && state->directory != NULL) {
            char *fullpath = util_join_paths(state->directory, basename, NULL);

            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), fullpath);
            lib_free(fullpath);
        } else {
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), res_value);
        }

        /* clean up */
        g_free(dirname);
        g_free(basename);
    }
    gtk_widget_show_all(dialog);
}

/** \brief  Handler for the "clicked" event of the save button
 *
 * \param[in]   widget  save button
 * \param[in]   data    state object pointer
 */
static void on_resource_browser_save_clicked(GtkWidget *widget, gpointer data)
{
    GtkWidget  *dialog;
    rb_state_t *state     = data;
    const char *res_value = NULL;

    /* get the filename/path in the resource */
    resources_get_string(state->res_name, &res_value);

    dialog = vice_gtk3_save_file_dialog(state->title,
                                        NULL,
                                        FALSE,
                                        NULL,
                                        save_filename_callback,
                                        state);

    if (res_value != NULL && *res_value != '\0') {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), res_value);
    } else {
        if (state->directory != NULL) {
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                                state->directory);
        }
    }
    gtk_widget_show_all(dialog);
}


/** \brief  Create file selection widget with browse button
 *
 * \param[in]   resource        resource name
 * \param[in]   patterns        file match patterns (optional)
 * \param[in]   pattern_name    name to use for \a patterns in the file dialog
 *                              (optional)
 * \param[in]   browser_title   title to display in the file dialog (optional,
 *                              defaults to "Select file")
 * \param[in]   label           label (optional)
 * \param[in]   callback        user callback (optional, not implemented yet)
 *
 * \note    both \a patterns and \a pattern_name need to be defined (ie not
 *          `NULL` for the \a patterns to work.
 *
 * \return  GtkGrid
 */
GtkWidget *vice_gtk3_resource_browser_new(const char         *resource,
                                          const char * const *patterns,
                                          const char         *pattern_name,
                                          const char         *title,
                                          const char         *label,
                                          void              (*callback)(GtkWidget*, gpointer))
{
    GtkWidget  *grid;
    GtkWidget  *lbl = NULL;
    rb_state_t *state;
    const char *orig;
    int         column = 0;

    grid = vice_gtk3_grid_new_spaced(8, 0);

    /* allocate and init state object */
    state = rb_state_new();

    /* copy resource name */
    state->res_name = lib_strdup(resource);
    resource_widget_set_resource_name(grid, resource);

    /* get current value of resource */
    if (resources_get_string(resource, &orig) < 0) {
        orig = "";
    } else if (orig == NULL) {
        orig = "";
    }
    state->res_orig = lib_strdup(orig);

    /* store optional callback */
    state->callback = callback;

    /* copy file matching patterns */
    rb_state_set_patterns(state, patterns);

    /* copy pattern name */
    if (pattern_name != NULL && *pattern_name != '\0') {
        state->pattern_name = lib_strdup(pattern_name);
    }

    /* copy browser title */
    if (title != NULL && *title != '\0') {
        state->title = lib_strdup("Select file");
    } else {
        state->title = lib_strdup(title);
    }

    /*
     * add widgets to the grid
     */

    /* label, if given */
    if (label != NULL && *label != '\0') {
        lbl = gtk_label_new(label);
        gtk_widget_set_halign(lbl, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), lbl, 0, 0, 1, 1);
        column++;
    }

    /* text entry */
    state->entry = vice_gtk3_resource_entry_new(resource);
    gtk_widget_set_hexpand(state->entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), state->entry, column, 0, 1, 1);
    column++;

    /* browse button */
    state->button = gtk_button_new_with_label("Browse ...");
    gtk_grid_attach(GTK_GRID(grid), state->button, column, 0, 1,1);

    /* store the state object in the widget */
    g_object_set_data(G_OBJECT(grid), "ViceState", (gpointer)state);

    /* connect signal handlers */
    g_signal_connect(state->button,
                     "clicked",
                     G_CALLBACK(on_resource_browser_browse_clicked),
                     state);
    g_signal_connect_unlocked(grid,
                              "destroy",
                              G_CALLBACK(on_resource_browser_destroy),
                              state);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set \a widget value to \a new
 *
 * \param[in,out]   widget  resource browser widget
 * \param[in]       new     new value for \a widget
 *
 * \return  bool
 */
gboolean vice_gtk3_resource_browser_set(GtkWidget *widget, const char *new)
{
    rb_state_t *state;
    state = g_object_get_data(G_OBJECT(widget), "ViceState");

    if (resources_set_string(state->res_name, new) < 0) {
        /* restore to default */
        resources_set_string(state->res_name, state->res_orig);
        gtk_entry_set_text(GTK_ENTRY(state->entry), state->res_orig);
        return FALSE;
    } else {
        gtk_entry_set_text(GTK_ENTRY(state->entry), new != NULL ? new : "");
        return TRUE;
    }
}


/** \brief  Get the current value of \a widget
 *
 * Get the current resource value of \a widget and store it in \a dest. If
 * getting the resource value fails for some reason, `FALSE` is returned and
 * \a dest is set to `NULL`.
 *
 * \param[in]   widget  resource browser widget
 * \param[out]  dest    destination of value
 *
 * \return  TRUE when the resource value was retrieved
 */
gboolean vice_gtk3_resource_browser_get(GtkWidget *widget, const char **dest)
{
    rb_state_t *state;
    state = g_object_get_data(G_OBJECT(widget), "ViceState");

    if (resources_get_string(state->res_name, dest) < 0) {
        *dest = NULL;
        return FALSE;
    }
    return TRUE;
}


/** \brief  Restore resource in \a widget to its original value
 *
 * \param[in,out]   widget  resource browser widget
 *
 * \return  bool
 */
gboolean vice_gtk3_resource_browser_reset(GtkWidget *widget)
{
    rb_state_t *state;

    state = g_object_get_data(G_OBJECT(widget), "ViceState");

    /* restore resource value */
    if (resources_set_string(state->res_name, state->res_orig) < 0) {
        return FALSE;
    }
    /* update text entry */
    gtk_entry_set_text(GTK_ENTRY(state->entry), state->res_orig);
    return TRUE;
}


/** \brief  Synchronize widget with current resource value
 *
 * Only needed to call if the resource's value is changed from code other than
 * this widget's code.
 *
 * \param[in,out]   widget  resource browser widget
 *
 * \return  TRUE if the resource value was retrieved
 */
gboolean vice_gtk3_resource_browser_sync(GtkWidget *widget)
{
    rb_state_t *state;
    const char *value;

    /* get current resource value */
    state = g_object_get_data(G_OBJECT(widget), "ViceState");
    if (resources_get_string(state->res_name, &value) < 0) {
        return FALSE;
    }

    gtk_entry_set_text(GTK_ENTRY(state->entry), value);
    return TRUE;
}


/** \brief  Reset widget to the resource's factory value
 *
 * \param[in,out]   widget  resource browser widget
 *
 * \return  bool
 */
gboolean vice_gtk3_resource_browser_factory(GtkWidget *widget)
{
    rb_state_t *state;
    const char *value;

    /* get resource factory value */
    state = g_object_get_data(G_OBJECT(widget), "ViceState");
    if (resources_get_default_value(state->res_name, &value) < 0) {
        return FALSE;
    }
    return vice_gtk3_resource_browser_set(widget, value);
}


/** \brief  Resource browser widget to select a file to save
 *
 * \param[in]   resource    resource name
 * \param[in]   title       dialog title
 * \param[in]   label       optional label before the text entry
 * \param[in]   suggested   suggested filename (unimplemented)
 * \param[in]   callback    callback
 *
 * \return  GtkGrid
 */
GtkWidget *vice_gtk3_resource_browser_save_new(const char *resource,
                                               const char *title,
                                               const char *label,
                                               const char *suggested,
                                               void (*callback)(GtkWidget*, gpointer))
{
    GtkWidget  *grid;
    GtkWidget  *lbl;
    rb_state_t *state;
    const char *orig = NULL;
    int         column = 0;

    grid = vice_gtk3_grid_new_spaced(8, 0);

    /* alloc and init state object */
    state = rb_state_new();
    state->res_name = lib_strdup(resource);
    resource_widget_set_resource_name(grid, resource);

    /* get current value, if any */
    if (resources_get_string(resource, &orig) < 0) {
        orig = "";
    } else if (orig == NULL) {
        orig = "";
    }
    state->res_orig = lib_strdup(orig);

    /* copy browser title */
    if (title == NULL || *title == '\0') {
        state->title = lib_strdup("Select file");
    } else {
        state->title = lib_strdup(title);
    }

    /* Add widgets */

    /* label */
    if (label != NULL) {
        lbl = gtk_label_new(label);
        gtk_widget_set_halign(lbl, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), lbl, 0, 0, 1, 1);
        column++;
    }

    /* text entry */
    state->entry = vice_gtk3_resource_entry_new(resource);
    gtk_widget_set_hexpand(state->entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), state->entry, column, 0, 1, 1);
    column++;

    /* browse button */
    state->button = gtk_button_new_with_label("Browse ...");
    gtk_grid_attach(GTK_GRID(grid), state->button, column, 0, 1,1);

    /* store the state object in the widget */
    g_object_set_data(G_OBJECT(grid), "ViceState", (gpointer)state);

    /* connect signal handlers */
    g_signal_connect(state->button,
                     "clicked",
                     G_CALLBACK(on_resource_browser_save_clicked),
                     state);
    g_signal_connect_unlocked(grid,
                              "destroy",
                              G_CALLBACK(on_resource_browser_destroy),
                              state);
    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set the directory to use when the resource only contains a filename
 *
 * Set directory to append to a filename with directory component, or to use
 * as the default directory for the 'save' variant of the resource browser.
 *
 * \param[in,out]   widget  resource browser widget
 * \param[in]       path    directory to use
 */
void vice_gtk3_resource_browser_set_directory(GtkWidget  *widget,
                                              const char *directory)
{
    rb_state_t *state;

    state = g_object_get_data(G_OBJECT(widget), "ViceState");
    rb_state_set_directory(state, directory);
}
