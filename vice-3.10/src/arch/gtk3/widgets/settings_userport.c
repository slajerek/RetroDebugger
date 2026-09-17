/** \file   settings_userport.c
 * \brief   Settings widget for userport devices
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES UserportDevice          x64 x64sc xscpu64 x128 xcbm2 xvic xpet
 * $VICERES UserportRTC58321aSave   x64 x64sc xscpu64 x128 xcbm2 xvic xpet
 * $VICERES UserportRTCDS1307Save   x64 x64sc xscpu64 x128 xcbm2 xvic xpet
 * $VICERES WIC64DefaultServer      x64 x64sc xscpu64 x128 xvic
 * $VICERES WIC64HexdumpLines       x64 x64sc xscpu64 x128 xvic
 * $VICERES WIC64Logenabled         x64 x64sc xscpu64 x128 xvic
 * $VICERES WIC64LogLevel           x64 x64sc xscpu64 x128 xvic
 * $VICERES WIC64Resetuser          x64 x64sc xscpu64 x128 xvic
 * $VICERES WIC64Timezone           x64 x64sc xscpu64 x128 xvic
 * $VICERES WIC64RemoteTimeout      x64 x64sc xscpu64 x128 xvic
 * $VICERES FunMP3Dir               x64 x64sc xscpu64 x128 xvic xpet
 *
 * The following resources are not user-configurable, but set indirectly via
 * the WiC64 code, so we list them here for `gtk3-resources.py` to find:
 *
 * $VICERES WIC64IPAddress          x64 x64sc xscpu64 x128 xvic
 * $VICERES WIC64MACAddress         x64 x64sc xscpu64 x128 xvic
 * $VICERES WIC64SecToken           x64 x64sc xscpu64 x128 xvic
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

#include "debug_gtk3.h"
#include "petdiagnosticpinwidget.h"
#include "lib.h"
#include "machine.h"
#include "resources.h"
#include "userport.h"
#ifdef HAVE_LIBCURL
#include "userport_wic64.h"
#endif
#include "vice_gtk3.h"

#include "settings_userport.h"


/** \brief  Column indexes in the useport devices model
 */
enum {
    COL_DEVICE_ID,          /**< device ID (int) */
    COL_DEVICE_NAME,        /**< device name (str) */
    COL_DEVICE_TYPE_ID,     /**< device type (int) */
    COL_DEVICE_TYPE_DESC,   /**< device type description (str) */

    COLUMN_COUNT            /**< number of columns in the model */
};


/*
 * Used for the event handlers
 */

/** \brief  58321a save enable toggle button */
static GtkWidget *rtc_58321a_save = NULL;

/** \brief  ds1307 save enable toggle button */
static GtkWidget *rtc_ds1307_save = NULL;

/** \brief  diag pin enable toggle button */
static GtkWidget *diag_pin_active = NULL;

#ifdef HAVE_LIBCURL
/** \brief  WiC64 save enable settigns */
static GtkWidget *wic64_save = NULL;

static GtkWidget *wic64_server_save = NULL;
static GtkWidget *wic64_remote_timeout_save = NULL;
static GtkWidget *wic64_tz_save = NULL;

#endif

#if defined(USE_MPG123) && defined(HAVE_GLOB_H)
static GtkWidget *funmp3_save = NULL;
#endif

/** \brief  Create left-aligned label with Pango markup
 *
 * \param[in]   markup  text using Pango markup
 *
 * \return  GtkLabel
 */
static GtkWidget *label_helper(const char *markup)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), markup);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}

/** \brief  Create widget for the "UserportRTC58321aSave" resource
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_rtc_58321a_save_widget(void)
{
    return vice_gtk3_resource_check_button_new("UserportRTC58321aSave",
                                               "Enable RTC (58321a) saving");
}

/** \brief  Create widget for the "UserportRTCDS1307Save" resource
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_rtc_ds1307_save_widget(void)
{
    return vice_gtk3_resource_check_button_new("UserportRTCDS1307Save",
                                               "Enable RTC (DS1307) saving");
}

/** \brief  Create widget for the "DiagPin" resource
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_diag_pin_active_widget(void)
{
    return vice_gtk3_resource_check_button_new("DiagPin",
                                               "Enable diagnostic pin");
}

/** \brief  Set the RTC checkboxes' or WiC64 settings sensitivity based on device ID
 *
 * Use userport device \a id to determine which widget to 'grey-out'.
 *
 * \param[in]   id  userport device ID
 */
