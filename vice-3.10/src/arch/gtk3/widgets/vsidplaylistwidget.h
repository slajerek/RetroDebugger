/** \file   vsidplaylistwidget.h
 * \brief   GTK3 playlist widget for VSID - header
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

#ifndef VICE_VSIDPLAYLISTWIDGET_H
#define VICE_VSIDPLAYLISTWIDGET_H

#include <gtk/gtk.h>

GtkWidget * vsid_playlist_widget_create(void);

gboolean    vsid_playlist_append_file(const gchar *path);
void        vsid_playlist_remove_file(gint row);

void        vsid_playlist_first(void);
void        vsid_playlist_previous(void);
void        vsid_playlist_next(void);
void        vsid_playlist_last(void);

void        vsid_playlist_add(void);
void        vsid_playlist_remove(gint row);
void        vsid_playlist_remove_selection(void);
void        vsid_playlist_clear(void);
void        vsid_playlist_load(void);
void        vsid_playlist_save(void);

gint        vsid_playlist_get_current_row(const gchar **path);

#endif
