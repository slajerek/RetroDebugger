/** \file   ui.c
 * \brief   Native GTK3 UI stuff
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 * \author  Marcus Sutton <loggedoubt@gmail.com>
 *
 * $VICERES AutostartOnDoubleClick  all
 * $VICERES CrtcFullscreen          xcbm2 xpet
 * $VICERES TEDFullscreen           xplus4
 * $VICERES VDCFullscreen           x128
 * $VICERES VICFullscreen           xvic
 * $VICERES VICIIFullscreen         x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES MonitorBG               all
 * $VICERES MonitorFG               all
 * $VICERES MonitorFont             all
 * $VICERES MonitorXPos             all
 * $VICERES MonitorYPos             all
 * $VICERES MonitorWidth            all
 * $VICERES MonitorHeight           all
 * $VICERES Window0Height           -vsid
 * $VICERES Window0Width            -vsid
 * $VICERES Window0Xpos             -vsid
 * $VICERES Window0Ypos             -vsid
 * $VICERES Window1Height           x128
 * $VICERES Window1Width            x128
 * $VICERES Window1Xpos             x128
 * $VICERES Window1Ypos             x128
 * $VICERES StartMinimized          -vsid
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
#include <stdio.h>
#include <string.h>
#include <limits.h>
#ifdef UNIX_COMPILE
# include <unistd.h>
#endif
#ifdef MACOS_COMPILE
# include <objc/runtime.h>
# include <objc/message.h>
# include <CoreFoundation/CFString.h>
# include <CoreGraphics/CGGeometry.h>

/* The proper way to use objc_msgSend is to cast it into the right shape each time */
# define OBJC_MSGSEND(return_type, ...) ((return_type (*)(__VA_ARGS__))objc_msgSend)
# define OBJC_MSGSEND_STRET(...) ((void (*)(__VA_ARGS__))objc_msgSend_stret)
#endif

#include "archdep.h"
#include "attach.h"
#include "autostart.h"
#include "basedialogs.h"
#include "cmdline.h"
#include "cartridgewidgets.h"
#include "crtcontrolwidget.h"
#include "debug.h"
#include "debug_gtk3.h"
#include "drive.h"
#include "extendimagedialog.h"
/* for the fullscreen_capability() stub */
#include "fullscreen.h"
#include "hotkeys.h"
#include "interrupt.h"
#include "jamdialog.h"
#include "kbd.h"
#include "lib.h"
#include "lightpen.h"
#include "log.h"
#include "machine.h"
#include "mainlock.h"
#include "mixerwidget.h"
#include "monitor.h"
#include "resources.h"
#include "settings_keyboard.h"
#include "settings_rom.h"
#include "types.h"
#include "uiactions.h"
#include "uiapi.h"
#include "uicart.h"
#include "uidata.h"
#include "uidiskattach.h"
#include "uihotkeys.h"
#include "uimachinemenu.h"
#include "uimachinewindow.h"
#include "uimedia.h"
#include "uimenu.h"
#include "uimon.h"
#include "uisettings.h"
#include "uismartattach.h"
#include "uistatusbar.h"
#include "uitapeattach.h"
#include "util.h"
#include "videoarch.h"
#include "vsync.h"
#include "vsyncapi.h"
#include "widgethelpers.h"

/* UI action implementations */
#include "actions-cartridge.h"
#include "actions-clipboard.h"
#include "actions-datasette.h"
#ifdef DEBUG
# include "actions-debug.h"
#endif
#include "actions-display.h"
#include "actions-drive.h"
#include "actions-help.h"
#include "actions-hotkeys.h"
#include "actions-joystick.h"
#include "actions-machine.h"
#include "actions-media.h"
#include "actions-printer.h"
#include "actions-settings.h"
#include "actions-snapshot.h"
#include "actions-speed.h"

#include "ui.h"


/* Forward declarations of static functions
 *
 * Using `gboolean` in place of `int` is safe since gboolean is a typedef for
 * `gint`, which in turn is a typedef for `int`. `FALSE` is defined as `0`,
 * while `TRUE` is defined as `(!FALSE)`, resulting in `1` according to the
 * C standard.
 */

static int set_save_resources_on_exit(gboolean save_on_exit, void *unused);
static int set_confirm_on_exit(gboolean confirm_on_exit, void *unused);
static int set_window_height(gint height, void *window_index);
static int set_window_width(gint width, void *window_index);
static int set_window_xpos(gint xpos, void *window_index);
static int set_window_ypos(gint ypos, void *window_index);
static int set_start_minimized(int start_minimized, void *unused);
static int set_start_maximized(int start_minimized, void *unused);
static int set_native_monitor(gboolean use_native_monitorl, void *unused);
static int set_monitor_font(const gchar *font_description, void *unused);
static int set_monitor_bg(const gchar *color, void *unused);
static int set_monitor_fg(const gchar *color, void *unused);
static int set_fullscreen_decorations(gboolean fullscreen_decorations, void *unused);
static int set_pause_on_settings(gboolean pause_on_settings, void *unused);
static int set_autostart_on_doubleclick(gboolean autostart_on_doubleclick, void *unused);
static int set_monitor_xpos(const gchar *xpos, void *window_index);
static int set_monitor_ypos(const gchar *ypos, void *window_index);
static int set_monitor_width(const gchar *width, void *window_index);
static int set_monitor_height(const gchar *height, void *window_index);
static int set_settings_node_path(const gchar *path, void *unused);
static int window_index_from_param(void *param);
static void ui_action_dispatch(ui_action_map_t *);

/*****************************************************************************
 *                  Defines, enums, type declarations                        *
 ****************************************************************************/

/** \brief  List of drag targets for the drag-n-drop event handler
 *
 * It would appear different OS'es/WM's pass dropped files using various
 * mime-types.
 */
GtkTargetEntry ui_drag_targets[UI_DRAG_TARGETS_COUNT] = {
    { "text/plain",     0, DT_TEXT_PLAIN    },  /* we get this on at least my
                                                   Linux box with Mate */
    { "text/uri",       0, DT_TEXT_URI      },  /* not yet encountered */
    { "text/uri-list",  0, DT_TEXT_URI_LIST }   /* we get this using Windows
                                                   Explorer or macOS Finder */
};


/** \brief  Struct holding basic UI rescources
 */
typedef struct ui_resources_s {

    int save_resources_on_exit; /**< SaveResourcesOnExit (bool) */
    int confirm_on_exit;        /**< ConfirmOnExit (bool) */
    int pause_on_settings;      /**< PauseOnSettings (bool) */

    int start_minimized;        /**< StartMinimized (bool) */
    int start_maximized;        /**< StartMaximized (bool) */

    int use_native_monitor;     /**< NativeMonitor (bool) */

    char *monitor_font;         /**< Pango font description string of the
                                     VTE monitor font */
    char *monitor_bg;           /**< Monitor background color */
    char *monitor_fg;           /**< Monitor foreground color */
    int autostart_on_doubleclick;   /**< Use autostart on double-clicking in
                                         file attach dialogs (bool) */
#if 0
    int depth;
#endif

    video_canvas_t *canvas[NUM_WINDOWS];    /**< video canvases */
    GtkWidget *window_widget[NUM_WINDOWS];  /**< the toplevel GtkWidget (Window) */
    int window_width[NUM_WINDOWS];          /**< window widths */
    int window_height[NUM_WINDOWS];         /**< window heights */
    int window_xpos[NUM_WINDOWS];           /**< window x positions */
    int window_ypos[NUM_WINDOWS];           /**< window y positions */

} ui_resource_t;


/** \brief  Collection of UI resources
 *
 * This needs to stay here, to allow the command line and resources initializers
 * to reference the UI resources.
 */
static ui_resource_t ui_resources;

/** \brief  Flag inidicating whether fullscreen mode shows the decorations
 *
 * Used by the resource "FullscreenDecorations".
 */
static int fullscreen_has_decorations = 0;


/** \brief  Settings node to activate after booting the emulator
 *
 * Used by the `-settings-node` command line option
 */
static const char *settings_node_path = NULL;


/** \brief  Row numbers of the various widgets packed in a main GtkWindow
 */
enum {
    ROW_MENU_BAR = 0,   /**< application menu bar */
    ROW_DISPLAY,        /**< emulated display */
    ROW_STATUS_BAR,     /**< status bar */
    ROW_CRT_CONTROLS,   /**< CRT control widgets */
    ROW_MIXER_CONTROLS  /**< mixer control widgets */
};



/*****************************************************************************
 *                              Static data                                  *
 ****************************************************************************/

/** \brief  String type resources list
 */
static const resource_string_t resources_string[] = {
    /* VTE-monitor font */
    /* { "MonitorFont", "Mono 11", RES_EVENT_NO, NULL, */
    { "MonitorFont", "C64 Pro Mono Regular 9", RES_EVENT_NO, NULL,
        &ui_resources.monitor_font, set_monitor_font, NULL },
    /* we want this one to be "terminal white" but NOT "bright white" */
    { "MonitorFG", "#cccccc", RES_EVENT_NO, NULL,
        &ui_resources.monitor_fg, set_monitor_fg, NULL },
    { "MonitorBG", "#000000", RES_EVENT_NO, NULL,
        &ui_resources.monitor_bg, set_monitor_bg, NULL },

    RESOURCE_STRING_LIST_END
};


/** \brief  Boolean and integer resources shared between windows
 */
static const resource_int_t resources_int_shared[] = {
    { "SaveResourcesOnExit", 0, RES_EVENT_NO, NULL,
        &ui_resources.save_resources_on_exit, set_save_resources_on_exit, NULL },
    { "ConfirmOnExit", 1, RES_EVENT_NO, NULL,
        &ui_resources.confirm_on_exit, set_confirm_on_exit, NULL },

    { "StartMinimized", 0, RES_EVENT_NO, NULL,
        &ui_resources.start_minimized, set_start_minimized, NULL },
    { "StartMaximized", 0, RES_EVENT_NO, NULL,
        &ui_resources.start_maximized, set_start_maximized, NULL },

    { "NativeMonitor", 0, RES_EVENT_NO, NULL,
        &ui_resources.use_native_monitor, set_native_monitor, NULL },

    { "FullscreenDecorations", 0, RES_EVENT_NO, NULL,
        &fullscreen_has_decorations, set_fullscreen_decorations, NULL },

    { "PauseOnSettings", 0, RES_EVENT_NO, NULL,
        &ui_resources.pause_on_settings, set_pause_on_settings, NULL },
    /* Use autostart on doubleclick in dialogs */
    { "AutostartOnDoubleClick", 0, RES_EVENT_NO, NULL,
        &ui_resources.autostart_on_doubleclick, set_autostart_on_doubleclick,
        NULL },

    { "MonitorXPos", INT_MIN, RES_EVENT_NO, NULL,
        &(ui_resources.window_xpos[MONITOR_WINDOW]), set_window_xpos,
        (void*)MONITOR_WINDOW },
    { "MonitorYPos", INT_MIN, RES_EVENT_NO, NULL,
        &(ui_resources.window_ypos[MONITOR_WINDOW]), set_window_ypos,
        (void*)MONITOR_WINDOW },
    { "MonitorWidth", INT_MIN, RES_EVENT_NO, NULL,
        &(ui_resources.window_width[MONITOR_WINDOW]), set_window_width,
        (void*)MONITOR_WINDOW },
    { "MonitorHeight", INT_MIN, RES_EVENT_NO, NULL,
        &(ui_resources.window_height[MONITOR_WINDOW]), set_window_height,
        (void*)MONITOR_WINDOW },

    RESOURCE_INT_LIST_END
};


