/*
 * uimenu.h - Common SDL menu functions.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Based on code by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *
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

#ifndef VICE_UIMENU_H
#define VICE_UIMENU_H

#include "vice.h"
#include "types.h"
#include <stddef.h>
#include <stdbool.h>

#include "uiactions.h"
#include "videoarch.h"

/* not sure these belong here, might have to move them to generic code to share
 * with the same macros for Gtk3 */
#define UNIT_DRIVE_TO_PTR(U, D) (vice_int_to_ptr(((U) << 8) | ((D) & 0xff)))
#define UNIT_FROM_PTR(P)        ((vice_ptr_to_int(P) >> 8) & 0xff)
#define DRIVE_FROM_PTR(P)       (vice_ptr_to_int(P) & 0xff)

extern int sdl_menu_state;
extern int sdl_pause_state;

typedef void* ui_callback_data_t;
typedef const char *(*ui_callback_t)(int activated, ui_callback_data_t param);

typedef enum {
    /* Text item (no operation): if data == 1 text colors are inverted */
    MENU_ENTRY_TEXT = 0,

    /* Resource toggle: no UI needed, callback is used */
    MENU_ENTRY_RESOURCE_TOGGLE,

    /* Resource radio: no UI needed, callback is used, data is the resource value */
    MENU_ENTRY_RESOURCE_RADIO,

    /* Resource int: needs UI, callback is used */
    MENU_ENTRY_RESOURCE_INT,

    /* Resource string: needs UI, callback is used */
    MENU_ENTRY_RESOURCE_STRING,

    /* Submenu: needs UI, data points to the submenu */
    MENU_ENTRY_SUBMENU,

    /* Dynamic submenu: needs UI, data points to the submenu, hotkeys disabled */
    MENU_ENTRY_DYNAMIC_SUBMENU,

    /* Custom dialog: needs UI */
    MENU_ENTRY_DIALOG,

    /* Other: no UI needed */
    MENU_ENTRY_OTHER,

    /* Other: no UI needed */
    MENU_ENTRY_OTHER_TOGGLE
} ui_menu_entry_type_t;

typedef enum {
    MENU_STATUS_ACTIVE = 0,
    MENU_STATUS_INACTIVE = 1,
    MENU_STATUS_NA = 2
} ui_menu_status_type_t;

typedef struct ui_menu_entry_s {
    char *string;
    ui_menu_entry_type_t type;
    ui_callback_t callback;
    ui_callback_data_t data;
    ui_menu_status_type_t status;

    /* extra members for the UI actions */
    int          action;        /**< UI action ID */
    const char  *resource;      /**< resource name for toggle buttons */
    const char  *activated;     /**< text to return when the item is activated */
    /** \brief   Function to call for custom display values */
    const char *(*displayed)(struct ui_menu_entry_s *item);
    /** \brief  Function to test if radio button should be checked
     *
     * When a radio button submenu's selection is required to print the single,
     * activatable, value and the items are of type `MENU_ENTRY_OTHER_TOGGLE`
     * there's no resource to test so we need a custom callback.
     */
    bool        (*checked)(struct ui_menu_entry_s *item);
} ui_menu_entry_t;

#define SDL_MENU_LIST_END { .string = NULL, .type = MENU_ENTRY_TEXT }

typedef enum {
    MENU_RETVAL_DEFAULT,
    MENU_RETVAL_EXIT_UI
} ui_menu_retval_t;

struct menufont_s {
    uint8_t *font;
    uint16_t *translate;
    int w;
    int h;
};
typedef struct menufont_s menufont_t;

struct menu_draw_s {
    int pitch;
    int offset;
    int max_text_x;
    int max_text_y;
    int extra_x;
    int extra_y;

    uint8_t color_back;     /* current background color index */
    uint8_t color_front;    /* current foreground color index */

    uint8_t color_default_back;
    uint8_t color_default_front;

    uint8_t color_cursor_back;
    uint8_t color_cursor_revers;

    uint8_t color_active_green;
    uint8_t color_inactive_red;

    uint8_t color_active_grey;
    uint8_t color_inactive_grey;
    uint8_t color_disabled_grey;
};
typedef struct menu_draw_s menu_draw_t;

