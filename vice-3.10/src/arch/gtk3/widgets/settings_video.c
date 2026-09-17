/** \file   settings_video.c
 * \brief   Settings widget to control video settings
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES CrtcDoubleSize      xcbm2 xpet
 * $VICERES CrtcDoubleScan      xcbm2 xpet
 * $VICERES CrtcStretchVertical xcbm2 xpet
 * $VICERES CrtcAudioLeak       xcbm2 xpet
 * $VICERES TEDDoubleSize       xplus4
 * $VICERES TEDDoubleScan       xplus4
 * $VICERES TEDAudioLeak        xplus4
 * $VICERES VDCDoubleSize       x128
 * $VICERES VDCDoubleScan       x128
 * $VICERES VDCStretchVertical  x128
 * $VICERES VDCAudioLeak        x128
 * $VICERES VICDoubleSize       xvic
 * $VICERES VICDoubleScan       xvic
 * $VICERES VICAudioLeak        xvic
 * $VICERES VICIIDoubleSize     x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIDoubleScan     x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIAudioLeak      x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIICheckSbColl    x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIICheckSsColl    x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIVSPBug         x64sc xscpu64
 * $VICERES C128HideVDC         x128
 *
 *  (see included widgets for more resources)
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
#include <string.h>


#include "debug_gtk3.h"
#include "machine.h"
#include "ui.h"
#include "uivideo.h"
#include "vice_gtk3.h"
#include "videobordermodewidget.h"
#include "videopalettewidget.h"
#include "videorenderfilterwidget.h"

#include "settings_video.h"


/** \brief  double-size widgets
 *
 * Used in render_filter_callback() to synchronize the double size widgets'
 * state.
 */
static GtkWidget *double_size_widget[2] = { NULL, NULL };


/** \brief  Callback for changes of the render-filter widgets
 *
 * \param[in]   widget  radio group
 * \param[in]   value   new resource value
 */
static void render_filter_callback(GtkWidget *widget, int value)
{
    GtkWidget *parent;
    int        index;

    parent = gtk_widget_get_parent(widget);
    index  = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(parent), "ChipIndex"));
    vice_gtk3_resource_check_button_sync(double_size_widget[index]);
}

/** \brief  Callback for changes of the "Double Size" widget
 *
 * Trigger a resize of the primary or secondary window if \a state is false.
 *
 * There's no need to trigger a resize here when \a state is true, than happens
 * automatically by GDK to accomodate for the larger canvas.
 *
 * \param[in]   widget  check button (unused)
 * \param[in]   state   check button toggle state
 */
static void double_size_callback(GtkWidget *widget, gboolean state)
{
    if (!state && !ui_is_fullscreen()) {

        GtkWidget *window;
        int index;

        /* Note:
         *
         * We cannot use `ui_get_active_window()` here: it returns the settings
         * window, not the primary or secondary window. Even if we could get
         * the active primary/secondary window it wouldn't help since that
         * would resize the window that spawned the settings window.
         * For example: spawning settings from the VDC window and toggling the
         * VICII double-size off would result in the VDC window getting resized.
         *
         * --compyx
         */

        /* get index passed in the main widget's constructor */
        index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "ChipIndex"));
        window = ui_get_window_by_index(index);
        if (window != NULL) {
            gtk_window_resize(GTK_WINDOW(window), 1, 1);
        }
    }
}

/** \brief  Event handler for the 'Hide VDC Window' checkbox
 *
 * \param[in]   check   checkbutton triggering the event
 * \param[in]   data    settings dialog
 */
static void on_hide_vdc_toggled(GtkWidget *check, gpointer data)
{
    GtkWidget *window;
    gboolean   hide;

    hide = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
    window = ui_get_window_by_index(SECONDARY_WINDOW);  /* VDC */
    if (window != NULL) {
        if (hide) {
            /* close setting dialog on VDC window */
            if (ui_get_main_window_index() == SECONDARY_WINDOW) {
                gtk_window_close(GTK_WINDOW(data));
            }
            /* hide VDC window and show VICII window */
            gtk_widget_hide(window);
            window = ui_get_window_by_index(PRIMARY_WINDOW);    /* VICII */
            gtk_window_present(GTK_WINDOW(window));
        } else {
            gtk_widget_show(window);
        }
    }
}


