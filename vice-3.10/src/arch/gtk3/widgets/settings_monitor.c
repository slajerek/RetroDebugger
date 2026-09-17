/** \file   settings_monitor.c
 * \brief   GTK3 monitor setting dialog
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES KeepMonitorOpen             all
 * $VICERES RefreshOnBreak              all
 * $VICERES MonitorServer               all
 * $VICERES MonitorServerAddress        all
 * $VICERES BinaryMonitorServer         all
 * $VICERES BinaryMonitorServerAddress  all
 * $VICERES NativeMonitor               all
 * $VICERES MonitorLogEnabled           all
 * $VICERES MonitorLogFileName          all
 * $VICERES MonitorChisLines            all
 * $VICERES MonitorFont                 all
 * $VICERES MonitorScrollbackLines      all
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
#include <limits.h>

#include "vice_gtk3.h"
#include "resources.h"
#include "uimon.h"

#include "settings_monitor.h"


/** \brief  Handler for the 'color-set' event of the background color widget
 *
 * \param[in]   button  background color button
 * \param[in]   data    extra event data (unused)
 */
static void on_bg_color_set(GtkColorButton *button, gpointer data)
{
    GdkRGBA color;
    char *repr;

    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &color);
    repr = gdk_rgba_to_string(&color);
    resources_set_string("MonitorBG", repr);
    g_free(repr);
}

/** \brief  Handler for the 'color-set' event of the foreground color widget
 *
 * \param[in]   button  foreground color button
 * \param[in]   data    extra event data (unused)
 */
static void on_fg_color_set(GtkColorButton *button, gpointer data)
{
    GdkRGBA color;
    char *repr;

    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &color);
    repr = gdk_rgba_to_string(&color);
    resources_set_string("MonitorFG", repr);
    g_free(repr);
}

/** \brief  Handler for the 'font-set' event of the font chooser
 *
 * \param[in]   button      font chooser button
 * \param[in]   user_data   extra event data (unused)
 */
static void on_font_set(GtkFontButton *button, gpointer user_data)
{
    gchar *font_desc;

    font_desc = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(button));
    if (font_desc != NULL) {
        if (resources_set_string("MonitorFont", font_desc) == 0) {
            /* try to 'live-update' the monitor font */
            uimon_set_font();
        }
        g_free(font_desc);
    }
}

/* temporarily disabled to see if that fixes bug #2015 */
#if 0
/** \brief  Filter function for font selection dialog
 *
 * \param[in]   family  font family
 * \param[in]   face    font face (unused)
 * \param[in]   data    optional user data (unused)
 *
 * \return  \c TRUE if font is monospace and should be shown in the dialog
 */
static gboolean font_button_filter(const PangoFontFamily *family,
                                   const PangoFontFace   *face,
                                   gpointer               data)
{
    /* we have to cast away const due to shitty API */
    return pango_font_family_is_monospace((PangoFontFamily*)family);
}
#endif

