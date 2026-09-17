/** \file   vsidcontrolwidget.c
 * \brief   GTK3 control widget for VSID
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * Icons used by this file:
 *
 * $VICEICON    actions/media-skip-backward
 * $VICEICON    actions/media-playback-start
 * $VICEICON    actions/media-playback-pause
 * $VICEICON    actions/media-playback-stop
 * $VICEICON    actions/media-seek-forward
 * $VICEICON    actions/media-skip-forward
 * $VICEICON    actions/media-eject
 * $VICEICON    actions/media-record
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

#include "c64mem.h"
#include "debug.h"
#include "machine.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "psid.h"
#include "ui.h"
#include "uiactions.h"
#include "vice_gtk3.h"
#include "vsidstate.h"
#include "vsidtuneinfowidget.h"
#include "vsync.h"

#include "vsidcontrolwidget.h"


/** \brief  Emulation speed during fast forward
 */
#define FFWD_SPEED  500

/** \brief  Columns of the buttons
 */
enum {
    COL_SUBTUNE_PREVIOUS,   /**< psid-subtune-previous */
    COL_PLAY,               /**< psid-play */
    COL_PAUSE,              /**< psid-pause */
    COL_STOP,               /**< psid-stop */
    COL_FFWD,               /**< psid-ffwd */
    COL_NEXT,               /**< psid-subtune-next */
    COL_EJECT,              /**< psid-load */
    COL_REPEAT,             /**< psid-loop-toggle */

    NUM_BUTTONS             /**< number of buttons */
};


/* Button types */
enum {
    BUTTON_PUSH,    /**< normal push button (GtkButton) */
    BUTTON_TOGGLE   /**< toggle button (GtkToggleButton) */
};



/** \brief  Object containing icon, action ID and tooltip
 */
typedef struct vsid_ctrl_button_s {
    const char *icon_name;  /**< icon name */
    int action;             /**< UI action ID */
    int button_type;        /**< button type */
    const char *tooltip;    /**< tool tip */
} vsid_ctrl_button_t;


/** \brief  Signal handler IDs for the buttons */
static gulong button_handler_ids[NUM_BUTTONS];

/** \brief  Push/toggle button widget references */
static GtkWidget *button_widgets[NUM_BUTTONS];

/** \brief  Progress bar */
static GtkWidget *progress = NULL;

/** \brief  Internal repeat state
 *
 * Since we toggle this state via a UI action that can be triggered both from
 * the toggle button and a hotkey we keep track of the state here.
 */
static gboolean repeat = FALSE;



/** \brief  Handler for 'clicked' event of push buttons
 *
 * Trigger UI \a action.
 *
 * \param[in]   button  push button
 * \param[in]   action  action ID
 */
static void push_button_callback(GtkWidget *button, gpointer action)
{
    int id = GPOINTER_TO_INT(action);

    if (id > 0) {
        debug_gtk3("calling action '%s' (%d).", ui_action_get_name(id), id);
        ui_action_trigger(id);
    }
}


/** \brief  Handler for 'toggled' event of toggle buttons
 *
 * Trigger UI \a action.
 *
 * \param[in]   button  push button
 * \param[in]   action  action ID
 */
static void toggle_button_callback(GtkWidget *button, gpointer action)
{

    int id = GPOINTER_TO_INT(action);
    if (id > 0) {
        debug_gtk3("calling action '%s' (%d).", ui_action_get_name(id), id);
        ui_action_trigger(id);
    }
}


/** \brief  List of media control buttons
 *
 * \note    Keep this array and the COL_* enum synchronized!
 */
static const vsid_ctrl_button_t buttons[] = {
    { "media-skip-backward",
      ACTION_PSID_SUBTUNE_PREVIOUS,
      BUTTON_PUSH,
      "Select previous subtune" },
    { "media-playback-start",
      ACTION_PSID_PLAY,
      BUTTON_TOGGLE,
      "Play tune" },
    { "media-playback-pause",
      ACTION_PSID_PAUSE,
      BUTTON_TOGGLE,
      "Pause playback" },
    { "media-playback-stop",
      ACTION_PSID_STOP,
      BUTTON_PUSH,
      "Stop playback" },
    { "media-seek-forward",
      ACTION_PSID_FFWD,
      BUTTON_TOGGLE,
      "Fast forward" },
    { "media-skip-forward",
      ACTION_PSID_SUBTUNE_NEXT,
      BUTTON_PUSH,
      "Select next subtune" },   /* select next tune */
    { "media-eject",
      ACTION_PSID_LOAD,
      BUTTON_PUSH,
      "Load PSID file" },   /* active file-open dialog */
    { "media-playlist-repeat",
      ACTION_PSID_LOOP_TOGGLE,
      BUTTON_TOGGLE,
      "Loop current subtune" },

    { NULL, 0, 0, NULL }
};



/** \brief  Create widget with media buttons to control VSID playback
 *
 * \return  GtkGrid
 */
