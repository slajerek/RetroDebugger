/** \file   basewidget_types.h
 * \brief   Types used for the base widgets
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 *
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

#ifndef VICE_BASEWIDGETTYPES_H
#define VICE_BASEWIDGETTYPES_H

#include "vice.h"
#include <gtk/gtk.h>

/** \def    GULONG_TO_POINTER
 * \brief   Cast gulong to pointer
 *
 * GLib appears to be "missing" this macro, so we define it here.
 *
 * \param[in]   ul  gulong value
 *
 * \return  gpointer
 */

#ifndef GULONG_TO_POINTER
# define GULONG_TO_POINTER(ul) (gpointer)(uintptr_t)(ul)
#endif

/** \def    GPOINTER_TO_ULONG
 * \brief   Cast pointer to gulong
 *
 * GLib appears to be "missing" this macro, so we define it here.
 *
 * \param[in]   p   gpointer
 *
 * \return  gulong
 */

#ifndef GPOINTER_TO_ULONG
# define GPOINTER_TO_ULONG(p) (gulong)(uintptr_t)(p)
#endif

/** \brief  Entry for a combo box using an integer as ID
 */
typedef struct vice_gtk3_combo_entry_int_s {
    char *name;     /**< displayed in the combo box */
    int   id;       /**< ID for the entry in the combo box */
} vice_gtk3_combo_entry_int_t;

/** \brief  combo box int entry list terminator */
#define VICE_GTK3_COMBO_ENTRY_INT_LIST_END { NULL, -1 }


/** \brief  Entry for a combo box using a string as ID
 */
typedef struct vice_gtk3_combo_entry_str_s {
    char *id;       /**< ID for the entry in the combo box */
    char *value;    /**< displayed value in the combo box */
} vice_gtk3_combo_entry_str_t;

/** \brief  combo box string entry list terminator */
#define VICE_GTK3_COMBO_ENTRY_STR_LIST_END { NULL, NULL }


/** \brief  Entry for a radio button group using an integer as ID
 */
typedef struct vice_gtk3_radiogroup_entry_s {
    char *name;     /**< label for the radio button */
    int   id;       /**< ID for the radio button */
} vice_gtk3_radiogroup_entry_t;

/** \brief  radiogroup entry list terminator */
#define VICE_GTK3_RADIOGROUP_ENTRY_LIST_END { NULL, -1 }

#endif
