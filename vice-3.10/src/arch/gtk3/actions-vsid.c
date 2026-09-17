/** \file   actions-vsid.c
 * \brief   UI action implementations for VSID
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * \note    This file cannot be used from ui.c since that causes massive
 *          linker errors due to the way vsid is linked. Currently registering
 *          the actions happens in vsidui.c, which magically does work.
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

/* Resources altered by this file:
 * $VICERES PSIDKeepEnv     vsid
 * $VICERES Speed           vsid
 */

#include "vice.h"

#include <gtk/gtk.h>
#include <stddef.h>
#include <stdbool.h>

#include "debug_gtk3.h"
#include "hotkeys.h"
#include "machine.h"
#include "psid.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uisidattach.h"
#include "vsidcontrolwidget.h"
#include "vsidmixerwidget.h"
#include "vsidplaylistwidget.h"
#include "vsidstate.h"

#include "actions-vsid.h"


/** \brief  Emulation speed during fast forward
 */
#define FFWD_SPEED  500


/** \brief  Trigger play of current tune
 *
 * Helper to (re)start playback of current selected subtune
 */
static void play_current_tune(void)
{
    vsid_state_t *state;
    int           current;
    const char   *filename;

    state    = vsid_state_lock();
    current  = state->tune_current;
    filename = state->psid_filename;

    if (state->tune_current < 0) {
        if (state->tune_previous < 0) {
            state->tune_previous = state->tune_default;
        }
        current = state->tune_current = state->tune_default;
    }
#ifdef HAVE_DEBUG_GTK3UI
    int count = state->tune_count;
    int def = state->tune_default;
    int prev = state->tune_previous;
#endif
    vsid_state_unlock();

    /* reload unloaded PSID file if loaded before */
    if (filename != NULL) {
        psid_load_file(filename);
    }

    resources_set_int("Speed", 100);
    debug_gtk3("current: %d, previous: %d  (total: %d, default: %d)",
                current, prev, count, def);
    debug_gtk3("calling psid_init_driver().");
    psid_init_driver();
    debug_gtk3("calling machine_play_psid(%d).", current);
    machine_play_psid(current);
    debug_gtk3("calling machine_trigger_reset(SOFT).");
    machine_trigger_reset(MACHINE_RESET_MODE_RESET_CPU);
    ui_pause_disable();
    vsid_control_widget_set_state(VSID_PLAYING);
}

/** \brief  Play subtune
 *
 * \param[in]   tune    subtune number
 */
static void play_subtune(int tune)
{
    vsid_state_t *state = vsid_state_lock();
    int count = state->tune_count;

    if (tune < 1) {
        tune = 1;
    } else if (tune > count) {
        tune = count;
    }
    if (state->tune_current > 0 && state->tune_current <= state->tune_count) {
        vsid_state_set_tune_played_unlocked(state->tune_current);
    }
    state->tune_previous = state->tune_current;
    state->tune_current = tune;
#ifdef DEBUG
    vsid_state_print_tunes_played_unlocked();
#endif
    vsid_state_unlock();

    play_current_tune();
}

/** \brief  Show PSID load dialog
 *
 * \param[in]   self    action map
 */
static void psid_load_action(ui_action_map_t *self)
{
    uisidattach_show_dialog();
}

/** \brief  Toggle override of PSID file settings
 *
 * \param[in]   self    action map
 */
static void psid_override_toggle_action(ui_action_map_t *self)
{
    int enabled = 0;

    resources_get_int("PSIDKeepEnv", &enabled);
    enabled = !enabled;
    resources_set_int("PSIDKeepEnv", enabled);
    vhk_gtk_set_check_item_blocked_by_action(ACTION_PSID_OVERRIDE_TOGGLE, enabled);
}

/** \brief  Start playback
 *
 * \param[in]   self    action map
 */
static void psid_play_action(ui_action_map_t *self)
{
    vsid_state_t *state;
    int           current;
    const char   *filename;

    state    = vsid_state_lock();
    current  = state->tune_current;
    filename = state->psid_filename;

    if (current < 0) {
        /* restart previous tune if stopped before */
        if (state->tune_previous >= 1) {
            current = state->tune_current = state->tune_previous;
        } else {
            /* haven't stopped before resuming, use default tune */
            current = state->tune_current = state->tune_default;
        }
        vsid_state_unlock();

        /* reload unloaded PSID file if loaded before */
        if (filename != NULL) {
            psid_load_file(filename);
        }

        psid_init_driver();
        machine_play_psid(current);
        machine_trigger_reset(MACHINE_RESET_MODE_RESET_CPU);
        vsid_mixer_widget_update();
    } else {
        /* return emulation speed back to 100% */
        vsid_state_unlock();
        resources_set_int("Speed", 100);
    }
    ui_pause_disable();
    vsid_control_widget_set_state(VSID_PLAYING);
}

