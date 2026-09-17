/** \file   drivemodelwidget.c
 * \brief   Drive model selection widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES Drive8Type      -vsid
 * $VICERES Drive9Type      -vsid
 * $VICERES Drive10Type     -vsid
 * $VICERES Drive11Type     -vsid
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

#include "vice_gtk3.h"
#include "drive-check.h"
#include "drive.h"
#include "driveparallelcablewidget.h"
#include "drivewidgethelpers.h"
#include "machine-drive.h"
#include "resources.h"

#include "drivemodelwidget.h"


/** \brief  Handler for the 'toggled' event of the radio buttons
 *
 * \param[in]   widget      radio button
 * \param[in]   user_data   unit number
 */
static void on_radio_toggled(GtkWidget *widget, gpointer user_data)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {

        GtkWidget *parent;
        int unit;
        int new_type;
        int old_type;

        parent = gtk_widget_get_parent(widget);
        unit = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(parent), "UnitNumber"));
        new_type = GPOINTER_TO_INT(user_data);
        old_type = ui_get_drive_type(unit);

        /* prevent drive reset when switching unit number and updating the
         * drive type widget */
        if (new_type != old_type) {

            void (*cb_func)(GtkWidget *, gpointer);
            gpointer cb_data;

            resources_set_int_sprintf("Drive%dType", new_type, unit);

            /* check for a custom callback */
            parent = gtk_widget_get_parent(widget);
            cb_func = g_object_get_data(G_OBJECT(parent), "CallbackFunc");
            if (cb_func != NULL) {
                /* get callback data */
                cb_data = g_object_get_data(G_OBJECT(parent), "CallbackData");
                /* trigger callback */
                cb_func(widget, cb_data);
            }
        }
    }
}


/** \brief  Create a drive unit selection widget
 *
 * Creates a widget with four radio buttons, horizontally aligned, to select
 * a drive unit (8-11).
 *
 * \param[in]   unit    default drive unit (8-11)
 *
 * \return  GtkGrid
 */
GtkWidget *drive_model_widget_create(int unit)
{
    GtkWidget *grid;
    drive_type_info_t *list;
    GtkRadioButton *last = NULL;
    GSList *group = NULL;
    size_t i;
    int type;
    size_t num;
    int row;
    GtkWidget *child;

    resources_get_int_sprintf("Drive%dType", &type, unit);

    grid = vice_gtk3_grid_new_spaced_with_label(-1, 0, "Drive type", 2);
    child = gtk_grid_get_child_at(GTK_GRID(grid), 0, 0);
    gtk_widget_set_margin_bottom(child, 8);
    /* store unit number as a property in the widget */
    g_object_set_data(G_OBJECT(grid), "UnitNumber", GINT_TO_POINTER(unit));

    list = machine_drive_get_type_info_list();
    for (i = 0; list[i].name != NULL; i++) {
        /* NOP */
    }
    num = i;

    for (i = 0; list[i].name != NULL && i < num / 2; i++) {

        GtkWidget *radio = gtk_radio_button_new_with_label(group, list[i].name);

        gtk_radio_button_join_group(GTK_RADIO_BUTTON(radio), last);
        gtk_widget_set_margin_start(radio, 16);
        g_object_set_data(G_OBJECT(radio),
                          "ModelID",
                          GINT_TO_POINTER(list[i].id));

        if (list[i].id == type) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
        }

        g_signal_connect(radio,
                         "toggled",
                         G_CALLBACK(on_radio_toggled),
                         GINT_TO_POINTER(list[i].id));

        gtk_grid_attach(GTK_GRID(grid), radio, 0, (gint)(i + 1), 1, 1);
        last = GTK_RADIO_BUTTON(radio);
    }

    row = 1;
    while (list[i].name != NULL) {

        GtkWidget *radio = gtk_radio_button_new_with_label(group, list[i].name);

        gtk_radio_button_join_group(GTK_RADIO_BUTTON(radio), last);
        gtk_widget_set_margin_start(radio, 16);
        g_object_set_data(G_OBJECT(radio),
                          "ModelID",
                          GINT_TO_POINTER(list[i].id));
        if (list[i].id == type) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
        }

        g_signal_connect(radio,
                         "toggled",
                         G_CALLBACK(on_radio_toggled),
                         GINT_TO_POINTER(list[i].id));

        gtk_grid_attach(GTK_GRID(grid), radio, 1, row, 1, 1);
        row++;
        last = GTK_RADIO_BUTTON(radio);
        i++;
    }

    drive_model_widget_update(grid);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Update the drive type widget
 *
 * This reiterates the drive info list to see if any setting changes made drive
 * types available/unavailable.
 *
 * \param[in,out]   widget  drive type widget
 */
