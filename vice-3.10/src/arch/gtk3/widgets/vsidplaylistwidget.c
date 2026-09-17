/** \file   vsidplaylistwidget.c
 * \brief   GTK3 playlist widget for VSID
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * Icons used by this file:
 *
 * $VICEICON    actions/media-skip-backward
 * $VICEICON    actions/media-seek-backward
 * $VICEICON    actions/media-seek-forward
 * $VICEICON    actions/media-skip-forward
 * $VICEICON    status/media-playlist-repeat
 * $VICEICON    status/media-playlist-shuffle
 * $VICEICON    actions/list-add
 * $VICEICON    actions/list-remove
 * $VICEICON    actions/document-open
 * $VICEICON    actions/document-save
 * $VICEICON    actions/edit-clear-all
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
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#include "archdep_get_hvsc_dir.h"
#include "hvsc.h"
#include "lastdir.h"
#include "lib.h"
#include "mainlock.h"
#include "m3u.h"
#include "resources.h"
#include "uiactions.h"
#include "uiapi.h"
#include "uivsidwindow.h"
#include "util.h"
#include "version.h"
#ifdef USE_SVN_REVISION
#include "svnversion.h"
#endif
#include "vice_gtk3.h"
#include "vsidplaylistadddialog.h"
#include "vsidtuneinfowidget.h"

#include "vsidplaylistwidget.h"


/* \brief  Control button types */
enum {
    CTRL_LIST_END = -1, /**< end of list */
    CTRL_PUSH_BUTTON,   /**< action button: simple push button */
    CTRL_TOGGLE_BUTTON, /**< toggle button: for 'repeat' and 'shuffle' */
    CTRL_CENTERED_LABEL /**< centered label, gtk_label_set_markup() is used
                             for the text, so some HTML tags such as bold and
                             italic can be used */
};

/* Playlist column indexes */
enum {
    COL_TITLE,          /**< title */
    COL_AUTHOR,         /**< author */
    COL_FULL_PATH,      /**< full path to psid file */
    COL_DISPLAY_PATH,   /**< displayed path (HVSC stripped if possible) */

    NUM_COLUMNS         /**< number of columns in the model */
};

/** \brief  Playlist control struct
 */
typedef struct plist_ctrl_ {
    const char *icon;           /**< icon name in the Gtk3 theme */
    int         type;           /**< type of button (action/toggle) */
    int         action;         /**< UI action ID */
    void        (*callback)(GtkWidget *, gpointer); /**< callback function */
    const char *text;           /**< text (for labels) */
    const char *tooltip;        /**< tooltip text */
    int         margin_start;    /**< left margin */
    int         margin_end;   /**< right margin */
} plist_ctrl_t;

/** \brief  Type of context menu item types
 */
typedef enum {
    CTX_MENU_ACTION,    /**< action */
    CTX_MENU_SEP        /**< menu separator */
} ctx_menu_item_type_t;

/** \brief  Context menu item object
 */
typedef struct ctx_menu_item_s {
    const char *            text;   /**< displayed text */
    /**< item callback */
    gboolean                (*callback)(GtkWidget *, gpointer);
    ctx_menu_item_type_t    type;   /**< menu item type, \see ctx_menu_item_type_t */
} ctx_menu_item_t;

/** \brief  VSID Hotkey info object
 */
typedef struct vsid_hotkey_s {
    guint keyval;                                   /**< GDK key value */
    guint modifiers;                                /**< GDK modifiers */
    gboolean (*callback)(GtkWidget *, gpointer);    /**< hotkey callback */
} vsid_hotkey_t;


/*
 * Forward declarations
 */
static void update_title(void);
static void update_current_and_total(void);


/** \brief  Reference to the playlist model
 */
static GtkListStore *playlist_model;

/** \brief  Reference to the playlist view
 */
static GtkWidget *playlist_view;

/** \brief  Playlist title widget
 *
 * Gets updated with the number of tunes in the list.
 */
static GtkWidget *title_widget;

/** \brief  Reference to the save-playlist dialog's entry to set playlist title
 *
 * Only valid during lifetime of the save-playlist dialog
 */
static GtkWidget *playlist_title_entry;

/** \brief  Playlist title
 *
 * Set when loading an m3u file with the "\#PLAYLIST:" directive.
 * Freed when the main widget is destroyed.
 */
static char *playlist_title;

/** \brief  Playlist path
 *
 * Used to suggest filename when saving playlist, set when loading a playlist.
 */
static char *playlist_path;

/** \brief  Playlist dialogs last-used directory
 *
 * Last used directory for playlist dialogs, used to set the directory of the
 * playlist dialogs.
 */
static char *playlist_last_dir;

#define MAX_CONTROLS 32
static GtkWidget *control_widgets[MAX_CONTROLS];
static gulong control_handlers[MAX_CONTROLS];


/* {{{ Utility functions */

/** \brief  Strip HVSC base dir from \a path
 *
 * Try to strip the HVSC base directory from \a path, otherwise return the
 * full \a path.
 *
 * \param[in]   path    full path to psid file
 *
 * \return  stripped path
 * \note    Free with g_free()
 */
static gchar *strip_hvsc_base(const gchar *path)
{
    const char *hvsc_base = archdep_get_hvsc_dir();

    if (hvsc_base != NULL && *hvsc_base != '\0' &&
            g_str_has_prefix(path, hvsc_base)) {
        /* skip base */
        path += strlen(hvsc_base);
        /* skip directory separator if present */
        if (*path == '/' || *path == '\\') {
            path++;
        }
    }
    return g_strdup(path);
}

/** \brief  Free playlist title
 */