static void set_widgets_sensitivity(int id)
{
    if (rtc_58321a_save != NULL) {
        gtk_widget_set_sensitive(rtc_58321a_save, id == USERPORT_DEVICE_RTC_58321A);
    }
    if (rtc_ds1307_save != NULL) {
        gtk_widget_set_sensitive(rtc_ds1307_save, id == USERPORT_DEVICE_RTC_DS1307);
    }
    if (diag_pin_active != NULL) {
        gtk_widget_set_sensitive(diag_pin_active, id == USERPORT_DEVICE_DIAGNOSTIC_PIN);
    }
#ifdef HAVE_LIBCURL
    if (wic64_save != NULL) {
        gtk_widget_set_sensitive(wic64_save, id == USERPORT_DEVICE_WIC64);
    }
#endif
#if defined(USE_MPG123) && defined(HAVE_GLOB_H)
    if (funmp3_save != NULL) {
        gtk_widget_set_sensitive(funmp3_save, id == USERPORT_DEVICE_FUNMP3);
    }
#endif
}

/** \brief  Handler for the 'changed' event of the device combobox
 *
 * Sets the active userport device via the "UserportDevice" resource.
 *
 * \param[in]   combo       device combo box
 * \param[in]   user_data   extra event data (unused)
 */
static void on_device_changed(GtkComboBox *combo, gpointer user_data)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;

    model = gtk_combo_box_get_model(combo);
    if (gtk_combo_box_get_active_iter(combo, &iter)) {
        gint   id   = 0;
        gchar *name = NULL;

        gtk_tree_model_get(model,
                           &iter,
                           COL_DEVICE_ID, &id,
                           COL_DEVICE_NAME, &name,
                           -1);
        resources_set_int("UserportDevice", id);
        set_widgets_sensitivity(id);
        g_free(name);
    }
}

/** \brief  Set userport device ID
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

    /* set RTC checkboxes or WiC64 settings "greyed-out" state */
    set_widgets_sensitivity(id);

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
 * \return  model
 */
static GtkListStore *create_device_model(void)
{
    GtkListStore *model;
    GtkTreeIter iter;
    userport_desc_t *devices;
    userport_desc_t *dev;

    model = gtk_list_store_new(COLUMN_COUNT,
                               G_TYPE_INT,      /* ID */
                               G_TYPE_STRING,   /* name */
                               G_TYPE_INT,      /* type ID */
                               G_TYPE_STRING    /* type description */
                               );
    devices = userport_get_valid_devices(TRUE);
    for (dev = devices; dev->name != NULL; dev++) {
        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model,
                           &iter,
                           COL_DEVICE_ID,        dev->id,
                           COL_DEVICE_NAME,      dev->name,
                           COL_DEVICE_TYPE_ID,   dev->device_type,
                           COL_DEVICE_TYPE_DESC, userport_get_device_type_desc(dev->device_type),
                           -1);
    }
    lib_free(devices);

    return model;
}


/** \brief  Create combobox for the userport devices
 *
 * Create a combobox with valid userport devices for current machine.
 *
 * The model of the combobox contains device ID, name and type, of which name
 * is shown and ID is used to set the related resource.
 *
 * \return  GtkComboBox
 *
 * \todo    Try using the device type to create little headers in the combobox,
 *          grouping the devices by type. Might be overkill for some machines
 *          that only have a few userport devices, we'll see.
 *          I tried using a second column for the device type description, and
 *          althought it doesn't look bad in the popup list, when the popup
 *          isn't active it looks weird ;)
 *          So for now the device type isn't used.
 */
static GtkWidget *create_device_combobox(void)
{
    GtkWidget       *combo;
    GtkListStore    *model;
    GtkCellRenderer *name_renderer;
#if 0
    GtkCellRenderer *type_renderer;
#endif

    model = create_device_model();

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

    g_signal_connect(combo, "changed", G_CALLBACK(on_device_changed), NULL);

    return combo;
}

#ifdef HAVE_LIBCURL

/** \brief  Create widget for the "WIC64Logenabled" resource
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_wic64_logenabled_widget(void)
{
    return vice_gtk3_resource_check_button_new("WIC64Logenabled",
                                               "Enable WiC64 logging");
}

/** \brief  Create widget for the "WIC64Resetuser" resource
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_wic64_resetuser_widget(void)
{
    return vice_gtk3_resource_check_button_new("WIC64Resetuser",
                                               "Reset User when resetting WiC64");
}

/* Columns in the WIC64 timezone combobox */
enum {
    TZ_COL_IDX,     /**< ID of the timezone */
    TZ_COL_NAME     /**< name of the timezone */
};

