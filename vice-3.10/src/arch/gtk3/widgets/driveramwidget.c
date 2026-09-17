/** \file   driveramwidget.c
 * \brief   Drive RAM expansions widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/* TODO: check drives against emus to turn this into a proper table:
 *  Drive[8-11]RAM2000 (1540, 1541 and 1541-II)
 *  Drive[8-11]RAM4000 (1540, 1541, 1541-II, 1570, 1571 and 1751CR)
 *  Drive[8-11]RAM6000 (1540, 1541, 1541-II, 1570, 1571 and 1751CR)
 *  Drive[8-11]RAM8000 (1540, 1541 and 1541-II)
 *  Drive[8-11]RAMA000 (1540, 1541 and 1541-II)
 *
 * This probably not quite correct:
 *
 * $VICERES Drive8RAM2000       -vsid
 * $VICERES Drive9RAM2000       -vsid
 * $VICERES Drive10RAM2000      -vsid
 * $VICERES Drive11RAM2000      -vsid
 * $VICERES Drive8RAM4000       -vsid
 * $VICERES Drive9RAM4000       -vsid
 * $VICERES Drive10RAM4000      -vsid
 * $VICERES Drive11RAM4000      -vsid
 * $VICERES Drive8RAM6000       -vsid
 * $VICERES Drive9RAM6000       -vsid
 * $VICERES Drive10RAM6000      -vsid
 * $VICERES Drive11RAM6000      -vsid
 * $VICERES Drive8RAM8000       -vsid
 * $VICERES Drive9RAM8000       -vsid
 * $VICERES Drive10RAM8000      -vsid
 * $VICERES Drive11RAM8000      -vsid
 * $VICERES Drive8RAMA000       -vsid
 * $VICERES Drive9RAMA000       -vsid
 * $VICERES Drive10RAMA000      -vsid
 * $VICERES Drive11RAMA000      -vsid
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

#include "drive-check.h"
#include "drive.h"
#include "machine.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "driveramwidget.h"


/** \brief  Data on placement and memory location of drive RAM expansions */
typedef struct drive_ram_exp_s {
    int      column;        /**< column */
    int      row;           /**< row */
    uint16_t base;          /**< memory location base */
    int      (*valid)(int); /**< function checking if the expansion is valid
                                 for the current drive model */
} drive_ram_exp_t;


/** \brief  Number of possibile RAM expansions */
#define RAM_EXP_COUNT 5


/** \brief  List of RAM expansions */
static const drive_ram_exp_t expansions[RAM_EXP_COUNT] = {
    { 0, 0, 0x2000, drive_check_expansion2000 },
    { 0, 1, 0x4000, drive_check_expansion4000 },
    { 0, 2, 0x6000, drive_check_expansion6000 },
    { 1, 0, 0x8000, drive_check_expansion8000 },
    { 1, 1, 0xa000, drive_check_expansionA000 }
};



/** \brief  Create drive RAM expansion check button
 *
 * \param[in]   unit    unit number (8-11)
 * \param[in]   base    RAM base address (word)
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_ram_check_button(int unit, unsigned int base)
{
    gchar label[256];

    g_snprintf(label, sizeof label, "$%04X-$%04X", base, base + 0x1fff);
    return vice_gtk3_resource_check_button_new_sprintf("Drive%dRAM%04X",
                                                       label,
                                                       unit, base);
}


/** \brief  Create extra drive RAM expansions widget
 *
 * Create a group of check buttons to enable/disable RAM expansions in a drive.
 *
 * The check buttons that can be toggled is dependent on the drive type, see
 * drive_ram_widget_update().
 *
 * \param[in]   unit    drive unit (8-11)
 *
 * \return  GtkGrid
 */
GtkWidget *drive_ram_widget_create(int unit)
{
    GtkWidget *grid;
    int        type = 0;

    /* create grid with header */
    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, "RAM expansions", 1);
    vice_gtk3_grid_set_title_margin(grid, 8);
    /* store unit number (is this actually used?) */
    g_object_set_data(G_OBJECT(grid), "UnitNumber", GINT_TO_POINTER(unit));


    /* add RAM check buttons */
    for (int i = 0; i < RAM_EXP_COUNT; i++) {
        GtkWidget *check = create_ram_check_button(unit, expansions[i].base);
        gtk_grid_attach(GTK_GRID(grid),
                        check,
                        expansions[i].column, expansions[i].row + 1, 1, 1);
    }

    /* set initial sensitivity of check buttons based on drive type */
    resources_get_int_sprintf("Drive%dType", &type, unit);
    drive_ram_widget_update(grid, type);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Update sensitivity of check buttons based on drive model
 *
 * \param[in]   widget  drive RAM expansions widget
 * \param[in]   model   drive model ID
 */
void drive_ram_widget_update(GtkWidget *widget, int model)
{
    for (int i = 0; i < RAM_EXP_COUNT; i++) {
        GtkWidget *check = gtk_grid_get_child_at(GTK_GRID(widget),
                                                 expansions[i].column,
                                                 expansions[i].row + 1);
        if (check != NULL) {
            gtk_widget_set_sensitive(check,
                                     expansions[i].valid(model));
        }
    }
}