static void vsid_playlist_free_title(void)
{
    if (playlist_title != NULL) {
        lib_free(playlist_title);
    }
    playlist_title = NULL;
}

/** \brief  Set playlist title
 *
 * \param[in]   title   playlist title
 */
static void vsid_playlist_set_title(const char *title)
{
    vsid_playlist_free_title();
    playlist_title = lib_strdup(title);
}

/** \brief  Get playlist title
 *
 * \return  playlist title
 */
static const char *vsid_playlist_get_title(void)
{
    return playlist_title;
}

/** \brief  Free playlist file path
 */
static void vsid_playlist_free_path(void)
{
    if (playlist_path != NULL) {
        lib_free(playlist_path);
    }
    playlist_path = NULL;
}

/** \brief  Set playlist path
 *
 * \param[in]   path    path to playlist file
 */
static void vsid_playlist_set_path(const char *path)
{
    vsid_playlist_free_path();
    playlist_path = lib_strdup(path);
}

/** \brief  Get playlist file path
 *
 * \return  playlist file path
 */
static const char *vsid_playlist_get_path(void)
{
    return playlist_path;
}

/** \brief  Set default directory of playlist load/save dialogs
 *
 * If the default directory (#playlist_last_dir) isn't set yet, we use the
 * HVSC base directory.
 */
static void set_playlist_dialogs_default_dir(void)
{
    if (playlist_last_dir == NULL) {
        const gchar *base = archdep_get_hvsc_dir();
        if (base != NULL) {
            /* the lastdir.c code uses GLib for memory management, so we use
             * g_strdup() here: */
            playlist_last_dir = g_strdup(base);
        }
    }
}

/** \brief  Scroll view so the selected row is visible
 *
 * \param[in]   iter    tree view iter
 *
 * \note    This function assumes the iter is valid since there's no quick
 *          way to check if the iter is valid
 */
static void scroll_to_iter(GtkTreeIter *iter)
{
    GtkTreePath *path;

    path = gtk_tree_model_get_path(GTK_TREE_MODEL(playlist_model), iter);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(playlist_view),
                                 path,
                                 NULL,      /* no column since we provide path */
                                 FALSE,     /* don't align, just do the minimum */
                                 0.0, 0.0); /* alignments, ignored */
    gtk_tree_path_free(path);
}

/* }}} */


/** \brief  Add SID files to the playlist
 *
 * \param[in,out]   files   List of selected files
 *
 * \note    The playlist-add dialog frees \a files after calling this function
 */
static void add_files_callback(GSList *files)
{
    GSList *pos = files;

    if (files != NULL) {
        do {
            const char *path = (const char *)(pos->data);
            vsid_playlist_append_file(path);
            pos = g_slist_next(pos);
        } while (pos != NULL);
    }
}

/* {{{ Context menu callbacks */

/** \brief  Delete selected rows
 *
 * Event handler for context menu 'delete-all' and hotkey 'Shift+Delete'.
 *
 * \param[in]   widget  widget triggering the event
 * \param[in]   data    extra event data (unused)
 *
 * \return  FALSE on error
 */
static gboolean on_ctx_delete_selected(GtkWidget *widget, gpointer data)
{
    vsid_playlist_remove_selection();
    return TRUE;
}

/* }}} */

/** \brief  Delete the entire playlist
 *
 * \param[in]   widget  widget triggering the callback (unused)
 * \param[in]   data    event reference (unused)
 *
 * \return  TRUE
 */
static gboolean on_ctx_delete_all(GtkWidget *widget, gpointer data)
{
    vsid_playlist_clear();
    return TRUE;
}

/** \brief  Callback for the Insert hotkey
 *
 * \param[in]   widget  widget triggering the callback (unused)
 * \param[in]   data    event reference (unused)
 *
 * \return  TRUE
 */
static gboolean open_add_dialog(GtkWidget *widget, gpointer data)
{
    vsid_playlist_add_dialog_exec(add_files_callback);
    return TRUE;
}


/* {{{ Playlist loading */
/** \brief  M3U entry handler
 *
 * Called by the m3u parser when encountering a normal entry.
 *
 * \param[in]   text    entry text
 * \param[in]   len     length of \a text
 *
 * \return  `false` to stop the parser on an error
 */
static bool playlist_entry_handler(const char *text, size_t len)
{
    /* ignore errors for now */
    vsid_playlist_append_file(util_skip_whitespace(text));
    return true;
}

/** \brief  M3U directive handler
 *
 * Called by the m3u parser when encountering an extended m3u directive.
 *
 * \param[in]   id      directive ID
 * \param[in]   text    text following the directive
 * \param[in]   len     length of \a text
 *
 * \return  `false` to stop the parser on an error
 */
static bool playlist_directive_handler(m3u_ext_id_t id, const char *text, size_t len)
{
    const char *title;

    switch (id) {
        case M3U_EXTM3U:
            debug_gtk3("HEADER: Valid M3Uext 1.1 file");
            break;
        case M3U_PLAYLIST:
            /* make copy of title, the text pointer is invalidated on the
             * next line the parser reads */
            title = util_skip_whitespace(text);
            /* only set title when not empty and not previously set */
            if (*title != '\0' && playlist_title == NULL) {
                vsid_playlist_set_title(title);
            }
            break;
        default:
            break;
    }
    return true;
}

/** \brief  Callback for the load-playlist dialog
 *
 * \param[in]   dialog      load-playlist dialog
 * \param[in]   filename    filename or `NULL` when canceled
 * \param[in]   data        extra callback data (ignored)
 */
