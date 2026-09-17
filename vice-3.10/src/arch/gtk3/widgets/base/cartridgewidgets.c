/** \file   cartridgewidgets.c
 * \brief   Widgets to control cartridge resources
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
 *
 */

#include "vice.h"
#include <gtk/gtk.h>

#include "basedialogs.h"
#include "cartridge.h"
#include "debug_gtk3.h"
#include "lastdir.h"
#include "log.h"
#include "resourcecheckbutton.h"
#include "resourcewidgetmediator.h"
#include "ui.h"

#include "cartridgewidgets.h"


/** \brief  State object containing per-image data
 *
 * This object is set as the resource mediator's extra data object so we can
 * access all relevant data on a cartridge image for the widgets.
 */
typedef struct ci_state_s {
    mediator_t *mediator;       /**< resource meditator backref */
    GtkWidget  *entry;          /**< GtkEntry for the image path */
    GtkWidget  *flush;          /**< GtkButton to flush the image */
    GtkWidget  *save;           /**< GtkButton to save the image */
    GtkWidget  *checks_grid;    /**< GtkGrid for the optional check buttons */
    int         checks_count;   /**< number of check buttons in \a checks_grid */
    int         cart_id;        /**< cartridge ID according to cartridge.h */
    char       *cart_name;      /**< cartridge name according to cartridge.h */
    cart_img_t  image_num;      /**< cartridge image number */
    char       *image_tag;      /**< image tag (EEPROM, Flash, BIOS, etc) */
} ci_state_t;


/* Forward declarations */
static GtkWidget *open_dialog_new(ci_state_t *state);
static GtkWidget *save_dialog_new(ci_state_t *state);
static gboolean   update_resource(ci_state_t *state, const char *filename);

/** \brief  Default image tags
 *
 * Tags used for images in case the user doesn't provide one.
 */
static const char *tag_defaults[CART_IMAGE_COUNT] = {
    "primary",
    "secondary",
    "tertiary",
    "quaternary"
};

/** \brief  Last used directory in file dialogs */
static char *last_dir = NULL;

/** \brief  Last used filename in file dialogs */
static char *last_file = NULL;


/*
 * Signal handlers
 */

/** \brief  Handler for the 'response' event of the open dialog
 *
 * Set image filename resource and update last used dir/file on success.
 *
 * \param[in]   self        open dialog
 * \param[in]   response    response ID
 * \param[in]   data        widget state object
 */
static void on_open_response(GtkDialog *self, gint response, gpointer data)
{
    if (response == GTK_RESPONSE_ACCEPT) {
        GtkWidget  *entry;
        ci_state_t *state;
        char       *filename;

        state    = data;
        entry    = state->entry;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(self));

        if (update_resource(state, filename)) {
            /* success */
            gtk_entry_set_text(GTK_ENTRY(entry), filename);
            /* move view to last part of filename string since that's the most
             * significant part visually */
            gtk_editable_set_position(GTK_EDITABLE(entry), -1);
            lastdir_update(GTK_WIDGET(self), &last_dir, &last_file);
        }
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(self));
}

/** \brief  Handler for the 'response' event of the save-as dialog
 *
 * Call the proper cartridge API function to save the image to file and update
 * the last used directory/filename on success.
 *
 * \param[in]   self        save-as dialog
 * \param[in]   response    response ID
 * \param[in]   data        widget state object
 */
static void on_save_response(GtkDialog *self, gint response, gpointer data)
{
    if (response == GTK_RESPONSE_ACCEPT) {
        ci_state_t *state = data;
        char       *filename;
        int         result;

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(self));

        switch (state->image_num) {
            case CART_IMAGE_PRIMARY:
                result = cartridge_save_image(state->cart_id, filename);
                break;
            case CART_IMAGE_SECONDARY:
                result = cartridge_save_secondary_image(state->cart_id, filename);
                break;
            default:
                log_error(LOG_DEFAULT,
                          "%s(): saving of %s cartridge image is not implemented.",
                          __func__, tag_defaults[state->image_num - CART_IMAGE_PRIMARY]);
                result = -1;
                break;
        }
        if (result == 0) {
            /* update last used directory and filename */
            lastdir_update(GTK_WIDGET(self), &last_dir, &last_file);
        } else {
            char title[256];

            g_snprintf(title, sizeof title, "%s Error", state->cart_name);
            vice_gtk3_message_error(GTK_WINDOW(self), title,
                                    "Failed to save %s %s image as '%s'.",
                                    state->cart_name, state->image_tag, filename);
        }
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(self));
}

