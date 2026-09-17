/** \file   settings_tapeport.c
 * \brief   Tape port settings dialog widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES TrapDevice1                     -xscpu64 -vsid
 * $VICERES TrapDevice2                     -xscpu64 -vsid -x64sc -x64 -xvic -xplus4 -xcbm2 -xcbm5x0
 * $VICERES TapePort1Device                 -xscpu64 -vsid
 * $VICERES TapePort2Device                 -xscpu64 -vsid -x64sc -x64 -xvic -xplus4 -xcbm2 -xcbm5x0
 * $VICERES DatasetteResetWithCPU           -xscpu64 -vsid
 * $VICERES DatasetteSound                  -xscpu64 -vsid
 * $VICERES DatasetteSoundVolume            -xscpu64 -vsid
 * $VICERES DatasetteSpeedTuning            -xscpu64 -vsid
 * $VICERES DatasetteTapeAzimuthError       -xscpu64 -vsid
 * $VICERES DatasetteTapeWobbleAmplitude    -xscpu64 -vsid
 * $VICERES DatasetteTapeWobbleFrequency    -xscpu64 -vsid
 * $VICERES DatasetteZeroGapDelay           -xscpu64 -vsid
 * $VICERES CPClockF83Save                  -xscpu64 -vsid
 * $VICERES TapecartUpdateTCRT              x64 x64sc x128
 * $VICERES TapecartOptimizeTCRT            x64 x64sc x128
 * $VICERES TapecartLogLevel                x64 x64sc x128
 * $VICERES TapecartTCRTFilename            x64 x64sc x128
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
#include <stdlib.h>

#include "datasette.h"
#include "lib.h"
#include "machine.h"
#include "resources.h"
#include "tapecart.h"
#include "tapeport.h"
#include "vice_gtk3.h"

#include "settings_tapeport.h"

/** \brief  Column indexes in the tapeport devices model
 */
enum {
    COL_DEVICE_ID,          /**< device ID (int) */
    COL_DEVICE_NAME,        /**< device name (str) */
    COL_DEVICE_TYPE_ID,     /**< device type (int) */
    COL_DEVICE_TYPE_DESC    /**< device type description (str) */
};

/** \brief  List of log levels and their descriptions for the Tapecart
 */
static const vice_gtk3_combo_entry_int_t tcrt_loglevels[] = {
    { "0 (errors only)",                         0 },
    { "1 (0 + mode changes and command bytes)",  1 },
    { "2 (1 + command parameter details)",       2 },
    { NULL,                                     -1 }
};


/*
 * Reference to widgets to be able to enable/disabled them through event
 * handlers
 */

/** \brief  Tape port #1 device combo */
static GtkWidget *port1_type = NULL;

/** \brief  Tape port #2 device combo */
static GtkWidget *port2_type = NULL;

/** \brief  Datasette 1 device traps toggle button */
static GtkWidget *ds_traps1 = NULL;

/** \brief  Datasette 2 device traps toggle button */
static GtkWidget *ds_traps2 = NULL;

/** \brief  Datasette reset toggle button */
static GtkWidget *ds_reset = NULL;

/** \brief  Datasette zerogap delay spine button */
static GtkWidget *ds_zerogap = NULL;

/** \brief  Datasette speed tuning spin button */
static GtkWidget *ds_speed = NULL;

/** \brief  Datasette wobble frequency spin button */
static GtkWidget *ds_wobblefreq = NULL;

/** \brief  Datasette wobble amplitude spin button */
static GtkWidget *ds_wobbleamp = NULL;

/** \brief  Datasette align spin button */
static GtkWidget *ds_align = NULL;

/** \brief  Datasette sound emulation toggle button */
static GtkWidget *ds_sound = NULL;

/** \brief  F83 RTC toggle button */
static GtkWidget *f83_rtc = NULL;

/** \brief  Tapecart save-when-changed toggle button */
static GtkWidget *tcrt_update = NULL;

