/** \file   settings_default_cart.c
 * \brief   GTK3 default cartridge settings
 *
 * Settings UI widget to control default cartridge
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES CartridgeFile   x64 x64sc xscpu64 x128 xvic xplus4
 * $VICERES CartridgeType   x64 x64sc xscpu64 x128 xvic xplus4
 *
 * $VICERES GenericCartridgeFile2000 xvic
 * $VICERES GenericCartridgeFile4000 xvic
 * $VICERES GenericCartridgeFile6000 xvic
 * $VICERES GenericCartridgeFileA000 xvic
 * $VICERES GenericCartridgeFileB000 xvic
 *
 * $VICERES Cart1Name   xcbm2 xcbm5x0
 * $VICERES Cart2Name   xcbm2 xcbm5x0
 * $VICERES Cart4Name   xcbm2 xcbm5x0
 * $VICERES Cart6Name   xcbm2 xcbm5x0
 *
 * These are only directly *read* by the code, manipulating them goes through
 * the cartridge API as intended.
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

#include "vice_gtk3.h"
#include "debug_gtk3.h"
#include "cartridge.h"
#include "carthelpers.h"
#include "resources.h"
#include "machine.h"
#include "uicart.h"

#include "settings_default_cart.h"



/** \brief  Default cartridge file widget
 *
 * Read-only GtkEntry
 */
static GtkWidget *cart_default_file_widget = NULL;

/** \brief  Default cartridge type widget
 *
 * Read-only GtkEntry
 */
static GtkWidget *cart_default_type_widget = NULL;

/** \brief  Current cartridge file widget
 *
 * Read-only GtkEntry
 */
static GtkWidget *cart_file_widget = NULL;

/** \brief  Current cartridge type widget
 *
 * Read-only GtkEntry
 */
static GtkWidget *cart_type_widget = NULL;


/** \brief  Button used to set the current cartridge as default
 */
static GtkWidget *set_default_button = NULL;


/** \brief  Button to attach a cart */
static GtkWidget *attach_button = NULL;

/** \brief  Button to remove the current cart */
static GtkWidget *remove_button = NULL;



/** \brief  List of cartridge types/names
 *
 * Set once in the constructor, using cartridge_get_info_list() via a carthelpers
 * function pointer.
 */
static const cartridge_info_t *cart_info_list = NULL;


/** \brief  Get cartridge name by looking up \a id in the cartridge info list
 *
 * \param[in]   id  cartridge ID
 *
 * \return  cartridge name
 */
static const char *get_cart_name_by_id(int id)
{
    int i;

    for (i = 0; cart_info_list[i].name != NULL; i++) {
        /*printf("%d:%d:%s\n",id,cart_info_list[i].crtid,cart_info_list[i].name);*/
        if (cart_info_list[i].crtid == id) {
            return cart_info_list[i].name;
        }
    }
    return "<Unknown cartridge type>";
}


/** \brief  Update the file widget by inspecting the CartridgeFile resource
 *
 */
static void update_cart_default_file_widget(void)
{
    const char *filename = NULL;

    resources_get_string("CartridgeFile", &filename);
    gtk_entry_set_text(GTK_ENTRY(cart_default_file_widget), filename);
}

/** \brief  Update the type widget by inspecting the CartridgeType resource
 *
 */
static void update_cart_default_type_widget(void)
{
    if (cart_info_list != NULL) {
        const char *name = "CRT";
        int         type = 0;

        resources_get_int("CartridgeType", &type);
        /*printf("CartridgeType: %d\n", type);*/
        if (type != 0) {
            name = get_cart_name_by_id(type);
        }
        gtk_entry_set_text(GTK_ENTRY(cart_default_type_widget), name);
    }
}

static void update_cart_file_widget(void)
{
    const char *filename = cartridge_get_filename_by_slot(0 /* FIXME: slot */);

    if (filename != NULL) {
        gtk_entry_set_text(GTK_ENTRY(cart_file_widget), filename);
    }
}

static void update_cart_type_widget(void)
{
    if (cart_info_list != NULL) {
        const char *name = "CRT";
        int         type;

        type = cartridge_get_id(0 /* FIXME: slot */);
        /*printf("CartridgeType: %d\n", type);*/
        if (type != 0) {
            name = get_cart_name_by_id(type);
        }
        gtk_entry_set_text(GTK_ENTRY(cart_type_widget), name);
    }
}


/** \brief  Set button sensitivity based on whether there is a default cart
 */
static void update_buttons(void)
{
    /* Faulty logic: */
#if 0
    const char *filename = NULL;

    resources_get_string("CartridgeFile", &filename);
    if (filename != NULL && *filename != '\0') {
        /* default cart is set */
        gtk_widget_set_sensitive(set_default_button, FALSE);
        gtk_widget_set_sensitive(remove_button, TRUE);
    } else {
        gtk_widget_set_sensitive(set_default_button, TRUE);
        gtk_widget_set_sensitive(remove_button, FALSE);
    }
#endif
}

/** \brief  Update all widgets */
static void update_widgets(void)
{
    update_cart_default_file_widget();
    update_cart_default_type_widget();
    update_cart_type_widget();
    update_cart_file_widget();
    update_buttons();
}

/** \brief  Callback triggered from the attach dialog
 */
static void attach_callback(void)
{
    update_widgets();
}

