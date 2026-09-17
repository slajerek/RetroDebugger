/** \file   settings_drive.c
 * \brief   Drive settings dialog
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES TrapDevice8                 -vsid -xcbm5x0 -xcbm2 -xpet
 * $VICERES TrapDevice9                 -vsid -xcbm5x0 -xcbm2 -xpet
 * $VICERES TrapDevice10                -vsid -xcbm5x0 -xcbm2 -xpet
 * $VICERES TrapDevice11                -vsid -xcbm5x0 -xcbm2 -xpet
 * $VICERES Drive8TrueEmulation         -vsid
 * $VICERES Drive9TrueEmulation         -vsid
 * $VICERES Drive10TrueEmulation        -vsid
 * $VICERES Drive11TrueEmulation        -vsid
 * $VICERES BusDevice8                  -vsid -xvic
 * $VICERES BusDevice9                  -vsid -xvic
 * $VICERES BusDevice10                 -vsid -xvic
 * $VICERES BusDevice11                 -vsid -xvic
 * $VICERES FileSystemDevice8           -vsid
 * $VICERES FileSystemDevice9           -vsid
 * $VICERES FileSystemDevice10          -vsid
 * $VICERES FileSystemDevice11          -vsid
 * $VICERES AttachDevice8d0Readonly     -vsid
 * $VICERES AttachDevice9d0Readonly     -vsid
 * $VICERES AttachDevice10d0Readonly    -vsid
 * $VICERES AttachDevice11d0Readonly    -vsid
 * $VICERES AttachDevice8d1Readonly     -vsid
 * $VICERES AttachDevice9d1Readonly     -vsid
 * $VICERES AttachDevice10d1Readonly    -vsid
 * $VICERES AttachDevice11d1Readonly    -vsid
 * $VICERES Drive8RTCSave               -vsid -xcbm5x0 -xcbm2 -xpet
 * $VICERES Drive9RTCSave               -vsid -xcbm5x0 -xcbm2 -xpet
 * $VICERES Drive10RTCSave              -vsid -xcbm5x0 -xcbm2 -xpet
 * $VICERES Drive11RTCSave              -vsid -xcbm5x0 -xcbm2 -xpet
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

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "attach.h"
#include "debug_gtk3.h"
#include "drive-check.h"
#include "drive.h"
#include "drivedoswidget.h"
#include "driveextendpolicywidget.h"
#include "drivefixedsizewidget.h"
#include "driveidlemethodwidget.h"
#include "drivemodelwidget.h"
#include "driveparallelcablewidget.h"
#include "driveramwidget.h"
#include "driverpmwidget.h"
#include "drivesoundwidget.h"
#include "driveunitwidget.h"
#include "drivewidgethelpers.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "settings_drive.h"


/** \brief  List of (virtual) file system types
 */
static const vice_gtk3_combo_entry_int_t device_types[] = {
    { "Disk images",            ATTACH_DEVICE_NONE },
    { "Host file system",       ATTACH_DEVICE_FS },
#ifdef HAVE_REALDEVICE
    { "Real device (OpenCBM)",  ATTACH_DEVICE_REAL },
#endif
    { NULL,                     -1 }
};


/* Widget references for the glue logic that updates sensitivity of those widgets
 * on drive model changes and IEC device check button toggling.
 */

/** \brief  Drive model widgets */
static GtkWidget *drive_model[NUM_DISK_UNITS];

/** \brief  TDE check buttons */
static GtkWidget *drive_tde[NUM_DISK_UNITS];

/** \brief  Virtual device check buttons */
static GtkWidget *drive_virtualdev[NUM_DISK_UNITS];

/** \brief  Read-only device check buttons */
static GtkWidget *drive_read_only[NUM_DISK_UNITS][2];

/** \brief  Real time clock save check buttons */
static GtkWidget *drive_rtc_save[NUM_DISK_UNITS];

/** \brief  IEC or IEEE-488 device check buttons */
static GtkWidget *drive_bus_device[NUM_DISK_UNITS];

/** \brief  Drive extend-policy widgets */
static GtkWidget *drive_extend[NUM_DISK_UNITS];

/** \brief  Drive idle-mode widgets */
static GtkWidget *drive_idle[NUM_DISK_UNITS];

/** \brief  Drive parallel port widgets */
static GtkWidget *drive_parallel[NUM_DISK_UNITS];

