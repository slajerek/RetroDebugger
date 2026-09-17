/** \file   hotkeys.c
 * \brief   Gtk3 custom hotkeys handling
 *
 * Provides custom keyboard shortcuts for the Gtk3 UI.
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

/* Resources manipulated in this file:
 *
 * $VICERES HotkeyFile all
 */

#include "vice.h"

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "archdep.h"
#include "debug_gtk3.h"
#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "parser.h"
#include "textfilereader.h"
#include "ui.h"
#include "uiapi.h"
#include "uiactions.h"
#include "uihotkeys.h"
#include "uimenu.h"
#include "util.h"
#include "version.h"
#include "vice_gtk3.h"
#ifdef USE_SVN_REVISION
#include "svnversion.h"
#endif

#include "hotkeys.h"


/** \brief  Reference to the accelerator group
 *
 * Global accelerator group used for all accelerators in UI.
 */
static GtkAccelGroup *accel_group = NULL;


/** \brief  Check if window ID is valid
 *
 * Check if \a window_id is either #PRIMARY_WINDOW or #SECONDARY_WINDOW.
 *
 * Logs an error if \a window_id is invalid.
 *
 * \param[in]   window_id   window ID
 */
static gboolean valid_window_id(gint window_id)
{
    if (window_id != PRIMARY_WINDOW && window_id != SECONDARY_WINDOW) {
        log_error(LOG_DEFAULT, "Invalid window ID of %d.", window_id);
        return FALSE;
    }
    return TRUE;
}

/** \brief  Callback that forwards accelerator codes
 *
 * \param[in]       accel_grp       accelerator group (unused)
 * \param[in]       acceleratable   ? (unused)
 * \param[in]       keyval          GDK keyval (unused)
 * \param[in]       modifiers       GDK key modifier(s) (unused)
 * \param[in]       action          UI action ID
 */
static gboolean handle_accelerator(GtkAccelGroup   *accel_grp,
                                   GObject         *acceleratable,
                                   guint            keyval,
                                   GdkModifierType  modifiers,
                                   gpointer         action)
{
    ui_action_trigger(GPOINTER_TO_INT(action));
    return TRUE;
}

/** \brief  Set up a closure to trigger UI action for a hotkey
 *
 * Create a closure to trigger UI \a action for \a keysym and \a modifier.
 * This way hotkeys will work in fullscreen and also when there's no menu item
 * associated with \a action.
 *
 * \param[in]   action      UI action ID
 * \param[in]   keysym      Gdk keysym
 * \param[in]   modifier    Gdk modifier mask
 * \param[in]   unlocked    connect accelator non-lockeding
 */
static void connect_accelerator(int             action,
                                guint           keysym,
                                GdkModifierType modifier,
                                bool            unlocked)
{
    GClosure *closure = g_cclosure_new(G_CALLBACK(handle_accelerator),
                                       GINT_TO_POINTER(action),
                                       NULL);

    if (unlocked) {
        gtk_accel_group_connect(accel_group,
                                keysym,
                                modifier,
                                GTK_ACCEL_MASK,
                                closure);
    } else {
        vice_locking_gtk_accel_group_connect(accel_group,
                                             keysym,
                                             modifier,
                                             GTK_ACCEL_MASK,
                                             closure);
    }
}

/** \brief  Set accelerator for menu item, using new API
 *
 * \param[in]   item            runtime menu item
 * \param[in]   arch_keysym     Gkd keysym
 * \param[in]   arch_modmask    Gdk modifier mask
 *
 * \note    This only sets the \a item's accelerator, it does not connect any
 *          handler, the handler is set up during menu creation in uimenu.c.
 */
static void set_menu_item_accel(GtkWidget *item,
                                uint32_t   arch_keysym,
                                uint32_t   arch_modmask)
{
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(item));

#if 0
    debug_gtk3("called with keysym %04x, mask %08x, item = %p",
               arch_keysym, arch_modmask, (const void*)item);
#endif
    if (label != NULL) {
        gtk_accel_label_set_accel(GTK_ACCEL_LABEL(label), arch_keysym, arch_modmask);
    }
}

