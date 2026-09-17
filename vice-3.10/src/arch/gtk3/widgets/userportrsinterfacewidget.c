/** \file   userportmodemwidget.c
 * \brief   Userport RS232 Interface widget
 *
 * \author  Pablo Roldan <pdroldan@gmail.com>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES RsUserUP9600    x64 x64sc xscpu64
 * $VICERES RsUserRTSInv    x64 x64sc xscpu64 x128 xvic
 * $VICERES RsUserCTSInv    x64 x64sc xscpu64 x128 xvic
 * $VICERES RsUserDSRInv    x64 x64sc xscpu64 x128 xvic
 * $VICERES RsUserDCDInv    x64 x64sc xscpu64 x128 xvic
 * $VICERES RsUserDTRInv    x64 x64sc xscpu64 x128 xvic
 * $VICERES RsUserRIInv     x64 x64sc xscpu64 x128 xvic
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
#include "machine.h"
#include "resources.h"

#include "userportrsinterfacewidget.h"


/** \brief  List of possible userport RS-232 interfaces for C64
 */
static const vice_gtk3_combo_entry_int_t userport_rsinterfaces64[] = {
    { "RS-232, normal control lines levels",    USERPORT_RS_NONINVERTED },
    { "RS-232, inverted control lines levels",  USERPORT_RS_INVERTED },
    { "RS-232, custom control lines levels",    USERPORT_RS_CUSTOM },
    { "UP-9600",                                USERPORT_RS_UP9600 },
    { NULL,                                     -1 }
};


/** \brief  List of possible userport RS-232 interfaces for VIC-20/C128
 */
static const vice_gtk3_combo_entry_int_t userport_rsinterfacesvic[] = {
    { "RS-232, normal control lines levels",    0 },
    { "RS-232, inverted control lines levels",  1 },
    { "RS-232, custom control lines levels",    2 },
    { NULL,                                    -1 }
};

/** \brief  Get RS-232 interface combo box index from the control lines checkboxes combinations
 *
 * \return  int
 */
static int get_rsinterface_index(void)
{
    int up9600;
    int rtsinv;
    int ctsinv;
    int dsrinv;
    int dtrinv;
    int index;

    resources_get_int("RsUserUP9600", &up9600);
    resources_get_int("RsUserRTSInv", &rtsinv);
    resources_get_int("RsUserCTSInv", &ctsinv);
    resources_get_int("RsUserDSRInv", &dsrinv);
    resources_get_int("RsUserDTRInv", &dtrinv);

    if (!up9600) {
        if (rtsinv && ctsinv && dsrinv && dtrinv) {
            /* Inverted control lines */
            index = USERPORT_RS_INVERTED;
        } else if (!rtsinv && !ctsinv && !dsrinv && !dtrinv) {
            /* Non inverted control lines */
            index = USERPORT_RS_NONINVERTED;
        } else {
            /* Custom control lines levels */
            index = USERPORT_RS_CUSTOM;
        }

    } else {
        /* UP9600 interface */
        index = USERPORT_RS_UP9600;
    }
    return index;
}

/** \brief  Handler for the "changed" event of the Userport RS-232 Interface combo box
 *
 * \param[in]   widget      combo box
 * \param[in]   user_data   extra event data (unused)
 */
static void on_combo_changed(GtkWidget *widget, gpointer user_data)
{
    void (*cb_func)(GtkWidget *);

    cb_func = g_object_get_data(G_OBJECT(widget), "CallbackFunc");
    if (cb_func != NULL) {
        /* trigger callback */
        cb_func(widget);
    }
}

/** \brief  Handler for the "toggled" event of the Userport RS-232 Interface control lines
 *          Refresh Userport RS-232 interface combo box
 *
 * \param[in]   widget      check button
 * \param[in]   user_data   extra event data (unused)
 */
static void on_control_toggled(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *grid;
    GtkWidget *combo;

    grid = gtk_widget_get_parent(widget);
    combo = gtk_grid_get_child_at(GTK_GRID(grid), 1, 1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), get_rsinterface_index());
}

/** \brief  Create left-aligned, 8 pixels indented label
 *
 * \param[in]   text    label text
 *
 * \return  GtkLabel
 */
static GtkWidget *create_label(const char *text)
{
    GtkWidget *label = gtk_label_new(text);

    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 8);
    return label;
}

/** \brief  Create userport rsinterface widget
 *
 * \return  GtkCombobox
 */
