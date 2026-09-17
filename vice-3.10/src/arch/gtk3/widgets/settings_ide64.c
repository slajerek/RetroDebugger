/** \file   settings_ide64.c
 * \brief   Settings widget to control IDE64 resources
 *
 * IDE64 settings widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES IDE64AutodetectSize1    x64 x64sc xscpu64 x128
 * $VICERES IDE64AutodetectSize2    x64 x64sc xscpu64 x128
 * $VICERES IDE64AutodetectSize3    x64 x64sc xscpu64 x128
 * $VICERES IDE64AutodetectSize4    x64 x64sc xscpu64 x128
 * $VICERES IDE64version            x64 x64sc xscpu64 x128
 * $VICERES IDE64Image1             x64 x64sc xscpu64 x128
 * $VICERES IDE64Image2             x64 x64sc xscpu64 x128
 * $VICERES IDE64Image3             x64 x64sc xscpu64 x128
 * $VICERES IDE64Image4             x64 x64sc xscpu64 x128
 * $VICERES IDE64ClockPort          x64 x64sc xscpu64 x128
 * $VICERES IDE64Cylinders1         x64 x64sc xscpu64 x128
 * $VICERES IDE64Cylinders2         x64 x64sc xscpu64 x128
 * $VICERES IDE64Cylinders3         x64 x64sc xscpu64 x128
 * $VICERES IDE64Cylinders4         x64 x64sc xscpu64 x128
 * $VICERES IDE64Heads1             x64 x64sc xscpu64 x128
 * $VICERES IDE64Heads2             x64 x64sc xscpu64 x128
 * $VICERES IDE64Heads3             x64 x64sc xscpu64 x128
 * $VICERES IDE64Heads4             x64 x64sc xscpu64 x128
 * $VICERES IDE64Sectors1           x64 x64sc xscpu64 x128
 * $VICERES IDE64Sectors2           x64 x64sc xscpu64 x128
 * $VICERES IDE64Sectors3           x64 x64sc xscpu64 x128
 * $VICERES IDE64Sectors4           x64 x64sc xscpu64 x128
 * $VICERES IDE64USBServer          x64 x64sc xscpu64 x128
 * $VICERES IDE64USBServerAddress   x64 x64sc xscpu64 x128
 * $VICERES IDE64RTCSave            x64 x64sc xscpu64 x128
 * $VICERES SBDIGIMAX               x64 x64sc xscpu64 x128
 * $VICERES SBDIGIMAXbase           x64 x64sc xscpu64 x128
 * $VICERES SBETFE                  x64 x64sc xscpu64 x128
 * $VICERES SBETFEbase              x64 x64sc xscpu64 x128
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

#include "cartridge.h"
#include "ide64.h"
#include "machine.h"
#include "resources.h"
#include "ui.h"
#include "vice_gtk3.h"

#include "settings_ide64.h"


/** \brief  IDE64 geometry spin button data object */
typedef struct {
    const char *label;      /**< label for the spin button */
    const char *resource;   /**< resource name format string */
    int         min;        /**< lower bound of spin button */
    int         max;        /**< upper bound of spin button */
    int         step;       /**< stepping for the +/- buttons */
} geometry_spin_t;


/** \brief  List of IDE64 revisions
 */
static const vice_gtk3_radiogroup_entry_t revisions[] = {
    { "Version 3",      IDE64_VERSION_3 },
    { "Version 4.1",    IDE64_VERSION_4_1 },
    { "Version 4.2",    IDE64_VERSION_4_2 },
    { NULL,             -1 }
};

/** \brief  List of ShortBus DIGIMAX I/O bases
 */
static const vice_gtk3_combo_entry_int_t digimax_addresses[] = {
    { "$DE40",  0xde40 },
    { "$DE48",  0xde48 },
    { NULL,     -1 }
};

#ifdef HAVE_RAWNET
/** \brief  List of ShortBus ETFE I/O bases
 */
static const vice_gtk3_combo_entry_int_t etfe_addresses[] = {
    { "$DE00",  0xde00 },
    { "$DE10",  0xde10 },
    { "$DF00",  0xdf00 },
    { NULL,     -1 }
};
#endif

