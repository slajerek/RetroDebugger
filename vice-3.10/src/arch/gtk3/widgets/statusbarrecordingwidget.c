/** \file   statusbarrecordingwidget.c
 * \brief   Widget to display and control recording of events/audio/video
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
#include "vice_gtk3.h"
#include "basedialogs.h"
#include "vice-event.h"
#include "machine.h"
#include "resources.h"
#include "screenshot.h"
#include "sound.h"
#include "ui.h"
#include "uiapi.h"
#include "uimedia.h"
#include "log.h"

#include "statusbarrecordingwidget.h"

/* #define DEBUG_RECORDING */

#ifdef DEBUG_RECORDING
#define DBG(x) log_printf x
#else
#define DBG(x)
#endif


/** \brief  Seconds to wait before hiding the widget after pressing 'STOP'
 */
#define HIDE_ALL_TIMEOUT 5

/** \brief  CSS rules for the media recording STOP button
 */
#define STOP_BUTTON_CSS \
    "button { \n" \
    "  padding: 0;\n" \
    "  min-width: 14px;\n" \
    "  min-height: 10px;\n" \
    "  margin-top: 0px;\n" \
    "  margin-bottom: 2px;\n" \
    "  margin-right: 8px;\n" \
    "}"


/** \brief  CSS for the timing widget */
#define TIMING_CSS \
    "label { \n" \
    "  font-family: monospace;\n" \
    "}"


/** \brief  Columns in the recording widget
 */
enum {
    RW_COL_TEXT = 0,    /**< recording status label */
    RW_COL_TIME = 1,    /**< recording time label */
    RW_COL_BUTTON = 2   /**< STOP button */
};


/** \brief  Rows in the recording widget
 */
enum {
    RW_ROW_TEXT = 0,    /**< recording status label */
    RW_ROW_TIME = 0,    /**< recording time label */
    RW_ROW_BUTTON = 0   /**< STOP button (takes both rows) */
};

#if 0
/** \brief  Types of recordings
 */
enum {
    RW_TYPE_NONE,   /**< nothing is being recorded */
    RW_TYPE_EVENTS, /**< recording events */
    RW_TYPE_AUDIO,  /**< recording audio */
    RW_TYPE_VIDEO   /**< recording video */
};
#endif

/** \brief  Types of recordings as strings
 */
static const gchar *rec_types[] = {
    "inactive",
    "events",
    "audio",
    "video"
};


/** \brief  GSource id of the timeout to hide the recording button
 */
static guint timeout_id = 0;


/** \brief  Callback for the g_timeout
 *
 * \param[in,out]   data    statusbar recording widget
 *
 * \return  boolean (TRUE == keep running, FALSE == delete timer source)
 */
static gboolean update_timer(gpointer data)
{
    guint time;
    int status;

    /* do we have status == stop? */
    status = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(data), "Status"));
    if (status == 0) {
        return FALSE;   /* unregister timer */
    }

    /* get time in seconds, but add one because the timer counts down 1 second,
     * so after one second we actually have 1 second */
    time = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(data), "Seconds"));
    statusbar_recording_widget_set_time(GTK_WIDGET(data), time + 1, 0);

    /* update time, again adjusting to the timer countdown behaviour */
    g_object_set_data(G_OBJECT(data), "Seconds", GUINT_TO_POINTER(time + 1));

    return TRUE;
}


/** \brief  Event handler for the 'clicked' event of the STOP button
 *
 * Stops all recordings.
 *
 * \param[in,out]   button  button triggering the event
 * \param[in,out]   data    statusbar recording widget (GtkGrid)
 */
static void on_stop_clicked(GtkWidget *button, gpointer data)
{
    GtkWidget *label;

    ui_media_stop_recording();
    if (event_record_active()) {
        event_record_stop();
    }

    label = gtk_grid_get_child_at(GTK_GRID(data), RW_COL_TEXT, RW_ROW_TEXT);
    gtk_label_set_text(GTK_LABEL(label), "Recording stopped.");

    statusbar_recording_widget_hide_all(GTK_WIDGET(data), HIDE_ALL_TIMEOUT);
}


/** \brief  Create "Stop" button
 *
 * Create a Stop button with an icon, reduced in size.
 *
 * \return  GtkButton
 */
static GtkWidget *create_stop_button(void)
{
    GtkWidget *button;

    button = gtk_button_new_from_icon_name("media-playback-stop-symbolic",
                                           GTK_ICON_SIZE_SMALL_TOOLBAR);
    /* set up CSS to reduce button size */
    vice_gtk3_css_add(button, STOP_BUTTON_CSS);
    return button;
}



/** \brief  Create recording status widget
 *
 * Generate a widget to show on the statusbar to display recording state.
 *
 * \return  GtkGrid
 */
