/** \file   joystickautofirewidget.c
 * \brief   Widget to set joystick autofire resources
 *
 * Allows setting the autofire options for a joystick.
 *
 * Three settings are available:
 *  - autofire enable   (resource JoyStick[1-10]AutoFire)
 *  - autofire mode     (resource JoyStick[1-10]AutoFireMode)
 *  - autofire speed    (resource Joystick[1-10]AutoFireSpeed)
 *
 * Autofire mode has two settings:
 * - autofire when button is pressed (0)
 * - autofire when button is not pressed (1)
 *
 * Autofire speed is in button presses per second, with a range of 1-255
 * inclusive.
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES JoyStick1AutoFire       -vsid -xpet -xcbm2
 * $VICERES JoyStick2AutoFire       -vsid -xpet -xcbm2 -xvic
 * $VICERES JoyStick3AutoFire       -vsid
 * $VICERES JoyStick4AutoFire       -vsid
 * $VICERES JoyStick5AutoFire       -vsid -xpet
 * $VICERES JoyStick6AutoFire       -vsid -xpet
 * $VICERES JoyStick7AutoFire       -vsid -xpet -xplus4
 * $VICERES JoyStick8AutoFire       -vsid -xpet -xplus4
 * $VICERES JoyStick9AutoFire       -vsid -xpet -xplus4
 * $VICERES JoyStick10AutoFire      -vsid -xpet -xplus4
 * $VICERES JoyStick11AutoFire      xplus4
 *
 * $VICERES JoyStick1AutoFireMode   -vsid -xpet -xcbm2
 * $VICERES JoyStick2AutoFireMode   -vsid -xpet -xcbm2 -xvic
 * $VICERES JoyStick3AutoFireMode   -vsid
 * $VICERES JoyStick4AutoFireMode   -vsid
 * $VICERES JoyStick5AutoFireMode   -vsid -xpet
 * $VICERES JoyStick6AutoFireMode   -vsid -xpet
 * $VICERES JoyStick7AutoFireMode   -vsid -xpet -xplus4
 * $VICERES JoyStick8AutoFireMode   -vsid -xpet -xplus4
 * $VICERES JoyStick9AutoFireMode   -vsid -xpet -xplus4
 * $VICERES JoyStick10AutoFireMode  -vsid -xpet -xplus4
 * $VICERES JoyStick11AutoFireMode  xplus4
 *
 * $VICERES JoyStick1AutoFireSpeed  -vsid -xpet -xcbm2
 * $VICERES JoyStick2AutoFireSpeed  -vsid -xpet -xcbm2 -xvic
 * $VICERES JoyStick3AutoFireSpeed  -vsid
 * $VICERES JoyStick4AutoFireSpeed  -vsid
 * $VICERES JoyStick5AutoFireSpeed  -vsid -xpet
 * $VICERES JoyStick6AutoFireSpeed  -vsid -xpet
 * $VICERES JoyStick7AutoFireSpeed  -vsid -xpet -xplus4
 * $VICERES JoyStick8AutoFireSpeed  -vsid -xpet -xplus4
 * $VICERES JoyStick9AutoFireSpeed  -vsid -xpet -xplus4
 * $VICERES JoyStick10AutoFireSpeed -vsid -xpet -xplus4
 * $VICERES JoyStick11AutoFireSpeed xplus4
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

#include "joystick.h"
#include "vice_gtk3.h"

#include "joystickautofirewidget.h"

/* rows of widgets in the main grid */
#define ROW_TOP_BAR         0
#define ROW_MODE_LABEL      1
#define ROW_MODE_WIDGET     1
#define ROW_SPEED_LABEL     2
#define ROW_SPEED_WIDGET    2

/* columns of widgets in the main grid */
#define COL_TOP_BAR         0
#define COL_MODE_LABEL      0
#define COL_MODE_WIDGET     1
#define COL_SPEED_LABEL     0
#define COL_SPEED_WIDGET    1

/** \brief  Coordinates of widgets in a grid */
typedef struct coord_s {
    int column; /**< column index */
    int row;    /**< row index */
} coord_t;

/** \brief  Coordinates of widgets which will be enabled/disabled
 *
 * Might seem overkill, but when the layout changes again, this should make
 * things a bit easier.
 */
static const coord_t coords[] = {
    { COL_MODE_LABEL,   ROW_MODE_LABEL },
    { COL_MODE_WIDGET,  ROW_MODE_WIDGET },
    { COL_SPEED_LABEL,  ROW_SPEED_LABEL },
    { COL_SPEED_WIDGET, ROW_SPEED_WIDGET }
};


/** \brief  List of autofire modes
 *
 * \note    Currently only two modes, so perhaps a different widget can be used,
 *          such as a switch or toggle button?
 */
static const vice_gtk3_radiogroup_entry_t autofire_modes[] = {
    { "Pressed",     JOYSTICK_AUTOFIRE_MODE_PRESS },
    { "Released",    JOYSTICK_AUTOFIRE_MODE_PERMANENT },
    VICE_GTK3_RADIOGROUP_ENTRY_LIST_END
};

/** \brief  Set sensitivity of the button mode and speed widgets
 *
 * \param[in]   grid        autofire settings grid
 * \param[in]   sensitive   sensitivity for the widgets
 */
