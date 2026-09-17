/** \file   ciamodelwidget.c
 * \brief   Widget to set the CIA model
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES CIA1Model   x64 x64sc xscpu64 x64dtv x128 xcbm5x0 xcbm2 vsid
 * $VICERES CIA2Model   x64 x64sc xscpu64 x64dtv x128 vsid
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

#include "cia.h"
#include "vice_gtk3.h"

#include "ciamodelwidget.h"


/** List of CIA models
 */
static const vice_gtk3_radiogroup_entry_t cia_models[] = {
    { "6526 (old)", CIA_MODEL_6526  },
    { "8521 (new)", CIA_MODEL_6526A },
    { NULL,         -1 }
};


/** \brief  Reference to the radiobutton group used for CIA1
 */
static GtkWidget *cia1_group;

/** \brief  Reference to the radiobutton group used for CIA2
 */
static GtkWidget *cia2_group;

/** \brief  Optional extra callback for widget changes
 */
static void (*cia_model_callback)(int, int);


/** \brief  Handler for the radiogroup callbacks for the CIA models
 *
 * Triggers the callback set with cia_model_widget_set_callback()
 *
 * \param[in]   widget  CIA radiogroup widget triggering the event
 * \param[in]   model   CIA model ID
 */
static void cia_model_callback_internal(GtkWidget *widget, int model)
{
    if (cia_model_callback != NULL) {
        cia_model_callback(widget == cia1_group ? 1 : 2, model);
    }
}

/** \brief  Properly clear references
 *
 * \param[in]   self    CIA widget (unused)
 * \param[in]   data    extra event data (unused)
 */
static void on_destroy(GtkWidget *self, gpointer data)
{
    cia_model_callback = NULL;
    cia1_group         = NULL;
    cia2_group         = NULL;
}

/** \brief  Create a widget for CIA \a num
 *
 * \param[in]   num     CIA number (1 or 2)
 *
 * \return  GtkGrid
 */
static GtkWidget *create_cia_widget(int num)
{
    GtkWidget *grid;
    GtkWidget *radio_group;
    GtkWidget *label;
    char buffer[64];

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

    g_snprintf(buffer, sizeof(buffer), "CIA%d", num);
    label = gtk_label_new(buffer);
    gtk_widget_set_margin_start(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    radio_group = vice_gtk3_resource_radiogroup_new_sprintf("CIA%dModel",
                                                            cia_models,
                                                            GTK_ORIENTATION_HORIZONTAL,
                                                            num);
    vice_gtk3_resource_radiogroup_add_callback(radio_group,
                                               cia_model_callback_internal);
    if (num == 1) {
        cia1_group = radio_group;
    } else {
        cia2_group = radio_group;
    }
    gtk_grid_attach(GTK_GRID(grid), radio_group, 1, 0, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create CIA model(s) widget
 *
 * Creates a CIA model widget for either one or two CIA's.
 *
 * \param[in]       count                   number of CIA's (1 or 2)
 *
 * \return  GtkGrid
 */
GtkWidget *cia_model_widget_create(int count)
{
    GtkWidget *grid;
    GtkWidget *cia1_widget;
    GtkWidget *cia2_widget;

    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, "CIA model", 1);
    vice_gtk3_grid_set_title_margin(grid, 8);

    cia1_widget = create_cia_widget(1);
    gtk_grid_attach(GTK_GRID(grid), cia1_widget, 0, 1, 1, 1);
    if (count > 1) {
        cia2_widget = create_cia_widget(2);
        gtk_grid_attach(GTK_GRID(grid), cia2_widget, 0, 2, 1, 1);
    }

    g_signal_connect_unlocked(grid,
                              "destroy",
                              G_CALLBACK(on_destroy),
                              NULL);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Synchronize CIA model widgets with their current resources
 *
 * Only updates the widget if the widget is out of sync with the resources,
 * this will avoid having the machine model widget and the CIA model widget
 * triggering each others' event handlers.
 *
 * \param[in,out]   widget  CIA model widget
 */
void cia_model_widget_sync(GtkWidget *widget)
{
    if (cia1_group != NULL) {
        vice_gtk3_resource_radiogroup_sync(cia1_group);
    }
    if (cia2_group != NULL) {
        vice_gtk3_resource_radiogroup_sync(cia2_group);
    }
}


/** \brief  Set extra callback for changes in the widget
 *
 * The callback \a func get called with two arguments: CIA number (1 or 2) and
 * CIA model ID.
 *
 * \param[in,out]   widget  CIA model widget
 * \param[in]       func    callback
 */
void cia_model_widget_set_callback(GtkWidget *widget,
                                   void (*func)(int cia_num, int cia_model))
{
    cia_model_callback = func;
}