/** \brief  Drive RPM widgets */
static GtkWidget *drive_rpm[NUM_DISK_UNITS];

/** \brief  Drive RAM widgets */
static GtkWidget *drive_ram[NUM_DISK_UNITS];

/** \brief  Drive DOS widgets */
static GtkWidget *drive_dos[NUM_DISK_UNITS];

/** \brief  Drive device type widgets */
static GtkWidget *drive_device_type[NUM_DISK_UNITS];

/** \brief  Drive device type labels */
static GtkWidget *drive_device_type_label[NUM_DISK_UNITS];

/** \brief  Drive fixed-size widgets */
static GtkWidget *drive_size[NUM_DISK_UNITS];


/** \brief  Determine if the current machine supports IEC
 *
 * \return  `TRUE` if IEC-related resources are available
 */
static gboolean has_iec(void)
{
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64DTV:   /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_VIC20:    /* fall through */
        case VICE_MACHINE_PLUS4:
            return TRUE;
        default:
            return FALSE;
    }
}

/*
 * Gtk event handlers and custom widget callbacks
 */

/** \brief  Custom callback for the IEC widget in driveoptions.c
 *
 * \param[in]   widget  IEC toggle button
 * \param[in]   unit    unit number (8-11)
 *
 * This callback is used if the virtual backend depends on TrapDevice%d
 * as well as the Virtual (bus) checkbox.
 */
static void iec_callback(GtkWidget *widget, int unit)
{
    if (unit >= DRIVE_UNIT_MIN && unit <= DRIVE_UNIT_MAX) {
        int iecdev  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
        int virtdev = 0;
        int index   = unit - DRIVE_UNIT_MIN;

        resources_get_int_sprintf("TrapDevice%d", &virtdev, unit);
        gtk_widget_set_sensitive(drive_device_type_label[index], iecdev | virtdev);
        gtk_widget_set_sensitive(drive_device_type[index], iecdev | virtdev);
    }
}

/** \brief  Custom callback for the IEEE widget in driveoptions.c
 *
 * \param[in]   widget  IEEE toggle button
 * \param[in]   unit    unit number (8-11)
 *
 * This callback is used if the virtual backend depends only on
 * the Virtual (bus) checkbox.
 */
static void ieee_callback(GtkWidget *widget, int unit)
{
    if (unit >= DRIVE_UNIT_MIN && unit <= DRIVE_UNIT_MAX) {
        int ieeedev = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
        int index   = unit - DRIVE_UNIT_MIN;

        gtk_widget_set_sensitive(drive_device_type_label[index], ieeedev);
        gtk_widget_set_sensitive(drive_device_type[index], ieeedev);
    }
}

/** \brief  Extra event handler for the drive model changes
 *
 * \param[in]   widget      drive type radio button
 * \param[in]   user_data   unit number
 */
static void on_model_changed(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *option;
    int unit = GPOINTER_TO_INT(user_data);
    int model = drive_model_widget_value_combo(widget);

    option = drive_read_only[unit - DRIVE_UNIT_MIN][1];
    if (option != NULL) {
        gtk_widget_set_sensitive(option, drive_check_dual(model));
    }

    option = drive_extend[unit - DRIVE_UNIT_MIN];
    if (option != NULL) {
        gtk_widget_set_sensitive(option, drive_check_extend_policy(model));
    }
    option = drive_parallel[unit - DRIVE_UNIT_MIN];
    if (option != NULL) {
        gtk_widget_set_sensitive(option, drive_check_parallel_cable(model));
    }
    option = drive_idle[unit - DRIVE_UNIT_MIN];
    if (option != NULL) {
        gtk_widget_set_sensitive(option, drive_check_idle_method(model));
    }

    /* RAM extensions */
    option = drive_ram[unit - DRIVE_UNIT_MIN];
    if (option != NULL) {
        drive_ram_widget_update(option, model);
    }

    /* DOS extensions */
    option = drive_dos[unit - DRIVE_UNIT_MIN];
    if (option != NULL) {
        drive_dos_widget_sync_combo(option);
    }
    /* RTC save widget */
    option = drive_rtc_save[unit - DRIVE_UNIT_MIN];
    if (option != NULL) {
        gtk_widget_set_sensitive(option, drive_check_rtc(model));
    }
}