GtkWidget *statusbar_recording_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *button;

    grid = vice_gtk3_grid_new_spaced(8, 0);
    gtk_widget_set_hexpand(grid, FALSE);
    gtk_widget_set_vexpand(grid, FALSE);

    g_object_set_data(G_OBJECT(grid), "Seconds", GINT_TO_POINTER(0));
    g_object_set_data(G_OBJECT(grid), "Status", GINT_TO_POINTER(0));

    /* recording status label */
    label = gtk_label_new("");  /* initially empty */
    gtk_grid_attach(GTK_GRID(grid), label, RW_COL_TEXT, RW_ROW_TEXT, 1, 1);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
/*    gtk_widget_set_hexpand(label, TRUE); */

    /* recording timestamp label */
    label = gtk_label_new("");  /* initially empty */
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_widget_set_hexpand(label, FALSE);
    vice_gtk3_css_add(label, TIMING_CSS);
    gtk_grid_attach(GTK_GRID(grid), label, RW_COL_TIME, RW_ROW_TIME, 1, 1);

    button = create_stop_button();

    gtk_grid_attach(GTK_GRID(grid), button, RW_COL_BUTTON, RW_ROW_BUTTON, 1, 2);
    gtk_widget_set_halign(button, GTK_ALIGN_END);
    gtk_widget_set_valign(button, GTK_ALIGN_START);
    gtk_widget_set_hexpand(button, TRUE);
    gtk_widget_set_vexpand(button, FALSE);
    gtk_widget_set_sensitive(button, FALSE);
    gtk_widget_set_no_show_all(button, TRUE);
    gtk_widget_hide(button);    /* initially hidden */
    g_signal_connect(button, "clicked", G_CALLBACK(on_stop_clicked),
           (gpointer)grid);
    return grid;
}


/** \brief  Set recording status
 *
 * Sets the type of recording and the status (ie 'Recording $rec-type or
 * 'Recording stopped).
 *
 * \note    Somehow getting the recording state of events doesn't work yet,
 *          so that currently displays 'inactive'.
 *
 * \param[in,out]   widget  statusbar recording status widget
 * \param[in]       status  recording status (boolean)
 */
void statusbar_recording_widget_set_recording_status(GtkWidget *widget,
                                                     int type)
{
    GtkWidget *label;
    gchar buffer[256];
    GtkWidget *button;
    int status = (type == UI_RECORDING_STATUS_NONE) ? 0 : 1;

    DBG(("statusbar_recording_widget_set_recording_status status:%d", status));

    if (timeout_id > 0) {
        g_source_remove(timeout_id);
        timeout_id = 0;
    }

    g_object_set_data(G_OBJECT(widget), "Status", GINT_TO_POINTER(status));
    if (status == 0) {
        g_object_set_data(G_OBJECT(widget), "Seconds", GINT_TO_POINTER(0));
    }
#if 0
    DBG(("event_record_active():%d sound_is_recording():%d screenshot_is_recording():%d",
         event_record_active(), sound_is_recording(), screenshot_is_recording()));
    /* determine recording type */
    if (event_record_active()) {
        type = RW_TYPE_EVENTS;
    } else if (sound_is_recording() && !screenshot_is_recording()) {
        /* FIXME: sound_is_recording() returns false when using the UI, it uses
         *        a variable that only gets set when using command-line options.
         *        Triggering the recording from the UI doesn't set the variable
         *        but does set the related resource, which I use in the function
         *        used to update the UI display.
         *
         *        So this needs some serious refactoring.
         */
        type = RW_TYPE_AUDIO;
    } else if (screenshot_is_recording()) {
        type = RW_TYPE_VIDEO;
    }
    if ((type != RW_TYPE_NONE) && (type != RW_TYPE_EVENTS) && status) {
        g_timeout_add_seconds(1, update_timer, (gpointer)widget);
    }
#endif

    DBG(("statusbar_recording_widget_set_recording_status type:%d", type));

    switch (type) {
        case UI_RECORDING_STATUS_NONE:
            statusbar_recording_widget_hide_all(GTK_WIDGET(widget), HIDE_ALL_TIMEOUT);
            break;
        case UI_RECORDING_STATUS_EVENTS:
            break;
        case UI_RECORDING_STATUS_AUDIO:
        case UI_RECORDING_STATUS_VIDEO:
            g_timeout_add_seconds(1, update_timer, (gpointer)widget);
            break;
    }

    /* update recording status text */
    label = gtk_grid_get_child_at(GTK_GRID(widget), RW_COL_TEXT, RW_ROW_TEXT);

    g_snprintf(buffer, sizeof(buffer), "Recording %s ...", rec_types[type]);
    gtk_label_set_text(GTK_LABEL(label), buffer);

    /* enable/disable STOP button based on the \a status variable */
    button = gtk_grid_get_child_at(
            GTK_GRID(widget), RW_COL_BUTTON, RW_ROW_BUTTON);
    gtk_widget_set_tooltip_text(button, "Stop recording");
    gtk_widget_set_sensitive(button, status);
    gtk_widget_show(button);
}


