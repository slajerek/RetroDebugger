/** \file   settings_rs232.c
 * \brief   Widget to control various RS232 resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES Acia1Enable     x64 x64sc xscpu64 x128 xvic
 * $VICERES Acia1Dev        x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0 xcbm2
 * $VICERES Acia1Base       x64 x64sc xscpu64 x128 xvic
 * $VICERES Acia1Irq        x64 x64sc xscpu64 x128 xvic
 * $VICERES Acia1Mode       x64 x64sc xscpu64 x128 xvic
 * $VICERES UserportDevice  x64 x64sc xscpu64 x128 xvic
 * $VICERES RsUserBaud      x64 x64sc xscpu64 x128 xvic
 * $VICERES RsUserDev       x64 x64sc xscpu64 x128 xvic
 * $VICERES RsUserUP9600    x64 x64sc xscpu64 x128 xvic
 * $VICERES RsUserRTSInv    x64 x64sc xscpu64 x128 xvic
 * $VICERES RsUserCTSInv    x64 x64sc xscpu64 x128 xvic
 * $VICERES RsUserDSRInv    x64 x64sc xscpu64 x128 xvic
 * $VICERES RsUserDCDInv    x64 x64sc xscpu64 x128 xvic
 * $VICERES RsDevice1       x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0 xcbm2
 * $VICERES RsDevice2       x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0 xcbm2
 * $VICERES RsDevice3       x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0 xcbm2
 * $VICERES RsDevice4       x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0 xcbm2
 * $VICERES RsDevice1ip232  x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0 xcbm2
 * $VICERES RsDevice2ip232  x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0 xcbm2
 * $VICERES RsDevice3ip232  x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0 xcbm2
 * $VICERES RsDevice4ip232  x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0 xcbm2
 * $VICERES RsDevice1Baud   x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0 xcbm2
 * $VICERES RsDevice2Baud   x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0 xcbm2
 * $VICERES RsDevice3Baud   x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0 xcbm2
 * $VICERES RsDevice4Baud   x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0 xcbm2
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

#include "vice_gtk3.h"
#include "lib.h"
#include "machine.h"
#include "resources.h"
#include "userport.h"
#include "userportdevicecheckbutton.h"
#include "userportrsinterfacewidget.h"
#include "settings_rs232.h"

#include "rsuser.h"
#include "acia.h"

/** \brief  List of ACIA devices
 *
 * \note    Also used by the Userport RS232 Device list combo box
 */
static const vice_gtk3_combo_entry_int_t acia_devices[] = {
    { "Serial 1",  ACIA_DEVICE_1 },
    { "Serial 2",  ACIA_DEVICE_2 },
    { "Serial 3",  ACIA_DEVICE_3 },
    { "Serial 4",  ACIA_DEVICE_4 },
    { NULL,       -1 }
};

/** \brief  List of base addresses for the emulated chip (x64/x64sc/xscpu64)
 */
static const vice_gtk3_radiogroup_entry_t acia_base_c64[] = {
    { "$DE00", 0xde00 },
    { "$DF00", 0xdf00 },
    { NULL,        -1 }
};

/** \brief  List of base addresses for the emulated chip (x128)
 */
static const vice_gtk3_radiogroup_entry_t acia_base_c128[] = {
    { "$D700", 0xd700 },
    { "$DE00", 0xde00 },
    { "$DF00", 0xdf00 },
    { NULL,        -1 }
};

/** \brief  List of base addresses for the emulated chip (xvic)
 */
static const vice_gtk3_radiogroup_entry_t acia_base_vic20[] = {
    { "$9800", 0x9800 },
    { "$9C00", 0x9c00 },
    { NULL,        -1 }
};

/** \brief  List of ACIA IRQ sources (x64/x64sc/xscpu64/x128/xvic)
 */
static const vice_gtk3_radiogroup_entry_t acia_irqs[] = {
    { "None",  ACIA_INT_NONE },
    { "NMI",   ACIA_INT_NMI },
    { "IRQ",   ACIA_INT_IRQ },
    { NULL,   -1 }
};

/** \brief  List of emulated RS232 modes (x64/x64sc/xscpu64/x128/xvic)
 */
static const vice_gtk3_radiogroup_entry_t acia_modes[] = {
    { "Normal",     ACIA_MODE_NORMAL },
    { "Swiftlink",  ACIA_MODE_SWIFTLINK },
    { "Turbo232",   ACIA_MODE_TURBO232 },
    { NULL,        -1 }
};

/** \brief  List of user-port RS232 baud rates (x64/x64sc/xscpu64/x128/xvic)
 */
static const vice_gtk3_combo_entry_int_t rsuser_baud_rates[] = {
    { "300",     300 },
    { "600",     600 },
    { "1200",   1200 },
    { "2400",   2400 },
    { "4800",   4800 },
    { "9600",   9600 },
    { "38400", 38400 },
    { "57600", 57600 },
    { NULL,       -1 }
};

