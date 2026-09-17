/** \file   kbd.c
 * \brief   Native GTK3 UI keyboard stuff
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 * \author  Michael C. Martin <mcmartin@gmail.com>
 * \author  Oliver Schaertel
 * \author  pottendo <pottendo@gmx.net>
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

/* #define HAVE_DEBUG_GTK3UI */

#include "vice.h"

#include <stdio.h>
#include <gtk/gtk.h>
#include <string.h>
#include "debug_gtk3.h"

#include "hotkeys.h"
#include "lib.h"
#include "log.h"
#include "ui.h"
#include "kbddebugwidget.h"
#include "keyboard.h"
#include "mainlock.h"
#include "uiactions.h"
#include "uimenu.h"
#include "uimedia.h"
#include "uistatusbar.h"

#include "kbd.h"

/** \brief  Enable debug_gtk3() calls for the Gdk scancode/keypad fixes
 */
/* #define DEBUG_GDK_FIXES */


/** \brief  Gdk keyval translation table array indexes
 */
enum {
    KV_FIXED,       /**< target keysym (Unix) */
    KV_ALIAS,       /**< Windows/MacOS keysym alias */
    KV_BIT8,        /**< bit 8 of the scancode */
    KV_ARR_SIZE     /**< size of the array for a single translation entry */
};


/** \brief  Initialize keyboard handling
 */
void kbd_arch_init(void)
{
    /* do NOT call kbd_hotkey_init(), keyboard.c calls this function *after*
     * the UI init stuff is called, allocating the hotkeys array again and thus
     * causing a memory leak
     */
}


/** \brief  Shutdown keyboard handling (NOP)
 */
void kbd_arch_shutdown(void)
{
    /* Also don't call kbd_hotkey_shutdown() here */
}


/** \brief  Get keynum from \a keyname
 *
 * \param[in]   keyname key name as a string
 *
 * \return  keynum or -1 on error
 *
 * \note    You don't see "signed $type-not-plain-char" too often :)
 */
signed long kbd_arch_keyname_to_keynum(char *keyname)
{
    guint sym = gdk_keyval_from_name(keyname);
    /* printf("kbd_arch_keyname_to_keynum %s=%u\n", keyname, sym); */

    if (sym == GDK_KEY_VoidSymbol) {
        return -1;
    }

    return (signed long)sym;
}


/** \brief  Get keyname from keynum
 *
 * \param[in]   keynum  key number
 *
 * \return  key value for \a keynum
 */
const char *kbd_arch_keynum_to_keyname(signed long keynum)
{
    return gdk_keyval_name((guint)keynum);
}


/** \brief  Initialize numpad joystick key mapping
 *
 * \param[out]  joykeys pointer to joykey array
 */
void kbd_initialize_numpad_joykeys(int *joykeys)
{
    joykeys[0] = GDK_KEY_KP_0;
    joykeys[1] = GDK_KEY_KP_1;
    joykeys[2] = GDK_KEY_KP_2;
    joykeys[3] = GDK_KEY_KP_3;
    joykeys[4] = GDK_KEY_KP_4;
    joykeys[5] = GDK_KEY_KP_6;
    joykeys[6] = GDK_KEY_KP_7;
    joykeys[7] = GDK_KEY_KP_8;
    joykeys[8] = GDK_KEY_KP_9;
    joykeys[9] = GDK_KEY_KP_Decimal;
    joykeys[10] = GDK_KEY_KP_Enter;
}

/* since GDK will not make a difference between left and right shift in the
   modifiers reported in the key event, we track the state of the shift keys
   ourself.
*/

/** \brief  Left SHIFT key state
 */
static int shiftl_state = 0;

/** \brief  Right SHIFT key state
 */
static int shiftr_state = 0;

/** \brief  CAPSLOCK key state (is the key currently pressed?)
 */
static int capslock_state = 0;

/** \brief  CAPSLOCK key state (is caps-lock active/locked?)
 */
static int capslock_lock_state = 0;

/** \brief  Set shift flags on key press
 *
 * \param[in]   report  GDK keypress event
 */
