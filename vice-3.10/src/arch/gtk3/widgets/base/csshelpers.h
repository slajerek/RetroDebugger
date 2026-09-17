/** \file   csshelpers.h
 *  \brief  Helper/wrapper functions for using CSS in Gtk3 code - header
 *
 *  \author Bas Wassink <b.wassink@ziggo.nl>
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

#ifndef VICE_CSSHELPERS_H
#define VICE_CSSHELPERS_H

#include <gtk/gtk.h>

GtkCssProvider *vice_gtk3_css_provider_new(const char *css);
gboolean        vice_gtk3_css_provider_add(GtkWidget *widget,
                                           GtkCssProvider *provider);
gboolean        vice_gtk3_css_provider_remove(GtkWidget *widget,
                                              GtkCssProvider *provider);
gboolean        vice_gtk3_css_add(GtkWidget *widget, const char *css);

#endif