static void set_widgets_sensitivity(GtkWidget *grid,
                                    gboolean   sensitive)
{
    int index;

    for (index = 0; index < G_N_ELEMENTS(coords); index++) {
        GtkWidget *widget = gtk_grid_get_child_at(GTK_GRID(grid),
                                                  coords[index].column,
                                                  coords[index].row);
        if (widget != NULL) {
            gtk_widget_set_sensitive(widget, sensitive);
        }
    }
}

/** \brief  Create left-aligned label using Pango markup
 *
 * \param[in]   text    label text
 *
 * \return  GtkLabel
 */
static GtkWidget *create_label(const char *text)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}

/** \brief  Handler for the 'notify:active' event of the autofire enable switch
 *
 * Extra event handler for setting the other widgets' sensitivity base on the
 * state of the switch.
 *
 * \param[in]   widget  autofire enable switch
 * \param[in]   joy     joyport number (unused)
 */
static void on_autofire_notify_active(GtkWidget *widget, gpointer joy)
{
    GtkWidget *bar = gtk_widget_get_parent(widget);
    if (bar != NULL) {
        GtkWidget *grid = gtk_widget_get_parent(bar);
        if (grid != NULL) {
            set_widgets_sensitivity(grid,
                                    gtk_switch_get_active(GTK_SWITCH(widget)));
        }
    }
}

/** \brief  Create widget to toggle autofire for a joystick
 *
 * \param[in]   joy joystick number (1-10)
 *
 * \return  GtkSwitch
 */
static GtkWidget *create_autofire_enable_widget(int joy)
{
    GtkWidget *widget;

    widget = vice_gtk3_resource_switch_new_sprintf("JoyStick%dAutoFire", joy);
    gtk_widget_set_halign(widget, GTK_ALIGN_END);
    gtk_widget_set_valign(widget, GTK_ALIGN_START);
    gtk_widget_set_hexpand(widget, FALSE);
    gtk_widget_set_vexpand(widget, FALSE);
    g_signal_connect_unlocked(widget,
                              "notify::active",
                              G_CALLBACK(on_autofire_notify_active),
                              GINT_TO_POINTER(joy));
    return widget;
}

/** \brief  Create widget to set the autofire mode of a joystick
 *
 * \param[in]   joy joystick number (1-10)
 *
 * \return  GtkGrid
 */
static GtkWidget *create_autofire_mode_widget(int joy)
{
    GtkWidget *widget;

    widget = vice_gtk3_resource_radiogroup_new_sprintf("JoyStick%dAutoFireMode",
                                                       autofire_modes,
                                                       GTK_ORIENTATION_HORIZONTAL,
                                                       joy);
    return widget;
}

/** \brief  Create widget to set the autofire speed of a joystick
 *
 * \param[in]   joy joystick number (1-10)
 *
 * \return  GtkScale
 */
static GtkWidget *create_autofire_speed_widget(int joy)
{
    GtkWidget *widget;

    widget = vice_gtk3_resource_scale_custom_new_printf("JoyStick%dAutoFireSpeed",
                                                       GTK_ORIENTATION_HORIZONTAL,
                                                       JOYSTICK_AUTOFIRE_SPEED_MIN,
                                                       JOYSTICK_AUTOFIRE_SPEED_MAX,
                                                       JOYSTICK_AUTOFIRE_SPEED_MIN,
                                                       JOYSTICK_AUTOFIRE_SPEED_MAX,
                                                       5,
                                                       "%3.0fHz",
                                                       joy);
    gtk_scale_set_value_pos(GTK_SCALE(widget), GTK_POS_RIGHT);
    return widget;
}


/** \brief  Create widget to set autofire resources for a joystick
 *
 * \param[in]   joy     joystick number in the resource names (1-10)
 * \param[in]   title   title for the widget
 *
 * \return  GtkGrid
 */
GtkWidget *joystick_autofire_widget_create(int joy, const char *title)
{
    GtkWidget *grid;
    GtkWidget *bar;
    GtkWidget *label;
    GtkWidget *enable;
    GtkWidget *mode;
    GtkWidget *speed;
    char       text[64];

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

    /* create top "bar" with title and switch */
    bar = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(bar), TRUE);
    /* can't use more, otherwise the grid containing instances of this widget
     * grows too large: */
    gtk_widget_set_margin_bottom(bar, 4);

    g_snprintf(text, sizeof text, "<b>%s</b>", title);
    label  = create_label(text);
    enable = create_autofire_enable_widget(joy);
    gtk_grid_attach(GTK_GRID(bar), label,  0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(bar), enable, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), bar,   COL_TOP_BAR,      ROW_TOP_BAR,      2, 1);

    label = create_label("Button mode");
    mode = create_autofire_mode_widget(joy);
    gtk_grid_attach(GTK_GRID(grid), label, COL_MODE_LABEL,   ROW_MODE_LABEL,   1, 1);
    gtk_grid_attach(GTK_GRID(grid), mode,  COL_MODE_WIDGET,  ROW_MODE_WIDGET,  1, 1);

    label = create_label("Speed");
    speed = create_autofire_speed_widget(joy);
    gtk_grid_attach(GTK_GRID(grid), label, COL_SPEED_LABEL,  ROW_SPEED_LABEL,  1, 1);
    gtk_grid_attach(GTK_GRID(grid), speed, COL_SPEED_WIDGET, ROW_SPEED_WIDGET, 1, 1);
    gtk_widget_show_all(grid);

    /* set sensitivity based on the enable switch */
    set_widgets_sensitivity(grid, gtk_switch_get_active(GTK_SWITCH(enable)));
    return grid;
}