/** \brief  Handler for the 'clicked' event of the save-as button
 *
 * Show file chooser dialog to save copy of image file.
 *
 * \param[in]   self    button (ignored)
 * \param[in]   data    widget state object
 */
static void on_save_clicked(GtkButton *self, gpointer data)
{
    GtkWidget *dialog;

    dialog = save_dialog_new(data);
    lastdir_set(dialog, &last_dir, &last_file);
    gtk_widget_show(dialog);
}

/** \brief  Handler for the 'clicked' event of the flush button
 *
 * \param[in]   self    button (ignored)
 * \param[in]   data    widget state object
 */
static void on_flush_clicked(GtkButton *self, gpointer data)
{
    ci_state_t *state = data;
    int         result;

    switch (state->image_num) {
        case CART_IMAGE_PRIMARY:
            result = cartridge_flush_image(state->cart_id);
            break;
        case CART_IMAGE_SECONDARY:
            result = cartridge_flush_secondary_image(state->cart_id);
            break;
        default:
            log_error(LOG_DEFAULT,
                      "%s(): flushing of %s cartridge image is not implemented.",
                      __func__, tag_defaults[state->image_num - CART_IMAGE_PRIMARY]);
            result = -1;
            break;
    }
    if (result != 0) {
        /* show error dialog */
        GtkWidget *parent;
        char       title[256];

        /* get parent window (usually the settings dialog) */
        parent = gtk_widget_get_toplevel(GTK_WIDGET(self));
        if (!GTK_IS_WINDOW(parent)) {
            /* widget isn't being used in the settings dialog, use active
             * emulator window as parent */
            parent = NULL;
        }
        g_snprintf(title, sizeof title, "%s Error", state->cart_name);
        vice_gtk3_message_error(GTK_WINDOW(parent),
                                title,
                                "Failed to flush %s %s image",
                                state->cart_name, state->image_tag);
    }
}

/** \brief  Handler for the 'focus-out-event' event of the filename entry
 *
 * Update filename resource with the text in \a self if the widget looses
 * focus.
 *
 * \param[in]   self    filename entry
 * \param[in]   event   event info
 * \param[in]   data    widget state object
 *
 * \return  `GDK_EVENT_PROPAGATE`
 */
static gboolean on_filename_focus_out_event(GtkEntry *self,
                                            GdkEvent *event,
                                            gpointer  data)
{
    /* this function will display a warning on errors */
    update_resource(data, gtk_entry_get_text(self));
    return GDK_EVENT_PROPAGATE;
}

/** \brief  Handler for the 'key-press-event' event of the filename entry
 *
 * Update filename resource with the text in \a self if the user presses Enter,
 * Return or "tabs out".
 *
 * \param[in]   self    filename entry
 * \param[in]   event   event info
 * \param[in]   data    widget state object
 *
 * \return  `GDK_EVENT_PROPAGATE`
 */
static gboolean on_filename_key_press_event(GtkEntry *self,
                                            GdkEvent *event,
                                            gpointer  data)
{
    if (gdk_event_get_event_type(event) == GDK_KEY_PRESS) {
        guint keyval = 0;

        gdk_event_get_keyval(event, &keyval);
        switch (keyval) {
            case GDK_KEY_Return:        /* fall through */
            case GDK_KEY_KP_Enter:      /* fall through */
            case GDK_KEY_Tab:           /* fall through */
            case GDK_KEY_ISO_Left_Tab:
                update_resource(data, gtk_entry_get_text(self));
                break;
            default:
                break;
        }
    }
    return GDK_EVENT_PROPAGATE;
}

/** \brief  Handler for the 'icon-press' event of the filename entry
 *
 * Show a file chooser dialog to select cartridge image.
 *
 * \param[in]   self        filename entry
 * \param[in]   icon_pos    icon position in \a self (ignored)
 * \param[in]   event       event information (ignored)
 * \param[in]   data        widget state object
 */
static void on_filename_icon_press(GtkEntry             *self,
                                   GtkEntryIconPosition  icon_pos,
                                   GdkEvent             *event,
                                   gpointer              data)
{
    GtkWidget *dialog;

    dialog = open_dialog_new(data);
    lastdir_set(dialog, &last_dir, &last_file);
    gtk_widget_show(dialog);
}


/*
 * Private API
 */

/** \brief  Allocate and initialize widget state object
 *
 * \param[in]   cart_id     cartridge ID
 * \param[in]   cart_name   cartridge name
 * \param[in]   image_num   cartridge image number
 * \param[in]   image_tag   cartridge image tag (can be `NULL`)
 *
 * \return  new initialized state object
 */
