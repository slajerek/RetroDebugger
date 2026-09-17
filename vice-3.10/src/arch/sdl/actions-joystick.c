/** \file   actions-joystick.c
 * \brief   UI action implementations for joystick (and mouse) settings (SDL)
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

#include "vice.h"

#include <stddef.h>
#include <stdbool.h>

#include "joy.h"
#include "menu_common.h"
#include "uiactions.h"
#include "uimenu.h"

#include "actions-joystick.h"


/** \brief  Toggle control port devices swap action
 *
 * \param[in]   self    action map
 */
static void swap_controlport_toggle_action(ui_action_map_t *self)
{
    sdljoy_swap_ports();
}


/** \brief  List of mappings for joystick and mouse actions
 */
static const ui_action_map_t joystick_actions[] = {
#ifdef HAVE_MOUSE
    {   .action  = ACTION_MOUSE_GRAB_TOGGLE,
        /* There's no visible indication of mouse grab being active like in the
         * Gtk3 UI, so just toggling the resource will suffice. */
        .handler = sdl_ui_toggle_resource_action,
        .data    = (void*)"Mouse"
    },
#endif
    {   .action  = ACTION_SWAP_CONTROLPORT_TOGGLE,
        .handler = swap_controlport_toggle_action
    },
    {   .action  = ACTION_KEYSET_JOYSTICK_TOGGLE,
        .handler = sdl_ui_toggle_resource_action,
        .data    = (void*)"KeySetEnable"
    },
    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register joystick/mouse actions */
void actions_joystick_register(void)
{
    ui_actions_register(joystick_actions);
}


/** \brief  Custom display callback for "swap-controlport-toggle"
 *
 * \param[in]   item    menu item (unused)
 */
const char *swap_controlport_toggle_display(ui_menu_entry_t *item)
{
    return sdljoy_get_swap_ports() ? MENU_CHECKMARK_CHECKED_STRING : NULL;
}
