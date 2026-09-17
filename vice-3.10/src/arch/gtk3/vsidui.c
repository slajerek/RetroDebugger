/** \file   vsidui.c
 * \brief   Native GTK3 VSID UI
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include <stdio.h>

#include "vice_gtk3.h"
#include "lib.h"
#include "machine.h"
#include "ui.h"
#include "uisidattach.h"
#include "uivsidwindow.h"
#include "vicii.h"
#include "resources.h"

#include "actions-vsid.h"
#include "videomodelwidget.h"
#include "vsidcontrolwidget.h"
#include "vsidtuneinfowidget.h"
#include "vsidmainwidget.h"
#include "hvsc.h"

#include "vsidui.h"


/** \brief  Video standard list for the model settings
 */
static const vice_gtk3_radiogroup_entry_t vsid_vicii_models[] = {
    { "PAL",            MACHINE_SYNC_PAL },
    { "NTSC",           MACHINE_SYNC_NTSC },
    { "NTSC (old)",     MACHINE_SYNC_NTSCOLD },
    { "PAL-N/Drean",    MACHINE_SYNC_PALN },
    { NULL,             -1 }
};



void vsid_ui_close(void)
{
    hvsc_exit();
    uisidattach_shutdown();
}


/** \brief  Identify the canvas used to create a window
 *
 * \return  window index on success, -1 on failure
 */
static int identify_canvas(video_canvas_t *canvas)
{
    if (canvas != vicii_get_canvas()) {
        return -1;
    }
    return PRIMARY_WINDOW;
}



/** \brief  Initialize the VSID UI
 *
 * \return  0 on success, -1 on failure
 */
int vsid_ui_init(void)
{
    video_canvas_t *canvas;

    canvas = vicii_get_canvas();

    video_model_widget_set_title("VIC-II model");
    video_model_widget_set_resource("MachineVideoStandard");
    video_model_widget_set_models(vsid_vicii_models);

    ui_vsid_window_init();
    ui_set_identify_canvas_func(identify_canvas);
    ui_create_main_window(canvas);
    ui_display_main_window(canvas->window_index);

    /* We cannot call this from the generic ui.c due to linker errors, so we
     * call it here and hope for the best =) */
    actions_vsid_register();

    /* Set initial state of checkboxes */
    actions_vsid_setup_ui();
    return 0;
}
