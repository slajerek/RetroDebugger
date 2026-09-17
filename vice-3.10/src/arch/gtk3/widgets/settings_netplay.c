/** \file   settings_netplay.c
 * \brief   Settings widget for Netplay
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES NetworkServerName           -vsid
 * $VICERES NetworkServerPort           -vsid
 * $VICERES NetworkServerBindAddress    -vsid
 * $VICERES NetworkControl              -vsid
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
#include <errno.h>

#include "debug_gtk3.h"
#include "lib.h"
#include "log.h"
#include "network.h"
#include "resources.h"
#include "ui.h"
#include "vice_gtk3.h"

#include "settings_netplay.h"


/** \brief  Net control widget info
 */
typedef struct netctrl_s {
    const char *    text;  /**< label */
    unsigned int    mask;  /**< bit to toggle */
} netctrl_t;


/** \brief  List of controls
 */
static const netctrl_t control_list[] = {
    { "Keyboard",       NETWORK_CONTROL_KEYB },
    { "Joystick #1",    NETWORK_CONTROL_JOY1 },
    { "Joystick #2",    NETWORK_CONTROL_JOY2 },
    { "Devices",        NETWORK_CONTROL_DEVC },
    { "Resources",      NETWORK_CONTROL_RSRC },
    { NULL,             0 }
};


/** \brief  Column headers for the NetworkControl resource
 */
static const char *netctrl_headers[] = { "", "<b>Server</b>", "<b>Client</b>" };


/** \brief  Modes for the netplay status display
 */
static const char *net_modes[] = {
    "Idle",             /* NETWORK_IDLE */
    "Server",           /* NETWORK_SERVER */
    "Server connected", /* NETWORK_SERVER_CONNECTED */
    "Client connected"  /* NETWORK_CLIENT */
};


/* Widget references for the glue logic */

/** \brief  Server address widget */
static GtkWidget *server_address = NULL;

/** \brief  Client address widget */
static GtkWidget *client_address = NULL;

/** \brief  Client and server port number */
static GtkWidget *port_number = NULL;

/** \brief  Netplay status widget */
static GtkWidget *netplay_status = NULL;

/** \brief  Network mode combo box */
static GtkWidget *combo_netplay = NULL;

/** \brief  Client enable widget */
static GtkWidget *netplay_enable = NULL;

/** \brief  Signal handler ID of the enable switch */
static gulong netplay_handler = 0;

/** \brief  last used setting was client or server? */
static int netplay_mode = -1;


/** \brief  Update display of the netplay status
 */
static void netplay_update_status(void)
{
    const char *text = NULL;
    char        temp[256];
    int         mode;
    gboolean    server;

    server = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_netplay)) == 0;
    mode   = network_get_mode();
    if (mode < 0 || mode >= G_N_ELEMENTS(net_modes)) {
        text = "invalid";
    } else {
        text = net_modes[mode];
    }
    debug_gtk3("mode = %d ('%s')", mode, text);

    g_snprintf(temp, sizeof temp, "<b>%s</b>", text);
    gtk_label_set_markup(GTK_LABEL(netplay_status), temp);

    /* mode combobox cant be changed when network is active */
    gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(combo_netplay),
        (mode == NETWORK_IDLE) ? GTK_SENSITIVITY_ON : GTK_SENSITIVITY_OFF);
    /* port cant be changed when network is active */
    gtk_widget_set_sensitive(port_number,
        (mode == NETWORK_IDLE) ? TRUE : FALSE);
    /* server address can only be changed when server is selected, and we are idle */
    gtk_widget_set_sensitive(server_address,
        ((mode == NETWORK_IDLE) && server) ? TRUE : FALSE);
    /* client address can only be changed when client is selected, and we are idle */
    gtk_widget_set_sensitive(client_address,
        ((mode == NETWORK_IDLE) && !server) ? TRUE : FALSE);
}

/** \brief  Handler for the 'toggled' event of the NetworkControl checkboxes
 *
 * Basically EOR's the resource NetworkControl with \c data
 *
 * \param[in,out]   widget  checkbox
 * \param[in]       data    bitmask
 */
static void on_server_mask_toggled(GtkWidget *widget, gpointer data)
{
    int value;
    unsigned int newval;
    unsigned int mask;

    resources_get_int("NetworkControl", &value);
    mask = GPOINTER_TO_UINT(data);

    /* do the actual work: flip a bit */
    newval = value ^ mask;
#if 1
    debug_gtk3("State: %s, Mask: %02x, Value: %02x",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))
            ? "Enabled" : "Disabled",
            mask,
            newval);
#endif
    if ((unsigned int)value != newval) {
        resources_set_int("NetworkControl", (int)newval);
    }
}