/** \brief  IDE64 geometry spin button data */
static const geometry_spin_t geometry_spins[] = {
    { "Cylinders", "IDE64cylinders%d", IDE64_CYLINDERS_MIN, IDE64_CYLINDERS_MAX, 256 },
    { "Heads",     "IDE64heads%d",     IDE64_HEADS_MIN,     IDE64_HEADS_MAX,       1 },
    { "Sectors",   "IDE64sectors%d",   IDE64_SECTORS_MIN,   IDE64_SECTORS_MAX,    16 }
};


/** \brief  References to the HD image file chooser widgets
 */
static GtkWidget *image_entry[IDE64_DEVICE_COUNT];


/*
 * Event handlers for the various toggle buttons
 */

/** \brief  Handler for the "toggled" event of the USB Server check button
 *
 * \param[in]       usb_enable  USB server check button
 * \param[in,out]   usb_address USB server address widget
 */
static void on_usb_enable_toggled(GtkWidget *usb_enable, gpointer usb_address)
{
    gboolean active;

    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(usb_enable));
    gtk_widget_set_sensitive(GTK_WIDGET(usb_address), active);
}

/** \brief  Handler for the "toggled" event of the "Autodetect Size" widgets
 *
 * \param[in]       autodetect_enable   autodetect enable check button
 * \param[in,out]   geometry_grid       geometry widgets grid
 */
static void on_autosize_toggled(GtkWidget *autodetect_enable,
                                gpointer   geometry_grid)
{
    gboolean active;

    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(autodetect_enable));
    gtk_widget_set_sensitive(GTK_WIDGET(geometry_grid), !active);
}

/** \brief  Handler for the "toggled" event of the "Enable DigiMAX" widget
 *
 * \param[in]       digimax_enable  DigiMax enable check button
 * \param[in,out]   digimax_address DigiMax address widget
 */
static void on_digimax_toggled(GtkWidget *digimax_enable,
                               gpointer   digimax_address)
{
    gboolean active;

    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(digimax_enable));
    gtk_widget_set_sensitive(GTK_WIDGET(digimax_address), active);
}

#ifdef HAVE_RAWNET
/** \brief  Handler for the "toggled" event of the "Enable ETFE" widget
 *
 * \param[in]       etfe_enable     ETFE enable check button
 * \param[in,out]   etfe_address    ETFE address widget
 */
static void on_etfe_toggled(GtkWidget *etfe_enable, gpointer etfe_address)
{
    gboolean active;

    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(etfe_enable));
    gtk_widget_set_sensitive(GTK_WIDGET(etfe_address), active);
}
#endif


/** \brief  Create grid with spin buttons for IDE64 disk geometry resources
 *
 * \param[in]   device  IDE64 device number (1-4)
 *
 * \return  GtkGrid
 */
static GtkWidget *create_ide64_geometry_widget(int device)
{
    GtkWidget *grid;
    int        index;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    /* lay out widgets horizontally */
    for (index = 0; index < G_N_ELEMENTS(geometry_spins); index++) {
        GtkWidget             *label;
        GtkWidget             *spin;
        const geometry_spin_t *geo;

        geo   = &geometry_spins[index];
        label = gtk_label_new(geo->label);
        spin  = vice_gtk3_resource_spin_int_new_sprintf(geo->resource,
                                                        geo->min,
                                                        geo->max,
                                                        geo->step,
                                                        device);
        gtk_widget_set_halign(label, GTK_ALIGN_END);
        /* using 1 column for the label and 2 for the spin button creates a
         * homogeneous layout without too much wasted space */
        gtk_grid_attach(GTK_GRID(grid), label, index * 3 + 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), spin,  index * 3 + 1, 0, 2, 1);
    }
    return grid;
}

/** \brief  Create widget to select HD image and set geometry
 *
 * \param[in]   device  device number (1-4)
 *
 * \return  GtkGrid
 */
