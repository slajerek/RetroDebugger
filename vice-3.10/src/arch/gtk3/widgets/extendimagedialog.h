/** \file   extendimagedialog.h
 * \brief   Gtk3 extend image dialog dialog - header
 *
 * \author  groepaz <groepaz@gmx.net>
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

#ifndef VICE_EXTENDIMAGEDIALOG_H
#define VICE_EXTENDIMAGEDIALOG_H

#include "vice.h"
#include <gtk/gtk.h>
#include "uiapi.h"

ui_extendimage_action_t extendimage_dialog(GtkWidget *parent, const char *msg);

#endif
