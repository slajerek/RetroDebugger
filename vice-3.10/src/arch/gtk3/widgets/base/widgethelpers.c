/** \file   widgethelpers.c
 * \brief   Helpers for creating Gtk3 widgets
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * This file is supposed to contain some helper functions for boiler plate
 * code, such as creating layout widgets, creating lists of radio or check
 * boxes, etc.
 *
 * \todo    turn the margin/padding values into defines and move into a file
 *          like uidefs.h (partially done, \a see vice_gtk3_settings.h)
 *
 * \todo    rename/replace functions (partially done)
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
#include <string.h>
#include <stdbool.h>

#include "lib.h"
#include "resources.h"

#include "vice_gtk3_settings.h"
#include "debug_gtk3.h"

#include "widgethelpers.h"


/** \brief  Get index of \a value in \a list
 *
 * Get the index in \a list for \a value. This function is required for custom
 * radiogroups that add 'unknown' options or something similar.
 *
 * \param[in]   list    radio button group array
 * \param[in]   value   value to find in \a list
 *
 * \return  index of \a value or -1 when not found
 */
int vice_gtk3_radiogroup_get_list_index(
        const vice_gtk3_radiogroup_entry_t *list,
        int value)
{
    int i;

    for (i = 0; list[i].name != NULL; i++) {
        if (list[i].id == value) {
            return i;
        }
    }
    return -1;
}


/** \brief  Set a radio button to active in a GktGrid
 *
 * This function only checks for radio buttons in the first row of the \a grid,
 * so it works fine with widgets created through
 * uihelpers_uihelpers_create_int_radiogroup_with_label(), but not much else.
 * So it might need some refactoring
 *
 * \param[in]   grid    GtkGrid containing radio buttons
 * \param[in]   index   index of the radio button (the actual index of the
 *                      radio button, other widgets are skipped)
 */
void vice_gtk3_radiogroup_set_index(GtkWidget *grid, int index)
{
    GtkWidget *radio;
    int row = 0;
    int radio_index = 0;

    if (index < 0) {
        debug_gtk3("Warning: negative index given, giving up.");
        return;
    }

    do {
        radio = gtk_grid_get_child_at(GTK_GRID(grid), 0, row);
        if (GTK_IS_TOGGLE_BUTTON(radio)) {
            if (radio_index == index) {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
                return;
            }
            radio_index++;
        }
        row++;
    } while (radio != NULL);
}


/** \brief  Create a left-aligned, 16 units indented label
 *
 * XXX: This function is of little use an should probably be removed in favour
 *      of something a little more flexible.
 *
 * \param[in]   text    label text
 *
 * \return  GtkLabel
  */
GtkWidget *vice_gtk3_create_indented_label(const char *text)
{
    GtkWidget *label = gtk_label_new(text);

    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 16);
    return label;
}


/** \brief  Create a new `GtkGrid`, setting column and row spacing
 *
 * \param[in]   column_spacing  column spacing (< 0 to use default)
 * \param[in]   row_spacing     row spacing (< 0 to use default)
 *
 * \return  GtkGrid
 */
GtkWidget *vice_gtk3_grid_new_spaced(int column_spacing, int row_spacing)
{
    GtkWidget *grid = gtk_grid_new();

    gtk_grid_set_column_spacing(GTK_GRID(grid),
            column_spacing < 0 ? VICE_GTK3_GRID_COLUMN_SPACING : column_spacing);
    gtk_grid_set_row_spacing(GTK_GRID(grid),
            row_spacing < 0 ? VICE_GTK3_GRID_ROW_SPACING : row_spacing);
    return grid;
}


/** \brief  Create a new `GtkGrid` with label, setting column and row spacing
 *
 * \param[in]   column_spacing  column spacing (< 0 to use default)
 * \param[in]   row_spacing     row spacing (< 0 to use default)
 * \param[in]   label           label text
 * \param[in]   span            number of columns for the \a label to span
 *
 * \return  GtkGrid
 */
GtkWidget *vice_gtk3_grid_new_spaced_with_label(int column_spacing,
                                                int row_spacing,
                                                const char *label,
                                                int span)
{
    GtkWidget *grid = vice_gtk3_grid_new_spaced(column_spacing, row_spacing);
    GtkWidget *lbl = gtk_label_new(NULL);
    char *temp;

    if (span < 1) {
        span = 1;
    }

    /* create left-indented bold label */
    temp = lib_msprintf("<b>%s</b>", label);
    gtk_label_set_markup(GTK_LABEL(lbl), temp);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    lib_free(temp);

    /* attach label */
    gtk_grid_attach(GTK_GRID(grid), lbl, 0, 0, span, 1);
    gtk_widget_show(grid);
    return grid;
}


/** \brief  Set 'margin-bottom' property of the title of a grid with title
 *
 * Since we've switched to using a lot of grids with 0 row spacing, it's often
 * desired to have a little space between a grid's title and its content.
 *
 * \param[in]   grid    GtkGrid created with vice_gtk3_grid_new_spaced_with_label()
 * \param[in]   margin  bottom-margin property value
 */
void vice_gtk3_grid_set_title_margin(GtkWidget *grid, int margin)
{
    if (grid != NULL && GTK_IS_GRID(grid)) {
        GtkWidget *title = gtk_grid_get_child_at(GTK_GRID(grid), 0, 0);
        if (title != NULL && GTK_IS_LABEL(title)) {
            gtk_widget_set_margin_bottom(title, margin);
        }
    }
}


/** \brief  Set margin on \a grid
 *
 * Set margins on a GtkGrid. passing a value of <0 means skipping that property.
 *
 * \param[in,out]   grid    GtkGrid instance
 * \param[in]       top     top margin
 * \param[in]       bottom  bottom margin
 * \param[in]       start   start (left) margin
 * \param[in]       end     end (right) margin
 *
 */
void vice_gtk3_grid_set_margins(GtkWidget *grid,
                                gint top,
                                gint bottom,
                                gint start,
                                gint end)
{
    if (top >= 0) {
        gtk_widget_set_margin_top(grid, top);
    }
    if (bottom >= 0) {
        gtk_widget_set_margin_bottom(grid, bottom);
    }
    if (start >= 0) {
        gtk_widget_set_margin_start(grid, start);
    }
    if (end >= 0) {
        gtk_widget_set_margin_end(grid, end);
    }
}

