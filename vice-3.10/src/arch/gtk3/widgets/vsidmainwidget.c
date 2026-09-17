/** \file   vsidmainwidget.c
 * \brief   GTK3 main widget for VSD
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

#include <stdlib.h>
#include <stdbool.h>
#include <gtk/gtk.h>

#include "vice_gtk3.h"
#include "debug.h"
#include "hvsc.h"
#include "machine.h"
#include "lib.h"
#include "log.h"
#include "ui.h"
#include "uivsidmenu.h"
#include "uivsidwindow.h"
#include "vsidtuneinfowidget.h"
#include "vsidcontrolwidget.h"
#include "vsidmixerwidget.h"
#include "vsidplaylistwidget.h"
#include "hvscstilwidget.h"
#include "sid.h"

#include "vsidmainwidget.h"


extern char *psid_autostart_image;


/** \brief  Main widget grid */
static GtkWidget *main_widget;

/** \brief  Left pane grid */
static GtkWidget *left_pane;

/** \brief  Tune info grid */
static GtkWidget *tune_info_widget;

/** \brief  Play controls grid */
static GtkWidget *control_widget;

/** \brief  Mixer controls grid */
static GtkWidget *mixer_widget;

/** \brief  STIL view grid */
static GtkWidget *stil_widget;

/** \brief  Playlist grid */
static GtkWidget *playlist_widget;


/** \brief  Handler for the 'drag-motion' event
 *
 * \param[in]   widget  widget triggering the event
 * \param[in]   context drag context
 * \param[in]   x       x coordinate of the current cursor position
 * \param[in]   y       y coordinate of the current cursor position
 * \param[in]   time    timestamp of the event
 * \param[in]   data    extra event data
 *
 * \return  TRUE
 */
static gboolean on_drag_motion(
        GtkWidget *widget,
        GdkDragContext *context,
        gint x,
        gint y,
        guint time,
        gpointer data)
{
    gdk_drag_status(context, GDK_ACTION_COPY, time);
    return TRUE;
}


/** \brief  Handler for the 'drag-drop' event of the GtkWindow
 *
 * Can be used to filter certain drop targets or altering the data before
 * triggering the 'drag-drop-received' event. Currently just returns TRUE
 *
 * \param[in]   widget  widget triggering the event
 * \param[in]   context drag context
 * \param[in]   x       x coordinate of the current cursor position
 * \param[in]   y       y coordinate of the current cursor position
 * \param[in]   time    timestamp of the event
 * \param[in]   data    extra event data (unused)
 *
 * \return  TRUE
 */
static gboolean on_drag_drop(
        GtkWidget *widget,
        GdkDragContext *context,
        gint x,
        gint y,
        guint time,
        gpointer data)
{
    GList *targets;
    GList *t;

    targets = gdk_drag_context_list_targets(context);
    if (targets == NULL) {
        debug_gtk3("No targets");
        return FALSE;
    }
    for (t = targets; t != NULL; t = t->next) {
        debug_gtk3("target: %p.", t->data);
    }

    return TRUE;
}


/** \brief  Handler for the 'drag-data-received' event
 *
 * Autostarts a SID file.
 *
 * \param[in]   widget      widget triggering the event (unused)
 * \param[in]   context     drag context (unused)
 * \param[in]   x           probably X-coordinate in the drop target?
 * \param[in]   y           probablt Y-coordinate in the drop target?
 * \param[in]   data        dragged data
 * \param[in]   info        int declared in the targets array (unclear)
 * \param[in]   time        no idea
 *
 * \todo    Once this works properly, remove a lot of debugging calls, perhaps
 *          changing a few into log calls.
 *
 * \todo    Keep list of multiple files/URIs so we can add multiple SID files
 *          to the playlist without a lot of code duplication.
 *
 * \todo    Figure out why the drop event on the STIL widget only triggers
 *          *outside* the textbox.
 */
