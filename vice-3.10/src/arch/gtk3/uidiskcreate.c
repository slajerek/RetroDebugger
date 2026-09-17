/** \file   uidiskcreate.c
 * \brief   Gtk3 dialog to create and attach a new disk image
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

#include "vice.h"
#include <gtk/gtk.h>
#include <string.h>

#include "attach.h"
#include "basedialogs.h"
#include "basewidgets.h"
#include "charset.h"
#include "diskimage.h"
#include "drive.h"
#include "drivenowidget.h"
#include "driveunitwidget.h"
#include "filechooserhelpers.h"
#include "imagecontents.h"
#include "lib.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "util.h"
#include "vdrive/vdrive-internal.h"
#include "widgethelpers.h"

#include "uidiskcreate.h"


/** \brief  Struct holding image type names and IDs
 */
typedef struct disk_image_type_s {
    const char *name;   /**< name (ext) */
    int id;             /**< image type ID */
} disk_image_type_t;


/* forward declaration */
static gboolean create_disk_image(GtkWindow *parent, const char *filename);


/** \brief  List of supported disk image types
 *
 * XXX: perhaps some function in diskimage.c or so producing a list of
 *      currently supported images types like this one would be better, that
 *      would avoid having to update UI's when a new image type is added or
 *      removed.
 */
static disk_image_type_t disk_image_types[] = {
    { "d64", DISK_IMAGE_TYPE_D64 },
    { "d67", DISK_IMAGE_TYPE_D67 },
    { "d71", DISK_IMAGE_TYPE_D71 },
    { "d80", DISK_IMAGE_TYPE_D80 },
    { "d81", DISK_IMAGE_TYPE_D81 },
    { "d82", DISK_IMAGE_TYPE_D82 },
    { "d90", DISK_IMAGE_TYPE_D90 },
    { "d1m", DISK_IMAGE_TYPE_D1M },
    { "d2m", DISK_IMAGE_TYPE_D2M },
    { "d4m", DISK_IMAGE_TYPE_D4M },
    { "dhd", DISK_IMAGE_TYPE_DHD },
    { "g64", DISK_IMAGE_TYPE_G64 },
    { "g71", DISK_IMAGE_TYPE_G71 },
    { "p64", DISK_IMAGE_TYPE_P64 },
#ifdef HAVE_X64_IMAGE
    { "x64", DISK_IMAGE_TYPE_X64 },
#endif
    { NULL, -1 }
};


/** \brief  Drive unit to attach image to */
static int unit_number = DRIVE_UNIT_MIN;
/** \brief  Drive number to attach image to in unit*/
static int drive_number = 0;
/** \brief  Disk image type to create */
static int image_type = 1541;

/** \brief  GtkEntry containing the disk name */
static GtkWidget *disk_name;
/** \brief  GtkEntry containing the disk ID */
static GtkWidget *disk_id;
/** \brief  Set drive type when attaching */
static GtkWidget *set_drive_type;


/** \brief  Handler for the 'destroy' event of the dialog
 *
 * \param[in]   self    dialog (unused)
 * \param[in]   unused  extra event data (unused)
 */
static void on_destroy(GtkWidget *self, gpointer unused)
{
    ui_action_finish(ACTION_DRIVE_CREATE);
}


/** \brief  Handler for 'response' event of the dialog
 *
 * This handler is called when the user clicks a button in the dialog.
 *
 * \param[in,out]   dialog      the dialog
 * \param[in]       response_id response ID
 * \param[in]       data        extra data (unused)
 */
static void on_response(GtkDialog *dialog, gint response_id, gpointer data)
{
    gchar    *filename;
    gboolean  status = TRUE;

    switch (response_id) {

        case GTK_RESPONSE_ACCEPT:
            filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            if (filename != NULL) {
                gchar *filename_locale;

                filename_locale = file_chooser_convert_to_locale(filename);
                status = create_disk_image(GTK_WINDOW(dialog), filename_locale);
                g_free(filename_locale);
            }
            g_free(filename);
            if (status) {
                /* image creation and attaching was succesful, exit dialog */
                gtk_widget_destroy(GTK_WIDGET(dialog));
            }
            break;

        case GTK_RESPONSE_REJECT:
            gtk_widget_destroy(GTK_WIDGET(dialog));
            break;
        default:
            break;
    }
}


