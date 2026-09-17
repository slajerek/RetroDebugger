/** \file   actions-cartridge.c
 * \brief   UI action implementations for cartridge-related dialogs and settings
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

/* Resources altered by this file:
 *
 */

#include "vice.h"

#include <gtk/gtk.h>
#include <stddef.h>
#include <stdbool.h>

#include "basedialogs.h"
#include "cartridge.h"
#include "resources.h"
#include "uiactions.h"
#include "uicart.h"

#include "actions-cartridge.h"


/** \brief  Show cartridge attach dialog
 *
 * \param[in]   self    action map
 */
static void cart_attach_action(ui_action_map_t *self)
{
    ui_cart_show_dialog();
}


/** \brief  Callback for the detach confirm dialog
 *
 * If \a result is TRUE, detach all carts.
 *
 * \param[in]   dialog  dialog reference (unused)
 * \param[in]   result  dialog result
 */
static void confirm_detach_callback(GtkDialog *dialog, gboolean result)
{
    if (result) {
        cartridge_unset_default();
    }
    ui_action_finish(ACTION_CART_DETACH);
}

/** \brief  Detach all cartridge images
 *
 * \param[in]   self    action map
 *
 * \todo    The question about removing the default enabled cartridge doesn't
 *          work for carts not on "slot 1", these seem to their own "enabled"
 *          resources and do not use the CartridgeType/CartridgeFile resources.
 */
static void cart_detach_action(ui_action_map_t *self)
{
    int cartid = CARTRIDGE_NONE;

    /* determine if one of the attached cartridges is set as default
       cartridge, and if so ask if it should be removed from default also */

    resources_get_int("CartridgeType", &cartid);
    if (cartid != CARTRIDGE_NONE) {
        /* default is set, ask to remove it */
        vice_gtk3_message_confirm(NULL, /* active window as parent */
                                  confirm_detach_callback,
                                  "Detach cartridge",
                                  "You're detaching the default cartridge.\n\n"
                                  "Would you also like to unregister this cartridge"
                                  " as the default cartridge?");
    } else {
        /* no dialog used, finish immediately */
        ui_action_finish(ACTION_CART_DETACH);
    }
    /* FIXME: the above will only check/ask for "slot1" cartridges. other
              cartridges have seperate "enable" resources which would
              have to be checked individually. perhaps make a dedicated
              function for this later */

    cartridge_detach_image(-1);    /* detach all cartridges */
}

/** \brief  Trigger the freeze button on the attached cartridge
 *
 * \param[in]   self    action map
 */
static void cart_freeze_action(ui_action_map_t *self)
{
    cartridge_trigger_freeze();
}


/** \brief  List of cartridge-related actions */
static const ui_action_map_t cartridge_actions[] = {
    {   .action = ACTION_CART_ATTACH,
        .handler = cart_attach_action,
        .blocks = true,
        .dialog = true
    },
    {   .action  = ACTION_CART_DETACH,
        .handler = cart_detach_action,
        .blocks  = true,
        .dialog  = true
    },
    {   .action  = ACTION_CART_FREEZE,
        .handler = cart_freeze_action
    },

    UI_ACTION_MAP_TERMINATOR
};

void actions_cartridge_register(void)
{
    ui_actions_register(cartridge_actions);
}