/** \brief  Tapecart optimize-when-saving toggle button */
static GtkWidget *tcrt_optimize = NULL;

/** \brief  Tapecart log level radiogroup */
static GtkWidget *tcrt_loglevel = NULL;

/** \brief  Tapecart image file browser */
static GtkWidget *tcrt_filename = NULL;

/** \brief  Tapecart flush button */
static GtkWidget *tcrt_flush = NULL;


/** \brief  Determine if current machine supports tapecart
 *
 * \return  `TRUE` if tapecart emulation is available
 */
static gboolean machine_has_tapecart(void)
{
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_C128:
            return TRUE;
        default:
            return FALSE;
    }
}

/** \brief  Determine if the current machine has a second tape port
 *
 * \return  `TRUE` if the machine has a second tape port
 */
static gboolean machine_has_second_tape_port(void)
{
    /* only a PET has a second tape port */
    return (gboolean)(machine_class == VICE_MACHINE_PET);
}

/** \brief  Set Datasette widget active/inactive
 *
 * \param[in]   state   status
 */
static void set_datasette_active(gboolean state)
{
    gtk_widget_set_sensitive(ds_reset,      state);
    gtk_widget_set_sensitive(ds_zerogap,    state);
    gtk_widget_set_sensitive(ds_speed,      state);
    gtk_widget_set_sensitive(ds_wobblefreq, state);
    gtk_widget_set_sensitive(ds_wobbleamp,  state);
    gtk_widget_set_sensitive(ds_align,      state);
    gtk_widget_set_sensitive(ds_sound,      state);
    /* xPET does not have device traps right now, grey out the selection */
    if (machine_class == VICE_MACHINE_PET) {
        state = FALSE;
    }
    gtk_widget_set_sensitive(ds_traps1, state);
    if (machine_has_second_tape_port()) {
        gtk_widget_set_sensitive(ds_traps2, state);
    }
}

/** \brief Set CP Clock F83 widget active/inactive
 *
 * \param[in]   state   status
 */
static void set_f83_active(gboolean state)
{
    gtk_widget_set_sensitive(f83_rtc, state);
}

/** \brief  Set tapecart active/inactive
 *
 * \param[in]   state   status
 */
static void set_tapecart_active(gboolean state)
{
    if (machine_has_tapecart()) {
        gtk_widget_set_sensitive(tcrt_update,   state);
        gtk_widget_set_sensitive(tcrt_optimize, state);
        gtk_widget_set_sensitive(tcrt_loglevel, state);
        gtk_widget_set_sensitive(tcrt_filename, state);
        gtk_widget_set_sensitive(tcrt_flush,    state);
    }
}

/** \brief  Set individual options active/inactive
 *
 * \param[in]   id      id of active device
 */
static void set_options_widgets_sensitivity(int id)
{
    set_datasette_active(id == TAPEPORT_DEVICE_DATASETTE);
    set_f83_active(id == TAPEPORT_DEVICE_CP_CLOCK_F83);
    set_tapecart_active(id == TAPEPORT_DEVICE_TAPECART);
}

/** \brief  Handler for the 'clicked' event of the tapecart flush button
 *
 * \param[in]   widget  button (unused)
 * \param[in]   data    extra event data (unused)
 */
static void on_tcrt_flush_clicked(GtkWidget *widget, gpointer data)
{
    tapecart_flush_tcrt();
}

/** \brief  Create left-aligned label with Pango markup
 *
 * \param[in]   text    text for label (can use Pango markup)
 *
 * \return  GtkLabel
 */
static GtkWidget *label_helper(const char *text)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}

/** \brief  Create widgets for the datasette
 *
 * TODO:    Someone needs to check the spin button bounds and steps for sane
 *          values.
 *
 * \return  GtkGrid
 */
