/** \file   settings_c64memhacks.c
 * \brief   Settings widget to control C64 memory expansion hacks
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES MemoryHack          x64 x64sc
 * $VICERES C64_256Kbase        x64 x64sc
 * $VICERES C64_256Kfilename    x64 x64sc
 * $VICERES PLUS60Kbase         x64 x64sc
 * $VICERES PLUS60Kfilename     x64 x64sc
 * $VICERES PLUS256Kfilename    x64 x64sc
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

#include "c64-memory-hacks.h"
#include "debug_gtk3.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "settings_c64memhacks.h"


/** \brief  List of C64 memory hack devices
 */
static const vice_gtk3_radiogroup_entry_t mem_hack_devices[] = {
    { "None",       MEMORY_HACK_NONE },
    { "C64 256K",   MEMORY_HACK_C64_256K },
    { "+60K",       MEMORY_HACK_PLUS60K },
    { "+256K",      MEMORY_HACK_PLUS256K },
    { NULL,         -1 }
};

/**\brief   List of I/O base addresses for the C64_256K memory hack
 */
static const vice_gtk3_radiogroup_entry_t c64_256k_base_addresses[] = {
    { "$DE00", 0xde00 },
    { "$DE80", 0xde80 },
    { "$DF00", 0xdf00 },
    { "$DF80", 0xdf80 },
    { NULL,        -1 }
};

/**\brief   List of I/O base addresses for the +60K memory hack
 */
static const vice_gtk3_radiogroup_entry_t plus_60k_base_addresses[] = {
    { "$D040", 0xd040 },
    { "$D100", 0xd100 },
    { NULL,        -1 }
};


/*
 * References to widget that need to be enabled/disabled, depending on the
 * memory expansion hack selected.
 */

/** \brief  c64 256K widget reference */
static GtkWidget *c64_256k_widget = NULL;
/** \brief  Plus60K widget reference */
static GtkWidget *plus_60k_widget = NULL;
/** \brief  Plus256K image widget reference */
static GtkWidget *plus_256k_widget = NULL;


/** \brief  Create left-aligned, 8 pixels left-indented label
 *
 * \param[in]   text    text for the label
 *
 * \return  new GtkLabel
 */
static GtkWidget *create_label(const char *text)
{
    GtkWidget *label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 8);
    return label;
}

/** \brief  Update sensitivity of memory hack widgets based on active hack
 *
 * \param[in]   hack_id ID of currently active memory hack
 */
static void update_widgets_sensitivity(int hack_id)
{
    switch (hack_id) {
        case MEMORY_HACK_NONE:
            gtk_widget_set_sensitive(c64_256k_widget,  FALSE);
            gtk_widget_set_sensitive(plus_60k_widget,  FALSE);
            gtk_widget_set_sensitive(plus_256k_widget, FALSE);
            break;
        case MEMORY_HACK_C64_256K:
            gtk_widget_set_sensitive(c64_256k_widget,  TRUE);
            gtk_widget_set_sensitive(plus_60k_widget,  FALSE);
            gtk_widget_set_sensitive(plus_256k_widget, FALSE);
            break;
        case MEMORY_HACK_PLUS60K:
            gtk_widget_set_sensitive(c64_256k_widget,  FALSE);
            gtk_widget_set_sensitive(plus_60k_widget,  TRUE);
            gtk_widget_set_sensitive(plus_256k_widget, FALSE);
            break;
        case MEMORY_HACK_PLUS256K:
            gtk_widget_set_sensitive(c64_256k_widget,  FALSE);
            gtk_widget_set_sensitive(plus_60k_widget,  FALSE);
            gtk_widget_set_sensitive(plus_256k_widget, TRUE);
            break;
        default:
            /* shouldn't get here */
            break;
    }
}

/** \brief  Extra handler for the "toggled" event of the mem hack radio buttons
 *
 * This handler enables/disables the widgets related to the memory hack selected
 *
 * \param[in]   widget  radio button of the select hack
 * \param[in]   hack_id hack ID
 */
static void on_hack_toggled(GtkWidget *widget, gpointer hack_id)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        update_widgets_sensitivity(GPOINTER_TO_INT(hack_id));
    }
}

/** \brief  Create widget to select the memory hacks device
 *
 * \return  GtkGrid
 */
