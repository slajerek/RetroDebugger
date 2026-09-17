/** \file   resourcecombobox.h
 * \brief   Combo box connected to a resource - header
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

#ifndef VICE_RESOURCECOMBOBOX_H
#define VICE_RESOURCECOMBOBOX_H

#include "vice.h"
#include <gtk/gtk.h>
#include "basewidget_types.h"

/*
 * Integer resource combo box, using integer for keys and strings for display.
 */

GtkWidget *vice_gtk3_resource_combo_int_new(
        const char *resource,
        const vice_gtk3_combo_entry_int_t *entries);

GtkWidget *vice_gtk3_resource_combo_int_new_sprintf(
        const char *fmt,
        const vice_gtk3_combo_entry_int_t *entries,
        ...);

void     vice_gtk3_resource_combo_int_append (GtkWidget  *widget,
                                              int         id,
                                              const char *value);
gboolean vice_gtk3_resource_combo_int_set    (GtkWidget *widget, int id);
gboolean vice_gtk3_resource_combo_int_factory(GtkWidget *widget);
gboolean vice_gtk3_resource_combo_int_reset  (GtkWidget *widget);
gboolean vice_gtk3_resource_combo_int_sync   (GtkWidget *widget);

/*
 * Integer combo box presenting values as hexadecimal literals.
 */

GtkWidget *vice_gtk3_resource_combo_hex_new        (const char *resource);
GtkWidget *vice_gtk3_resource_combo_hex_new_list   (const char *resource,
                                                    const int  *list);
GtkWidget *vice_gtk3_resource_combo_hex_new_range  (const char *resource,
                                                    int         lower,
                                                    int         upper,
                                                    int         step);
void       vice_gtk3_resource_combo_hex_append     (GtkWidget *combo, int value);
gboolean   vice_gtk3_resource_combo_hex_set        (GtkWidget *combo, int id);
gboolean   vice_gtk3_resource_combo_hex_factory    (GtkWidget *combo);
gboolean   vice_gtk3_resource_combo_hex_reset      (GtkWidget *combo);
gboolean   vice_gtk3_resource_combo_hex_sync       (GtkWidget *combo);


/*
 * String resource combo box API
 */

GtkWidget *vice_gtk3_resource_combo_str_new(
        const char *resource,
        const vice_gtk3_combo_entry_str_t *entries);

GtkWidget *vice_gtk3_resource_combo_str_new_sprintf(
        const char *fmt,
        const vice_gtk3_combo_entry_str_t *entries,
        ...);

void     vice_gtk3_resource_combo_str_append (GtkWidget  *widget,
                                              const char *id,
                                              const char *value);
gboolean vice_gtk3_resource_combo_str_set    (GtkWidget *widget, const char *id);
gboolean vice_gtk3_resource_combo_str_factory(GtkWidget *widget);
gboolean vice_gtk3_resource_combo_str_reset  (GtkWidget *widget);
gboolean vice_gtk3_resource_combo_str_sync   (GtkWidget *widget);

#endif
