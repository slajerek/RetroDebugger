/** \file   statusbarspeedwidget.c
 * \brief   CPU speed, FPS display, Pause, Warp widget for the statusbar
 *
 * Widget for the status bar that displays CPU speed, FPS and warp/pause state.
 *
 * When primary-button-clicking on the widget a menu will pop up allowing the
 * user to control refresh rate, emulation speed, warp and pause.
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
#include <math.h>
#include <string.h>

#include "vice_gtk3.h"
#include "basedialogs.h"
#include "drive.h"
#include "hotkeys.h"
#include "keyboard.h"
#include "lib.h"
#include "machine.h"
#include "petpia.h"
#include "resources.h"
#include "statusbarledwidget.h"
#include "uiapi.h"
#include "ui.h"
#include "uiactions.h"
#include "uimenu.h"
#include "uistatusbar.h"
#include "userport.h"
#include "vsync.h"
#include "vsyncapi.h"

#include "statusbarspeedwidget.h"


/** \brief  Predefined emulation speeds
 */
static int emu_speeds[][2] = {
    { 200, ACTION_SPEED_CPU_200 },
    { 100, ACTION_SPEED_CPU_100 },
    {  50, ACTION_SPEED_CPU_50 },
    {  25, ACTION_SPEED_CPU_25 },
    {  10, ACTION_SPEED_CPU_10 },
    {   0, ACTION_NONE }
};


/** \brief  Predefined emulation speed fps targets
 */
static int emu_fps_targets[][2] = {
    { 60, ACTION_SPEED_FPS_60 },
    { 50, ACTION_SPEED_FPS_50 },
    {  0, ACTION_NONE }
};


/** \brief  Add separator to \a menu
 *
 * Little helper function to add a separator item to a menu.
 *
 * \param[in,out]   menu    GtkMenu instance
 */
static void add_separator(GtkWidget *menu)
{
    GtkWidget *item = gtk_separator_menu_item_new();
    gtk_container_add(GTK_CONTAINER(menu), item);
}


static void trigger_ui_action(GtkWidget *item, gpointer action)
{
    ui_action_trigger(GPOINTER_TO_INT(action));
}


/** \brief  Create emulation speed submenu
 *
 * \return  GtkMenu
 */
static GtkWidget *emulation_speed_submenu_create(void)
{
    GtkWidget *menu;
    GtkWidget *item;
    char buffer[256];
    int curr_speed;
    int i;
    gboolean found = FALSE;

    resources_get_int("Speed", &curr_speed);

    menu = gtk_menu_new();

    /* cpu speed values */
    for (i = 0; emu_speeds[i][0] != 0; i++) {
        g_snprintf(buffer, sizeof(buffer), "%d%%", emu_speeds[i][0]);
        item = gtk_check_menu_item_new_with_label(buffer);
        gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(item), TRUE);
        if (curr_speed == emu_speeds[i][0]) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
            found = TRUE;
        }
        gtk_container_add(GTK_CONTAINER(menu), item);

        g_signal_connect(item,
                         "toggled",
                         G_CALLBACK(trigger_ui_action),
                         GINT_TO_POINTER(emu_speeds[i][1]));
    }

    /* custom speed */
    if (!found) {
        g_snprintf(buffer, sizeof(buffer), "Custom CPU speed (%d%%) ...", curr_speed);
        item = gtk_check_menu_item_new_with_label(buffer);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
    } else {
        item = gtk_check_menu_item_new_with_label("Custom CPU speed ...");
    }
    gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(item), TRUE);
    gtk_container_add(GTK_CONTAINER(menu), item);
    g_signal_connect(item,
                     "toggled",
                     G_CALLBACK(trigger_ui_action),
                     GINT_TO_POINTER(ACTION_SPEED_CPU_CUSTOM));

    /* fps targets */

    add_separator(menu);

    /* True emulated FPS (sets Speed to 100) */
    g_snprintf(buffer, sizeof(buffer), "%s FPS", machine_name);
    item = gtk_check_menu_item_new_with_label(buffer);
    gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(item), TRUE);
    if (curr_speed == 100) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
    }
    g_signal_connect(item,
                     "toggled",
                     G_CALLBACK(trigger_ui_action),
                     GINT_TO_POINTER(ACTION_SPEED_CPU_100));
    gtk_container_add(GTK_CONTAINER(menu), item);

    /* predefined fps targets */
    for (i = 0; emu_fps_targets[i][0] != 0; i++) {
        g_snprintf(buffer, sizeof(buffer), "%d FPS", emu_fps_targets[i][0]);
        item = gtk_check_menu_item_new_with_label(buffer);
        gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(item), TRUE);
        if (curr_speed == 0 - emu_fps_targets[i][0]) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
            found = TRUE;
        }
        gtk_container_add(GTK_CONTAINER(menu), item);

        g_signal_connect(item,
                         "toggled",
                         G_CALLBACK(trigger_ui_action),
                         GINT_TO_POINTER(emu_fps_targets[i][1]));
    }

    /* custom fps target */
    if (!found && curr_speed < 0) {
        g_snprintf(buffer, sizeof(buffer), "Custom (%d FPS) ...", 0 - curr_speed);
        item = gtk_check_menu_item_new_with_label(buffer);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
    } else {
        item = gtk_check_menu_item_new_with_label("Custom FPS ...");
    }
    gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(item), TRUE);
    gtk_container_add(GTK_CONTAINER(menu), item);
    g_signal_connect(item,
                     "toggled",
                     G_CALLBACK(trigger_ui_action),
                     GINT_TO_POINTER(ACTION_SPEED_FPS_CUSTOM));

    gtk_widget_show_all(menu);
    return menu;
}