/** \brief  Window size and position resources for the primary window
 *
 * These are used by all emulators.
 */
static const resource_int_t resources_int_primary_window[] = {
    /* FIXME: this is a generic (not GTK specific) resource */
    { "Window0Height", INT_MIN, RES_EVENT_NO, NULL,
        &(ui_resources.window_height[PRIMARY_WINDOW]), set_window_height,
        (void*)PRIMARY_WINDOW },
    /* FIXME: this is a generic (not GTK specific) resource */
    { "Window0Width", INT_MIN, RES_EVENT_NO, NULL,
        &(ui_resources.window_width[PRIMARY_WINDOW]), set_window_width,
        (void*)PRIMARY_WINDOW },
    { "Window0Xpos", INT_MIN, RES_EVENT_NO, NULL,
        &(ui_resources.window_xpos[PRIMARY_WINDOW]), set_window_xpos,
        (void*)PRIMARY_WINDOW },
    { "Window0Ypos", INT_MIN, RES_EVENT_NO, NULL,
        &(ui_resources.window_ypos[PRIMARY_WINDOW]), set_window_ypos,
        (void*)PRIMARY_WINDOW },

    RESOURCE_INT_LIST_END
};


/** \brief  Window size and position resources list for the secondary window
 *
 * These are only used by x128's VDC window.
 */
static const resource_int_t resources_int_secondary_window[] = {
    /* FIXME: this is a generic (not GTK specific) resource */
    { "Window1Height", 0, RES_EVENT_NO, NULL,
        &(ui_resources.window_height[SECONDARY_WINDOW]), set_window_height,
        (void*)SECONDARY_WINDOW },
    /* FIXME: this is a generic (not GTK specific) resource */
    { "Window1Width", 0, RES_EVENT_NO, NULL,
        &(ui_resources.window_width[SECONDARY_WINDOW]), set_window_width,
        (void*)SECONDARY_WINDOW },
    { "Window1Xpos", 0, RES_EVENT_NO, NULL,
        &(ui_resources.window_xpos[SECONDARY_WINDOW]), set_window_xpos,
        (void*)SECONDARY_WINDOW },
    { "Window1Ypos", 0, RES_EVENT_NO, NULL,
        &(ui_resources.window_ypos[SECONDARY_WINDOW]), set_window_ypos,
        (void*)SECONDARY_WINDOW },

    RESOURCE_INT_LIST_END
};


/** \brief  Command line options shared between emu's, include VSID
 */
static const cmdline_option_t cmdline_options_common[] =
{
    { "-confirmonexit", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "ConfirmOnExit", (void *)1,
        NULL, "Confirm quitting VICE" },
    { "+confirmonexit", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "ConfirmOnExit", (void *)0,
        NULL, "Do not confirm quitting VICE" },
    { "-pauseonsettings", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "PauseOnSettings", (void *)1,
        NULL, "Pause emulation when activating settings dialog" },
    { "+pauseonsettings", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "PauseOnSettings", (void *)0,
        NULL, "Do not pause emulation when activating settings dialog" },
    { "-saveres", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "SaveResourcesOnExit", (void *)1,
        NULL, "Save settings on exit" },
    { "+saveres", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "SaveResourcesOnExit", (void *)0,
        NULL, "Do not save settings on exit" },
    { "-minimized", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "StartMinimized", (void *)1,
        NULL, "Start VICE minimized" },
    { "+minimized", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "StartMinimized", (void *)0,
        NULL, "Do not start VICE minimized" },
    { "-maximized", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "StartMaximized", (void*)1,
        NULL, "Start VICE maximized" },
    { "+maximized", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "StartMaximized", (void*)0,
        NULL, "Do not start VICE maximized" },
    { "-nativemonitor", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "NativeMonitor", (void *)1,
        NULL, "Use native monitor on OS terminal" },
    { "+nativemonitor", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "NativeMonitor", (void *)0,
        NULL, "Use VICE Gtk3 monitor terminal" },
    { "-fullscreen-decorations", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "FullscreenDecorations", (void*)1,
        NULL, "Enable fullscreen decorations" },
    { "+fullscreen-decorations", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "FullscreenDecorations", (void*)0,
        NULL, "Disable fullscreen decorations" },
    { "-monitorfont", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_font, NULL, "MonitorFont", NULL,
        "font-description", "Set monitor font for the Gtk3 monitor" },
    { "-monitorbg", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_bg,  NULL, "MonitorBG", NULL,
        "font-background", "Set monitor font background color" },
    { "-monitorfg", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_fg,  NULL, "MonitorFG", NULL,
        "font-foreground", "Set monitor font foreround color" },
    { "-autostart-on-doubleclick", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "AutostartOnDoubleClick", (void*)1,
        NULL, "Autostart files on doubleclick" },
    { "+autostart-on-doubleclick", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
        NULL, NULL, "AutostartOnDoubleClick", (void*)0,
        NULL, "Open files on doubleclick" },
    { "-settings-node", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
        set_settings_node_path, NULL, NULL, NULL,
        "settings-node", "Open settings dialog at <settings-node>" },
    { "-monitorxpos", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_xpos, (void*)MONITOR_WINDOW, "MonitorXPos", NULL,
        "X", "Set monitor window X position" },
    { "-monitorypos", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_ypos, (void*)MONITOR_WINDOW, "MonitorYPos", NULL,
        "Y", "Set monitor window Y position" },
    { "-monitorwidth", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_width, (void*)MONITOR_WINDOW, "MonitorWidth", NULL,
        "width", "Set monitor window width" },
    { "-monitorheight", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_height, (void*)MONITOR_WINDOW, "MonitorHeight", NULL,
        "height", "Set monitor window height" },
    /* Note: the following options are common/the same in SDL port */
    { "-windowxpos", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_xpos, (void*)PRIMARY_WINDOW, "Window0XPos", NULL,
        "X", "Set window X position" },
    { "-windowypos", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_ypos, (void*)PRIMARY_WINDOW, "Window0YPos", NULL,
        "Y", "Set window Y position" },
    { "-windowwidth", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_width, (void*)PRIMARY_WINDOW, "Window0Width", NULL,
        "width", "Set window width" },
    { "-windowheight", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_height, (void*)PRIMARY_WINDOW, "Window0Height", NULL,
        "height", "Set window height" },
    { "-windowxpos1", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_xpos, (void*)SECONDARY_WINDOW, "Window1XPos", NULL,
        "X", "Set secondary window X position" },
    { "-windowypos1", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_ypos, (void*)SECONDARY_WINDOW, "Window1YPos", NULL,
        "Y", "Set secondary window Y position" },
    { "-windowwidth1", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_width, (void*)SECONDARY_WINDOW, "Window1Width", NULL,
        "width", "Set secondary window width" },
    { "-windowheight1", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
        set_monitor_height, (void*)SECONDARY_WINDOW, "Window1Height", NULL,
        "height", "Set secondary window height" },
    CMDLINE_LIST_END
};


/** \brief  Flag indicating pause mode
 */
static int is_paused = 0;

/** \brief  Flag indicating that the monitor should be entered whilst paused
 */
static int enter_monitor_while_paused = 0;

/** \brief  Index of the most recently focused main window
 */
static int active_win_index = -1;

/** \brief  Function to handle files dropped on a main window
 */
static int (*handle_dropped_files_func)(const char *) = NULL;

/** \brief  Function to help create a main window
 */
static void (*create_window_func)(video_canvas_t *) = NULL;

/** \brief  Function to identify a canvas from its video chip
 */
static int (*identify_canvas_func)(video_canvas_t *) = NULL;

/** \brief  Function to help create a CRT controls widget
 */
static GtkWidget *(*create_controls_widget_func)(int) = NULL;



/******************************************************************************
 *                              Resource setters                              *
 *****************************************************************************/

/** \brief  Resource setter for "FullscreenDecorations"
 *
 * \param[in]   enabled enable fullscreen decorations
 * \param[in]   unused   extra argument (unused)
 *
 * \return 0
 */
static int set_fullscreen_decorations(gboolean fullscreen_decorations, void *unused)
{
    fullscreen_has_decorations = fullscreen_decorations;
    return 0;
}

/** \brief  Set SaveResourcesOnExit resource
 *
 * \param[in]   save_on_exit    save resources on emulator exit
 * \param[in]   unused          extra param (ignored)
 *
 * \return 0
 */
static int set_save_resources_on_exit(gboolean save_on_exit, void *unused)
{
    ui_resources.save_resources_on_exit = save_on_exit;
    return 0;
}

/** \brief  Set ConfirmOnExit resource
 *
 * \param[in]   confirm_on_exit pop up confirmation dialog on exit
 * \param[in]   unused          extra param (ignored)
 *
 * \return 0
 */
static int set_confirm_on_exit(gboolean confirm_on_exit, void *unused)
{
    ui_resources.confirm_on_exit = confirm_on_exit;
    return 0;
}

/** \brief  Set PauseOnSettings resource
 *
 * \param[in]   pause_on_settings   pause emulation when entering the settings
 *                                  dialog
 * \param[in]   unused              extra param (ignored)
 *
 * \return 0
 */
static int set_pause_on_settings(gboolean pause_on_settings, void *param)
{
    ui_resources.pause_on_settings = pause_on_settings;
    return 0;
}

/** \brief  Set AutostartOnDoubleClick resource
 *
 * \param[in]   autostart_on_doublelick autostart when doubleclicking in attach
 *                                      dialogs
 * \param[in]   unused                  extra param (ignored)
 *
 * \return 0
 */
static int set_autostart_on_doubleclick(gboolean autostart_on_doubleclick, void *unused)
{
    ui_resources.autostart_on_doubleclick = autostart_on_doubleclick;
    return 0;
}

/** \brief  Set StartMinimized resource
 *
 * \param[in]   start_minimized start the emulator window minimized
 * \param[in]   unused          extra param (ignored)
 *
 * \return  0 on success, -1 when StartMaximized is true
 */
static int set_start_minimized(int start_minimized, void *unused)
{
    if (start_minimized && ui_resources.start_maximized) {
        log_error(LOG_DEFAULT,
                  "cannot request both minimized and maximized window.");
        return -1;
    }
    ui_resources.start_minimized = start_minimized;
    return 0;
}

/** \brief  Set StartMaximized resource
 *
 * \param[in]   start_maximized start the emulator window maximized
 * \param[in]   unused          extra param (ignored)
 *
 * \return  0 on success, -1 when StartMinimized is true
 */
static int set_start_maximized(int start_maximized, void *unused)
{
    if (start_maximized && ui_resources.start_minimized) {
        log_error(LOG_DEFAULT,
                  "cannot request both minimized and maximized window.");
        return -1;
    }
    ui_resources.start_maximized = start_maximized;
    return 0;
}

/** \brief  Set NativeMonitor resource (bool)
 *
 * Use the spawning shell for the monitor instead of the VTE widget.
 *
 * \param[in]   use_native_monitor  use native monitor instead of VTE
 * \param[in]   unused              extra param (ignored)
 *
 * \return 0
 */