static void kbd_fix_shift_press(GdkEvent *report)
{
    switch (report->key.keyval) {
        case GDK_KEY_Shift_L:
            shiftl_state = 1;
            break;
        case GDK_KEY_Shift_R:
            shiftr_state = 1;
            break;
        /* CAUTION: On linux we get regular key down and key up events for the
                    caps-lock key. on macOS we get a key down event when caps
                    become locked, and a key up event when it becomes unlocked */
        case GDK_KEY_Caps_Lock:
#ifdef MACOS_COMPILE
            capslock_lock_state = 1;
#else
            capslock_state = 1;
#endif
            break;
        /* HACK: this will update the capslock state from other keypresses,
                 hopefully making sure its kept in sync at all times */
#ifdef MACOS_COMPILE
        default:
            capslock_lock_state = (report->key.state & GDK_LOCK_MASK) ? 1 : 0;
            break;
#endif
    }
    /* printf("kbd_fix_shift_press   gtk lock state: %d\n", capslock_lock_state); */
}

/** \brief  Unset shift flags on key release
 *
 * \param[in]   report  GDK keypress event
 */
static void kbd_fix_shift_release(GdkEvent *report)
{
    switch (report->key.keyval) {
        case GDK_KEY_Shift_L:
            shiftl_state = 0;
            break;
        case GDK_KEY_Shift_R:
            shiftr_state = 0;
            break;
        /* CAUTION: On linux we get regular key down and key up events for the
                    caps-lock key. on macOS we get a key down event when caps
                    become locked, and a key up event when it becomes unlocked */
        case GDK_KEY_Caps_Lock:
#ifdef MACOS_COMPILE
            capslock_lock_state = 0;
#else
            capslock_lock_state ^= 1;
            capslock_state = 0;
#endif
            break;
        /* HACK: this will update the capslock state from other keypresses,
                 hopefully making sure its kept in sync at all times */
#ifdef MACOS_COMPILE
        default:
            capslock_lock_state = (report->key.state & GDK_LOCK_MASK) ? 1 : 0;
            break;
#endif
    }
    /* printf("kbd_fix_shift_release gtk lock state: %d\n", capslock_lock_state); */
}

/** \brief  Clear shift flags
 */
static void kbd_fix_shift_clear(void)
{
    shiftl_state = 0;
    shiftr_state = 0;
    capslock_state = 0;
}

/** \brief  sync caps lock status with the keyboard emulation
 */
static void kbd_sync_caps_lock(void)
{
#ifdef MACOS_COMPILE
    if (keyboard_get_shiftlock() != capslock_lock_state) {
        keyboard_set_shiftlock(capslock_lock_state);
    }
#else
    GdkDisplay *display = gdk_display_get_default();
    GdkKeymap *keymap = gdk_keymap_get_for_display(display);
    int capslock = gdk_keymap_get_caps_lock_state(keymap);

    if (keyboard_get_shiftlock() != capslock) {
        keyboard_set_shiftlock(capslock);
        capslock_lock_state = capslock;
    }
#endif
#if 0
    printf("kbd_sync_caps_lock host caps-lock: %d kbd shift-lock: %d gtk lock state: %d\n",
        capslock, keyboard_get_shiftlock(), capslock_lock_state);
#endif
}

/** \brief  Get modifiers keys for keyboard event
 *
 * \param[in]   report  GDK keypress event
 *
 * \return  bitmasks of modifiers combined into a single int
 */
static int kbd_get_modifier(GdkEvent *report)
{
    int ret = 0;
    /* printf("key.state: %04x key.keyval: %04x (%s) key.hardware_keycode: %04x\n",
            report->key.state, report->key.keyval, gdk_keyval_name(report->key.keyval),
            report->key.hardware_keycode); */
    if (report->key.state & GDK_SHIFT_MASK) {
        if (shiftl_state || capslock_state) {
            ret |= KBD_MOD_LSHIFT;
        }
        if (shiftr_state) {
            ret |= KBD_MOD_RSHIFT;
        }
    }
    if (report->key.state & GDK_MOD1_MASK) {
        ret |= KBD_MOD_LALT;
    }
    if (report->key.state & GDK_MOD5_MASK) {
        ret |= KBD_MOD_RALT;
    }
    if (report->key.state & GDK_CONTROL_MASK) {
        ret |= KBD_MOD_LCTRL;
    }
    return ret;
}

