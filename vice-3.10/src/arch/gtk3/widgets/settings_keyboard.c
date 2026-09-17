/** \file   settings_keyboard.c
 * \brief   GTK3 keyboard settings main widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES KbdStatusbar    all
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
#include <stdlib.h>
#include <errno.h>
#include <string.h>


#include "kbdhostlayoutwidget.h"
#include "kbdmappingwidget.h"
#include "keyboard.h"
#include "keymap.h"
#include "lastdir.h"
#include "lib.h"
#include "resources.h"
#include "uistatusbar.h"
#include "util.h"
#include "vice_gtk3.h"

#include "settings_keyboard.h"


/** \brief  Last directory of the save-as dialog */
static char *last_dir = NULL;

/** \brief  Last filename of the save-as dialog */
static char *last_file = NULL;


/** \brief  Toggle statusbar keyboard debugging widget display
 *
 *
 * \param[in]   widget  toggle button
 * \param[in]   data    extra event data (unused)
 */
static void on_kbd_debug_toggled(GtkWidget *widget, gpointer data)
{
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    ui_statusbar_set_kbd_debug(active);
}

/** \brief  Callback for the save-dialog response handler
 *
 * \param[in,out]   dialog      save-file dialog
 * \param[in,out]   filename    filename
 * \param[in]       data        extra data (unused)
 */
static void save_filename_callback(GtkDialog *dialog,
                                   gchar     *filename,
                                   gpointer   data)
{
    if (filename != NULL) {
        char *path;
        int   result;

        /* `filename` is owned by GLib */
        path = util_add_extension_const(filename, "vkm");
        g_free(filename);

        result = keyboard_keymap_dump(path);
        if (result == 0) {
            vice_gtk3_message_info(GTK_WINDOW(dialog),
                                   "Succesfully saved current keymap",
                                   "Wrote current keymap as '%s'.", path);
            lastdir_update(GTK_WIDGET(dialog), &last_dir, &last_file);
        } else {
            vice_gtk3_message_error(GTK_WINDOW(dialog),
                                    "Failed to save custom keymap",
                                    "Error %d: %s",
                                    errno, strerror(errno));
        }
        lib_free(path);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/** \brief  Write current keymap to host system
 *
 * \param[in]   widget  button triggering the event (ignored)
 * \param[in]   data    extra event data (ignored)
 */
static void on_save_custom_keymap_clicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;

    dialog = vice_gtk3_save_file_dialog(
            "Save current keymap",  /* title */
            NULL,                   /* proposed filename: might use this later */
            TRUE,                   /* query user before overwrite */
            NULL,                   /* base path, maybe ~ ?) */
            save_filename_callback, /* filename callback */
            NULL                    /* extra data */
         );
    lastdir_set(dialog, &last_dir, &last_file);
    gtk_widget_show(dialog);
}


/** \brief  Create keyboard settings widget
 *
 * \param[in]   widget  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_keyboard_widget_create(GtkWidget *widget)
{
    GtkWidget *grid;
    GtkWidget *mapping;
    GtkWidget *layout;
    GtkWidget *status;
    GtkWidget *save;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    mapping = kbdmapping_widget_create(widget);

    /* Add button to save current (perhaps altered) keymap */
    save = gtk_button_new_with_label("Save current keymap to file...");
    gtk_widget_set_hexpand(save, FALSE);
    gtk_widget_set_halign(save, GTK_ALIGN_END);
    gtk_widget_set_margin_top(save, 8);
    g_signal_connect(G_OBJECT(save),
                     "clicked",
                     G_CALLBACK(on_save_custom_keymap_clicked),
                     NULL);

    layout = kbdhostlayout_widget_create();
    gtk_widget_set_margin_top(layout, 16);

    status = vice_gtk3_resource_check_button_new("KbdStatusbar",
                                                 "Enable keyboard debugging on statusbar");
    gtk_widget_set_margin_top(status, 16);
    g_signal_connect(G_OBJECT(status),
                     "toggled",
                     G_CALLBACK(on_kbd_debug_toggled),
                     NULL);

    gtk_grid_attach(GTK_GRID(grid), mapping, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), save,    0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), layout,  0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), status,  0, 3, 1, 1);
    gtk_widget_show_all(grid);

    /* update widget so sym/pos is greyed out correctly */
    kbdmapping_widget_update();

    return grid;
}


/** \brief  Free last used directory and file strings
 *
 * This function should be called on emulator shutdown to clean up the resources
 * used by the last used directory/file strings.
 */
void settings_keyboard_widget_shutdown(void)
{
    lastdir_shutdown(&last_dir, &last_file);
}
