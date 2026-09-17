/** \file   ramsizeradiogroup.c
 * \brief   Widget to show a list of radio buttons for RAM sizes
 *
 * Creates a grid with title and a vertically oriented radio group with
 * radio buttons with KiB/MiB/GiB labels for each ram size given.
 * Note that the sizes are taken to be in kibibytes, so an integer value
 * of 64 will result in "64KiB" and the resource value 64 (NOT 65536).
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

#include "vice.h"
#include <gtk/gtk.h>

#include "resourceradiogroup.h"
#include "widgethelpers.h"

#include "ramsizeradiogroup.h"


/** \brief  Generate radio group entries for given array
 *
 * \param[in]   sizes   sizes in KiB, terminate with <0
 *
 * \return  heap-allocated entries, owned by Gtk
 */
static vice_gtk3_radiogroup_entry_t *create_entries(const int *sizes)
{
    vice_gtk3_radiogroup_entry_t *entries;
    size_t i = 0;

    while (sizes[i] >= 0) {
        i++;
    }
    entries = g_malloc((i + 1u) * sizeof *entries);

    i = 0;
    while (sizes[i] >= 0) {
        char *name;
        int   s = sizes[i];

        if (s < (1 << 10)) {
            name = g_strdup_printf("%dKiB", s);
        } else if (s < (1 << 20)) {
            name = g_strdup_printf("%dMiB", s / (1 << 10));
        } else if (s < (1 << 30)) {
            name = g_strdup_printf("%dGiB", s / (1 << 20));
        } else {
            name = g_strdup_printf("%dTiB", s / (1 << 30));
        }

        entries[i].name = name;
        entries[i].id   = s;
        i++;
    }
    entries[i].name = NULL;
    entries[i].id   = -1;
    return entries;
}

/** \brief  Free entries
 *
 * \param[in]   entries heap-allocated list of radio group entries
 */
static void free_entries(vice_gtk3_radiogroup_entry_t *entries)
{
    size_t i;

    for (i = 0; entries[i].name != NULL; i++) {
        g_free(entries[i].name);
    }
    g_free(entries);
}

/** \brief  Create radio group with RAM size entries and a title
 *
 * Create a GtkGrid with a label in the top row set to \a title and a resource
 * radio group for \a sizes. If \a title is `NULL` a resource radio group is
 * returned without the surrounding grid.
 *
 * The \a sizes are taken as kibibytes, so plain 64 results in "64KiB" while
 * setting the \a resource to 64 (not 65536).
 *
 * \param[in]   resource    resource to set
 * \param[in]   title       title for the grid (can be `NULL`)
 * \param[in]   sizes       list of sizes in kibibytes, terminated with < 0
 *
 * \return  GtkGrid
 */
GtkWidget *ram_size_radiogroup_new(const char *resource,
                                   const char *title,
                                   const int  *sizes)
{
    GtkWidget *group;
    vice_gtk3_radiogroup_entry_t *entries;

    entries = create_entries(sizes);
    group   = vice_gtk3_resource_radiogroup_new(resource,
                                                entries,
                                                GTK_ORIENTATION_VERTICAL);
    free_entries(entries);

    if (title != NULL) {
        GtkWidget *grid;
        GtkWidget *label;
        char       buffer[256];

        g_snprintf(buffer, sizeof buffer, "<b>%s</b>", title);

        grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

        label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), buffer);
        gtk_widget_set_halign(label, GTK_ALIGN_START);

        gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);
        return grid;
    }
    return group;
}
