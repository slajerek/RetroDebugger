/*
 * keyboard.h - Common keyboard emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Tibor Biczo <crown@mail.matav.hu>
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

#ifndef VICE_KEYBOARD_H
#define VICE_KEYBOARD_H

#include "vicetypes.h"

#ifndef COMMON_KBD
#include "kbd.h"
#endif

/* Maximum of keyboard array (CBM-II values
 * (8 for C64/VIC20, 10 for PET, 11 for C128; we need max).  */
#define KBD_ROWS    16

/* (This is actually the same for all the machines.) */
/* (All have 8, except CBM-II that has 6) */
#define KBD_COLS    8

/* joystick port attached keypad */
#define KBD_JOY_KEYPAD_ROWS 5
#define KDB_JOY_KEYPAD_COLS 4

/* index to select the current keymap ("KeymapIndex") */
#define KBD_INDEX_SYM     0
#define KBD_INDEX_POS     1
#define KBD_INDEX_USERSYM 2
#define KBD_INDEX_USERPOS 3
#define KBD_INDEX_LAST    3
#define KBD_INDEX_NUM     4

/* the mapping of the host ("KeyboardMapping")
   (keep in sync with table in keyboard.c) */
#define KBD_MAPPING_US    0     /* "" (us mapping) */
#define KBD_MAPPING_UK    1     /* "uk" */
#define KBD_MAPPING_DE    2     /* "de" */
#define KBD_MAPPING_DA    3     /* "da" */
#define KBD_MAPPING_NO    4     /* "no" */
#define KBD_MAPPING_FI    5     /* "fi" */
#define KBD_MAPPING_IT    6     /* "it" */
#define KBD_MAPPING_LAST  6
#define KBD_MAPPING_NUM   7
extern int keyboard_get_num_mappings(void);

/* mapping info for GUIs */
typedef struct {
    char *name;
    int mapping;
    char *mapping_name;
} mapping_info_t;

extern mapping_info_t *keyboard_get_info_list(void);

struct snapshot_s;

extern void keyboard_init(void);
extern void keyboard_shutdown(void);
extern void keyboard_set_keyarr(int row, int col, int value);
extern void keyboard_set_keyarr_any(int row, int col, int value);
extern void keyboard_clear_keymatrix(void);
extern void keyboard_event_playback(CLOCK offset, void *data);
extern void keyboard_restore_event_playback(CLOCK offset, void *data);
extern int keyboard_snapshot_write_module(struct snapshot_s *s);
extern int keyboard_snapshot_read_module(struct snapshot_s *s);
extern void keyboard_event_delayed_playback(void *data);
extern void keyboard_register_delay(unsigned int delay);
extern void keyboard_register_clear(void);
extern void keyboard_set_map_any(signed long sym, int row, int col, int shift);
extern void keyboard_set_unmap_any(signed long sym);

extern int keyboard_set_keymap_index(int vak, void *param);
extern int keyboard_set_keymap_file(const char *val, void *param);
extern int keyboard_keymap_dump(const char *filename);

extern int keyboard_key_pressed(signed long key);
extern int keyboard_key_released(signed long key);
extern void keyboard_key_clear(void);

typedef void (*key_ctrl_column4080_func_t)(void);
extern void keyboard_register_column4080_key(key_ctrl_column4080_func_t func);

typedef void (*key_ctrl_caps_func_t)(void);
extern void keyboard_register_caps_key(key_ctrl_caps_func_t func);

typedef void (*key_joy_keypad_func_t)(int row, int col, int pressed);
extern void keyboard_register_joy_keypad(key_joy_keypad_func_t func);

typedef void (*keyboard_machine_func_t)(int *);
extern void keyboard_register_machine(keyboard_machine_func_t func);

extern void keyboard_register_machine(keyboard_machine_func_t func);
extern void keyboard_alternative_set(int alternative);

/* These ugly externs will go away sooner or later.  */
extern int keyarr[KBD_ROWS];
extern int rev_keyarr[KBD_COLS];
extern int keyboard_shiftlock;

#ifdef COMMON_KBD
extern int keyboard_resources_init(void);
extern int keyboard_cmdline_options_init(void);
#endif


/// c64d
void c64d_keyboard_keymap_clear();
extern void keyboard_parse_set_pos_row(signed long sym, int row, int col,
									   int shift);
extern int keyboard_parse_set_neg_row(signed long sym, int row, int col);
///

#endif