/** \brief  Handler for the 'toggled' event of the IEC/IEEE-488 bus checkbox
 *
 * Triggers the user-provided callback function on toggle.
 *
 * \param[in]   widget  IEC checkbox
 * \param[in]   data    unit number
 */
static void on_bus_toggled(GtkWidget *widget, gpointer data)
{
    void (*callback)(GtkWidget *, int);
    int unit = GPOINTER_TO_INT(data);

    callback = g_object_get_data(G_OBJECT(widget), "UnitCallback");
    if (callback != NULL) {
        callback(widget, unit);
    }
}

/** \brief  Handler for the 'toggled' event of the Virtual Devices checkbox
 *
 * Triggers the user-provided callback function on toggle.
 *
 * \param[in]   widget  Virtual Devices checkbox
 * \param[in]   data    unit number
 */
static void on_virtual_device_toggled(GtkWidget *widget, gpointer data)
{
    void (*callback)(GtkWidget *, int);
    int unit = GPOINTER_TO_INT(data);

    callback = g_object_get_data(G_OBJECT(widget), "UnitCallback");
    if (callback != NULL) {
        callback(widget, unit);
    }
}


/*
 * Private functions
 */

/** \brief  Create left aligned GtkLabel with Pango markup
 *
 * \param[in]   text    label text with Pango markup allowed
 *
 * \return  GtkLabel
 */
static GtkWidget *create_left_aligned_label(const char *text)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}

/** \brief  Create checkbox to toggle IEC/IEEE-488 (bus) Device emulation for \a unit
 *
 * \param[in]   unit        unit number (8-11)
 * \param[in]   callback    function to call on checkbutton toggle events
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_bus_check_button(int unit, void (*callback)(GtkWidget *, int))
{
    GtkWidget *check;

    check = vice_gtk3_resource_check_button_new_sprintf("BusDevice%d",
                                                        "Virtual (bus)",
                                                        unit);
    g_object_set_data(G_OBJECT(check), "UnitCallback", (gpointer)callback);
    g_signal_connect(GTK_TOGGLE_BUTTON(check),
                     "toggled",
                     G_CALLBACK(on_bus_toggled),
                     GINT_TO_POINTER(unit));
    return check;
}

/** \brief  Create widget to control read-only mode for \a unit
 *
 * \param[in]   unit    unit number (8-11)
 * \param[in]   drive   drive number (0-1)
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_readonly_check_button(int unit, int drive)
{
    GtkWidget *check;
    GtkWidget *label;
    char       markup[64];

    check =  vice_gtk3_resource_check_button_new_sprintf("AttachDevice%dd%dReadonly",
                                                         "i'm a temporary label",
                                                         unit, drive);
    /* Hack to properly align the two check buttons for drive 0 and 1:
     * Get the GtkLabel child of the GtkCheckButton and use Pango markup to
     * make the 0/1 fixed size, which properly aligns the text following
     * the 0/1. =) */
    label = gtk_bin_get_child(GTK_BIN(check));
    g_snprintf(markup, sizeof markup, "Drive <tt>%d</tt> read-only", drive);
    gtk_label_set_markup(GTK_LABEL(label), markup);

    if (drive != 0) {
        int type = ui_get_drive_type(unit);
        int dual = drive_check_dual(type);

        gtk_widget_set_sensitive(check, (gboolean)dual);
    }
    return check;
}

/** \brief  Create widget to control Real time clock emulation for \a unit
 *
 * \param[in]   unit    unit number (8-11)
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_rtc_check_button(int unit)
{
    GtkWidget *check;
    int        type;

    type  = ui_get_drive_type(unit);
    check = vice_gtk3_resource_check_button_new_sprintf("Drive%dRTCSave",
                                                        "Save real-time clock data",
                                                        unit);
    gtk_widget_set_sensitive(check, (gboolean)drive_check_rtc(type));
    return check;
}

/** \brief  Create widget to control IEC/IEEE-488 device type
 *
 * \param[in]   unit    unit number (8-11)
 *
 * \return  GtkGrid
 */
static GtkWidget *create_drive_device_type_widget(int unit)
{
    GtkWidget *combo;

    combo = vice_gtk3_resource_combo_int_new_sprintf("FileSystemDevice%d",
                                                         device_types,
                                                         unit);
    gtk_widget_set_hexpand(combo, TRUE);
    gtk_widget_show_all(combo);
    return combo;
}