void drive_model_widget_update(GtkWidget *widget)
{
    drive_type_info_t *list;
    size_t i;
    int unit;
    int type;
    size_t num;
    int row;

    unit = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "UnitNumber"));
    type = ui_get_drive_type(unit);

    list = machine_drive_get_type_info_list();

    for (i = 0; list[i].name != NULL; i++) {
        /* NOP */
    }
    num = i;


    for (i = 0; list[i].name != NULL && i < num / 2; i++) {

        GtkWidget *radio = gtk_grid_get_child_at(GTK_GRID(widget),
                0, (gint)(i + 1));

        if (radio != NULL && GTK_IS_RADIO_BUTTON(radio)) {
            gtk_widget_set_sensitive(radio,
                    drive_check_type((unsigned int)(list[i].id),
                                     (unsigned int)(unit - DRIVE_UNIT_MIN)));
            if (list[i].id == type) {
                /* TODO: temporary block the resource-set callback */
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
            }
        }
    }

    row = 1;
    while (list[i].name != NULL) {

        GtkWidget *radio = gtk_grid_get_child_at(GTK_GRID(widget), 1, row);

        if (radio != NULL && GTK_IS_RADIO_BUTTON(radio)) {
            gtk_widget_set_sensitive(radio,
                    drive_check_type((unsigned int)(list[i].id),
                                     (unsigned int)(unit - DRIVE_UNIT_MIN)));
            if (list[i].id == type) {
                /* TODO: temporary block the resource-set callback */
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
            }
        }
        row++;
        i++;
    }
}


/** \brief  Add custom callback to \a widget
 *
 * Adds a user-defined callback function on drive type changes.
 *
 * \param[in,out]   widget      drive options widget
 * \param[in]       cb_func     callback function
 * \param[in]       cb_data     data passed with \a cb_func
 */
void drive_model_widget_add_callback(GtkWidget *widget,
                                     void (*cb_func)(GtkWidget *, gpointer),
                                     gpointer cb_data)
{
    g_object_set_data(G_OBJECT(widget), "CallbackFunc", (gpointer)cb_func);
    g_object_set_data(G_OBJECT(widget), "CallbackData", cb_data);
}


/*****************************************************************************
 *          Drive model widget implementation using a GtkComboBox            *
 *****************************************************************************/


/* Combo box columns */
enum {
    COL_NAME,   /**< drive model name column */
    COL_ID,     /**< drive model id column */
    COL_COUNT   /**< number of columns */
};


/** \brief  Get drive unit number
 *
 * \param[in]   self    combo box
 *
 * \return  unit number
 */
static int get_unit_number(GtkWidget *self)
{
    return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(self), "UnitNumber"));
}


/** \brief  Get drive type (called model in the UI)
 *
 * \param[in]   self    combo box
 *
 * \return  drive type
 */
static int get_drive_type(GtkWidget *self)
{
    int unit;
    int type;

    unit = get_unit_number(self);
    resources_get_int_sprintf("Drive%dType", &type, unit);
    return type;
}


/** \brief  Handler for the 'changed' event of the combo box
 *
 * Trigger user-defined callback on value change.
 *
 * \param[in]   self    drive model combo box
 * \param[in]   data    extra event data (unused)
 */