GtkWidget *vsid_control_widget_create(void)
{
    GtkWidget *grid;
    int i;

    grid = vice_gtk3_grid_new_spaced(0, VICE_GTK3_DEFAULT);

    for (i = 0; buttons[i].icon_name != NULL; i++) {
        GtkWidget *button;
        GtkWidget *image;
        gchar buf[1024];
        gulong handler_id;

        g_snprintf(buf, sizeof(buf), "%s-symbolic", buttons[i].icon_name);

        switch (buttons[i].button_type) {
            case BUTTON_PUSH:
                button = gtk_button_new_from_icon_name(buf, GTK_ICON_SIZE_LARGE_TOOLBAR);
                /* always show the image, the button would useless without an image */
                gtk_button_set_always_show_image(GTK_BUTTON(button), TRUE);
                handler_id = g_signal_connect(button,
                                              "clicked",
                                              G_CALLBACK(push_button_callback),
                                              GINT_TO_POINTER(buttons[i].action));
                break;
            case BUTTON_TOGGLE:
                button = gtk_toggle_button_new();
                image = gtk_image_new_from_icon_name(buf, GTK_ICON_SIZE_LARGE_TOOLBAR);
                gtk_container_add(GTK_CONTAINER(button), image);
                handler_id = g_signal_connect(button,
                                              "toggled",
                                              G_CALLBACK(toggle_button_callback),
                                              GINT_TO_POINTER(buttons[i].action));
                break;
            default:
                /* shouldn't get here */
                button = NULL;
                continue;
        }

        button_handler_ids[i] = handler_id;
        button_widgets[i] = button;

        /* don't initialy focus on a button */
        gtk_widget_set_can_focus(button, FALSE);
#if 0
        /* don't grab focus when clicked */
        gtk_widget_set_focus_on_click(button, FALSE);
#endif
        gtk_grid_attach(GTK_GRID(grid), button, i, 0, 1,1);

        if (buttons[i].tooltip != NULL) {
            gtk_widget_set_tooltip_text(button, buttons[i].tooltip);
        }
    }

    /* add progress bar */
    progress = gtk_progress_bar_new();
    gtk_grid_attach(GTK_GRID(grid), progress, 0, 1, i, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set number of tunes
 *
 * \param[in]   n   tune count
 */
void vsid_control_widget_set_tune_count(int n)
{
    vsid_state_t *state = vsid_state_lock();

    state->tune_count = n;
    vsid_state_unlock();
}


/** \brief  Set current tune
 *
 * \param[in]   n   tune number
 */
void vsid_control_widget_set_tune_current(int n)
{
    vsid_state_t *state = vsid_state_lock();

    state->tune_current = n;
    vsid_state_unlock();
}


/** \brief  Set default tune
 *
 * \param[in]   n   tune number
 */
void vsid_control_widget_set_tune_default(int n)
{
    vsid_state_t *state = vsid_state_lock();

    state->tune_default = n;
    vsid_state_unlock();
}


/** \brief  Set tune progress bar value
 *
 * \param[in]   fraction    amount to fill (0.0 - 1.0)
 */
void vsid_control_widget_set_progress(gdouble fraction)
{
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), fraction);
}


/** \brief  Play next tune
 */
void vsid_control_widget_next_tune(void)
{
    ui_action_trigger(ACTION_PSID_SUBTUNE_NEXT);
}


/** \brief  Set repeat of psid
 *
 * Set internal repeat state and toggle button state
 *
 * \param[in]   enabled repeat current psid
 */
void vsid_control_widget_set_repeat(gboolean enabled)
{
    GtkWidget *button = button_widgets[COL_REPEAT];
    gulong handler_id = button_handler_ids[COL_REPEAT];

    repeat = !repeat;
    g_signal_handler_block(button, handler_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), repeat);
    g_signal_handler_unblock(button, handler_id);
}


/** \brief  Get repeat/loop widget state
 *
 * \return  loop state
 */
gboolean vsid_control_widget_get_repeat(void)
{
    return repeat;
}


/** \brief  Update control buttons toggled state
 *
 * \param[in]   state   player state
 */
void vsid_control_widget_set_state(vsid_control_t state)
{
    GtkToggleButton *play;
    GtkToggleButton *pause;
    GtkToggleButton *ffwd;
    gulong play_handler;
    gulong pause_handler;
    gulong ffwd_handler;

    if (state < VSID_STOPPED || state > VSID_FORWARDING) {
        state = VSID_ERROR;
    }

    play = GTK_TOGGLE_BUTTON(button_widgets[COL_PLAY]);
    pause = GTK_TOGGLE_BUTTON(button_widgets[COL_PAUSE]);
    ffwd = GTK_TOGGLE_BUTTON(button_widgets[COL_FFWD]);
    play_handler = button_handler_ids[COL_PLAY];
    pause_handler = button_handler_ids[COL_PAUSE];
    ffwd_handler = button_handler_ids[COL_FFWD];

    /* block all signal handler before toggling buttons */
    g_signal_handler_block(play, play_handler);
    g_signal_handler_block(pause, pause_handler);
    g_signal_handler_block(ffwd, ffwd_handler);

    switch (state) {
        case VSID_PLAYING:
            gtk_toggle_button_set_active(play, TRUE);
            gtk_toggle_button_set_active(pause, FALSE);
            gtk_toggle_button_set_active(ffwd, FALSE);
            break;
        case VSID_PAUSED:
            gtk_toggle_button_set_active(play, FALSE);
            gtk_toggle_button_set_active(pause, TRUE);
            gtk_toggle_button_set_active(ffwd, FALSE);
            break;
        case VSID_STOPPED:
            gtk_toggle_button_set_active(play, FALSE);
            gtk_toggle_button_set_active(pause, FALSE);
            gtk_toggle_button_set_active(ffwd, FALSE);
            break;
        case VSID_FORWARDING:
            gtk_toggle_button_set_active(play, FALSE);
            gtk_toggle_button_set_active(pause, FALSE);
            gtk_toggle_button_set_active(ffwd, TRUE);
            break;
        default:
            gtk_toggle_button_set_active(play, FALSE);
            gtk_toggle_button_set_active(pause, FALSE);
            gtk_toggle_button_set_active(ffwd, FALSE);
            break;
    }

    /* unblock signal handlers */
    g_signal_handler_unblock(play, play_handler);
    g_signal_handler_unblock(pause, pause_handler);
    g_signal_handler_unblock(ffwd, ffwd_handler);
}