static int set_native_monitor(gboolean use_native_monitor, void *unused)
{
    /* FIXME: setting this to 1 should probably fail if either stdin or stdout
              is not a terminal. */
    /* this doesn't work on Windows (Surprise!) */
#if 0
    if (!isatty(stdin) || !isatty(stdout)) {
        return -1;
    }
#endif
    ui_resources.use_native_monitor = use_native_monitor;
    return 0;
}

/** \brief  Resource handler: set monitor font for VTE-based monitor
 *
 * \param[in]   font_description    font description, passed to Pango
 * \param[in]   unused              extra argument (unused)
 *
 * \return  0 (success)
 */
static int set_monitor_font(const gchar *font_description, void *param)
{
    util_string_set(&ui_resources.monitor_font, font_description);
    return 0;
}

/** \brief  Resource handler: set monitor background color for VTE-based monitor
 *
 * \param[in]   color   Gdk RGBA color string
 * \param[in]   unused  extra argument (unused)
 *
 * \return  0 on success, -1 if \a color could not be parsed by gdk_rgba_parse()
 */
static int set_monitor_bg(const gchar *color, void *param)
{
    GdkRGBA rgba;

    if (gdk_rgba_parse(&rgba, color)) {
        util_string_set(&ui_resources.monitor_bg, color);
        uimon_set_background_color(color);
        return 0;
    }
    return -1;
}

/** \brief  Resource handler: set monitor foreground color for VTE-based monitor
 *
 * \param[in]   color   Gdk RGBA color string
 * \param[in]   unused  extra argument (unused)
 *
 * \return  0 on success, -1 if \a color could not be parsed by gdk_rgba_parse()
 */
static int set_monitor_fg(const gchar *color, void *param)
{
    GdkRGBA rgba;

    if (gdk_rgba_parse(&rgba, color)) {
        util_string_set(&ui_resources.monitor_fg, color);
        uimon_set_foreground_color(color);
        return 0;
    }
    return -1;
}

/** \brief  Set Window[X]Width resource
 *
 * \param[in]   width           width in pixels
 * \param[in]   window_index    window index
 *
 * \return 0
 */
static int set_window_width(gint width, void *window_index)
{
    int index = window_index_from_param(window_index);
    if (index < 0) {
        return -1;
    }
    ui_resources.window_width[index] = width;
    return 0;
}

/** \brief  Set Window[X]Height resource
 *
 * \param[in]   height          height in pixels
 * \param[in]   window_index    window index
 *
 * \return 0
 */
static int set_window_height(gint height, void *window_index)
{
    int index = window_index_from_param(window_index);
    if (index < 0) {
        return -1;
    }
    ui_resources.window_height[index] = height;
    return 0;
}

/** \brief  Set Window[X]Xpos resource
 *
 * \param[in]   xpos            xpos in pixels
 * \param[in]   window_index    window index
 *
 * \return 0
 */
static int set_window_xpos(gint xpos, void *window_index)
{
    int index = window_index_from_param(window_index);
    if (index < 0) {
        return -1;
    }
    ui_resources.window_xpos[index] = xpos;
    return 0;
}

/** \brief  Set Window[X]Ypos resource (int)
 *
 * \param[in]   ypos            ypos in pixels
 * \param[in]   window_index    window index
 *
 * \return 0
 */
static int set_window_ypos(gint ypos, void *window_index)
{
    int index = window_index_from_param(window_index);
    if (index < 0) {
        return -1;
    }
    ui_resources.window_ypos[index] = ypos;
    return 0;
}

/** \brief  Cmdline handler for -monitorxpos, -windowxpos, -windowxpos1
 *
 * \param[in]   xpos            xpos in pixels as string
 * \param[in]   window_index    window index
 *
 * \return  0 on success
 */
static int set_monitor_xpos(const gchar *xpos, void *window_index)
{
    char *endptr;
    long result;

    result = strtol(xpos, &endptr, 0);
    if (*endptr != '\0') {
        return -1;
    }
    return set_window_xpos((gint)result, window_index);
}

/** \brief  Cmdline handler for -monitorypos, -windowypos, -windowypos1
 *
 * \param[in]   ypos            ypos in pixels as string
 * \param[in]   window_index    window index
 *
 * \return  0 on success
 */
static int set_monitor_ypos(const gchar *ypos, void *window_index)
{
    char *endptr;
    long result;

    result = strtol(ypos, &endptr, 0);
    if (*endptr != '\0') {
        return -1;
    }
    return set_window_ypos((gint)result, window_index);
}

/** \brief  Cmdline handler for -monitorwidth, -windowwidth, -windowwidth1
 *
 * \param[in]   width           width in pixels as string
 * \param[in]   window_index    window index
 *
 * \return  0 on success
 */
static int set_monitor_width(const gchar *width, void *window_index)
{
    char *endptr;
    long result;

    result = strtol(width, &endptr, 0);
    if (*endptr != '\0') {
        return -1;
    }
    return set_window_width((gint)result, window_index);
}

/** \brief  Cmdline handler for -monitorheight, -windowheight, -windowheight1
 *
 * \param[in]   height          height in pixels as string
 * \param[in]   window_index    window index
 *
 * \return  0 on success
 */
static int set_monitor_height(const gchar *height, void *window_index)
{
    char *endptr;
    long result;

    result = strtol(height, &endptr, 0);
    if (*endptr != '\0') {
        return -1;
    }
    return set_window_height((gint)result, window_index);
}



/******************************************************************************
 *                      Main window event handlers                            *
 *****************************************************************************/

/** \brief  Handler for the 'drag-drop' event of the GtkWindow(s)
 *
 * Can be used to filter certain drop targets or altering the data before
 * triggering the 'drag-drop-received' event. Currently just returns TRUE
 *
 * \param[in]   widget  widget triggering the event
 * \param[in]   context gtk drag context
 * \param[in]   x       x position of drag event
 * \param[in]   y       y position of drag event
 * \param[in]   time    (I don't have a clue)
 * \param[in]   data    extra event data (unused)
 *
 * \return  TRUE
 */
static gboolean on_drag_drop(GtkWidget *widget,
                             GdkDragContext *context,
                             gint x,
                             gint y,
                             guint time,
                             gpointer data)
{
    return TRUE;
}

/** \brief  Extract filename from drag data
 *
 * \param[in]   data    drag data from dropping media onto the emulator window
 * \param[in]   info    drag target mime type info
 *
 * \return  filename or \c NULL
 * \note    the filename should be freed with \c g_free() after use
 *
 * \todo    Perhaps make public, VSID uses duplicate code.
 */
static gchar *drag_data_get_filename(GtkSelectionData *data,
                                     guint             info)
{
    guchar  *text;
    gchar  **uri_list = NULL;
    gchar   *filename = NULL;

    switch (info) {

        case DT_TEXT_URI_LIST:  /* 'text/uri-list */
            /* Windows and MacOS */
            uri_list = gtk_selection_data_get_uris(data);
           break;

        case DT_TEXT_PLAIN:     /* 'text/plain' */
            /* Qt- and Gtk-based WM's on Linux */
            text = gtk_selection_data_get_text(data);
            /* text contains newline-separated lines of 'file://' URIs and a
             * trailing newline */
            g_strchomp((gchar *)text);
            uri_list = g_strsplit((const gchar *)text, "\n", -1);
            g_free(text);
            break;

        case DT_TEXT_URI:       /* fall through ('text/uri') */
        default:
            log_error(LOG_DEFAULT, "Unhandled drag target %u", info);
            break;
    }

    if (uri_list != NULL) {
        if (uri_list[0] != NULL) {
            /* grab first URI in list */
            filename = g_filename_from_uri(uri_list[0], NULL, NULL);
        }
        g_strfreev(uri_list);
    }

    return filename;
}

/** \brief  Handler for the 'drag-data-received' event
 *
 * Autostarts an image/prg when valid. Please note that VSID now has its own
 * drag-n-drop handlers.
 *
 * \param[in]   widget      widget triggering the event (unused)
 * \param[in]   context     drag context (unused)
 * \param[in]   x           probably X-coordinate in the drop target?
 * \param[in]   y           probablt Y-coordinate in the drop target?
 * \param[in]   data        dragged data
 * \param[in]   info        int declared in the targets array (unclear)
 * \param[in]   time        no idea
 */
static void on_drag_data_received(GtkWidget        *widget,
                                  GdkDragContext   *context,
                                  gint              x,
                                  gint              y,
                                  GtkSelectionData *data,
                                  guint             info,
                                  guint             time)
{
    gchar *filename = drag_data_get_filename(data, info);

    if (filename != NULL) {
        GdkDragAction action;
        int           drop_mode = 0;

        action = gdk_drag_context_get_selected_action(context);

        /* determine autostart behaviour */
        switch (action) {
            case GDK_ACTION_COPY: /* fall through */
            default:
                /* no modifier key held, use the resource value */
                resources_get_int("AutostartDropMode", &drop_mode);
                break;
            case GDK_ACTION_MOVE:
                /* Shift ("move"): attach image */
                drop_mode = AUTOSTART_DROP_MODE_ATTACH;
                break;
            case GDK_ACTION_LINK:
                /* Alt ("link"): attach and load  */
                drop_mode = AUTOSTART_DROP_MODE_LOAD;
                break;
        }

        switch (drop_mode) {
            case AUTOSTART_DROP_MODE_ATTACH:
                file_system_attach_disk(8, 0, filename);
                break;
            case AUTOSTART_DROP_MODE_LOAD:
                autostart_autodetect(filename, NULL, 0, AUTOSTART_MODE_LOAD);
                break;
            case AUTOSTART_DROP_MODE_RUN:
                autostart_autodetect(filename, NULL, 0, AUTOSTART_MODE_RUN);
                break;
            default:
                break;
        }
        g_free(filename);
    }
}

/** \brief  Handler for the 'delete-event' of a main window
 *
 * \param[in]   widget      window triggering the event (unused)
 * \param[in]   event       event details (unused)
 * \param[in]   user_data   extra data for the event (unused)
 *
 * \return  TRUE, if the function returns at all
 */
static gboolean on_delete_event(GtkWidget *widget,
                                GdkEvent *event,
                                gpointer user_data)
{
    ui_action_trigger(ACTION_QUIT);
    return TRUE;
}

/** \brief  Handler for the "focus-in-event" of a main window
 *
 * \param[in]   widget      window triggering the event
 * \param[in]   event       window focus details
 * \param[in]   user_data   extra data for the event (ignored)
 *
 * \return  FALSE to continue processing
 *
 * \note    We only use this for canvas-window-specific stuff like
 *          fullscreen mode.
 */
static gboolean on_focus_in_event(GtkWidget *widget, GdkEventFocus *event,
                                  gpointer user_data)
{
    int index = ui_get_window_index(widget);

    ui_set_ignore_mouse_hide(FALSE);
    ui_mouse_grab_pointer();

    if (index < 0) {
        /* We should never end up here. */
        log_error(LOG_DEFAULT, "focus-in-event: window not found\n");
        archdep_vice_exit(1);
    }

    if (event->in == TRUE) {
        active_win_index = index;
    }

    return FALSE;
}

