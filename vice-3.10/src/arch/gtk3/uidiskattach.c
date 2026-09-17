/** \file   uidiskattach.c
 * \brief   GTK3 disk-attach dialog
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
 * $VICERES AttachDevice9d0Readonly   -vsid
 * $VICERES AttachDevice10d0Readonly  -vsid
 * $VICERES AttachDevice11d0Readonly  -vsid
 * $VICERES AttachDevice8d1Readonly   -vsid
 * $VICERES AttachDevice9d1Readonly   -vsid
 * $VICERES AttachDevice10d1Readonly  -vsid
 * $VICERES AttachDevice11d1Readonly  -vsid
 */

#include "vice.h"

#include <gtk/gtk.h>

#include "attach.h"
#include "autostart.h"
#include "basedialogs.h"
#include "contentpreviewwidget.h"
#include "debug_gtk3.h"
#include "diskcontents.h"
#include "drive.h"
#include "drivenowidget.h"
#include "driveunitwidget.h"
#include "filechooserhelpers.h"
#include "imagecontents.h"
#include "lastdir.h"
#include "log.h"
#include "mainlock.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uiapi.h"

#include "uidiskattach.h"


/** \brief  File type filters for the dialog
 */
static ui_file_filter_t filters[] = {
    { "Disk images",        file_chooser_pattern_disk },
    { "Compressed files",   file_chooser_pattern_compressed },
    { "All files",          file_chooser_pattern_all },
    { NULL, NULL }
};

/** \brief  Array to keep track of drive numbers for each unit
 */
static int unit_drive_nums[NUM_DISK_UNITS] = { 0, 0, 0, 0};

/** \brief  Preview widget reference
 */
static GtkWidget *preview_widget = NULL;

/** \brief  Drive number widget reference
 */
static GtkWidget *driveno_widget = NULL;


/** \brief  Last directory used
 *
 * When a disk is attached, this is set to the directory of that file. Since
 * it's heap-allocated by Gtk3, it must be freed with a call to
 * ui_disk_attach_shutdown() on emulator shutdown.
 */
static gchar *last_dir = NULL;

/** \brief  Last filename used */
static gchar *last_file = NULL;

/** \brief  Reference to the custom 'Autostart' button
 */
static GtkWidget *autostart_button;


/** \brief  Unit number to attach disk to
 */
static int unit_number = DRIVE_UNIT_DEFAULT;

/** \brief  Drive number in unit to attach disk to
 */
static int drive_number = 0;



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
    resources_set_int_sprintf("AttachDevice%dd%dReadonly", state, unit_number, drive_number);
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


/** \brief  Callback for the drive unit widget
 *
 * Updates the drive number widget, depending on the drive unit's type:
 * 'greys-out' the drive number widget and sets drive number to 0 if the current
 * unit doesn't support dual drives.
 *
 * \param[in]   unit    drive unit number
 */
static void on_unit_changed(int unit)
{
    int dual = drive_is_dualdrive_by_devnr(unit);

    gtk_widget_set_sensitive(driveno_widget, dual);
    drive_no_widget_update(driveno_widget,
                           unit_drive_nums[unit - DRIVE_UNIT_MIN]);
}


/** \brief  Callback for the drive number widget
 *
 * Update drive number for the current unit, to later be used to restore the
 * drive number widget when changing units.
 *
 * \param[in]   drive   drive number
 */
static void on_drive_num_changed(int drive)
{
    unit_drive_nums[unit_number - DRIVE_UNIT_MIN] = drive;

}


/** \brief  Trigger autostarting a disk image
 *
 * \param[in,out]   widget      dialog
 * \param[in]       index       file index in the image's directory listing
 * \param[in]       autostart   issue "RUN:" after loading
 */
static void do_autostart(GtkWidget *widget, int index, int autostart)
{
    gchar *filename;
    gchar *filename_locale;

    lastdir_update(widget, &last_dir, &last_file);
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    /* convert filename to current locale */
    filename_locale = file_chooser_convert_to_locale(filename);

    /* if this function exists, why is there no attach_autodetect()
     * or something similar? -- compyx */
    if (autostart_disk(
                unit_number,
                drive_number,
                filename_locale,
                NULL,   /* program name */
                index,  /* Program number? Probably used when clicking
                           in the preview widget to load the proper
                           file in an image */
                autostart ? AUTOSTART_MODE_RUN : AUTOSTART_MODE_LOAD) < 0) {
        /* oeps */
        log_error(LOG_DEFAULT, "autostart disk attach failed.");
        vice_gtk3_message_error(GTK_WINDOW(widget),
                                "Autostart failure",
                                "Autostarting disk image '%s' failed.",
                                filename);
    }
    g_free(filename);
    g_free(filename_locale);
}


/** \brief  Attach a disk image
 *
 * \param[in,out]   widget      dialog
 * \param[in]       user_data   file index in the image
 */
