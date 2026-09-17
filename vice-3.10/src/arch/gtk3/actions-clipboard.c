/** \file   actions-clipboard.c
 * \brief   UI action implementations for clipboard handling
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
#include <string.h>
#include <ctype.h>

#include "charset.h"
#include "clipboard.h"
#include "kbdbuf.h"
#include "lib.h"
#include "uiactions.h"

#include "actions-clipboard.h"


/** \brief  Copy emulated screen content to clipboard
 *
 * \param[in]   self    action map
 */
static void edit_copy_action(ui_action_map_t *self)
{
#ifdef WINDOWS_COMPILE
    char *text = clipboard_read_screen_output("\r\n");
#else
    char *text = clipboard_read_screen_output("\n");
#endif
    if (text != NULL) {
        size_t  i;
        size_t  len;
        char   *tmp;

        /* Some characters were translated to ASCII by clipboard_read_screen_output(),
         * but just about anything in the non a-zA-Z range wasn't so we need to
         * mangle the text further: */
        len = strlen(text);
        tmp = lib_malloc(len + 1u);
        for (i = 0; i < len; i++) {
            unsigned char c = (unsigned char)text[i];
            if ((c == '\r' || c == '\n') || (c < 127 && isprint(c))) {
                tmp[i] = (char)c;
            } else {
                tmp[i] = '?';
            }
        }
        tmp[len] = '\0';

        gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
                               tmp,
                               (gint)len);
        lib_free(tmp);
        lib_free(text);
    }
}


/** \brief  GtkClipboardTextReceivedTextFunc callback for the paste action
 *
 * Pastes \a text into the emulated machine via the machine's keyboard buffer.
 *
 * \param[in]   clipboard   clipboard (unused)
 * \param[in]   text        text to paste into the emulated machine
 * \param[in]   data        extra event data (unused)
 */
static void paste_callback(GtkClipboard *clipboard,
                           const gchar  *text,
                           gpointer      data)
{
    char *text_in_petscii;

    if (text == NULL) {
        return;
    }
    text_in_petscii = lib_strdup(text);

    charset_petconvstring((unsigned char*)text_in_petscii, CONVERT_TO_PETSCII);
    kbdbuf_feed(text_in_petscii);
    lib_free(text_in_petscii);
}


/** \brief  Paste clipboard content into the emulated machine
 *
 * Paste clipboard content into the emulated machine by translating the text to
 * PETSCII and feeding it to the keyboard buffer.
 *
 * \param[in]   self    action map
 */
static void edit_paste_action(ui_action_map_t *self)
{
    gtk_clipboard_request_text(gtk_clipboard_get(GDK_NONE), paste_callback, NULL);
}


/** \brief  List of clipboard actions */
static const ui_action_map_t clipboard_actions[] = {
    {   .action = ACTION_EDIT_COPY,
        .handler = edit_copy_action,
        .uithread = true
    },
    {   .action = ACTION_EDIT_PASTE,
        .handler = edit_paste_action,
        .uithread = true
    },

    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register clipboard actions */
void actions_clipboard_register(void)
{
    ui_actions_register(clipboard_actions);
}