/* when shift is pressed, then a regular key, then shift released, and then
   the regular key, the key-release event of the last key will not match the
   key-press event. this will confuse our generic keyboard handling. to fight
   agains this problem, we remember what keys were pressed and alter the
   events accordingly. */

/** \brief  Current number of pressed key buffers elements
 */
static int keyspressed = 0;


/** \brief  Maximum size of pressed key buffers
 */
#define KEYS_PRESSED_MAX 200


/** \brief  GDK key codes
 */
static int pressedkeys[KEYS_PRESSED_MAX];

/** \brief  GDK key modifiers
 */
static int pressedkeysmod[KEYS_PRESSED_MAX];

/** \brief  Key hardware scancodes
 */
static int pressedkeyshw[KEYS_PRESSED_MAX];

/** \brief  GDK key states
 */
static int pressedkeysstate[KEYS_PRESSED_MAX];


/** \brief  Look up key buffer index for \a report by hardware scancode
 *
 * \param[in]   report  GDK key-press event
 *
 * \return  index or -1 when not found
 */
static int findpressedkey(GdkEvent *report)
{
    int i;
    for (i = 0; i < keyspressed; i++) {
        if(report->key.hardware_keycode == pressedkeyshw[i]) {
            return i;
        }
    }
    return -1;
}


/** \brief  Add pressed key
 *
 * \param[in]   report  GDK key-press event
 * \param[in]   key     GDK key value
 * \param[out]  mod     GDK key modifiers
 *
 * \return  boolean (1 if added)
 */
static int addpressedkey(GdkEvent *report, int *key, int *mod)
{
    int idx;
    *mod = kbd_get_modifier(report);
    if (keyspressed < KEYS_PRESSED_MAX) {
        idx = findpressedkey(report);
        if (idx == -1) {
            pressedkeys[keyspressed] = *key;
            pressedkeyshw[keyspressed] = report->key.hardware_keycode;
            pressedkeysmod[keyspressed] = *mod;
            pressedkeysstate[keyspressed] = report->key.state;
#if 0
            printf("addpressedkey    val:%5d hw:%04x mod:%04x state:%04x\n",
                *key, report->key.hardware_keycode, *mod, report->key.state);
#endif
            keyspressed++;
            return 1;
        }
    }
    return 0;
}


/** \brief  Remove pressed key
 *
 * \param[in]   report  GDK key-press event
 * \param[out]  key     GDK key value
 * \param[out]  mod     GDK key modifiers
 *
 * \return  boolean (1 if removed)
 */
static int removepressedkey(GdkEvent *report, int *key, int *mod)
{
    int i, idx;
    *mod = 0;
    if (keyspressed > 0) {
        idx = findpressedkey(report);
        if (idx != -1) {
            *key = pressedkeys[idx];
            *mod = pressedkeysmod[idx];
            report->key.state = pressedkeysstate[idx];
#if 0
            printf("removepressedkey val:%5d hw:%04x mod:%04x state:%04x\n",
                *key, report->key.hardware_keycode, *mod, report->key.state);
#endif
            for (i = idx; i < keyspressed; i++) {
                pressedkeys[i] = pressedkeys[i + 1];
                pressedkeyshw[i] = pressedkeyshw[i + 1];
                pressedkeysmod[i] = pressedkeysmod[i + 1];
                pressedkeysstate[i] = pressedkeysstate[i + 1];
            }
            keyspressed--;
            return 1;
        }
    }
    return 0;
}

/** \brief  Check if a key event report refers to a "reset" action
 *
 * \param[in]   report  event object
 *
 * \return  TRUE if reported key combo is mapped to a "reset" action
 */
