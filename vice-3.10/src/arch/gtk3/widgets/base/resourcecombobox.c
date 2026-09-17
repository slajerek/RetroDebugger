/** \file   resourcecombobox.c
 * \brief   Combo boxes connected to a resource
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
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "basewidget_types.h"
#include "debug_gtk3.h"
#include "lib.h"
#include "log.h"
#include "resourcehelpers.h"
#include "resources.h"
#include "resourcewidgetmediator.h"

#include "resourcecombobox.h"

/* Model columns for (id, value) pairs */
enum {
    COLUMN_ID,      /**< ID column (hidden) */
    COLUMN_VALUE    /**< value column (shown) */
};


/******************************************************************************
 *                      Combo box for integer resources                       *
 *                                                                            *
 * Presents a combo box with integer keys and strings as displayed values.    *
 *****************************************************************************/

/** \brief  Create a model for a combo box with ints as IDs
 *
 * \param[in]   list    list of options
 *
 * \return  model
 */
static GtkListStore *combo_int_model_new(const vice_gtk3_combo_entry_int_t *list)
{
    GtkListStore *model;
    int           index;

    model = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING);
    if (list != NULL) {
        for (index = 0; list[index].name != NULL; index++) {
            GtkTreeIter iter;
            gtk_list_store_append(model, &iter);
            gtk_list_store_set(model,
                               &iter,
                               COLUMN_ID,    list[index].id,
                               COLUMN_VALUE, list[index].name,
                               -1);
        }
    }
    return model;
}

/** \brief  Append row to int combo model
 *
 * \param[in]   model   combo box model
 * \param[in]   id      ID for row
 * \param[in]   value   displayed value for row
 */
static void combo_int_model_append(GtkListStore *model, int id, const char *value)
{
    GtkTreeIter iter;

    gtk_list_store_append(model, &iter);
    gtk_list_store_set(model,
                       &iter,
                       COLUMN_ID,    id,
                       COLUMN_VALUE, value,
                       -1);
}

/** \brief  Create a view for a combo box with ints as IDs
 *
 * \param[in]   model   model
 *
 * \return  GtkComboBox
 */
static GtkWidget *combo_int_view_new(GtkListStore *model)
{
    GtkWidget       *view;
    GtkCellRenderer *renderer;

    /* TODO: almost exact copy of create_combo_hex_view(), so refactor! */

    /* create combo box with text renderer */
    view = gtk_combo_box_new();
    gtk_combo_box_set_model(GTK_COMBO_BOX(view), GTK_TREE_MODEL(model));
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(view), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(view),
                                   renderer,
                                   "text", COLUMN_VALUE,
                                   NULL);
    return view;
}

/** \brief  Get current ID of \a combo
 *
 * \param[in]   combo   combo box
 * \param[out]  id      target of ID value
 *
 * \return  boolean
 *
 * \note    When this function returns `false`, the value in \a id is unchanged
 */
static gboolean get_combo_int_id(GtkComboBox *combo, int *id)
{
    GtkTreeIter iter;

    if (gtk_combo_box_get_active_iter(combo, &iter)) {
        GtkTreeModel *model = gtk_combo_box_get_model(combo);
        gtk_tree_model_get(model, &iter, COLUMN_ID, id, -1);
        return TRUE;
    }
    return FALSE;
}

/** \brief  Set ID of \a combo to \a id
 *
 * \param[in,out]   combo   combo box
 * \param[in]       id      ID for \a combo
 *
 * \return  boolean
 */
static gboolean set_combo_int_id(GtkComboBox *combo, int id)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;

    model = gtk_combo_box_get_model(combo);
    if (gtk_tree_model_get_iter_first(model, &iter)) {
        do {
            int current = 0;

            gtk_tree_model_get(model, &iter, COLUMN_ID, &current, -1);
            if (id == current) {
                gtk_combo_box_set_active_iter(combo, &iter);
                return TRUE;
            }
        } while (gtk_tree_model_iter_next(model, &iter));
    }
    return FALSE;
}

/** \brief  Handler for the "changed" event of the integer combo box
 *
 * Updates the resource connected to the combo box
 *
 * \param[in]   self    combo box
 * \param[in]   data    extra event data (unused)
 */