/** \brief  Handler for the "focus-out-event" of a main window
 *
 * \param[in]   widget      window triggering the event
 * \param[in]   event       window focus details
 * \param[in]   user_data   extra data for the event (ignored)
 *
 * \return  FALSE to continue processing
 *
 * \note    We only use this for canvas-window-specific stuff like
 *          fullscreen mode.
 */
static gboolean on_focus_out_event(GtkWidget *widget, GdkEventFocus *event,
                                  gpointer user_data)
{
    ui_set_ignore_mouse_hide(TRUE);
    ui_mouse_ungrab_pointer();
    return FALSE;
}

/** \brief  Handler for the "window-state-event" of a main window
 *
 * \param[in]   widget      window triggering the event
 * \param[in]   event       window state details
 * \param[in]   user_data   extra data for the event (ignored)
 *
 * \return  FALSE to continue processing
 *
 * \note    This handler is not used on VSID since we have neither fullscreen
 *          nor fullscreen decorations in VSID
 */
static gboolean on_window_state_event(GtkWidget *widget,
                                      GdkEventWindowState *event,
                                      gpointer user_data)
{
    GdkWindowState win_state = event->new_window_state;
    int index = ui_get_window_index(widget);
    gboolean is_fullscreen = ui_is_fullscreen();

    if (index < 0) {
        /* We should never end up here. */
        log_error(LOG_DEFAULT, "window-state-event: window not found\n");
        archdep_vice_exit(1);
    }

    if (win_state & GDK_WINDOW_STATE_FULLSCREEN) {
        if (!is_fullscreen) {
            ui_set_fullscreen_enabled(TRUE);
            ui_update_fullscreen_decorations();
        }
    } else {
        if (is_fullscreen) {
            ui_set_fullscreen_enabled(FALSE);
            ui_update_fullscreen_decorations();
        }
    }

    return FALSE;
}


/******************************************************************************
 *                          Other static functions                            *
 *****************************************************************************/

/** \brief  Create an icon by loading it from the vice.gresource file
 *
 * \return  App icon for the current machine
 *
 * \todo    Refactor to use arch/shared/archdep_icon_path.c
 */