/** \brief  Create widget to control monitor resources
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_monitor_widget_create(GtkWidget *parent)
{
    GtkWidget  *grid;
    GtkWidget  *native;
    GtkWidget  *keep_open;
    GtkWidget  *refresh;
    GtkWidget  *rmon_enable;
    GtkWidget  *rmon_addr;
    GtkWidget  *bmon_enable;
    GtkWidget  *bmon_addr;
    GtkWidget  *log_enable;
    GtkWidget  *log_name;
    GtkWidget  *scroll_lines;
    GtkWidget  *scroll_label;
    GtkWidget  *scroll_info;
#ifdef FEATURE_CPUMEMHISTORY
    GtkWidget  *chis_lines;
    GtkWidget  *chis_label;
    GtkWidget  *chis_warning;
#endif
    GtkWidget  *font_label;
    GtkWidget  *font_button;
    GtkWidget  *colors_label;
    GtkWidget  *colors_wrapper;
    GtkWidget  *bg_button;
    GtkWidget  *fg_button;
    GtkWidget  *bg_label;
    GtkWidget  *fg_label;
    GdkRGBA     color;
    const char *patterns[] = { "*.log", "*.txt", NULL };
    const char *color_res = NULL;
    const char *font_name = NULL;
    int         row = 0;

    resources_get_string("MonitorFont", &font_name);

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

    native = vice_gtk3_resource_check_button_new("NativeMonitor",
                                                 "Use native monitor interface");
    keep_open = vice_gtk3_resource_check_button_new("KeepMonitorOpen",
                                                    "Keep monitor open");
    refresh = vice_gtk3_resource_check_button_new("RefreshOnBreak",
                                                  "Refresh display after command");

    /* Remote monitor */
    rmon_enable = vice_gtk3_resource_check_button_new("MonitorServer",
                                                      "Enable remote monitor");
    rmon_addr = vice_gtk3_resource_entry_new("MonitorServerAddress");
    gtk_widget_set_hexpand(rmon_addr, TRUE);
    gtk_widget_set_margin_top(rmon_enable, 8);
    gtk_widget_set_margin_top(rmon_addr, 8);

    /* Binary monitor */
    bmon_enable = vice_gtk3_resource_check_button_new("BinaryMonitorServer",
                                                      "Enable binary monitor");
    bmon_addr = vice_gtk3_resource_entry_new("BinaryMonitorServerAddress");
    gtk_widget_set_hexpand(bmon_addr, TRUE);
    gtk_widget_set_margin_top(bmon_enable, 8);
    gtk_widget_set_margin_top(bmon_addr, 8);

    log_enable = vice_gtk3_resource_check_button_new("MonitorLogEnabled",
                                                     "Enable logging to a file");
#if 0
    log_name = vice_gtk3_resource_browser_save_new("MonitorLogFileName",
                                                   "Select monitor log filename",
                                                   NULL,
                                                   NULL,
                                                   NULL);