static void playlist_load_callback(GtkDialog *dialog,
                                   gchar *filename,
                                   gpointer data)
{
    if (filename != NULL) {
        char buf[1024];

        lastdir_update(GTK_WIDGET(dialog), &playlist_last_dir, NULL);

        g_snprintf(buf, sizeof buf, "Loading playlist %s", filename);
        ui_display_statustext(buf, true);

        if (m3u_open(filename, playlist_entry_handler, playlist_directive_handler)) {
            /* clear playlist now */
            gtk_list_store_clear(playlist_model);
            /* clear title and path */
            vsid_playlist_free_title();
            vsid_playlist_free_path();

            /* run the parser to populate the playlist */
            if (!m3u_parse()) {
                g_snprintf(buf, sizeof buf, "Error parsing %s.", filename);
                ui_display_statustext(buf, false);
            }
            /* remember path for the playlist-save dialog */
            vsid_playlist_set_path(filename);
            m3u_close();
            update_current_and_total();
        }
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    ui_action_finish(ACTION_PSID_PLAYLIST_LOAD);
}
/* }}} */


/* {{{ Playlist saving */
/** \brief  Create content area widget for the 'save-playlist dialog
 *
 * Create GtkGrid with label and text entry to set/edit the playlist title.
 *
 * \return  GtkGrid
 */
static GtkWidget *create_save_content_area(void)
{
    GtkWidget *grid;
    GtkWidget *label;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_widget_set_margin_top(grid, 8);
    gtk_widget_set_margin_start(grid, 16);
    gtk_widget_set_margin_end(grid, 16);
    gtk_widget_set_margin_bottom(grid, 8);

    label = gtk_label_new_with_mnemonic("Playlist _title:");
    playlist_title_entry = gtk_entry_new();
    gtk_widget_set_hexpand(label, FALSE);
    gtk_widget_set_hexpand(playlist_title_entry, TRUE);
    gtk_entry_set_text(GTK_ENTRY(playlist_title_entry), vsid_playlist_get_title());

    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), playlist_title_entry, 1, 0, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Callback for the 'save-playlist' dialog
 *
 * Save the current playlist to \a filename.
 *
 * \param[in]   dialog      save-playlist dialog
 * \param[in]   filename    filename or `NULL` to cancel saving
 * \param[in]   data        extra event data (ignored)
 */
static void playlist_save_dialog_callback(GtkDialog *dialog,
                                          gchar     *filename,
                                          gpointer   data)
{
    if (filename != NULL) {
        GtkTreeIter iter;
        char buf[256];
        time_t t;
        const struct tm *tinfo;
        const char *title;
        char *filename_ext;

        lastdir_update(GTK_WIDGET(dialog), &playlist_last_dir, NULL);

        /* add .m3u extension if missing */
        filename_ext = util_add_extension_const(filename, "m3u");
        g_free(filename);

        /* update playlist title */
        title = gtk_entry_get_text(GTK_ENTRY(playlist_title_entry));
        if (title == NULL || *title == '\0') {
            vsid_playlist_free_title();
        } else {
            vsid_playlist_set_title(title);
            update_title();
        }

        /* try to open playlist file for writing */
        if (!m3u_create(filename_ext)) {
            vice_gtk3_message_error(GTK_WINDOW(dialog),
                                    "VICE error",
                                    "Failed to open '%s' for writing.",
                                    filename_ext);
            lib_free(filename_ext);
            gtk_widget_destroy(GTK_WIDGET(dialog));
            return;
        }
        /* m3u code makes a copy of the path, so we can clean up here */
        lib_free(filename_ext);

        /* add empty line */
        if (!m3u_append_newline()) {
            goto save_error;
        }

        /* add timestamp */
        t = time(NULL);
        tinfo = localtime(&t);
        if (tinfo != NULL) {
            strftime(buf, sizeof buf, "Generated on %Y-%m-%dT%H:%M%z", tinfo);
            if (!m3u_append_comment(buf)) {
                goto save_error;
            }
        }

        /* add VICE version */
#ifdef USE_SVN_REVISION
        g_snprintf(buf, sizeof buf,
                   "Generated by VICE (Gtk) %s r%s",
                   VERSION, VICE_SVN_REV_STRING);
#else
        g_snprintf(buf, sizeof buf, "Generated by VICE (Gtk) %s", VERSION);
#endif
        if (!m3u_append_comment(buf)) {
            goto save_error;
        }

        /* add empty line */
        if (!m3u_append_newline()) {
            goto save_error;
        }

        /* add playlist title, if set */
        if (title != NULL && *title != '\0') {
            if (!m3u_set_playlist_title(title)) {
                goto save_error;
            }
        }

        /* add empty line */
        if (!m3u_append_newline()) {
            goto save_error;
        }

        /* finally! iterate playlist items, writing SID file entries */
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playlist_model), &iter)) {
            do {
                const char *fullpath;
                GValue value = G_VALUE_INIT;

                gtk_tree_model_get_value(GTK_TREE_MODEL(playlist_model),
                                         &iter,
                                         COL_FULL_PATH,
                                         &value);
                fullpath = g_value_get_string(&value);
                if (!m3u_append_entry(fullpath)) {
                    goto save_error;
                }
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playlist_model),
                                              &iter));
        }
        m3u_close();
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    ui_action_finish(ACTION_PSID_PLAYLIST_SAVE);
    return;

save_error:
    vice_gtk3_message_error(GTK_WINDOW(dialog),
                            "VICE error",
                            "I/O error while writing playlist.");
    m3u_close();
    gtk_widget_destroy(GTK_WIDGET(dialog));
    ui_action_finish(ACTION_PSID_PLAYLIST_SAVE);
}
/* }}} */


/** \brief  Update title widget with the playlist title, if present
 */