static GdkPixbuf *get_default_icon(void)
{
    char buffer[256];


    /* machine_name for VSID is 'C64' to be able to load ROMs from data/C64 */
    if (machine_class == VICE_MACHINE_VSID) {
        strncpy(buffer, "SID.svg", sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
    } else {
        g_snprintf(buffer, sizeof(buffer), "%s.svg", machine_name);
    }

#ifdef MACOS_COMPILE
    /* The icon is SVG, so lets try to figure out the right size to render */
    id application;
    id dock_tile;
    CGSize dock_tile_size;

    application    = OBJC_MSGSEND(id, id, SEL)((id)objc_getClass("NSApplication"), sel_getUid("sharedApplication"));
    dock_tile      = OBJC_MSGSEND(id, id, SEL)(application, sel_getUid("dockTile"));
    dock_tile_size = OBJC_MSGSEND(CGSize, id, SEL)(dock_tile, sel_getUid("size"));

    return uidata_get_pixbuf_at_scale(buffer, dock_tile_size.width, dock_tile_size.height, true);
#else
    /* TODO: Can we figure out the right icon size on Windows, Linux? */
    return uidata_get_pixbuf(buffer);
#endif
}

/** \brief  Get a window-spec array index from \a param
 *
 * Also performs a bounds check and returns -1 on boundary violation.
 *
 * \param[in]   param   extra param passed to a setter
 *
 * \return  index in array or -1 on error
 */
static int window_index_from_param(void *param)
{
    int index = vice_ptr_to_int(param);
    return (index >= 0 && index < NUM_WINDOWS) ? index : -1;
}

/** \brief  Set settings node path to activate on UI startup
 *
 * Triggers opening the settings dialog at node \a path once when starting VICE.
 *
 * Useful for working on settings dialogs, avoiding clicking through the UI.
 * For example: `x64sc -settings-node peripheral/drive` will open the drive
 * settings.
 *
 * \param[in]   path    setting node path
 * \param[in]   unused  extra data (unused);
 *
 * \return  0
 */
static int set_settings_node_path(const gchar *path, void *param)
{
#if 0
    debug_gtk3("Activating settings node '%s'.", path);
#endif
    settings_node_path = path;
    return 0;   /* we won't know if the path is valid until later */
}


/* Dispatch function and its helper for the UI actions */


/** \brief  GSourceFunc to call a UI action
 *
 * \param[in]   data    UI action map
 *
 * \return  `G_SOURCE_REMOVE` to remove this timeout source
 */
static gboolean ui_action_dispatch_impl(gpointer data)
{
    ui_action_map_t *map = data;

    map->handler(map);
    return G_SOURCE_REMOVE;
}

/** \brief  Dispatcher for UI actions
 *
 * Executes UI action handler on the the UI thread if requested.
 *
 * \param[in]   map UI action map
 */
static void ui_action_dispatch(ui_action_map_t *map)
{
    if ((map->uithread || map->dialog) && mainlock_is_vice_thread()) {
        /* we're on the main thread and we need the UI thread: push to UI thread */
        gdk_threads_add_timeout(0, ui_action_dispatch_impl, (void*)map);
    } else {
        map->handler(map);
    }
}


/******************************************************************************
 *                              Public functions                              *
 *****************************************************************************/

/** \brief  Get the most recently focused toplevel window
 *
 * \return  pointer to a toplevel window, or NULL
 *
 * \note    Not an event handler, needs to be moved
 */
GtkWindow *ui_get_active_window(void)
{
    GtkWindow *window = NULL;
    GList *tlist = gtk_window_list_toplevels();
    GList *list = tlist;

    /* Find the window that has the toplevel focus. */
    while (list != NULL) {
        if (gtk_window_has_toplevel_focus(list->data)) {
            window = list->data;
            break;
        }
        list = list->next;
    }
    g_list_free(tlist);

    /* If no window has the toplevel focus, then fall back
     * to the most recently focused main window.
     */
    if (window == NULL
            && active_win_index >= 0 && active_win_index < NUM_WINDOWS) {
        window = GTK_WINDOW(ui_resources.window_widget[active_win_index]);
    }

    /* If "window" still is NULL, it probably means
     * that no windows have been created yet.
     */
    return window;
}


/** \brief  Get video canvas of active window
 *
 * \return  current active video canvas, or NULL
 */
video_canvas_t *ui_get_active_canvas(void)
{
    video_canvas_t *canvas;

    if (active_win_index < 0) {
        /* If we end up here it probably means no main window has
         * been created yet. */
        return NULL;
    }

    canvas = ui_resources.canvas[active_win_index];
    if (canvas == NULL) {
        log_error(LOG_DEFAULT, "No canvas for window %d!", active_win_index);
    }
    return canvas;
}


/** \brief  Get canvas by main window index
 *
 * \param[in]   index   window index (`PRIMARY_WINDOW` or `SECONDARY_WINDOW`)
 *
 * \return  video canvas or `NULL` on error
 */
video_canvas_t *ui_get_canvas_for_window(int index)
{
    if (index < PRIMARY_WINDOW || index > SECONDARY_WINDOW) {
        return NULL;
    }
    return ui_resources.canvas[index];
}


/** \brief  Get the active main window's index
 *
 * \return  index of a main emulator window
 */
int ui_get_main_window_index(void)
{
    /* SOMETHING CHANGED */
    return active_win_index;
}


/** \brief  Get active main window
 *
 * \param[in]   index   window index (PRIMARY WINDOW or SECONDARY_WINDOW)
 *
 * \return  window or `NULL` when \a index is out of bounds
 */
GtkWidget *ui_get_main_window_by_index(gint index)
{
    if (index == PRIMARY_WINDOW || index == SECONDARY_WINDOW) {
        return ui_resources.window_widget[index];
    }
    return NULL;
}


/** \brief  Get a window's index
 *
 * \param[in]   widget      window to get the index of
 *
 * \return  window index, or -1 if not a main window
 */
int ui_get_window_index(GtkWidget *widget)
{
    if (widget == NULL) {
        return -1;
    } else if (widget == ui_resources.window_widget[PRIMARY_WINDOW]) {
        return PRIMARY_WINDOW;
    } else if (widget == ui_resources.window_widget[SECONDARY_WINDOW]) {
        return SECONDARY_WINDOW;
    } else {
        return -1;
    }
}


/** \brief Show or hide the decorations of the active main window as needed
 */
void ui_update_fullscreen_decorations(void)
{
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *menu_bar;
    GtkWidget *crt_grid;
    GtkWidget *mixer_grid;
    GtkWidget *status_bar;
    video_canvas_t *canvas;
    int has_decorations;
    gboolean is_fullscreen;

    /* FIXME: this function does not work properly for vsid and should never
     * get called by it, but at least on Macs it can get called if the user
     * clicks the fullscreen button in the main vsid window.
     */
    if (active_win_index < 0 || machine_class == VICE_MACHINE_VSID) {
        debug_gtk3("Error: active_win_index < 0");
        return;
    }

    /* determine fullscreen state */
    canvas = ui_get_active_canvas();
    if (canvas == NULL) {
        debug_gtk3("failed: canvas == NULL.");
        return;
    }

    is_fullscreen = ui_is_fullscreen();
    has_decorations = (!is_fullscreen) || fullscreen_has_decorations;
#if 0
    debug_gtk3("Has decorations = %d (is_fullscreen = %d, fullscreen_has_decorations = %d)",
            has_decorations, is_fullscreen, fullscreen_has_decorations);
#endif
    window = ui_resources.window_widget[active_win_index];
    grid = gtk_bin_get_child(GTK_BIN(window));
    menu_bar = gtk_grid_get_child_at(GTK_GRID(grid), 0, ROW_MENU_BAR);
    crt_grid = gtk_grid_get_child_at(GTK_GRID(grid), 0, ROW_CRT_CONTROLS);
    mixer_grid = gtk_grid_get_child_at(GTK_GRID(grid), 0, ROW_MIXER_CONTROLS);
    status_bar = gtk_grid_get_child_at(GTK_GRID(grid), 0, ROW_STATUS_BAR);

    if (has_decorations) {
        int show_statusbar;

        resources_get_int_sprintf("%sShowStatusbar",
                                  &show_statusbar,
                                  canvas->videoconfig->chip_name);
        gtk_widget_show(menu_bar);
        if (ui_statusbar_crt_controls_enabled(window)) {
            gtk_widget_show(crt_grid);
        }
        if (ui_statusbar_mixer_controls_enabled(window)) {
            gtk_widget_show(mixer_grid);
        }
        if (show_statusbar) {
            gtk_widget_show(status_bar);
        } else {
            gtk_widget_hide(status_bar);
        }
    } else {
        gtk_widget_hide(menu_bar);
        gtk_widget_hide(crt_grid);
        gtk_widget_hide(mixer_grid);
        gtk_widget_hide(status_bar);
    }
}


/** \brief  Stub to satisfy the various $videochip-resources.c files
 *
 * \param[in]   cap_fullscreen  unused
 */
void fullscreen_capability(struct cap_fullscreen_s *cap_fullscreen)
{
    /*
     * A NOP for the Gtk3 UI, since we don't support custom fullscreen modes.
     */
    return;
}


/** \brief  Determine fullscreen state via canvas
 *
 * Work around situations where ui_is_fullscreen() cannot be used.
 *
 *  \param[in]   canvas  video canvas reference
 *
 *  \return non-0 if fullscreen is enabled
 */
gboolean ui_is_fullscreen_from_canvas(const video_canvas_t *canvas)
{
    gchar resource[32];
    int is_fullscreen;

    if (machine_class == VICE_MACHINE_VSID) {
        return FALSE;   /* VSID doesn't have fullscreen mode nor the resource */
    }

    /* Using resources_get_int_sprintf() is relatively expensive since it uses
     * malloc()/free(): */
    g_snprintf(resource, sizeof(resource), "%sFullscreen", canvas->videoconfig->chip_name);
    resources_get_int(resource, &is_fullscreen);
    return is_fullscreen ? TRUE : FALSE;
}


/** \brief  Checks if we're in fullscreen mode
 *
 * Determines fullscreen state by inspecting the "${CHIP}Fullscreen" resource
 * for the active canvas.
 *
 * \return  nonzero if we're in fullscreen mode
 */
gboolean ui_is_fullscreen(void)
{
    video_canvas_t *canvas;
    const char *chip_name;
    int is_fullscreen = 0;

    if (machine_class == VICE_MACHINE_VSID) {
        return FALSE;   /* VSID doesn't have fullscreen mode nor the resource */
    }

    /* FIXME:   During emu boot the array ui_resources.canvas[] will not be
     *          properly initialized yet, so when the opengl renderer calls
     *          this function during a realize() call the references will still
     *          be NULL and we cannot access the chip name, and thus not access
     *          CHIPFullscreen.
     */
    canvas = ui_get_active_canvas();
    if (canvas == NULL) {
        debug_gtk3("error: canvas is NULL.");
        return FALSE;
    }
    chip_name = canvas->videoconfig->chip_name;
    resources_get_int_sprintf("%sFullscreen", &is_fullscreen, chip_name);

    return is_fullscreen ? TRUE : FALSE;
}


/** \brief  Enable/disable fullscreen for current canvas
 *
 * Set the "${CHIP}Fullscreen" resource and enables or disables fullscreen
 * mode for the main window of the current canvas.
 *
 * \param[in]   enabled enable fullscreen
 */
void ui_set_fullscreen_enabled(gboolean enabled)
{
    GtkWindow *window;
    video_canvas_t *canvas;
    const char *chip_name;

    window = ui_get_active_window();
    if (window == NULL) {
        debug_gtk3("error: window is NULL.");
        return;
    }
    canvas = ui_get_active_canvas();
    if (canvas == NULL) {
        debug_gtk3("error: canvas is NULL.");
        return;
    }
    chip_name = canvas->videoconfig->chip_name;
    resources_set_int_sprintf("%sFullscreen", enabled, chip_name);
    if (enabled) {
        gtk_window_fullscreen(window);
    } else {
        gtk_window_unfullscreen(window);
    }
}


/** \brief  Updates UI in response to the simulated machine screen
 *          changing its dimensions or aspect ratio
 */
void ui_trigger_resize(void)
{
    int i;
    for (i = 0; i < NUM_WINDOWS; ++i) {
        if (ui_resources.canvas[i]) {
            video_canvas_adjust_aspect_ratio(ui_resources.canvas[i]);
        }
        if (ui_resources.window_widget[i]) {
            gtk_widget_queue_resize(ui_resources.window_widget[i]);
        }
    }
}







/*
 * Function pointer setters
 */


/** \brief  Set function to handle files dropped on a main window
 *
 * \param[in]   func    handle dropped files function
 */
void ui_set_handle_dropped_files_func(int (*func)(const char *))
{
    handle_dropped_files_func = func;
}


/** \brief  Set function to help create the main window(s)
 *
 * \param[in]   func    create window function
 */
void ui_set_create_window_func(void (*func)(video_canvas_t *))
{
    create_window_func = func;
}


/** \brief  Set function to identify a canvas from its video chip
 *
 * \param[in]   func    identify canvas function
 */
void ui_set_identify_canvas_func(int (*func)(video_canvas_t *))
{
    identify_canvas_func = func;
}


/** \brief  Set function to help create the CRT controls widget(s)
 *
 * \param[in]   func    create CRT controls widget function
 */
void ui_set_create_controls_widget_func(GtkWidget *(*func)(int))
{
    create_controls_widget_func = func;
}


/** \brief  Handler for the "destroy" event of \a widget
 *
 * Looks like debug code. But better keep it here to debug the warnings about
 * GtkEventBox'es getting destroyed prematurely.
 *
 * \param[in]   widget  widget triggering the event
 * \param[in]   data    extra event data (unused)
 */
static void on_window_grid_destroy(GtkWidget *widget, gpointer data)
{
}


/** \brief  Handler for window 'configure' events
 *
 * Triggered when a window is moved or changes size. Used currently to update
 * the Window[01]* resources to allow restoring window size and position on
 * emulator start.
 *
 * \param[in]   widget  widget triggering the event
 * \param[in]   event   event reference
 * \param[in]   data    extra event data (used to pass window index)
 *
 * \return  bool
 */
static gboolean on_window_configure_event(GtkWidget *widget,
                                          GdkEvent *event,
                                          gpointer data)
{
    if (event->type == GDK_CONFIGURE) {
#if 0
        GdkEventConfigure *cfg = (GdkEventConfigure *)event;
#endif
        /* determine Window index */
        int windex = GPOINTER_TO_INT(data);

        /* DO NOT UNCOMMENT
         * Uncommenting this will cause the code after it compile just fine.
         * But it would trigger C99.
         */
#if 0
        debug_gtk3("updating window #%d coords and size to (%d,%d)/(%d*%d)"
                " in resources.",
                0, cfg->x, cfg->y, cfg->width, cfg->height);
#endif
        /* set resources, ignore failures */

        gint root_x;
        gint root_y;
        gint width;
        gint height;

        gtk_window_get_position(GTK_WINDOW(widget), &root_x, &root_y);
        gtk_window_get_size(GTK_WINDOW(widget), &width, &height);

        resources_set_int_sprintf("Window%dWidth", width, windex);
        resources_set_int_sprintf("Window%dHeight", height, windex);
        resources_set_int_sprintf("Window%dXpos", root_x, windex);
        resources_set_int_sprintf("Window%dYpos", root_y, windex);
    }
    return FALSE;
}

#ifdef MACOS_COMPILE

void macos_set_dock_icon_workaround(GdkPixbuf *icon);
void macos_activate_application_workaround(void);

/** \brief  Set the macOS dock icon
 *
 * Gtk dock icon support doesn't work on macos (last tested with Gtk 3.24.8)
 * Therefore we get it done via the obj-c API. Except rather than integrate
 * support for obj-c into the project, leverage some low level C functionality
 * to interact with the obj-c runtime.
 */
void macos_set_dock_icon_workaround(GdkPixbuf *icon)
{
    GBytes *gbytes;
    gconstpointer bytes;
    gsize bytesSize;
    id imageData;
    id logo;
    id application;
    gchar *png_buffer;
    gsize png_buffer_size;

    gdk_pixbuf_save_to_buffer(icon, &png_buffer, &png_buffer_size, "png", NULL, NULL);
    gbytes = g_bytes_new_take(png_buffer, png_buffer_size);

    if (!gbytes) {
        log_error(LOG_DEFAULT, "macos_set_dock_icon_workaround: failed to access icon bytes from gresource file.\n");
        return;
    }

    bytes = g_bytes_get_data(gbytes, &bytesSize);
    imageData =
        OBJC_MSGSEND(id, id, SEL, gconstpointer, gsize, BOOL)(
            (id)objc_getClass("NSData"),
            sel_getUid("dataWithBytesNoCopy:length:freeWhenDone:"),
            bytes,
            bytesSize,
            NO);
    logo = OBJC_MSGSEND(id, id, SEL)((id)objc_getClass("NSImage"), sel_getUid("alloc"));
    logo = OBJC_MSGSEND(id, id, SEL, id)(logo, sel_getUid("initWithData:"), imageData);

    if (logo) {
        application = OBJC_MSGSEND(id, id, SEL)((id)objc_getClass("NSApplication"), sel_getUid("sharedApplication"));
        OBJC_MSGSEND(id, id, SEL, id)(application, sel_getUid("setApplicationIconImage:"), logo);
        OBJC_MSGSEND(id, id, SEL)(logo, sel_getUid("release"));
    } else {
        log_error(LOG_DEFAULT, "macos_set_dock_icon_workaround: failed to initialise image from resource");
    }
}

/** \brief  Bring emulator main window to front on macOS
 *
 * On macOS, this Gtk3 app doesn't activate properly on launch.
 * (last tested with Gtk 3.24.8). This means that the user needs
 * to click the icon in the dock for the emulator window to appear.
 *
 * This workaround is the obj-c runtime equivalent of calling:
 * [[NSApplication sharedApplication] activateIgnoringOtherApps: YES];
 */
void macos_activate_application_workaround(void)
{
    id ns_application;

    /* [[NSApplication sharedApplication] activateIgnoringOtherApps: YES]; */
    ns_application = OBJC_MSGSEND(id, id, SEL)((id)objc_getClass("NSApplication"), sel_getUid("sharedApplication"));
    OBJC_MSGSEND(void, id, SEL, BOOL)(ns_application, sel_getUid("activateIgnoringOtherApps:"), YES);
}

#endif


/** \brief  Event handler for the rendering area's button presses
 *
 * Currently switches fullscreen mode when double-clicking, but can also be
 * used to present a context menu to for some video settings via right-click,
 * which is one of our many TODO's.
 *
 * \param[in]   canvas  rendering area
 * \param[in]   event   event object
 * \param[in]   data    GtkWindow parent of \a canvas
 *
 * \return  TRUE if event accepted
 */
static gboolean rendering_area_event_handler(GtkWidget *canvas,
                                             GdkEventButton *event,
                                             gpointer data)
{
    if (machine_class == VICE_MACHINE_VSID) {
        return FALSE;
    }

    if (event->type == GDK_DOUBLE_BUTTON_PRESS
            && event->button == GDK_BUTTON_PRIMARY) {
        int mouse;

        /* only trigger fullscreen switching when mouse-grab isn't active and
         * a lightpen isn't active */
        resources_get_int("Mouse", &mouse);
        if (!mouse && !lightpen_enabled) {
            ui_action_trigger(ACTION_FULLSCREEN_TOGGLE);
        }
        /* signal event handled */
        return TRUE;
    }
    /* signal event not handled, avoids the host mouse pointer showing up
     * during mouse grab */
    return FALSE;
}

/** \brief  Set window geomeotry of a window by index from resources
 *
 * \param[in]   index   window index (\c PRIMARY_WINDOW or \c SECONDARY_WINDOW)
 *
 * \return  \c true if the geomeotry for the given \a index was set due to the
 *          resources not containing their default values
 */
static bool set_window_geometry_from_resources(int index)
{
    GtkWidget *window;
    bool       restored = false;

    window = ui_resources.window_widget[index];
    if (window != NULL) {
        int xpos;
        int ypos;
        int width;
        int height;

        resources_get_int_sprintf("Window%dXpos",   &xpos,   index);
        resources_get_int_sprintf("Window%dYpos",   &ypos,   index);
        resources_get_int_sprintf("Window%dWidth",  &width,  index);
        resources_get_int_sprintf("Window%dHeight", &height, index);

        if (xpos > INT_MIN && ypos > INT_MIN) {
            gtk_window_move(GTK_WINDOW(window), xpos, ypos);
            restored = true;
        }
        if (width > 0 && height > 0) {
            gtk_window_resize(GTK_WINDOW(window), width, height);
            restored = true;
        }
    }
    return restored;
}


/** \brief  Set geometry of (both) emulated display window(s) from resources
 *
 * Attempt to set the size and position of the display window(s) from resources.
 */
void ui_set_window_geometries(void)
{
    set_window_geometry_from_resources(PRIMARY_WINDOW);
    if (machine_class == VICE_MACHINE_C128) {
        set_window_geometry_from_resources(SECONDARY_WINDOW);
    }
}


/** \brief  Create a toplevel window to represent a video canvas
 *
 * This function takes a video canvas structure and builds the widgets
 * that will represent that canvas in the UI as a whole.
 *
 * While it creates the widgets, it does not make them visible. The
 * video canvas routines are expected to do any last-minute processing
 * or preparation, and then call ui_display_main_window() when ready.
 *
 * \param[in]   canvas  the video_canvas_s to initialize
 *
 * \warning The order of the windows created for x128 depends on the order of
 *          the calls to vicii_init() and vdc_init() in src/c128/c128.c.
 *          That order is currently set to vicii before vdc so we get the proper
 *          window indexes. There has to be a better way.
 */
void ui_create_main_window(video_canvas_t *canvas)
{
    GtkWidget *new_window;
    GtkWidget *grid;
    GtkWidget *status_bar;
    int target_window;

    GtkWidget *crt_controls;
    GtkWidget *mixer_controls;

    int kbd_status = 0;
    int mouse_grab = 0;

    GdkPixbuf *icon;
    gchar title[256];

    bool restored = false;

    if (machine_class != VICE_MACHINE_VSID) {
        resources_get_int("Mouse", &mouse_grab);
    }

    new_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    /* Set the gravity so that gtk doesn't over-compensate for the
     * window's border width when saving/restoring its position. */
    gtk_window_set_gravity(GTK_WINDOW(new_window), GDK_GRAVITY_STATIC);
    /* this needs to be here to make the menus with accelerators work */
    vhk_gtk_init_accelerators(new_window);

    /* set the dock / taskbar icon */
    icon = get_default_icon();

#ifdef MACOS_COMPILE
    macos_set_dock_icon_workaround(icon);
#else
    if (icon != NULL) {
        gtk_window_set_icon(GTK_WINDOW(new_window), icon);
    }
#endif

    /* set title */
    if (!mouse_grab) {
        g_snprintf(title, sizeof(title), "VICE (%s)", machine_get_name());
    } else {
        gchar *name = vhk_gtk_get_accel_label_by_action(ACTION_MOUSE_GRAB_TOGGLE);
        g_snprintf(title, sizeof(title),
                   "VICE (%s) (Use %s to disable mouse grab)",
                   machine_get_name(), name);
        g_free(name);
    }

    gtk_window_set_title(GTK_WINDOW(new_window), title);

    grid = gtk_grid_new();
    g_signal_connect(grid, "destroy", G_CALLBACK(on_window_grid_destroy), NULL);
    gtk_container_add(GTK_CONTAINER(new_window), grid);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(grid), GTK_ORIENTATION_VERTICAL);
    canvas->grid = grid;

    if (create_window_func != NULL) {
        create_window_func(canvas);
    }

    target_window = -1;
    if (identify_canvas_func != NULL) {
        /* Identify the window as the PRIMARY_WINDOW or SECONDARY_WINDOW. */
        target_window = identify_canvas_func(canvas);
    }
    if (target_window < 0) {
        log_error(LOG_DEFAULT, "ui_create_main_window: canvas not identified!\n");
        archdep_vice_exit(1);
    }
    if (ui_resources.window_widget[target_window] != NULL) {
        log_error(LOG_DEFAULT, "ui_create_main_window: existing window recreated??\n");
        archdep_vice_exit(1);
    }

    /* add status bar */
    status_bar = ui_statusbar_create(target_window);
    gtk_widget_show_all(status_bar);
    gtk_widget_set_no_show_all(status_bar, TRUE);

    gtk_container_add(GTK_CONTAINER(grid), status_bar);

    /* add CRT controls */
    crt_controls = NULL;

    if (machine_class != VICE_MACHINE_VSID) {

        if (create_controls_widget_func != NULL) {
            crt_controls = create_controls_widget_func(target_window);
        }
        if (crt_controls != NULL) {
            gtk_widget_hide(crt_controls);
            gtk_container_add(GTK_CONTAINER(grid), crt_controls);
            gtk_widget_set_no_show_all(crt_controls, TRUE);
        }
    }

    mixer_controls = NULL;
    if (machine_class != VICE_MACHINE_VSID) {

        /* add sound mixer controls */
        mixer_controls = mixer_widget_create(TRUE, GTK_ALIGN_END);
        gtk_widget_hide(mixer_controls);
        gtk_container_add(GTK_CONTAINER(grid), mixer_controls);
        gtk_widget_set_no_show_all(mixer_controls, TRUE);
    }

    g_signal_connect_unlocked(new_window, "focus-in-event",
                     G_CALLBACK(on_focus_in_event), NULL);
    g_signal_connect_unlocked(new_window, "focus-out-event",
                     G_CALLBACK(on_focus_out_event), NULL);
    if (machine_class != VICE_MACHINE_VSID) {
        g_signal_connect_unlocked(new_window, "window-state-event",
                                  G_CALLBACK(on_window_state_event), NULL);
    }
    /* This event never returns so must not hold the vice lock */
    g_signal_connect(new_window, "delete-event",
                     G_CALLBACK(on_delete_event), NULL);
    g_signal_connect_unlocked(new_window, "configure-event",
                     G_CALLBACK(on_window_configure_event),
                     GINT_TO_POINTER(target_window));
    /*
     * Set up drag-n-drop handling for files
     */
    if (machine_class != VICE_MACHINE_VSID) {
        /* VSID has its own drag-n-drop handlers */

        gtk_drag_dest_set(new_window,
                          GTK_DEST_DEFAULT_ALL,
                          ui_drag_targets,
                          UI_DRAG_TARGETS_COUNT,
                          GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
        g_signal_connect(new_window,
                         "drag-data-received",
                         G_CALLBACK(on_drag_data_received),
                         NULL);
        g_signal_connect(new_window,
                         "drag-drop",
                         G_CALLBACK(on_drag_drop),
                         NULL);
        if (ui_resources.start_minimized) {
            gtk_window_iconify(GTK_WINDOW(new_window));
        }
    }
    ui_resources.canvas[target_window] = canvas;
    ui_resources.window_widget[target_window] = new_window;

    canvas->window_index = target_window;

    /* gtk_window_set_title(GTK_WINDOW(new_window), canvas->viewport->title); */

    /* Connect keyboard handlers, except for VSID */
    if (machine_class != VICE_MACHINE_VSID) {
        kbd_connect_handlers(new_window, NULL);
    }

    /*
     * Try to restore windows position and size
     */
    restored = set_window_geometry_from_resources(target_window);
    if (!restored) {
        /*
         * If not restoring location and size from config, attempt to place
         * the new application window centred on the active screen at launch.
         * Doesn't work perfectly because the size of the UI at this point
         * doesn't include the size of the canvas. But it's better than 0,0
         * on some random screen.
         */
        gtk_window_set_position(GTK_WINDOW(new_window), GTK_WIN_POS_CENTER);
    }

    /*
     * Do we start minimized or maximized?
     */
    if (ui_resources.start_minimized && ui_resources.start_maximized) {
        /* error: cannot have both: log and ignore */
        log_error(LOG_DEFAULT, "minimized and maximized window requested, ignoring.");
    } else if (ui_resources.start_minimized) {
        /* there's no gtk_window_minimize() so we do this */
        gtk_window_iconify(GTK_WINDOW(new_window));
    } else if (ui_resources.start_maximized) {
        gtk_window_maximize(GTK_WINDOW(new_window));
    } else {
        /* my guess is a minimized/iconified window cannot be fullscreen */
        if (ui_is_fullscreen_from_canvas(canvas)) {
            gtk_window_fullscreen(GTK_WINDOW(new_window));
            if (!fullscreen_has_decorations) {
                /* hide window decorations */
                GtkWidget *menu_bar;

                menu_bar = gtk_grid_get_child_at(GTK_GRID(grid), 0, 0);
                gtk_widget_hide(menu_bar);
                if (crt_controls != NULL) {
                    gtk_widget_hide(crt_controls);
                }
                if (mixer_controls != NULL) {
                    gtk_widget_hide(mixer_controls);
                }
                gtk_widget_hide(status_bar);
            }
        } else {
            gtk_window_unfullscreen(GTK_WINDOW(new_window));
        }
    }

    /* set any menu checkboxes that aren't connected to resources */
    vhk_gtk_set_check_item_blocked_by_action(ACTION_WARP_MODE_TOGGLE,
                                             vsync_get_warp_mode());

    if (machine_class != VICE_MACHINE_VSID) {

        if (resources_get_int("KbdStatusbar", &kbd_status) < 0) {
            kbd_status = 0;
        }
        ui_statusbar_set_kbd_debug_for_window(new_window, kbd_status);
    }

    if (grid != NULL) {
        /* get rendering area */
        GtkWidget *render_area = gtk_grid_get_child_at(GTK_GRID(grid), 0, 1);

        /* set up event handler for clicks on the canvas */
        g_signal_connect_unlocked(
                render_area,
                "button-press-event",
                G_CALLBACK(rendering_area_event_handler),
                new_window);
    }

    /* activate settings dialog at a specific node if requested via the
     * -settings-node command line option
     */
    if (settings_node_path != NULL) {
        ui_settings_dialog_show(settings_node_path);
        settings_node_path = NULL;
    }
}


/** \brief  Makes a main window visible once it's been initialized
 *
 * \param[in]   index   which window to display
 *
 * \sa      ui_resources_s::window_widget
 */
void ui_display_main_window(int index)
{
    GtkWidget *window;
    GdkFrameClock *frame_clock;
    video_canvas_t *canvas;

    window = ui_resources.window_widget[index];

    if (!window) {
        /* This function is called blindly for both primary and secondary windows */
        return;
    }
    active_win_index = index;

    /* Normally this would show everything in the window,
     * including hidden status bar displays, but we've
     * disabled secondary displays in the status bar code with
     * gtk_widget_set_no_show_all(). */
    gtk_widget_show_all(window);
    if (machine_class != VICE_MACHINE_VSID) {
        ui_update_fullscreen_decorations();
    }

#ifdef MACOS_COMPILE
    macos_activate_application_workaround();
#endif

    /* Queue up a redraw opportunity each frame */
    canvas = ui_resources.canvas[index];
    if (canvas->event_box) {
        /* no canvas for vsid */
        frame_clock = gdk_window_get_frame_clock(gtk_widget_get_window(window));
        g_signal_connect_unlocked(frame_clock, "update", G_CALLBACK(canvas->renderer_backend->queue_redraw), canvas);
        gdk_frame_clock_begin_updating(frame_clock);
    }

}

/** \brief  Destroy a main window
 *
 * \param[in]   index   which window to destroy
 *
 * \sa      ui_resources_s::window_widget
 */
void ui_destroy_main_window(int index)
{
    GtkWidget *window;
    GdkFrameClock *frame_clock;
    video_canvas_t *canvas;

    window = ui_resources.window_widget[index];
    ui_resources.window_widget[index] = NULL;

    if (!window) {
        /* This function is called blindly for both primary and secondary windows */
        return;
    }

    /* Explicitly shut down the frame clock based rendering updates - not sure if necessary but cleaner. */
    canvas = ui_resources.canvas[index];
    if (canvas->event_box) {
        /* no canvas for vsid */
        frame_clock = gdk_window_get_frame_clock(gtk_widget_get_window(window));
        gdk_frame_clock_end_updating(frame_clock);
    }

    gtk_widget_destroy(window);
}


/** \brief  Initialize command line options (generic)
 *
 * \return  0 on success, -1 on failure
 */
int ui_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options_common);
}


