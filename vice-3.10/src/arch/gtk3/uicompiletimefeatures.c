/** \file   uicompiletimefeatures.c
 * \brief   Dialog to display compile time features
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

#include "widgethelpers.h"
#include "debug_gtk3.h"
#include "machine.h"
#include "vicefeatures.h"
#include "ui.h"
#include "uiactions.h"

#include "uicompiletimefeatures.h"


/** \brief  Column indexes
 */
enum {
    COL_DESCR = 0,  /**< Symbol description */
    COL_SYMBOL,     /**< Symbol name in config.h */
    COL_ISDEFINED   /**< Symbol is defined */
};


/** \brief  Handler for the 'destroy' event of the dialog
 *
 * \param[in]   dialog  dialog triggering the event
 * \param[in]   unused  extra event data (unused)
 */
static void on_destroy(GtkDialog *dialog, gpointer unused)
{
    ui_action_finish(ACTION_HELP_COMPILE_TIME);
}


/** \brief  Handler for the 'response' event of the dialog
 *
 * \param[in,out]   dialog      dialog triggering the event
 * \param[in]       response_id response ID
 * \param[in]       user_data   extra event data (unused)
 */
static void on_response(GtkDialog *dialog,
                        gint response_id,
                        gpointer user_data)
{
    if (response_id == GTK_RESPONSE_CLOSE) {
        gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}


/** \brief  Sort callback for column \a data
 *
 * \param[in,out]   model   tree model
 * \param[in]       a       tree iterator to first element
 * \param[in]       b       tree iterator to second element
 * \param[in]       data    column ID
 *
 * \return  0 if equal, -1 if a < b, 1 if a > b
 */
static int sort_symbol_func(GtkTreeModel *model,
                            GtkTreeIter *a,
                            GtkTreeIter *b,
                            gpointer data)
{
    gchar *name_a = NULL;
    gchar *name_b = NULL;
    gint result = 0;
    gint column_id = GPOINTER_TO_INT(data);

    gtk_tree_model_get(model, a, column_id, &name_a, -1);
    gtk_tree_model_get(model, b, column_id, &name_b, -1);

    if (name_a == NULL || name_b == NULL) {
        if (name_a == NULL && name_b == NULL) {
            /* no data for either, equal */
            return 0;
        }
        /* prefer value over non-value */
        return name_a == NULL ? -1 : 1;
    }
    result = g_utf8_collate(name_a, name_b);
    g_free(name_a);
    g_free(name_b);

    return result;
}


/** \brief  Create tree model with compile time features
 *
 * \return  GtkTreeStore
 */
static GtkTreeStore *create_store(void)
{
    GtkTreeStore *store;
    GtkTreeIter iter;
    const feature_list_t *list;

    store = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    list = vice_get_feature_list();

    while (list->symbol != NULL) {
        gtk_tree_store_append(store, &iter, NULL);
        gtk_tree_store_set(store, &iter,
                COL_DESCR, list->descr,
                COL_SYMBOL, list->symbol,
                COL_ISDEFINED, list->isdefined ? "yes" : "no",
                -1);
        list++;
    }
    return store;
}


/** \brief  Create listview with compile time features
 *
 * \return  GtkScrolledWindow
 */
static GtkWidget *create_content_widget(void)
{
    GtkWidget *view;
    GtkWidget *scrolled;
    GtkTreeStore *store;
    GtkTreeSortable *sortable;
    GtkCellRenderer *text_renderer;
    GtkTreeViewColumn *column_descr;
    GtkTreeViewColumn *column_symbol;
    GtkTreeViewColumn *column_isdefined;

    scrolled = gtk_scrolled_window_new(NULL, NULL);
    store = create_store();

    /* make model sortable */
    sortable = GTK_TREE_SORTABLE(store);
    gtk_tree_sortable_set_sort_func(sortable, COL_DESCR, sort_symbol_func,
            GINT_TO_POINTER(COL_DESCR), NULL);
    gtk_tree_sortable_set_sort_func(sortable, COL_SYMBOL, sort_symbol_func,
            GINT_TO_POINTER(COL_SYMBOL), NULL);
    gtk_tree_sortable_set_sort_func(sortable, COL_ISDEFINED, sort_symbol_func,
            GINT_TO_POINTER(COL_ISDEFINED), NULL);
    /* set default sort order */
    gtk_tree_sortable_set_sort_column_id(sortable, COL_SYMBOL,
            GTK_SORT_ASCENDING);

    /* create view */
    view =  gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(view), TRUE);

    text_renderer = gtk_cell_renderer_text_new();

    column_descr = gtk_tree_view_column_new_with_attributes(
            "Description", text_renderer,
            "text", COL_DESCR,
            NULL);
    gtk_tree_view_column_set_sort_column_id(column_descr, COL_DESCR);
    column_symbol = gtk_tree_view_column_new_with_attributes(
            "Symbol", text_renderer,
            "text", COL_SYMBOL,
            NULL);
    gtk_tree_view_column_set_sort_column_id(column_symbol, COL_SYMBOL);
    column_isdefined = gtk_tree_view_column_new_with_attributes(
            "Defined", text_renderer,
            "text", COL_ISDEFINED,
            NULL);
    gtk_tree_view_column_set_sort_column_id(column_isdefined, COL_ISDEFINED);

    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column_descr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column_symbol);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column_isdefined);

    gtk_widget_set_size_request(scrolled, 800, 600);
    gtk_container_add(GTK_CONTAINER(scrolled), view);

    gtk_widget_show_all(scrolled);
    return scrolled;
}


/** \brief  Show list of compile time features
 */
void uicompiletimefeatures_dialog_show(void)
{
    GtkWidget *dialog;
    GtkWidget *content;
    gchar title[256];

    g_snprintf(title, sizeof(title), "%s compile time features", machine_name);

    dialog = gtk_dialog_new_with_buttons(title,
            ui_get_active_window(),
            GTK_DIALOG_MODAL,
            "Close", GTK_RESPONSE_CLOSE,
            NULL);

    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content), create_content_widget(),
            TRUE, TRUE, 0);

    g_signal_connect_unlocked(dialog,
                              "response",
                              G_CALLBACK(on_response),
                              NULL);
    g_signal_connect(dialog,
                     "destroy",
                     G_CALLBACK(on_destroy),
                     NULL);
    gtk_widget_show_all(dialog);
}
