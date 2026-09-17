/** \file   sidmodelwidget.c
 * \brief   Widget to select SID model
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES SidModel    all
 *  (all = if a SidCart is installed)
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
#include <glib/gstdio.h>

#include "sid.h"
#include "vice_gtk3.h"
#include "lib.h"
#include "resources.h"
#include "machine.h"
#include "machinemodelwidget.h"
#include "mixerwidget.h"

#include "sidmodelwidget.h"


/** \brief  Empty list of SID models
 */
static const vice_gtk3_radiogroup_entry_t sid_models_none[] = {
    { NULL, -1 }
};

/** \brief  SID models used in the C64/C64SCPU, C128 and expanders for PET,
 *          VIC-20 and Plus/4
 */
static const vice_gtk3_radiogroup_entry_t sid_models_c64[] = {
    { "6581",               SID_MODEL_6581 },
    { "8580",               SID_MODEL_8580 },
    { "8580 + digi boost",  SID_MODEL_8580D },
    { NULL,                 -1 }
};

/** \brief  SID models used in the C64DTV
 */
static const vice_gtk3_radiogroup_entry_t sid_models_c64dtv[] = {
    { "DTVSID (ReSID-DTV)", SID_MODEL_DTVSID },
    { "6581",               SID_MODEL_6581 },
    { "8580",               SID_MODEL_8580 },
    { "8580 + digi boost",  SID_MODEL_8580D },
    { NULL,                 -1 }
};

/** \brief  SID models used in the CBM-II 510/520 models
 */
static const vice_gtk3_radiogroup_entry_t sid_models_cbm5x0[] = {
    { "6581",               SID_MODEL_6581 },
    { "8580",               SID_MODEL_8580 },
    { "8580 + digi boost",  SID_MODEL_8580D },
    { NULL,                 -1 }
};


/** \brief  Reference to the machine model widget
 *
 * Used to update the widget when the SID model changes
 */
static GtkWidget *machine_widget = NULL;


/** \brief  Extra callback for the SID type radio buttons
 *
 * \param[in]   widget      radio button triggering the event
 * \param[in]   sid_type    SID type
 */
static void sid_model_callback(GtkWidget *widget, int sid_type)
{
    GtkWidget *parent;
    void (*callback)(int);

    /* sync mixer widget */
    mixer_widget_sid_type_changed();

    parent = gtk_widget_get_parent(widget);
    callback = g_object_get_data(G_OBJECT(parent), "ExtraCallback");
    if (callback != NULL) {
        callback(sid_type);
    }
}


/** \brief  Create SID model widget
 *
 * Creates a SID model widget, depending on `machine_class`. Also sets a
 * callback to force an update of the 'machine model' widget.
 *
 * \param[in,out]   machine_model_widget    reference to machine model widget
 *
 * \return  GtkGrid
 */
GtkWidget *sid_model_widget_create(GtkWidget *machine_model_widget)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *group;
    const vice_gtk3_radiogroup_entry_t *models;

    machine_widget = machine_model_widget;

    switch (machine_class) {

        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_VSID:
            models = sid_models_c64;
            break;

        case VICE_MACHINE_CBM5x0:   /* fall through */
        case VICE_MACHINE_CBM6x0:
            models = sid_models_cbm5x0;
            break;

        case VICE_MACHINE_PLUS4:    /* fall through */
        case VICE_MACHINE_PET:      /* fall through */
        case VICE_MACHINE_VIC20:
            models = sid_models_c64;
            break;

        case VICE_MACHINE_C64DTV:
            models = sid_models_c64dtv;
            break;

        default:
            /* shouldn't get here */
            models = sid_models_none;
    }

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>SID model</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    group = vice_gtk3_resource_radiogroup_new("SidModel",
                                              models,
                                              GTK_ORIENTATION_VERTICAL);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);
    vice_gtk3_resource_radiogroup_add_callback(group, sid_model_callback);

    /* SID cards for the Plus4, PET or VIC20:
     *
     *  plus4: http://plus4world.powweb.com/hardware.php?ht=11
     *  pet  : http://www.cbmhardware.de/show.php?r=14&id=71/PETSID
     *  vic20: c64 cart adapter with any c64 sid cart
     *
     * check if Plus4/PET/VIC20 has a SID cart active:
     */
    if (machine_class == VICE_MACHINE_PLUS4
            || machine_class == VICE_MACHINE_PET
            || machine_class == VICE_MACHINE_VIC20) {
        int sidcart_enabled = 0;

        /* make SID widget sensitive, depending on if a SID cart is active */
        resources_get_int("SidCart", &sidcart_enabled);
        gtk_widget_set_sensitive(grid, (gboolean)sidcart_enabled);
    }
    return grid;
}


/** \brief  Update the SID model widget
 *
 * \param[in,out]   widget      SID model widget
 * \param[in]       model       SID model ID
 */
void sid_model_widget_update(GtkWidget *widget, int model)
{
    vice_gtk3_radiogroup_set_index(widget, model);
    mixer_widget_sid_type_changed();
}


/** \brief  Synchronize the widget with its resource
 *
 * \param[in,out]   widget  SID model widget
 */
void sid_model_widget_sync(GtkWidget *widget)
{
    int model;

    if (resources_get_int("SidModel", &model) < 0) {
        debug_gtk3("failed to get SidModel resource");
        return;
    }
    sid_model_widget_update(widget, model);
}


/** \brief  Set extra callback to trigger when the model changes
 *
 * \param[in]   widget      the SID model widget
 * \param[in]   callback    function to call on model change
 */
void sid_model_widget_set_callback(GtkWidget *widget, void (*callback)(int))
{
    g_object_set_data(G_OBJECT(widget), "ExtraCallback", (gpointer)callback);
}