/** \brief  Handler for the 'clicked' event of the 'set default' button
 *
 * \param[in]   widget  button triggering the event
 * \param[in]   data    extra event data (unused)
 */
static void on_set_default_clicked(GtkWidget *widget, gpointer data)
{
    cartridge_set_default();
    update_widgets();
}

/** \brief  Callback for the attach dialog
 *
 * \param[in]   widget  parent widget
 * \param[in]   data    extra even data (unused)
 */
static void on_attach_clicked(GtkWidget *widget, gpointer data)
{
    ui_cart_default_attach(widget, attach_callback);
}

/** \brief  Handler for the 'clicked' event of the 'Remove' button
 *
 * Unsets the current default cart.
 *
 * \param[in]   widget  button triggering the event
 * \param[in]   data    extra event data (unused)
 */
static void on_remove_clicked(GtkWidget *widget, gpointer data)
{
    cartridge_unset_default();
    update_widgets();
}

/** \brief  Create left aligned, 8 pixels indented label
 *
 * \param[in]   text    label text (uses Pango markup)
 *
 * \return  GtkLabel
 */
static GtkWidget *create_label(const char *text)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 8);
    return label;
}

/** \brief  Create non-sensitive, non-editable GtkEntry
 *
 * \return  GtkEntry
 */
static GtkWidget *create_disabled_entry(void)
{
    GtkWidget *entry = gtk_entry_new();

    gtk_widget_set_hexpand(entry, TRUE);
    gtk_widget_set_sensitive(entry, FALSE);
    g_object_set(G_OBJECT(entry), "editable", FALSE, NULL);
    return entry;
}

/** \brief  Create default cart file entry
 *
 * \return  GtkEntry
 */
static GtkWidget *create_cart_default_file_widget(void)
{
    return create_disabled_entry();
}

/** \brief  Create default cart type entry
 *
 * \return  GtkEntry
 */
static GtkWidget *create_cart_default_type_widget(void)
{
    return create_disabled_entry();
}

/** \brief  Create default cart file entry
 *
 * \return  GtkEntry
 */
static GtkWidget *create_cart_file_widget(void)
{
    return create_disabled_entry();
}

/** \brief  Create cart type entry
 *
 * \return  GtkEntry
 */
static GtkWidget *create_cart_type_widget(void)
{
    return create_disabled_entry();
}

/** \brief  Create a default cart settings widget for the settings UI
 *
 * \param[in]   parent  parent widget (ignored)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_default_cart_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;

    grid = vice_gtk3_grid_new_spaced(8, 8);
    label = create_label("<b>Default cartridge</b>");
    gtk_widget_set_margin_start(label, 0);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 4, 1);

    if (cart_info_list == NULL) {
        cart_info_list = cartridge_get_info_list();
    }

    /* resource CartridgeFile */
    label = create_label("File");
    cart_default_file_widget = create_cart_default_file_widget();
    gtk_grid_attach(GTK_GRID(grid), label,                    0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cart_default_file_widget, 1, 1, 1, 1);
    update_cart_default_file_widget();

    attach_button = gtk_button_new_with_label("Attach");
    g_signal_connect(attach_button,
                     "clicked",
                     G_CALLBACK(on_attach_clicked),
                     NULL);
    gtk_grid_attach(GTK_GRID(grid), attach_button, 2, 1, 1, 1);

    remove_button = gtk_button_new_with_label("Remove");
    g_signal_connect(remove_button,
                     "clicked",
                     G_CALLBACK(on_remove_clicked),
                     NULL);
    gtk_grid_attach(GTK_GRID(grid), remove_button, 3, 1, 1, 1);


    /* resource CartridgeType */
    if (cart_info_list != NULL) {
        label = create_label("Type");
        cart_default_type_widget = create_cart_default_type_widget();
        gtk_grid_attach(GTK_GRID(grid), label,                    0, 2, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), cart_default_type_widget, 1, 2, 1, 1);
        update_cart_default_type_widget();

        label = create_label("<b>Current cartridge</b>");
        gtk_widget_set_margin_top(label, 8);
        gtk_widget_set_margin_start(label, 0);
        gtk_widget_set_margin_bottom(label, 8);
        gtk_grid_attach(GTK_GRID(grid), label, 0, 3, 4, 1);

        label = create_label("File");
        cart_file_widget = create_cart_file_widget();
        gtk_grid_attach(GTK_GRID(grid), label,            0, 4, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), cart_file_widget, 1, 4, 1, 1);
        update_cart_file_widget();

        label = create_label("Type");
        cart_type_widget = create_cart_type_widget();
        gtk_grid_attach(GTK_GRID(grid), label,            0, 5, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), cart_type_widget, 1, 5, 1, 1);
        update_cart_type_widget();
    }

    /* buttons */
    set_default_button = gtk_button_new_with_label("Set current cartidge as default");
    gtk_widget_set_halign(set_default_button, GTK_ALIGN_START);
    /* gtk_widget_set_hexpand(set_default_button, TRUE); */
    gtk_grid_attach(GTK_GRID(grid), set_default_button, 1, 6, 1, 1);

    /* set sensitivity of buttons (ie grey-out or not) */
    update_buttons();

    /* connect signal handlers */
    g_signal_connect(set_default_button,
                     "clicked",
                     G_CALLBACK(on_set_default_clicked),
                     NULL);

    gtk_widget_show_all(grid);
    return grid;
}