/** \brief  Display a generic file chooser dialog
 *
 * \param[in]   format  format string for the dialog's title
 *
 * \return  a copy of the chosen file's name; free it with lib_free()
 *
 * \note    This is currently only called by event_playback_attach_image()
 *
 * \warning This function is unimplemented and will intentionally crash
 *          VICE if it is called.
 */
char *ui_get_file(const char *format, ...)
{
    /*
     * Also not called when trying to play back events, at least, I've never
     * seen this called.
     */
    NOT_IMPLEMENTED();
    return NULL;
}


/** \brief  Initialize Gtk3/GLib
 *
 * \param[in]   argc    pointer to main()'s argc
 * \param[in]   argv    main()'s argv
 *
 * \return  0 on success, -1 on failure
 */
void ui_init_with_args(int *argc, char **argv)
{
    gtk_init(argc, &argv);
}


/** \brief  Initialize UI
 *
 * Loads gresource data, disables F10 as the accelerator for the menu bar,
 * registers the CBM font with the host and initializes the statusbar.
 *
 * \return  0
 */
int ui_init(void)
{
    GSettings *settings;
    GVariant *variant;
    GtkSettings *settings_default;

    /*
     * Make sure F10 doesn't trigger the menu bar
     *
     * I tried unmapping via CSS, but according to the Gtk devs, this little
     * hack works, and it does.
     */
    settings_default = gtk_settings_get_default();
    /* i've seen gtk example code use the returned value directly, but the docs
     * say it can return NULL, so let's be safe */
    if (settings_default != NULL) {
        g_object_set(settings_default, "gtk-menu-bar-accel", "F20", NULL);
    }

    if (!uidata_init()) {
        log_error(LOG_DEFAULT,
                "failed to initialize GResource data, don't expect much"
                " when it comes to icons, fonts or logos.");
    }

    if (!archdep_register_cbmfont()) {
        log_error(LOG_DEFAULT, "failed to register CBM font.");
    }

    /*
     * Sort directories before files in GtkFileChooser
     *
     * Perhaps turn this into a resource when people start complaining? Though
     * personally I'm used to having directories sorted before files.
     *
     * FIXME:   This alters Gtk/GLib settings globally, ie Wm/Desktop-wide.
     *          Which probably isn't the correct way.
     */
    settings = g_settings_new("org.gtk.Settings.FileChooser");
    /* returns floating ref */
    variant = g_variant_new("b", TRUE);
    /* floating ref is consumed here */
    g_settings_set_value(settings, "sort-directories-first", variant);

    /* disable the "Recent files" crap when opening without explicitly setting
     * a directory */
    variant = g_variant_new("s", "cwd");
    g_settings_set_value(settings, "startup-mode", variant);

    /* this should be unref'ed after use */
    g_object_unref(settings);

    ui_statusbar_init();
    return 0;
}