static void update_title(void)
{
    if (playlist_title == NULL) {
        gtk_label_set_markup(GTK_LABEL(title_widget), "<b>Playlist:</b>");
    } else {
        char buffer[1024];
        g_snprintf(buffer, sizeof buffer,
                   "<b>Playlist: \"%s\"</b>",
                   vsid_playlist_get_title());
        gtk_label_set_markup(GTK_LABEL(title_widget), buffer);
    }
}

/** \brief  Update playlist 'current/total' display
 *
 * \todo    Add way to determine currently playing SID when multiple rows
 *          have been selected: probably we'll need to keep track of the last
 *          selected row in the playlist, also updating when pressing the
 *          next/prev etc buttons, or autoskipping to the next SID.
 */
static void update_current_and_total(void)
{
    GtkWidget *widget = control_widgets[2]; /* FIXME! */

    if (widget != NULL) {
        gint row;
        gint total;
        gchar buffer[256];

        row = vsid_playlist_get_current_row(NULL);
        total = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playlist_model),
                                               NULL);
        if (row < 0) {
            g_snprintf(buffer, sizeof buffer,
                       "<b><tt> ?</tt></b> / <b><tt>%2d</tt></b>",
                       total);
        } else {
            g_snprintf(buffer, sizeof buffer,
                       "<b><tt>%2d</tt></b> / <b><tt>%2d</tt></b>",
                       row + 1, total);
        }
        gtk_label_set_markup(GTK_LABEL(widget), buffer);
    }
}


/*
 * Event handlers
 */

/** \brief  Event handler for the 'destroy' event of the playlist widget
 *
 * \param[in]   widget  playlist widget
 * \param[in]   data    extra event data (unused)
 */
static void on_destroy(GtkWidget *widget, gpointer data)
{
    vsid_playlist_add_dialog_free();
    vsid_playlist_free_title();
    vsid_playlist_free_path();
    lastdir_shutdown(&playlist_last_dir, NULL);
}

/** \brief  Event handler for the 'row-activated' event of the view
 *
 * Triggered by double-clicking on a SID file in the view
 *
 * \param[in,out]   view    the GtkTreeView instance
 * \param[in]       path    the path to the activated row
 * \param[in]       column  the column in the \a view (unused)
 * \param[in]       data    extra event data (unused)
 */
static void on_row_activated(GtkTreeView *view,
                             GtkTreePath *path,
                             GtkTreeViewColumn *column,
                             gpointer data)
{
    GtkTreeIter iter;
    const gchar *filename;
    GValue value = G_VALUE_INIT;

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_model), &iter, path)) {
        debug_gtk3("error: failed to get tree iter.");
        return;
    }

    gtk_tree_model_get_value(GTK_TREE_MODEL(playlist_model),
                             &iter,
                             COL_FULL_PATH,
                             &value);
    filename = g_value_get_string(&value);

    if (ui_vsid_window_load_psid(filename) < 0) {
        /* looks like adding files to the playlist already checks the files
         * being added, so this may not be neccesarry */
        char msg[1024];

        g_snprintf(msg, sizeof(msg), "'%s' is not a valid PSID file", filename);
        ui_display_statustext(msg, true);
    }
    update_current_and_total();

    g_value_unset(&value);
}

/** \brief  Event handler for the 'remove' button
 *
 * Remove selected items from the playlist.
 *
 * \param[in]   widget  button triggering the event
 * \param[in]   data    extra event data (unused)
 */
static void on_btn_playlist_remove_clicked(GtkWidget *widget, gpointer data)
{
    vsid_playlist_remove_selection();
}

/** \brief  Callback for the clear-playlist confirmation dialog
 *
 * Destroys \a dialog and clears playlist if \a result is `TRUE`.
 *
 * \param[in]   dialog  confirmation dialog
 * \param[in]   result  result of dialog
 */