/** \brief  Create combobox for WIC64 timezone selection
 *
 * \return  GtkComboBox
 */
static GtkWidget *create_wic64_timezone_combo(void)
{
    GtkWidget      *combo;
    const tzones_t *zones;
    size_t          znum;
    size_t          zidx;
    char            buffer[256];

    combo = vice_gtk3_resource_combo_int_new("WIC64Timezone", NULL);
    zones = userport_wic64_get_timezones(&znum);
    for (zidx = 0; zidx < znum; zidx++) {
        g_snprintf(buffer, sizeof buffer, "%d: %s",
                   zones[zidx].idx, zones[zidx].tz_name);
        vice_gtk3_resource_combo_int_append(combo, zones[zidx].idx, buffer);
    }
    vice_gtk3_resource_combo_int_sync(combo);

    return combo;
}

#if 1 /* disabled, as security token editable actually makes no sense */
/** \brief  Handler for the 'icon-press' event of the "Security token" entry
 *
 * Toggle visibility of the WIC64 security token when clicking the "eye" icon
 * in the left corner of the widget.
 *
 * \param[in]   self        security token entry
 * \param[in]   icon_pos    icon position
 * \param[in]   event       button press event data (unused)
 * \param[in]   data        extra event data (unused)
 */
static void on_sec_token_icon_press(GtkEntry             *self,
                                    GtkEntryIconPosition  icon_pos,
                                    GdkEvent             *event,
                                    gpointer              data)
{
    if (icon_pos == GTK_ENTRY_ICON_PRIMARY) {
        const char *name;
        gboolean    visible = gtk_entry_get_visibility(self);

        visible = !visible;
        gtk_entry_set_visibility(self, visible);
        if (visible) {
            name = "view-conceal-symbolic";
        } else {
            name = "view-reveal-symbolic";
        }
        gtk_entry_set_icon_from_icon_name(self, GTK_ENTRY_ICON_PRIMARY, name);
    }
}
#endif

#if defined(USE_MPG123) && defined (HAVE_GLOB_H)
/* FunMP3 path selection widget */
#include "mainlock.h"

/** \brief  Callback for the directory-select dialog
 *
 * \param[in]   dialog      directory-select dialog
 * \param[in]   filename    filename (NULL if canceled)
 * \param[in]   entry       entry box for the filename
 */
static void funmp3dir_browse_callback(GtkDialog *dialog, gchar *filename, gpointer entry)
{
    if (filename != NULL) {
        vice_gtk3_resource_entry_set(GTK_WIDGET(entry), filename);
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/** \brief  Handler for the 'clicked' event of the FunMP3 directory browse button
 *
 * \param[in]   button  FunMP3 directory browse button (unused)
 * \param[in]   entry   FunMP3 directory text entry
 */
static void on_funmp3dir_browse_clicked(GtkWidget *button, gpointer entry)
{
    GtkWidget *dialog;

    mainlock_assert_is_not_vice_thread();

    dialog = vice_gtk3_select_directory_dialog("Select FunMP3 directory",
                                               NULL,
                                               TRUE,
                                               NULL,
                                               funmp3dir_browse_callback,
                                               entry);
    gtk_widget_show_all(dialog);
}

/** \brief  Create FunMP3 path selection entry widget
 *
 */
static GtkWidget *create_funmp3dir_entry_widget(void)
{
    GtkWidget *entry;
    entry = vice_gtk3_resource_entry_new("FunMP3Dir");
    gtk_widget_set_tooltip_text(entry,
                                "Select the host directory for MP3 files");
    gtk_widget_set_hexpand(entry, TRUE);
    return entry;
}

/** \brief  Create FunMP3 path selection widget
 *
 */
static GtkWidget *create_funmp3dir_widget(void)
{
    GtkWidget *grid;
    GtkWidget *entry;
    GtkWidget *label;
    GtkWidget *browse;

    grid = vice_gtk3_grid_new_spaced(8, 0);

    label  = gtk_label_new("FunMP3 directory");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    entry  = create_funmp3dir_entry_widget();
    browse = gtk_button_new_with_label("Browse ...");
    g_signal_connect_unlocked(browse,
                              "clicked",
                              G_CALLBACK(on_funmp3dir_browse_clicked),
                              (gpointer)entry);
    gtk_grid_attach(GTK_GRID(grid), label,  0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry,  1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), browse, 2, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}

#endif  /* MPG123 && HAVE_GLOB_H */

/** \brief   Handler for the 'clicked' event of the 'reset' buttons
 *
 * \param[in]   widget      button triggering the event
 * \param[in]   user_data   n/a
 */
static void on_wic64_reset_settings_clicked(GtkWidget *widget, gpointer p)
{
    userport_wic64_factory_reset();
    vice_gtk3_resource_entry_factory(wic64_server_save);
    vice_gtk3_resource_spin_int_factory(wic64_remote_timeout_save);
    vice_gtk3_resource_combo_int_sync(wic64_tz_save);
    /* FIXME: also update MAC, SecToken */
}

/** \brief   Handler for the 'clicked' event of the 'dhcp' checkbox
 *
 * \param[in]   widget      checkbox triggering the event
 * \param[in]   user_data   the textfield to enable/disable accordingly
 */
static void on_wic64_dhcp_clicked(GtkWidget *dhcp, gpointer p)
{
    GtkWidget *ip_addr = p;
    gboolean enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dhcp)) ^ 1;
    gtk_widget_set_sensitive(ip_addr, enabled);
}