static void on_combo_int_changed(GtkComboBox *self, gpointer data)
{
    mediator_t *mediator;
    int         id = 0;

    mediator = mediator_for_widget(GTK_WIDGET(self));
    if (get_combo_int_id(self, &id)) {
        /* set resource, update state and trigger callback if present */
        if (!mediator_update_int(mediator, id)) {
            /* revert without trigger useless resource update */
            mediator_handler_block(mediator);
            set_combo_int_id(self, mediator_get_current_int(mediator));
            mediator_handler_unblock(mediator);
        }
    }
}


/** \brief  Create a combo box to control an integer resource
 *
 * Create integer resource-bound combo box for \a resource, populating it with
 * \a entries. The \a entries list is allowed to be `NULL`, in which case an
 * empty model is created and the user can manually add rows using
 * vice_gtk3_resource_combo_int_append().
 *
 * \param[in]   resource    resource name
 * \param[in]   entries     list of entries for the combo box (can be `NULL`)
 *
 * \return  GtkComboBoxText
 *
 */
GtkWidget *vice_gtk3_resource_combo_int_new(const char *resource,
                                            const vice_gtk3_combo_entry_int_t *entries)
{
    GtkWidget    *view;
    GtkListStore *model;
    mediator_t   *mediator;
    gulong        handler;

    model    = combo_int_model_new(entries);
    view     = combo_int_view_new(model);
    mediator = mediator_new(view, resource, G_TYPE_INT);

    /* set current ID */
    if (entries != NULL) {
        set_combo_int_id(GTK_COMBO_BOX(view), mediator_get_current_int(mediator));
    }
    /* connect handler and register with mediator */
    handler = g_signal_connect(G_OBJECT(view),
                               "changed",
                               G_CALLBACK(on_combo_int_changed),
                               NULL);
    mediator_set_handler(mediator, handler);

    return view;
}


/** \brief  Create a combo box to control an integer resource
 *
 * * Create integer resource-bound combo box, populating it with \a entries.
 * The \a entries list is allowed to be `NULL`, in which case an empty model is
 * created and the user can manually add rows using
 * vice_gtk3_resource_combo_int_append().
 *
 * This verions allows setting the resource name via sprintf()-syntax.
 *
 * \param[in]   fmt     format string for the resource name
 * \param[in]   entries list of entries for the combo box (can be `NULL`)
 *
 * \return  GtkComboBoxText
 */
GtkWidget *vice_gtk3_resource_combo_int_new_sprintf(
        const char *fmt,
        const vice_gtk3_combo_entry_int_t *entries,
        ...)
{
    char    resource[256];
    va_list args;

    va_start(args, entries);
    g_vsnprintf(resource, sizeof resource, fmt, args);
    va_end(args);

    return vice_gtk3_resource_combo_int_new(resource, entries);
}


/** \brief  Append row to integer combo box
 *
 * \param[in]   widget  integer resource combo box
 * \param[in]   id      ID for row
 * \param[in]   value   displayed string for row
 */
void vice_gtk3_resource_combo_int_append(GtkWidget  *widget,
                                         int         id,
                                         const char *value)
{
    GtkListStore *model;

    model = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(widget)));
    combo_int_model_append(model, id, value);
}


/** \brief  Set new ID for integer combo box
 *
 * Set new ID of the combo box
 *
 * \param[in,out]   widget  integer resource combo box
 * \param[in]       id      new ID for the combo box resource
 *
 * \return  TRUE if the new \a id was set
 */
gboolean vice_gtk3_resource_combo_int_set(GtkWidget *widget, int id)
{
    return set_combo_int_id(GTK_COMBO_BOX(widget), id);
}


/** \brief  Reset \a widget to its factory default
 *
 * \param[in,out]   widget  integer resource combo box
 *
 * \return  TRUE if the widget was reset to its factory value
 */
gboolean vice_gtk3_resource_combo_int_factory(GtkWidget *widget)
{
    mediator_t *mediator;
    int         factory;

    mediator = mediator_for_widget(widget);
    factory  = mediator_get_factory_int(mediator);
    return set_combo_int_id(GTK_COMBO_BOX(widget), factory);
}