static void on_drag_data_received(
        GtkWidget *widget,
        GdkDragContext *context,
        int x,
        int y,
        GtkSelectionData *data,
        guint info,
        guint time)
{
    gchar **uris = NULL;
    gchar *filename = NULL;
    gchar **files = NULL;
    guchar *text = NULL;
    int i;

    debug_gtk3("got drag-data, info = %u:", info);
    if (widget == left_pane) {
        debug_gtk3("got data for 'left_pane'.");
    } else if (widget == stil_widget || hvsc_stil_widget_get_view()) {
        debug_gtk3("got data for 'stil_widget'.");
    } else if (widget == playlist_widget) {
        debug_gtk3("got data for 'playlist_widget.");
    } else {
        debug_gtk3("got data for unhandled widget.");
        return;
    }

    gtk_drag_finish(context, TRUE, FALSE, time);

    switch (info) {

        case DT_TEXT_URI_LIST:
            /*
             * This branch appears to be taken on both Windows and macOS.
             */

            /* got possible list of URI's */
            uris = gtk_selection_data_get_uris(data);
            if (uris != NULL) {
                /* dump URI's on stdout */
                debug_gtk3("got URI's:");
                for (i = 0; uris[i] != NULL; i++) {

                    debug_gtk3("URI: '%s'\n", uris[i]);
                    filename = g_filename_from_uri(uris[i], NULL, NULL);
                    debug_gtk3("filename: '%s'.", filename);
                    if (filename != NULL) {
                        g_free(filename);
                    }
                }

                /* use the first/only entry as the autostart file
                 */
                if (uris[0] != NULL) {
                    filename = g_filename_from_uri(uris[0], NULL, NULL);
                } else {
                    filename = NULL;
                }
            }
            break;

        case DT_TEXT_PLAIN:
            /*
             * this branch appears to be taken on both Gtk and Qt based WM's
             * on Linux
             */


            /* text will contain a newline separated list of 'file://' URIs,
             * and a trailing newline */
            text = gtk_selection_data_get_text(data);
            /* remove trailing whitespace */
            g_strchomp((gchar *)text);

            debug_gtk3("Got data as text: '%s'.", text);
            files = g_strsplit((const gchar *)text, "\n", -1);
            g_free(text);

            for (i = 0; files[i] != NULL; i++) {
#ifdef HAVE_DEBUG_GTK3UI
                gchar *tmp = g_filename_from_uri(files[i], NULL, NULL);
#endif
                debug_gtk3("URI: '%s', filename: '%s'.",
                        files[i], tmp);
            }
            /* now grab the first file */
            filename = g_filename_from_uri(files[0], NULL, NULL);

            debug_gtk3("got filename '%s'.", filename);
            break;

        default:
            debug_gtk3("Warning: unhandled d'n'd target %u.", info);
            filename = NULL;
            break;
    }


    if (widget == left_pane
            || widget == stil_widget || widget == hvsc_stil_widget_get_view()) {
        /* can we attempt autostart? */
        if (filename != NULL) {
            /* fix for Nautilus and perhaps other file managers */
            gchar *tmp = g_filename_from_uri(filename, NULL, NULL);
            if (tmp != NULL) {
                g_free(filename);
                filename = tmp;
            }

            debug_gtk3("Attempting to autostart '%s'.", filename);
            if (ui_vsid_window_load_psid(filename) != 0) {
                debug_gtk3("failed.");
            } else {
                debug_gtk3("OK!");
            }
        }
    } else if (widget == playlist_widget) {
        debug_gtk3("attempting to add SIDs to the playlist.");
        if (files != NULL) {
            debug_gtk3("got array of filenames:");
            for (i = 0; files[i] != NULL; i++) {
                gchar *tmp;

                /*
                 * It looks like at least Gnome 3/Nautilus returns a simple
                 * string, but after splitting we end with URI's anyway.
                 */
                tmp = g_filename_from_uri(files[i], NULL, NULL);
                if (tmp == NULL) {
                    debug_gtk3("adding '%s'.", files[i]);
                    vsid_playlist_append_file(files[i]);
                } else {
                    debug_gtk3("adding '%s'.", tmp);
                    vsid_playlist_append_file(tmp);
                    g_free(tmp);
                }
            }
        } else if (uris != NULL) {
            debug_gtk3("got array of URIs:");
            for (i = 0; uris[i] != NULL; i++) {
                gchar *tmp = g_filename_from_uri(uris[i], NULL, NULL);

                debug_gtk3("adding '%s'.", tmp);
                vsid_playlist_append_file(tmp);
                g_free(tmp);
            }
        }
    }


    if (files != NULL) {
        g_strfreev(files);
    }
    if (uris != NULL) {
        g_strfreev(uris);
    }
    if (filename != NULL) {
        g_free(filename);
    }

}

/** \brief Called each frame for UI updates
 */
void vsid_main_widget_update(void)
{
    vsid_tune_info_widget_update();
}


/** \brief  Create VSID main widget
 *
 * \return  GtkGrid
 */
