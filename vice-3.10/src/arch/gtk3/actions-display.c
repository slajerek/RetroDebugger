/** \file   actions-display.c
 * \brief   UI action implementations for display-related settings
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

/* The following resources are manipulated here:
 *
 * $VICERES FullscreenDecorations   -vsid
 * $VICERES CrtcFullscreen          xcbm2 xpet
 * $VICERES TEDFullscreen           xplus4
 * $VICERES VDCFullscreen           x128
 * $VICERES VICFullscreen           xvic
 * $VICERES VICIIFullscreen         x64 x64dtv x64sc x128 xcbm5x0 xscpu64
 * $VICERES CrtcShowStatusbar       xcbm2 xpet
 * $VICERES TEDShowStatusbar        xplus4
 * $VICERES VDCShowStatusbar        x128
 * $VICERES VICShowStatusbar        xvic
 * $VICERES VICIIShowStatusbar      x64 x64dtv x64sc x128 xcbm5x0 xscpu64
 */

#include "vice.h"

#include <gtk/gtk.h>
#include <stdbool.h>
#include <stddef.h>

#include "debug_gtk3.h"
#include "hotkeys.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uimenu.h"
#include "uistatusbar.h"
#include "vicii.h"
#include "video.h"
#include "videoarch.h"

#include "actions-display.h"


/** \brief  Toggles fullscreen mode
 *
 * If fullscreen is enabled and there are no window decorations requested for
 * fullscreen mode, the mouse pointer is hidden until fullscreen is disabled.
 *
 * \param[in]   self    action map
 */
static void fullscreen_toggle_action(ui_action_map_t *self)
{
    gboolean enabled = 0;
    gint     index = ui_get_main_window_index();

    if (index != PRIMARY_WINDOW && index != SECONDARY_WINDOW) {
        return;
    }

    /* flip fullscreen mode */
    enabled = !ui_is_fullscreen();
    ui_set_fullscreen_enabled(enabled);
    vhk_gtk_set_check_item_blocked_by_action_for_window(self->action, index, enabled);
    ui_update_fullscreen_decorations();
}

/** \brief Toggles fullscreen window decorations
 *
 * \param[in]   self    action map
 */
static void fullscreen_decorations_toggle_action(ui_action_map_t *self)
{
    gboolean decorations = !ui_fullscreen_has_decorations();

    resources_set_int("FullscreenDecorations", decorations);
    vhk_gtk_set_check_item_blocked_by_action(self->action, decorations);
    ui_update_fullscreen_decorations();
}

/** \brief  Attempt to restore the active window's size to its "natural" size
 *
 * Also unmaximizes and unfullscreens the window.
 *
 * \param[in]   self    action map
 */
static void restore_display_action(ui_action_map_t *self)
{
    GtkWindow *window = ui_get_active_window();

    if (window != NULL) {
        /* disable fullscreen if active */
        if (ui_is_fullscreen()) {
            ui_action_trigger(ACTION_FULLSCREEN_TOGGLE);
        }
        /* unmaximize */
        gtk_window_unmaximize(window);
        /* requesting a 1x1 window forces the window to resize to its natural
         * size, ie the minimal size required to display the window's
         * decorations and contents without wasting space
         */
        gtk_window_resize(window, 1, 1);
    }
}

/** \brief  Toggle status bar visibility for the primary window action
 *
 * \param[in]   self    action map
 */
static void show_statusbar_toggle_action(ui_action_map_t *self)
{
    GtkWidget      *window = ui_get_window_by_index(PRIMARY_WINDOW);
    video_canvas_t *canvas = ui_get_canvas_for_window(PRIMARY_WINDOW);

    if (window != NULL && canvas != NULL ) {
        int         show = 0;
        const char *chip_name = canvas->videoconfig->chip_name;

        resources_get_int_sprintf("%sShowStatusbar", &show, chip_name);
        show = !show;
        resources_set_int_sprintf("%sShowStatusbar", show, chip_name);
        debug_gtk3("%sShowStatusbar => %s.", chip_name, show ? "True" : "False");

        ui_statusbar_set_visible_for_window(window, show);
        /* update menu item's toggled state */
        vhk_gtk_set_check_item_blocked_by_action_for_window(ACTION_SHOW_STATUSBAR_TOGGLE,
                                                            PRIMARY_WINDOW,
                                                            show);
        gtk_window_resize(GTK_WINDOW(window), 1, 1);
    }
}

/** \brief  Toggle status bar visibility for the secondary window action
 *
 * Toggle visibility of the status bar of the VDC window of x128.
 *
 * \param[in]   self    action map
 */
static void show_statusbar_secondary_toggle_action(ui_action_map_t *self)
{
    GtkWidget *window;
    int        show = 0;

    window = ui_get_window_by_index(SECONDARY_WINDOW);
    if (window == NULL) {
        return;
    }
    /* only x128 has a secondary window, so no need to get the chip name from
     * the canvas */
    resources_get_int("VDCShowStatusbar", &show);
    show = !show;
    resources_set_int("VDCShowStatusbar", show);
    ui_statusbar_set_visible_for_window(window, show);
    vhk_gtk_set_check_item_blocked_by_action_for_window(ACTION_SHOW_STATUSBAR_SECONDARY_TOGGLE,
                                                        SECONDARY_WINDOW,
                                                        show);
    gtk_window_resize(GTK_WINDOW(window), 1, 1);
}