/** \brief  Handler for the 'notify::active' event of the client enable switch
 *
 * \param[in,out]   widget  client enable switch
 * \param[in]       pspec   GObject parameter specification (unused)
 * \param[in]       data    extra event data (unused)
 */
static void on_netplay_notify_active(GtkSwitch  *widget,
                                     GParamSpec *pspec,
                                     gpointer    data)
{
    gboolean active = gtk_switch_get_active(widget);
    int      role   = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_netplay));
    int      mode   = network_get_mode();

    debug_gtk3("active = %s, role = %s, mode = %s",
               active ? "TRUE" : "FALSE",
               role == 0 ? "Server" : "Client",
               mode >= 0 && mode < G_N_ELEMENTS(net_modes) ? net_modes[mode] : "(invalid)");

    /* disconnect when not idle */
    if (mode != NETWORK_IDLE) {
        network_disconnect();
    }

    if (active) {
        bool failed = false;
        if (role == 0) {
            /* attempt to start server */
            if (network_start_server() < 0) {
                log_error(LOG_DEFAULT, "Failed to start netplay server.");
                failed = true;
           }
        } else {
            /* start the client */
            if (network_connect_client() < 0) {
                log_error(LOG_DEFAULT, "Failed to start client.");
                failed = true;
            }
        }
        if (failed) {
            g_signal_handler_block(widget, netplay_handler);
            gtk_switch_set_active(widget, FALSE);
            g_signal_handler_unblock(widget, netplay_handler);
        }
    }
    netplay_update_status();
}

/** \brief  Handler for the "changed" event of the network mode combo box
 *
 * \param[in]   combo       combo box
 * \param[in]   user_data   extra event data (unused)
 */
static void on_combo_changed(GtkComboBox *combo, gpointer user_data)
{
    int server = (gtk_combo_box_get_active(combo) == 0);
    int mode = network_get_mode();

    netplay_mode = gtk_combo_box_get_active(combo);

    /* server address can only be changed when server is selected, and we are idle */
    gtk_widget_set_sensitive(server_address,
        ((mode == NETWORK_IDLE) && server) ? TRUE : FALSE);
    /* client address can only be changed when client is selected, and we are idle */
    gtk_widget_set_sensitive(client_address,
        ((mode == NETWORK_IDLE) && !server) ? TRUE : FALSE);
}


/** \brief  Create left-aligned label using Pango markup
 *
 * \param[in]   text    label text, can contain Pango markup
 *
 * \return  GtkLabel
 */
static GtkWidget *label_helper(const char *text)
{
    GtkWidget *label;

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}

/** \brief  Create combo box with network modes
 *
 * \return  GtkComboBoxText
 */
static GtkWidget *create_combo_box(void)
{
    GtkWidget *combo;
    int mode = network_get_mode();

    combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo),
            NULL, "This emulator is the server");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo),
            NULL, "This emulator is the client");

    if (netplay_mode < 0) {
        netplay_mode = (mode == NETWORK_CLIENT ? 1 : 0);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), netplay_mode);

    g_signal_connect(combo, "changed", G_CALLBACK(on_combo_changed), NULL);
    return combo;
}

/** \brief  Create netplay switch
 *
 * \return  GtkSwitch
 */
static GtkWidget *create_netplay_enable_widget(void)
{
    GtkWidget *widget;
    int mode;

    mode = network_get_mode();
    widget = gtk_switch_new();

    gtk_widget_set_halign(widget, GTK_ALIGN_END);
    gtk_widget_set_valign(widget, GTK_ALIGN_CENTER);
    gtk_switch_set_active(GTK_SWITCH(widget), mode != NETWORK_IDLE);
    gtk_widget_set_hexpand(widget, FALSE);
    gtk_widget_set_vexpand(widget, FALSE);
    netplay_handler = g_signal_connect(widget,
                                       "notify::active",
                                       G_CALLBACK(on_netplay_notify_active),
                                       NULL);
    return widget;
}

/** \brief  Add controls checkboxes
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to start adding widgets
 * \param[in]   columns number of columns in \a grid, for proper column spans
 *
 * \return  next row in \a grid to add widgets
 */
