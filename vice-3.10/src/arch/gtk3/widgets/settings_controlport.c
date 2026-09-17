/** \file   settings_controlport.c
 * \brief   Widget to control settings for control ports
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES JoyPort1Device      x64 x64sc x64dtv xscpu64 x128 xcbm5x0 xplus4 xvic
 * $VICERES JoyPort2Device      x64 x64sc x64dtv xscpu64 x128 xcbm5x0 xplus4
 * $VICERES JoyPort3Device      x64 x64sc x64dtv xscpu64 x128 xcbm2 xvic
 * $VICERES JoyPort4Device      x64 x64sc xscpu64 x128 xcbm2 xpet xvice
 * $VICERES JoyPort5Device      xplus4
 * $VICERES JoyPort6Device      x64 x64sc x64dtv xscpu64 x128 xcbm2 xcbm5x0 xvic
 * $VICERES JoyPort7Device      x64 x64sc x64dtv xscpu64 x128 xcbm2 xcbm5x0 xvic
 * $VICERES JoyPort8Device      x64 x64sc x64dtv xscpu64 x128 xcbm2 xcbm5x0 xvic
 * $VICERES JoyPort9Device      x64 x64sc x64dtv xscpu64 x128 xcbm2 xcbm5x0 xvic
 * $VICERES JoyPort10Device     x64 x64sc x64dtv xscpu64 x128 xcbm2 xcbm5x0 xvic
 * $VICERES JoyPort11Device     xplus4
 *
 * $VICERES BBRTCSave           -vsid
 * $VICERES ps2mouse            x64dtv
 * $VICERES SmartMouseRTCSave   x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0
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

#include "joyport.h"
#include "lib.h"
#include "machine.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "settings_controlport.h"


/*
 * Forward declarations
 */
static void update_adapter_ports_visibility(GtkGrid *grid, int row);


/** \brief  Resource combo box for control port #1 */
static GtkWidget *control_port_1_combo = NULL;

/** \brief  Resource combo box for control port #2 */
static GtkWidget *control_port_2_combo = NULL;


/** \brief  Handler for the 'changed' event of the control port combo boxes
 *
 * \param[in]   widget  control port widget's combo box
 * \param[in]   data    row in grid of the first adapter port widget
 */
static void on_control_port_changed(GtkWidget *widget, gpointer data)
{
    GtkWidget *grid;
    GtkWidget *other;
    int        row = GPOINTER_TO_INT(data);

    grid = gtk_widget_get_parent(gtk_widget_get_parent(widget));

    debug_gtk3("calling adapter port visibilty update with row %d.", row);
    update_adapter_ports_visibility(GTK_GRID(grid), row);

    debug_gtk3("synchronizing control port combo boxes with resources.");
    if (machine_class == VICE_MACHINE_VIC20) {
        return; /* VIC20 only has a single control port */
    }
    if (widget == control_port_1_combo) {
        debug_gtk3("syncing port 2");
        other = control_port_2_combo;
    } else {
        debug_gtk3("syncing port 1");
        other = control_port_1_combo;
    }

    /* temporarily block this signal handler so we don't trigger it again */
    g_signal_handlers_block_by_func(other, G_CALLBACK(on_control_port_changed), data);
    vice_gtk3_resource_combo_int_sync(other);
    g_signal_handlers_unblock_by_func(other, G_CALLBACK(on_control_port_changed), data);
}

/** \brief  Create combo box for joy port device
 *
 * \param[in]   port    index in devices, add 1 for JoyPort[n]Device
 *
 * \return  GtkComboBox
 */
static GtkWidget *create_joyport_combo(int port)
{
    GtkWidget      *combo;
    joyport_desc_t *devices;

    combo = vice_gtk3_resource_combo_int_new_sprintf("JoyPort%dDevice",
                                                     NULL, /* empty model */
                                                     port + 1);

    devices = joyport_get_valid_devices(port, TRUE);
    if (devices != NULL) {
        int row;
        int id = 0;

        resources_get_int_sprintf("JoyPort%dDevice", &id, port + 1);

        for (row = 0; devices[row].name != NULL; row++) {
            vice_gtk3_resource_combo_int_append(combo,
                                                devices[row].id,
                                                devices[row].name);
            if (devices[row].id == id) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(combo), row);
            }
        }
        lib_free(devices);
    }
    return combo;
}

/** \brief  Create combo box for joyport \a port
 *
 * \param[in]   port    Joyport number (0-4, 0 == JoyPort1Device)
 * \param[in]   title   widget title
 *
 * \return  GtkGrid
 */
static GtkWidget *create_joyport_widget(int port, const char *title)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *combo;
    char       buffer[64];

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    /* header */
    g_snprintf(buffer, sizeof buffer, "<b>%s</b>", title);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), buffer);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    /* combo box */
    combo = create_joyport_combo(port);
    gtk_widget_set_hexpand(combo, TRUE);
    gtk_grid_attach(GTK_GRID(grid), combo, 0, 1, 1, 1);

    gtk_widget_set_margin_bottom(grid, 16);
    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Add widgets for the control ports
 *
 * Adds \a count comboboxes to select the emulated device for the control ports.
 *
 * We don't use joyport_port_is_active() here due to the number of control ports
 * for a specific machine being fixed.
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     starting row in \a layout
 * \param[in]       count   number of widgets to add to \a layout
 *
 * \return  row in the \a layout for additional widgets
 */