/** \brief  Clear accelerator from runtime menu item
 *
 * \param[in]   item    runtime menu item
 */
static void clear_menu_item_accel(GtkWidget *item)
{
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(item));
    gtk_accel_label_set_accel(GTK_ACCEL_LABEL(label), 0, 0);
}


/** \brief  Remove accelerator from global accelerator group
 *
 * \param[in]   keysym      Gdk keysym
 * \param[in]   modifier    Gdk modifier mask
 *
 * \return  `TRUE` on success
 */
gboolean vhk_gtk_remove_accelerator(guint keysym, GdkModifierType modifier)
{
    return gtk_accel_group_disconnect_key(accel_group, keysym, modifier);
}


/** \brief  Create accelerator group and add it to \a window
 *
 * \param[in]       window  top level window
 *
 * \note    Allocating the accel group in ui_hotkeys_arch_init() cannot be done
 *          since it's too late in the UI setup process.
 */
void vhk_gtk_init_accelerators(GtkWidget *window)
{
    /* single accelerator group for both windows */
    if (accel_group == NULL) {
        accel_group = gtk_accel_group_new();
    }
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
}


/** \brief  Get accelerator label for hotkey in \a map
 *
 * \param[in]   map hotkey map
 *
 * \return  accelerator label or `NULL` when no hotkey defined
 *
 * \note    The value returned is allocated by Gtk and should be freed after
 *          use with g_free()
 *
 * \todo    Rename
 */
gchar *vhk_gtk_get_accel_label_by_map(const ui_action_map_t *map)
{
    if (map->arch_keysym > 0) {
        return gtk_accelerator_get_label(map->arch_keysym, map->arch_modmask);
    }
    return NULL;
}


/** \brief  Get accelerator label for UI action, if any
 *
 * \param[in]   action  UI action ID
 *
 * \return  accelerator label or `NULL when \a action not found or no hotkey
 *          exists for the \a action
 *
 * \todo    Rename
 */
gchar *vhk_gtk_get_accel_label_by_action(int action)
{
    ui_action_map_t *map = ui_action_map_get(action);
    if (map != NULL) {
        return vhk_gtk_get_accel_label_by_map(map);
    }
    return NULL;
}


/** \brief  Get runtime menu item for UI action and main window
 *
 * \param[in]   action      UI action ID
 * \param[in]   window_id   window ID (`PRIMARY_WINDOW` or `SECONDARY_WINDOW`)
 *
 * \return  menu item or `NULL` when not found
 *
 * TODO:    Nothing Gtk-specific in this function: could be moved to shared
 *          code and return a `void*`.
 */
GtkWidget *vhk_gtk_get_menu_item_by_action_for_window(int action, int window_id)
{
    if (valid_window_id(window_id)) {
        ui_action_map_t *map = ui_action_map_get(action);
        if (map != NULL) {
            return map->menu_item[window_id];
        }
    }
    return NULL;
}


/** \brief  Set accelerator label to the hotkey associated with UI action
 *
 * Look up \a action and if found set the accelerator label of \a item to the
 * hotkey defined for \a action, if the action is not found the accelerator
 * label will be cleared.
 * This allows context menus to display an accelerator next to an item. Only
 * sets the label of a menu item, no action handler is set for the item.
 *
 * \param[in]   item    runtime menu item
 * \param[in]   action  UI action ID
 */
void vhk_gtk_set_menu_item_accel_label(GtkWidget *item, int action)
{
    ui_action_map_t *map;
    GtkWidget       *label;

    label = gtk_bin_get_child(GTK_BIN(item));
    map   = ui_action_map_get(action);
    if (map != NULL) {
        gtk_accel_label_set_accel(GTK_ACCEL_LABEL(label),
                                  map->arch_keysym,
                                  map->arch_modmask);
    } else {
        gtk_accel_label_set_accel(GTK_ACCEL_LABEL(label), 0, 0);
    }
}


/** \brief  Check/uncheck check menu item while blocking its handler
 *
 * Set a checkbox menu item's state while blocking the 'activate' handler so
 * the handler won't recursively call itself.
 *
 * \param[in]   item    GtkCheckMenuItem instance
 * \param[in]   checked new check button state
 */