static void clear_playlist_callback(GtkDialog *dialog, gboolean result)
{
    if (result) {
        vsid_playlist_clear();
        update_current_and_total();
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    ui_action_finish(ACTION_PSID_PLAYLIST_CLEAR);
}


/** \brief  Playlist context menu items
 */
static const ctx_menu_item_t cmenu_items[] = {
    { "Play",
      NULL,
      CTX_MENU_ACTION },
    { "Delete selected item(s)",
      on_ctx_delete_selected,
      CTX_MENU_ACTION },

    { "---", NULL, CTX_MENU_SEP },

    { "Load playlist",
      NULL,
      CTX_MENU_ACTION },
    { "Save playlist",
      on_ctx_delete_all,
      CTX_MENU_ACTION },
    { "Clear playlist",
      NULL,
      CTX_MENU_ACTION },

    { "---", NULL, CTX_MENU_SEP },


    { "Export binary",
      NULL,
      CTX_MENU_ACTION },
    { NULL, NULL, -1 }
};


/** \brief  Create context menu for the playlist
 *
 * \return  GtkMenu
 */
static GtkWidget *create_context_menu(void)
{
    GtkWidget *menu;
    GtkWidget *item;
    int i;

    menu = gtk_menu_new();

    for (i = 0; cmenu_items[i].text != NULL; i++) {

        if (cmenu_items[i].type == CTX_MENU_SEP) {
            item = gtk_separator_menu_item_new();
        } else {
            item = gtk_menu_item_new_with_label(cmenu_items[i].text);
            if (cmenu_items[i].callback != NULL) {
                g_signal_connect(
                        item,
                        "activate",
                        G_CALLBACK(cmenu_items[i].callback),
                        GINT_TO_POINTER(i));
            } else {
                gtk_widget_set_sensitive(item, FALSE);
            }
        }
        gtk_container_add(GTK_CONTAINER(menu), item);

    }
    gtk_widget_show_all(menu);
    return menu;
}

/** \brief  Event handler for button press events on the playlist
 *
 * Pops up a context menu on the playlist.
 *
 * \param[in]   view    playlist view widget (unused)
 * \param[in]   event   event reference
 * \param[in]   data    extra even data (unused)
 *
 * \return  TRUE when event consumed and no further propagation is needed
 */
static gboolean on_button_press_event(GtkWidget *view,
                                      GdkEvent *event,
                                      gpointer data)
{

    if (((GdkEventButton *)event)->button == GDK_BUTTON_SECONDARY) {
        gtk_menu_popup_at_pointer(GTK_MENU(create_context_menu()), event);
        return TRUE;
    }
    return FALSE;
}


/** \brief  Playlist hotkeys
 */
static const vsid_hotkey_t hotkeys[] = {
    { GDK_KEY_Insert,   0,              open_add_dialog },
    { GDK_KEY_Delete,   0,              on_ctx_delete_selected },
    { GDK_KEY_Delete,   GDK_SHIFT_MASK, on_ctx_delete_all },
    { 0, 0, NULL }
};


/** \brief  Event handler for key press events on the playlist
 *
 * \param[in,out]   view    playlist view widget
 * \param[in]       event   event reference
 * \param[in]       data    extra even data
 *
 * \return  TRUE when event consumed and no further propagation is needed
 */
static gboolean on_key_press_event(GtkWidget *view,
                                   GdkEvent *event,
                                   gpointer data)
{
    GdkEventType type = ((GdkEventKey *)event)->type;

    if (type == GDK_KEY_PRESS) {
        guint keyval = ((GdkEventKey *)event)->keyval;  /* key ID */
        guint modifiers = ((GdkEventKey *)event)->state;    /* modifiers */
        int i;

        for (i = 0; hotkeys[i].keyval != 0; i++) {
            if (hotkeys[i].keyval == keyval
                    && hotkeys[i].modifiers == modifiers
                    && hotkeys[i].callback != NULL) {
                return hotkeys[i].callback(view, event);
            }
        }
    }
    return FALSE;
}

/** \brief  Create playlist model
 */
static void vsid_playlist_model_create(void)
{
    playlist_model = gtk_list_store_new(NUM_COLUMNS,
                                        G_TYPE_STRING,      /* title */
                                        G_TYPE_STRING,      /* author */
                                        G_TYPE_STRING,      /* full path */
                                        G_TYPE_STRING);     /* stripped path */
}

/** \brief  Create playlist view widget
 *
 */
static void vsid_playlist_view_create(void)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;

    playlist_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(playlist_model));

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
            "Title",
            renderer,
            "text", COL_TITLE,
            NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
    gtk_tree_view_append_column(GTK_TREE_VIEW(playlist_view), column);

    column = gtk_tree_view_column_new_with_attributes(
            "Author",
            renderer,
            "text", COL_AUTHOR,
            NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
    gtk_tree_view_append_column(GTK_TREE_VIEW(playlist_view), column);

    column = gtk_tree_view_column_new_with_attributes(
            "Path",
            renderer,
            "text", COL_DISPLAY_PATH,
            NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(playlist_view), column);

    /* Allow selecting multiple items (for deletion) */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

    /*
     * Set event handlers
     */

    /* Enter/double-click */
    g_signal_connect(playlist_view,
                     "row-activated",
                     G_CALLBACK(on_row_activated),
                     NULL);
    /* context menu (right-click, or left-click) */
    g_signal_connect(playlist_view,
                     "button-press-event",
                     G_CALLBACK(on_button_press_event),
                     playlist_view);
    /* special keys (Del for now) */
    g_signal_connect(playlist_view,
                     "key-press-event",
                     G_CALLBACK(on_key_press_event),
                     playlist_view);
}


/** \brief  List of playlist controls
 */
static const plist_ctrl_t controls[] = {
    {
        .icon = "media-skip-backward",
        .type = CTRL_PUSH_BUTTON,
        .action = ACTION_PSID_PLAYLIST_FIRST,
        .tooltip = "Go to start of playlist"
    },
    {
        .icon = "media-seek-backward",
        .type = CTRL_PUSH_BUTTON,
        .action = ACTION_PSID_PLAYLIST_PREVIOUS,
        .tooltip = "Go to previous tune"
    },
    {
        .type = CTRL_CENTERED_LABEL,
        .text = "<b>0</b> of <b>0</b>",
        .margin_start = 8,
        .margin_end = 8
    },
    {
        .icon = "media-seek-forward",
        .type = CTRL_PUSH_BUTTON,
        .action = ACTION_PSID_PLAYLIST_NEXT,
        .tooltip = "Go to next tune"
    },
    {
        .icon = "media-skip-forward",
        .type = CTRL_PUSH_BUTTON,
        .action = ACTION_PSID_PLAYLIST_LAST,
        .tooltip = "Go to end of playlist"
    },
    {
        .icon = "media-playlist-repeat",
        .type = CTRL_TOGGLE_BUTTON,
        .tooltip = "Repeat playlist",
        .margin_start = 16
    },
    {
        .icon = "media-playlist-shuffle",
        .type = CTRL_TOGGLE_BUTTON,
        .tooltip = "Shuffle playlist"
    },
    {
        .icon ="list-add",
        .type = CTRL_PUSH_BUTTON,
        .action = ACTION_PSID_PLAYLIST_ADD,
        .tooltip = "Add tunes to playlist",
        .margin_start = 16
    },
    {
        .icon = "list-remove",
        .type = CTRL_PUSH_BUTTON,
        .callback = on_btn_playlist_remove_clicked,
        .tooltip = "Remove selected tune from playlist"
    },
    {
        .icon = "edit-clear-all",
        .type = CTRL_PUSH_BUTTON,
        .action = ACTION_PSID_PLAYLIST_CLEAR,
        .tooltip = "Clear playlist"
    },
    {
        .icon = "document-open",
        .type = CTRL_PUSH_BUTTON,
        .action = ACTION_PSID_PLAYLIST_LOAD,
        .tooltip = "Open playlist file",
        .margin_start = 16
    },
    {
        .icon = "document-save",
        .type = CTRL_PUSH_BUTTON,
        .action = ACTION_PSID_PLAYLIST_SAVE,
        .tooltip = "Save playlist file"
    },
    { .type = CTRL_LIST_END }
};