static gboolean isresethotkey(GdkEvent *report)
{
    int checkaccel[2] = { ACTION_MACHINE_RESET_CPU, ACTION_MACHINE_POWER_CYCLE };
    gboolean res = FALSE;
    int i;
    char *this_accel = gtk_accelerator_get_label(report->key.keyval,
                                                 report->key.state & VHK_ACCEPTED_MODIFIERS);
    for (i = 0; i < 2; i++) {
        gchar *accel = vhk_gtk_get_accel_label_by_action(checkaccel[i]);

        if (accel != NULL && strcmp(this_accel, accel) == 0) {
            i = 2;
            res = TRUE;
        }
        g_free(accel);
    }
    g_free(this_accel);
    return res;
}

#ifdef WINDOWS_COMPILE
/** \brief  Table of numpad keyval translations
 *
 * Array of translations from Gdk Windows keyvals to "proper" Gdk keyvals
 * on Linux, as expected by the VICE keymaps.
 *
 * The first entry is the (correct) keyval on Linux, the second the (incorrect)
 * keyval on Windows and the third the state of bit 8 in the scancode.
 *
 * The list is terminated with 0 in the first (Linux) entry.
 */
static const guint numpad_fixes[][KV_ARR_SIZE] = {
    /* Linux                Windows             scancode bit 8 */
    { GDK_KEY_KP_Home,      GDK_KEY_Home,       FALSE },    /* 7 Home */
    { GDK_KEY_KP_Up,        GDK_KEY_Up,         FALSE },    /* 8 arrow up */
    { GDK_KEY_KP_Page_Up,   GDK_KEY_Page_Up,    FALSE },    /* 9 PgUp (same keyval as _Prior) */
    { GDK_KEY_KP_Prior,     GDK_KEY_Prior,      FALSE },    /* 9 Prior (same keyval as _Page_Up) */
    { GDK_KEY_KP_Left,      GDK_KEY_Left,       FALSE },    /* 4 arrow left */
    { GDK_KEY_KP_Begin,     GDK_KEY_Begin,      FALSE },    /* 5 Begin */
#if 0
    /* GDK_KEY_KP_Clear does not exist, the alias is remapped instead */
    { GDK_KEY_KP_Clear,     GDK_KEY_Clear,      FALSE },    /* 5 Clear */
#endif
    { GDK_KEY_KP_Right,     GDK_KEY_Right,      FALSE },    /* 6 arrow right */
    { GDK_KEY_KP_End,       GDK_KEY_End,        FALSE },    /* 1 End */
    { GDK_KEY_KP_Down,      GDK_KEY_Down,       FALSE },    /* 2 arrow down */
    { GDK_KEY_KP_Page_Down, GDK_KEY_Page_Down,  FALSE },    /* 3 PgDn (same keyval as _Next) */
    { GDK_KEY_KP_Next,      GDK_KEY_Next,       FALSE },    /* 3 Next (same keyval as _Page_Down) */
    { GDK_KEY_KP_Insert,    GDK_KEY_Insert,     FALSE },    /* 0 Ins */
    { GDK_KEY_KP_Delete,    GDK_KEY_Delete,     FALSE },    /* . Del */
    { GDK_KEY_KP_Enter,     GDK_KEY_Return,     TRUE },     /* Enter */
    { 0,                    0,                  FALSE }
};
#endif

#ifdef WINDOWS_COMPILE
/** \brief  Fix numpad keyvals
 *
 * On Windows Gdk doesn't discriminate between some "normal" keys and some
 * numpad keys, so we try to translate these for the VICE keymaps.
 *
 * ShiftLock OFF (US keyboard):
 *
 *  Key             Linux           Windows
 *  ----------      --------------- -----------
 *  /               KP_Divide       KP_Divide
 *  *               KP_Multiply     KP_Multiply
 *  -               KP_Subtract     KP_Subtract
 *  +               KP_Add          KP_Add
 *  . Del           KP_Delete       Delete
 *  Enter           KP_Enter        Return
 *  7 Home          KP_Home         Home
 *  8 up arrow      KP_Up           Up
 *  9 PgUp          KP_Page_Up      Page_Up
 *  4 left arrow    KP_Left         Left
 *  5               KP_Begin        Clear
 *  6 right arrow   KP_Right        Right
 *  1 End           KP_End          End
 *  2 down arrow    KP_Down         Down
 *  3 PgDn          KP_Next         Page_Down
 *
 * \param[in]   event   key press/release event
 *
 * \return  fixed keyval for numpad keys
 */