void vhk_gtk_set_check_item_blocked(GtkWidget *item, gboolean checked)
{
    gulong handler_id = GPOINTER_TO_ULONG(g_object_get_data(G_OBJECT(item),
                                                            "HandlerID"));
    g_signal_handler_block(item, handler_id);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), checked);
    g_signal_handler_unblock(item, handler_id);
}


/** \brief  Check/uncheck check menu item(s) mapped to UI action
 *
 * Set menu item or items (x128) that are registered for UI \a action to
 * \a checked.
 *
 * \note    This will usually suffice for most items, but in cases where x128
 *          can have different states for a menu item per window (status bar
 *          enable/disable, fullscreen, fulllscreen-decorations) the function
 *          vhk_gtk_set_check_item_blocked_by_action_for_window() is available.
 *
 * \param[in]   action  UI action ID
 * \param[in]   checked new checked state for menu item(s)
 */
void vhk_gtk_set_check_item_blocked_by_action(int action, gboolean checked)
{
    ui_action_map_t *map = ui_action_map_get(action);
    if (map != NULL) {
        if (map->menu_item[PRIMARY_WINDOW] != NULL) {
            vhk_gtk_set_check_item_blocked(map->menu_item[PRIMARY_WINDOW], checked);
        }
        if (map->menu_item[SECONDARY_WINDOW] != NULL) {
            vhk_gtk_set_check_item_blocked(map->menu_item[SECONDARY_WINDOW], checked);
        }
    }
}


/** \brief  Check/uncheck check menu item mapped to UI action for a window
 *
 * Set menu item for a main window that is registered for UI \a action to
 * \a checked.
 *
 * \param[in]   action      UI action ID
 * \param[in]   window_id   window ID (`PRIMARY_WINDOW` or `SECONDARY_WINDOW`)
 * \param[in]   checked     new checked state for menu item(s)
 */
void vhk_gtk_set_check_item_blocked_by_action_for_window(int      action,
                                                         int      window_id,
                                                         gboolean checked)
{
    if (valid_window_id(window_id)) {
        ui_action_map_t *map = ui_action_map_get(action);
        if (map != NULL) {
            vhk_gtk_set_check_item_blocked(map->menu_item[window_id], checked);
        }
    }
}



/******************************************************************************
 *      Virtual method implementations of the interface in src/uiapi.h        *
 *****************************************************************************/

/*
 * Helpers functions
 */

/** \brief  Create gtk3-specific hotkey data object
 *
 * Create object with a reference to the menu item declaration for a UI action
 * and signal handler IDs to allow the UI code to block/unblock signal handlers.
 * The menu item declaration contains a bool indicating whether to connect a
 * signal handler directly or through the VICE locking mechanism. The signal
 * handler IDs are used to temporarily block signal handlers from triggering
 * when updating their menu items' state (e.g. check/radio buttons).
 *
 * \param[in]   decl    UI menu item declaration
 *
 * \return  new gtk3-specific hotkey data object
 *
 * \note    Free object with vhk_map_gtk_free()
 */
vhk_gtk_map_t *vhk_gtk_map_new(const ui_menu_item_t *decl)
{
    vhk_gtk_map_t *arch_map;

    arch_map = lib_malloc(sizeof *arch_map);
    arch_map->decl = decl;
    arch_map->handler[PRIMARY_WINDOW] = 0;
    arch_map->handler[SECONDARY_WINDOW] = 0;
    return arch_map;
}


/** \brief  Free memory used by gtk3-specific hotkey data object
 *
 * Free memory used by \a arch_map and its members.
 *
 * \param[in]   arch_map    gtk3-specific hotkey data object
 */
void vhk_gtk_map_free(vhk_gtk_map_t *arch_map)
{
    if (arch_map != NULL) {
        lib_free(arch_map);
    }
}

/*
 * API implementations
 */