/** \brief  Set border mode action
 *
 * Set border mode for current video chip, the \c data member of \a self contains
 * the border mode.
 *
 * \param[in]   self    action map
 */
static void border_mode_action(ui_action_map_t *self)
{
    /* x128's VDC doesn't support border mode, so we can simply use the
     * primary window index here */
    video_canvas_t *canvas = ui_get_canvas_for_window(PRIMARY_WINDOW);
    const char     *chip   = canvas->videoconfig->chip_name;

    resources_set_int_sprintf("%sBorderMode", vice_ptr_to_int(self->data), chip);
}


/** \brief  List of display-related actions */
static const ui_action_map_t display_actions[] = {
    {   .action   = ACTION_FULLSCREEN_TOGGLE,
        .handler  = fullscreen_toggle_action,
        .uithread = true
    },
    {   .action   = ACTION_FULLSCREEN_DECORATIONS_TOGGLE,
        .handler  = fullscreen_decorations_toggle_action,
        .uithread = true
    },
    {   .action   = ACTION_RESTORE_DISPLAY,
        .handler  = restore_display_action,
        .uithread = true
    },
    {   .action   = ACTION_SHOW_STATUSBAR_TOGGLE,
        .handler  = show_statusbar_toggle_action,
        .uithread = true
    },
    {   .action   = ACTION_SHOW_STATUSBAR_SECONDARY_TOGGLE,
        .handler  = show_statusbar_secondary_toggle_action,
        .uithread = true
    },
    UI_ACTION_MAP_TERMINATOR
};

/** \brief  Border mode actions
 *
 * UI action maps for setting border mode.
 * Since all ${CHIP}_${MODE}_BORDER constants are the same for each CHIP, we
 * simply use the constants for VICII here, should that ever change we'll need
 * separate lists for each CHIP.
 */
static const ui_action_map_t border_mode_actions[] = {
    {   .action   = ACTION_BORDER_MODE_NONE,
        .handler  = border_mode_action,
        .data     = vice_int_to_ptr(VICII_NO_BORDERS),
        .uithread = true
    },
    {   .action   = ACTION_BORDER_MODE_NORMAL,
        .handler  = border_mode_action,
        .data     = vice_int_to_ptr(VICII_NORMAL_BORDERS),
        .uithread = true
    },
    {   .action   = ACTION_BORDER_MODE_FULL,
        .handler  = border_mode_action,
        .data     = vice_int_to_ptr(VICII_FULL_BORDERS),
        .uithread = true
    },
    {   .action   = ACTION_BORDER_MODE_DEBUG,
        .handler  = border_mode_action,
        .data     = vice_int_to_ptr(VICII_DEBUG_BORDERS),
        .uithread = true
    },
    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register display-related UI actions */
void actions_display_register(void)
{
    ui_actions_register(display_actions);
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_C64DTV:   /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_PLUS4:    /* fall through */
        case VICE_MACHINE_CBM5x0:   /* fall through */
        case VICE_MACHINE_VIC20:
            ui_actions_register(border_mode_actions);
            break;
        default:
            /* no BorderMode resource for other emulator */
            break;
    }
}


/** \brief  Set correct check button states
 *
 * Set check buttons for fullscreen, fullscreen-decorations and show-statusbar.
 */
void actions_display_setup_ui(void)
{
    video_canvas_t *canvas;
    const char     *chip;
    int             enabled = 0;

    canvas = ui_get_canvas_for_window(PRIMARY_WINDOW);
    chip   = canvas->videoconfig->chip_name;

    /* set fullscreen check button for primary window */
    resources_get_int_sprintf("%sFullscreen", &enabled, chip);
    vhk_gtk_set_check_item_blocked_by_action_for_window(ACTION_FULLSCREEN_TOGGLE,
                                                        PRIMARY_WINDOW,
                                                        (gboolean)enabled);

    /* set fullscreen check button for secondary window (x128 VDC) */
    if (machine_class == VICE_MACHINE_C128) {
        resources_get_int("VDCFullscreen", &enabled);
        vhk_gtk_set_check_item_blocked_by_action_for_window(ACTION_FULLSCREEN_TOGGLE,
                                                            SECONDARY_WINDOW,
                                                            (gboolean)enabled);
    }

    /* fullscreen decorations is currently a single resource for all chips
     * and thus windows */
    resources_get_int("FullscreenDecorations", &enabled);
    vhk_gtk_set_check_item_blocked_by_action(ACTION_FULLSCREEN_DECORATIONS_TOGGLE,
                                             (gboolean)enabled);

    /* set check buttons for show-statusbar */
    resources_get_int_sprintf("%sShowStatusbar", &enabled, chip);
    vhk_gtk_set_check_item_blocked_by_action_for_window(ACTION_SHOW_STATUSBAR_TOGGLE,
                                                        PRIMARY_WINDOW,
                                                        enabled);
    if (machine_class == VICE_MACHINE_C128) {
        resources_get_int("VDCShowStatusbar", &enabled);
        vhk_gtk_set_check_item_blocked_by_action_for_window(ACTION_SHOW_STATUSBAR_SECONDARY_TOGGLE,
                                                            SECONDARY_WINDOW,
                                                            enabled);
    }
}