static GtkWidget *create_datasette_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *scale;
    int        row = 0;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    /* TRUE looks better but makes the dialog too wide :( */
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), FALSE);

    /* header */
    label = label_helper("<b>Datasette C2N</b>");
    gtk_widget_set_margin_bottom(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 4, 1);
    row++;

    /* device traps for datasette #1 and #2 */
    ds_traps1 = vice_gtk3_resource_check_button_new("TrapDevice1",
                                                    "Kernal traps for Device #1 (required for t64)");
    gtk_grid_attach(GTK_GRID(grid), ds_traps1, 0, row, 2, 1);
    if (machine_has_second_tape_port()) {
        ds_traps2 = vice_gtk3_resource_check_button_new("TrapDevice2",
                                                        "Kernal traps for Device #2 (required for t64)");
        gtk_grid_attach(GTK_GRID(grid), ds_traps2, 2, row, 2, 1);
    }
    row++;

    ds_reset = vice_gtk3_resource_check_button_new("DatasetteResetWithCPU",
                                                   "Reset datasette with CPU");
    gtk_widget_set_margin_bottom(ds_reset, 8);
    gtk_grid_attach(GTK_GRID(grid), ds_reset, 0, row, 2, 1);

    row++;

    ds_sound = vice_gtk3_resource_check_button_new("DatasetteSound",
                                                   "Datasette sound");
    gtk_widget_set_margin_bottom(ds_sound, 8);
    gtk_grid_attach(GTK_GRID(grid), ds_sound, 0, row, 1, 1);

    scale = vice_gtk3_resource_scale_custom_new_printf("%s",
                                                       GTK_ORIENTATION_HORIZONTAL,
                                                       0,
                                                       TAPE_SOUND_VOLUME_MAX,
                                                       0,
                                                       (TAPE_SOUND_VOLUME_MAX * 100) / TAPE_SOUND_VOLUME_ONE,
                                                       10,
                                                       "%3.0f%%",
                                                       "DatasetteSoundVolume");
    gtk_widget_set_hexpand(scale, TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
    gtk_grid_attach(GTK_GRID(grid), scale, 1, row, 3, 1);
    row++;

    label      = label_helper("TAP v0 Zero gap delay");
    ds_zerogap = vice_gtk3_resource_spin_int_new("DatasetteZeroGapDelay",
                                                 0, TAP_ZERO_GAP_DELAY_MAX, 1);
    gtk_widget_set_margin_bottom(ds_zerogap, 8);
    gtk_grid_attach(GTK_GRID(grid), label,      0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), ds_zerogap, 1, row, 1, 1);

    label    = label_helper("Tape speed tuning");
    ds_speed = vice_gtk3_resource_spin_custom_new("DatasetteSpeedTuning",
                                                  -TAP_SPEED_TUNING_MAX,
                                                  TAP_SPEED_TUNING_MAX,
                                                  -50.0,
                                                  50.0,
                                                  1.0,
                                                  "%4.1f%%");
    gtk_widget_set_margin_bottom(ds_speed, 8);
    gtk_grid_attach(GTK_GRID(grid), label,    2, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), ds_speed, 3, row, 1, 1);
    row++;

    label = label_helper("Wobble frequency");
    ds_wobblefreq = vice_gtk3_resource_spin_custom_new("DatasetteTapeWobbleFrequency",
                                                       0, TAP_WOBBLE_FREQ_MAX,
                                                       0.0, 50.0, 1.0,
                                                       "%4.1fHz");
    gtk_widget_set_margin_bottom(ds_wobblefreq, 8);
    gtk_grid_attach(GTK_GRID(grid), label,         0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), ds_wobblefreq, 1, row, 1, 1);

    label    = label_helper("Alignment error");
    ds_align = vice_gtk3_resource_spin_int_new("DatasetteTapeAzimuthError",
                                               0,
                                               TAP_AZIMUTH_ERROR_MAX,
                                               TAP_AZIMUTH_ERROR_MAX / 1000);
    gtk_widget_set_margin_bottom(ds_align, 8);
    gtk_grid_attach(GTK_GRID(grid), label,    2, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), ds_align, 3, row, 1, 1);
    row++;

    label        = label_helper("Wobble amplitude");
    ds_wobbleamp = vice_gtk3_resource_spin_custom_new("DatasetteTapeWobbleAmplitude",
                                                      0, TAP_WOBBLE_AMPLITUDE_MAX,
                                                      0.0, 50.0, 0.5,
                                                      "%5.1f%%");
    gtk_grid_attach(GTK_GRID(grid), label,        0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), ds_wobbleamp, 1, row, 1, 1);

    return grid;
}