/** \brief  Update recording/playback time display
 *
 * \param[in,out]   widget  recording widget
 * \param[in]       current current time in seconds
 * \param[in]       total   total time in seconds
 *
 * \note    \a total only makes sense when replaying events.
 *          I could make the time display '--:--' when \a total is 0, but it
 *          it possible, though unlikely, we're replaying a sequence of events
 *          which take less than a second.
 */
void statusbar_recording_widget_set_time(GtkWidget *widget,
                                         unsigned int current,
                                         unsigned int total)
{
    GtkWidget *status;
    GtkWidget *time;
    int type;
    gchar buffer[256];
    const char *dev = NULL;

    resources_get_string("SoundRecordDeviceName", &dev);
    time = gtk_grid_get_child_at(GTK_GRID(widget), RW_COL_TIME, RW_ROW_TIME);
    if (total > 0) {
        g_snprintf(buffer, sizeof(buffer), "%02u:%02u/%02u:%02u",
                current / 60, current % 60, total / 60, total % 60);
    } else {
        g_snprintf(buffer, sizeof(buffer), "%02u:%02u",
                current / 60, current % 60);
    }
    gtk_label_set_text(GTK_LABEL(time), buffer);


    /* FIXME: Determining the recording status/type is currently very flaky.
     *        When triggering a recording from the UI the variables that should
     *        indicate the recording type aren't set yet. So for now we have to
     *        check them in this timer callback. Also the sound recording status
     *        function is broken in src/sound.c.
     */
    status = gtk_grid_get_child_at(GTK_GRID(widget), RW_COL_TEXT, RW_ROW_TEXT);
    /* determine recording type */
    if (event_record_active()) {
        type = UI_RECORDING_STATUS_EVENTS;
    } else if ((dev != NULL && *dev != '\0') && !screenshot_is_recording()) {
        type = UI_RECORDING_STATUS_AUDIO;
    } else {
        type = UI_RECORDING_STATUS_VIDEO;
    }
    g_snprintf(buffer, sizeof(buffer), "Recording %s ...", rec_types[type]);
    gtk_label_set_text(GTK_LABEL(status), buffer);
}



/** \brief  Set event playback status
 *
 * \param[in,out]   widget  statusbar recording status widget
 * \param[in]       version VICE version major.minor string during playback,
 *                          NULL when playback is done
 */
void statusbar_recording_widget_set_event_playback(GtkWidget *widget,
                                                   char *version)
{
    GtkWidget *label;

    label = gtk_grid_get_child_at(GTK_GRID(widget), RW_COL_TEXT, RW_ROW_TEXT);
    if (version != NULL && *version != '\0') {
        /* got VICE version string, seems like we're still playing back events */
        gtk_label_set_text(GTK_LABEL(label), "Playing back events ...");
    } else {
        /* look like we're done (this API really needs improving) */
        gtk_label_set_text(GTK_LABEL(label), "Event playback done.");
    }
}


/** \brief  Timer callback for the hide_all() method
 *
 * \param[in,out]   data    recording widget
 *
 * \return  FALSE to delete the timer source
 */
static gboolean hide_all_timer_callback(gpointer data)
{
    GtkWidget *widget = data;
    GtkWidget *label;
    GtkWidget *button;

    /* clear recording status label */
    label = gtk_grid_get_child_at(GTK_GRID(widget), RW_COL_TEXT, RW_ROW_TEXT);
    gtk_label_set_text(GTK_LABEL(label), "");

    /* clear time display label */
    label = gtk_grid_get_child_at(GTK_GRID(widget), RW_COL_TIME, RW_ROW_TIME);
    gtk_label_set_text(GTK_LABEL(label), "");

    /* hide button */
    button = gtk_grid_get_child_at(
            GTK_GRID(widget), RW_COL_BUTTON, RW_ROW_BUTTON);
    gtk_widget_hide(button);

    /* delete timer source */
    return FALSE;
}


/** \brief  Clear the labels and hide the button after \a timeout seconds
 *
 * Use a \a timeout of 0 to immediately clear the labels and hide the button,
 * skipping timer setup.
 *
 * \param[in,out]   widget  recording widget
 * \param[in]       timeout timeout in seconds
 */
void statusbar_recording_widget_hide_all(GtkWidget *widget, guint timeout)
{
    /* remove previous timeout */
    if (timeout_id > 0) {
        g_source_remove(timeout_id);
    }
    timeout_id = g_timeout_add_seconds(timeout,
                                       hide_all_timer_callback,
                                       (gpointer)widget);
}