GtkWidget *userport_rsinterface_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *combo;
    GtkWidget *label;
    GtkWidget *rsuser_rtsinv_widget;
    GtkWidget *rsuser_ctsinv_widget;
    GtkWidget *rsuser_dsrinv_widget;
    GtkWidget *rsuser_dcdinv_widget;
    GtkWidget *rsuser_dtrinv_widget;
    GtkWidget *rsuser_riinv_widget;
    const vice_gtk3_combo_entry_int_t *list;
    int i;

    if ((machine_class == VICE_MACHINE_VIC20) || (machine_class == VICE_MACHINE_C128)) {
        list = userport_rsinterfacesvic;
    } else {
        list = userport_rsinterfaces64;
    }

    grid = vice_gtk3_grid_new_spaced(8, 0);

    combo = gtk_combo_box_text_new();
    for (i = 0; list[i].name != NULL; i++) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo),
                                  NULL,
                                  list[i].name);
    }
    gtk_widget_set_hexpand(combo, TRUE);
    gtk_widget_set_margin_top(combo, 8);
    gtk_widget_set_margin_bottom(combo, 8);
    label = create_label("RS-232 Interface type");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), combo, 1, 1, 2, 1);

    label = create_label("Control lines");
    rsuser_rtsinv_widget = vice_gtk3_resource_check_button_new("RsUserRTSInv",
                                                               "Invert RTS");
    gtk_grid_attach(GTK_GRID(grid), label,                0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), rsuser_rtsinv_widget, 1, 2, 1, 1);
    g_signal_connect(rsuser_rtsinv_widget,
                     "toggled",
                     G_CALLBACK(on_control_toggled),
                     NULL);

    rsuser_ctsinv_widget = vice_gtk3_resource_check_button_new("RsUserCTSInv",
                                                               "Invert CTS");
    gtk_grid_attach(GTK_GRID(grid), rsuser_ctsinv_widget, 2, 2, 1, 1);
    g_signal_connect(rsuser_ctsinv_widget,
                     "toggled",
                     G_CALLBACK(on_control_toggled),
                     NULL);

    rsuser_dsrinv_widget = vice_gtk3_resource_check_button_new("RsUserDSRInv",
                                                               "Invert DSR");
    gtk_grid_attach(GTK_GRID(grid), rsuser_dsrinv_widget, 1, 3, 1, 1);
    g_signal_connect(rsuser_dsrinv_widget,
                     "toggled",
                     G_CALLBACK(on_control_toggled),
                     NULL);

    rsuser_dcdinv_widget = vice_gtk3_resource_check_button_new("RsUserDCDInv",
                                                               "Invert DCD");
    gtk_grid_attach(GTK_GRID(grid), rsuser_dcdinv_widget, 2, 3, 1, 1);
    g_signal_connect(rsuser_dcdinv_widget,
                     "toggled",
                     G_CALLBACK(on_control_toggled),
                     NULL);

    rsuser_dtrinv_widget = vice_gtk3_resource_check_button_new("RsUserDTRInv",
                                                               "Invert DTR");
    gtk_grid_attach(GTK_GRID(grid), rsuser_dtrinv_widget, 1, 4, 1, 1);
    g_signal_connect(rsuser_dtrinv_widget,
                     "toggled",
                     G_CALLBACK(on_control_toggled),
                     NULL);

    rsuser_riinv_widget = vice_gtk3_resource_check_button_new("RsUserRIInv",
                                                               "Invert RI");
    gtk_grid_attach(GTK_GRID(grid), rsuser_riinv_widget, 2, 4, 1, 1);
    g_signal_connect(rsuser_riinv_widget,
                     "toggled",
                     G_CALLBACK(on_control_toggled),
                     NULL);

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), get_rsinterface_index());
    g_signal_connect(combo,
                     "changed",
                     G_CALLBACK(on_combo_changed),
                     NULL);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Add custom callback to \a widget
 *
 * Adds a user-defined callback function on modem type changes.
 *
 * \param[in,out]   widget      userport modem widget
 * \param[in]       cb_func     callback function
 */
void userport_rsinterface_widget_add_callback(GtkGrid  *widget,
                                              void    (*cb_func)(GtkWidget*))
{
    GtkWidget *combo;

    combo = gtk_grid_get_child_at(widget, 1, 1);
    g_object_set_data(G_OBJECT(combo), "CallbackFunc", (gpointer)cb_func);
}