static void do_attach(GtkWidget *widget, gpointer user_data)
{
    gchar *filename;
    gchar *filename_locale;
    gchar buffer[1024];

    lastdir_update(widget, &last_dir, &last_file);

    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    /* convert filename to current locale */
    filename_locale = file_chooser_convert_to_locale(filename);

    /* copied from Gtk2: I fail to see how brute-forcing your way
        * through file types is 'smart', but hell, it works */
    if (file_system_attach_disk(unit_number, drive_number, filename_locale) < 0) {
        /* failed */
        g_snprintf(buffer, sizeof buffer,
                   "Unit #%d: failed to attach '%s'",
                   unit_number, filename);
    } else {
        g_snprintf(buffer, sizeof buffer,
                   "Unit #%d: attached '%s'",
                   unit_number, filename);
    }
    ui_display_statustext(buffer, true);
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

/** \brief  Handler for the 'destroy' event of the dialog
 *
 * \param[in]   self    dialog
 * \param[in]   data    extra event data (unit+drive)
 */
static void on_destroy(GtkWidget *self, gpointer data)
{
    int unit = GPOINTER_TO_INT(data) >> 8;
    int drive = GPOINTER_TO_INT(data) & 0xff;

    ui_action_finish(ui_action_id_drive_attach(unit, drive));
}


/** \brief  Handler for 'response' event of the dialog
 *
 * This handler is called when the user clicks a button in the dialog.
 *
 * \param[in]   widget      the dialog
 * \param[in]   response_id response ID
 * \param[in]   user_data   unit number
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
            do_attach(widget, user_data);

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
 * \param[in]   parent  parent widget
 * \param[in]   unit    unit number
 * \param[in]   drive   drive number
 *
 * \return  GtkGrid
 *
 * TODO: 'grey-out'/disable units without a proper drive attached
 */
static GtkWidget *create_extra_widget(GtkWidget *parent, int unit, int drive)
{
    GtkWidget *grid;
    GtkWidget *hidden_check;
    GtkWidget *readonly_check;
    int readonly_state;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

    hidden_check = gtk_check_button_new_with_label("Show hidden files");
    g_signal_connect(hidden_check, "toggled", G_CALLBACK(on_hidden_toggled),
            (gpointer)(parent));
    gtk_grid_attach(GTK_GRID(grid), hidden_check, 0, 0, 1, 1);

    readonly_check = gtk_check_button_new_with_label("Attach read-only");
    g_signal_connect(readonly_check, "toggled", G_CALLBACK(on_readonly_toggled),
            (gpointer)(parent));
    gtk_grid_attach(GTK_GRID(grid), readonly_check, 1, 0, 1, 1);
    resources_get_int_sprintf("AttachDevice%dd%dReadonly", &readonly_state, unit_number, drive_number);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(readonly_check), readonly_state);

    /* second row, three cols wide */
    gtk_grid_attach(GTK_GRID(grid),
            drive_unit_widget_create(unit, &unit_number, on_unit_changed),
            0, 1, 3, 1);

    /* add drive number widget */
    driveno_widget = drive_no_widget_create(drive, &drive_number, on_drive_num_changed);
    gtk_widget_set_sensitive(driveno_widget, drive_is_dualdrive_by_devnr(unit));

    gtk_grid_attach(GTK_GRID(grid), driveno_widget, 3, 1, 3, 1);

    gtk_widget_show_all(grid);
    return grid;
}



/** \brief  Create the disk attach dialog
 *
 * \param[in]   parent  parent widget, used to get the top level window
 * \param[in]   unit    drive unit
 *
 * \return  GtkFileChooserDialog
 */
static GtkWidget *create_disk_attach_dialog(gint unit, gint drive)
{
    GtkWidget *dialog;
    size_t i;
    int autostart = 0;

    resources_get_int("AutostartOnDoubleClick", &autostart);

    /* create new dialog */
    dialog = gtk_file_chooser_dialog_new(
            "Attach a disk image",
            ui_get_active_window(),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            /* buttons */
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

    /* set last directory */
    lastdir_set(dialog, &last_dir, &last_file);

    /* add 'extra' widget: 'readony' and 'show preview' checkboxes */
    if (unit < DRIVE_UNIT_MIN || unit > DRIVE_UNIT_MAX) {
        unit = DRIVE_UNIT_DEFAULT;
    }
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog),
                                      create_extra_widget(dialog, unit, drive));

    preview_widget = content_preview_widget_create(
            dialog, diskcontents_filesystem_read, on_response, unit);
    gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(dialog),
            preview_widget);


    /* add filters */
    for (i = 0; filters[i].name != NULL; i++) {
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),
                create_file_chooser_filter(filters[i], FALSE));
    }

    /* connect "reponse" handler: the `user_data` argument gets filled in when
     * the "response" signal is emitted: a response ID */
    g_signal_connect(dialog, "response",
            G_CALLBACK(on_response), GINT_TO_POINTER(0));
    g_signal_connect(dialog, "update-preview",
            G_CALLBACK(on_update_preview), NULL);
    g_signal_connect_unlocked(dialog, "selection-changed",
            G_CALLBACK(on_selection_changed), NULL);
    g_signal_connect(dialog, "destroy",
            G_CALLBACK(on_destroy),
            GINT_TO_POINTER((unit << 8) | (drive)));
    return dialog;
}


/** \brief  Create and show disk attach dialog.
 *
 * \param[in]   unit        integer from 8-11 for the default drive
 * \param[in]   drive       drive number (0 or 1)
 */
void ui_disk_attach_dialog_show(int unit, int drive)
{
    GtkWidget *dialog = create_disk_attach_dialog(unit, drive);
    gtk_widget_show(dialog);
}


/** \brief  Clean up the last directory string
 */
void ui_disk_attach_shutdown(void)
{
    lastdir_shutdown(&last_dir, &last_file);
}