static void on_combo_changed(GtkWidget *self, gpointer data)
{
    int combo_val = drive_model_widget_value_combo(self);
    int resource_val = get_drive_type(self);
    int unit = get_unit_number(self);


    if (combo_val != resource_val) {
        void (*callback)(GtkWidget *, gpointer);

        if (resources_set_int_sprintf("Drive%dType", combo_val, unit) < 0) {
            /* TODO: flip combo back to previous value? */
            debug_gtk3("Failed to set resource Drive%dType to %d.", unit, combo_val);
            return;
        }

        callback = g_object_get_data(G_OBJECT(self), "CallbackFunc");
        if (callback != NULL) {
            callback(self, g_object_get_data(G_OBJECT(self), "CallbackData"));
        }
    }
}


/** \brief  Create model for the combo box
 *
 * \param[in]   self        drive model widget
 * \param[in]   show_all    show all drive types, use FALSE to only show types
 *                          supported by the current machine config
 *
 * \return  GtkListStore
 */
static GtkListStore *create_combo_model(GtkWidget *self, gboolean show_all)
{
    GtkListStore *model;
    GtkTreeIter iter;
    drive_type_info_t *list;
    int unit;
    int i;

    unit = get_unit_number(self);
    list = machine_drive_get_type_info_list();
    model = gtk_list_store_new(COL_COUNT, G_TYPE_STRING, G_TYPE_INT);

    for (i = 0; list[i].name != NULL; i++) {
        /* TODO: check if supported by current config */
        if (show_all || drive_check_type((unsigned int)(list[i].id),
                                         (unsigned int)(unit - DRIVE_UNIT_MIN))) {
            gtk_list_store_append(model, &iter);
            gtk_list_store_set(model, &iter,
                               COL_NAME, list[i].name,
                               COL_ID, list[i].id,
                               -1);
        }
    }
    return model;
}


/** \brief  Create drive model combo box
 *
 * \param[in]   unit        drive unit (8-11)
 * \param[in]   show_all    show all drive types, use FALSE to only show types
 *                          supported by the current machine config
 *
 * \return  GtkComboBox
 */
GtkWidget *drive_model_widget_create_combo(int unit, gboolean show_all)
{
    GtkWidget *combo;
    GtkListStore *model;
    GtkCellRenderer *renderer;
    gulong handler_id;

    combo = gtk_combo_box_new();
    g_object_set_data(G_OBJECT(combo), "UnitNumber", GINT_TO_POINTER(unit));

    model = create_combo_model(combo, show_all);
    gtk_combo_box_set_model(GTK_COMBO_BOX(combo), GTK_TREE_MODEL(model));

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo),
                                   renderer,
                                   "text", 0,
                                   NULL);

    handler_id = g_signal_connect(combo, "changed", G_CALLBACK(on_combo_changed), NULL);
    g_object_set_data(G_OBJECT(combo), "ChangedHandlerID", GULONG_TO_POINTER(handler_id));

    drive_model_widget_sync_combo(combo);
    gtk_widget_show_all(combo);
    return combo;
}


/** \brief  Synchronize the widget with its resource
 *
 * Select current item based on resource without triggering the 'changed' event.
 *
 * \param[in]   widget  drive model widget
 *
 * \return  TRUE if an item matched the current resource value
 */
gboolean drive_model_widget_sync_combo(GtkWidget *widget)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gulong handler_id;
    int type;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    type = get_drive_type(widget);
    handler_id = GPOINTER_TO_ULONG(g_object_get_data(G_OBJECT(widget),
                                                     "ChangedHandlerID"));

    if (gtk_tree_model_get_iter_first(model, &iter)) {
        do {
            int id;

            gtk_tree_model_get(model, &iter, COL_ID, &id, -1);
            if (id == type) {
                g_signal_handler_block(G_OBJECT(widget), handler_id);
                gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget), &iter);
                g_signal_handler_unblock(G_OBJECT(widget), handler_id);
                return TRUE;
            }
        } while (gtk_tree_model_iter_next(model, &iter));
    }
    return FALSE;
}


/** \brief  Get current drive model
 *
 * \param[in]   widget  drive model widget
 *
 * \return  drive model ID or -1 when no item is selected in the widget
 */
int drive_model_widget_value_combo(GtkWidget *widget)
{
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) >= 0) {
        GtkTreeModel *model;
        GtkTreeIter iter;
        int id;

        model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
        if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)) {
            gtk_tree_model_get(model, &iter, COL_ID, &id, -1);
            return id;
        }
    }
    return -1;
}
