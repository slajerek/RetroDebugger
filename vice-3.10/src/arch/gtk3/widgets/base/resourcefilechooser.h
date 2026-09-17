/** \file   resourcefilechooser.h
 * \brief   Resource file chooser widget - header
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

#ifndef VICE_RESOURCEFILECHOOSER_H
#define VICE_RESOURCEFILECHOOSER_H

#include <gtk/gtk.h>

GtkWidget *vice_gtk3_resource_filechooser_new(const char           *resource,
                                              GtkFileChooserAction  action);
GtkWidget *vice_gtk3_resource_filechooser_new_sprintf(const char           *format,
                                                      GtkFileChooserAction  action,
                                                      ...);
gboolean   vice_gtk3_resource_filechooser_set(GtkWidget  *widget,
                                              const char *pathname);
void       vice_gtk3_resource_filechooser_set_confirm(GtkWidget *widget,
                                                      gboolean   confirm_overwrite);
void       vice_gtk3_resource_filechooser_set_directory(GtkWidget  *widget,
                                                        const char *directory);
void       vice_gtk3_resource_filechooser_set_filter(GtkWidget   *widget,
                                                     const char  *name,
                                                     const char **patterns,
                                                     gboolean     show_patterns);
void       vice_gtk3_resource_filechooser_set_custom_title(GtkWidget  *widget,
                                                           const char *title);
void       vice_gtk3_resource_filechooser_set_callback(GtkWidget *widget,
                                                       void (*callback)(GtkEntry *, gchar *));

void       vice_gtk3_resource_filechooser_sync(GtkWidget *widget);

#endif