static guint fix_numpad_keyval(GdkEvent *event)
{
    guint keyval = event->key.keyval;
    int scancode = gdk_event_get_scancode(event);
    gboolean numpad = (scancode & 0x100) ? TRUE : FALSE;
    int i = 0;
#ifdef DEBUG_GDK_FIXES
    debug_gtk3("scancode 0x%04x numpad: %d",
               (unsigned int)scancode, numpad);
#endif
    while (numpad_fixes[i][KV_FIXED] != 0) {
        if (keyval == numpad_fixes[i][KV_ALIAS] && numpad == numpad_fixes[i][KV_BIT8]) {
            return numpad_fixes[i][KV_FIXED];
        }
        i++;
    }
    return keyval;
}
#endif

/** \brief  Table of numpad aliases
 *
 * Depending on the Keyboard layout some keys produce different keycodes, which
 * we try to combat here. Eg on a german keyboard there is a comma on the "delete"
 * key instead of the decimal point.
 *
 * The first entry is the (correct) keyval, the second the (incorrect) alias.
 *
 * CAUTION: when a key exist on KP and as a regular key (example: HOME), you
 *          MUST make sure to only map KP to KP and non KP to non KP symbols here.
 *
 * The list is terminated with 0 in the first (Linux) entry.
 */
static const guint numpad_aliases[][2] = {
    /* Linux                Linux (alias) */
    { GDK_KEY_KP_Decimal,   GDK_KEY_KP_Separator }, /* . Del (same physical key) */
    { GDK_KEY_KP_Page_Up,   GDK_KEY_KP_Prior     }, /* 9 PgUp */
    { GDK_KEY_KP_Page_Down, GDK_KEY_KP_Next      }, /* 3 PgDn */
    /* GDK_KEY_KP_Clear does not exist, but Clear only exists on numpad */
    { GDK_KEY_KP_Begin,     GDK_KEY_Clear        }, /* 5 Begin */
    { GDK_KEY_Page_Up,      GDK_KEY_Prior        }, /* 9 PgUp */
    { GDK_KEY_Page_Down,    GDK_KEY_Next         }, /* 3 PgDn */
    { 0,                    0,                   }
};

/** \brief  Fix numpad aliases
 *
 * Depending on the Keyboard Layout/Localisation some numpad keys produce
 * a different keyval - we fix it here so the joystick mappings can always
 * use the same values
 *
 * \param[in]   event   key press/release event
 *
 * \return  fixed keyval for numpad keys
 */
static guint fix_numpad_aliases(GdkEvent *event)
{
    guint keyval = event->key.keyval;
    int i = 0;
#ifdef DEBUG_GDK_FIXES
    debug_gtk3("fix_numpad_aliases: keyval 0x%04x", (unsigned int)keyval);
#endif
    while (numpad_aliases[i][0] != 0) {
        if (keyval == numpad_aliases[i][1]) {
            return numpad_aliases[i][0];
        }
        i++;
    }
    return keyval;
}

/** \brief  Fixup the GTK keycodes so they are the same on all OSs */
/**
 *  \return fixed keyval (also modifies the value in the event)
 */
guint kbd_fix_keyval(GdkEvent *event)
{
    guint key = event->key.keyval;
#ifdef WINDOWS_COMPILE
# ifdef DEBUG_GDK_FIXES
    debug_gtk3("key before numpad fix: 0x%04x (GDK_KEY_%s).",
                (unsigned int)key, gdk_keyval_name(key));
# endif
    key = event->key.keyval = fix_numpad_keyval(event);
# ifdef DEBUG_GDK_FIXES
    debug_gtk3("key after numpad fix : 0x%04x (GDK_KEY_%s).",
                (unsigned int)key, gdk_keyval_name(key));
# endif
#endif /* WINDOWS_COMPILE */

#ifdef DEBUG_GDK_FIXES
    debug_gtk3("key before aliases fix : 0x%04x (GDK_KEY_%s).",
                (unsigned int)key, gdk_keyval_name(key));
#endif
    key = event->key.keyval = fix_numpad_aliases(event);
#ifdef DEBUG_GDK_FIXES
    debug_gtk3("key after aliases fix : 0x%04x (GDK_KEY_%s).",
                (unsigned int)key, gdk_keyval_name(key));
#endif
    return key;
}