#define MENU_FIRST_Y 2
#define MENU_FIRST_X 0

typedef enum {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_UP,
    MENU_ACTION_DOWN,
    MENU_ACTION_LEFT,
    MENU_ACTION_RIGHT,
    MENU_ACTION_SELECT,
    MENU_ACTION_CANCEL,
    MENU_ACTION_EXIT,
    MENU_ACTION_MAP,
    MENU_ACTION_PAGEUP,
    MENU_ACTION_PAGEDOWN,
    MENU_ACTION_HOME,
    MENU_ACTION_END,
    MENU_ACTION_NUM,
    MENU_ACTION_NONE_RELEASE,
    MENU_ACTION_UP_RELEASE,
    MENU_ACTION_DOWN_RELEASE,
    MENU_ACTION_LEFT_RELEASE,
    MENU_ACTION_RIGHT_RELEASE,
    MENU_ACTION_SELECT_RELEASE,
    MENU_ACTION_CANCEL_RELEASE,
    MENU_ACTION_EXIT_RELEASE,
    MENU_ACTION_MAP_RELEASE,
    MENU_ACTION_PAGEUP_RELEASE,
    MENU_ACTION_PAGEDOWN_RELEASE,
    MENU_ACTION_HOME_RELEASE,
    MENU_ACTION_END_RELEASE,
} ui_menu_action_t;

void sdl_ui_set_main_menu(const ui_menu_entry_t *menu);
ui_menu_entry_t *sdl_ui_get_main_menu(void);
menu_draw_t *sdl_ui_get_menu_param(void);
menufont_t *sdl_ui_get_menu_font(void);
extern void (*sdl_ui_set_menu_params)(int index, menu_draw_t *menu_draw);

uint8_t *sdl_ui_get_draw_buffer(void);
void sdl_ui_activate_pre_action(void);
void sdl_ui_activate_post_action(void);
void sdl_ui_init_draw_params(video_canvas_t *canvas);
void sdl_ui_reverse_colors(void);
void sdl_ui_refresh(void);
ui_menu_action_t sdl_ui_menu_poll_input(void);
void sdl_ui_display_cursor(int pos, int old_pos);
void sdl_ui_putchar(uint8_t c, int pos_x, int pos_y);
int sdl_ui_print(const char *text, int pos_x, int pos_y);
int sdl_ui_print_center(const char *text, int pos_y);
int sdl_ui_print_eol(int pos_x, int pos_y);
int sdl_ui_display_title(const char *title);
void sdl_ui_clear(void);
void sdl_ui_activate(void);
ui_menu_retval_t sdl_ui_external_menu_activate(ui_menu_entry_t *item);
char* sdl_ui_readline(const char* previous, int pos_x, int pos_y);
char* sdl_ui_text_input_dialog(const char* title, const char* previous);
int sdl_ui_slider_input_dialog(const char* title, const int cur, const int min, const int max);
void sdl_ui_invert_char(int pos_x, int pos_y);
void sdl_ui_scroll_screen_up(void);

int sdl_ui_set_cursor_colors(void);
int sdl_ui_reset_cursor_colors(uint8_t color);

int sdl_ui_set_tickmark_colors(int state);
int sdl_ui_reset_tickmark_colors(uint8_t color);
int sdl_ui_set_toggle_colors(int state);
int sdl_ui_set_default_colors(void);
void sdl_ui_menu_shutdown(void);

const char *sdl_ui_menu_video_slider_helper(int activated, ui_callback_data_t param, const char *resource_name, const int min, const int max);

void sdl_ui_create_draw_buffer_backup(void);
void sdl_ui_restore_draw_buffer_backup(void);
void sdl_ui_destroy_draw_buffer_backup(void);

void sdl_ui_menu_item_activate_by_action  (int action);
void sdl_ui_menu_item_set_status_by_action(int action, ui_menu_status_type_t status);

/* Public UI action handlers */
void sdl_ui_activate_item_action  (ui_action_map_t *map);
void sdl_ui_toggle_resource_action(ui_action_map_t *map);

#endif