/** \brief  List of baud rates for C64/C128/VIC-20 serial devices
 */
static const vice_gtk3_combo_entry_int_t serial_baud_rates_c64[] = {
    { "300",       300 },
    { "1200",     1200 },
    { "2400",     2400 },
    { "9600",     9600 },
    { "19200",   19200 },
    { "38400",   38400 },
    { "57600",   57600 },
    { "115200", 115200 },
    { NULL,         -1 }
};

/** \brief  List of baud rates for Plus4/CBM-II
 */
static const vice_gtk3_combo_entry_int_t serial_baud_rates_plus4[] = {
    { "300",     300 },
    { "1200",   1200 },
    { "2400",   2400 },
    { "9600",   9600 },
    { "19200", 19200 },
    { NULL,       -1 }
};

/** \brief  Userport baud rate widget */
static GtkWidget *rsuser_baud_widget;

/** \brief  Determine if the current machine supports ACIA
 *
 * \return  `true` if ACIA either native or through expansions */
static bool machine_has_acia(void)
{
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_VIC20:
            return true;
        default:
            return false;
    }
}

/** \brief  Extra event handler for the userport RS-232 interface type changes
 *
 * \param[in]   widget      Userport RS-232 interface type combobox
 */
static void on_rsinterface_changed(GtkWidget *widget)
{
    GtkWidget *baud;
    GtkWidget *rts;
    GtkWidget *cts;
    GtkWidget *dcd;
    GtkWidget *dsr;
    GtkWidget *dtr;
    GtkWidget *grid;
    int        up9600 = 0;
    int        index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));


    resources_get_int("RsUserUP9600", &up9600);
    baud = rsuser_baud_widget;
    grid = gtk_widget_get_parent(widget);
    rts = gtk_grid_get_child_at(GTK_GRID(grid), 1, 2);
    cts = gtk_grid_get_child_at(GTK_GRID(grid), 2, 2);
    dsr = gtk_grid_get_child_at(GTK_GRID(grid), 1, 3);
    dcd = gtk_grid_get_child_at(GTK_GRID(grid), 2, 3);
    dtr = gtk_grid_get_child_at(GTK_GRID(grid), 1, 4);

    if (rts != NULL && cts != NULL && dcd != NULL && dsr != NULL && dtr != NULL) {
        switch (index) {
            case USERPORT_RS_NONINVERTED:
                vice_gtk3_resource_check_button_set(rts, FALSE);
                vice_gtk3_resource_check_button_set(cts, FALSE);
                /*vice_gtk3_resource_check_button_set(dcd, FALSE);*/
                vice_gtk3_resource_check_button_set(dsr, FALSE);
                vice_gtk3_resource_check_button_set(dtr, FALSE);
                resources_set_int("RsUserUP9600", 0);
                break;
            case USERPORT_RS_INVERTED:
                vice_gtk3_resource_check_button_set(rts, TRUE);
                vice_gtk3_resource_check_button_set(cts, TRUE);
                /*vice_gtk3_resource_check_button_set(dcd, TRUE);*/
                vice_gtk3_resource_check_button_set(dsr, TRUE);
                vice_gtk3_resource_check_button_set(dtr, TRUE);
                resources_set_int("RsUserUP9600", 0);
                break;
            case USERPORT_RS_UP9600:
                if (baud != NULL) {
                    vice_gtk3_resource_combo_int_set(baud, 9600);
                }
                resources_set_int("RsUserUP9600", 1);
                break;
            default:
                resources_set_int("RsUserUP9600", 0);
                break;
        }
    }
}


/** \brief  Helper: create left-aligned, 16 px indented label
 *
 * \param[in]   text    label text
 *
 * \return  GtkLabel
 */
static GtkWidget *create_indented_label(const char *text)
{
    GtkWidget *label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 8);
    return label;
}

/* {{{ ACIA widgets */
/** \brief  Create a check button to enable/disable ACIA emulation
 *
 * \return  GtkCheckButton or `NULL` when the current machine doesn't support
 *          toggling emulation
 */
static GtkWidget *create_acia_enable_widget(void)
{
    GtkWidget *widget = NULL;

    if (machine_has_acia()) {
        widget = vice_gtk3_resource_check_button_new("Acia1Enable",
                "Enable ACIA RS232 interface emulation");
    }
    return widget;
}

/** \brief  Create a group of radio buttons to set the ACIA chip address
 *
 * \return  GtkGrid with radio buttons or `NULL` when the current machine
 *          doesn't support selecting a base address
 */
