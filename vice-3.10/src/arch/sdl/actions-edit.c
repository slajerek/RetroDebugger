/** \file   actions-edit.c
 * \brief   UI action implementations for clipboard operations
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

#ifdef USE_SDL2UI

#include "vice_sdl.h"
#include "charset.h"
#include "clipboard.h"
#include "kbdbuf.h"
#include "lib.h"
#include "uiactions.h"
#include "uimenu.h"

#include "actions-edit.h"


/** \brief  Paste host clipboard into emulated machine action
 *
 * Paste the host's clipboard contents into the emulated machine by feeding
 * the content to the machine's keyboard buffer.
 *
 * \param[in]   self    action map
 */
static void edit_paste_action(ui_action_map_t *self)
{
    char *text_in_petscii;
    char *text = SDL_GetClipboardText();

    if (text == NULL) {
        return;
    }
    text_in_petscii = lib_strdup(text);

    charset_petconvstring((unsigned char*)text_in_petscii, CONVERT_TO_PETSCII);
    kbdbuf_feed(text_in_petscii);
    lib_free(text_in_petscii);
    SDL_free(text);
}

/** \brief  Copy emulated machine's screen to host clipboard action
 *
 * Copy the emulated machine's screen contents to the host's clipboard.
 *
 * \param[in]   self    action map
 */
static void edit_copy_action(ui_action_map_t *self)
{
    char *text = clipboard_read_screen_output("\n");
    if (text != NULL) {
        SDL_SetClipboardText(text);
        lib_free(text);
    }
}


/** \brief  List of mappings for editing actions */
static const ui_action_map_t edit_actions[] = {
    {   .action  = ACTION_EDIT_PASTE,
        .handler = edit_paste_action
    },
    {   .action  = ACTION_EDIT_COPY,
        .handler = edit_copy_action
    },
    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register editing actions */
void actions_edit_register(void)
{
    ui_actions_register(edit_actions);
}

#endif  /* ifdef USE_SDL2UI */