static ci_state_t *ci_state_new(int         cart_id,
                                const char *cart_name,
                                cart_img_t  image_num,
                                const char *image_tag)
{
    ci_state_t *state = g_malloc(sizeof *state);

    state->entry        = NULL;
    state->flush        = NULL;
    state->save         = NULL;
    state->checks_grid  = NULL;
    state->checks_count = 0;
    state->cart_id      = cart_id;
    state->cart_name    = g_strdup(cart_name);
    state->image_num    = image_num;
    if (image_tag != NULL) {
        state->image_tag = g_strdup(image_tag);
    } else {
        state->image_tag = g_strdup(tag_defaults[image_num - CART_IMAGE_PRIMARY]);
    }

    return state;
}

/** \brief  Free \a state and its members
 *
 * Free the \a state object and its members. This is called by the 'destroy'
 * signal handler of the main widget, which is registered by the resource
 * mediator.
 *
 * \param[in]   state   widget state object
 */
static void ci_state_free(void *state)
{
    ci_state_t *st = state;
    g_free(st->cart_name);
    g_free(st->image_tag);
    g_free(state);
}

/** \brief  Test if image number is valid
 *
 * \param[in]   num cartridge image number
 *
 * \return  TRUE if valid
 */
static gboolean valid_image_num(cart_img_t num)
{
    return (gboolean)(num >= CART_IMAGE_PRIMARY && num <= CART_IMAGE_QUATERNARY);
}

/** \brief  Update sensitivity of flush/save buttons
 *
 * Use the _can_flush()/_can_save() cartridge functions to set the sensitivity
 * of the save/flush buttons.
 *
 * \param[in]   state   widget state object
 */
static void update_buttons_sensitivity(const ci_state_t *state)
{
    gboolean can_flush;
    gboolean can_save;

    switch (state->image_num) {
        case CART_IMAGE_PRIMARY:
            can_save  = (gboolean)cartridge_can_save_image(state->cart_id);
            can_flush = (gboolean)cartridge_can_flush_image(state->cart_id);
            break;
        case CART_IMAGE_SECONDARY:
            can_save  = (gboolean)cartridge_can_save_secondary_image(state->cart_id);
            can_flush = (gboolean)cartridge_can_flush_secondary_image(state->cart_id);
            break;
        default:
            /* no support for tertiary/quaternary images yet */
            can_save  = FALSE;
            can_flush = FALSE;
    }
#if 0
    debug_gtk3("can-save : %s", can_save  ? "TRUE" : "FALSE");
    debug_gtk3("can-flush: %s", can_flush ? "TRUE" : "FALSE");
#endif
    if (state->flush != NULL) {
        gtk_widget_set_sensitive(state->flush, can_flush);
    }
    if (state->save != NULL) {
        gtk_widget_set_sensitive(state->save, can_save);
    }
}

/** \brief  Update filename resource of cartridge image
 *
 * Update the resource for the cartridge image to \a filename, if updating fails
 * an error message is displayed.
 *
 * \param[in]   state       widget state object
 * \param[in]   filename    new value for the resource
 *
 * \return  `TRUE` on success
 */
static gboolean update_resource(ci_state_t *state, const char *filename)
{
    mediator_t *mediator = state->mediator;
    gboolean    result;

    result = mediator_update_string(mediator, filename);
    if (!result) {
        char       title[256];
        GtkWidget *parent;

        /* get parent window (usually the settings dialog) */
        parent = gtk_widget_get_toplevel(mediator->widget);
        if (!GTK_IS_WINDOW(parent)) {
            /* widget isn't being used in the settings dialog, use active
             * emulator window as parent */
            parent = NULL;
        }
        g_snprintf(title, sizeof title, "%s Error", state->cart_name);
        vice_gtk3_message_error(GTK_WINDOW(parent),
                                title,
                                "Failed to set '%s' as the %s image file.",
                                filename, state->image_tag);
    }
    update_buttons_sensitivity(state);
    return result;
}

/** \brief  Create open dialog for cartridge images
 *
 * \param[in]   state   widget state object
 *
 * \return  GtkFileChooserDialog
 */
static GtkWidget *open_dialog_new(ci_state_t *state)
{
    GtkWidget *dialog;
    GtkWindow *parent;
    char       title[256];

    g_snprintf(title, sizeof title,
               "Select %s %s image",
               state->cart_name, state->image_tag);

    parent = ui_get_active_window();
    dialog = gtk_file_chooser_dialog_new(title,
                                         parent,
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "Open",
                                         GTK_RESPONSE_ACCEPT,
                                         "Close",
                                         GTK_RESPONSE_REJECT,
                                         NULL);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

    g_signal_connect(G_OBJECT(dialog),
                     "response",
                     G_CALLBACK(on_open_response),
                     (gpointer)state);
    return dialog;
}