static int layout_add_control_ports(GtkGrid *layout, int row, int count)
{
    GtkWidget *widget;

    if (count < 1) {
        return row;
    }

    widget = create_joyport_widget(JOYPORT_1, "Control Port #1");
    gtk_grid_attach(layout, widget, 0, row, 1, 1);
    control_port_1_combo = gtk_grid_get_child_at(GTK_GRID(widget), 0, 1);
    g_signal_connect(control_port_1_combo,
                     "changed",
                     G_CALLBACK(on_control_port_changed),
                     GINT_TO_POINTER(row + 1));
    gtk_widget_show(widget);

    if (count > 1) {
        widget = create_joyport_widget(JOYPORT_2, "Control Port #2"),
        gtk_grid_attach(layout, widget, 1, row, 1, 1);
        control_port_2_combo = gtk_grid_get_child_at(GTK_GRID(widget), 0, 1);
        g_signal_connect(control_port_2_combo,
                         "changed",
                         G_CALLBACK(on_control_port_changed),
                         GINT_TO_POINTER(row + 1));
        gtk_widget_show(widget);
    }

    return row + 1;
}

/** \brief  Add widgets for the joystick adapter ports
 *
 * Adds \a count widgets to select the emulated device for the adapter ports.
 *
 * Only the widgets for active adapter ports (according to current configuration)
 * will be shown. The function update_adapter_ports_visibility() is used when
 * the control port devices change to update the adapter port widgets' visibility
 * due to a changed configuration.
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     starting row in \a layout
 * \param[in]       count   number of widgets to add to \a layout
 *
 * \return  row in the \a layout for additional widgets
 */
static int layout_add_adapter_ports(GtkGrid *layout, int row, int count)
{
    int i;
    int r = row;
    int c = 0;
    int d = JOYPORT_3;

    for (i = 0; i < count; i++, d++) {
        GtkWidget *widget;
        char       label[256];

        g_snprintf(label, sizeof label, "Extra Joystick #%d", i + 1);
        widget = create_joyport_widget(d, label);
        gtk_grid_attach(layout, widget, c, r, 1, 1);
        if (joyport_port_is_active(d)) {
            gtk_widget_show(widget);
        } else {
            gtk_widget_hide(widget);
        }

        c ^= 1; /* swap column */
        if (c == 0) {
            r++;
        }
    }

    return r + 1 + c;
}

/** \brief  Update visibility of adapter port widgets
 *
 * Show/hide adapter port widgets based on current configuration.
 *
 * \param[in]   grid    Grid containing all the widgets
 * \param[in]   row     row in \a grid of the first adapter port widget
 */
static void update_adapter_ports_visibility(GtkGrid *grid, int row)
{
    int r;

    for (r = 0; r < 4; r++) {
        GtkWidget *widget;

        debug_gtk3("Updating Joydevice%d visibility.", r * 2 + 1);
        widget = gtk_grid_get_child_at(grid, 0, row + r);
        if (widget != NULL) {
            if (joyport_port_is_active(r * 2 + 2)) {
                gtk_widget_show(widget);
            } else {
                gtk_widget_hide(widget);
            }
        }
        debug_gtk3("Updating Joydevice%d visibility.", r * 2 + 2);
        widget = gtk_grid_get_child_at(grid, 1, row + r);
        if (widget != NULL) {
            if (joyport_port_is_active(r * 2 + 3)) {
                gtk_widget_show(widget);
            } else {
                gtk_widget_hide(widget);
            }
        }
    }
}

/** \brief  Add widget for the Plus4 SIDCart joystick port
 *
 * Adds a widget for the Plus4-specific SIDCard extra joystick port.
 *
 * Currently the resource "JoyPort5Device" is used for this widget, but once
 * eight adapter ports are implemented for Plus4 like the other emulators, we
 * probably should switch the widget to "JoyPort11Device".
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     starting row in \a layout
 *
 * \return  row in the \a layout for additional widgets
 */
static int layout_add_sidcard_port(GtkGrid *layout, int row)
{
    GtkWidget *widget = create_joyport_widget(JOYPORT_PLUS4_SIDCART, "SIDCard Joystick Port");

    gtk_grid_attach(layout, widget, 0, row, 1, 1);
    if (joyport_port_is_active(JOYPORT_PLUS4_SIDCART)) {
        gtk_widget_show(widget);
    } else {
        gtk_widget_hide(widget);
    }
    return row + 1;
}

/** \brief  Add checkbox for the battery-backed RTC save option
 *
 * Add a checkbox for the "BBRTCSave" resource.
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     row in \a layout to add the checkbox
 *
 * \return  row in the \a layout for additional widgets
 *
 * \note    the added widget spans two columns in the layout
 */
