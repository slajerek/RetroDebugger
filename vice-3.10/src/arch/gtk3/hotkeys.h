/** \file   hotkeys.h
 * \brief   Gtk3 custom hotkeys handling - header
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

#ifndef VICE_HOTKEYS_H
#define VICE_HOTKEYS_H

#include "vice.h"

#include <gtk/gtk.h>
#include <stdbool.h>
#include "archdep_defs.h"
#include "hotkeystypes.h"
#include "uiactions.h"
#include "uitypes.h"


/** \brief  Accepted GDK modifiers for hotkeys
 *
 * This is required to avoid keys like NumLock showing up in the accelerators,
 * and sometimes GDK will pass along reserved bits (MOD27 etc).
 *
 * GDK_MOD1_MASK refers to Alt/Option.
 * GDK_MOD2_MASK refers to NumLock, so we filter it out.
 * GDK_META_MASK refers to the Command key on MacOS, doesn't appear to do
 * anything on Linux.
 * GDK_SUPER_MASK refers to the "Windows key" on PC keyboards. Since window
 * managers on Linux, and Windows itself, use this key for all sorts of things,
 * we filter it out.
 */
#ifdef MACOS_COMPILE
/* Command, Control, Option, Shift */
# define VHK_ACCEPTED_MODIFIERS \
    (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD1_MASK|GDK_META_MASK)
#else
/* Control, Alt, Shift */
# define VHK_ACCEPTED_MODIFIERS \
    (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD1_MASK)
#endif

/* TODO: Perhaps move to src/arch/gtk3/widget/settings_hotkeys.c ? */


/** \brief  Gtk3-specific `user_data` object in `vhk_map_t`
 */
typedef struct vhk_gtk_map_s {
    const ui_menu_item_t *decl;         /**< menu item declaration with additional
                                             info for Gtk3, such as whether to
                                             connect locked or unlocked */
    gulong                handler[2];   /**< signal handler ID, used to be able
                                             to block signals when changing
                                             state such as 'toggled' of menu
                                             items */
} vhk_gtk_map_t;


vhk_gtk_map_t *vhk_gtk_map_new (const ui_menu_item_t *decl);
void           vhk_gtk_map_free(vhk_gtk_map_t *map);

void           vhk_gtk_init_accelerators(GtkWidget *window);
gboolean       vhk_gtk_remove_accelerator(guint keysym, GdkModifierType modifier);

gchar         *vhk_gtk_get_accel_label_by_action(int action);
gchar         *vhk_gtk_get_accel_label_by_map(const ui_action_map_t *map);
GtkWidget     *vhk_gtk_get_menu_item_by_action_for_window(int action,
                                                          int window_id);
void           vhk_gtk_set_menu_item_accel_label(GtkWidget *item, int action);
void           vhk_gtk_set_check_item_blocked(GtkWidget *item,
                                              gboolean   checked);
void           vhk_gtk_set_check_item_blocked_by_action(int      action,
                                                        gboolean checked);
void           vhk_gtk_set_check_item_blocked_by_action_for_window(int      action,
                                                                   int      window_id,
                                                                   gboolean checked);

#endif