/** \brief  Virtual method for obtaining Gdk modifier mask from VICE modifier mask
 *
 * Get GDK modifier mask bit from VICE modifier mask bit
 *
 * \param[in]   vice_mod    VICE modifier mask bit
 *
 * \return  GDK modifier mask bit
 *
 * \note    Only works on single-bit values, for combined values we'll need
 *          another (UI-agnostic) function.
 */
uint32_t ui_hotkeys_arch_modifier_to_arch(uint32_t vice_mod)
{
    switch (vice_mod) {
        case VHK_MOD_ALT:   /* fall through */
        case VHK_MOD_OPTION:
            return GDK_MOD1_MASK;
        case VHK_MOD_COMMAND:   /* fall through */
        case VHK_MOD_META:
            return GDK_META_MASK;
        case VHK_MOD_CONTROL:
            return GDK_CONTROL_MASK;
        case VHK_MOD_HYPER:
            return GDK_HYPER_MASK;
        case VHK_MOD_SHIFT:
            return GDK_SHIFT_MASK;
        case VHK_MOD_SUPER:
            return GDK_SUPER_MASK;
        default:
            return 0;
    }
}


/** \brief  VICE modifier mask bit from GDK modifier mask bit
 *
 * \param[in]   arch_mod    GDK modifier mask bit
 *
 * \return  VICE modifier mask bit
 *
 * \note    Returns different values on MacOS vs normal OSes.
 */
uint32_t ui_hotkeys_arch_modifier_from_arch(uint32_t arch_mod)
{
    switch (arch_mod) {
        case GDK_MOD1_MASK:
#ifdef MACOS_COMPILE
            return VHK_MOD_OPTION;
#else
            return VHK_MOD_ALT;
#endif
        case GDK_META_MASK:
#ifdef MACOS_COMPILE
            return VHK_MOD_COMMAND;
#else
            return VHK_MOD_META;
#endif
        case GDK_CONTROL_MASK:
            return VHK_MOD_CONTROL;
        case GDK_HYPER_MASK:
            return VHK_MOD_HYPER;
        case GDK_SHIFT_MASK:
            return VHK_MOD_SHIFT;
        case GDK_SUPER_MASK:
            return VHK_MOD_SUPER;
        default:
            return 0;
    }
}


/** \brief  Translate VICE keysym to UI keysym
 *
 * \param[in]   vice_keysym VICE-specific keysym
 *
 * \return  UI-specific keysym
 *
 * \see src/arch/shared/hotkeys/vhkkeysyms.h
 */
uint32_t ui_hotkeys_arch_keysym_to_arch(uint32_t vice_keysym)
{
    /* Compyx was clever here, using X11 keysyms for VICE, which happen to
     * also be used by Gdk ;) */
    return vice_keysym;
}


/** \brief  Translate UI keysym to VICE keysym
 *
 * \param[in]   arch_keysym UI-specific keysym
 *
 * \return  VICE-specific keysym
 *
 * \see src/arch/shared/hotkeys/vhkkeysyms.h
 */
uint32_t ui_hotkeys_arch_keysym_from_arch(uint32_t arch_keysym)
{
    if (arch_keysym == GDK_KEY_VoidSymbol) {
        arch_keysym = 0;
    }
    return arch_keysym;
}


/** \brief  Translate a full modifier mask from VICE to Gdk
 *
 * \param[in]   vice_modmask    VICE modifier mask
 *
 * \return  Gdk modifier mask
 *
 * \todo    We might be able to move this into shared/hotkeys, reducing the
 *          number of virtual methods required for a UI.
 */
uint32_t ui_hotkeys_arch_modmask_to_arch(uint32_t vice_modmask)
{
    uint32_t mask = 0;
    uint32_t bit  = 0;

    while (vice_modmask != 0) {
        /* printf("%s(): vice_modmask = %08x\n", __func__, vice_modmask);*/
        /* add UI modifier translated from VICE modifier */
        mask |= ui_hotkeys_arch_modifier_to_arch(vice_modmask & (1u << bit));
        /* clear VICE modifier */
        vice_modmask &= ~(1u << bit);
        bit++;
    }
    return mask;
}