/** \brief  Reset \a widget to its state when it was instanciated
 *
 * \param[in,out]   widget  integer resource combo box
 *
 * \return  bool
 */
gboolean vice_gtk3_resource_combo_int_reset(GtkWidget *widget)
{
    mediator_t *mediator;
    int         initial = 0;

    mediator = mediator_for_widget(widget);
    initial  = mediator_get_initial_int(mediator);
    return vice_gtk3_resource_combo_int_set(widget, initial);
}


/** \brief  Synchronize \a widget with its resource value
 *
 * \param[in,out]   widget  integer resource combo box
 *
 * \return  bool
 */
gboolean vice_gtk3_resource_combo_int_sync(GtkWidget *widget)
{
    mediator_t *mediator;
    int         id = -1;
    int         value;
    gboolean    result = TRUE;

    mediator = mediator_for_widget(widget);
    get_combo_int_id(GTK_COMBO_BOX(widget), &id);
    value = mediator_get_resource_int(mediator);
    if (id != value) {
        mediator_handler_block(mediator);
        result = set_combo_int_id(GTK_COMBO_BOX(widget), value);
        mediator_handler_unblock(mediator);
    }
    return result;
}


/******************************************************************************
 *    GtkComboBox for integer resources, displaying values in hexadecimal     *
 *                                                                            *
 * Displays a list, or range, of integer values in hexadecimal notation,      *
 * presenting the values as unsigned 16-bit values in the form '$hhhh'.       *
 *****************************************************************************/

/** \brief  Add value to model
 *
 * Add \a value as integer ID and as a hex string (prefixed with '$') to be
 * displayed.
 *
 * \param[in]   model   hex resource combo model
 * \param[in]   value   value for \a model
 */
static void combo_hex_model_append(GtkListStore *model, int value)
{
    char hexstr[64];

    g_snprintf(hexstr, sizeof hexstr, "$%04x", (unsigned int)value);
    combo_int_model_append(model, value, hexstr);
}

/** \brief  Create model from list of integer values
 *
 * Create hex combo model from values in \a list.
 *
 * \param[in]   list    values, terminated with <0
 *
 * \return  populated model
 */
static GtkListStore *combo_hex_model_list_new(const int *list)
{
    GtkListStore *model;
    int           index = 0;

    model = combo_int_model_new(NULL);
    while (list[index] >= 0) {
        combo_hex_model_append(model, list[index]);
        index++;
    }
    return model;
}

/** \brief  Create model from range of integer values
 *
 * Create hex combo model with values starting with \a lower up to, but excluding,
 * \a upper, incrementing with \a step.
 *
 * \param[in]   list    values, terminated with <0
 *
 * \return  populated model
 */
static GtkListStore *combo_hex_model_range_new(int lower,
                                               int upper,
                                               int step)
{
    GtkListStore *model;
    int           value;

    model = combo_int_model_new(NULL);
    for (value = lower; value < upper; value += step) {
        combo_hex_model_append(model, value);
    }
    return model;
}

/** \brief  Create view for a hex resource combo
 *
 * Create combo box for \a model.
 *
 * \param[in]   model   model for the combo box
 *
 * \return  GtkComboBox
 */
static GtkWidget *combo_hex_view_new(GtkListStore *model)
{
    GtkWidget       *view;
    GtkCellRenderer *renderer;

    /* create combo box with text renderer */
    view = gtk_combo_box_new();
    gtk_combo_box_set_model(GTK_COMBO_BOX(view), GTK_TREE_MODEL(model));
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(view), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(view),
                                   renderer,
                                   "text", COLUMN_VALUE,
                                   NULL);
    return view;
}

/** \brief  Helper to create hex resource combo box
 *
 * \param[in]   resource    resource name
 * \param[in]   model       model for the combo box
 *
 * \return  GtkComboBox
 */
