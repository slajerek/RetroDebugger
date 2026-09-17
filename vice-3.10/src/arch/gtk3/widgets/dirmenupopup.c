/** \file   dirmenupopup.c
 *  \brief  Create a menu to show a directory of a drive or tape deck
 *
 * FIXME: The current code depends way too much on internal/core code. The code
 *        to retrieve the current disk/tape image should be implemented
 *        somewhere in the core code, not here.
 *
 *
 *  \author Bas Wassink <b.wassink@ziggo.nl>
 *  \author Groepaz (groepaz@gmx.de>
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

#include "attach.h"
#include "autostart.h"
#include "charset.h"
#include "csshelpers.h"
#include "debug_gtk3.h"
#include "diskimage.h"
#include "diskimage/fsimage.h"
#include "drive.h"
#include "drivetypes.h"
#include "imagecontents/diskcontents.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "tape.h"
#include "tapeport.h"
#include "unicodehelpers.h"
#include "util.h"
#include "widgethelpers.h"

#include "dirmenupopup.h"



/** \brief  Function to read the contents of an image
 *
 * FIXME:   Hiding the pointer-ness of a function is NOT a good idea
 */
static read_contents_func_type content_func;

/** \brief  Function to call when a file in the directory is selected */
static void (*response_func)(const char *, int, int, unsigned int);

/** \brief  Disk image being used
 *
 * FIXME:   Somehow pass this via the event handlers
 */
static const char *autostart_diskimage;


/** \brief  CSS style string to set the CBM font and remove padding
 */
#define MENULABEL_CSS \
    "label {\n" \
    "  font-family: \"C64 Pro Mono\";\n" \
    "  font-size: 16px;\n" \
    "  min-height: 16px;\n" \
    "  letter-spacing: 0;\n" \
    "  margin: -2px;\n" \
    "  border: 0px;\n" \
    "  padding: 0px;\n" \
    "}"

/** \brief  CSS style string to remove padding from menu items
 */
#define MENUITEM_CSS \
    "menuitem {\n" \
    "  min-height: 16px;\n" \
    "  margin: 0px;\n" \
    "  border: 0px;\n" \
    "  padding: 0px;\n" \
    "}"


/** \brief  CSS provider used for directory entry GtkMenuItem labels */
static GtkCssProvider *menulabel_css_provider;

/** \brief  CSS provider used for directory entry GtkMenuItem's */
static GtkCssProvider *menuitem_css_provider;


/** \brief  Handler for the "activate" event of a menu item
 *
 * \param[in]   item    menu item triggering the event
 * \param[in]   data    index in the directory
 */
static void on_item_activate(GtkWidget *item, gpointer data)
{
    int index = GPOINTER_TO_INT(data);
    int device = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "DeviceNumber"));
    unsigned int drive = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "DriveNumber"));

    response_func(autostart_diskimage, index, device, drive);
}


/** \brief  Create reusable CSS providers
 *
 * \return bool
 */
static gboolean create_css_providers(void)
{
    menulabel_css_provider = vice_gtk3_css_provider_new(MENULABEL_CSS);
    if (menulabel_css_provider == NULL) {
        return FALSE;
    }
    menuitem_css_provider = vice_gtk3_css_provider_new(MENUITEM_CSS);
    if (menuitem_css_provider == NULL) {
        return FALSE;
    }
    return TRUE;
}


/** \brief  Apply CSS style and margins to a directory listing item
 *
 * \param[in,out]   item    direct list item
 */
static void dir_item_apply_style(GtkWidget *item)
{
    GtkWidget *label;

    gtk_widget_set_margin_top(item, 0);
    gtk_widget_set_margin_bottom(item, 0);
    label = gtk_bin_get_child(GTK_BIN(item));
    vice_gtk3_css_provider_add(label, menulabel_css_provider);
    vice_gtk3_css_provider_add(item, menuitem_css_provider);
}


/** \brief  Create a popup menu to select a file to autostart
 *
 * XXX: This is an UNHOLY MESS, and should be refactored
 *
 * \param[in]   unit        unit number (1,2 for tapes, 8-11 for drives)
 * \param[in]   drive       drive number (0 or 1 (dual-drives))
 * \param[in]   func        function to read image contents
 * \param[in]   response    function to call when an item has been selected
 *
 * \return  GtkMenu
 */
