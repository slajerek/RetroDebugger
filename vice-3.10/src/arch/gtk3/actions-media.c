/** \file   actions-media.c
 * \brief   UI action implementations for media recording/screenshots
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

#include <stddef.h>
#include <stdbool.h>

#include "uiactions.h"
#include "uimedia.h"
#include "screenshot.h"

#include "actions-media.h"


/** \brief  Pop up dialog to record video/audio or make a screenshot
 *
 * \param[in]   self    action map
 */
static void media_record_action(ui_action_map_t *self)
{
    ui_media_dialog_show();
}

/** \brief  Stop media recording, if active
 *
 * \param[in]   self    action map
 */
static void media_stop_action(ui_action_map_t *self)
{
    ui_media_stop_recording();
}

/** \brief  Save screenshot with auto-generated filename
 *
 * \param[in]   self    action map
 */
static void screenshot_quicksave_action(ui_action_map_t *self)
{
    screenshot_ui_auto_screenshot();
}


/** \brief  List of media actions */
static const ui_action_map_t media_actions[] = {
    {   .action  = ACTION_MEDIA_RECORD,
        .handler = media_record_action,
        .blocks  = true,
        .dialog  = true
    },
    {   /* Needs to run on the UI thread */
        .action   = ACTION_MEDIA_STOP,
        .handler  = media_stop_action,
        .uithread = true
    },
    {   /* Needs to run on the UI thread (calls ui_get_active_canvas()) */
        .action   = ACTION_SCREENSHOT_QUICKSAVE,
        .handler  = screenshot_quicksave_action,
        .uithread = true
    },

    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register media actions */
void actions_media_register(void)
{
    ui_actions_register(media_actions);
}