GtkWidget *vsid_main_widget_create(void)
{
    GtkWidget *grid;
#if 0
    GtkWidget *view;
#endif
    char fullpath[ARCHDEP_PATH_MAX];

    grid = vice_gtk3_grid_new_spaced(32, 8);
    gtk_widget_set_margin_top(grid, 16);
    gtk_widget_set_margin_start(grid, 16);
    gtk_widget_set_margin_end(grid, 16);
    gtk_widget_set_margin_bottom(grid, 16);

    /* left pane: info, playback controls, mixer */
    left_pane = vice_gtk3_grid_new_spaced(0, 16);

    tune_info_widget = vsid_tune_info_widget_create();
    gtk_grid_attach(GTK_GRID(left_pane), tune_info_widget, 0, 0, 1, 1);

    control_widget = vsid_control_widget_create();
    gtk_grid_attach(GTK_GRID(left_pane), control_widget, 0, 1, 1, 1);

    mixer_widget = vsid_mixer_widget_create();
    gtk_widget_set_valign(mixer_widget, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(left_pane), mixer_widget, 0, 2, 1, 1);

    gtk_widget_set_hexpand(left_pane, FALSE);
    gtk_grid_attach(GTK_GRID(grid), left_pane, 0, 0, 1, 2);

    /* right top pane: STIL widget */
    stil_widget = hvsc_stil_widget_create();
    /*gtk_widget_set_vexpand(stil_widget, TRUE);*/
    gtk_widget_set_hexpand(stil_widget, FALSE);
    gtk_grid_attach(GTK_GRID(grid), stil_widget, 1, 0, 1, 1);

    /* right bottom pane: playlist */
    playlist_widget = vsid_playlist_widget_create();
    gtk_grid_attach(GTK_GRID(grid), playlist_widget, 1, 1, 1, 1);


    gtk_widget_set_vexpand(grid, TRUE);

    /*
     * Set up drag-n-drop handlers
     */

    /* left pane: info, playback controls, mixer */
    gtk_drag_dest_set(
            left_pane,
            GTK_DEST_DEFAULT_ALL,
            ui_drag_targets,
            UI_DRAG_TARGETS_COUNT,
            GDK_ACTION_COPY);
    g_signal_connect(left_pane, "drag-data-received",
                     G_CALLBACK(on_drag_data_received), NULL);
    g_signal_connect(left_pane, "drag-drop",
                     G_CALLBACK(on_drag_drop), NULL);

    /* middle pane: STIL widget */
    gtk_drag_dest_set(
            stil_widget,
            GTK_DEST_DEFAULT_ALL,
            ui_drag_targets,
            UI_DRAG_TARGETS_COUNT,
            GDK_ACTION_COPY);
    g_signal_connect(stil_widget, "drag-motion",
                     G_CALLBACK(on_drag_motion), NULL);
    g_signal_connect(stil_widget, "drag-data-received",
                     G_CALLBACK(on_drag_data_received), NULL);
    g_signal_connect(stil_widget, "drag-drop",
                     G_CALLBACK(on_drag_drop), NULL);

    /* Enabling the following makes the GtTextView widget accept all sorts of
     * data, including pasting text from the clipboard with the context menu.
     * So for now dropping a SID onto the text view is disabled until I find
     * a better solution. --compyx
     */
#if 0
    /* not the cleanest method maybe, but somehow the GtkTextView doesn't
     * trigger the drag-drop events otherwise */
    view = hvsc_stil_widget_get_view();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), TRUE);

    gtk_drag_dest_set(
            view,
            GTK_DEST_DEFAULT_ALL,
            ui_drag_targets,
            UI_DRAG_TARGETS_COUNT,
            GDK_ACTION_COPY);
    g_signal_connect(view, "drag-data-received",
                     G_CALLBACK(on_drag_data_received), NULL);
    g_signal_connect(view, "drag-drop",
                     G_CALLBACK(on_drag_drop), NULL);
    g_signal_connect(stil_widget, "drag-motion",
                     G_CALLBACK(on_drag_motion), NULL);
#endif

    /* right pane: playlist */
    gtk_drag_dest_set(
            playlist_widget,
            GTK_DEST_DEFAULT_ALL,
            ui_drag_targets,
            UI_DRAG_TARGETS_COUNT,
            GDK_ACTION_COPY);
    g_signal_connect(playlist_widget, "drag-data-received",
                     G_CALLBACK(on_drag_data_received), NULL);
    g_signal_connect(playlist_widget, "drag-drop",
                     G_CALLBACK(on_drag_drop), NULL);

    main_widget = grid;
    gtk_widget_show_all(grid);

    /* Try to load STIL info and SLDB data for a file passed on the command line */
    if (psid_autostart_image != NULL) {
        if (archdep_real_path(psid_autostart_image, fullpath)) {
            char digest[33];

            debug_gtk3("Looking up STIL/SLDB info for PSID specified on command line: %s",
                       fullpath);
            if (hvsc_md5_digest(fullpath, digest)) {
                debug_gtk3("setting STIL and SLDB info for md5 digest %s", digest);
                hvsc_stil_widget_set_psid_md5(digest);
                vsid_tune_info_widget_set_song_lengths_md5(digest);
            } else {
                /* normally won't happen */
                debug_gtk3("failed to get md5 digest for %s", fullpath);
            }
        }
        lib_free(psid_autostart_image);
        psid_autostart_image = NULL;
    }

    return grid;
}