GtkWidget *dir_menu_popup_create(
        int unit,
        int drive,
        read_contents_func_type func,
        void (*response)(const char *, int, int, unsigned int))
{
    GtkWidget *menu;
    GtkWidget *item;
    char buffer[1024];
    image_contents_t *contents = NULL;
    image_contents_file_list_t *entry;
    char *utf8;
    char *tmp;
    char *sep;
    int index;
    int blocks;
    unsigned int drv = (unsigned int)drive;

    /* create style providers */
    if (!create_css_providers()) {
        return NULL;
    }

    /* set callback functions */
    content_func = func;
    response_func = response;

    /* create new menu */
    menu = gtk_menu_new();

    if (unit >= DRIVE_UNIT_MIN) {
        /*
         * The following is complete horseshit, this needs to be implemented in
         * a function in drive/vdrive somehow. This much dereferencing in UI
         * code is not normal method.
         */

        struct disk_image_s *diskimg = NULL;
        autostart_diskimage = NULL;

        diskimg = file_system_get_image(unit, drv);
        if (diskimg != NULL) {
            autostart_diskimage = diskimg->media.fsimage->name;
        }
    } else {
        int port = unit == TAPEPORT_UNIT_2 ? TAPEPORT_PORT_2 : TAPEPORT_PORT_1;

        /* tape image */
        if (tape_image_dev[port] == NULL) {
            item = gtk_menu_item_new_with_label("<<NO IMAGE ATTACHED>>");
            gtk_container_add(GTK_CONTAINER(menu), item);
            return menu;
        }
        autostart_diskimage = tape_image_dev[port]->name;
    }

    tmp = NULL;
    if (autostart_diskimage) {
        util_fname_split(autostart_diskimage, NULL, &tmp);
    }
    if (unit >= DRIVE_UNIT_MIN) {
        if (drive_is_dualdrive_by_devnr(unit)) {
            g_snprintf(buffer, sizeof(buffer),
                       "Directory of drive #%d:%u (%s):",
                       unit, drv, tmp ? tmp : "n/a");
        } else {
            g_snprintf(buffer, sizeof(buffer),
                       "Directory of drive #%d (%s):",
                       unit, tmp ? tmp : "n/a");
        }
    } else {
        if (machine_class == VICE_MACHINE_PET) {
            g_snprintf(buffer, sizeof(buffer),
                       "Directory of tape #%d (%s):",
                       unit, tmp ? tmp : "n/a");
        } else {
            g_snprintf(buffer, sizeof(buffer),
                       "Directory of tape (%s):",
                       tmp ? tmp : "n/a");
        }
    }
    item = gtk_menu_item_new_with_label(buffer);
    gtk_container_add(GTK_CONTAINER(menu), item);
    if (tmp) {
        lib_free(tmp);
    }

    if (autostart_diskimage != NULL) {
        /* read dir and add them as menu items */
        contents = content_func(autostart_diskimage);
        if (contents == NULL) {
            item = gtk_menu_item_new_with_label(
                    "Failed to read directory");
            gtk_container_add(GTK_CONTAINER(menu), item);
        } else {
            /* DISK name & ID */

            tmp = image_contents_to_string(contents, IMAGE_CONTENTS_STRING_PETSCII);

            /* only the disk name and id itself should be reverse, not the line number and space before that */
            sep = strstr(tmp, "\""); /* find start of disk name */
            if (sep) {
                /* if we found the disk name, produce seperate strings for line number and name/id,
                   reverse only name/od and then concat them */
                char *utf8a, *utf8b;
                *sep = 0;
                utf8a = (char *)vice_gtk3_petscii_upper_to_utf8((unsigned char *)tmp, 0);
                *sep = '"';
                utf8b = (char *)vice_gtk3_petscii_upper_to_utf8((unsigned char *)sep, 1);
                utf8 = util_concat(utf8a, utf8b, NULL);
                lib_free(utf8a);
                lib_free(utf8b);
            } else {
                /* if start of disk name was not found use the entire string */
                utf8 = (char *)vice_gtk3_petscii_upper_to_utf8((unsigned char *)tmp, 1);
            }

            item = gtk_menu_item_new_with_label(utf8);

            dir_item_apply_style(item);

            gtk_container_add(GTK_CONTAINER(menu), item);
            lib_free(tmp);
            lib_free(utf8);
#if 0
            /* add separator */
            item = gtk_separator_menu_item_new();
            gtk_container_add(GTK_CONTAINER(menu), item);
#endif
            /* add files */
            index = 0;
            for (entry = contents->file_list; entry != NULL;
                    entry = entry->next) {

                tmp = image_contents_file_to_string(entry, IMAGE_CONTENTS_STRING_PETSCII);
                utf8 = (char *)vice_gtk3_petscii_upper_to_utf8((unsigned char *)tmp, 0);
                item = gtk_menu_item_new_with_label(utf8);
                /* set extra data to used in the event handler */
                g_object_set_data(G_OBJECT(item),
                                  "DeviceNumber",
                                  GINT_TO_POINTER(unit - DRIVE_UNIT_MIN));
                g_object_set_data(G_OBJECT(item),
                                  "DriveNumber",
                                  GUINT_TO_POINTER(drv));

                dir_item_apply_style(item);

                gtk_container_add(GTK_CONTAINER(menu), item);
                g_signal_connect(item, "activate",
                        G_CALLBACK(on_item_activate),
                        GINT_TO_POINTER(index));
                index++;
                lib_free(tmp);
                lib_free(utf8);
            }

            /* add BLOCKS FREE. */
            blocks = contents->blocks_free;
            if (blocks >= 0) {
                tmp = lib_msprintf("%d BLOCKS FREE.", contents->blocks_free);
                item = gtk_menu_item_new_with_label(tmp);

                /* move this into separate function: */
                dir_item_apply_style(item);
                gtk_container_add(GTK_CONTAINER(menu), item);
                lib_free(tmp);
            }
        }
        if (contents != NULL) {
            image_contents_destroy(contents);
        }
    } else {
        item = gtk_menu_item_new_with_label("<<NO IMAGE ATTACHED>>");
        gtk_container_add(GTK_CONTAINER(menu), item);
    }
    gtk_widget_show_all(GTK_WIDGET(menu));

    return menu;
}