/** \brief  Create per-unit True Drive Emulation check button
 *
 * \param[in]   unit    unit number (8-11)
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_drive_true_emulation_widget(int unit)
{
    return vice_gtk3_resource_check_button_new_sprintf("Drive%dTrueEmulation",
                                                       "True drive emulation",
                                                       unit);
}

/** \brief  Create per-unit Virtual Devices check button
 *
 * \param[in]   unit    unit number (8-11)
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_drive_virtual_device_widget(int unit, void (*callback)(GtkWidget *, int))
{
    GtkWidget *check;

    check = vice_gtk3_resource_check_button_new_sprintf("TrapDevice%d",
                                                        "Virtual (traps)",
                                                        unit);
    g_object_set_data(G_OBJECT(check), "UnitCallback", (gpointer)callback);
    g_signal_connect(GTK_TOGGLE_BUTTON(check),
                     "toggled",
                     G_CALLBACK(on_virtual_device_toggled),
                     GINT_TO_POINTER(unit));
    return check;
}

/** \brief  Create layout for xvic
 *
 * \param[in]   left_grid   grid of the left column
 * \param[in]   left_row    row in \a left_grid to start adding widgets
 * \param[in]   right_grid  grid of the right column
 * \param[in]   right_row   row in \a right_grid to start adding widgets
 * \param[in]   unit        unit number (8-1)
 */
static void create_vic20_layout(GtkWidget *left_grid,
                                int        left_row,
                                GtkWidget *right_grid,
                                int        right_row,
                                int        unit)
{
    int index = unit - DRIVE_UNIT_MIN;

    /* Left column widgets */

    /* IEC device type combo box */
    drive_device_type_label[index] = create_left_aligned_label("Virtual backend");
    drive_device_type[index] = create_drive_device_type_widget(unit);
    gtk_grid_attach(GTK_GRID(left_grid), drive_device_type_label[index], 0, left_row, 1, 1);
    gtk_grid_attach(GTK_GRID(left_grid), drive_device_type[index],       1, left_row, 1, 1);
    left_row++;

    /* Drive RAM */
    drive_ram[index] = drive_ram_widget_create(unit);
    gtk_widget_set_margin_top(drive_ram[index], 16);
    gtk_grid_attach(GTK_GRID(left_grid), drive_ram[index], 0, left_row, 2, 1);
    left_row++;

    /* Right column widgets (none right now) */
}

/** \brief  Create layout for x64, x64sc, xscpu64 and x128
 *
 * \param[in]   left_grid   grid of the left column
 * \param[in]   left_row    row in \a left_grid to start adding widgets
 * \param[in]   right_grid  grid of the right column
 * \param[in]   right_row   row in \a right_grid to start adding widgets
 * \param[in]   unit        unit number (8-1)
 */
static void create_c64_layout(GtkWidget *left_grid,
                              int        left_row,
                              GtkWidget *right_grid,
                              int        right_row,
                              int        unit)
{
    GtkWidget *label;
    int        index = unit - DRIVE_UNIT_MIN;  /* index in widget arrays */

    /* Left column widgets */

    /* IEC/IEEE-488 (bus) device check button */
    drive_bus_device[index] = create_bus_check_button(unit, iec_callback);
    gtk_grid_attach(GTK_GRID(left_grid), drive_bus_device[index], 0, left_row, 2, 1);
    left_row++;

    /* IEC/IEEE-488 (bus) device type combo box */
    drive_device_type_label[index] = create_left_aligned_label("Virtual backend");
    drive_device_type[index] = create_drive_device_type_widget(unit);
    gtk_widget_set_margin_top(drive_device_type_label[index], 8);
    gtk_widget_set_margin_top(drive_device_type[index], 8);
    gtk_grid_attach(GTK_GRID(left_grid), drive_device_type_label[index], 0, left_row, 1, 1);
    gtk_grid_attach(GTK_GRID(left_grid), drive_device_type[index],       1, left_row, 1, 1);
    left_row++;

    /* Fixed HD size */
    label = create_left_aligned_label("CMD-HD size");
    drive_size[index] = drive_fixed_size_widget_create(unit);
    gtk_widget_set_margin_top(drive_size[index], 8);
    gtk_grid_attach(GTK_GRID(left_grid), label,             0, left_row, 1, 1);
    gtk_grid_attach(GTK_GRID(left_grid), drive_size[index], 1, left_row, 1, 1);
    left_row++;

    /* Drive RAM */
    drive_ram[index] = drive_ram_widget_create(unit);
    gtk_widget_set_margin_top(drive_ram[index], 16);
    gtk_grid_attach(GTK_GRID(left_grid), drive_ram[index], 0, left_row, 2, 1);
    left_row++;


    /* Right column widgets */

    /* DOS expansion */
    label = create_left_aligned_label("DOS expansion");
    drive_dos[index] = drive_dos_widget_create_combo(unit);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_top(drive_dos[index], 8);
    gtk_grid_attach(GTK_GRID(right_grid), label,            0, right_row, 1, 1);
    gtk_grid_attach(GTK_GRID(right_grid), drive_dos[index], 1, right_row, 1, 1);
    right_row++;

    /* Parallel cable */
    label = create_left_aligned_label("Parallel cable");
    drive_parallel[index] = drive_parallel_cable_widget_create(unit);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_top(drive_parallel[index], 8);
    gtk_grid_attach(GTK_GRID(right_grid), label,                 0, right_row, 1, 1);
    gtk_grid_attach(GTK_GRID(right_grid), drive_parallel[index], 1, right_row, 1, 1);
    right_row++;
}

