/** \file   statusbarrecordingwidget.h
 * \brief   Widget to display and control recording of events/audio/video - header
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

#ifndef VICE_STATUSBAR_RECORDING_WIDGET_H
#define VICE_STATUSBAR_RECORDING_WIDGET_H

#include <gtk/gtk.h>

GtkWidget *statusbar_recording_widget_create(void);
void statusbar_recording_widget_set_recording_status(GtkWidget *widget,
                                                     int status);
void statusbar_recording_widget_set_time(GtkWidget *widget,
                                         unsigned int current,
                                         unsigned int total);


void statusbar_recording_widget_set_event_playback(GtkWidget *widget,
                                                   char *version);

void statusbar_recording_widget_hide_all(GtkWidget *widget, guint timeout);

#endif
