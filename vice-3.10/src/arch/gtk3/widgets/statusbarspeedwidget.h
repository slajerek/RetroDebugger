/** \file   statusbarspeedwidget.h
 * \brief   CPU speed, FPS display widget for the statusbar - header
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

#ifndef VICE_STATUSBARSPEEDWIDGET_H
#define VICE_STATUSBARSPEEDWIDGET_H

#include "vice.h"
#include <gtk/gtk.h>

#include "archdep.h"
#include "vice_gtk3.h"

/** \brief Used to optimise display updates
 */
typedef struct statusbar_speed_widget_state_s {
    tick_t last_render_tick;
    int last_cpu_int;
    int last_fps_int;
    int last_warp;
    int last_paused;
    int last_shiftlock;
    int last_mode4080;
    int last_capslock;
    int last_diagnostic_pin;
} statusbar_speed_widget_state_t;

GtkWidget *speed_menu_popup_create(void);

GtkWidget *statusbar_speed_widget_create(statusbar_speed_widget_state_t *state);

void statusbar_speed_widget_update(GtkWidget *widget, statusbar_speed_widget_state_t *state, int window_identity);

#endif