#endif
    log_name = vice_gtk3_resource_filechooser_new("MonitorLogFileName",
                                                  GTK_FILE_CHOOSER_ACTION_SAVE);
    vice_gtk3_resource_filechooser_set_custom_title(log_name,
                                                    "Select or create monitor log file");
    vice_gtk3_resource_filechooser_set_filter(log_name,
                                              "Log files",
                                              patterns,
                                              TRUE);
    gtk_widget_set_hexpand(log_name, TRUE);
    gtk_widget_set_margin_top(log_enable, 8);
    gtk_widget_set_margin_top(log_name, 8);

    scroll_label = gtk_label_new("Scrollback buffer (lines)");
    scroll_lines = vice_gtk3_resource_spin_int_new("MonitorScrollbackLines",
                                                   -1, INT_MAX, 256);
    scroll_info  = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(scroll_info),
                         "<i>Use -1 for no limit</i>");
    gtk_widget_set_halign (scroll_label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(scroll_lines, FALSE);
    gtk_widget_set_vexpand(scroll_lines, FALSE);
    gtk_widget_set_halign (scroll_lines, GTK_ALIGN_START);
    gtk_widget_set_valign (scroll_lines, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (scroll_info,  GTK_ALIGN_START);
    gtk_widget_set_margin_top(scroll_label, 8);
    gtk_widget_set_margin_top(scroll_lines, 8);
    gtk_widget_set_margin_top(scroll_info,  8);

#ifdef FEATURE_CPUMEMHISTORY
    chis_label   = gtk_label_new("CPU History (lines)");
    chis_lines   = vice_gtk3_resource_spin_int_new("MonitorChisLines",
                                                   10, INT_MAX, 1024);
    chis_warning = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(chis_warning),
                         "<i>Changing this will clear CPU history</i>");
    gtk_widget_set_halign (chis_label,   GTK_ALIGN_START);
    gtk_widget_set_hexpand(chis_lines,   FALSE);
    gtk_widget_set_vexpand(chis_lines,   FALSE);
    gtk_widget_set_halign (chis_lines,   GTK_ALIGN_START);
    gtk_widget_set_valign (chis_lines,   GTK_ALIGN_CENTER);
    gtk_widget_set_halign (chis_warning, GTK_ALIGN_START);
    gtk_widget_set_margin_top(chis_label,   8);
    gtk_widget_set_margin_top(chis_lines,   8);
    gtk_widget_set_margin_top(chis_warning, 8);
#endif

    /* font selection label and button */
    font_label = gtk_label_new("Monitor font");
    gtk_widget_set_halign(font_label, GTK_ALIGN_START);

    /* create button that pops up a font selector */
    font_button = gtk_font_button_new();
#if 0
    gtk_font_chooser_set_filter_func(GTK_FONT_CHOOSER(font_button),
                                     font_button_filter,
                                     NULL,  /* extra data */
                                     NULL   /* destroy callback for extra data */);
#endif
    gtk_font_button_set_use_font(GTK_FONT_BUTTON(font_button), TRUE);
    if (font_name != NULL) {
        gtk_font_chooser_set_font(GTK_FONT_CHOOSER(font_button), font_name);
    }
    gtk_widget_set_margin_top(font_label, 8);
    gtk_widget_set_margin_top(font_button, 8);
    g_signal_connect(font_button, "font-set", G_CALLBACK(on_font_set), NULL);

    /* Monitor colors */
    colors_label = gtk_label_new("Monitor colors");
    gtk_widget_set_halign(colors_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(colors_label, 8);

    colors_wrapper = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(colors_wrapper), 8);
    gtk_grid_set_column_homogeneous(GTK_GRID(colors_wrapper), TRUE);
    gtk_widget_set_margin_top(colors_wrapper, 8);

    /* create button for the monitor backbround color */
    resources_get_string("MonitorBG", &color_res);
    gdk_rgba_parse(&color, color_res);
    bg_button = gtk_color_button_new_with_rgba(&color);
    bg_label = gtk_label_new("Background");
    gtk_widget_set_halign(bg_label, GTK_ALIGN_START);
    g_signal_connect(bg_button, "color-set", G_CALLBACK(on_bg_color_set), NULL);

    /* create button for the monitor foreground color */
    resources_get_string("MonitorFG", &color_res);
    gdk_rgba_parse(&color, color_res);
    fg_button = gtk_color_button_new_with_rgba(&color);
    fg_label = gtk_label_new("Foreground");
    gtk_widget_set_halign(fg_label, GTK_ALIGN_START);
    g_signal_connect(fg_button, "color-set", G_CALLBACK(on_fg_color_set), NULL);

    /* color pickers and their labels are packing into a grid of their own */
    gtk_grid_attach(GTK_GRID(colors_wrapper), bg_label,  0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(colors_wrapper), bg_button, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(colors_wrapper), fg_label,  2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(colors_wrapper), fg_button, 3, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), native,         0, row, 3, 1);
    row++;
    gtk_grid_attach(GTK_GRID(grid), keep_open,      0, row, 3, 1);
    row++;
    gtk_grid_attach(GTK_GRID(grid), refresh,        0, row, 3, 1);
    row++;
    gtk_grid_attach(GTK_GRID(grid), rmon_enable,    0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), rmon_addr,      1, row, 2, 1);
    row++;
    gtk_grid_attach(GTK_GRID(grid), bmon_enable,    0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), bmon_addr,      1, row, 2, 1);
    row++;
    gtk_grid_attach(GTK_GRID(grid), log_enable,     0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), log_name,       1, row, 2, 1);
    row++;
    gtk_grid_attach(GTK_GRID(grid), scroll_label,   0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), scroll_lines,   1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), scroll_info,    2, row, 1, 1);
    row++;
#ifdef FEATURE_CPUMEMHISTORY
    gtk_grid_attach(GTK_GRID(grid), chis_label,     0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), chis_lines,     1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), chis_warning,   2, row, 1 ,1);
    row++;
#endif
    gtk_grid_attach(GTK_GRID(grid), font_label,     0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), font_button,    1, row, 2, 1);
    row++;
    gtk_grid_attach(GTK_GRID(grid), colors_label,   0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), colors_wrapper, 1, row, 2, 1);
    gtk_widget_show_all(grid);
    return grid;
}