/** \brief  Event handler for playlist control buttons
 *
 * Trigger UI action on click/toggle.
 *
 * \param[in]   button  button triggering the event (currently ignored)
 * \param[in]   action  UI action ID
 */
static void trigger_ui_action(GtkWidget *button, gpointer action)
{
    ui_action_trigger(GPOINTER_TO_INT(action));
}


/** \brief  Create a grid with a list of buttons to control the playlist
 *
 * Most of the playlist should also be controllable via a context-menu
 * (ie mouse right-click), which is a big fat TODO now.
 *
 * \return  GtkGrid
 */
static GtkWidget *vsid_playlist_controls_create(void)
{
    GtkWidget *grid;
    int i;

    grid = vice_gtk3_grid_new_spaced(0, VICE_GTK3_DEFAULT);
    for (i = 0; (i < MAX_CONTROLS) && (controls[i].type != CTRL_LIST_END); i++) {
        GtkWidget *widget;
        GtkWidget *image;
        gulong handler;
        gchar icon[256];

        /* create symbolic icon name */
        g_snprintf(icon, sizeof icon, "%s-symbolic", controls[i].icon);

        widget = NULL;
        handler = 0;
        switch (controls[i].type) {

            case CTRL_PUSH_BUTTON:
                /* normal GtkButton */
                widget = gtk_button_new_from_icon_name(icon,
                                                       GTK_ICON_SIZE_LARGE_TOOLBAR);
                /* always show icon and don't grab focus on click/tab */
                gtk_button_set_always_show_image(GTK_BUTTON(widget), TRUE);
                if (controls[i].action > ACTION_NONE) {
                    handler = g_signal_connect(widget,
                                               "clicked",
                                               G_CALLBACK(trigger_ui_action),
                                               GINT_TO_POINTER(controls[i].action));
                } else if (controls[i].callback != NULL) {
                    handler = g_signal_connect(widget,
                                               "clicked",
                                               G_CALLBACK(controls[i].callback),
                                               NULL);
                } else {
                    gtk_widget_set_sensitive(widget, FALSE);
                }
                break;

            case CTRL_TOGGLE_BUTTON:
                /* GtkToggleButton */
                widget = gtk_toggle_button_new();
                image = gtk_image_new_from_icon_name(icon,
                                                     GTK_ICON_SIZE_LARGE_TOOLBAR);
                gtk_container_add(GTK_CONTAINER(widget), image);
                if (controls[i].action > ACTION_NONE) {
                    handler = g_signal_connect(widget,
                                               "toggled",
                                               G_CALLBACK(trigger_ui_action),
                                               GINT_TO_POINTER(controls[i].action));
                } else if (controls[i].callback != NULL) {
                    handler = g_signal_connect(widget,
                                               "toggled",
                                               G_CALLBACK(controls[i].callback),
                                               NULL);
                } else {
                    gtk_widget_set_sensitive(widget, FALSE);
                }
                break;

            case CTRL_CENTERED_LABEL:
                /* GtkLabel, centered */
                widget = gtk_label_new(NULL);
                if (controls[i].text != NULL) {
                    gtk_label_set_markup(GTK_LABEL(widget), controls[i].text);
                }
                gtk_widget_set_halign(widget, GTK_ALIGN_CENTER);
               break;

            default:
                debug_gtk3("Unknown control type %d!!!", controls[i].type);
        }

        if (widget != NULL) {
            gtk_widget_set_can_focus(widget, FALSE);
            control_widgets[i] = widget;
            control_handlers[i] = handler;

            if (controls[i].margin_start > 0) {
                gtk_widget_set_margin_start(widget, controls[i].margin_start);
            }
            if (controls[i].margin_end > 0) {
                gtk_widget_set_margin_end(widget, controls[i].margin_end);
            }

            gtk_grid_attach(GTK_GRID(grid), widget, i, 0, 1, 1);
            if (controls[i].tooltip != NULL) {
                gtk_widget_set_tooltip_text(widget, controls[i].tooltip);
            }
        }
    }
    return grid;
}


/** \brief  Create main playlist widget
 *
 * \return  GtkGrid
 */
GtkWidget *vsid_playlist_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *scroll;

    vsid_playlist_model_create();
    vsid_playlist_view_create();

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Playlist:</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    title_widget = label;

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scroll, 600, 180);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), playlist_view);
    gtk_grid_attach(GTK_GRID(grid), scroll, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid),
                    vsid_playlist_controls_create(),
                    0, 2, 1, 1);

    g_signal_connect_unlocked(grid,
                              "destroy",
                              G_CALLBACK(on_destroy),
                              NULL);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Append \a path to the playlist
 *
 * \param[in]   path    path to SID file
 *
 * \return  TRUE on success, FALSE on failure
 */
