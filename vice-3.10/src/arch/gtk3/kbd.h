/** \file   kbd.h
 * \brief   Native GTK3 specfic keyboard driver - header
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README file for copyright notice.
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

#ifndef VICE_KBD_H
#define VICE_KBD_H

#include <gtk/gtk.h>


/** \brief  Gtk3 keyboard shortcut to be used without Gtk3's accelerators
 *
 * In Gtk3 there is no normal method to create a keyboard shortcut without
 * it being connected to a G(tk)MenuItem. This object "solves" this problem
 * by having the kbd_event_handler() function scan a list of these objects
 * and trigger a callback when a specific key press matches.
 */
typedef struct kbd_gtk3_hotkey_s {
    guint code;                 /**< key code */
    guint mask;                 /**< key mask bits */
    void (*callback)(void);     /**< function to call when the key matches */
} kbd_gtk3_hotkey_t;


void kbd_arch_init(void);
void kbd_arch_shutdown(void);
void kbd_initialize_numpad_joykeys(int *joykeys);
void kbd_connect_handlers(GtkWidget *widget, void *data);

/** \brief  Prefix for the Gtk3 port keymap files
 */
#define KBD_PORT_PREFIX "gtk3"

/* add more function prototypes as needed below */

signed long kbd_arch_keyname_to_keynum(char *keyname);
const char *kbd_arch_keynum_to_keyname(signed long keynum);

/** \brief  Only here for c1541-stubs.c, not used in the Gtk3 UI */
/**
 *  \return NULL
 */
const char *kbd_get_menu_keyname(void);

/** \brief  Fixup the GTK keycodes so they are the same on all OSs */
/**
 *  \return fixed keyval (also modifies the value in the event)
 */
guint kbd_fix_keyval(GdkEvent *event);

#endif