/** \brief  Create layout for xplus4 and c64dtv
 *
 * \param[in]   left_grid   grid of the left column
 * \param[in]   left_row    row in \a left_grid to start adding widgets
 * \param[in]   right_grid  grid of the right column
 * \param[in]   right_row   row in \a right_grid to start adding widgets
 * \param[in]   unit        unit number (8-1)
 */
static void create_plus4_layout(GtkWidget *left_grid,
                                int        left_row,
                                GtkWidget *right_grid,
                                int        right_row,
                                int        unit)
{
    GtkWidget *label;
    int index = unit - DRIVE_UNIT_MIN;  /* index in widget arrays */

    /* Left column widgets */

    /* IEC device check button */
    drive_bus_device[index] = create_bus_check_button(unit, iec_callback);
    gtk_grid_attach(GTK_GRID(left_grid), drive_bus_device[index], 0, left_row, 2, 1);
    left_row++;

    /* IEC device type combo box */
    drive_device_type_label[index] = create_left_aligned_label("Virtual backend");
    drive_device_type[index] = create_drive_device_type_widget(unit);
    gtk_widget_set_margin_top(drive_device_type_label[index], 8);
    gtk_widget_set_margin_top(drive_device_type[index], 8);
    gtk_grid_attach(GTK_GRID(left_grid), drive_device_type_label[index], 0, left_row, 1, 1);
    gtk_grid_attach(GTK_GRID(left_grid), drive_device_type[index],       1, left_row, 1, 1);
    left_row++;

    /* Drive RAM */
    drive_ram[index] = drive_ram_widget_create(unit);
    gtk_widget_set_margin_top(drive_ram[index], 16);
    gtk_grid_attach(GTK_GRID(left_grid), drive_ram[index], 0, left_row, 2, 1);


    /* Right column widgets (none at the moment) */

    /* Parallel cable */
    label = create_left_aligned_label("Parallel cable");
    drive_parallel[index] = drive_parallel_cable_widget_create(unit);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_top(drive_parallel[index], 8);
    gtk_grid_attach(GTK_GRID(right_grid), label,                 0, right_row, 1, 1);
    gtk_grid_attach(GTK_GRID(right_grid), drive_parallel[index], 1, right_row, 1, 1);
    right_row++;
}

/** \brief  Create layout for xpet, xcbm5x0 and xcbm2
 *
 * \param[in]   left_grid   grid of the left column
 * \param[in]   left_row    row in \a left_grid to start adding widgets
 * \param[in]   right_grid  grid of the right column
 * \param[in]   right_row   row in \a right_grid to start adding widgets
 * \param[in]   unit        unit number (8-1)
 */