/** \brief  Create widget to handler the Cassette Port Clock F83 resources
 *
 * \return  GtkGrid
 */
static GtkWidget *create_cpcf83_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label   = label_helper("<b>Cassette Port Clock F83</b>");
    f83_rtc = vice_gtk3_resource_check_button_new("CPClockF83Save",
                                                   "Save RTC data when changed");
    gtk_grid_attach(GTK_GRID(grid), label,   0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), f83_rtc, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create widget to handle the tapecart resources
 *
 * \return  GtkGrid
 */
static GtkWidget *create_tapecart_widget(void)
{
    GtkWidget  *grid;
    GtkWidget  *label;
    GtkWidget  *wrapper;
    const char *patterns[] = { "*.tcrt", NULL };
    int         row = 0;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    /* header */
    label = label_helper("<b>Tapecart</b>");
    gtk_widget_set_margin_bottom(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 3, 1);
    row++;

    /* TapecartTCRTFilename */
    label         = label_helper("TCRT filename");
    tcrt_filename = vice_gtk3_resource_filechooser_new("TapecartTCRTFilename",
                                                       GTK_FILE_CHOOSER_ACTION_OPEN);
    vice_gtk3_resource_filechooser_set_custom_title(tcrt_filename,
                                                    "Select tapecart image");
    vice_gtk3_resource_filechooser_set_filter(tcrt_filename,
                                              "Tapecart images",
                                              patterns,
                                              TRUE);
    tcrt_flush = gtk_button_new_with_label("Flush image");
    gtk_widget_set_hexpand(tcrt_flush, FALSE);
    gtk_grid_attach(GTK_GRID(grid), label,         0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), tcrt_filename, 1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), tcrt_flush,    2, row, 1, 1);
    g_signal_connect(G_OBJECT(tcrt_flush),
                     "clicked",
                     G_CALLBACK(on_tcrt_flush_clicked),
                     NULL);
    row++;

    /* TCRT log level */
    label = label_helper("Log level");
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    tcrt_loglevel = vice_gtk3_resource_combo_int_new("TapecartLogLevel",
                                                     tcrt_loglevels);
    gtk_widget_set_margin_top(tcrt_loglevel, 8);
    gtk_widget_set_margin_bottom(tcrt_loglevel, 8);
    gtk_grid_attach(GTK_GRID(grid), tcrt_loglevel, 1, row, 2, 1);
    row++;

    /* wrapper for update/optimize check buttons */
    wrapper = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(wrapper), TRUE);
    /* TapecartUpdateTCRT */
    tcrt_update = vice_gtk3_resource_check_button_new("TapecartUpdateTCRT",
                                                      "Save data when changed");
    gtk_grid_attach(GTK_GRID(wrapper), tcrt_update, 0, 0, 1, 1);
    /* TapecartOptimizeTCRT */
    tcrt_optimize = vice_gtk3_resource_check_button_new("TapecartOptimizeTCRT",
                                                        "Optimize data when changed");
    gtk_grid_attach(GTK_GRID(wrapper), tcrt_optimize, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), wrapper, 0, row, 3, 1);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Handler for the 'changed' event of the device combobox
 *
 * Sets the active tapeport device via the "TapePort[12]Device" resource.
 *
 * \param[in]   combo       device combo box
 * \param[in]   portnum     tape port number
 */