/** \brief  Finalize initialization after creating the main window(s)
 *
 * Currently sets the proper state for the CHIPShowStatusbar toggle buttons since
 * the resources aren't yet initialized when creating the menu structure(s) and
 * the main window(s) is/are created.
 *
 * \return  0 on success, -1 on failure
 */
int ui_init_finalize(void)
{
   /* Set the CHIPShowStatusbar toggle button according to its resource */
    if (machine_class != VICE_MACHINE_VSID) {
        GtkWidget *window;
        video_canvas_t *canvas;
        int show_statusbar;
        int xpos;
        int ypos;
        int width;
        int height;

        window = ui_resources.window_widget[PRIMARY_WINDOW];
        canvas = ui_resources.canvas[PRIMARY_WINDOW];
        /* guard against NULL in case of -console */
        if (canvas != NULL && window != NULL) {
            resources_get_int_sprintf("%sShowStatusbar",
                                      &show_statusbar,
                                      canvas->videoconfig->chip_name);
            ui_statusbar_set_visible_for_window(window, show_statusbar);

            /* if any of the following is INT_MIN it means we don't want to restore
             * window position and size, and thus can use the resize(1,1) trick to
             * get rid of any extra space added by the hidden statusbar */
            if (!show_statusbar) {
                resources_get_int_sprintf("Window%dXpos", &xpos, PRIMARY_WINDOW);
                resources_get_int_sprintf("Window%dYpos", &ypos, PRIMARY_WINDOW);
                resources_get_int_sprintf("Window%dwidth", &width, PRIMARY_WINDOW);
                resources_get_int_sprintf("Window%dheight", &height, PRIMARY_WINDOW);
                if (xpos == INT_MIN || ypos == INT_MIN ||
                        width == INT_MIN || height == INT_MIN) {
                    gtk_window_resize(GTK_WINDOW(window), 1, 1);
                }
            }

            if (machine_class == VICE_MACHINE_C128) {
                /* set the secondary (VDC) window's menu toggle button */
                window = ui_resources.window_widget[SECONDARY_WINDOW];
                canvas = ui_resources.canvas[SECONDARY_WINDOW];
                resources_get_int_sprintf("%sShowStatusbar",
                                          &show_statusbar,
                                          canvas->videoconfig->chip_name);
                ui_statusbar_set_visible_for_window(window, show_statusbar);

                if (!show_statusbar) {
                    resources_get_int_sprintf("Window%dXpos", &xpos, SECONDARY_WINDOW);
                    resources_get_int_sprintf("Window%dYpos", &ypos, SECONDARY_WINDOW);
                    resources_get_int_sprintf("Window%dwidth", &width, SECONDARY_WINDOW);
                    resources_get_int_sprintf("Window%dheight", &height, SECONDARY_WINDOW);
                    if (xpos == INT_MIN || ypos == INT_MIN ||
                            width == INT_MIN || height == INT_MIN) {
                        gtk_window_resize(GTK_WINDOW(window), 1, 1);
                    }
                }
            }
        }
    }


    if (!console_mode) {
        /* ui_actions_init() is called in src/main.c */
        ui_actions_set_dispatch(ui_action_dispatch);

        if (machine_class != VICE_MACHINE_VSID) {

            actions_cartridge_register();
            actions_clipboard_register();
            actions_datasette_register();
#ifdef DEBUG
            actions_debug_register();
#endif
            actions_display_register();
            actions_drive_register();
            actions_hotkeys_register();
            actions_joystick_register();
            actions_machine_register();
            actions_media_register();
            actions_printer_register();
            actions_settings_register();
            actions_snapshot_register();
            actions_speed_register();
        } else {
            /* VSID-specific actions */
            actions_machine_register(); /* reset, monitor & quit */
            actions_settings_register();
            actions_speed_register();
#ifdef DEBUG
            actions_debug_register();
#endif
            /* Triggers linker errors, we register these in vsidui.c: */
            /* actions_vsid_register(); */
        }

        /* new hotkeys API in shared/hotkeys/ */
        ui_hotkeys_init("gtk3");

        /* Set proper radio buttons, check buttons and menu item labels
         * (All emus including VSID) */
#ifdef DEBUG
        actions_debug_setup_ui();
#endif
        actions_help_register();

        if (machine_class != VICE_MACHINE_VSID) {
            actions_display_setup_ui();
            actions_joystick_setup_ui();
            actions_speed_setup_ui();
        }
    }
    return 0;
}


/** \brief  Result of the JAM dialog
 */
static ui_jam_action_t jam_dialog_result;


/** \brief  JAM dialog handler for the threaded UI
 *
 * \param[in]   user_data   message
 *
 * \return  FALSE
 */
static gboolean ui_jam_dialog_impl(gpointer user_data)
{
    /* XXX: this probably needs a variable index into the window_widget array */
    jam_dialog_result = jam_dialog(ui_resources.window_widget[PRIMARY_WINDOW], (char *)user_data);

    return FALSE;
}

/** \brief  Display a dialog box in response to a CPU jam
 *
 * \param[in]   format  format string for the message to display
 *
 * \return  the action the user selected in response to the jam
 */
ui_jam_action_t ui_jam_dialog(const char *format, ...)
{
    va_list args;
    char *buffer;

    va_start(args, format);
    buffer = lib_mvsprintf(format, args);
    va_end(args);

    /*
     * We need to use the main thread to do UI stuff. And we
     * also need to block the VICE thread until we get the
     * user decision.
     */
    jam_dialog_result = UI_JAM_INVALID;
    gdk_threads_add_timeout(0, ui_jam_dialog_impl, (gpointer)buffer);

    /* block until the result is set */
    while (jam_dialog_result == UI_JAM_INVALID) {
        mainlock_yield_and_sleep(tick_per_second() / 60);
    }

    lib_free(buffer);

    return jam_dialog_result;
}


/** \brief  Initialize resources related to the UI in general
 *
 * \return  0 on success, -1 on failure
 */
int ui_resources_init(void)
{
    int i;

    /* initialize command int/bool resources */
    if (resources_register_int(resources_int_shared) != 0) {
        return -1;
    }
    /* initialize string resources */
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    /* initialize int/bool resources */
    if (resources_register_int(resources_int_primary_window) < 0) {
        return -1;
    }

    if (machine_class == VICE_MACHINE_C128) {
        if (resources_register_int(resources_int_secondary_window) < 0) {
            return -1;
        }
    }

    for (i = 0; i < NUM_WINDOWS; ++i) {
        ui_resources.canvas[i] = NULL;
        ui_resources.window_widget[i] = NULL;
    }

    return 0;
}


/** \brief  Clean up memory used by VICE resources
 */
void ui_resources_shutdown(void)
{
    lib_free(ui_resources.monitor_font);
    lib_free(ui_resources.monitor_fg);
    lib_free(ui_resources.monitor_bg);
}


/** \brief Clean up memory used by the UI system itself
 */
void ui_shutdown(void)
{
    uidata_shutdown();
    ui_statusbar_shutdown();
    cart_image_widgets_shutdown();
    settings_keyboard_widget_shutdown();
    actions_settings_shutdown();
    settings_rom_widget_shutdown();
    /* hotkeys are shut down in src/main.c */
}


/** \brief  Result of the extend image dialog
 */
static ui_extendimage_action_t extendimage_dialog_result;


/** \brief  extend image dialog handler for the threaded UI
 *
 * \param[in]   user_data   message
 *
 * \return  FALSE
 */
static gboolean ui_extendimage_dialog_impl(gpointer user_data)
{
    /* XXX: this probably needs a variable index into the window_widget array
     *
     *      Nope, our code is so shitty it uses ui_get_active_window(), so we
     *      pass NULL.
     */
    extendimage_dialog_result = extendimage_dialog(NULL, (char *)user_data);

    return FALSE;
}