/** \brief  Append WIC64 widgets to the main grid
 *
 * \param[in]   parent_grid main grid
 * \param[in]   parent_row  row in \a parent_grid to add widgets
 *
 * \return  row in \a parent_grid for additional widgets
 */
static int append_wic64_widgets(GtkWidget *parent_grid, int parent_row)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *server;
    GtkWidget *tz_widget;
    GtkWidget *tracing;
    GtkWidget *resetuser;
    GtkWidget *lines_widget;
    GtkWidget *trace_level;
    GtkWidget *mac_addr;
    GtkWidget *ip_addr;
    GtkWidget *sec_token;
    GtkWidget *dhcp;

    GtkWidget *reset;
    int        row = 0;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_widget_set_margin_top(grid, 32);

    label = label_helper("<b>WiC64 settings</b>");
    gtk_widget_set_margin_bottom(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 2, 1);
    row++;

    label = gtk_label_new("Remote Timeout\n(1 - 255)");
    gtk_widget_set_margin_start(label, 4);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    wic64_remote_timeout_save = vice_gtk3_resource_spin_int_new(
        "WIC64RemoteTimeout", 1, 255, 1);
    gtk_grid_attach(GTK_GRID(grid), wic64_remote_timeout_save, 1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label, 2, row, 1, 1);
    row++;

    label  = label_helper("Default server");
    wic64_server_save = server = vice_gtk3_resource_entry_new("WIC64DefaultServer");
    gtk_widget_set_hexpand(server, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label,  0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), server, 1, row, 1, 1);
    row++;

    label    = label_helper("MAC address");
    mac_addr = vice_gtk3_resource_entry_new("WIC64MACAddress");
    gtk_widget_set_hexpand(mac_addr, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label,    0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), mac_addr, 1, row, 1, 1);
    row++;

    label   = label_helper("IP address");
    ip_addr = vice_gtk3_resource_entry_new("WIC64IPAddress");
    dhcp = vice_gtk3_resource_check_button_new("WIC64DHCP", "DHCP");
    gtk_widget_set_hexpand(ip_addr, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label,   0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), ip_addr, 1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), dhcp,    2, row, 1, 1);
    row++;

    g_signal_connect(dhcp, "clicked", G_CALLBACK(on_wic64_dhcp_clicked), ip_addr);
    gtk_widget_set_sensitive(ip_addr, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dhcp)) ^ 1);

    label    = label_helper("Timezone");
    wic64_tz_save = tz_widget = create_wic64_timezone_combo();
    gtk_widget_set_hexpand(tz_widget, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label,     0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), tz_widget, 1, row, 1, 1);
    row++;

#if 1 /* keep it for now, as it was @compyx's fun to hack it ;-) */
    label     = label_helper("Security token");
    sec_token = vice_gtk3_resource_entry_new("WIC64SecToken");
    gtk_widget_set_hexpand(sec_token, TRUE);
    gtk_entry_set_input_purpose(GTK_ENTRY(sec_token), GTK_INPUT_PURPOSE_PASSWORD);
    gtk_entry_set_visibility(GTK_ENTRY(sec_token), FALSE);
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(sec_token),
                                      GTK_ENTRY_ICON_PRIMARY,
                                      "view-reveal-symbolic");
    gtk_entry_set_icon_sensitive(GTK_ENTRY(sec_token),
                                 GTK_ENTRY_ICON_PRIMARY,
                                 TRUE);
    g_signal_connect(G_OBJECT(sec_token),
                     "icon-press",
                     G_CALLBACK(on_sec_token_icon_press),
                     NULL);
    gtk_grid_attach(GTK_GRID(grid), label,     0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), sec_token, 1, row, 1, 1);
    row++;