/** \brief  Handler for the 'changed' event of the image type combo box
 *
 * \param[in]   combo   combo box
 * \param[in]   data    extra event data (unused)
 *
 */
static void on_disk_image_type_changed(GtkComboBox *combo, gpointer data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_combo_box_get_active(combo) >= 0) {
        model = gtk_combo_box_get_model(combo);
        if (gtk_combo_box_get_active_iter(combo, &iter)) {
            gtk_tree_model_get(model, &iter, 1, &image_type, -1);
        }
    }
}


/** \brief  Attempt to set drive to match image type
 *
 * \return  TRUE if succesful
 */
static gboolean attempt_to_set_drive_type(void)
{
    if (resources_set_int_sprintf("Drive%dType", image_type, unit_number) < 0) {
        return FALSE;
    }
    return TRUE;
}


/** \brief  Get the extension for image \a type
 *
 * \param[in]   type    image type
 *
 * \return  extension or `NULL` when not found
 */
static const char *get_ext_by_image_type(int type)
{
    int i = 0;

    for (i = 0; disk_image_types[i].name != NULL; i++) {
        if (disk_image_types[i].id == type) {
            return disk_image_types[i].name;
        }
    }
    return NULL;
}


/** \brief  Actually create the disk image and attach it
 *
 * \param[in]   parent      parent dialog
 * \param[in]   filename    filename of the new image
 *
 * \return  bool
 */
static gboolean create_disk_image(GtkWindow *parent, const char *filename)
{
    char       *fname_copy;
    char        name_vice[IMAGE_CONTENTS_NAME_LEN + 1];
    char        id_vice[IMAGE_CONTENTS_ID_LEN + 1];
    const char *name_gtk3;
    const char *id_gtk3;
    char       *vdr_text;
    gboolean    status = TRUE;

    memset(name_vice, 0, sizeof name_vice);
    memset(id_vice, 0, sizeof id_vice);
    name_gtk3 = gtk_entry_get_text(GTK_ENTRY(disk_name));
    id_gtk3   = gtk_entry_get_text(GTK_ENTRY(disk_id));

    /* fix extension of filename */
    fname_copy = util_add_extension_const(filename,
                                          get_ext_by_image_type(image_type));

    /* convert name & ID to PETSCII */
    if (name_gtk3 != NULL && *name_gtk3 != '\0') {
        strncpy(name_vice, name_gtk3, IMAGE_CONTENTS_NAME_LEN);
        charset_petconvstring((unsigned char *)name_vice, CONVERT_TO_PETSCII);
    }
    if (id_gtk3 != NULL && *id_gtk3 != '\0') {
        strncpy(id_vice, id_gtk3, IMAGE_CONTENTS_ID_LEN);
        charset_petconvstring((unsigned char *)id_vice, CONVERT_TO_PETSCII);
    } else {
        strcpy(id_vice, "00");
    }

    vdr_text = util_concat(name_vice, ",", id_vice, NULL);

    /* create image */
    if (vdrive_internal_create_format_disk_image(fname_copy, vdr_text,
                image_type) < 0) {
        vice_gtk3_message_error(parent,
                                "Fail",
                                "Could not create image '%s'",
                                fname_copy);
        status = FALSE;
    } else {
        /* do we need to attempt to set the proper drive type? */
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(set_drive_type))) {
            /* try to set the proper drive type, but keep going if it fails */
            if (!attempt_to_set_drive_type()) {
                vice_gtk3_message_error(parent,
                                        "Core error",
                                        "Failed to set drive type to %d\nContinuing.",
                                        image_type);
            }
        }

        /* finally attach the disk image */
        if (file_system_attach_disk(unit_number, drive_number, fname_copy) < 0) {
            vice_gtk3_message_error(parent,
                                    "fail",
                                    "Could not attach image '%s'",
                                    fname_copy);
            status = FALSE;
        }
    }

    lib_free(fname_copy);
    lib_free(vdr_text);
    return status;
}


/** \brief  Create model for the image type combo box
 *
 * \return  model
 */
