/** file    c128ui.c
 * \brief   Native GTK3 C128 UI
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
#include <gtk/gtk.h>

#include "c128model.h"
#include "crtcontrolwidget.h"
#include "machine.h"
#include "machinemodelwidget.h"
#include "resources.h"
#include "settings_model.h"
#include "ui.h"
#include "uimachinewindow.h"
#include "vdc.h"
#include "vicii.h"
#include "videomodelwidget.h"

#include "c128ui.h"


/** \brief  List of C128 models
 *
 * Used in the machine-model widget
 */
static const char *c128_model_list[] = {
    "C128 PAL",
    "C128D PAL",
    "C128DCR PAL",
    "C128 NTSC",
    "C128D NTSC",
    "C128DCR NTSC",
    NULL
};

/** \brief  List of VIC-II models
 *
 * Used in the VIC-II model widget
 */
static const vice_gtk3_radiogroup_entry_t c128_vicii_models[] = {
    { "PAL",    MACHINE_SYNC_PAL },
    { "NTSC",   MACHINE_SYNC_NTSC },
    { NULL,     -1 }
};


/** \brief  Identify the canvas used to create a window
 *
 * \param[in]   canvas  video canvas
 *
 * \return  window index on success, -1 on failure
 */
static int identify_canvas(video_canvas_t *canvas)
{
    /* XXX: Functions in the common code number the
     *      windows in the opposite order.
     */
    if (canvas == vicii_get_canvas()) {
        return PRIMARY_WINDOW;
    }
    if (canvas == vdc_get_canvas()) {
        return SECONDARY_WINDOW;
    }

    return -1;
}

/** \brief  Create CRT controls widget for \a target window
 *
 * \param[in]   target_window   target window index
 *
 * \return  GtkGrid
 */
static GtkWidget *create_crt_widget(int target_window)
{
    if (target_window == PRIMARY_WINDOW) {
        return crt_control_widget_create(NULL, "VICII", TRUE);
    } else {
        return crt_control_widget_create(NULL, "VDC", TRUE);
    }
}

/** \brief  Pre-initialize the UI before the canvas windows get created
 *
 * \return  0 on success, -1 on failure
 */
int c128ui_init_early(void)
{
    ui_machine_window_init();
    ui_set_identify_canvas_func(identify_canvas);
    ui_set_create_controls_widget_func(create_crt_widget);
    return 0;
}


/** \brief  Initialize the UI
 *
 * \return  0 on success, -1 on failure
 */
int c128ui_init(void)
{
    GtkWidget *window;
    int        forty = 0;
    int        hide_vdc = 0;

    machine_model_widget_getter(c128model_get);
    machine_model_widget_setter(c128model_set);
    machine_model_widget_set_models(c128_model_list);

    video_model_widget_set_title("VIC-II model");
    video_model_widget_set_resource("MachineVideoStandard");
    video_model_widget_set_models(c128_vicii_models);

    /* set model getter for the model settings dialog */
    settings_model_widget_set_model_func(c128model_get);

    /* push VDC display to front depending on 40/80 key */
    resources_get_int("C128ColumnKey", &forty);
    if (forty) {
        window = ui_get_window_by_index(PRIMARY_WINDOW); /* VICIIe */
    } else {
        window = ui_get_window_by_index(SECONDARY_WINDOW); /* VDC */
    }
    if (window != NULL) {
        gtk_window_present(GTK_WINDOW(window));
    }

    /* Hide VDC window, ignoring the stuff before this */
    resources_get_int("C128HideVDC", &hide_vdc);
    if (hide_vdc) {
        window = ui_get_window_by_index(SECONDARY_WINDOW); /* VDC */
        if (window != NULL) {
            gtk_widget_hide(window);
        }
    }

    return 0;
}


/** \brief  Shut down the UI
 */
void c128ui_shutdown(void)
{
    /* NOP */
}
