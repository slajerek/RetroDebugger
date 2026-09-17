/** \file   statusbarledwidget.h
 * \brief   Small LED widgets for the status bar - header
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

#ifndef VICE_STATUSBARLEDWIDGET_H
#define VICE_STATUSBARLEDWIDGET_H

#include <gtk/gtk.h>

GtkWidget * statusbar_led_widget_create(const gchar *text,
                                        const gchar *on,
                                        const gchar *off);

void        statusbar_led_widget_set_toggleable(GtkWidget *widget,
                                                gboolean toggleable);
gboolean    statusbar_led_widget_get_toggleable(GtkWidget *widget);
void        statusbar_led_widget_set_toggle_callback(GtkWidget *widget,
                                                     void (*cb)(GtkWidget *, gboolean));

gboolean    statusbar_led_widget_get_active(GtkWidget *widget);
void        statusbar_led_widget_set_active(GtkWidget *widget, gboolean active);

#endif