static GtkListStore *create_disk_image_type_model(void)
{
    GtkListStore *model;
    int i;

    model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    for (i = 0; disk_image_types[i].name != NULL; i++) {
        GtkTreeIter iter;

        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model, &iter,
                           0, disk_image_types[i].name,
                           1, disk_image_types[i].id,
                           -1);
    }
    return model;
}


/** \brief  Create combo box with image types
 *
 * \return  GtkComboBox
 */
static GtkWidget *create_disk_image_type_widget(void)
{
    GtkWidget *combo;
    GtkListStore *model;
    GtkCellRenderer *renderer;

    model = create_disk_image_type_model();
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer,
                                   "text", 0,
                                   NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

    g_signal_connect_unlocked(G_OBJECT(combo),
                              "changed",
                              G_CALLBACK(on_disk_image_type_changed),
                              NULL);
    return combo;
}


/** \brief  Create the 'extra' widget for the dialog
 *
 * \param[in]   parent  parent widget (dialog, unused at the moment)
 * \param[in]   unit    default unit number (unused)
 *
 * \return  GtkGrid
 */
static GtkWidget *create_extra_widget(GtkWidget *parent, int unit)
{
    GtkWidget *grid;
    GtkWidget *unit_widget;
    GtkWidget *drive_widget;
    GtkWidget *type_widget;
    GtkWidget *label;

    /* create a grid with some spacing and margins */
    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);
    gtk_widget_set_margin_start(grid, 16);
    gtk_widget_set_margin_end(grid, 16);

    /* add unit selection widget */
    unit_widget = drive_unit_widget_create(unit, &unit_number, NULL);
    gtk_widget_set_valign(unit_widget, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), unit_widget, 0, 0, 1, 1);

    drive_widget = drive_no_widget_create(0, &drive_number, NULL);
    gtk_widget_set_valign(drive_widget, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), drive_widget, 0, 1, 1, 1);

    /* disk name */
    label = gtk_label_new("Name:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    disk_name = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(disk_name), IMAGE_CONTENTS_NAME_LEN);
    gtk_entry_set_max_length(GTK_ENTRY(disk_name), IMAGE_CONTENTS_NAME_LEN);
    gtk_grid_attach(GTK_GRID(grid), label, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), disk_name, 2, 0, 1, 1);

    /* disk ID */
    label = gtk_label_new("ID:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    disk_id = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(disk_id), IMAGE_CONTENTS_ID_LEN);
    gtk_entry_set_max_length(GTK_ENTRY(disk_id), IMAGE_CONTENTS_ID_LEN);
    gtk_grid_attach(GTK_GRID(grid), label, 3, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), disk_id, 4, 0, 1, 1);

    /* add image type selection widget */
    label = gtk_label_new("Type:");
    type_widget = create_disk_image_type_widget();
    gtk_grid_attach(GTK_GRID(grid), label, 5, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), type_widget, 6, 0, 1, 1);

    /* add 'set drive type for attached image' checkbox */
    set_drive_type = gtk_check_button_new_with_label(
            "Set proper drive type when attaching image");
    /* disable by default */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(set_drive_type), FALSE);
    gtk_grid_attach(GTK_GRID(grid), set_drive_type, 4, 1, 4, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create and show 'attach new disk image' dialog
 *
 *  \param[in]  unit    unit number (8-11)
 */
void ui_disk_create_dialog_show(gint unit)
{
    GtkWidget *dialog;
    GtkFileFilter *filter;

    if (unit < DRIVE_UNIT_MIN || unit > DRIVE_UNIT_MAX) {
        unit = DRIVE_UNIT_DEFAULT;
    }
    unit_number = unit;

    dialog = gtk_file_chooser_dialog_new("Create and attach a new disk image",
                                         ui_get_active_window(),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         /* buttons */
                                         "Save", GTK_RESPONSE_ACCEPT,
                                         "Close", GTK_RESPONSE_REJECT,
                                         NULL);

    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog),
                                      create_extra_widget(dialog, unit));

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
                                                   TRUE);

    filter = create_file_chooser_filter(file_chooser_filter_disk, FALSE);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    g_signal_connect(G_OBJECT(dialog),
                     "response",
                     G_CALLBACK(on_response),
                     NULL);
    g_signal_connect_unlocked(G_OBJECT(dialog),
                              "destroy",
                              G_CALLBACK(on_destroy),
                              NULL);
    gtk_widget_show(dialog);
}
