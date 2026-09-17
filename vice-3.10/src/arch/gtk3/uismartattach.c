/** \file   uismartattach.c
 * \brief   GTK3 smart-attach dialog
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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

/*
 * $VICERES AttachDevice8d0Readonly   -vsid
 */


#include "vice.h"

#include <gtk/gtk.h>

#include "vice_gtk3.h"
#include "attach.h"
#include "autostart.h"
#include "cartridge.h"
#include "drive.h"
#include "tape.h"
#include "debug_gtk3.h"
#include "contentpreviewwidget.h"
#include "diskcontents.h"
#include "diskimage.h"
#include "driveimage.h"
#include "tapecontents.h"
#include "machine.h"
#include "mainlock.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uiapi.h"
#include "uimachinewindow.h"
#include "lastdir.h"
#include "initcmdline.h"
#include "log.h"
#include "tapecart.h"
#include "tapeport.h"

#include "uismartattach.h"

/* #define HAVE_DEBUG_GTK3UI */

/** \brief  File type filters for the dialog
 */
static ui_file_filter_t filters[] = {
    { "All files",          file_chooser_pattern_all },
    { "Disk images",        file_chooser_pattern_disk },
    { "Tape images",        file_chooser_pattern_tape },
    { "Cartridge images",   file_chooser_pattern_cart },
    { "Program files",      file_chooser_pattern_program },
    { "Snapshot files",     file_chooser_pattern_snapshot },
    { "Archives files",     file_chooser_pattern_archive },
    { "Compressed files",   file_chooser_pattern_compressed },
    { NULL, NULL }
};


/** \brief  Preview widget reference
 */
static GtkWidget *preview_widget = NULL;


/** \brief  Last directory used
 *
 * When an image is attached, this is set to the directory of that file.
 *
 * Since it's heap-allocated by Gtk3, it must be freed with a call to
 * ui_smart_attach_shutdown() on emulator shutdown.
 */
static gchar *last_dir = NULL;

/** \brief  Last file selected
 */
static gchar *last_file = NULL;


/** \brief  Reference to the custom 'Autostart' button
 */
static GtkWidget *autostart_button;


/** \brief  Trigger autostart
 *
 * \param[in]   dialog      dialog
 * \param[in]   index       file index in the directory preview
 * \param[in]   autostart   flag: 0: just load, 1: autostart
 */
static void do_autostart(GtkWidget *dialog, int index, int autostart)
{
    gchar *filename;
    gchar *filename_locale;

    lastdir_update(dialog, &last_dir, &last_file);
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    filename_locale = file_chooser_convert_to_locale(filename);

    /* if this function exists, why is there no attach_autodetect()
     * or something similar? -- compyx */
    if (autostart_autodetect(
                filename_locale,
                NULL,   /* program name */
                index,  /* Program number? Probably used when clicking
                           in the preview widget to load the proper
                           file in an image */
                autostart ? AUTOSTART_MODE_RUN : AUTOSTART_MODE_LOAD) < 0) {
        vice_gtk3_message_error(GTK_WINDOW(dialog),
                                "Autodetect error",
                                "Failed to smart attach '%s'",
                                filename_locale);
    }
    g_free(filename);
    g_free(filename_locale);
}

/** \brief  try attach a disk image, change drive type if needed
 *
 * \param[in]   unit_number
 * \param[in]   drive_number
 * \param[in]   filename_locale
 */