static int create_controls_layout(GtkWidget *grid, int row, int columns)
{
    GtkWidget *label;
    int        i;
    int        status = 0;

    resources_get_int("NetworkControl", &status);

    /* header */
    label = label_helper("<b>Controls</b>");
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, columns, 1);
    gtk_widget_set_margin_top(label, 16);
    row++;

    /* column headers */
    for (i = 0; i < G_N_ELEMENTS(netctrl_headers); i++) {
        label = label_helper(netctrl_headers[i]);
        gtk_widget_set_margin_bottom(label, 8);
        gtk_widget_set_hexpand(label, FALSE);
        gtk_grid_attach(GTK_GRID(grid), label, i, row, 1, 1);
    }
    row++;

    for (i = 0; control_list[i].text != NULL; i++) {

        GtkWidget *check;
        netctrl_t data = control_list[i];

        /* name of the thing */
        label = gtk_label_new(data.text);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

        /* checkbutton to toggle the server side */
        check = gtk_check_button_new();
        gtk_widget_set_halign(check, GTK_ALIGN_START);
        gtk_widget_set_hexpand(check, FALSE);
        /* FIXME: there has to be a better way to properly center the check
         *        buttons and the headers without taking up so much space in
         *        the main grid. */
        gtk_widget_set_margin_start(check, 12);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                     status & data.mask);
        g_signal_connect(G_OBJECT(check),
                         "toggled",
                         G_CALLBACK(on_server_mask_toggled),
                         GINT_TO_POINTER(data.mask));
        gtk_grid_attach(GTK_GRID(grid), check, 1, row, 1, 1);

        /* checkbutton to toggle the client side */
        check = gtk_check_button_new();
        gtk_widget_set_halign(check, GTK_ALIGN_START);
        gtk_widget_set_hexpand(check, FALSE);
        gtk_widget_set_margin_start(check, 12);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                     (status >> 8) & data.mask);
        g_signal_connect(G_OBJECT(check),
                         "toggled",
                         G_CALLBACK(on_server_mask_toggled),
                         GINT_TO_POINTER(data.mask << 8));

        gtk_grid_attach(GTK_GRID(grid), check, 2, row, 1, 1);

        row++;
    }

    return row;
}


/** \brief  Create Netplay settings widget
 *
 * \param[in]   parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_netplay_widget_create(GtkWidget *parent)
{
#define NUM_COLS 5
    GtkWidget *grid;
    GtkWidget *label;
    int        row = 0;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    /* network settings header */
    label = label_helper("<b>Network settings</b>");
    gtk_widget_set_margin_bottom(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, NUM_COLS, 1);
    row++;

    /* client or server? */
    label          = label_helper("Network mode");
    combo_netplay  = create_combo_box();
    netplay_enable = create_netplay_enable_widget();
    gtk_widget_set_hexpand(combo_netplay, TRUE);
    /* gtk_widget_set_halign(combo_netplay, GTK_ALIGN_START); */
    gtk_grid_attach(GTK_GRID(grid), label,          0, row, 1,            1);
    gtk_grid_attach(GTK_GRID(grid), combo_netplay,  1, row, NUM_COLS - 2, 1);
    gtk_grid_attach(GTK_GRID(grid), netplay_enable, 4, row, 1,            1);
    row++;

    /* Server widgets */

    /* label */
    label = label_helper("Server address");
    /* address */
    server_address = vice_gtk3_resource_entry_new("NetworkServerBindAddress");
    gtk_widget_set_hexpand(server_address, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label,          0, row, 1,            1);
    gtk_grid_attach(GTK_GRID(grid), server_address, 1, row, NUM_COLS - 1, 1);
    row++;

    /* Client widgets */

    /* label */
    label = label_helper("Remote server");
    /* address */
    client_address = vice_gtk3_resource_entry_new("NetworkServerName");
    gtk_widget_set_hexpand(client_address, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label,          0, row, 1,            1);
    gtk_grid_attach(GTK_GRID(grid), client_address, 1, row, NUM_COLS - 1, 1);
    row++;

    /* Port widgets */

    /* label */
    label = label_helper("Port");
    /* port */
    port_number = vice_gtk3_resource_spin_int_new("NetworkServerPort",
                                                  1, 65535, 1);
    gtk_widget_set_hexpand(port_number, FALSE);
    gtk_widget_set_halign(port_number, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label,       0, row, 1,            1);
    gtk_grid_attach(GTK_GRID(grid), port_number, 1, row, NUM_COLS - 1, 1);
    row++;

    /* Network status widgets */

    /* label */
    label = label_helper("Network status");
    /* status widget */
    netplay_status = gtk_label_new(NULL);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_top(netplay_status, 8);
    gtk_widget_set_halign(netplay_status, GTK_ALIGN_START);
    gtk_widget_set_hexpand(netplay_status, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label,          0, row, 1,            1);
    gtk_grid_attach(GTK_GRID(grid), netplay_status, 1, row, NUM_COLS - 1, 1);
    row++;
    /* update status text */
    netplay_update_status();

    row = create_controls_layout(grid, row, NUM_COLS);
#undef NUM_COLS
    gtk_widget_show_all(grid);
    return grid;
}
