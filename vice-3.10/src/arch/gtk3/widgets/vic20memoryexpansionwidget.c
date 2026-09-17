/** \file   vic20memoryexpansionwidget.c
 * \brief   VIC20 memory expansion widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Allow enabling/disabling VIC-20 RAM expansions, and also provides a list of
 * common memory expansion configurations.
 */

/*
 * $VICERES RAMBlock0   xvic
 * $VICERES RAMBlock1   xvic
 * $VICERES RAMBlock2   xvic
 * $VICERES RAMBlock3   xvic
 * $VICERES RAMBlock5   xvic
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
#include <stdbool.h>

#include "debug_gtk3.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "vic20memoryexpansionwidget.h"


/** \brief  Number of RAM blocks that can be enabled in xvic
 */
#define RAM_BLOCK_COUNT 5

/** \brief  Struct containing information on the "common configurations"
 */
typedef struct common_config_s {
    const char *text;                       /**< description */
    bool        blocks[RAM_BLOCK_COUNT];    /**< enabled states for 0/1/2/3/5 */
} common_config_t;


/** \brief  List of available RAM expansions
 */
static const vice_gtk3_radiogroup_entry_t ram_blocks[] = {
    { "Block 0 (3KiB at $0400-$0FFF)", 0 },
    { "Block 1 (8KiB at $2000-$3FFF)", 1 },
    { "Block 2 (8KiB at $4000-$5FFF)", 2 },
    { "Block 3 (8KiB at $6000-$7FFF)", 3 },
    { "Block 5 (8KiB at $A000-$BFFF)", 5 },
    VICE_GTK3_RADIOGROUP_ENTRY_LIST_END
};


/** \brief  List of common memory expansion configurations
 *
 * Taken from the Gtk2 UI, kinda.
 */
static const common_config_t common_configs[] = {
    /*                              0      1      2      3      5    */
    { "No expansion memory",    { false, false, false, false, false } },
    { "3KiB (block 0)",         { true,  false, false, false, false } },
    { "8KiB (block 1)",         { false, true,  false, false, false } },
    { "16KiB (blocks 1+2)",     { false, true,  true,  false, false } },
    { "24KiB (blocks 1+2+3)",   { false, true,  true,  true,  false } },
    { "All (blocks 0+1+2+3+5)", { true,  true,  true,  true,  true  } },
    { NULL,                     { false, false, false, false, false } }
};


/** \brief  Reference to the RAM blocks widget
 *
 * Used by the combo box to set the proper check buttons
 */
static GtkWidget *blocks_widget = NULL;

/** \brief  Reference to the common configs combo box
 *
 * Used by the toggle buttons to set the proper common config
 */
static GtkWidget *configs_combo = NULL;


/** \brief  Get index in common configs array
 *
 * Inspects the various RAM blocks and tries to find a matching entry in the
 * common configs array.
 *
 * \param[in]   widget  widget containing the check buttons
 *
 * \return  index in the common configs array, or -1 when not found
 *
 * \note    Probably can remove the \a widget argument, we have a reference to
 *          that with the 'blocks_widget'.
 */
static int get_common_config_index(GtkWidget *widget)
{
    bool blocks[RAM_BLOCK_COUNT];
    int i;

    /* collect current config */
    for (i = 0; i < RAM_BLOCK_COUNT; i++) {
        GtkWidget *toggle = gtk_grid_get_child_at(GTK_GRID(widget), 0, i + 1);
        blocks[i] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle))
                    ? true : false;
    }

    /* look up current config in list of common configs */
    for (i = 0; common_configs[i].text != NULL; i++) {
        int b;

        for (b = 0; b < RAM_BLOCK_COUNT; b++) {
            if (blocks[b] != common_configs[i].blocks[b]) {
                /* no match */
                break;
            }
        }
        if (b == RAM_BLOCK_COUNT) {
            /* got a match */
            return i;
        }
    }
    return -1;
}

/** \brief  Update the common configs combo box when a RAM block toggle changed
 *
 * Collects the current configuration of enabled RAM blocks and checks that
 * against commenly used configs and sets the combo box accordingly.
 *
 * \param[in]   widget  RAM block toggle button
 */