static GtkWidget *create_ide64_device_widget(int device)
{
    GtkWidget  *grid;
    GtkWidget  *chlabel;
    GtkWidget  *autosize;
    GtkWidget  *geometry;
    GtkWidget  *chooser;
    const char *patterns[] = {
        "*.hdd", "*.iso", "*.fdd", "*.cfa", "*.dsk", "*.img", NULL
    };

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    /* set up file chooser for HD image files */
    chlabel = gtk_label_new("HD image");
    gtk_widget_set_halign(chlabel, GTK_ALIGN_START);
    chooser = vice_gtk3_resource_filechooser_new_sprintf("IDE64image%d",
                                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                                         device);
    vice_gtk3_resource_filechooser_set_filter(chooser,
                                              "HD images",
                                              patterns,
                                              TRUE);
    image_entry[device - 1] = chooser;
    gtk_widget_set_hexpand(chooser, TRUE);

    autosize = vice_gtk3_resource_check_button_new_sprintf("IDE64AutodetectSize%d",
                                                           "Autodetect image size",
                                                           device);

    /* create grid for the geomerty spin buttons */
    geometry = create_ide64_geometry_widget(device);

    gtk_grid_attach(GTK_GRID(grid), chlabel,  0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), chooser,  1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), autosize, 0, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), geometry, 0, 2, 2, 1);

    /* enable/disable geometry widgets depending on 'autosize' state */
    gtk_widget_set_sensitive(geometry,
                             !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(autosize)));
    g_signal_connect_unlocked(autosize,
                              "toggled",
                              G_CALLBACK(on_autosize_toggled),
                              (gpointer)geometry);

    gtk_widget_show_all(grid);
    return grid;
}


/*
 * Layout helpers: functions adding widgets to the main grid
 */

/** \brief  Add IDE64 revision widgets to the main grid
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to add widgets
 *
 * \return  row in \a grid for next widgets
 */
static int create_ide64_revision_layout(GtkWidget *grid, int row)
{
    GtkWidget *label;
    GtkWidget *group;

    label = gtk_label_new(CARTRIDGE_NAME_IDE64 " revision");
    group = vice_gtk3_resource_radiogroup_new("IDE64version",
                                              revisions,
                                              GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_set_column_homogeneous(GTK_GRID(group), TRUE);
    gtk_widget_set_hexpand(group, TRUE);
    gtk_widget_set_margin_bottom(label, 16);
    gtk_widget_set_margin_bottom(group, 16);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), group, 1, 0, 2, 1);

    return row + 1;
}

/** \brief  Add USB server widgets to the main grid
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to add widgets
 *
 * \return  row in \a grid for next widgets
 */
static int create_ide64_usb_layout(GtkWidget *grid, int row)
{
    GtkWidget *enable;
    GtkWidget *address;
    GtkWidget *label;

    enable  = vice_gtk3_resource_check_button_new("IDE64USBServer",
                                                  "Enable USB server");
    label   = gtk_label_new("USB server address");
    address = vice_gtk3_resource_entry_new("IDE64USBServerAddress");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(address, TRUE);

    gtk_grid_attach(GTK_GRID(grid), enable,  0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label,   1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), address, 2, row, 1, 1);

    gtk_widget_set_sensitive(address,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enable)));
    g_signal_connect_unlocked(enable,
                              "toggled",
                              G_CALLBACK(on_usb_enable_toggled),
                              (gpointer)address);
    return row + 1;
}

/** \brief  Add RTC/ClockPort widget to main grid
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to add widgets
 *
 * \return  row in \a grid for next widgets
 */
static int create_ide64_clockport_layout(GtkWidget *grid, int row)
{
    GtkWidget *rtc_enable;
    GtkWidget *cp_label;
    GtkWidget *cp_combo;

    rtc_enable =  vice_gtk3_resource_check_button_new("IDE64RTCSave",
                                                      "Enable RTC saving");
    cp_label   = gtk_label_new("ClockPort device");
    cp_combo   = clockport_device_widget_create("IDE64ClockPort");
    gtk_widget_set_halign(cp_label, GTK_ALIGN_START);

    gtk_grid_attach(GTK_GRID(grid), rtc_enable, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cp_label,   1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cp_combo,   2, row, 1, 1);

    return row + 1;
}

/** \brief  Add ShortBus settings widgets to the main grid
 *
 * Handles the SBDIGIMAX/SBDIGIMAXbase and SBETFE/SBETFEbase resources.
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to add widgets
 *
 * \return  row for in \a grid for next widgets
 */