/** \brief  Create popup menu for the statusbar speed widget
 *
 * \return  GtkMenu
 */
GtkWidget *speed_menu_popup_create(void)
{
    GtkWidget *menu;
    GtkWidget *submenu;
    GtkWidget *item;

    menu = gtk_menu_new();

    /* Emulation speed submenu */
    item = gtk_menu_item_new_with_label("Maximum speed");
    gtk_container_add(GTK_CONTAINER(menu), item);
    submenu = emulation_speed_submenu_create();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    add_separator(menu);

    /* pause */
    item = gtk_check_menu_item_new_with_label("Pause emulation");
    vhk_gtk_set_menu_item_accel_label(item, ACTION_PAUSE_TOGGLE);
    if (ui_pause_active()) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
    }
    gtk_container_add(GTK_CONTAINER(menu), item);
    g_signal_connect(item,
                     "toggled",
                     G_CALLBACK(trigger_ui_action),
                     GINT_TO_POINTER(ACTION_PAUSE_TOGGLE));

    /* advance frame */
    item = gtk_menu_item_new_with_label("Advance frame");
    vhk_gtk_set_menu_item_accel_label(item, ACTION_ADVANCE_FRAME);
    gtk_container_add(GTK_CONTAINER(menu), item);
    g_signal_connect(item,
                     "activate",
                     G_CALLBACK(trigger_ui_action),
                     GINT_TO_POINTER(ACTION_ADVANCE_FRAME));

    /* enable warp mode */
    item = gtk_check_menu_item_new_with_label("Warp mode");
    vhk_gtk_set_menu_item_accel_label(item, ACTION_WARP_MODE_TOGGLE);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), (gboolean)vsync_get_warp_mode());
    gtk_container_add(GTK_CONTAINER(menu), item);
    g_signal_connect(item,
                     "toggled",
                     G_CALLBACK(trigger_ui_action),
                     GINT_TO_POINTER(ACTION_WARP_MODE_TOGGLE));

    gtk_widget_show_all(menu);
    return menu;
}


/** \brief  Event handler for the mouse clicks on the speed widget
 *
 * \param[in]   widget  event box around the speed label
 * \param[in]   event   event
 * \param[in]   data    extra event data (unused)
 *
 * \return  TRUE when event was handled, FALSE otherwise
 */
static gboolean on_widget_clicked(GtkWidget *widget,
                                  GdkEvent *event,
                                  gpointer data)
{
    GdkEventButton *ev = (GdkEventButton *)event;

    if (ev->button == GDK_BUTTON_PRIMARY || ev->button == GDK_BUTTON_SECONDARY) {
        GtkWidget *menu = speed_menu_popup_create();
        gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
                GDK_GRAVITY_NORTH_WEST, GDK_GRAVITY_SOUTH_WEST,
                event);
        return TRUE;
    }
    return FALSE;
}


/** \brief  Reference to the alternate "hand" mouse pointer
 *
 * FIXME:   Do I need to clean this up somehow? Gdk docs aren't clear at all
 */
static GdkCursor *mouse_ptr;