static void update_common_config_combo(GtkWidget *widget)
{
    GtkWidget *parent;

    /* get grid containing the toggle button */
    parent = gtk_widget_get_parent(widget);
    if (GTK_IS_GRID(parent)) {
        int index = get_common_config_index(parent);
        gtk_combo_box_set_active(GTK_COMBO_BOX(configs_combo), index);
    }
}

/** \brief  Handler for the "toggled" event of the check buttons
 *
 * \param[in]   widget      check button triggering the event
 * \param[in]   user_data   RAM block ID (`int`)
 */
static void on_ram_block_toggled(GtkWidget *widget, gpointer user_data)
{
    int old_state = 0;
    int new_state = 0;
    int block = GPOINTER_TO_INT(user_data);

    resources_get_int_sprintf("RamBlock%d", &old_state, block);
    new_state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    if (new_state != old_state) {
        resources_set_int_sprintf("RamBlock%d", new_state, block);
        /* update common configurations combo box */
        update_common_config_combo(widget);
    }
}


/** \brief  Handler for the "changed" event of the "commong config" combobox
 *
 * If a valid item is selected, this sets the check buttons determining which
 * RAM expansions are enabled. The event handlers of the check buttons will set
 * the proper resources.
 *
 * \param[in]   widget      "common config" combo box
 * \param[in]   user_data   data for the event (unused)
 */
static void on_common_config_changed(GtkWidget *widget, gpointer user_data)
{
    const bool *config;
    int index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    int count = G_N_ELEMENTS(common_configs) - 1;
    int i;

    if (index < 0 || index >= count) {
        return;
    }

    config = common_configs[index].blocks;
    for (i = 0; i < 5; i++) {
        GtkWidget *check = gtk_grid_get_child_at(GTK_GRID(blocks_widget), 0, i +1);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                     (gboolean)config[i]);
    }
}

/** \brief  Create VIC20 common RAM configurations widget
 *
 * \return  GtkGrid
 */
static GtkWidget *vic20_common_config_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    int        i;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         "<span font_weight=\"semibold\">Common configurations</span>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    configs_combo = gtk_combo_box_text_new();
    for (i = 0; common_configs[i].text != NULL; i++) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(configs_combo),
                                  NULL, common_configs[i].text);
    }
    g_signal_connect(configs_combo,
                     "changed",
                     G_CALLBACK(on_common_config_changed),
                     NULL);

    gtk_grid_attach(GTK_GRID(grid), configs_combo, 0, 1, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create VIC20 RAM blocks widget
 *
 * \return  GtkGrid
 */
static GtkWidget *vic20_ram_blocks_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    int        i;

    grid = gtk_grid_new();

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         "<span font_weight=\"semibold\">RAM blocks</span>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    for (i = 0; ram_blocks[i].name != NULL; i++) {
        GtkWidget *check;
        int        active = 0;

        check = gtk_check_button_new_with_label(ram_blocks[i].name);
        resources_get_int_sprintf("RamBlock%d", &active, ram_blocks[i].id);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), active);
        g_signal_connect(G_OBJECT(check),
                         "toggled",
                         G_CALLBACK(on_ram_block_toggled),
                         GINT_TO_POINTER(ram_blocks[i].id));
        gtk_grid_attach(GTK_GRID(grid), check, 0, i + 1, 1, 1);
    }

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create VIC20 memory expansion widget
 *
 * Contains two sub-widgets, one "common configurations" combo box, and one
 * group of check buttons to enable/disable each RAM expansion
 *
 * \return  GtkGrid
 */
GtkWidget *vic20_memory_expansion_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *common;
    int cfg_idx;

    grid = vice_gtk3_grid_new_spaced_with_label(8, 8, "Memory expansions", 1);

    common = vic20_common_config_widget_create();
    gtk_widget_set_margin_start(common, 8);
    gtk_grid_attach(GTK_GRID(grid), common, 0, 1, 1, 1);

    blocks_widget = vic20_ram_blocks_widget_create();
    gtk_widget_set_margin_start(blocks_widget, 8);
    gtk_grid_attach(GTK_GRID(grid), blocks_widget, 0, 2, 1, 1);

    /* set proper 'common configs' combobox index */
    cfg_idx = get_common_config_index(blocks_widget);
    gtk_combo_box_set_active(GTK_COMBO_BOX(configs_combo), cfg_idx);

    gtk_widget_show_all(grid);
    return grid;
}