static GtkWidget *create_acia_base_widget(void)
{
    GtkWidget *widget = NULL;

    if (machine_has_acia()) {
        const vice_gtk3_radiogroup_entry_t *entries;

        if (machine_class == VICE_MACHINE_VIC20) {
            entries = acia_base_vic20;
        } else if (machine_class == VICE_MACHINE_C128) {
            entries = acia_base_c128;
        } else {
            entries = acia_base_c64;
        }
        widget = vice_gtk3_resource_radiogroup_new("Acia1Base",
                                                   entries,
                                                   GTK_ORIENTATION_HORIZONTAL);
        gtk_grid_set_column_homogeneous(GTK_GRID(widget), TRUE);
    }
    return widget;
}

/** \brief  Create a group of radio buttons to set the ACIA IRQ source
 *
 * \return  GtkGrid with radio buttons or `NULL` when the current machine
 *          doesn't support selecting an IRQ
 */
static GtkWidget *create_acia_irq_widget(void)
{
    GtkWidget *widget = NULL;

    if (machine_has_acia()) {
        widget = vice_gtk3_resource_radiogroup_new("Acia1Irq",
                                                   acia_irqs,
                                                   GTK_ORIENTATION_HORIZONTAL);
        gtk_grid_set_column_homogeneous(GTK_GRID(widget), TRUE);
    }
    return widget;
}

/** \brief  Create a group of radio buttons to set the RS232 mode
 *
 * \return  GtkGrid with radio buttons or `NULL` when the current machine
 *          doesn't support selecting an interface
 */
static GtkWidget *create_acia_mode_widget(void)
{
    GtkWidget *widget = NULL;

    if (machine_has_acia()) {
        widget = vice_gtk3_resource_radiogroup_new("Acia1Mode",
                                                   acia_modes,
                                                   GTK_ORIENTATION_HORIZONTAL);
        gtk_grid_set_column_homogeneous(GTK_GRID(widget), TRUE);
    }
    return widget;
}

/** \brief  Create RS232 ACIA settings widget
 *
 * \return  GtkGrid
 */
static GtkWidget *create_acia_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *acia_enable_widget;
    GtkWidget *acia_device_widget;
    GtkWidget *acia_base_widget;
    GtkWidget *acia_irq_widget;
    GtkWidget *acia_mode_widget;
    int        row = 1;    /* header label is always present */

    grid = vice_gtk3_grid_new_spaced_with_label(8, 8, "ACIA settings", 2);

    /* widgets */
    acia_enable_widget = create_acia_enable_widget();
    acia_device_widget = vice_gtk3_resource_combo_int_new("Acia1Dev",
                                                              acia_devices);
    acia_base_widget   = create_acia_base_widget();
    acia_irq_widget    = create_acia_irq_widget();
    acia_mode_widget   = create_acia_mode_widget();

    /* layout */
    if (acia_enable_widget != NULL) {
        gtk_widget_set_margin_start(acia_enable_widget, 8);
        gtk_grid_attach(GTK_GRID(grid), acia_enable_widget, 0, row, 2, 1);
        row++;
    }
    label = create_indented_label("Device");
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), acia_device_widget, 1, row, 1, 1);
    row++;
    if (acia_base_widget != NULL) {
        label = create_indented_label("Base address");
        gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), acia_base_widget, 1, row, 1, 1);
        row++;
    }
    if (acia_irq_widget != NULL) {
        label = create_indented_label("IRQ");
        gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), acia_irq_widget, 1, row, 1, 1);
        row++;
    }
    if (acia_mode_widget != NULL) {
        label = create_indented_label("Emulation mode");
        gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), acia_mode_widget, 1, row, 1, 1);
        row++;
    }

    gtk_widget_show_all(grid);
    return grid;
}
/* }}} */

/** \brief  Create combo box with baud rates for serial devices
 *
 * \param[in]   resource    resource that contains the baud rate
 *
 * \return  GtkComboBox
 */
static GtkWidget *create_serial_baud_widget(const char *resource)
{
    const vice_gtk3_combo_entry_int_t *entries = NULL;

    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_VIC20:
            entries = serial_baud_rates_c64;
            break;
        case VICE_MACHINE_PLUS4:    /* fall through */
        case VICE_MACHINE_CBM5x0:   /* fall through */
        case VICE_MACHINE_CBM6x0:
            entries = serial_baud_rates_plus4;
            break;
        default:
            return NULL;
    }
    return vice_gtk3_resource_combo_int_new(resource, entries);
}

/** \brief  Create user-port RS232 emulation settings widget
 *
 * \note    C64,C128 and VIC-20 only
 *
 * \return  GtkGrid
 */