/** \brief  Handler for the "enter/leave" events of the event box
 *
 * This changes the mouse cursor into a little "hand" when hovering over the
 * widget, to indicate to the user they can click on the widget.
 *
 * \param[in]   widget  widget triggering the event
 * \param[in]   event   event triggered
 * \param[in]   data    extra event data (unused)
 *
 * TODO: refactor, code can be simplified
 *
 * \return  TRUE if the event was handled
 */
static gboolean on_widget_hover(GtkWidget *widget,
                                GdkEvent *event,
                                gpointer data)
{
    if (event != NULL) {

        GdkDisplay *display;
        GdkWindow *window;
        int mouse;

        if (resources_get_int("Mouse", &mouse) < 0) {
            mouse = 0;
        }

        display = gtk_widget_get_display(widget);
        window = gtk_widget_get_window(widget);

        if (event->type == GDK_ENTER_NOTIFY) {
            if (display != NULL && mouse_ptr == NULL) {
                mouse_ptr = gdk_cursor_new_from_name(display, "pointer");
            }
            if (mouse_ptr != NULL && window != NULL) {
                gdk_window_set_cursor(window, mouse_ptr);
            }
        } else if (window != NULL) {
            gdk_window_set_cursor(window, NULL);
        }
        return TRUE;
    }
    return FALSE;
}


/** \brief  Create widget to display CPU/FPS/pause
 *
 * \param[in,out]   state   current widget state
 *
 * \return  GtkEventBox
 */
GtkWidget *statusbar_speed_widget_create(statusbar_speed_widget_state_t *state)
{
    GtkWidget *grid;
    GtkWidget *label_cpu;
    GtkWidget *label_fps;
#if 0
    GtkWidget *label_status;
#endif
    PangoContext *context;
    const PangoFontDescription *desc_static;
    PangoFontDescription *desc;
    GtkWidget *event_box;

    state->last_cpu_int = -1;
    state->last_fps_int = -1;
    state->last_paused = -1;
    state->last_warp = -1;
    state->last_shiftlock = -1;
    state->last_mode4080 = -1;
    state->last_diagnostic_pin = -1;

    grid = gtk_grid_new();
    gtk_widget_set_valign(grid, GTK_ALIGN_START);

    /* Use fixed width font to show cpu/fps, to avoid the displayed values
     * jumping around when being updated.
     *
     * A simpler way would be to use gtk_label_set_markup("<tt>...</tt>") in
     * statusbar_speed_widget_update(), but I fear that would eat more CPU
     * since the string needs to be parsed for special tags, and those tags
     * will probably internally do the Pango stuff I do here on every call.
     */

    /* label just for CPU  */
    label_cpu = gtk_label_new("");
    context = gtk_widget_get_pango_context(label_cpu);  /* don't free */
    desc_static = pango_context_get_font_description(context);
    desc = pango_font_description_copy_static(desc_static);
    pango_font_description_set_family(desc, "Consolas,monospace");
    pango_context_set_font_description(context, desc);
    pango_font_description_free(desc);
    gtk_widget_set_halign(label_cpu, GTK_ALIGN_START);
    gtk_widget_set_valign(label_cpu, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label_cpu, 0, 0, 1, 1);

    /* label just for FPS  */
    label_fps = gtk_label_new("");
    context = gtk_widget_get_pango_context(label_fps);  /* don't free */
    desc_static = pango_context_get_font_description(context);
    desc = pango_font_description_copy_static(desc_static);
    pango_font_description_set_family(desc, "Consolas,monospace");
    pango_context_set_font_description(context, desc);
    pango_font_description_free(desc);
    gtk_widget_set_halign(label_fps, GTK_ALIGN_START);
    gtk_widget_set_valign(label_fps, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label_fps, 0, 1, 1, 1);
#if 0
    /* label for pause/warp and perhaps CPU jam */
    label_status = gtk_label_new("");
    gtk_widget_set_halign(label_status, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), label_status, 0, 2, 1, 1);
#endif
    /* warp mode and pause LEDs */
#if 0
    wrapper = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(wrapper), 16);
    led_warp = statusbar_led_widget_create("warp:", "#00ff00", "#000");
    gtk_widget_set_halign(led_warp, GTK_ALIGN_START);
    led_pause = statusbar_led_widget_create("pause:", "#00ff00", "#000");
    gtk_widget_set_halign(led_pause, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(wrapper), led_warp, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(wrapper), led_pause, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), wrapper, 0, 2, 1, 1);