/** \brief  Toggle pause
 *
 * \param[in]   self    action map
 */
static void psid_pause_action(ui_action_map_t *self)
{
    ui_pause_toggle();
    vsid_control_widget_set_state(ui_pause_active() ? VSID_PAUSED : VSID_PLAYING);
}

/** \brief  Stop playback
 *
 * \param[in]   self    action map
 */
static void psid_stop_action(ui_action_map_t *self)
{
    vsid_state_t *state = vsid_state_lock();

    /* remember which tune we were playing so we can resume later */
    state->tune_previous = state->tune_current;
    state->tune_current = -1;
    vsid_state_unlock();

    psid_init_driver();
    machine_play_psid(-1);
    machine_trigger_reset(MACHINE_RESET_MODE_RESET_CPU);
    vsid_control_widget_set_state(VSID_STOPPED);
}

/** \brief  Toggle fast forward
 *
 * \param[in]   self    action map
 */
static void psid_ffwd_action(ui_action_map_t *self)
{
    int speed = 0;

    resources_get_int("Speed", &speed);
    if (speed == 100) {
        resources_set_int("Speed", FFWD_SPEED);
        vsid_control_widget_set_state(VSID_FORWARDING);
    } else {
        resources_set_int("Speed", 100);
        vsid_control_widget_set_state(VSID_PLAYING);
    }
    ui_pause_disable();
}

/** \brief  Play next subtune
 *
 * \param[in]   self    action map
 *
 * \note   Wraps around to the first tune if the current tune is the last.
 */
static void psid_subtune_next_action(ui_action_map_t *self)
{
    vsid_state_t *state = vsid_state_lock();

    if (state->tune_current < 0) {
        state->tune_current = state->tune_previous;
    } else if (state->tune_current > 0 && state->tune_current < 256) {
        /* mark subtune played */
        int t = state->tune_current;
        state->tunes_played[(t - 1) >> 3] |= (1 << ((t - 1) & 7));
        /* we have the lock, so we can call this: */
#ifdef DEBUG
        vsid_state_print_tunes_played_unlocked();
#endif
    }

    if (state->tune_current >= state->tune_count || state->tune_current < 1) {
        state->tune_current = 1;
    } else {
        state->tune_current++;
    }
    vsid_state_unlock();
    play_current_tune();
}

/** \brief  Play previous subtune
 *
 * \param[in]   self    action map
 *
 * \note   Wraps around to the last tune if the current tune is the first.
 */
static void psid_subtune_previous_action(ui_action_map_t *self)
{
    vsid_state_t *state = vsid_state_lock();

    if (state->tune_current < 0) {
        state->tune_current = state->tune_previous;
    }

    if (state->tune_current <= 1) {
        state->tune_current = state->tune_count;
    } else {
        state->tune_current--;
    }
    vsid_state_unlock();
    ui_pause_disable();
    play_current_tune();
}

/** \brief  Play a subtune
 *
 * \param[in]   self    action map
 */
static void psid_subtune_action(ui_action_map_t *self)
{
    play_subtune(vice_ptr_to_int(self->data));
}

/** \brief  Toggle SID tune looping
 *
 * \param[in]   self    action map
 */
static void psid_loop_toggle_action(ui_action_map_t *self)
{
    gboolean enabled = vsid_control_widget_get_repeat();

    vsid_control_widget_set_repeat(!enabled);
}

/** \brief  Call a VSID playlist function
 *
 * \param[in]   self    action map
 */
static void psid_playlist_action(ui_action_map_t *self)
{
    /* the action system makes sure we're on the UI thread, so we can simply
     * call the function passed */
    void (*func)(void) = self->data;
    func();
}


