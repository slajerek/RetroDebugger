/** \file   settings_ltkernal.c
 * \brief   Settings widget to control Lt. Kernal resources
 *
 * Lt. Kernal cartridge settings.
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES LTKio           x64 x64sc x128
 * $VICERES LTKserial       x64 x64sc x128
 * $VICERES LTKimage0       x64 x64sc x128
 * $VICERES LTKimage1       x64 x64sc x128
 * $VICERES LTKimage2       x64 x64sc x128
 * $VICERES LTKimage3       x64 x64sc x128
 * $VICERES LTKimage4       x64 x64sc x128
 * $VICERES LTKimage5       x64 x64sc x128
 * $VICERES LTKimage6       x64 x64sc x128
 * $VICERES LTKport         x64 x64sc x128
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
#include "ltkernal.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "settings_ltkernal.h"


/** \brief  CSS used to mark 'serial number' invalid
 */
#define SERIAL_INVALID_CSS \
    "entry {\n" \
    "  text-decoration: crimson wavy underline;\n" \
    "}"


/** \brief  Filter patterns for HD image files
 *
 * Used in the HD image file chooser widgets.
 */
static const char *patterns[] = {
    "*.hdd", "*.iso", "*.fdd", "*.cfa", "*.dsk", "*.img", NULL
};

/** \brief  List of I/O addresses for the cartridge
 */
static vice_gtk3_radiogroup_entry_t io_entries[] = {
    { "$DExx",  LTKIO_DE00 },
    { "$DFxx",  LTKIO_DF00 },
    { NULL,     -1 }
};

/** \brief  List of valid keys for the serial number widget
 */
static const gint valid_keys[] = {
    GDK_KEY_0, GDK_KEY_1, GDK_KEY_2, GDK_KEY_3, GDK_KEY_4,
    GDK_KEY_5, GDK_KEY_6, GDK_KEY_7, GDK_KEY_8, GDK_KEY_9,
    GDK_KEY_BackSpace,
    GDK_KEY_Delete,
    GDK_KEY_Insert,
    GDK_KEY_Left,
    GDK_KEY_Right,
    GDK_KEY_Home,
    GDK_KEY_End,
    -1
};


/** \brief  Reference to the CSS provider used to mark 'serial number' invalid */
static GtkCssProvider *css_provider;

/** \brief  Indicates whether the css provider has been added */
static gboolean provider_added = FALSE;


/** \brief  Create widget to set port number
 *
 * \return  GtkSpinButton
 */
static GtkWidget *create_port_number_widget(void)
{
    return vice_gtk3_resource_spin_int_new("LTKport",
                                           LTK_PORT_MIN,
                                           LTK_PORT_MAX,
                                           1);
}

/** \brief  Create widget to select I/O address range
 *
 * \return  GtkGrid
 */
static GtkWidget *create_io_address_widget(void)
{
    GtkWidget *group;

    group = vice_gtk3_resource_radiogroup_new("LTKio",
                                              io_entries,
                                              GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_set_column_spacing(GTK_GRID(group), 16);
    return group;
}

/** \brief  Attempt to set the "LTKserial" resource
 *
 * Uses the text in \a entry to set the "LTKserial" resource.
 * If successful any CSS provider marking the text invalid is removed, if
 * unsuccessful a CSS provider is added marking the text invalid.
 *
 * \param[in]   entry   entry with serial number
 *
 * \return  TRUE on success
 */
static gboolean set_serial_resource(GtkEntry *entry)
{
    if (resources_set_string("LTKserial", gtk_entry_get_text(entry)) == 0) {
        if (provider_added) {
            vice_gtk3_css_provider_remove(GTK_WIDGET(entry), css_provider);
            provider_added = FALSE;
        }
        return TRUE;
    } else {
        vice_gtk3_css_provider_add(GTK_WIDGET(entry), css_provider);
        provider_added = TRUE;
        return FALSE;
    }
}

/** \brief  Handler for the 'destroy' event of the entry
 *
 * Called on entry widget destruction, unrefs CSS provider.
 *
 * \param[in]   entry   entry widget (unused);
 * \param[in]   data    extra event data (unused)
 */
static void on_serial_destroy(GtkEntry *self, gpointer data)
{
    g_object_unref(css_provider);
}

/** \brief  Handler for the 'focus-out' event of the serial number entry
 *
 * \param[in]   entry   entry box
 * \param[in]   event   event object (unused)
 * \param[in]   data    extra event data (unused)
 *
 * \return  GDK_EVENT_PROPAGATE
 */
static gboolean on_serial_focus_out_event(GtkEntry *entry,
                                          GdkEvent *event,
                                          gpointer  data)
{
    set_serial_resource(entry);
    return GDK_EVENT_PROPAGATE;
}

/** \brief  Handler for the 'on-key-press' event for the serial number entry
 *
 * \param[in]   entry   entry box
 * \param[in]   event   event object
 * \param[in]   data    extra event data (unused)
 *
 * \return  GDK_EVENT_STOP if Return or Enter was pushed, GDK_EVENT_PROPAGATE
 *          otherwise
 */
static gboolean on_serial_key_press_event(GtkEntry *entry,
                                          GdkEvent *event,
                                          gpointer  data)
{
    if (gdk_event_get_event_type(event) == GDK_KEY_PRESS) {
        guint keyval = 0;

        if (gdk_event_get_keyval(event, &keyval)) {
            /* Iso_Left_Tab is used by X11 for Shift+Tab */
            if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter ||
                    keyval == GDK_KEY_Tab || keyval == GDK_KEY_ISO_Left_Tab) {
                set_serial_resource(entry);
                return GDK_EVENT_PROPAGATE;
            } else {
                int k;

                for (k = 0; valid_keys[k] >= 0; k++) {
                    if ((guint)valid_keys[k] == keyval) {
                        return GDK_EVENT_PROPAGATE; /* accept this key */
                    }
                }
                return GDK_EVENT_STOP;  /* we don't accept this key */
            }
        }
    }
    return GDK_EVENT_PROPAGATE;
}

