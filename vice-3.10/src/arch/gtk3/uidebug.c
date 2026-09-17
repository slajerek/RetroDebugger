/** \file   uidebug.c
 * \brief   Debug menu dialogs
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES AutoPlaybackFrames  all
 * $VICERES TraceMode           all
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
#include "debug.h"

#ifdef DEBUG

#include <gtk/gtk.h>

#include "debug_gtk3.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "vice_gtk3.h"

#include "uidebug.h"


/** \brief  List of debug trace modes
 */
static const vice_gtk3_radiogroup_entry_t trace_modes[] = {
    { "Normal",     DEBUG_NORMAL },
    { "Small",      DEBUG_SMALL },
    { "History",    DEBUG_HISTORY },
    { "Autoplay",   DEBUG_AUTOPLAY },
    { NULL, -1 }
};


/** \brief  Create widget to control number of autoplayback frames
 *
 * \return  GtkGrid
 */
static GtkWidget *create_playback_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *spin;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    label = gtk_label_new("Auto playback frames");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    spin = vice_gtk3_resource_spin_int_new("AutoPlaybackFrames",
            0, 65536, 10);
    gtk_grid_attach(GTK_GRID(grid), spin, 1, 0, 1,1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create widget to control trace mode
 *
 * \return  GtkGrid
 */
static GtkWidget *create_trace_widget(void)
{
    GtkWidget *grid;
    GtkWidget *group;

    grid = vice_gtk3_grid_new_spaced_with_label(
            -1, -1,
            "Select CPU/Drive trace mode",
            1);
    group = vice_gtk3_resource_radiogroup_new("TraceMode", trace_modes,
            GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_margin_top(group, 16);
    gtk_widget_set_margin_start(group, 16);
    gtk_widget_set_margin_end(group, 16);
    gtk_widget_set_margin_bottom(group, 16);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create dialog to control trace mode
 *
 * \return  GtkDialog
 */
static GtkWidget *create_trace_mode_dialog(void)
{
    GtkWidget *dialog;
    GtkWidget *content;

    dialog = gtk_dialog_new_with_buttons("Select trace mode",
            ui_get_active_window(), GTK_DIALOG_MODAL,
            "Close", GTK_RESPONSE_CLOSE,
            NULL);

    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_widget_set_margin_top(content, 16);
    gtk_widget_set_margin_start(content, 16);
    gtk_widget_set_margin_end(content, 16);
    gtk_widget_set_margin_bottom(content, 16);

    gtk_container_add(GTK_CONTAINER(content), create_trace_widget());

    return dialog;
}


/** \brief  Create dialog to control playback frames
 *
 * \return  GtkDialog
 */
static GtkWidget *create_playback_frames_dialog(void)
{
    GtkWidget *dialog;
    GtkWidget *content;

    dialog = gtk_dialog_new_with_buttons("Set auto playback frames",
            ui_get_active_window(), GTK_DIALOG_MODAL,
            "Close", GTK_RESPONSE_CLOSE,
            NULL);

    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content), create_playback_widget());

    return dialog;
}


/** \brief  Handler for the 'destroy' event of the dialogs
 *
 * Signals the UI actions system the action has finished.
 *
 * \param[in]   dialog  dialog (unused)
 * \param[in]   action  UI action ID
 */
static void on_destroy(GtkWidget *dialog, gpointer action)
{
    ui_action_finish(GPOINTER_TO_INT(action));
}


/** \brief  Handler for the 'response' event of the trace mode dialog
 *
 * \param[in,out]   dialog      dialog triggering the event
 * \param[in]       response_id response ID
 * \param[in]       data        extra event data (unused)
 */
static void on_response_trace_mode(GtkDialog *dialog,
                                   gint response_id,
                                   gpointer data)
{
    debug_gtk3("got response %d.", response_id);
    if (response_id == GTK_RESPONSE_CLOSE) {
        gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}


/** \brief  Handler for the 'response' event of the playback frames dialog
 *
 * \param[in,out]   dialog      dialog triggering the event
 * \param[in]       response_id response ID
 * \param[in]       data        extra event data (unused)
 */
static void on_response_playback_frames(GtkDialog *dialog,
                                        gint response_id,
                                        gpointer data)
{
    debug_gtk3("got response %d.", response_id);
    if (response_id == GTK_RESPONSE_CLOSE) {
        gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}


/** \brief  Show dialog to set trace mode
 */
void ui_debug_trace_mode_dialog_show(void)
{
    GtkWidget *dialog;

    dialog = create_trace_mode_dialog();
    g_signal_connect(dialog,
                     "response",
                     G_CALLBACK(on_response_trace_mode),
                     NULL);
    g_signal_connect(dialog,
                     "destroy",
                     G_CALLBACK(on_destroy),
                     GINT_TO_POINTER(ACTION_DEBUG_TRACE_MODE));
    gtk_widget_show_all(dialog);
}


/** \brief  Show 'Autoplay playback frames' dialog
*/
void ui_debug_playback_frames_dialog_show(void)
{
    GtkWidget *dialog;

    dialog = create_playback_frames_dialog();
    g_signal_connect(dialog,
                     "response",
                     G_CALLBACK(on_response_playback_frames),
                     NULL);
    g_signal_connect(dialog,
                     "destroy",
                     G_CALLBACK(on_destroy),
                     GINT_TO_POINTER(ACTION_DEBUG_AUTOPLAYBACK_FRAMES));
    gtk_widget_show_all(dialog);
}

#endif