#endif
    reset = gtk_button_new_with_label("Reset WiC64");
    g_signal_connect(reset,
                     "clicked",
                     G_CALLBACK(on_wic64_reset_settings_clicked),
                     0);
    gtk_grid_attach(GTK_GRID(grid), reset, 0, row, 1, 1);
    /* enable WiC64 tracing */
    resetuser = create_wic64_resetuser_widget();
    gtk_grid_attach(GTK_GRID(grid), resetuser, 1, row, 1, 1);
    row++;

    /* enable WiC64 tracing */
    tracing = create_wic64_logenabled_widget();
    gtk_grid_attach(GTK_GRID(grid), tracing,     0, row, 1, 1);

    label = gtk_label_new("Hexdump Lines\n(0: unlimited)");
    gtk_widget_set_margin_start(label, 4);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    lines_widget = vice_gtk3_resource_spin_int_new(
        "WIC64HexdumpLines", 0, 32768, 1);
    gtk_grid_attach(GTK_GRID(grid), lines_widget, 1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label, 2, row, 1, 1);
    row++;

    label = gtk_label_new("Log level\n(0: off)");
    gtk_widget_set_margin_start(label, 4);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    trace_level = vice_gtk3_resource_spin_int_new(
        "WIC64LogLevel", 0, WIC64_MAXTRACELEVEL, 1);
    gtk_grid_attach(GTK_GRID(grid), trace_level, 1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label, 2, row, 1, 1);
    row++;

    wic64_save = grid;
    gtk_grid_attach(GTK_GRID(parent_grid), grid, 0, parent_row, 2, 1);
    return parent_row + 1;
}
#endif


/** \brief  Create widget to select userport devices
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_userport_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *combo;
    int        device_id = 0;
    int        row = 0;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    /* combobox with the userport devices */
    label = label_helper("Userport device");
    combo = create_device_combobox();
    gtk_widget_set_hexpand(combo, TRUE);
    gtk_widget_set_margin_bottom(label, 16);
    gtk_widget_set_margin_bottom(combo, 16);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), combo, 1, row, 1, 1);
    row++;

    /* the RTC devices are not available for all emus */
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_VIC20:    /* fall through */
        case VICE_MACHINE_PET:      /* fall through */
        case VICE_MACHINE_CBM6x0:
            /* RTC 58321A save checkbox */
            rtc_58321a_save = create_rtc_58321a_save_widget();
            gtk_grid_attach(GTK_GRID(grid), rtc_58321a_save, 0, row, 2, 1);
            row++;

            /* RTC DS1307 save checkbox */
            rtc_ds1307_save = create_rtc_ds1307_save_widget();
            gtk_grid_attach(GTK_GRID(grid), rtc_ds1307_save, 0, row, 2, 1);
            row++;
            break;
        default:
            break;
    }

#if defined(USE_MPG123) && defined (HAVE_GLOB_H)
    /* FunMP3 widget for userport featured machine, except cbm */
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_VIC20:    /* fall through */
        case VICE_MACHINE_PET:      /* fall through */
            funmp3_save = create_funmp3dir_widget();
            gtk_grid_attach(GTK_GRID(grid), funmp3_save, 0, row, 2, 1);
            row++;
            break;
        default:
            break;
    }
#endif

    /* PET userport diagnostic pin */
    if (machine_class == VICE_MACHINE_PET) {
        diag_pin_active = create_diag_pin_active_widget();
        gtk_grid_attach(GTK_GRID(grid), diag_pin_active, 0, row, 2, 1);
        row++;
    }
#ifdef HAVE_LIBCURL
    if (machine_class == VICE_MACHINE_C64 ||
        machine_class == VICE_MACHINE_C64SC ||
        machine_class == VICE_MACHINE_C128 ||
        machine_class == VICE_MACHINE_VIC20 ||
        machine_class == VICE_MACHINE_SCPU64) {
        row = append_wic64_widgets(grid, row);
    }
#endif

    /* set the active item using the resource */
    if (resources_get_int("UserportDevice", &device_id) == 0) {
        set_device_id(GTK_COMBO_BOX(combo), device_id, TRUE);
    }

    gtk_widget_show_all(grid);
    return grid;
}