/** \brief  Create entry box for the Lt. Kernal serial number
 *
 * The serial number should be a string of 8 decimal digits.
 *
 * TODO:    Check input and set resource on focus loss or Enter pressed
 *
 * \return  GtkWidget
 */
static GtkWidget *create_serial_entry(void)
{
    GtkWidget  *entry;
    const char *value = NULL;

    /* create text entry box */
    entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), 8);
    resources_get_string("LTKserial", &value);
    gtk_entry_set_text(GTK_ENTRY(entry), value);
    /* doesn't actually block non-digit characters, is used for screen readers
     * etc. */
    gtk_entry_set_input_purpose(GTK_ENTRY(entry), GTK_INPUT_PURPOSE_DIGITS);

    /* add event handlers */
    g_signal_connect(entry,
                     "focus-out-event",
                     G_CALLBACK(on_serial_focus_out_event),
                     NULL);
    g_signal_connect(entry, "key-press-event",
                     G_CALLBACK(on_serial_key_press_event),
                     NULL);

    /* create CSS provider to mark input invalid */
    css_provider = vice_gtk3_css_provider_new(SERIAL_INVALID_CSS);
    /* register destroy handler to unref the provider */
    g_signal_connect_unlocked(G_OBJECT(entry),
                              "destroy",
                              G_CALLBACK(on_serial_destroy),
                              NULL);
    return entry;
}

/** \brief  Add resource file choosers for HD images 0-6 to main grid
 *
 * Creates labels and resource file chooser widgets for HD images 0-6 and adds
 * them to \a grid, starting at \a row.
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to start adding widgets
 * \param[in]   columns number of columns in the grid (used to set proper span)
 *
 * \return  row for next widgets
 */
static int create_hd_images_layout(GtkWidget *grid, int row, int columns)
{
    GtkWidget *label;
    int        disk;

    /* create header */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
                         "<b>" CARTRIDGE_NAME_LT_KERNAL " HD Images</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, columns, 1);
    row++;

    for (disk = LTK_HD_MIN; disk <= LTK_HD_MAX; disk++) {

        GtkWidget *chooser;
        char       buffer[256];

        /* create label */
        g_snprintf(buffer, sizeof buffer, "HD%d image file", disk);
        label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), buffer);
        gtk_widget_set_halign(label, GTK_ALIGN_START);

        /* create file chooser */
        g_snprintf(buffer, sizeof buffer, "Select image file for HD%d", disk);
        chooser = vice_gtk3_resource_filechooser_new_sprintf("LTKimage%d",
                                                             GTK_FILE_CHOOSER_ACTION_OPEN,
                                                             disk);
        vice_gtk3_resource_filechooser_set_filter(chooser,
                                                  "HD images",
                                                  patterns,
                                                  TRUE);
        vice_gtk3_resource_filechooser_set_custom_title(chooser, buffer);

        gtk_grid_attach(GTK_GRID(grid), label,   0, row, 1,           1);
        gtk_grid_attach(GTK_GRID(grid), chooser, 1, row, columns - 1, 1);
        row++;
    }
    return row;
}


/** \brief  Create main Lt. Kernal settings widget
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_ltkernal_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *serial;
    GtkWidget *info;
    GtkWidget *io_address;
    GtkWidget *port_number;
    int        row = 0;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    row = create_hd_images_layout(grid, row, 4);

    /* add serial number */
    label  = gtk_label_new("Serial number");
    serial = create_serial_entry();
    info   = gtk_label_new("The serial number consists of 8 decimal digits.");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_halign(info,  GTK_ALIGN_START);
    gtk_widget_set_margin_top(label,  16);
    gtk_widget_set_margin_top(serial, 16);
    gtk_widget_set_margin_top(info,   16);
    gtk_grid_attach(GTK_GRID(grid), label,  0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), serial, 1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), info,   2, row, 2, 1);
    row++;

    /* add I/O address */
    label      = gtk_label_new("I/O address");
    io_address = create_io_address_widget();
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(io_address, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), label,      0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), io_address, 1, row, 1, 1);

    /* add port number */
    label       = gtk_label_new("Port number");
    port_number = create_port_number_widget();
    gtk_grid_attach(GTK_GRID(grid), label,       2, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), port_number, 3, row, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
