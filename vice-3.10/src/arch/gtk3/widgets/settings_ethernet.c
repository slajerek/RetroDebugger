/** \file   settings_ethernet.c
 * \brief   GTK3 ethernet settings widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES ETHERNET_DRIVER     x64 x64sc xscpu64 x128 xvic
 * $VICERES ETHERNET_INTERFACE  x64 x64sc xscpu64 x128 xvic
 *
 * read only, set internally:
 * $VICERES ETHERNET_DISABLED   x64 x64sc xscpu64 x128 xvic
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

#include "archdep.h"
#include "lib.h"
#include "machine.h"
#ifdef HAVE_RAWNET
# include "rawnet.h"
#endif
#include "vice_gtk3.h"

#include "settings_ethernet.h"


#ifdef HAVE_RAWNET
/** \brief  Create combo box with ethernet drivers
 *
 * \return  GtkComboBox
 */
static GtkWidget *create_driver_combo(void)
{
    GtkWidget *combo;

    /* create resource combo with empty model */
    combo = vice_gtk3_resource_combo_str_new("ETHERNET_DRIVER", NULL);
    if (!rawnet_enumdriver_open()) {
        gtk_widget_set_sensitive(combo, FALSE);
    } else {
        char *name;
        char *desc;

        while (rawnet_enumdriver(&name, &desc)) {
            if (desc != NULL) {
                char buffer[1024];

                g_snprintf(buffer, sizeof buffer, "%s (%s)", name, desc);
                lib_free(desc);
                vice_gtk3_resource_combo_str_append(combo, name, buffer);
            } else {
                vice_gtk3_resource_combo_str_append(combo, name, name);
            }
            lib_free(name);
        }

        rawnet_enumdriver_close();
        vice_gtk3_resource_combo_str_sync(combo);
    }
    return combo;
}

/** \brief  Create combo box to select the ethernet interface
 *
 * \return  GtkComboBoxText
 */
static GtkWidget *create_device_combo(void)
{
    GtkWidget *combo;

    /* create resource combo with empty model */
    combo = vice_gtk3_resource_combo_str_new("ETHERNET_INTERFACE", NULL);
    if (!rawnet_enumadapter_open()) {
        gtk_widget_set_sensitive(combo, FALSE);
    } else {
        char *name;
        char *desc;

        while (rawnet_enumadapter(&name, &desc)) {
            if (desc != NULL) {
                char buffer[1024];

                g_snprintf(buffer, sizeof buffer, "%s (%s)", name, desc);
                lib_free(desc);
                vice_gtk3_resource_combo_str_append(combo, name, buffer);
            } else {
                vice_gtk3_resource_combo_str_append(combo, name, name);
            }
            lib_free(name);
        }

        rawnet_enumadapter_close();
        /* since we're manually adding rows we need to sync the widget with
         * its resource */
        vice_gtk3_resource_combo_str_sync(combo);
    }
    return combo;
}
#endif


/** \brief  Create Ethernet settings widget for the settings UI
 *
 * \param[in]   parent  parent widget (ignored)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_ethernet_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;
    char       text[1024];
#ifdef HAVE_RAWNET
    GtkWidget *iface_label;
    GtkWidget *iface_combo;
    GtkWidget *driver_label;
    GtkWidget *driver_combo;
#endif

    grid = vice_gtk3_grid_new_spaced(8, 8);

    switch (machine_class) {
        case VICE_MACHINE_C64DTV:   /* fall through */
        case VICE_MACHINE_PLUS4:    /* fall through */
        case VICE_MACHINE_PET:      /* fall through */
        case VICE_MACHINE_CBM5x0:   /* fall through */
        case VICE_MACHINE_CBM6x0:   /* fall through */
        case VICE_MACHINE_VSID:

            g_snprintf(text, sizeof text,
                       "<b>Error</b>: Ethernet not supported for <b>%s</b>, "
                       "please fix the code that calls this code!",
                        machine_name);
            label = gtk_label_new(NULL);
            gtk_label_set_markup(GTK_LABEL(label), text);
            gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
            gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
            gtk_widget_show_all(grid);
            return grid;
        default:
            break;
    }

#ifdef HAVE_RAWNET
    driver_label = gtk_label_new("Ethernet driver:");
    driver_combo = create_driver_combo();
    gtk_widget_set_halign(driver_label, GTK_ALIGN_START);

    iface_label = gtk_label_new("Ethernet interface:");
    iface_combo = create_device_combo();
    gtk_widget_set_halign(iface_label, GTK_ALIGN_START);

    gtk_grid_attach(GTK_GRID(grid), driver_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), driver_combo, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), iface_label,  0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), iface_combo,  1, 1, 1, 1);

    if (!archdep_ethernet_available()) {
        gtk_widget_set_sensitive(driver_combo, FALSE);
        gtk_widget_set_sensitive(iface_combo, FALSE);
        label = gtk_label_new(NULL);
# ifdef UNIX_COMPILE
        gtk_label_set_markup(GTK_LABEL(label),
                "<i>VICE needs TUN/TAP support or the proper permissions"
                " (with libpcap) to be able to use ethernet emulation.</i>");
# elif defined(WINDOWS_COMPILE)
        gtk_label_set_markup(GTK_LABEL(label),
                "<i>Couldn't load <b>wpcap.dll</b>, please install WinPCAP"
                " to use ethernet emulation.</i>");
# else
        gtk_label_set_markup(GTK_LABEL(label),
                "<i>Ethernet emulation disabled due to unsupported OS.</i>");
# endif
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 2, 1);
    }
#else
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
            "Ethernet not supported, please compile with <tt>--enable-ethernet</tt>.");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
#endif
    gtk_widget_show_all(grid);
    return grid;
}
