/** \file   petmiscwidget.c
 * \brief   Widget to set the PET Crtc and EoiBlank resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES Crtc        xpet
 * $VICERES EoiBlank    xpet
 * $VICERES Screen2001  xpet
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

#include "resources.h"
#include "vice_gtk3.h"

#include "petmiscwidget.h"


/** \brief  Crtc checkbox */
static GtkWidget *crtc_widget = NULL;

/** \brief  Blank-on-EOI checkbox */
static GtkWidget *blank_widget = NULL;

/** \brief  Screen mirrors like the 2001 checkbox */
static GtkWidget *screen2001_widget = NULL;

/** \brief  User-defined callback for changes in the Crtc resource
 */
static void (*user_callback_crtc)(int) = NULL;

/** \brief  User-defined callback for changes in the EoiBlank resource
 */
static void (*user_callback_blank)(int) = NULL;

/** \brief  User-defined callback for changes in the Screen2001 resource
 */
static void (*user_callback_screen2001)(int) = NULL;


/** \brief  Handler for the "toggled" event of the CRTC check button
 *
 * Sets the Crtc resource when it has been changed.
 *
 * \param[in]   widget      radio button triggering the event
 * \param[in]   user_data   extra data (unused)
 */
static void on_crtc_toggled(GtkWidget *widget, gpointer user_data)
{
    int old_val = 0;
    int new_val;

    resources_get_int("Crtc", &old_val);
    new_val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (new_val != old_val) {
        resources_set_int("Crtc", new_val);
        if (user_callback_crtc != NULL) {
            user_callback_crtc(new_val);
        }
    }
}

/** \brief  Handler for the "toggled" event of the EOI Blank check button
 *
 * Sets the EoiBlank resource when it has been changed.
 *
 * \param[in]   widget      radio button triggering the event
 * \param[in]   user_data   extra data (unused)
 */
static void on_blank_toggled(GtkWidget *widget, gpointer user_data)
{
    int old_val;
    int new_val;

    resources_get_int("EoiBlank", &old_val);
    new_val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (new_val != old_val) {
        resources_set_int("EoiBlank", new_val);
        if (user_callback_blank != NULL) {
            user_callback_blank(new_val);
        }
    }
}

/** \brief  Handler for the "toggled" event of the Screen2001 check button
 *
 * Sets the Screen2001 resource when it has been changed.
 *
 * \param[in]   widget      radio button triggering the event
 * \param[in]   user_data   extra data (unused)
 */
static void on_screen2001_toggled(GtkWidget *widget, gpointer user_data)
{
    int old_val;
    int new_val;

    resources_get_int("Screen2001", &old_val);
    new_val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (new_val != old_val) {
        resources_set_int("Screen2001", new_val);
        if (user_callback_screen2001 != NULL) {
            user_callback_screen2001(new_val);
        }
    }
}


/** \brief  Create PET miscellaneous settings widget
 *
 * Adds check buttons for the Crtc and EoiBlank resources.
 *
 * \return  GtkGrid
 */
GtkWidget *pet_misc_widget_create(void)
{
    GtkWidget *grid;
    int        crtc       = 0;
    int        blank      = 0;
    int        screen2001 = 0;

    user_callback_crtc       = NULL;
    user_callback_blank      = NULL;
    user_callback_screen2001 = NULL;

    resources_get_int("Crtc",       &crtc);
    resources_get_int("EoiBlank",   &blank);
    resources_get_int("Screen2001", &screen2001);

    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, "Miscellaneous", 1);
    vice_gtk3_grid_set_title_margin(grid, 8);

    crtc_widget = gtk_check_button_new_with_label("CRTC chip enable");
    gtk_widget_set_margin_start(crtc_widget, 8);
    blank_widget = gtk_check_button_new_with_label("2001 quirk: EOI blanks screen");
    gtk_widget_set_margin_start(blank_widget, 8);
    screen2001_widget = gtk_check_button_new_with_label("2001 quirk: extra screen mirrors");
    gtk_widget_set_margin_start(screen2001_widget, 8);

    gtk_grid_attach(GTK_GRID(grid), crtc_widget, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), blank_widget, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), screen2001_widget, 0, 3, 1, 1);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(crtc_widget), crtc);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(blank_widget), blank);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen2001_widget), blank);

    g_signal_connect(crtc_widget,
                     "toggled",
                     G_CALLBACK(on_crtc_toggled),
                     NULL);
    g_signal_connect(blank_widget,
                     "toggled",
                     G_CALLBACK(on_blank_toggled),
                     NULL);
    g_signal_connect(screen2001_widget,
                     "toggled",
                     G_CALLBACK(on_screen2001_toggled),
                     NULL);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Set function to trigger on Crtc checkbox toggle
 *
 * \param[in]   func    callback function
 */
void pet_misc_widget_set_crtc_callback(void (*func)(int))
{
    user_callback_crtc = func;
}


/** \brief  Set function to trigger on EOI-blank checkbox toggle
 *
 * \param[in]   func    callback function
 */
void pet_misc_widget_set_blank_callback(void (*func)(int))
{
    user_callback_blank = func;
}


/** \brief  Set function to trigger on Screen2001 checkbox toggle
 *
 * \param[in]   func    callback function
 */
void pet_misc_widget_set_screen2001_callback(void (*func)(int))
{
    user_callback_screen2001 = func;
}


/** \brief  Synchronize \a widget with its resources
 *
 * Synchronize the PET misc widget's child widgets with their resources.
 *
 * \param[in,out]   widget  PET misc widget
 */
void pet_misc_widget_sync(GtkWidget *widget)
{
    int state;

    resources_get_int("Crtc", &state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(crtc_widget), state);

    resources_get_int("EoiBlank", &state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(blank_widget), state);

    resources_get_int("Screen2001", &state);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen2001_widget), state);
}
