/** \file   kbdhostlayoutwidget.c
 * \brief   GTK3 host keyboard layout widget for the settings dialog
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES KeyboardMapping     -vsid
 *
 * sets indirectly:
 * $VICERES KeymapPosFile       -vsid
 * $VICERES KeymapSymFile       -vsid
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


#include "vice.h"
#include <gtk/gtk.h>

#include "kbdmappingwidget.h"
#include "keyboard.h"
#include "keymap.h"
#include "lib.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "kbdhostlayoutwidget.h"


/** \brief  Handler for the 'changed' event of the widget
 *
 * Updates the symbolic/positional radio buttons' sensitivity.
 *
 * \param[in]   combo   combo box (unused)
 * \param[in]   data    extra user data (unused)
 */
static void on_combo_changed(GtkWidget *combo, gpointer data)
{
    /* update widget so sym/pos is greyed out correctly */
    kbdmapping_widget_update();
}

/** \brief  Generate list of entries for combo box from list of host mappings
 *
 * \return  list of entries, free with lib_free()
 */
static vice_gtk3_combo_entry_int_t *create_combo_entries(void)
{
    const mapping_info_t        *mappings;
    vice_gtk3_combo_entry_int_t *entries = NULL;

    mappings = keyboard_get_info_list();
    if (mappings != NULL) {
        int num;
        int m;
        int e;

        num = keyboard_get_num_mappings();
        entries = lib_malloc((size_t)(num + 1) * sizeof *entries);
        for (m = 0, e = 0; mappings[m].name != NULL; m++) {
            if (keyboard_is_hosttype_valid(mappings[m].mapping) == 0) {
                entries[e].id   = mappings[m].mapping;
                entries[e].name = mappings[m].name;
                e++;
            }
        }
        entries[e].id   = -1;
        entries[e].name = NULL;
    }
    return entries;
}


/** \brief  Create a keyboard layout selection widget
 *
 * \return  GtkGrid
 */
GtkWidget *kbdhostlayout_widget_create(void)
{
    GtkWidget                   *grid;
    GtkWidget                   *label;
    GtkWidget                   *combo;
    vice_gtk3_combo_entry_int_t *entries;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Host keyboard layout</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    entries = create_combo_entries();
    combo = vice_gtk3_resource_combo_int_new("KeyboardMapping", entries);
    lib_free(entries);
    gtk_widget_set_hexpand(combo, TRUE);
    gtk_grid_attach(GTK_GRID(grid), combo, 0, 1, 1, 1);
    g_signal_connect(G_OBJECT(combo),
                     "changed",
                     G_CALLBACK(on_combo_changed),
                     NULL);

    gtk_widget_show_all(grid);
    return grid;
}