gboolean vsid_playlist_append_file(const gchar *path)
{
    GtkTreeIter iter;
    hvsc_psid_t psid;

    /* Attempt to parse sid header for title & composer */
    if (!hvsc_psid_open(path, &psid)) {
        vice_gtk3_message_error(NULL,   /* use VSID window as parent */
                                "VSID",
                                "Failed to parse PSID header of '%s'.",
                                path);


         debug_gtk3("Perhaps it's a MUS?");
        /* append MUS to playlist
         *
         * TODO: somehow parse the .mus file for author/tune name,
         *       which is pretty much impossible.
         */
        gtk_list_store_append(playlist_model, &iter);
        gtk_list_store_set(playlist_model,
                           &iter,
                           COL_TITLE, "n/a",
                           COL_AUTHOR, "n/a",
                           COL_FULL_PATH, path,
                           COL_DISPLAY_PATH, path,
                           -1);

    } else {
        char name[HVSC_PSID_TEXT_LEN + 1];
        char author[HVSC_PSID_TEXT_LEN + 1];
        char *name_utf8;
        char *author_utf8;
        char *display_path;

        /* get SID name and author */
        memcpy(name, psid.name, HVSC_PSID_TEXT_LEN);
        name[HVSC_PSID_TEXT_LEN] = '\0';
        memcpy(author, psid.author, HVSC_PSID_TEXT_LEN);
        author[HVSC_PSID_TEXT_LEN] = '\0';

        name_utf8 = convert_to_utf8(name);
        author_utf8 = convert_to_utf8(author);
        display_path = strip_hvsc_base(path);

        /* append SID to playlist */
        gtk_list_store_append(playlist_model, &iter);
        gtk_list_store_set(playlist_model,
                           &iter,
                           COL_TITLE, name_utf8,
                           COL_AUTHOR, author_utf8,
                           COL_FULL_PATH, path,
                           COL_DISPLAY_PATH, display_path,
                           -1);

        /* clean up */
        g_free(name_utf8);
        g_free(author_utf8);
        g_free(display_path);
        hvsc_psid_close(&psid);
    }

    update_current_and_total();
    return TRUE;
}


/** \brief  Remove SID file at \a row from playlist
 *
 * \param[in]   row     row in playlist
 *
 * FIXME:   unlikely this will be used.
 */
void vsid_playlist_remove_file(int row)
{
    if (row < 0) {
        return;
    }
    /* TODO: actually remove item :) */

    update_current_and_total();
}


/** \brief  Play first tune in the playlist
 */
void vsid_playlist_first(void)
{
    GtkTreeIter iter;

    mainlock_assert_is_not_vice_thread();

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playlist_model), &iter)) {
        const gchar *filename;
        GtkTreeSelection *selection;
        GValue value = G_VALUE_INIT;

        /* get selection object, unselect all rows and select the first row */
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_view));
        gtk_tree_selection_unselect_all(selection);
        gtk_tree_selection_select_iter(selection, &iter);
        /* scroll row into view */
        scroll_to_iter(&iter);
        gtk_tree_model_get_value(GTK_TREE_MODEL(playlist_model),
                                 &iter,
                                 COL_FULL_PATH,
                                 &value);
        filename = g_value_get_string(&value);

        /* TODO: check result */
        ui_vsid_window_load_psid(filename);
        g_value_unset(&value);

        update_current_and_total();
    }
}


/** \brief  Play previous tune in the playlist
 */
void vsid_playlist_previous(void)
{
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GtkTreeModel *model;

    mainlock_assert_is_not_vice_thread();

    /* we can't take the address of GTK_TREE_MODEL(x) so we need this: */
    model = GTK_TREE_MODEL(playlist_model);
    /* get selection object and temporarily set mode to single-selection so
     * we can get an iter (this will keep the 'anchor' selected: the first
     * row clicked when selecting multiple rows) */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    if (gtk_tree_selection_get_selected(selection,
                                        &model,
                                        &iter)) {
        if (gtk_tree_model_iter_previous(model, &iter)) {
            const gchar *filename;
            GValue value = G_VALUE_INIT;

            gtk_tree_selection_select_iter(selection, &iter);
            scroll_to_iter(&iter);
            gtk_tree_model_get_value(model,
                                     &iter,
                                     COL_FULL_PATH,
                                     &value);
            filename = g_value_get_string(&value);
            ui_vsid_window_load_psid(filename);
            g_value_unset(&value);

            update_current_and_total();
        }
    }
    /* restore multi-select */
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
}


/** \brief  Play next tune in the playlist
 */
void vsid_playlist_next(void)
{
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GtkTreeModel *model;

    mainlock_assert_is_not_vice_thread();

    /* we can't take the address of GTK_TREE_MODEL(x) so we need this: */
    model = GTK_TREE_MODEL(playlist_model);
    /* get selection object and temporarily set mode to single-selection so
     * we can get an iter (this will keep the 'anchor' selected: the first
     * row clicked when selecting multiple rows) */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    if (gtk_tree_selection_get_selected(selection,
                                        &model,
                                        &iter)) {
        if (gtk_tree_model_iter_next(model, &iter)) {
            const gchar *filename;
            GValue value = G_VALUE_INIT;

            gtk_tree_selection_select_iter(selection, &iter);
            scroll_to_iter(&iter);
            gtk_tree_model_get_value(model,
                                     &iter,
                                     COL_FULL_PATH,
                                     &value);
            filename = g_value_get_string(&value);
            ui_vsid_window_load_psid(filename);
            g_value_unset(&value);

            update_current_and_total();
        }
    }
    /* restore multi-select */
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
}


/** \brief  Play last tune in the playlist
 */