/** \brief  Create left-indented label with Pango markup
 *
 * \param[in]   text    text for label (Pango markup allowed)
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

/** \brief  Create "Double Size" checkbox
 *
 * \param[in]   chip    chip name
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_double_size_widget(const char *chip)
{
    GtkWidget *widget;

    widget = vice_gtk3_resource_check_button_new_sprintf("%sDoubleSize",
                                                         "Double size",
                                                         chip);
    vice_gtk3_resource_check_button_add_callback(widget,
                                                 double_size_callback);
    return widget;
}

/** \brief  Create "Double Scan" checkbox
 *
 * \param[in]   chip    chip name
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_double_scan_widget(const char *chip)
{
    return vice_gtk3_resource_check_button_new_sprintf("%sDoubleScan",
                                                       "Double scan",
                                                       chip);
}

/** \brief  Create "Vertical Stretch" checkbox
 *
 * \param[in]   chip    chip name
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_vert_stretch_widget(const char *chip)
{
    return vice_gtk3_resource_check_button_new_sprintf("%sStretchVertical",
                                                       "Stretch vertically",
                                                       chip);
}

/** \brief  Create "Audio leak emulation" checkbox
 *
 * \param[in]   chip    chip name
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_audio_leak_widget(const char *chip)
{
    return vice_gtk3_resource_check_button_new_sprintf("%sAudioLeak",
                                                       "Audio leak emulation",
                                                       chip);
}

/** \brief  Create "Sprite-sprite collisions" checkbox
 *
 * \param[in]   chip    chip name
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_sprite_sprite_widget(const char *chip)
{
    return vice_gtk3_resource_check_button_new_sprintf("%sCheckSsColl",
                                                       "Sprite-sprite collisions",
                                                       chip);
}

/** \brief  Create "Sprite-background collisions" checkbox
 *
 * \param[in]   chip    chip name
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_sprite_background_widget(const char *chip)
{
    return vice_gtk3_resource_check_button_new_sprintf("%sCheckSbColl",
                                                       "Sprite-background collisions",
                                                       chip);
}

/** \brief  Create "VSP bug emulation" checkbox
 *
 * \param[in]   chip    chip name
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_vsp_bug_widget(const char *chip)
{
    return vice_gtk3_resource_check_button_new_sprintf("%sVSPBug",
                                                       "VSP bug emulation",
                                                       chip);
}

/** \brief  Create widget for double size, double scan and vertical stretch
 *
 * \param[in]   index   chip index (using in x128)
 * \param[in]   chip    chip name
 *
 * \return  GtkGrid
 */
