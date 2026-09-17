/** \file   userportrsinterfacewidget.h
 * \brief   Userport RS232 Interface widget - header
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

#ifndef VICE_USERPORTRSINTERFACEWIDGET_H
#define VICE_USERPORTRSINTERFACEWIDGET_H

#include "vice.h"

#include <gtk/gtk.h>

#define USERPORT_RS_NONINVERTED 0
#define USERPORT_RS_INVERTED    1
#define USERPORT_RS_CUSTOM      2
#define USERPORT_RS_UP9600      3


GtkWidget * userport_rsinterface_widget_create(void);

void userport_rsinterface_widget_add_callback(GtkGrid *widget, void (*cb_func)(GtkWidget *));
#endif
