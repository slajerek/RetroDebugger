/** \file   actions-printer.c
 * \brief   UI action implementations for printers
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

#include "vice.h"

#include "debug_gtk3.h"
#include "printer.h"
#include "types.h"
#include "uiactions.h"

#include "actions-printer.h"


/** \brief  Send formfeed to print action
 *
 * \param[in]   self    action map
 */
static void printer_formfeed_action(ui_action_map_t *self)
{
    printer_formfeed(vice_ptr_to_int(self->data));
}

/** \brief  Printer actions */
static const ui_action_map_t printer_actions[] = {
    {   .action  = ACTION_PRINTER_FORMFEED_4,
        .handler = printer_formfeed_action,
        .data    = vice_int_to_ptr(PRINTER_IEC_4)
    },
    {   .action  = ACTION_PRINTER_FORMFEED_5,
        .handler = printer_formfeed_action,
        .data    = vice_int_to_ptr(PRINTER_IEC_5)
    },
    {   .action  = ACTION_PRINTER_FORMFEED_6,
        .handler = printer_formfeed_action,
        .data    = vice_int_to_ptr(PRINTER_IEC_6)
    },
    {   .action  = ACTION_PRINTER_FORMFEED_USERPORT,
        .handler = printer_formfeed_action,
        .data    = vice_int_to_ptr(PRINTER_USERPORT)
    },

    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register printer-related UI action handlers
 */
void actions_printer_register(void)
{
    ui_actions_register(printer_actions);
}