/** \brief  Translate a full modifier mask from Gdk to VICE
 *
 * \param[in]   arch_modmask    Gdk modifier mask
 *
 * \return  VICE modifier mask
 *
 * \todo    We might be able to move this into shared/hotkeys, reducing the
 *          number of virtual methods required for a UI.
 */
uint32_t ui_hotkeys_arch_modmask_from_arch(uint32_t arch_modmask)
{
    uint32_t mask = 0;
    uint32_t bit  = 0;

    while (arch_modmask != 0) {
        mask |= ui_hotkeys_arch_modifier_from_arch(arch_modmask & (1u << bit));
        arch_modmask &= ~(1u << bit);
        bit++;
    }
    return mask;
}


/* Install hotkey, see src/uiapi.h for docblock */
void ui_hotkeys_arch_install_by_map(ui_action_map_t *map)
{
    const vhk_gtk_map_t  *arch_map;
    const ui_menu_item_t *item_decl;
    gboolean              unlocked = FALSE;

    /* get arch-specific data and try to get `unlocked` property of the
     * signal handler */
    arch_map = map->user_data;
    if (arch_map != NULL) {
        item_decl = arch_map->decl;
        if (item_decl != NULL) {
            unlocked = item_decl->unlocked;
        }
    }
#if 0
    debug_gtk3("connecting accelerator");
#endif
    connect_accelerator(map->action,
                        map->arch_keysym,
                        map->arch_modmask,
                        unlocked);

    if (map->menu_item[PRIMARY_WINDOW] != NULL) {
#if 0
        debug_gtk3("got primary menu item");
#endif
        set_menu_item_accel(map->menu_item[PRIMARY_WINDOW],
                            map->arch_keysym,
                            map->arch_modmask);
    }
    /* x128 */
    if (map->menu_item[SECONDARY_WINDOW] != NULL) {
#if 0
        debug_gtk3("got secondary menu item");
#endif
        set_menu_item_accel(map->menu_item[SECONDARY_WINDOW],
                            map->arch_keysym,
                            map->arch_modmask);
    }
}


/* Remove hotkey, see src/uiapi.h for docblock */
void ui_hotkeys_arch_remove_by_map(ui_action_map_t *map)
{
#if 0
    printf("%s(): action = %d (%s)\n",
           __func__, map->action, ui_action_get_name(map->action));
    printf("%s(): vice keysym/modmask: %04x/%08x, arch keysym/modmask: %04x/%08x\n",
           __func__, map->vice_keysym, map->vice_modmask, map->arch_keysym, map->arch_modmask);
#endif
    /* remove accelator from Gtk accel group */
    vhk_gtk_remove_accelerator(map->arch_keysym, map->arch_modmask);

    /* remove accelerator labels from menu items */
    if (map->menu_item[PRIMARY_WINDOW] != NULL) {
        clear_menu_item_accel(map->menu_item[PRIMARY_WINDOW]);
    }
    /* x128 */
    if (map->menu_item[SECONDARY_WINDOW] != NULL) {
        clear_menu_item_accel(map->menu_item[SECONDARY_WINDOW]);
    }
}


/** \brief  Initialize Gtk3-specific hotkeys resources
 *
 * \note    Does *not* initialize command line options or vice resources, that
 *          is done separately in hotkeys_cmdline_options_init() and
 *          hotkeys_resources_init().
 */
void ui_hotkeys_arch_init(void)
{
}


/** \brief  Gtk3-specific hotkeys shutdown function
 *
 * Implements virtual method in uiapi.h that's called on generic hotkeys shut
 * down.
 * Frees the Gtk3-specific hotkey data in the generic hotkeys array.
 */
void ui_hotkeys_arch_shutdown(void)
{
    int action;

    for (action = 0; action < ACTION_ID_COUNT; action++) {
        ui_action_map_t *map = ui_action_map_get(action);
        if (map != NULL) {
            vhk_gtk_map_t *arch_map = map->user_data;
            if (arch_map != NULL) {
                vhk_gtk_map_free(arch_map);
            }
            map->user_data = NULL;
        }
    }

    g_object_unref(accel_group);
}