static GtkWidget *create_userport_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *rsuser_enable;
    GtkWidget *rsuser_device;
    GtkWidget *rsuser_rsiface;
    GtkWidget *wrapper;

    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, "Userport RS232 settings", 1);
    vice_gtk3_grid_set_title_margin(grid, 8);

    rsuser_enable = userport_device_check_button_new("Enable userport RS232 emulation",
                                                     USERPORT_DEVICE_RS232_MODEM);
    gtk_widget_set_margin_start(rsuser_enable, 8);
    gtk_grid_attach(GTK_GRID(grid), rsuser_enable, 0, 1, 1, 1);

    /* RS-232 Interface Widget */
    rsuser_rsiface = userport_rsinterface_widget_create();
    userport_rsinterface_widget_add_callback(GTK_GRID(rsuser_rsiface),
                                             on_rsinterface_changed);
    gtk_grid_attach(GTK_GRID(grid), rsuser_rsiface, 0, 2, 1, 1);

    /* Add the device and baud widgets to the RS-323 interface widget's grid
     * to align the widgets more consistently */

    label = create_indented_label("Device");
    rsuser_device = vice_gtk3_resource_combo_int_new("RsUserDev",
                                                         acia_devices);
    gtk_widget_set_margin_top(rsuser_device, 8);
    gtk_grid_attach(GTK_GRID(rsuser_rsiface), label,         0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(rsuser_rsiface), rsuser_device, 1, 5, 1, 1);

    /* use a wrapper grid for the "Baud" label and combo to allocate the
     * horizontal more aesthetically pleasing, kinda ; */
    wrapper = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(wrapper), 8);
    gtk_widget_set_margin_top(wrapper, 8);
    label = create_indented_label("Baud");
    rsuser_baud_widget = vice_gtk3_resource_combo_int_new("RsUserBaud",
                                                              rsuser_baud_rates);
    gtk_grid_attach(GTK_GRID(wrapper), label,              0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(wrapper), rsuser_baud_widget, 1, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(rsuser_rsiface), wrapper, 2, 5, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}




/** \brief  Create RS232 devices widget
 *
 * XXX: only supports Unix, Windows appears do things differently.
 *
 * \return  GtkGrid
 */
static GtkWidget *create_rs232_devices_widget(void)
{
    GtkWidget *grid;
    int        serial;

    grid = vice_gtk3_grid_new_spaced_with_label(8, 8, "RS232 devices", 5);
    for (serial = 1; serial <= 4; serial++) {
        GtkWidget  *widget;
        char        buffer[64];
        /* ttyu[0-3] are supposedly set up on FreeBSD */
        const char *patterns_ttys[] = { "ttyS*", "ttyu*", NULL };
        int         row = serial;   /* currently equal, might change with
                                       layout change */

        /* label separate from the browser widget to have proper alignment
         * of the entry box */
        g_snprintf(buffer, sizeof buffer, "Serial %d", serial);
        widget = create_indented_label(buffer);
        gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);

        g_snprintf(buffer, sizeof buffer, "RsDevice%d", serial);
        widget = vice_gtk3_resource_browser_new(buffer,
                                                patterns_ttys,
                                                "Serial ports",
                                                "Select serial port",
                                                NULL,  /* no label */
                                                NULL);
        gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);

        widget = gtk_label_new("Baud");
        gtk_grid_attach(GTK_GRID(grid), widget, 2, row, 1, 1);

        g_snprintf(buffer, sizeof buffer, "RsDevice%dBaud", serial);
        widget = create_serial_baud_widget(buffer);
        gtk_grid_attach(GTK_GRID(grid), widget, 3, row, 1, 1);

        widget = vice_gtk3_resource_check_button_new_sprintf("RsDevice%dip232",
                                                             "IP232",
                                                             serial);
        gtk_grid_attach(GTK_GRID(grid), widget, 4, row, 1, 1);
    }
    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create RS232 settings widget
 *
 * Invalid for PET, C64DTV and VSID
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_rs232_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *userport_widget;
    int        row = 0;

    grid = vice_gtk3_grid_new_spaced(8, 16);

    /* guard against accidental use on unsupported machines */
    if (machine_class == VICE_MACHINE_C64DTV
            || machine_class == VICE_MACHINE_PET
            || machine_class == VICE_MACHINE_VSID) {
        GtkWidget *label;
        char       message[1024];

        g_snprintf(message, sizeof message,
                   "<b>Error</b>: RS232 not supported for <b>%s</b>, please fix"
                   " the code that calls this code!",
                   machine_name);
        label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), message);
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
        gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
        gtk_widget_show_all(grid);
        return grid;
    }

    gtk_grid_attach(GTK_GRID(grid), create_acia_widget(), 0, row, 1, 1);
    row++;

    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_VIC20:
            userport_widget = create_userport_widget();
            gtk_grid_attach(GTK_GRID(grid), userport_widget, 0, row, 1, 1);
            row++;
            break;
        default:
            break;
    }

    gtk_grid_attach(GTK_GRID(grid), create_rs232_devices_widget(), 0, row, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