static void on_device_changed(GtkComboBox *combo, gpointer portnum)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;

    model = gtk_combo_box_get_model(combo);
    if (gtk_combo_box_get_active_iter(combo, &iter)) {
        gint   id;
        gchar *name = NULL;
        gint   port = GPOINTER_TO_INT(portnum);

        gtk_tree_model_get(model,
                           &iter,
                           COL_DEVICE_ID, &id,
                           COL_DEVICE_NAME, &name,
                           -1);
        resources_set_int_sprintf("TapePort%iDevice", id, port);
        set_options_widgets_sensitivity(id);

        g_free(name);
    }
}

/** \brief  Set tapeport device ID
 *
 * Sets the currently selected combobox item via device ID.
 *
 * To avoid updating the related resource via the combobox' event handler, use
 * the \a blocked argument.
 *
 * \param[in]   combo   device combo box
 * \param[in]   id      device ID
 * \param[in]   blocked block 'changed' signal handler
 */
static gboolean set_device_id(GtkComboBox *combo, gint id, gboolean blocked)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;
    gulong        handler_id;
    gboolean      result = FALSE;

    /* do we need to block the 'changed' event handler? */
    if (blocked) {
        /* look up handler ID by callback */
        handler_id = g_signal_handler_find(combo,
                                           G_SIGNAL_MATCH_FUNC,
                                           0,       /* signal_id */
                                           0,       /* detail */
                                           NULL,    /* closure */
                                           on_device_changed,   /* func */
                                           NULL);
        if (handler_id > 0) {
            g_signal_handler_block(combo, handler_id);
        }
    }

    /* iterate the model until we find the device ID */
    model = gtk_combo_box_get_model(combo);
    if (gtk_tree_model_get_iter_first(model, &iter)) {
        do {
            gint current;

            gtk_tree_model_get(model, &iter, COL_DEVICE_ID, &current, -1);
            if (id == current) {
                gtk_combo_box_set_active_iter(combo, &iter);
                result = TRUE;
                break;
            }
        } while (gtk_tree_model_iter_next(model, &iter));
    }

    /* set options checkboxes "greyed-out" state */
    set_options_widgets_sensitivity(id);

    /* unblock signal, if blocked */
    if (blocked) {
        g_signal_handler_unblock(combo, handler_id);
    }

    return result;
}

/** \brief  Create model for the device combobox
 *
 * Create a model with (dev-id, dev-name, dev-type-id, dev-type-desc).
 *
 * \param[in]   port    tape port number (1 or 2 (PET))
 *
 * \return  model
 */
static GtkListStore *create_device_model(int port)
{
    GtkListStore    *model;
    tapeport_desc_t *devices;
    tapeport_desc_t *dev;

    model = gtk_list_store_new(4,
                               G_TYPE_INT,      /* ID */
                               G_TYPE_STRING,   /* name */
                               G_TYPE_INT,      /* type ID */
                               G_TYPE_STRING    /* type description */
                               );
    devices = tapeport_get_valid_devices(port - 1 /*index, not port */, TRUE);
    for (dev = devices; dev->name != NULL; dev++) {
        GtkTreeIter iter;

        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model,
                           &iter,
                           COL_DEVICE_ID, dev->id,
                           COL_DEVICE_NAME, dev->name,
                           COL_DEVICE_TYPE_ID, dev->device_type,
                           COL_DEVICE_TYPE_DESC, tapeport_get_device_type_desc(dev->device_type),
                           -1);
    }
    lib_free(devices);

    return model;
}

/** \brief  Create combobox for the tapeport devices
 *
 * Create a combobox with valid tapeport devices for current machine.
 *
 * The model of the combobox contains device ID, name and type, of which name
 * is shown and ID is used to set the related resource.
 *
 * \param[in]   port    tape port number (1 or 2 (PET only))
 *
 * \return  GtkComboBox
 *
 * \todo    Try using the device type to create little headers in the combobox,
 *          grouping the devices by type. Might be overkill for some machines
 *          that only have a few tapeport devices, we'll see.
 *          I tried using a second column for the device type description, and
 *          althought it doesn't look bad in the popup list, when the popup
 *          isn't active it looks weird ;)
 *          So for now the device type isn't used.
 */