static int layout_add_bbrtc_widget(GtkGrid *layout, int row)
{
    GtkWidget *check;

    check = vice_gtk3_resource_check_button_new("BBRTCSave",
            "Save battery-backed real time clock data when changed");
    gtk_grid_attach(layout, check, 0, row, 2, 1);
    gtk_widget_show(check);

    return row + 1;
}

/** \brief  Add checkbox for the SmartMouse RTC save option
 *
 * Add a checkbox for the "SmartMouseRTCSave" resource.
 *
 * Valid for x64, x64sc, xscpu64, x128, xcbm5x0 and xvic.
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     row in \a layout to add the checkbox
 *
 * \return  row in the \a layout for additional widgets
 *
 * \note    the added widget spans two columns in the layout
 */
static int layout_add_smartmouse_rtc_widget(GtkGrid *layout, int row)
{
    GtkWidget *check;

    check = vice_gtk3_resource_check_button_new("SmartMouseRTCSave",
                                                "Enable SmartMouse RTC Saving");
    gtk_grid_attach(layout, check, 0, row, 2, 1);
    gtk_widget_show(check);

    return row + 1;
}

/** \brief  Add checkbox for the userport PS/2 mouse
 *
 * Add a checkbox for the "ps2mouse" resource.
 *
 * Valid for x64dtv.
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     row in \a layout to add the checkbox
 *
 * \return  row in the \a layout for additional widgets
 *
 * \note    the added widget spans two columns in the layout
 */
static int layout_add_ps2mouse_widget(GtkGrid *layout, int row)
{
    GtkWidget *check;

    check = vice_gtk3_resource_check_button_new("ps2mouse",
                                                "Enable PS/2 mouse on Userport");
    gtk_grid_attach(layout, check, 0, row, 2, 1);
    gtk_widget_show(check);

    return row + 1;
}


/*
 * Functions to create the layouts for the various emulators
 */

/** \brief  Create layout for x64, x64sc, xscpu64 and x128
 *
 * Two control ports and eight joystick adapter ports.
 *
 * \param[in,out]   layout  main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_c64_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_control_ports(layout, row, 2);
    row = layout_add_adapter_ports(layout, row, 8);
    row = layout_add_bbrtc_widget(layout, row);
    row = layout_add_smartmouse_rtc_widget(layout, row);

    return row;
}

/** \brief  Create layout for x64dtv
 *
 * Two control ports and eight joystick adapter ports.
 *
 * \param[in,out]   layout  main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_c64dtv_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_control_ports(layout, row, 2);
    row = layout_add_adapter_ports(layout, row, 8);
    row = layout_add_bbrtc_widget(layout, row);
    row = layout_add_ps2mouse_widget(layout, row);

    return row;
}

/** \brief  Create layout for xvic
 *
 * One control port and eight joystick adapter ports.
 *
 * \param[in,out]   layout    main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_vic20_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_control_ports(layout, row, 1);
    row = layout_add_adapter_ports(layout, row, 8);
    row = layout_add_bbrtc_widget(layout, row);
    row = layout_add_smartmouse_rtc_widget(layout, row);

    return row;
}

/** \brief  Create layout for xplus4
 *
 * Two control ports, two userport adapter ports and one SIDCard control port.
 *
 * \param[in,out]   layout  main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_plus4_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_control_ports(layout, row, 2);
    row = layout_add_adapter_ports(layout, row, 8);
    row = layout_add_sidcard_port(layout, row);
    row = layout_add_bbrtc_widget(layout, row);

    return row;
}

/** \brief  Create layout for xpet
 *
 * Two userport adapter ports.
 *
 * \param[in,out]   layout  main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_pet_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_adapter_ports(layout, row, 3);
    row = layout_add_bbrtc_widget(layout, row);

    return row;
}

/** \brief  Create layout for xcbm5x0
 *
 * Two control ports and eight joystick adapter ports.
 *
 * \param[in,out]   layout  main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_cbm5x0_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_control_ports(layout, row, 2);
    row = layout_add_adapter_ports(layout, row, 8);
    row = layout_add_bbrtc_widget(layout, row);
    row = layout_add_smartmouse_rtc_widget(layout, row);

    return row;
}

/** \brief  Create layout for xcbm2
 *
 * Eight joystick adapter ports.
 *
 * \param[in,out]   layout  main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_cbm6x0_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_adapter_ports(layout, row, 8);
    row = layout_add_bbrtc_widget(layout, row);

    return row;
}


/** \brief  Create widget to control control ports
 *
 * Creates a widget to control the settings for the control ports, userport
 * joystick adapter ports and the SIDCard control port, depending on the
 * currently emulated machine.
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_controlport_widget_create(GtkWidget *parent)
{
    GtkWidget *layout;

    layout = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(layout), 8);
    gtk_widget_set_no_show_all(layout, TRUE);

    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
            create_c64_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_C64DTV:
            create_c64dtv_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_VIC20:
            create_vic20_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_PLUS4:
            create_plus4_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_PET:
            create_pet_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_CBM5x0:
            create_cbm5x0_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_CBM6x0:
            create_cbm6x0_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_VSID: /* fallthrough */
        default:
            debug_gtk3("Warning: should never get here!");
            break;
    }

    gtk_widget_show(layout);
    return layout;
}