#endif
    /* create event box to capture mouse clicks to spawn popup menus */
    event_box = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(event_box), FALSE);
    gtk_container_add(GTK_CONTAINER(event_box), grid);
    gtk_widget_show_all(grid);

    if (machine_class != VICE_MACHINE_VSID) {
        g_signal_connect(event_box, "button-press-event",
                G_CALLBACK(on_widget_clicked), NULL);
        g_signal_connect(event_box, "enter-notify-event",
                G_CALLBACK(on_widget_hover), NULL);
        g_signal_connect(event_box, "leave-notify-event",
                G_CALLBACK(on_widget_hover), NULL);
    }
    return event_box;
}


/* Doxygen doesn't allow documenting #define's inside blocks, so we need to
 * work around that.
 */

/** \def    CPU_DECIMAL_PLACES
 *  \brief  CPU display decimals
 */

/** \def    FPS_DECIMAL_PLACES
 *  \brief  FPS display decimals
 */

/** \def    STR_
 *  \brief  Helper macro to allow concatenating string literals
 */

/** \def    STR
 *  \brief  Helper macro to allow concatenating string literals
 */


/** \brief  Update the speed widget's display state
 *
 * \param[in,out]   widget          GtkEventBox containing the CPU/FPS widgets
 * \param[in,out]   state           current widget state
 * \param[in]       window_identity window index (primary/secondary)
 */