static int try_attach_disk(int unit_number, int drive_number, char *filename_locale)
{

    if (file_system_attach_disk(unit_number, drive_number, filename_locale) < 0) {
        /* failed */
        return -1;
    } else {
        /* shitty code, we really need to extend the drive API to
        * get at these sorts for things without breaking into core code
        */
        struct disk_image_s *diskimg = file_system_get_image(unit_number, drive_number);

        if (diskimg == NULL) {
            log_error(LOG_DEFAULT, "Failed to get disk image for unit %d.", unit_number);
            return -1;
        } else {
            int chk = drive_check_image_format(diskimg->type, 0);
            log_message(LOG_DEFAULT, "mounted image is type: %u, %schanging drive.",
                        diskimg->type, (chk < 0) ? "" : "not ");
            /* change drive type only when image does not work in current drive */
            if (chk < 0) {
                if (resources_set_int_sprintf("Drive%dType", drive_image_type_to_drive_type(diskimg->type), unit_number) < 0) {
                    log_error(LOG_DEFAULT, "Failed to set drive type.");
                }
            }

            /* detach disk before reattaching */
            file_system_detach_disk(unit_number, drive_number);

            if (file_system_attach_disk(unit_number, drive_number, filename_locale) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

/** \brief  try attach a Tapecart image, change tape port device if needed
 *
 * \param[in]   filename
 */
static int try_attach_tapecart(char *filename)
{
    int tapedevice_temp = TAPEPORT_DEVICE_NONE;

    /* check if file is a valid Tapecart */
    if (!tapecart_is_valid(filename)) {
        return -1;
    }

    /* get current tapeport device */
    if (resources_get_int("TapePort1Device", &tapedevice_temp) < 0) {
        log_error(LOG_DEFAULT, "Failed to get tape port device.");
        return -1;
    }

    /* first disable all devices, so we dont get any conflicts */
    if (resources_set_int("TapePort1Device", TAPEPORT_DEVICE_NONE) < 0) {
        log_error(LOG_DEFAULT, "Failed to disable the tape port device.");
        goto exiterror;
    }

    /* enable the tape cart */
    if (resources_set_int("TapePort1Device", TAPEPORT_DEVICE_TAPECART) < 0) {
        log_error(LOG_DEFAULT, "Failed to enable the Tapecart.");
        goto exiterror;
    }

    /* attach image and return on success */
    if (tapecart_attach_tcrt(filename, NULL) == 0) {
        return 0;
    }

exiterror:
    /* restore tape port device */
    if (resources_set_int("TapePort1Device", tapedevice_temp) < 0) {
        log_error(LOG_DEFAULT, "Failed to restore tape port device.");
    }
    return -1;
}

/** \brief  Do smart attach
 *
 * \param[in]   widget  dialog
 * \param[in]   data    file index in the directory preview
 */
static void do_smart_attach(GtkWidget *widget, gpointer data)
{
    gchar *filename_locale;
    gchar *filename;

    lastdir_update(widget, &last_dir, &last_file);
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    filename_locale = file_chooser_convert_to_locale(filename);

    /* Smart attach for C64/C128/Plus4
        *
        * This tries to attach a file as a cartridge image, which is only
        * valid for C64/C128/Plus4
        */
    if ((machine_class == VICE_MACHINE_C64)
            || (machine_class == VICE_MACHINE_C64SC)
            || (machine_class == VICE_MACHINE_SCPU64)
            || (machine_class == VICE_MACHINE_C128)
            || (machine_class == VICE_MACHINE_PLUS4)) {
        if (try_attach_disk(DRIVE_UNIT_DEFAULT, 0, filename_locale) < 0
                && tape_image_attach(1, filename_locale) < 0
                && autostart_snapshot(filename_locale, NULL) < 0
                && try_attach_tapecart(filename_locale) < 0
                && cartridge_attach_image(CARTRIDGE_CRT, filename_locale) < 0
                && autostart_prg(filename_locale, AUTOSTART_MODE_LOAD) < 0) {
            /* failed */
            log_error(LOG_DEFAULT, "smart attach failed for '%s' failed", filename);
        }
    } else if ((machine_class == VICE_MACHINE_VIC20)
            || (machine_class == VICE_MACHINE_CBM5x0)
            || (machine_class == VICE_MACHINE_CBM6x0)) {
        if (try_attach_disk(DRIVE_UNIT_DEFAULT, 0, filename_locale) < 0
                && tape_image_attach(1, filename_locale) < 0
                && autostart_snapshot(filename_locale, NULL) < 0
                /* && autostart_prg(filename_locale, AUTOSTART_MODE_LOAD) < 0 */
                && cartridge_attach_image(CARTRIDGE_CRT, filename_locale) < 0) {
            /* failed */
            log_error(LOG_DEFAULT, "smart attach failed for '%s' failed", filename);
        }
    } else {
        /* Smart attach for other emulators: don't try to attach a file
            * as a cartidge, it'll result in false positives
            */
        if (try_attach_disk(DRIVE_UNIT_DEFAULT, 0, filename_locale) < 0
                && tape_image_attach(1, filename_locale) < 0
                && autostart_snapshot(filename_locale, NULL) < 0)
        {
            log_error(LOG_DEFAULT, "Failed to smart attach '%s'",
                    filename_locale);
        }
    }
    g_free(filename_locale);
}


/** \brief  Handler for 'selection-changed' event of the preview widget
 *
 * Checks if a proper selection was made and activates the 'Autostart' button
 * if so, disabled it otherwise
 *
 * \param[in]   chooser Parent dialog
 * \param[in]   data    Extra event data (unused)
 */
static void on_selection_changed(GtkFileChooser *chooser, gpointer data)
{
    gchar *filename;

    filename = gtk_file_chooser_get_filename(chooser);
    if (filename != NULL) {
        gtk_widget_set_sensitive(autostart_button, TRUE);
        g_free(filename);
    } else {
        gtk_widget_set_sensitive(autostart_button, FALSE);
    }
}


/** \brief  Handler for the "update-preview" event
 *
 * \param[in]   chooser file chooser dialog
 * \param[in]   data    extra event data (unused)
 */
static void on_update_preview(GtkFileChooser *chooser, gpointer data)
{
    GFile *file;
    gchar *path;

    file = gtk_file_chooser_get_preview_file(chooser);
    if (file != NULL) {
        path = g_file_get_path(file);
        if (path != NULL) {
            gchar *filename_locale = file_chooser_convert_to_locale(path);

            content_preview_widget_set_image(preview_widget, filename_locale);
            g_free(path);
            g_free(filename_locale);
        }
        g_object_unref(file);
    }
}


/** \brief  Handler for the 'toggled' event of the 'show hidden files' checkbox
 *
 * \param[in]   widget      checkbox triggering the event
 * \param[in]   user_data   data for the event (the dialog)
 */
static void on_hidden_toggled(GtkWidget *widget, gpointer user_data)
{
    int state;

    state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(user_data), state);
}


/** \brief  Handler for the 'toggled' event of the 'attach read-only' checkbox
 *
 * \param[in]   widget      checkbox triggering the event
 * \param[in]   user_data   data for the event (the dialog)
 */
static void on_readonly_toggled(GtkWidget *widget, gpointer user_data)
{
    int state;

    state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    resources_set_int_sprintf("AttachDevice%dd0Readonly", state, DRIVE_UNIT_DEFAULT);
}


/** \brief  Handler for 'response' event of the dialog
 *
 * This handler is called when the user clicks a button in the dialog.
 *
 * \param[in]   widget      the dialog
 * \param[in]   response_id response ID
 * \param[in]   user_data   unit number
 *
 * TODO:    proper (error) messages
 */
static void on_response(GtkWidget *widget, gint response_id, gpointer user_data)
{
    gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    int index = content_preview_widget_get_index(preview_widget);
    int autostart = 0;

    resources_get_int("AutostartOnDoubleClick", &autostart);

    /* first, to make the following logic less funky, map some events to others,
       depending on whether autostart-on-doubleclick is enabled or not, and
       depending on the event coming from the preview window or not. */
    switch (response_id) {
        /* double-click on file in the preview widget when autostart-on-doubleclick is NOT enabled */
        case VICE_RESPONSE_AUTOLOAD_INDEX:
        /* double-click on file in the preview widget when autostart-on-doubleclick is enabled */
        case VICE_RESPONSE_AUTOSTART_INDEX:
            if ((index < 0) || (filename == NULL)) {
                response_id = VICE_RESPONSE_INVALID;   /* drop this event */
            }
            break;
        /* 'Open' button clicked when autostart-on-doubleclick is enabled */
        case VICE_RESPONSE_CUSTOM_OPEN:
            if (filename == NULL) {
                response_id = VICE_RESPONSE_INVALID;   /* drop this event */
            } else if (index >= 0) {
                response_id = VICE_RESPONSE_AUTOLOAD_INDEX;
            }
            break;
        /* double-click on file in the main file chooser,
           'Autostart' button when autostart-on-doubleclick is enabled
           'Open' button when autostart-on-doubleclick is NOT enabled */
        case GTK_RESPONSE_ACCEPT:
            if (filename == NULL) {
                response_id = VICE_RESPONSE_INVALID;   /* drop this event */
            } else if ((index >= 0) && (autostart == 0)) {
                response_id = VICE_RESPONSE_AUTOLOAD_INDEX;
            } else if ((index >= 0) && (autostart == 1)) {
                response_id = VICE_RESPONSE_AUTOSTART_INDEX;
            } else if (autostart == 0) {
                response_id = VICE_RESPONSE_CUSTOM_OPEN;
            } else {
                response_id = VICE_RESPONSE_AUTOSTART;
            }
            break;
        default:
            break;
    }

    switch (response_id) {
        /* 'Open' button clicked when autostart-on-doubleclick is enabled */
        case VICE_RESPONSE_CUSTOM_OPEN:
            do_smart_attach(widget, user_data);

            mainlock_release();
            gtk_widget_destroy(widget);
            mainlock_obtain();
            break;

        /* 'Autostart' button clicked when autostart-on-doubleclick is NOT enabled */
        case VICE_RESPONSE_AUTOSTART:
        /* double-click on file in the preview widget when autostart-on-doubleclick is enabled */
        case VICE_RESPONSE_AUTOSTART_INDEX:
            do_autostart(widget, index + 1, 1);

            mainlock_release();
            gtk_widget_destroy(widget);
            mainlock_obtain();
            break;

        /* double-click on file in the preview widget when autostart-on-doubleclick is NOT enabled */
        case VICE_RESPONSE_AUTOLOAD_INDEX:
            do_autostart(widget, index + 1, 0);

            mainlock_release();
            gtk_widget_destroy(widget);
            mainlock_obtain();
            break;

        /* 'Close'/'X' button */
        case GTK_RESPONSE_REJECT:
            mainlock_release();
            gtk_widget_destroy(widget);
            mainlock_obtain();
            break;
        default:
            break;
    }

    if (filename != NULL) {
        g_free(filename);
    }
}


/** \brief  Create the 'extra' widget
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
static GtkWidget *create_extra_widget(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *hidden_check;
    GtkWidget *readonly_check;
    int readonly_state;

    grid = vice_gtk3_grid_new_spaced(0, 8);

    hidden_check = gtk_check_button_new_with_label("Show hidden files");
    g_signal_connect(hidden_check, "toggled", G_CALLBACK(on_hidden_toggled),
            (gpointer)(parent));
    gtk_grid_attach(GTK_GRID(grid), hidden_check, 0, 0, 1, 1);

    readonly_check = gtk_check_button_new_with_label("Attach read-only");
    g_signal_connect(readonly_check, "toggled", G_CALLBACK(on_readonly_toggled),
            (gpointer)(parent));
    gtk_grid_attach(GTK_GRID(grid), readonly_check, 1, 0, 1, 1);
    resources_get_int_sprintf("AttachDevice%dd0Readonly", &readonly_state, DRIVE_UNIT_DEFAULT);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(readonly_check), readonly_state);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Wrapper around disk/tape contents readers
 *
 * First treats \a path as disk image file and when that fails it falls back
 * to treating \a path as a tape image, when that fails as well, it gives up.
 *
 * \param[in]   path    path to image file
 *
 * \return  image contents or `NULL` on failure
 */
static image_contents_t *read_contents_wrapper(const char *path)
{
    image_contents_t *content;

    /* try disk contents first */
    content = diskcontents_filesystem_read(path);
    if (content == NULL) {
        /* fall back to tape */
        content = tapecontents_read(path);
    }
    return content;
}



static void on_destroy(GtkWidget *self, gpointer data)
{
    ui_action_finish(ACTION_SMART_ATTACH);
}


/** \brief  Create the smart-attach dialog
 *
 * \return  GtkFileChooserDialog
 *
 * \todo    Figure out how to only enable the 'Autostart' button when an actual
 *          file/image has been selected. And when I do, make sure it's somehow
 *          reusable for other 'open file' dialogs'.
 */
static GtkWidget *create_smart_attach_dialog(void)
{
    GtkWidget *dialog;
    size_t i;
    int autostart = 0;

    resources_get_int("AutostartOnDoubleClick", &autostart);

    /* create new dialog */
    dialog = gtk_file_chooser_dialog_new(
            "Smart-attach a file",
            ui_get_active_window(),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            /* buttons */
            NULL, NULL);

    /* We need to manually add the buttons here, not using the constructor
     * above, to keep the order of the buttons the same as in the other
     * 'attach' dialogs. Gtk doesn't allow getting references to buttons when
     * added via the constructor, meaning we cannot get a reference to the
     * "Autostart" button in order to "grey it out".
     */
    /* to handle the "double click means autostart" option, we need to always
       connect a button to GTK_RESPONSE_ACCEPT, else double clicks will stop
       working */
    if (autostart) {
        gtk_dialog_add_button(GTK_DIALOG(dialog), "Attach / Load", VICE_RESPONSE_CUSTOM_OPEN);
        autostart_button = gtk_dialog_add_button(GTK_DIALOG(dialog),
                                                "Autostart",
                                                GTK_RESPONSE_ACCEPT);
    } else {
        gtk_dialog_add_button(GTK_DIALOG(dialog), "Attach / Load", GTK_RESPONSE_ACCEPT);
        autostart_button = gtk_dialog_add_button(GTK_DIALOG(dialog),
                                                "Autostart",
                                                VICE_RESPONSE_AUTOSTART);
    }

    gtk_widget_set_sensitive(autostart_button, FALSE);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Close", GTK_RESPONSE_REJECT);

    /* set modal so mouse-grab doesn't get triggered */
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    /* set last used directory */
    lastdir_set(dialog, &last_dir, &last_file);

    /* add 'extra' widget: 'readony' and 'show preview' checkboxes */
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog),
            create_extra_widget(dialog));

    preview_widget = content_preview_widget_create(dialog,
            read_contents_wrapper, on_response, 0);
    gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(dialog),
            preview_widget);

    /* add filters */
    for (i = 0; filters[i].name != NULL; i++) {
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),
                create_file_chooser_filter(filters[i], FALSE));
    }

    /* connect "reponse" handler: the `user_data` argument gets filled in when
     * the "response" signal is emitted: a response ID */
    g_signal_connect(dialog, "response", G_CALLBACK(on_response), NULL);
    g_signal_connect_unlocked(dialog, "update-preview",
            G_CALLBACK(on_update_preview), NULL);
    g_signal_connect_unlocked(dialog, "selection-changed",
            G_CALLBACK(on_selection_changed), NULL);
    g_signal_connect(dialog, "destroy", G_CALLBACK(on_destroy), NULL);

    return dialog;

}


/** \brief  Callback for the File menu's "smart-attach" item
 *
 * Creates the smart-dialog and runs it.
 *
 * \param[in]   widget      menu item triggering the callback
 * \param[in]   user_data   data for the event (unused)
 *
 * \return  TRUE
 */
void ui_smart_attach_dialog_show(void)
{
    GtkWidget *dialog = create_smart_attach_dialog();
    gtk_widget_show(dialog);
}


/** \brief  Clean up last used directory string
 */
void ui_smart_attach_shutdown(void)
{
    lastdir_shutdown(&last_dir, &last_file);
}