/** \brief  Display the "Do you want to extend the disk image?" dialog
 *
 * \return  nonzero to extend the image, 0 otherwise
 *
 */
int ui_extend_image_dialog(void)
{
    const char * const msg =
        "  The drive has written to tracks that are not included in the currently  \n"
        "  mounted image. Do you want to write those extra tracks into the current  \n"
        "  image?";

    if (console_mode) {
        /* XXX: Can't really ask, so make a decision. */
        return UI_EXTEND_IMAGE_ALWAYS;
    }

    if (mainlock_is_vice_thread()) {
        /*
         * We need to use the main thread to do UI stuff. And we
         * also need to block the VICE thread until we get the
         * user decision.
         */
        extendimage_dialog_result = UI_EXTEND_IMAGE_INVALID;
        /* FIXME: ideally we would somehow get the drive and perhaps name of the
                  mounted image and put it into the message. */
        gdk_threads_add_timeout(0, ui_extendimage_dialog_impl, (void *)msg);

        /* block until the result is set */
        while (extendimage_dialog_result == UI_EXTEND_IMAGE_INVALID) {
            mainlock_yield_and_sleep(tick_per_second() / 60);
        }
    } else {
        /*
         * Shutdown code is executed by the UI thread, not the vice thread.
         * And this code can be called during shutdown.
         */
        extendimage_dialog_result = extendimage_dialog(NULL, msg);
    }

    return extendimage_dialog_result;
}

/** \brief  Not used */
void ui_dispatch_events(void)
{
}

/** \brief  Show settings dialog from a joystick button
 */
void arch_ui_activate(void)
{
    /* Use actions system to avoid popping up multiple dialogs on multiple
     * button presses: */
    ui_action_trigger(ACTION_SETTINGS_DIALOG);
}

/** \brief  Error dialog handler for the threaded UI
 *
 * \param[in]   user_data   error message (allocated by VICE)
 *
 * \return  \c G_SOURCE_REMOVE to only run once
 */
static gboolean ui_error_impl(gpointer user_data)
{
    vice_gtk3_message_error(NULL, /* current emu window as parent */
                            "VICE Error", "%s", (const char*)user_data);
    lib_free(user_data);
    return G_SOURCE_REMOVE;
}


/** \brief  Display error message through the UI
 *
 * \param[in]   format  format string for the error
 */
void ui_error(const char *format, ...)
{
    char *buffer;
    va_list ap;

    va_start(ap, format);
    buffer = lib_mvsprintf(format, ap);
    va_end(ap);

    /* call from ui thread */
    gdk_threads_add_timeout(0, ui_error_impl, (gpointer)buffer);
}


/** \brief  Message dialog handler for the threaded UI
 *
 * \param[in]   user_data   message
 *
 * \return  FALSE
 */
static gboolean ui_message_impl(gpointer user_data)
{
    vice_gtk3_message_info(NULL, "VICE Message", "%s", (const char *)user_data);
    lib_free(user_data);
    return G_SOURCE_REMOVE;
}


/** \brief  Display a message through the UI
 *
 * \param[in]   format  format string for message
 */
void ui_message(const char *format, ...)
{
    char *buffer;
    va_list ap;

    va_start(ap, format);
    buffer = lib_mvsprintf(format, ap);
    va_end(ap);

    /* call from ui thread */
    gdk_threads_add_timeout(0, ui_message_impl, (gpointer)buffer);
}


/** \brief Perform a single iteration of the pause loop
 *
 * \return boolean whether to keep iterating
 */
bool ui_pause_loop_iteration(void)
{
    if (!is_paused) {
        return false;
    }

    /* Exit pause loop to enter monitor if needed. */
    if (enter_monitor_while_paused) {
        enter_monitor_while_paused = 0;
        monitor_startup_trap();
        return false;
    }

    /* Otherwise give the UI the lock for a while */
    mainlock_yield_and_sleep(tick_per_second() / 60);

    /* Another iteration needed unless pause was disabled during sleep */
    return is_paused;
}


/** \brief  Keeps the ui events going while the emulation is paused
 */
static void pause_loop(void *param)
{
    vsync_suspend_speed_eval();
    sound_suspend();

    if (ui_pause_loop_iteration()) {
        /*
         * Still paused, schedule another run. Doing it this way allows
         * other, perhaps newly queued, vsync_on_vsync_do callcacks to
         * be called.
         */
        vsync_on_vsync_do(pause_loop, NULL);
    }
}


/** \brief  Get pause active state
 *
 * \return  boolean
 */
int ui_pause_active(void)
{
    return is_paused;
}


/** \brief  Pause emulation
 */
void ui_pause_enable(void)
{
    if (!is_paused) {
        is_paused = 1;
        vsync_on_vsync_do(pause_loop, NULL);
    }
}


/** \brief  Unpause emulation
 */
void ui_pause_disable(void)
{
    is_paused = 0;
}

/** \brief  The pause loop should trigger the monitor
 */
void ui_pause_enter_monitor(void)
{
    enter_monitor_while_paused = 1;
}


/** \brief  Toggle pause state
 */
void ui_pause_toggle(void)
{
    if (ui_pause_active()) {
        ui_pause_disable();
    } else {
        ui_pause_enable();
    }
}

#if 0
/** \brief  Pause toggle action
 *
 * \return  TRUE (indicates the Alt+P got consumed by Gtk, so it won't be
 *          passed to the emu)
 */
gboolean ui_action_toggle_pause(void)
{
    ui_pause_toggle();
    vhk_gtk_set_check_item_blocked_by_action(ACTION_PAUSE_TOGGLE,
                                             (gboolean)ui_pause_active());

    return TRUE;    /* has to be TRUE to avoid passing Alt+P into the emu */
}


/** \brief  Toggle warp mode action
 *
 * \return  TRUE to signal GDK the key got consumed so it doesn't end up in
 *          the emulated machine
 */
gboolean ui_action_toggle_warp(void)
{
    vsync_set_warp_mode(!vsync_get_warp_mode());
    vhk_gtk_set_check_item_blocked_by_action(ACTION_WARP_MODE_TOGGLE,
                                             (gboolean)vsync_get_warp_mode());

    return TRUE;
}


/** \brief  Advance frame action
 *
 * \return  TRUE (indicates the Alt+SHIFT+P got consumed by Gtk, so it won't be
 *          passed to the emu)
 *
 * \note    The gboolean return value is no longer required since the 'hotkey'
 *          handling in kbd.c takes care of passing TRUE to Gtk3.
 */
gboolean ui_action_advance_frame(void)
{
    if (ui_pause_active()) {
        vsyncarch_advance_frame();
    } else {
        ui_pause_enable();
        vhk_gtk_set_check_item_blocked_by_action(ACTION_PAUSE_TOGGLE,
                                                 (gboolean)ui_pause_active());
    }

    return TRUE;    /* has to be TRUE to avoid passing Alt+SHIFT+P into the emu */
}
#endif


/** \brief  Destroy UI resources (but NOT vice 'resources')
 *
 * Don't call this directly except from main_exit();
 */
void ui_exit(void)
{
    mainlock_obtain();

    /* clean up UI resources */
    if (machine_class != VICE_MACHINE_VSID) {
        ui_cart_shutdown();
        ui_disk_attach_shutdown();
        ui_tape_attach_shutdown();
        ui_smart_attach_shutdown();
        ui_media_shutdown();
        crt_control_widget_shutdown();
    }

    ui_settings_shutdown();

    /* Destroy the main window(s) */
    ui_destroy_main_window(PRIMARY_WINDOW);
    ui_destroy_main_window(SECONDARY_WINDOW);

    /* unregister the CBM font */
    archdep_unregister_cbmfont();

    /* Show any async errors that haven't been shown yet. */
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    mainlock_release();
}

/** \brief  Send current light pen state to the emulator core for all windows
 */
void ui_update_lightpen(void)
{
    video_canvas_t *canvas;
    canvas = ui_resources.canvas[PRIMARY_WINDOW];

    if (machine_class == VICE_MACHINE_C128) {
        /* According to lightpen.c, x128 flips primary and secondary
         * windows compared to what the GTK3 backend expects. */
        if (canvas) {
            pthread_mutex_lock(&canvas->lock);
            lightpen_update(1, canvas->pen_x, canvas->pen_y, canvas->pen_buttons);
            pthread_mutex_unlock(&canvas->lock);
        }
        canvas = ui_resources.canvas[SECONDARY_WINDOW];
    }
    if (canvas) {
        pthread_mutex_lock(&canvas->lock);
        lightpen_update(0, canvas->pen_x, canvas->pen_y, canvas->pen_buttons);
        pthread_mutex_unlock(&canvas->lock);
    }
}


/** \brief  Enable/disable CRT controls
 *
 * \param[in]   enabled enabled state for the CRT controls
 */
void ui_enable_crt_controls(int enabled)
{
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *crt;

    if (active_win_index < 0 || active_win_index >= NUM_WINDOWS) {
        /* No window created yet, most likely. */
        return;
    }

    window = ui_resources.window_widget[active_win_index];
    grid = gtk_bin_get_child(GTK_BIN(window));
    crt = gtk_grid_get_child_at(GTK_GRID(grid), 0, ROW_CRT_CONTROLS);

    if (enabled) {
        gtk_widget_show(crt);
    } else {
        gtk_widget_hide(crt);
        /*
         * This is completely counter-intuitive, but it works, unlike all other
         * size_request()/set_size_hint() stuff.
         * Appearently setting a size of 1x1 pixels forces Gtk3 to render the
         * window to the appropriate (minimum) size,
         */
        gtk_window_resize(GTK_WINDOW(window), 1, 1);
    }
}


/** \brief  Enable/disable mixer controls
 *
 * \param[in]   enabled enabled state for the mixer controls
 */
void ui_enable_mixer_controls(int enabled)
{
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *mixer;

    if (active_win_index < 0 || active_win_index >= NUM_WINDOWS) {
        /* No window created yet, most likely. */
        return;
    }

    window = ui_resources.window_widget[active_win_index];
    grid = gtk_bin_get_child(GTK_BIN(window));
    mixer = gtk_grid_get_child_at(GTK_GRID(grid), 0, ROW_MIXER_CONTROLS);

    if (enabled) {
        gtk_widget_show(mixer);
    } else {
        gtk_widget_hide(mixer);
        /*
         * This is completely counter-intuitive, but it works, unlike all other
         * size_request()/set_size_hint() stuff.
         * Appearently setting a size of 1x1 pixels forces Gtk3 to render the
         * window to the appropriate (minimum) size,
         */
        gtk_window_resize(GTK_WINDOW(window), 1, 1);
    }
}


/** \brief  Get GtkWindow instance by \a index
 *
 * \param[in]   index   index in the windows widgets array (0-2)
 *
 * \return  GtkWindow instance
 */
GtkWidget *ui_get_window_by_index(int index)
{
    if (index < 0 || index >= NUM_WINDOWS) {
        return NULL;
    }
    return ui_resources.window_widget[index];
}


/** \brief  Determine if the window should have decorations in fullscreen mode
 *
 * \return  bool
 */
gboolean ui_fullscreen_has_decorations(void)
{
    return fullscreen_has_decorations ? TRUE : FALSE;
}