void statusbar_speed_widget_update(GtkWidget *widget,
                                  statusbar_speed_widget_state_t *state,
                                  int window_identity)
{
#   define CPU_DECIMAL_PLACES 0
#   define FPS_DECIMAL_PLACES 1
#   define STR_(x) #x
#   define STR(x) STR_(x)

    static bool jammed = false;
    static bool drivejammed[NUM_DISK_UNITS] = { false, false, false, false };
    int drv;

    GtkWidget *grid = NULL;
    GtkWidget *label;
    char buffer[1024];

    double vsync_metric_cpu_percent;
    double vsync_metric_emulated_fps;
    int vsync_metric_warp_enabled;
    tick_t now;

    /*
     * FIXME: Don't redraw too often, as it will trigger layout issues and slow joystick widget redraw
     */

    now = tick_now();
    if (now - state->last_render_tick < tick_per_second() / 5) {
        return;
    }
    state->last_render_tick = now;

    /*
     * Jammed machines show the jam message instead of stats
     */

    if (machine_is_jammed()) {
        if (!jammed) {
#if 0
            char *temp = lib_strdup(machine_jam_reason());
            char *temp2 = strstr(temp, "JAM");
            jammed = true;

            grid = gtk_bin_get_child(GTK_BIN(widget));

            label = gtk_grid_get_child_at(GTK_GRID(grid), 0, 1);
            gtk_label_set_text(GTK_LABEL(label), temp2);
            *temp2 = 0;
            label = gtk_grid_get_child_at(GTK_GRID(grid), 0, 0);
            gtk_label_set_text(GTK_LABEL(label), temp);

            lib_free(temp);
#endif
            jammed = true;
            ui_display_statustext(machine_jam_reason(), false);
        }
        return;
    } else if (jammed) {
        /* machine is not jammed, but was jammed before */
        ui_display_statustext("", false);
        jammed = false;
    }

    for (drv = 0; drv < NUM_DISK_UNITS; drv++) {
        if (drive_is_jammed(drv)) {
            if (drivejammed[drv] == false) {
                drivejammed[drv] = true;
                ui_display_statustext(drive_jam_reason(drv), false);
            }
        } else if (drivejammed[drv] == true) {
            /* drive is not jammed, but was jammed before */
            ui_display_statustext("", false);
            drivejammed[drv] = false;
        }
    }

    vsyncarch_get_metrics(&vsync_metric_cpu_percent, &vsync_metric_emulated_fps, &vsync_metric_warp_enabled);

    /*
     * Updating GTK labels is expensive and this is called each frame,
     * so we avoid updates that wouldn't actually change the text.
     */

    int this_cpu_int = (int)(vsync_metric_cpu_percent  * pow(10, CPU_DECIMAL_PLACES) + 0.5);
    int this_fps_int = (int)(vsync_metric_emulated_fps * pow(10, FPS_DECIMAL_PLACES) + 0.5);
    bool is_paused = ui_pause_active();
    bool is_shiftlock = keyboard_get_shiftlock();
    bool is_mode4080 = false;
    bool is_capslock = false;
    bool is_diagnostic_pin = false;
    int updev = userport_get_device();

    if (machine_class == VICE_MACHINE_C128) {
        is_mode4080 = keyboard_custom_key_get(KBD_CUSTOM_4080);
        is_capslock = keyboard_custom_key_get(KBD_CUSTOM_CAPS);
    }

    if ((machine_class == VICE_MACHINE_PET) && (updev == USERPORT_DEVICE_DIAGNOSTIC_PIN)) {
        is_diagnostic_pin = pia1_get_diagnostic_pin();
    }

    if (state->last_cpu_int != this_cpu_int) {
        /* get grid containing the two labels */
        grid = gtk_bin_get_child(GTK_BIN(widget));

        /* get CPU label and update its text */
        label = gtk_grid_get_child_at(GTK_GRID(grid), 0, 0);
        g_snprintf(buffer,
                   sizeof(buffer),
                   "%7." STR(CPU_DECIMAL_PLACES) "f%% cpu",
                   vsync_metric_cpu_percent);
        gtk_label_set_text(GTK_LABEL(label), buffer);
        state->last_cpu_int = this_cpu_int;
    }

    /* Somehow the last state gets out of sync when pressing Alt+W and clicking
     * the warp led, or when pressing Alt+P and clicking the pause led, nearly
     * simultaneously, so we don't check for changes but always rerender the
     * leds.
     * See bug #1840: https://sourceforge.net/p/vice-emu/bugs/1840/
     */
#if 0
    /* warp */
    if (state->last_warp != vsync_metric_warp_enabled) {
        warp_led_set_active(window_identity, vsync_metric_warp_enabled);
        state->last_warp = vsync_metric_warp_enabled;
    }
    /* pause */
    if (state->last_paused != is_paused) {
        pause_led_set_active(window_identity, is_paused);
        state->last_paused = is_paused;
    }
    /* shiftlock */
    if (state->last_shiftlock != is_shiftlock) {
        shiftlock_led_set_active(window_identity, is_shiftlock);
        state->last_shiftlock = is_shiftlock;
    }
    /* 40/80 colums */
    if (state->last_mode4080 != is_mode4080) {
        mode4080_led_set_active(window_identity, is_mode4080);
        state->last_mode4080 = is_mode4080;
    }
    /* capslock */
    if (state->last_capslock != is_capslock) {
        capslock_led_set_active(window_identity, is_capslock);
        state->last_capslock = is_capslock;
    }
    /* userport diagnostic pin */
    if (state->last_diagnostic_pin != is_diagnostic_pin) {
        diagnosticpin_led_set_active(window_identity, is_diagnostic_pin);
        state->last_diagnostic_pin = is_diagnostic_pin;
    }
#else
    /* warp */
    warp_led_set_active(window_identity, vsync_metric_warp_enabled);
    state->last_warp = vsync_metric_warp_enabled;
    /* pause */
    pause_led_set_active(window_identity, is_paused);
    state->last_paused = is_paused;
    /* shiftlock */
    shiftlock_led_set_active(window_identity, is_shiftlock);
    state->last_shiftlock = is_shiftlock;
    /* 40/80 colums */
    mode4080_led_set_active(window_identity, is_mode4080);
    state->last_mode4080 = is_mode4080;
    /* capslock */
    capslock_led_set_active(window_identity, is_capslock);
    state->last_capslock = is_capslock;
    /* userport diagnostic pin */
    if (updev == USERPORT_DEVICE_DIAGNOSTIC_PIN) {
        diagnosticpin_led_set_visible(window_identity, 1);
        diagnosticpin_led_set_active(window_identity, is_diagnostic_pin);
        state->last_diagnostic_pin = is_diagnostic_pin;
    } else {
        diagnosticpin_led_set_visible(window_identity, 0);
    }
#endif

    if (window_identity == PRIMARY_WINDOW) {
        if (state->last_fps_int != this_fps_int) {

            if (grid == NULL) {
                grid = gtk_bin_get_child(GTK_BIN(widget));
            }

            /* get FPS label and update its text */
            label = gtk_grid_get_child_at(GTK_GRID(grid), 0, 1);

            g_snprintf(buffer,
                       sizeof(buffer),
                       "%8." STR(FPS_DECIMAL_PLACES) "f fps",
                       vsync_metric_emulated_fps);

            gtk_label_set_text(GTK_LABEL(label), buffer);

            state->last_fps_int = this_fps_int;
        }
    }

#   undef CPU_DECIMAL_PLACES
#   undef FPS_DECIMAL_PLACES
#   undef STR_
#   undef STR
}