static GtkWidget *create_render_widget(int index, const char *chip)
{
    GtkWidget *grid;
    GtkWidget *double_scan = NULL;
    GtkWidget *vert_stretch = NULL;
    int        row = 0;

    grid = gtk_grid_new();

    double_size_widget[index] = create_double_size_widget(chip);
    g_object_set_data(G_OBJECT(double_size_widget[index]),
                               "ChipIndex",
                               GINT_TO_POINTER(index));
    gtk_grid_attach(GTK_GRID(grid), double_size_widget[index], 0, row, 1, 1);
    row++;

    double_scan = create_double_scan_widget(chip);
    gtk_grid_attach(GTK_GRID(grid), double_scan, 0, row, 1, 1);
    row++;

    if (uivideo_chip_has_vert_stretch(chip)) {
        vert_stretch = create_vert_stretch_widget(chip);
        gtk_widget_set_margin_start(vert_stretch, 8);
        gtk_grid_attach(GTK_GRID(grid), vert_stretch, 0, row, 1, 1);
    }

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create widget for audio leak, sprite collisions and VSP bug
 *
 * \param[in]   index   chip index (using in x128)
 * \param[in]   chip    chip name
 *
 * \return  GtkGrid
 */
static GtkWidget *create_misc_widget(int index, const char *chip)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *audio_leak;
    int        row = 0;

    grid = gtk_grid_new();

    /* header */
    label = label_helper("<b>Miscellaneous</b>");
    gtk_widget_set_margin_bottom(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    row++;

    /* audio leak emluation */
    audio_leak = create_audio_leak_widget(chip);
    gtk_grid_attach(GTK_GRID(grid), audio_leak, 0, row, 1, 1);
    row++;

    if (uivideo_chip_has_sprites(chip)) {
        GtkWidget *ss_col;
        GtkWidget *sb_col;

        ss_col = create_sprite_sprite_widget(chip);
        gtk_grid_attach(GTK_GRID(grid), ss_col, 0, row, 1, 1);
        row++;

        sb_col = create_sprite_background_widget(chip);
        gtk_grid_attach(GTK_GRID(grid), sb_col, 0, row, 1, 1);
        row++;
    }
    if (uivideo_chip_has_vsp_bug(chip)) {
        GtkWidget *vsp_bug = create_vsp_bug_widget(chip);

        gtk_grid_attach(GTK_GRID(grid), vsp_bug, 0, row, 1, 1);
    }
    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create a per-chip video settings layout
 *
 * \param[in]   parent  parent widget, required for dialogs and window switching
 * \param[in]   chip    chip name ("Crtc", "TED", "VDC", "VIC", or "VICII")
 * \param[in]   index   index in the general layout (0 or 1 (x128))
 *
 * \return  GtkGrid
 */
static GtkWidget *create_layout(GtkWidget *parent, const char *chip, int index)
{
    GtkWidget *layout;
    GtkWidget *label;
    GtkWidget *render;
    GtkWidget *palette;
    GtkWidget *filter;
    GtkWidget *border;
    GtkWidget *misc;
    GtkWidget *hide_vdc;
    char       title[64];
    int        row = 0;

    layout = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(layout), 8);

    /* row 0, col 0-2: title */
    g_snprintf(title, sizeof title, "<b>%s Settings</b>", chip);
    label = label_helper(title);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_grid_attach(GTK_GRID(layout), label, 0, row, 3, 1);
    row++;

    /* row 1, col 0-2: video rendering options */
    render = create_render_widget(index, chip);
    gtk_grid_attach(GTK_GRID(layout), render, 0, row, 3, 1);
    row++;

    /* row 2, col 0-2: palette selection */
    palette = video_palette_widget_create(chip);
    gtk_widget_set_margin_top(palette, 16);
    gtk_widget_set_margin_bottom(palette, 16);
    gtk_grid_attach(GTK_GRID(layout), palette, 0, row, 3, 1);
    row++;

    /* row 3, col 0: rendering filter */
    filter = video_render_filter_widget_create(chip);
    g_object_set_data(G_OBJECT(filter), "ChipIndex", GINT_TO_POINTER(index));
    video_render_filter_widget_add_callback(filter, render_filter_callback);
    gtk_grid_attach(GTK_GRID(layout), filter, 0, row, 1, 1);

    /* row 3, col 1: border mode  */
    if (uivideo_chip_has_border_mode(chip)) {
        border = video_border_mode_widget_create(chip);
        gtk_grid_attach(GTK_GRID(layout), border, 1, row, 1, 1);
    }
    /* row 3, col 2: misc options */
    misc = create_misc_widget(index, chip);
    gtk_grid_attach(GTK_GRID(layout), misc, 2, row, 1, 1);
    row++;

    /* Hide VDC checkbox (x128 only) */
    if (machine_class == VICE_MACHINE_C128 && g_strcmp0(chip, "VDC") == 0) {
        hide_vdc = vice_gtk3_resource_check_button_new("C128HideVDC",
                                                       "Hide VDC display");
        gtk_widget_set_margin_top(hide_vdc, 16);
        g_signal_connect_unlocked(G_OBJECT(hide_vdc),
                                  "toggled",
                                  G_CALLBACK(on_hide_vdc_toggled),
                                  (gpointer)parent);
        gtk_grid_attach(GTK_GRID(layout), hide_vdc, 0, row, 3, 1);
    }
    gtk_widget_show_all(layout);
    return layout;
}


/** \brief  Create video settings widget
 *
 * This will create the VICII widget for x128, for the VDC widget use
 * settings_video_create_vdc().
 *
 * \param[in]   parent  settings dialog
 *
 * \return  GtkGrid
 */
GtkWidget *settings_video_widget_create(GtkWidget *parent)
{
    const char *chip = uivideo_chip_name();

    double_size_widget[0] = NULL;
    double_size_widget[1] = NULL;

    return create_layout(parent, chip, PRIMARY_WINDOW);
}


/** \brief  Create video settings widget for VDC
 *
 * This will create the VDC widget for x128, for the VICII widget use
 * settings_video_create().
 *
 * \param[in]   parent  settings dialog
 *
 * \return  GtkGrid
 */
GtkWidget *settings_video_widget_create_vdc(GtkWidget *parent)
{
    return create_layout(parent, "VDC", SECONDARY_WINDOW);
}