static void create_pet_layout(GtkWidget *left_grid,
                              int        left_row,
                              GtkWidget *right_grid,
                              int        right_row,
                              int        unit)
{
    int index = unit - DRIVE_UNIT_MIN;  /* index in widget arrays */

    /* Left column widgets */

    /* IEEE-488 device check button */
    drive_bus_device[index] = create_bus_check_button(unit, ieee_callback);
    gtk_grid_attach(GTK_GRID(left_grid), drive_bus_device[index], 0, left_row, 2, 1);
    left_row++;

    /* Virtual device type combo box */
    drive_device_type_label[index] = create_left_aligned_label("Virtual backend");
    drive_device_type[index] = create_drive_device_type_widget(unit);
    gtk_widget_set_margin_top(drive_device_type_label[index], 8);
    gtk_widget_set_margin_top(drive_device_type[index], 8);
    gtk_grid_attach(GTK_GRID(left_grid), drive_device_type_label[index], 0, left_row, 1, 1);
    gtk_grid_attach(GTK_GRID(left_grid), drive_device_type[index],       1, left_row, 1, 1);
    left_row++;

    /* Right column widgets (none at the moment) */
}

/** \brief  Add widgets to left column grid that are shared by all machines
 *
 * Add widgets that are valid for each machine to the \a grid making up the
 * left column of the layout.
 *
 * \param[in]   grid    left column grid (contains two columns)
 * \param[in]   unit    drive unit number (8-11)
 *
 * \return  row index in \a grid for additional widgets
 */
static int create_left_layout(GtkWidget *grid, int unit)
{
    GtkWidget *label;
    int        index = unit - DRIVE_UNIT_MIN;
    int        row = 0;

    /* Drive model */
    label = create_left_aligned_label("Drive model");
    drive_model[index] = drive_model_widget_create_combo(unit, FALSE);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_widget_set_margin_bottom(drive_model[index], 8);
    gtk_widget_set_hexpand(drive_model[index], TRUE);
    gtk_grid_attach(GTK_GRID(grid), label,              0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), drive_model[index], 1, row, 1, 1);
    row++;

    /* Read-only check buttons */
    drive_read_only[index][0] = create_readonly_check_button(unit, 0);
    gtk_grid_attach(GTK_GRID(grid), drive_read_only[index][0], 0, row, 2, 1);
    row++;
    drive_read_only[index][1] = create_readonly_check_button(unit, 1);
    gtk_grid_attach(GTK_GRID(grid), drive_read_only[index][1], 0, row, 2, 1);
    row++;

    /* RTC save check button */
    if (has_iec()) {
        drive_rtc_save[index] = create_rtc_check_button(unit);
        gtk_grid_attach(GTK_GRID(grid), drive_rtc_save[index],     0, row, 2, 1);
        row++;
    }

    /* true drive emulation check button */
    drive_tde[index] = create_drive_true_emulation_widget(unit);
    gtk_grid_attach(GTK_GRID(grid), drive_tde[index],          0, row, 2, 1);
    row++;

    /* Virtual Device (bus) check button */
    if (has_iec()) {
        drive_virtualdev[index] = create_drive_virtual_device_widget(unit, iec_callback);
        gtk_grid_attach(GTK_GRID(grid), drive_virtualdev[index],   0, row, 2, 1);
        row++;
    }

    drive_model_widget_add_callback(drive_model[index],
                                    on_model_changed,
                                    GINT_TO_POINTER(unit));
    return row;
}

/** \brief  Add widgets to right column grid that are shared by all machines
 *
 * Add widgets that are valid for each machine to the \a grid making up the
 * right column of the layout.
 *
 * \param[in]   grid    right column grid (contains two columns)
 * \param[in]   unit    drive unit number (8-11)
 *
 * \return  row index in \a grid for additional widgets
 */
static int create_right_layout(GtkWidget *grid, int unit)
{
    GtkWidget *label;
    int        index = unit - DRIVE_UNIT_MIN;
    int        row = 0;

    /* RPM settings */
    drive_rpm[index] = drive_rpm_widget_create(unit);
    gtk_widget_set_margin_bottom(drive_rpm[index], 16);
    gtk_grid_attach(GTK_GRID(grid), drive_rpm[index],    0, row, 2, 1);
    row++;

    /* Image extend policy for track 36+ */
    label = create_left_aligned_label("40-track policy");
    drive_extend[index] = drive_extend_policy_widget_create(unit);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_top(drive_extend[index], 8);
    gtk_grid_attach(GTK_GRID(grid), label,               0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), drive_extend[index], 1, row, 1, 1);
    row++;

    /* Idle method */
    label = create_left_aligned_label("Idle method");
    drive_idle[index] = drive_idle_method_widget_create(unit);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_top(drive_idle[index], 8);
    gtk_grid_attach(GTK_GRID(grid), label,               0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), drive_idle[index],   1, row, 1, 1);
    row++;

    return row;
}