static int create_ide64_shortbus_layout(GtkWidget *grid, int row)
{
    GtkWidget *digimax_enable;
    GtkWidget *digimax_adlabel;
    GtkWidget *digimax_address;
#ifdef HAVE_RAWNET
    GtkWidget *etfe_enable;
    GtkWidget *etfe_adlabel;
    GtkWidget *etfe_address;
#endif
    GtkWidget *title;

    title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>ShortBus settings</b>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_widget_set_margin_top(title, 8);
    gtk_grid_attach(GTK_GRID(grid), title, 0, row, 3, 1);
    row++;

    digimax_enable = vice_gtk3_resource_check_button_new("SBDIGIMAX",
                                                         "Enable " CARTRIDGE_NAME_DIGIMAX);
    digimax_adlabel = gtk_label_new(CARTRIDGE_NAME_DIGIMAX " base address");
    digimax_address = vice_gtk3_resource_combo_int_new("SBDIGIMAXbase",
                                                       digimax_addresses);
    gtk_widget_set_halign(digimax_adlabel, GTK_ALIGN_START);

    gtk_grid_attach(GTK_GRID(grid), digimax_enable,  0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), digimax_adlabel, 1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), digimax_address, 2, row, 1, 1);

    gtk_widget_set_sensitive(digimax_address,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(digimax_enable)));
    g_signal_connect_unlocked(digimax_enable,
                              "toggled",
                              G_CALLBACK(on_digimax_toggled),
                              (gpointer)digimax_address);

#ifdef HAVE_RAWNET
    row++;

    etfe_enable = vice_gtk3_resource_check_button_new("SBETFE",
                                                      "Enable ETFE");
    etfe_adlabel = gtk_label_new("ETFE base address");
    etfe_address = vice_gtk3_resource_combo_int_new("SBETFEbase",
                                                    etfe_addresses);
    gtk_widget_set_halign(etfe_adlabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), etfe_enable,  0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), etfe_adlabel, 1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), etfe_address, 2, row, 1, 1);

    gtk_widget_set_sensitive(etfe_address,
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(etfe_enable)));
    g_signal_connect_unlocked(etfe_enable,
                              "toggled",
                              G_CALLBACK(on_etfe_toggled),
                              (gpointer)etfe_address);
#endif
    return row + 1;
}

/** \brief  Add widgets for the IDE64 devices to the main grid
 *
 * Add a stack and stack switcher with four stack children for each device.
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to add widgets
 *
 * \return  row in \a grid for next widgets
 */
static int create_ide64_devices_layout(GtkWidget *grid, int row)
{
    GtkWidget *stack;
    GtkWidget *switcher;
    int        device;

    /* create stack and stack switcher (tab-like interface) for the HD image
     * setting widgets */
    stack    = gtk_stack_new();
    switcher = gtk_stack_switcher_new();
    gtk_stack_set_transition_type(GTK_STACK(stack),
                                  GTK_STACK_TRANSITION_TYPE_NONE);
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher),
                                 GTK_STACK(stack));

    /* add child widgets */
    for (device = IDE64_DEVICE_MIN; device <= IDE64_DEVICE_MAX; device++) {
        char name[64];

        g_snprintf(name, sizeof name, "IDE Device %d", device);
        gtk_stack_add_titled(GTK_STACK(stack),
                             create_ide64_device_widget(device),
                             name,
                             name);
    }

    switcher = gtk_stack_switcher_new();
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher),
                                 GTK_STACK(stack));
    gtk_widget_set_halign(switcher, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(switcher, 16);
    gtk_widget_set_margin_bottom(switcher, 8);
    gtk_widget_show_all(stack);
    gtk_widget_show_all(switcher);

    gtk_grid_attach(GTK_GRID(grid), switcher, 0, row,     3, 1);
    gtk_grid_attach(GTK_GRID(grid), stack,    0, row + 1, 3, 1);

    return row + 1;
}


/** \brief  Create widget to control IDE64 resources
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_ide64_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    int        row = 0;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    row = create_ide64_revision_layout (grid, row);
    row = create_ide64_usb_layout      (grid, row);
    row = create_ide64_clockport_layout(grid, row);
    row = create_ide64_shortbus_layout (grid, row);
    row = create_ide64_devices_layout  (grid, row);

    gtk_widget_show_all(grid);
    return grid;
}