static GtkWidget *combo_hex_helper(const char   *resource,
                                   GtkListStore *model)
{
    GtkWidget  *view;
    mediator_t *mediator;
    gulong      handler;

    view     = combo_hex_view_new(model);
    mediator = mediator_new(view, resource, G_TYPE_INT);

    set_combo_int_id(GTK_COMBO_BOX(view), mediator_get_current_int(mediator));
    /* Connect handler and register with mediator. The hex combo box is just
     * an int combo box with a different presentation of the values, so we
     * use the int combo as the "parent class".
     */
    handler = g_signal_connect(G_OBJECT(view),
                               "changed",
                               G_CALLBACK(on_combo_int_changed),
                               NULL);
    mediator_set_handler(mediator, handler);

    return view;
}


/******************************************************************************
 *                      Hex resource combo box - Public API                   *
 *****************************************************************************/

/** \brief  Create resource-bound combo box showing hex literals
 *
 * Create combo box bound to an integer resource that shows a hex string in
 * the form '$hhhh' for each value in \a list.
 *
 * This function creates the combo box with an empty model, which can then
 * be populated using vice_gtk3_resource_combo_hex_append().
 *
 * \return  GtkComboBox
 */
GtkWidget *vice_gtk3_resource_combo_hex_new(const char *resource)
{
    GtkListStore *model = combo_int_model_new(NULL);

    return combo_hex_helper(resource, model);
}


/** \brief  Create resource-bound combo box showing hex literals
 *
 * Create combo box bound to an integer resource that shows a hex string in
 * the form '$hhhh' for each value in \a list.
 *
 * \param[in]   resource    resource name
 * \param[in]   list        list of values, terminate with <0
 *
 * \return  GtkComboBox
 */
GtkWidget *vice_gtk3_resource_combo_hex_new_list(const char *resource,
                                                 const int  *list)
{
    return combo_hex_helper(resource, combo_hex_model_list_new(list));
}


/** \brief  Create resource-bound combo box showing hex literals
 *
 * Create combo box bound to an integer resource that shows a hex string in
 * the form '$hhhh' for each value in range \a lower to \a upper (exclusive).
 *
 * \param[in]   resource    resource name
 * \param[in]   lower       range starting value (inclusive)
 * \param[in]   upper       range ending value (exclusive)
 * \param[in]   step        increment of value
 *
 * \return  GtkComboBox
 */
GtkWidget *vice_gtk3_resource_combo_hex_new_range(const char *resource,
                                                  int         lower,
                                                  int         upper,
                                                  int         step)
{
    if (upper <= lower) {
        log_error(LOG_DEFAULT, "%s(): invalid range %d-%d", __func__, lower, upper);
        return NULL;
    }
    if (step < 1) {
        log_error(LOG_DEFAULT, "%s(): invalid step value %d", __func__, step);
        return NULL;
    }
    return combo_hex_helper(resource,
                            combo_hex_model_range_new(lower, upper, step));
}


/** \brief  Append value to hex combo box
 *
 * \param[in]   combo   hex resource combo box
 * \param[in]   value   value to apend (will be displayed as "\$hhhhh")
 */
void vice_gtk3_resource_combo_hex_append(GtkWidget *combo, int value)
{
    GtkListStore *model;

    model = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo)));
    combo_hex_model_append(model, value);
}


/** \brief  Set hex resource combo box ID, triggering resource update
 *
 * \param[in]   combo   hex resource combo box
 * \param[in]   id      new ID for \a combo
 *
 * \return  `TRUE` if the combo box ID and resource value could be set
 */
gboolean vice_gtk3_resource_combo_hex_set(GtkWidget *combo, int id)
{
    return vice_gtk3_resource_combo_int_set(combo, id);
}


/** \brief  Reset hex combo box to the resource factory value
 *
 * Set combo box and the bound resource to the resource factory value.
 *
 * \param[in]   combo
 *
 * \return  `TRUE` if the combo box and the resource could be set
 */
gboolean vice_gtk3_resource_combo_hex_factory(GtkWidget *combo)
{
    return vice_gtk3_resource_combo_int_factory(combo);
}


/** \brief  Reset hex combo box to its value on instanciation
 *
 * Reset \a combo to the value the resource had when the \a combo was created.
 *
 * \param[in]   combo
 *
 * \return  `TRUE` if the combo box and the resource could be set
 */