/** \brief  Create save-as dialog for cartridge images
 *
 * \param[in]   state   widget state object
 *
 * \return  GtkFileChooserDialog
 */
static GtkWidget *save_dialog_new(ci_state_t *state)
{
    GtkWidget *dialog;
    GtkWindow *parent;
    char       title[256];

    g_snprintf(title, sizeof title,
               "Save %s %s image as ..",
               state->cart_name, state->image_tag);

    parent = ui_get_active_window();
    dialog = gtk_file_chooser_dialog_new(title,
                                         parent,
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         "Save",
                                         GTK_RESPONSE_ACCEPT,
                                         "Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         NULL);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

    g_signal_connect(G_OBJECT(dialog),
                     "response",
                     G_CALLBACK(on_save_response),
                     (gpointer)state);
    return dialog;
}
/** \brief  Create entry with clickable icon for image filename
 *
 * \param[in]   state   widget state object
 *
 * \return  GtkEntry
 */
static GtkWidget *create_filename_entry(ci_state_t *state)
{
    GtkWidget  *entry;
    const char *path;
    char        buffer[256];

    entry = gtk_entry_new();
    gtk_widget_set_hexpand(entry, TRUE);
    /* add clickable icon for the file chooser dialog */
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry),
                                      GTK_ENTRY_ICON_SECONDARY,
                                      "document-open-symbolic");
    /* set tooltip */
    g_snprintf(buffer, sizeof buffer, "Select %s image file", state->image_tag);
    gtk_entry_set_icon_tooltip_text(GTK_ENTRY(entry),
                                    GTK_ENTRY_ICON_SECONDARY,
                                    buffer);
    /* set path to image, if any */
    path = mediator_get_resource_string(state->mediator);
    if (path != NULL) {
        gtk_entry_set_text(GTK_ENTRY(entry), path);
    }

    /* set up signal handlers (all of these can set resources and thus cannot
     * be connected unlocked) */
    g_signal_connect(G_OBJECT(entry),
                     "focus-out-event",
                     G_CALLBACK(on_filename_focus_out_event),
                     (gpointer)state);
    g_signal_connect(G_OBJECT(entry),
                     "key-press-event",
                     G_CALLBACK(on_filename_key_press_event),
                     (gpointer)state);
    g_signal_connect(G_OBJECT(entry),
                     "icon-press",
                     G_CALLBACK(on_filename_icon_press),
                     (gpointer)state);

    return entry;
}

/** \brief  Create left-aligned label with Pango markup
 *
 * \param[in]   text    text for the label using Pango markup
 *
 * \return  GtkLabel
 */
static GtkWidget *label_helper(const char *text)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}


/*
 * Public API
 */

/** \brief  Create new cartridge image widget
 *
 * Create widget to select/save/flush a cartridge image.
 *
 * If \a image_tag is `NULL` a default "primary", "secondary" etc tag will be
 * used. If \a resource is `NULL` the GtkEntry to select an image file won't be
 * added.
 *
 * \param[in]   cart_id         cartridge ID
 * \param[in]   cart_name       cartridge name
 * \param[in]   image_num       cartridge image number
 * \param[in]   image_tag       cartridge image tag (can be `NULL`)
 * \param[in]   resource        cartridge image file resource (can be `NULL`)
 * \param[in]   flush_button    cartridge widget needs "flush image" button
 * \param[in]   save_button     cartridge widget needs "save image as" button
 *
 * \return  GtkGrid
 *
 * \see cartridge.h for constants for \a cart_id and \a cart_name.
 * \see #cart_img_t for valid values for \a image_num.
 */