static GtkWidget *memory_hacks_device_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *group;
    int        index;

    grid = vice_gtk3_grid_new_spaced_with_label(8, 0,
                                                "C64 memory expansion hack device",
                                                1);
    group = vice_gtk3_resource_radiogroup_new("MemoryHack",
                                              mem_hack_devices,
                                              GTK_ORIENTATION_HORIZONTAL);

    gtk_grid_set_column_spacing(GTK_GRID(group), 8);
    gtk_widget_set_margin_top(group, 8);
    gtk_widget_set_margin_start(group, 8);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);
    gtk_widget_show_all(grid);

    /* now set up extra event handlers on the radio buttons to be able to
     * enable/disable widgets */
    for (index = 0; mem_hack_devices[index].id >= 0; index++) {
        GtkWidget *radio = gtk_grid_get_child_at(GTK_GRID(group), index, 0);

        if (radio != NULL && GTK_IS_RADIO_BUTTON(radio)) {
            g_signal_connect_unlocked(radio,
                                      "toggled",
                                      G_CALLBACK(on_hack_toggled),
                                      GINT_TO_POINTER(mem_hack_devices[index].id));
        }
    }
    return grid;
}

/** \brief  Create widget for memory hack
 *
 * Create a grid with \a title and I/O base and/or image file browser widgets.
 *
 * \param[in]   title               widget title
 * \param[in]   base_resource       resource name of the memhack I/O base address (optional)
 * \param[in]   base_addresses      radio group entries for the I/O base address (optional)
 * \param[in]   image_resource      resource name of the memhack image file (optional)
 * \param[in]   image_browser_title title of the file chooser dialog to select image file (optional)
 *
 * \return  GtkGrid
 */
static GtkWidget *create_memhack_widget(const char                         *title,
                                        const char                         *base_resource,
                                        const vice_gtk3_radiogroup_entry_t *base_addresses,
                                        const char                         *image_resource,
                                        const char                         *image_browser_title)
{
    GtkWidget *grid;
    int        row = 1;

    grid = vice_gtk3_grid_new_spaced_with_label(8, 8, title, 2);

    /* add I/O base widget */
    if (base_resource != NULL && base_addresses != NULL) {
        GtkWidget *label;
        GtkWidget *group;

        label = create_label("I/O base");
        group = vice_gtk3_resource_radiogroup_new(base_resource,
                                                  base_addresses,
                                                  GTK_ORIENTATION_HORIZONTAL);
        gtk_grid_set_column_spacing(GTK_GRID(group), 8);
        gtk_widget_set_margin_start(group, 8);
        gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), group, 1, row, 1, 1);
        row++;
    }

    /* add image file widget */
    if (image_resource != NULL) {
        GtkWidget *label;
        GtkWidget *browser;

        label   = create_label("Image file");
        browser = vice_gtk3_resource_browser_new(image_resource,
                                                 NULL,
                                                 NULL,
                                                 image_browser_title,
                                                 NULL,
                                                 NULL);
       gtk_grid_attach(GTK_GRID(grid), label,   0, row, 1, 1);
       gtk_grid_attach(GTK_GRID(grid), browser, 1, row, 1, 1);
       row++;
    }

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create widget controlling C64 memory hacks
 *
 * \param[in]   parent  parent widhet, used for dialog
 *
 * \return  GtkGrid
 */
GtkWidget *settings_c64_memhacks_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *hacks;
    int        hack_id = 0;
    int        row = 1;

    grid = vice_gtk3_grid_new_spaced(8, 32);

    hacks = memory_hacks_device_widget_create();
    gtk_widget_set_hexpand(hacks, TRUE);
    gtk_grid_attach(GTK_GRID(grid), hacks, 0, row++, 1, 1);

    c64_256k_widget = create_memhack_widget("C64 256K",
                                            "C64_256Kbase",
                                            c64_256k_base_addresses,
                                            "C64_256Kfilename",
                                            "Select C64 256K image file");
    gtk_grid_attach(GTK_GRID(grid), c64_256k_widget, 0, row++, 1, 1);

    plus_60k_widget = create_memhack_widget("+60K",
                                            "PLUS60Kbase",
                                            plus_60k_base_addresses,
                                            "PLUS60Kfilename",
                                            "Select +60K image file");
    gtk_grid_attach(GTK_GRID(grid), plus_60k_widget, 0, row++, 1, 1);

    plus_256k_widget = create_memhack_widget("+256K",
                                             NULL,
                                             NULL,
                                             "PLUS256Kfilename",
                                             "Select +256K image file");
    gtk_grid_attach(GTK_GRID(grid), plus_256k_widget, 0, row++, 1, 1);

    /* enable/disable the widgets depending on the hack selected */
    resources_get_int("MemoryHack", &hack_id);
    update_widgets_sensitivity(hack_id);

    gtk_widget_show_all(grid);
    return grid;
}