/** \brief  List of VSID-specific actions */
static const ui_action_map_t vsid_actions[] = {
    {   .action  = ACTION_PSID_LOAD,
        .handler = psid_load_action,
        .blocks  = true,
        .dialog  = true
    },
    {   .action   = ACTION_PSID_OVERRIDE_TOGGLE,
        .handler  = psid_override_toggle_action,
        .uithread = true,
    },
    {   .action   = ACTION_PSID_PLAY,
        .handler  = psid_play_action,
        .uithread = true,   /* if we want to toggle the button's pressed state */
    },
    {   .action   = ACTION_PSID_PAUSE,
        .handler  = psid_pause_action,
        .uithread = true
    },
    {   .action   = ACTION_PSID_STOP,
        .handler  = psid_stop_action,
        .uithread = true
    },
    {   .action   = ACTION_PSID_FFWD,
        .handler  = psid_ffwd_action,
        .uithread = true    /* if we want to toggle the button's pressed state */
    },
    {   .action   = ACTION_PSID_SUBTUNE_NEXT,
        .handler  = psid_subtune_next_action,
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_PREVIOUS,
        .handler  = psid_subtune_previous_action,
        .uithread = true
    },
    /* {{{ actions for subtune 1-30 */
    {   .action   = ACTION_PSID_SUBTUNE_1,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(1),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_2,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(2),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_3,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(3),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_4,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(4),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_5,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(5),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_6,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(6),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_7,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(7),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_8,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(8),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_9,
        .handler  = psid_subtune_action,
        .data    = vice_int_to_ptr(9),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_10,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(10),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_11,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(11),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_12,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(12),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_13,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(13),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_14,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(14),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_15,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(15),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_16,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(16),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_17,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(17),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_18,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(18),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_19,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(19),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_20,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(20),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_21,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(21),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_22,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(22),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_23,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(23),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_24,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(24),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_25,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(25),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_26,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(26),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_27,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(27),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_28,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(28),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_29,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(29),
        .uithread = true
    },
    {   .action   = ACTION_PSID_SUBTUNE_30,
        .handler  = psid_subtune_action,
        .data     = vice_int_to_ptr(30),
        .uithread = true
    },
    /* }}} */

    {   .action   = ACTION_PSID_LOOP_TOGGLE,
        .handler  = psid_loop_toggle_action,
        .uithread = true
    },
    {   .action   = ACTION_PSID_PLAYLIST_FIRST,
        .handler  = psid_playlist_action,
        .data     = (void*)vsid_playlist_first,
        .uithread = true
    },
    {   .action   = ACTION_PSID_PLAYLIST_PREVIOUS,
        .handler  = psid_playlist_action,
        .data     = (void*)vsid_playlist_previous,
        .uithread = true
    },
    {   .action   = ACTION_PSID_PLAYLIST_NEXT,
        .handler  = psid_playlist_action,
        .data     = (void*)vsid_playlist_next,
        .uithread = true
    },
    {   .action   = ACTION_PSID_PLAYLIST_LAST,
        .handler  = psid_playlist_action,
        .data     = (void*)vsid_playlist_last,
        .uithread = true
    },
    {   .action   = ACTION_PSID_PLAYLIST_ADD,
        .handler  = psid_playlist_action,
        .data     = (void*)vsid_playlist_add,
        .uithread = true,
        .dialog   = true,
        .blocks   = true
    },
    {   .action   = ACTION_PSID_PLAYLIST_LOAD,
        .handler  = psid_playlist_action,
        .data     = (void*)vsid_playlist_load,
        .uithread = true,
        .dialog   = true,
        .blocks   = true
    },
    {   .action   = ACTION_PSID_PLAYLIST_SAVE,
        .handler  = psid_playlist_action,
        .data     = (void*)vsid_playlist_save,
        .uithread = true,
        .dialog   = true,
        .blocks   = true
    },
    {   .action   = ACTION_PSID_PLAYLIST_CLEAR,
        .handler  = psid_playlist_action,
        .data     = (void*)vsid_playlist_clear,
        .uithread = true,
        .dialog   = true,
        .blocks   = true
    },

    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register VSID-specific actions */
void actions_vsid_register(void)
{
    ui_actions_register(vsid_actions);
}


/** \brief  Set initial UI element states for VSID
 */
void actions_vsid_setup_ui(void)
{
    int enabled = 0;

    /* Override PSID settings */
    resources_get_int("PSIDKeepEnv", &enabled);
    vhk_gtk_set_check_item_blocked_by_action(ACTION_PSID_OVERRIDE_TOGGLE, enabled);
}