GtkWidget *cart_image_widget_new(int         cart_id,
                                 const char *cart_name,
                                 cart_img_t  image_num,
                                 const char *image_tag,
                                 const char *resource,
                                 gboolean    flush_button,
                                 gboolean    save_button)
{
    GtkWidget  *grid;
    GtkWidget  *label;
    GtkWidget  *entry;
    GtkWidget  *checks;
    GtkWidget  *buttons = NULL;
    GtkWidget  *save = NULL;
    GtkWidget  *flush = NULL;
    mediator_t *mediator;
    ci_state_t *state;
    char        buffer[256];
    int         row = 0;

    if (!valid_image_num(image_num)) {
        log_error(LOG_DEFAULT,
                  "%s: invalid image number %d, valid range is %d-%d.",
                  __func__, image_num, CART_IMAGE_PRIMARY, CART_IMAGE_COUNT);
        return NULL;
    }

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    /* set up resource mediator and its extra state object */
    mediator = mediator_new(grid, resource, G_TYPE_STRING);
    state    = ci_state_new(cart_id, cart_name, image_num, image_tag);
    mediator_set_data(mediator, state, ci_state_free);
    state->mediator = mediator;

    /* header */
    g_snprintf(buffer, sizeof buffer,
               "<b>%s %s image</b>",
               cart_name, state->image_tag);
    label = label_helper(buffer);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 2, 1);
    row++;

    /* filename entry */
    if (resource != NULL) {
        /* put label and entry in new grid avoid Gtk adding too much padding
         * between the widgets */
        GtkWidget *wrapper = gtk_grid_new();

        gtk_grid_set_column_spacing(GTK_GRID(wrapper), 16);
        label = label_helper("Image file");
        entry = create_filename_entry(state);
        state->entry = entry;
        gtk_grid_attach(GTK_GRID(wrapper), label, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(wrapper), entry, 1, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), wrapper, 0, row, 2, 1);
        row++;
    }

    /* check buttons grid */
    checks = gtk_grid_new();
    state->checks_grid = checks;
    gtk_grid_attach(GTK_GRID(grid), checks,  0, row, 1, 1);

    /* save and flush buttons, if requested */
    if (flush_button || save_button) {
        buttons = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
        gtk_box_set_spacing(GTK_BOX(buttons), 8);
        gtk_widget_set_halign(buttons, GTK_ALIGN_END);
        gtk_widget_set_hexpand(buttons, TRUE);

        if (save_button) {
            save = gtk_button_new_with_label("Save image as ..");
            gtk_box_pack_start(GTK_BOX(buttons), save,  FALSE, FALSE, 0);
            g_signal_connect(G_OBJECT(save),
                             "clicked",
                             G_CALLBACK(on_save_clicked),
                             (gpointer)state);
            state->save = save;
        }
        if (flush_button) {
            flush = gtk_button_new_with_label("Flush image");
            gtk_box_pack_start(GTK_BOX(buttons), flush, FALSE, FALSE, 0);
            g_signal_connect(G_OBJECT(flush),
                             "clicked",
                             G_CALLBACK(on_flush_clicked),
                             (gpointer)state);
            state->flush = flush;
        }
        gtk_grid_attach(GTK_GRID(grid), buttons, 1, row, 1, 1);

        /* set sensitivity of buttons */
        update_buttons_sensitivity(state);
    }

    return grid;
}


/** \brief  Append a check button for a resource
 *
 * Append a check button for \a resource to the cartridge image \a widget.
 *
 * \param[in]   widget      cartridge image widget
 * \param[in]   resource    resource name
 * \param[in]   text        text for the check button
 *
 * \return  check button
 */
GtkWidget *cart_image_widget_append_check(GtkWidget  *widget,
                                          const char *resource,
                                          const char *text)
{
    mediator_t *mediator = mediator_for_widget(widget);
    if (mediator != NULL) {
        GtkWidget  *check;
        ci_state_t *state;

        state = mediator_get_data(mediator);
        check = vice_gtk3_resource_check_button_new(resource, text);
        gtk_grid_attach(GTK_GRID(state->checks_grid),
                        check,
                        0, state->checks_count, 1, 1);
        state->checks_count++;
        return check;
    }
    return NULL;
}


/** \brief  Update sensitivity of widget's save and flush buttons
 *
 * \param[in]   widget  cartridge image widget
 *
 * \note    If we require this multiple times in various settings pages to
 *          react to an "Enable" check button like the REU settings, we might
 *          consider adding a helper function to set up a event handler on
 *          such a check button that then calls this function.
 */
void cart_image_widget_update_sensitivity(GtkWidget *widget)
{
    mediator_t *mediator = mediator_for_widget(widget);

    if (mediator != NULL) {
        ci_state_t *state = mediator_get_data(mediator);

        if (state != NULL) {
            update_buttons_sensitivity(state);
        }
    }
}


/** \brief  Clean up resources used by this file on emulator shutdown
 *
 * Free the last used directory/filename which might be allocated by the
 * cartridge image widgets.
 * This function should be called once on emulator shutdown.
 */
void cart_image_widgets_shutdown(void)
{
    lastdir_shutdown(&last_dir, &last_file);
}