static GtkWidget *create_device_combobox(int port)
{
    GtkWidget       *combo;
    GtkListStore    *model;
    GtkCellRenderer *name_renderer;
#if 0
    GtkCellRenderer *type_renderer;
#endif

    /* TODO:    Check if the model can be shared for both ports, perhaps the
     *          second tape port has a different set of supported devices
     */
    model = create_device_model(port);

    /* create combobox with a single cell renderer for the device name column */
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
    name_renderer = gtk_cell_renderer_text_new();
#if 0
    type_renderer = gtk_cell_renderer_text_new();
#endif
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),
                               name_renderer,
                               TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo),
                                   name_renderer,
                                   "text", COL_DEVICE_NAME,
                                   NULL);
#if 0
    gtk_cell_layout_pack_end(GTK_CELL_LAYOUT(combo),
                             type_renderer,
                             TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo),
                                   type_renderer,
                                   "text", COL_DEVICE_TYPE_DESC,
                                   NULL);
#endif

    g_signal_connect(G_OBJECT(combo),
                     "changed",
                     G_CALLBACK(on_device_changed),
                     GINT_TO_POINTER(port));

    return combo;
}

/** \brief  Create combobox(es) to select device type for port 1 (and 2 for PET)
 *
 * \return  GtkGrid
 */
static GtkWidget *create_device_types_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    int        second;

    second = machine_has_second_tape_port();

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    /* header */
    label = label_helper("<b>Tape port device types</b>");
    gtk_widget_set_margin_bottom(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, second ? 4 : 2, 1);

    /* first tape port */
    label      = label_helper("Tape port #1");
    port1_type = create_device_combobox(TAPEPORT_UNIT_1);
    gtk_widget_set_hexpand(port1_type, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label,      0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), port1_type, 1, 1, 1, 1);

    /* PET has a second tape port */
    if (second) {
        label      = label_helper("Tape port #2");
        port2_type = create_device_combobox(TAPEPORT_UNIT_2);
        gtk_widget_set_hexpand(port2_type, TRUE);
        gtk_grid_attach(GTK_GRID(grid), label,      2, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), port2_type, 3, 1, 1, 1);
    }

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create widget to select/control tape port devices
 *
 * \param[in]   parent  parent widget (ignored)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_tapeport_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *devices;
    GtkWidget *datasette;
    GtkWidget *cpcf83;
    int        device_id = 0;
    int        row = 0;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    /* comboboxes with the tapeport devices */
    devices = create_device_types_widget();
    gtk_widget_set_margin_bottom(devices, 8);
    gtk_grid_attach(GTK_GRID(grid), devices, 0, row, 1, 1);
    row++;

    /* datasette device settings */
    datasette = create_datasette_widget();
    gtk_widget_set_margin_bottom(datasette, 8);
    gtk_grid_attach(GTK_GRID(grid), datasette, 0, row, 1, 1);
    row++;

    /* Cassette Port Clock F83 */
    cpcf83 = create_cpcf83_widget();
    gtk_grid_attach(GTK_GRID(grid), cpcf83, 0, row, 1, 1);
    row++;

    /* TapeCart settings */
    if (machine_has_tapecart()) {
        gtk_grid_attach(GTK_GRID(grid), create_tapecart_widget(), 0, row, 1, 1);
        row++;
    }

    /* these need to happen here, after the above widgets are created */
    /* set port1 type using the resource */
    resources_get_int("TapePort1Device", &device_id);
    set_device_id(GTK_COMBO_BOX(port1_type), device_id, TRUE);
    if (machine_has_second_tape_port()) {
        /* set port2 type using the resource */
        resources_get_int("TapePort2Device", &device_id);
        set_device_id(GTK_COMBO_BOX(port2_type), device_id, TRUE);
    }

    gtk_widget_show_all(grid);
    return grid;
}
