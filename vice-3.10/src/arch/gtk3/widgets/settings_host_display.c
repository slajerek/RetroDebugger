/** \file   settings_host_display.c
 * \brief   Widget to control resources related to the host display
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES FullscreenDecorations   -vsid
 * $VICERES StartMinimized          -vsid
 * $VICERES StartMaximized          -vsid
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

#include "canvasrenderfilterwidget.h"
#include "canvasrendermirrorwidget.h"
#include "canvasrendervsyncwidget.h"
#include "machine.h"
#include "uivideo.h"
#include "vice_gtk3.h"
#include "videoarch.h"
#include "videoaspectwidget.h"

#include "settings_host_display.h"


/** \brief  Create "Enable fulllscreen decorations" widget
 *
 * Currently completely ignores C128 VDC/VCII, maybe once we can differenciate
 * between displays (ie two or more) and put the GtkWindows there, we can
 * try to handle these scenarios.
 *
 * \param[in]   index   window index (unused)
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_fullscreen_decorations_widget(int index)
{
    return vice_gtk3_resource_check_button_new(
            "FullscreenDecorations",
            "Fullscreen decorations (Show menu and statusbar in fullscreen mode)");
}

/** \brief  Create grid with host rendering options
 *
 * Create GtkGrid with widgets controlling host GPU rendering options.
 *
 * <pre>
 * +----------------------------------------------+
 * | "${CHIP} rendering options"                  |
 * +------------------+-----------+---------------+
 * | AspectMode/Ratio | GL filter | Mirror/Rotate |
 * +------------------+-----------+---------------+
 * | Synchronization                              |
 * +------------------+---------------------------+
 * </pre>
 *
 * \param[in]   chip    chip name
 *
 * \return  GtkGrid
 */
static GtkWidget *create_rendering_options_widget(const char *chip)
{
    GtkWidget *grid;
    GtkWidget *widget;
    char       title[256];
    int        row = 1;

    g_snprintf(title, sizeof title, "<b>%s rendering options</b>", chip);
    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 32);

    /* row 0, col 0-3: title */
    widget = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(widget), title);
    gtk_widget_set_halign(widget, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(widget, 8);
    gtk_grid_attach(GTK_GRID(grid), widget, 0, 0, 3, 1);

    /* row 1+2, col 0: scaling and aspect ratio resources */
    widget = video_aspect_widget_create(chip);
    gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 2);

    /* row 1, col 1: glfilter */
    widget = canvas_render_filter_widget_create(chip);
    gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);

    /* row 1, col 2: mirror options */
    widget = canvas_render_mirror_widget_create(chip);
    gtk_grid_attach(GTK_GRID(grid), widget, 2, row, 1, 1);

    row++;

    /* row 2, col 0-2: vsync */
    widget = canvas_render_vsync_widget_create(chip);
    gtk_widget_set_margin_top(widget, 32);
    gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 3, 1);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Handler for the 'toggled' event of the minimized/maximized check buttons
 *
 * Prevent user from enabling both "start minimized" and "start maximized".
 *
 * \param[in]   self    check button triggering event
 * \param[in]   other   check button for opposite state
 */
static void on_minmax_toggled(GtkToggleButton *self, gpointer other)
{
    if (gtk_toggle_button_get_active(self) &&
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(other))) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(other), FALSE);
    }
}


/** \brief  Create host display settings widget
 *
 * \param[in]   widget  parent widget (used for dialogs)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_host_display_widget_create(GtkWidget *widget)
{
    GtkWidget *grid;

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, 0);

    if (machine_class != VICE_MACHINE_VSID) {

        GtkWidget  *decorations;
        GtkWidget  *minimized;
        GtkWidget  *maximized;
        GtkWidget  *rendering_options;
        GtkWidget  *rendering_options_vdc = NULL;
        const char *chip;

        chip = uivideo_chip_name();
        decorations = create_fullscreen_decorations_widget(0);
        minimized = vice_gtk3_resource_check_button_new(
                "StartMinimized",
                "Start the emulator window minimized");
        maximized = vice_gtk3_resource_check_button_new(
                "StartMaximized",
                "Start the emulator window maximized");
        rendering_options = create_rendering_options_widget(chip);
        gtk_widget_set_margin_top(rendering_options, 24);
        if (machine_class == VICE_MACHINE_C128) {
            rendering_options_vdc = create_rendering_options_widget("VDC");
            gtk_widget_set_margin_top(rendering_options_vdc, 24);
        }

        g_signal_connect(G_OBJECT(minimized),
                         "toggled",
                         G_CALLBACK(on_minmax_toggled),
                         (gpointer)maximized);
        g_signal_connect(G_OBJECT(maximized),
                         "toggled",
                         G_CALLBACK(on_minmax_toggled),
                         (gpointer)minimized);

        gtk_grid_attach(GTK_GRID(grid), decorations, 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), minimized, 0, 2, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), maximized, 0, 3, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), rendering_options, 0, 4, 1, 1);
        if (machine_class == VICE_MACHINE_C128) {
            gtk_grid_attach(GTK_GRID(grid), rendering_options_vdc, 0, 5, 1, 1);
        }
    }
    gtk_widget_show_all(grid);
    return grid;
}