void vsid_playlist_last(void)
{
    GtkTreeSelection *selection;
    GtkTreeIter iter;

    mainlock_assert_is_not_vice_thread();

    /* get selection object and deselect all rows */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_view));
    gtk_tree_selection_unselect_all(selection);

    /* There is no gtk_tree_model_get_iter_last(), so: */
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playlist_model), &iter)) {
        GtkTreeIter prev;
        const gchar *filename;
        GValue value = G_VALUE_INIT;

        /* move to last row */
        prev = iter;
        while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playlist_model), &iter)) {
            prev = iter;
        }
        iter = prev;
        gtk_tree_selection_select_iter(selection, &iter);
        /* scroll row into view */
        scroll_to_iter(&iter);

        /* now get the sid filename and load it */
        gtk_tree_model_get_value(GTK_TREE_MODEL(playlist_model),
                                 &iter,
                                 COL_FULL_PATH,
                                 &value);
        filename = g_value_get_string(&value);
        ui_vsid_window_load_psid(filename);
        g_value_unset(&value);

        update_current_and_total();
    }
}


/** \brief  Remove selected tunes from the playlist
 */
void vsid_playlist_remove_selection(void)
{
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GList *rows;
    GList *elem;

    /* get model in the correct type for gtk_tree_model_get_iter(),
     * taking the address of GTK_TREE_MODEL(M) doesn't work.
     */
    model = GTK_TREE_MODEL(playlist_model);
    /* get current selection */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_view));
    /* get rows in the selection, which is a GList of GtkTreePath *'s */
    rows = gtk_tree_selection_get_selected_rows(selection, &model);

    /* iterate the list of rows in reverse order to avoid
     * invalidating the GtkTreePath*'s in the list
     */
    for (elem = g_list_last(rows); elem != NULL; elem = elem->prev) {
        GtkTreeIter iter;
        GtkTreePath *path = elem->data;

        /* delete row */
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_model),
                                    &iter,
                                    path)) {
            gtk_list_store_remove(playlist_model, &iter);
        }
    }
    g_list_free_full(rows, (GDestroyNotify)gtk_tree_path_free);
    update_current_and_total();
}


/** \brief  Show dialog to add files to the playlist
 */
void vsid_playlist_add(void)
{
    vsid_playlist_add_dialog_exec(add_files_callback);
}


/** \brief  Clear playlist
 *
 * Shows confirmation dialog box before clearing.
 */
void vsid_playlist_clear(void)
{
    mainlock_assert_is_not_vice_thread();

    vice_gtk3_message_confirm(NULL, /* VSID window as parent */
                              clear_playlist_callback,
                              "VSID",
                              "Are you sure you wish to clear the playlist?");
}


/** \brief  Show dialog to load a playlist
 */
void vsid_playlist_load(void)
{
    GtkWidget *dialog;

    mainlock_assert_is_not_vice_thread();

    /* if we don't have a previous directory, we use the HVSC base directory */
    set_playlist_dialogs_default_dir();

    /* create dialog and set the initial directory */
    dialog = vice_gtk3_open_file_dialog("Load playlist",
                                        "Playlist files",
                                        file_chooser_pattern_playlist,
                                        NULL,
                                        playlist_load_callback,
                                        NULL);
    lastdir_set(dialog, &playlist_last_dir, NULL);
    gtk_widget_show_all(dialog);
}


/** \brief  Show dialog to save the playlist
 */
void vsid_playlist_save(void)
{
    GtkWidget *dialog;
    GtkWidget *content;
    gint rows;

    mainlock_assert_is_not_vice_thread();

    /* don't try to save an empty playlist */
    rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playlist_model), NULL);
    if (rows < 1) {
        ui_display_statustext("Error: cannot save empty playlist.", true);
        return;
    }

    /* if we don't have a previous directory, we use the HVSC base directory */
    set_playlist_dialogs_default_dir();
    /* create dialog and set initial directory */
    dialog = vice_gtk3_save_file_dialog("Save playlist file",
                                        vsid_playlist_get_path(),
                                        TRUE,
                                        NULL,
                                        playlist_save_dialog_callback,
                                        NULL);
    lastdir_set(dialog, &playlist_last_dir, NULL);
    /* add content area widget which allows setting playlist title */
    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content), create_save_content_area());
    gtk_widget_show_all(dialog);
}



/** \brief  Get row number of currently selected tune
 *
 * \param[out]  path    path to SID file (optional, pass `NULL` to ignore
 *
 * \return  row number or -1 when no selection or multiple rows selected
 */
gint vsid_playlist_get_current_row(const gchar **path)
{
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreePath *treepath;
    GList *rows;
    gint rownum = -1;

    model = GTK_TREE_MODEL(playlist_model);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_view));
    rows = gtk_tree_selection_get_selected_rows(selection, &model);

    if (rows != NULL) {
        if (g_list_length(rows) > 1) {
            /* multiple rows selected */
            if (path != NULL) {
                *path = NULL;
            }
        } else {
            const gint *indices = NULL;

            /* first (and only) element contains the tree path */
            treepath = rows->data;
            indices = gtk_tree_path_get_indices(treepath);
            if (indices != NULL) {
                /* finally, we have the row number :) */
                rownum = indices[0];
                if (path != NULL) {
                    GtkTreeIter iter;

                    if (gtk_tree_model_get_iter(model, &iter, treepath)) {
                        GValue value = G_VALUE_INIT;

                        gtk_tree_model_get_value(model,
                                                 &iter,
                                                 COL_FULL_PATH,
                                                 &value);
                        /* and now we finally have the path to the SID file */
                        *path = g_value_get_string(&value);
                    }
                }
            }
        }
        g_list_free_full(rows, (GDestroyNotify)gtk_tree_path_free);
    }
    return rownum;
}