gboolean vice_gtk3_resource_combo_hex_reset(GtkWidget *combo)
{
    return vice_gtk3_resource_combo_int_reset(combo);
}


/** \brief  Synchronized hex combo box with its resource
 *
 * Set \a combo to the current value of its resource, without triggering a
 * resource-set operation.
 *
 * \return  `TRUE` if the combo box could be synchronized with its resource
 */
gboolean vice_gtk3_resource_combo_hex_sync(GtkWidget *combo)
{
    return vice_gtk3_resource_combo_int_sync(combo);
}


/******************************************************************************
 *                      Combo box for string resources                        *
 *                                                                            *
 *      Presents a combo box with strings, and uses strings for keys.         *
 *****************************************************************************/

/** \brief  Create model for combo box with string (id,value) pairs
 *
 * Create model for combo box with column 0 as ID (string) and column 1 as
 * display value (string)).
 *
 * \param[in]   entries list of entries for the model (can be `NULL`)
 *
 * \return  new model
 */
static GtkListStore *combo_str_model_new(const vice_gtk3_combo_entry_str_t *entries)
{
    GtkListStore *model;
    int           index;

    model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    if (entries != NULL) {
        for (index = 0; entries[index].id != NULL; index++) {
            GtkTreeIter iter;

            gtk_list_store_append(model, &iter);
            gtk_list_store_set(model,
                               &iter,
                               COLUMN_ID,    entries[index].id,
                               COLUMN_VALUE, entries[index].value,
                               -1);
        }
    }
    return model;
}

/** \brief  Append row to string resource combo box model
 *
 * \param[in]   model   model
 * \param[in]   id      ID for the row
 * \param[in]   value   display value for the row
 */
static void combo_str_model_append(GtkListStore *model,
                                   const char   *id,
                                   const char   *value)
{
    if (id != NULL && *id != '\0' && value != NULL && *value != '\0') {
        GtkTreeIter iter;

        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model,
                           &iter,
                           COLUMN_ID,    id,
                           COLUMN_VALUE, value,
                           -1);
    } else {
        log_error(LOG_DEFAULT,
                  "%s(): cannot append row, `id` and `value` must both be"
                  " non-NULL and non-empty",
                  __func__);
    }
}

/** \brief  Create combo box for string resources and initialize with model
 *
 * \param[in]   model   model
 *
 * \return  GtkComboBox
 */
static GtkWidget *combo_str_view_new(GtkListStore *model)
{
    GtkWidget       *view;
    GtkCellRenderer *renderer;

    view = gtk_combo_box_new();
    gtk_combo_box_set_model(GTK_COMBO_BOX(view), GTK_TREE_MODEL(model));
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(view), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(view),
                                   renderer,
                                   "text", COLUMN_VALUE,
                                   NULL);
    /* set ID column so we can use gtk_combo_box_get_active_id() (only works
     * for string IDs) */
    gtk_combo_box_set_id_column(GTK_COMBO_BOX(view), COLUMN_ID);
    return view;
}

/** \brief  Handler for the 'changed' event of the string combo box
 *
 * Updates the resource connected to the combo box
 *
 * \param[in]   combo  combo box
 * \param[in]   data   extra event data (unused)
 */
static void on_combo_str_changed(GtkComboBox *self, gpointer data)
{
    mediator_t *mediator;
    const char *combo_id;

    mediator = mediator_for_widget(GTK_WIDGET(self));
    combo_id = gtk_combo_box_get_active_id(self);
    if (combo_id != NULL) {
        /* update resource and mediator state, call callback on success */
        if (!mediator_update_string(mediator, combo_id)) {
            /* failed to set resource, revert widget state */
            mediator_handler_block(mediator);
            gtk_combo_box_set_active_id(self,
                                        mediator_get_current_string(mediator));
            mediator_handler_unblock(mediator);
        }
    }
}


/** \brief  Create a combo box bound to a string resource
 *
 * \param[in]   resource    resource name
 * \param[in]   entries     list of entries for the combo box or `NULL`
 *
 * \return  GtkComboBox
 */