/** \brief  Create a composite widget with settings for drive \a unit
 *
 * \param[in]   unit    unit number
 *
 * \return  GtkGrid
 */
static GtkWidget *create_stack_child_widget(int unit)
{
    GtkWidget *stack_grid;
    GtkWidget *left_grid;
    GtkWidget *right_grid;
    int        left_row;
    int        right_row;

    stack_grid = vice_gtk3_grid_new_spaced(16, 0);
    left_grid  = vice_gtk3_grid_new_spaced(8, 0);
    right_grid = vice_gtk3_grid_new_spaced(8, 0);
    left_row   = create_left_layout(left_grid, unit);
    right_row  = create_right_layout(right_grid, unit);
    gtk_grid_attach(GTK_GRID(stack_grid), left_grid,  0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(stack_grid), right_grid, 1, 0, 1, 1);
    g_object_set_data(G_OBJECT(stack_grid), "UnitNumber", GINT_TO_POINTER(unit));

    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:
            create_c64_layout(left_grid, left_row, right_grid, right_row, unit);
            break;
        case VICE_MACHINE_VIC20:
            create_vic20_layout(left_grid, left_row, right_grid, right_row, unit);
            break;
        case VICE_MACHINE_PLUS4:    /* fall through */
        case VICE_MACHINE_C64DTV:
            create_plus4_layout(left_grid, left_row, right_grid, right_row, unit);
            break;
        case VICE_MACHINE_PET:      /* fall through */
        case VICE_MACHINE_CBM5x0:   /* fall through */
        case VICE_MACHINE_CBM6x0:
            create_pet_layout(left_grid, left_row, right_grid, right_row, unit);
            break;
        default:
            break;
    }

    gtk_widget_show_all(stack_grid);
    return stack_grid;
}


/** \brief  Create main drive settings widget
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 */
GtkWidget *settings_drive_widget_create(GtkWidget *parent)
{
    GtkWidget *layout;
    GtkWidget *sound;
    GtkWidget *stack;
    GtkWidget *switcher;
    int        unit;

    layout = gtk_grid_new();

    sound = drive_sound_widget_create();
    gtk_widget_set_margin_bottom(sound, 16);
    gtk_grid_attach(GTK_GRID(layout), sound, 0, 0, 1, 1);

    stack = gtk_stack_new();
    for (unit = DRIVE_UNIT_MIN; unit <= DRIVE_UNIT_MAX; unit++) {
        char title[32];

        g_snprintf(title, sizeof title , "Drive %d", unit);
        gtk_stack_add_titled(GTK_STACK(stack),
                             create_stack_child_widget(unit),
                             title,
                             title);
    }
    gtk_stack_set_transition_type(GTK_STACK(stack),
                                  GTK_STACK_TRANSITION_TYPE_NONE);

    switcher = gtk_stack_switcher_new();
    gtk_widget_set_halign(switcher, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(switcher, TRUE);
    gtk_widget_set_margin_bottom(switcher, 16);
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack));

    gtk_widget_show_all(stack);
    gtk_widget_show_all(switcher);

    gtk_grid_attach(GTK_GRID(layout), switcher, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(layout), stack, 0, 2, 1, 1);

    /* set sensitivity of the filesystem-type comboboxes, depending on the
     * BusDevice (not for VIC20 and PET/CBM-II) and TrapDevice resource
     */
    for (unit = DRIVE_UNIT_MIN; unit <= DRIVE_UNIT_MAX; unit++) {
        int iecdev = 0;
        int virtdev = 0;
        int index = unit - DRIVE_UNIT_MIN;  /* index in the widget arrays */

        /* FIXME: xvic doesn't use BusDevice (yet), uses its own iecbus code */
        if (machine_class != VICE_MACHINE_VIC20) {
            resources_get_int_sprintf("BusDevice%d", &iecdev, unit);
        }
        resources_get_int_sprintf("TrapDevice%d", &virtdev, unit);
        /* try to set sensitive, regardless of if the widget actually
         * exists, this helps with debugging since Gtk will print a warning
         * on the console.
         */
        gtk_widget_set_sensitive(drive_device_type_label[index], iecdev | virtdev);
        gtk_widget_set_sensitive(drive_device_type[index], iecdev | virtdev);
    }

    gtk_widget_show_all(layout);
    return layout;
}