/** \brief  Gtk keyboard event handler
 *
 * \param[in]   w       widget triggering the event
 * \param[in]   report  event object
 * \param[in]   gp      extra data (unused)
 *
 * \return  TRUE if event handled (won't get passed to the emulated machine)
 */
static gboolean kbd_event_handler(GtkWidget *w, GdkEvent *report, gpointer gp)
{
    int key = report->key.keyval;
    int mod = 0;

    switch (report->type) {
        case GDK_KEY_PRESS:
            /* fprintf(stderr, "GDK_KEY_PRESS: %u %04x.\n",
                       report->key.keyval,  report->key.state); */
            kbd_fix_shift_press(report);
#ifdef WINDOWS_COMPILE
/* HACK: The Alt-Gr Key seems to work differently on windows and linux.
         On Linux one Keypress "ISO_Level3_Shift" will be produced, and
         the modifier mask for combined keys will be GDK_MOD5_MASK.
         On Windows two Keypresses will be produced, first "Control_L"
         then "Alt_R", and the modifier mask for combined keys will be
         GDK_MOD2_MASK.
         The following is a hack to compensate for that and make it
         always work like on linux.
*/
            if ((report->key.keyval == GDK_KEY_Alt_R) && (report->key.state & GDK_MOD2_MASK)) {
                /* Alt-R with modifier MOD2 */
                key = report->key.keyval = GDK_KEY_ISO_Level3_Shift;
                report->key.state &= ~GDK_MOD2_MASK;
                /* release control in the emulated keymap */
                keyboard_key_released(GDK_KEY_Control_L, KBD_MOD_LCTRL);
            } else if (report->key.state & GDK_MOD2_MASK) {
                report->key.state &= ~GDK_MOD2_MASK;
                report->key.state |= GDK_MOD5_MASK;
            }
            /* fprintf(stderr, "               %u %04x.\n",
                       report->key.keyval,  report->key.state); */
#endif

#ifdef DEBUG_GDK_FIXES
            debug_gtk3("(press) key: 0x%04x (GDK_KEY_%s).",
                       (unsigned int)key, gdk_keyval_name(key));
#endif
            key = kbd_fix_keyval(report);

            /* FIXME:   This still gets the unmodified keyval, but we cannot
             *          modify `report` since that's owned by Gdk.
             */
            ui_statusbar_update_kbd_debug(report);

            /* Don't hold the mainlock while dealing with hotkeys. Some of these
               need to run with an unlocked handler in order to avoid glitching VICE. */
            mainlock_release();
            if (gtk_window_activate_key(GTK_WINDOW(w), (GdkEventKey *)report)) {
                mainlock_obtain();
                /* mnemonic or accelerator was found and activated. */
                /* release all previously pressed keys to prevent stuck keys,
                   except we detected a "reset" hotkey (because certain cartridges
                   or kernals want you to hold a key on reset for certain features) */
                if (!isresethotkey(report)) {
                    keyspressed = 0;
                    keyboard_key_clear();
                    kbd_fix_shift_clear();
                }
                kbd_sync_caps_lock();
                return TRUE;
            }
            mainlock_obtain();

            /* only press keys that were not yet pressed */
            if (addpressedkey(report, &key, &mod)) {
#if 0
                printf("%2d key press,   %5u %04x %04x. lshift: %d rshift: %d slock: %d mod:  %04x\n",
                    keyspressed,
                    report->key.keyval, report->key.state, report->key.hardware_keycode,
                    shiftl_state, shiftr_state, capslock_state, mod);
#endif
                keyboard_key_pressed((signed long)key, mod);
            }
/* HACK: on windows the caps-lock key generates an invalid keycode of 0xffffff,
         so when we see this in the event we explicitly sync caps-lock state
         to make it work in the emulation */
#ifdef WINDOWS_COMPILE
            if (report->key.keyval == 0xffffff) {
                kbd_sync_caps_lock();
            }
#endif
/* HACK: on macOS caps-lock ON and OFF generate events, checking the state as
         such does not work, so we must track und update on our own */
#ifdef MACOS_COMPILE
            kbd_sync_caps_lock();
#endif
            return TRUE;
        case GDK_KEY_RELEASE:
            /* fprintf(stderr, "GDK_KEY_RELEASE: %u %04x.\n",
                       report->key.keyval,  report->key.state); */
            kbd_fix_shift_release(report);
#ifdef WINDOWS_COMPILE
            /* HACK: remap control,alt+r to alt-gr, see above */
            if (report->key.keyval == GDK_KEY_Alt_R) {
                key = report->key.keyval = GDK_KEY_ISO_Level3_Shift;
            }
            /* fprintf(stderr, "                 %u %04x.\n",
                       report->key.keyval,  report->key.state); */
#endif

#ifdef DEBUG_GDK_FIXES
            debug_gtk3("(release) key: 0x%04x (GDK_KEY_%s).",
                       (unsigned int)key, gdk_keyval_name(key));
#endif
            key = kbd_fix_keyval(report);

            ui_statusbar_update_kbd_debug(report);

            if (removepressedkey(report, &key, &mod)) {
#if 0
                printf("%2d key release, %5u %04x %04x. lshift: %d rshift: %d slock: %d mod:  %04x\n",
                    keyspressed, report->key.keyval, report->key.state, report->key.hardware_keycode,
                    shiftl_state, shiftr_state, capslock_state, mod);
#endif
                keyboard_key_released(key, mod);
            } else {
                /* we released a key that was not pressed, something is wrong */
                keyspressed = 0;
                kbd_fix_shift_clear();
                keyboard_key_clear();
                kbd_sync_caps_lock();
            }
/* HACK: on windows the caps-lock key generates an invalid keycode of 0xffffff,
         so when we see this in the event we explicitly sync caps-lock state
         to make it work in the emulation */
#ifdef WINDOWS_COMPILE
            if (report->key.keyval == 0xffffff) {
                kbd_sync_caps_lock();
            }
#endif
/* HACK: on macOS caps-lock ON and OFF generate events, checking the state as
         such does not work, so we must track und update on our own */
#ifdef MACOS_COMPILE
            kbd_sync_caps_lock();
#endif
            break;
        /* mouse pointer enters or exits the emulator */
        case GDK_LEAVE_NOTIFY:
            keyspressed = 0;
            kbd_fix_shift_clear();
            keyboard_key_clear();
            kbd_sync_caps_lock();
            break;
        case GDK_ENTER_NOTIFY:
            keyspressed = 0;
            kbd_fix_shift_clear();
            keyboard_key_clear();
            kbd_sync_caps_lock();
            break;
        /* focus change */
        case GDK_FOCUS_CHANGE:
            keyspressed = 0;
            kbd_fix_shift_clear();
            keyboard_key_clear();
            kbd_sync_caps_lock();
            break;
        default:
            break;
    }                           /* switch */
    return FALSE;
}


/** \brief  Connect keyboard event handlers to the current window
 *
 * \param[in]   widget  GtkWindow instance
 * \param[in]   data    extra event data
 */
void kbd_connect_handlers(GtkWidget *widget, void *data)
{
    g_signal_connect(
            G_OBJECT(widget),
            "key-press-event",
            G_CALLBACK(kbd_event_handler), data);
    g_signal_connect(
            G_OBJECT(widget),
            "key-release-event",
            G_CALLBACK(kbd_event_handler), data);
    g_signal_connect(
            G_OBJECT(widget),
            "enter-notify-event",
            G_CALLBACK(kbd_event_handler), data);
    g_signal_connect(
            G_OBJECT(widget),
            "leave-notify-event",
            G_CALLBACK(kbd_event_handler), data);
    g_signal_connect(
            G_OBJECT(widget),
            "focus-in-event",
            G_CALLBACK(kbd_event_handler), data);
    g_signal_connect(
            G_OBJECT(widget),
            "focus-out-event",
            G_CALLBACK(kbd_event_handler), data);
}