GtkWidget *vice_gtk3_resource_combo_str_new(
        const char *resource,
        const vice_gtk3_combo_entry_str_t *entries)
{
    GtkListStore *model;
    GtkWidget    *view;
    mediator_t   *mediator;
    gulong        handler;
    const char   *id = NULL;

    model    = combo_str_model_new(entries);
    view     = combo_str_view_new(model);
    mediator = mediator_new(view, resource, G_TYPE_STRING);

    /* set proper ID */
    id = mediator_get_resource_string(mediator);
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(view), id);

    /* connect signal handler and register with mediator */
    handler = g_signal_connect(G_OBJECT(view),
                               "changed",
                               G_CALLBACK(on_combo_str_changed),
                               NULL);
    mediator_set_handler(mediator, handler);

    return view;
}


/** \brief  Create a combo box to control a string resource
 *
 * \param[in]   fmt         resource name, printf-style format string
 * \param[in]   entries     list of entries for the combo box or `NULL`
 *
 * \return  GtkComboBoxText
 */
GtkWidget *vice_gtk3_resource_combo_str_new_sprintf(
        const char *fmt,
        const vice_gtk3_combo_entry_str_t *entries,
        ...)
{
    char    resource[256];
    va_list args;

    va_start(args, entries);
    g_vsnprintf(resource, sizeof resource, fmt, args);
    va_end(args);
    return vice_gtk3_resource_combo_str_new(resource, entries);
}


/** \brief  Append row to string resource combo box
 *
 * \param[in]   widget  string resource combo box
 * \param[in]   id      ID for the row
 * \param[in]   value   display value for the row
 *
 * \note    \a id and \a value must both be non-NULL and non-empty.
 */
void vice_gtk3_resource_combo_str_append(GtkWidget  *widget,
                                         const char *id,
                                         const char *value)
{
    GtkListStore *model;

    model = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(widget)));
    if (model != NULL) {
        combo_str_model_append(model, id, value);
    }
}


/** \brief  Update string combo box by ID
 *
 * Set new ID of the combo box
 *
 * \param[in,out]   widget  string resource combo box
 * \param[in]       id      new ID of the combo box
 *
 * \return  TRUE if the new \a id was set
 */
gboolean vice_gtk3_resource_combo_str_set(GtkWidget *widget, const char *id)
{
    return gtk_combo_box_set_active_id(GTK_COMBO_BOX(widget), id);
}


/** \brief  Reset string combo box to its factory default
 *
 * \param[in,out]   widget  string resource combo box
 *
 * \return  TRUE if the factory default was set
 */
gboolean vice_gtk3_resource_combo_str_factory(GtkWidget *widget)
{
    mediator_t *mediator;
    const char *factory;

    mediator = mediator_for_widget(widget);
    factory  = mediator_get_factory_string(mediator);
    return gtk_combo_box_set_active_id(GTK_COMBO_BOX(widget), factory);
}


/** \brief  Reset string combo box to its original value
 *
 * Restores the state of the widget as it was when instanciated.
 *
 * \param[in,out]   widget  string resource combo box
 *
 * \return  TRUE if the combo box was reset
 */
gboolean vice_gtk3_resource_combo_str_reset(GtkWidget *widget)
{
    mediator_t *mediator;
    const char *initial;

    mediator = mediator_for_widget(widget);
    initial  = mediator_get_initial_string(mediator);
    return gtk_combo_box_set_active_id(GTK_COMBO_BOX(widget), initial);
}


/** \brief  Synchronize widget with its resource
 *
 * Updates the widget state to what its resource currently is.
 *
 * \param[in,out]   widget  string resource combo box
 *
 * \return  TRUE if the widget was synchronized with its resource
 */
gboolean vice_gtk3_resource_combo_str_sync(GtkWidget *widget)
{
    mediator_t *mediator;
    const char *res_val;
    const char *combo_id;
    gboolean    result = TRUE;

    mediator = mediator_for_widget(widget);
    res_val  = mediator_get_resource_string(mediator);
    combo_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    if (g_strcmp0(res_val, combo_id) != 0) {
        mediator_handler_block(mediator);
        result = gtk_combo_box_set_active_id(GTK_COMBO_BOX(widget), res_val);
        mediator_handler_unblock(mediator);
    }
    return result;
}
