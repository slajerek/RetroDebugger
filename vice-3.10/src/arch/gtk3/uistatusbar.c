/** \file   uistatusbar.c
 *  \brief  Native GTK3 UI statusbar stuff
 *
 *  The status bar widget is part of every machine window. This widget
 *  reacts to UI notifications from the emulation core and otherwise
 *  does not interact with the rest of the main menu.
 *
 *  Functions described as "Statusbar API functions" are called by the
 *  rest of the UI or the emulation system itself to report that the
 *  status displays must be updated to reflect possibly new
 *  information. It is not necessary for the data to be truly new; the
 *  statusbar implementation will treat repeated reports of the same
 *  state as no-ops when necessary for performance.
 *
 *  \author Marco van den Heuvel <blackystardust68@yahoo.com>
 *  \author Michael C. Martin <mcmartin@gmail.com>
 *  \author Bas Wassink <b.wassink@ziggo.nl>
 *  \author David Hogan <david.q.hogan@gmail.com>
 */

/*
 * Resources controlled by this widget. We probably need to expand this list.
 * (Do not add resources controlled by widgets #include'd by this widget, only
 *  add resources actually controlled from this widget)
 *
 * $VICERES SoundVolume         all
 * $VICERES C128ColumnKey       x128
 * $VICERES CrtcShowStatusbar   xcbm2 xpet
 * $VICERES TEDShowStatusbar    xplus4
 * $VICERES VDCShowStatusbar    x128
 * $VICERES VICShowStatusbar    xvic
 * $VICERES VICIIShowStatusbar  x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES DiagPin             xpet
 * $VICERES SpeedSwitch         xscpu64
 * $VICERES JiffySwitch         xscpu64
 *
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
#include <stdio.h>
#include <gtk/gtk.h>

#include "archdep_defs.h"
#include "archdep.h"
#include "attach.h"
#include "autostart.h"
#include "contentpreviewwidget.h"
#include "crtcontrolwidget.h"
#include "datasette.h"
#include "dirmenupopup.h"
#include "diskcontents.h"
#include "diskimage.h"
#include "diskimage/fsimage.h"
#include "drive-check.h"
#include "drive.h"
#include "drive.h"
#include "drivetypes.h"
#include "fliplist.h"
#include "hotkeys.h"
#include "joyport.h"
#include "joystickmenupopup.h"
#include "kbddebugwidget.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "mainlock.h"
#include "resources.h"
#include "statusbarledwidget.h"
#include "statusbarrecordingwidget.h"
#include "statusbarspeedwidget.h"
#include "tapecontents.h"
#include "tapeport.h"
#include "types.h"
#include "ui.h"
#include "uiapi.h"
#include "uiactions.h"
#include "uidatasette.h"
#include "uidiskattach.h"
#include "uifliplist.h"
#include "uimenu.h"
#include "uisettings.h"
#include "userport/userport_joystick.h"
#include "vice_gtk3.h"

#include "uistatusbar.h"

/* #define DEBUG_STATUSBAR */

#ifdef DEBUG_STATUSBAR
#define DBG(x) log_printf x
#else
#define DBG(x)
#endif


/** \brief The maximum number of status bars we will permit to exist at once. */
#define MAX_STATUS_BARS 3

/** \brief  Timeout for statusbar messages in seconds */
#define MESSAGE_TIMEOUT 5

/** \brief  Maximum length for drive unit status string */
#define DRIVE_UNIT_STR_MAX_LEN 8

/** \brief  Maximum length for drive track status string */
#define DRIVE_TRACK_STR_MAX_LEN 16


/** \brief  Tape status widget column indexes
 */
enum {
    TAPE_STATUS_COL_HEADER,     /**< label with 'datasette [12] */
    TAPE_STATUS_COL_COUNTER,    /**< label with counter */
    TAPE_STATUS_COL_MOTOR       /**< custom motor widget */
};


/** \brief  Diskdrive drive indexes per unit
 *
 * A dual-drive unit has two drives as the name implies.
 */
enum {
    DRIVE_UNIT_DRIVE_0,     /**< first drive, always valid */
    DRIVE_UNIT_DRIVE_1,     /**< second drive (2040, 3040, 4040, 8050, 8250) */
    DRIVE_UNIT_DRIVE_MAX    /**< maximum number of drives per unit */
};


/* Position of diskdrive units in their wrapper grid:
 *
 * +--------------+---------------+
 * | 8:0 18.0 [=] | 10:0 18.0 [=] |
 * | 8:1 18.0 [=] | 10:1 18.0 [=] |
 * +--------------+---------------+
 * | 9:0 18.0 [=] | 11:0 18.0 [=] |
 * | 9:1 18.0 [=] | 11:1 18.0 [=] |
 * +--------------+---------------+
 */
#define UNIT_8_COL  0
#define UNIT_8_ROW  0
#define UNIT_9_COL  0
#define UNIT_9_ROW  1
#define UNIT_10_COL 1
#define UNIT_10_ROW 0
#define UNIT_11_COL 1
#define UNIT_11_ROW 1


/** \brief  Column indexes for the drive widgets
 *
 * These are columns in their containing grid, not the status bar grid.
 */
static const int unit_cols[NUM_DISK_UNITS] = {
    UNIT_8_COL, UNIT_9_COL, UNIT_10_COL, UNIT_11_COL
};


/** \brief  Row indexes for the drive widgets
 *
 * These are rows in their containing grid, not the status bar grid.
 */
static const int unit_rows[NUM_DISK_UNITS] = {
    UNIT_8_ROW, UNIT_9_ROW, UNIT_10_ROW, UNIT_11_ROW
};


/** \brief  Column indexes for the drive status widgets
 *
 * Each status widget has a label for the unit number (8-11), suffixed with
 * a drive number for dual-drive units, ie '8:0 and 8:1' for unit 8.
 * Another label is used to display the current position of the head, currently
 * only the track number is displayed, not the sector number. The third widget
 * is the drive LED.
 */
enum {
    DRIVE_STATUS_NUMBER,    /**< label containing unit number and drive number */
    DRIVE_STATUS_HEAD,      /**< label containing position of the head */
    DRIVE_STATUS_LED        /**< led custom widget */
};


/** \brief  CSS for the drive widgets
 *
 * The negative margins look weird, but otherwise we have a lot of padding, and
 * the drive widgets already take up a lot of space when having dual-drives
 * enabled.
 */
#define DRIVE_WIDGET_CSS \
    "label {\n" \
    "    font-family: monospace;\n" \
    "    font-size:100%;\n" \
    "    margin-top: 0px;\n" \
    "    margin-bottom: -4px;\n" \
    "}\n"


/** \brief  Joysticks widget: column of label
 */
#define JOYSTICK_COL_LABEL  0

/** \brief  Joysticks widget: column of first joystick indicator widget
 */
#define JOYSTICK_COL_STATUS 1

/* Status bar column indexes */
#define SB_COL_LEDS             0
#define SB_COL_WIDGETS          0
#define SB_COL_MESSAGES         0
#define SB_COL_KBD_DEBUG        0
#define SB_COL_MESSAGES_VSEP    (SB_COL_MESSAGES + 1)
#define SB_COL_RECORDING        (SB_COL_MESSAGES_VSEP + 1)
#define SB_COLUMN_COUNT         (SB_COL_RECORDING + 1)


/** \brief  Status bar row indexes
 */
enum {
    SB_ROW_LEDS,            /**< row with small LEDs */
    SB_ROW_LEDS_HSEP,       /**< horizontal separator between LEDs and large
                                 widgets */
    SB_ROW_WIDGETS,         /**< large widgets/containers */
    SB_ROW_WIDGETS_HSEP,    /**< horizontal separator between large widgets
                                 and messages/recording */
    SB_ROW_MESSAGES,        /**< messages AND separator AND recording */
    SB_ROW_MESSAGES_HSEP,   /**< horizontal separator between messages/recording
                                 and the keyboard debugging widget */
    SB_ROW_KBD_DEBUG        /**< keyboard debugging */
};


/** \brief  Size of a statusbar message, including nul character
 */
#define MESSAGE_TEXT_SIZE   1024


/** \brief Global data that custom status bar widgets base their rendering
 *         on.
 *
 *  This data is usually set by calls from the emulation core or from
 *  other parts of the UI in response to user commands or I/O events.
 *
 *  \todo The PET can have two tape drives.
 *
 *  \todo Two-unit drive units are not covered by this structure.
 */
typedef struct ui_sb_state_s {
    /** \brief Identifier for the currently displayed status bar
     * message.
     *
     * Used to correlate timeout events so that a new message
     * isn't erased by some older message timing out. */
    intptr_t statustext_msgid;

    /** \brief Current tape state (play, rewind, etc) */
    int tape_control[TAPEPORT_MAX_PORTS];

    /** \brief Nonzero if the tape motor is powered. */
    int tape_motor_status[TAPEPORT_MAX_PORTS];

    /** \brief Location on the tape of datasette #1 */
    int tape_counter[TAPEPORT_MAX_PORTS];

    /** \brief Which drives are to be displayed in the status bar.
     *
     *  This is a bitmask, with bits 0-3 representing drives 8-11,
     *  respectively.
     */
    int drives_enabled;

    /** \brief  Drive type for each unit
     *
     * Used to determine if the layout needs changing due to changes in drive
     * type that change a unit's dual-drive status.
     */
    int drives_type[NUM_DISK_UNITS];

    /** \brief  Drive type changes involving dual-drive status
     *
     * Each bit represents a unit, with bit 0-3 -> representing units 8-11.
     * If a bit is set it means the dual-drive state of that unit has changed
     * and the UI needs updating of the widgets for that unit.
     */
    unsigned int drives_dual;

    /** \brief Nonzero if True Drive Emulation is active and drive
     *         LEDs should be drawn. */
    int drives_tde_enabled;

    /** \brief true if drive ui layout is needed */
    bool drives_layout_needed;

    /** \brief Color descriptors for the drive LED colors, 0=red, 1=green */
    int drive_led_types[NUM_DISK_UNITS][2][DRIVE_LEDS_MAX];

    /** \brief Current intensity of each drive LED, 0=off,
     *         1000=max. */
    unsigned int current_drive_leds[NUM_DISK_UNITS][2][DRIVE_LEDS_MAX];

    /** \brief true if a drive led has been changed */
    bool current_drive_leds_updated[NUM_DISK_UNITS][2][DRIVE_LEDS_MAX];

    /** \brief unit:drive label for each unit and its drives */
    char current_drive_unit_str[NUM_DISK_UNITS][2][DRIVE_UNIT_STR_MAX_LEN];

    /** \brief true if a drive unit string has been changed */
    bool current_drive_unit_str_updated[NUM_DISK_UNITS][2];

    /** \brief device:track.halftrack label for each unit and its drives */
    char current_drive_track_str[NUM_DISK_UNITS][2][DRIVE_TRACK_STR_MAX_LEN];

    /** \brief true if a drive track string has been changed */
    bool current_drive_track_str_updated[NUM_DISK_UNITS][2];

    /** \brief Current state for each of the joyports.
     *
     *  This is an 7-bit bitmask, representing, from least to most
     *  significant bits: up, down, left, right, fire button,
     *  secondary fire button, tertiary fire button. */
    int current_joyports[JOYPORT_MAX_PORTS];

    /** \brief Which joystick ports are actually available.
     *
     *  This is a bitmask representing notional ports 0-4, which are
     *  themselves defined in joyport/joyport.h. Cases like a SIDcard
     *  control port on a Plus/4 without other userport control ports
     *  mean that the set of active joyports may be discontinuous. */
    uint32_t active_joyports;

    /** \brief  Statusbar message
     *
     * Message to display in the message widget.
     */
    char message_text[MESSAGE_TEXT_SIZE];

    /** \brief  Statusbar message is pending
     *
     * Keeps track of whether a newly arrived message has been displayed by the
     * UI thread.
     */
    bool message_pending;

    /** \brief  Statusbar fadeout
     *
     * Determines if the current statusbar message should fade out after 5
     * seconds.
     */
    bool message_fadeout;

} ui_sb_state_t;


/** \brief Used to safely access sb_state between threads. */
static pthread_mutex_t sb_state_lock = PTHREAD_MUTEX_INITIALIZER;


/** \brief The current state of the status bars across the UI.
 *
 * Don't use directly! Use lock_sb_state / unlock_sb_state instead.
 * I thought this pattern would make it more obvious to future
 * developers that they shouldn't just use sb_state without locking.
 */
static ui_sb_state_t sb_state_do_not_use_directly;


/** \brief The full structure representing a status bar widget.
 *
 *  This includes the top-level widget and then every subwidget that
 *  needs to be individually addressed or manipulated by the
 *  status-report API. */
typedef struct ui_statusbar_s {
    /** \brief The status bar widget proper.
     *
     *  This is the widget the rest of the UI code will store and pack
     *  into windows. */
    GtkWidget *bar;

    /** \brief  Row with LED widgets
     */
    GtkWidget *led_row_grid;

    /** \brief  Current index in the LED row to append new LEDs
     */
    int led_row_column;

    /** \brief  Row with large/composed widgets of a status bar
     *
     * A grid containing widgets for the large row of a status bar.
     */
    GtkWidget *widget_row_grid;

    /** \brief  Current column to append new widgets
     *
     * Keeps track of the column to append new widgets with statusbar_append()
     * and statusbar_apppend_end().
     *
     * The function statusbar_append() appends widgets from left to right to
     * the top row of a status bar, the function statusbar_append_end() is used
     * to put the last top widget (volume) on the status bar setting its aligment
     * and hexpand properties so it appears to the right of the status bar.
     */
    int widget_row_column;

    /** \brief  Warp mode LED widget */
    GtkWidget *warp_led;

    /** \brief  Pause LED widget */
    GtkWidget *pause_led;

    /** \brief  shiftlock LED widget */
    GtkWidget *shiftlock_led;

    /** \brief  40/80 LED widget */
    GtkWidget *mode4080_led;

    /** \brief  capslock LED widget */
    GtkWidget *capslock_led;

    /** \brief  Userport diagnostic pin widget (PET) */
    GtkWidget *diagnosticpin_led;

    /** \brief  Turbo LED widget (SCPU64) */
    GtkWidget *supercpu_turbo_led;

    /** \brief  JiffyDOS LED widget (SCPU64) */
    GtkWidget *supercpu_jiffy_led;

    /** \brief  Widget displaying CPU speed and FPS
     *
     * Also used to set refresh rate, CPU speed, pause, warp and adv-frame
     */
    GtkWidget *speed;

    /** \brief  Stateful data used by the speed widget */
    statusbar_speed_widget_state_t speed_state;

    /** \brief  Status bar messages */
    GtkWidget *msg;

    /** \brief  Recording control/display widget */
    GtkWidget *record;

    /** \brief CRT control widget checkbox */
    GtkWidget *crt;

    /** \brief  Mixer control widget checkbox */
    GtkWidget *mixer;

    /** \brief  Tape status widgets
     */
    GtkWidget *tape_status[TAPEPORT_MAX_PORTS];

    /** \brief  Tape popup menus
     */
    GtkWidget *tape_menu[TAPEPORT_MAX_PORTS];

    /** \brief  Used to optimise tape widget updates
     *
     * FIXME:   Should probably be moved to sb_status?
     */
    int displayed_tape_counter[TAPEPORT_MAX_PORTS];

    /** \brief The joyport status widget. */
    GtkWidget *joysticks;

    /*
     * New diskdrive layout code
     */

    /* Four diskdrive units (8-11) */
    GtkWidget *drive_unit[NUM_DISK_UNITS];

    /* Four units with max two drives each: */
    GtkWidget *drive_menu[NUM_DISK_UNITS][DRIVE_UNIT_DRIVE_MAX];

    /** \brief The volume control
     *
     * Only enabled for VSID at the moment.
     */
    GtkWidget *volume;

    /** \brief The hand-shaped cursor to change to when popup menus
     *         are available. */
    GdkCursor *hand_ptr;

    /** \brief  Keyboard debugging widget */
    GtkWidget *kbd_debug;

    /** \brief PRIMARY_WINDOW, SECONDARY_WINDOW, etc */
    int window_identity;
} ui_statusbar_t;


/** \brief The collection of status bars currently active.
 *
 *  Inactive status bars have a NULL pointer for their "bar" field. */
static ui_statusbar_t allocated_bars[MAX_STATUS_BARS];


/** \brief  Cursor used when hovering over the joysticks widgets
 *
 * TODO:    figure out if I need to clean this up or that Gtk will?
 */
static GdkCursor *joywidget_mouse_ptr = NULL;


/** \brief  Timeout ID of the message widget
 */
static guint timeout_id = 0;


/* Forward decl. */
static void tape_dir_autostart_callback(const char *image,
                                        int index,
                                        int device,
                                        unsigned int drive);
static void disk_dir_autostart_callback(const char *image,
                                        int index,
                                        int device,
                                        unsigned int drive);


/** \brief  Trigger redraw of a widget on the UI thread
 *
 * \param[in,out]   user_data   widget to redraw
 *
 * \return  FALSE
 */
static gboolean redraw_widget_on_ui_thread_impl(gpointer user_data)
{
    gtk_widget_queue_draw((GtkWidget *)user_data);

    return FALSE;
}

/** \brief Queue a redraw of widget on the ui thread.
 *
 * It's not safe to ask a widget to redraw from the vice thread.
 *
 * \param[in,out]   widget  widget to redraw
 */
static void redraw_widget_on_ui_thread(GtkWidget *widget)
{
    gdk_threads_add_timeout(0, redraw_widget_on_ui_thread_impl, (gpointer)widget);
}

/** \brief Get a locked reference to sb_state */
static ui_sb_state_t *lock_sb_state(void)
{
    pthread_mutex_lock(&sb_state_lock);
    return &sb_state_do_not_use_directly;
}

/** \brief Release a locked reference to sb_state_do_not_use_directly */
static void unlock_sb_state(void)
{
    pthread_mutex_unlock(&sb_state_lock);
}

/*****************************************************************************
 *                          Gtk3 event handlers                              *
 ****************************************************************************/


/** \brief  Timeout callback for the stausbar message widget
 *
 * \param[in,out]   data    message widget
 *
 * \return  FALSE (delete timer source)
 */
static gboolean message_timeout_handler(gpointer data)
{
    GtkLabel *label = data;

    gtk_label_set_text(label, "");
    timeout_id = 0; /* signal no timeouts pending (this should be fun) */
    return FALSE;
}


/** \brief Draws the tape icon based on the current control and motor status.
 *
 *  \param widget  The tape icon GtkDrawingArea being drawn to.
 *  \param cr      The cairo context that handles the drawing.
 *  \param data    tape port index
 *
 *  \return FALSE, telling GTK to continue event processing
 */
static gboolean draw_tape_icon_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    int width, height;
    double x, y, inset;
    int tape_motor_status;
    int tape_control;
    ui_sb_state_t *sb_state;
    int index = GPOINTER_TO_INT(data);

    /* Copy any sb_state that we need to use - don't hold lock while drawing */
    sb_state = lock_sb_state();
    tape_motor_status = sb_state->tape_motor_status[index];
    tape_control = sb_state->tape_control[index];
    unlock_sb_state();

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);
    if (width > height) {
        x = (width - height) / 2.0;
        y = 0.0;
        inset = height / 10.0;
    } else {
        x = 0.0;
        y = (height - width) / 2.0;
        inset = width / 10.0;
    }

    if (tape_motor_status) {
        cairo_set_source_rgb(cr, 0, 0.75, 0);
    } else {
        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    }
    cairo_rectangle(cr, x + inset, y + inset, inset * 8, inset * 8);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    switch (tape_control) {
    case DATASETTE_CONTROL_STOP:
        cairo_rectangle(cr, x + 2.5*inset, y + 2.5*inset, inset * 5, inset * 5);
        cairo_fill(cr);
        break;
    case DATASETTE_CONTROL_START:
        cairo_move_to(cr, x + 3*inset, y + 2.5*inset);
        cairo_line_to(cr, x + 3*inset, y + 7.5*inset);
        cairo_line_to(cr, x + 7*inset, y + 5*inset);
        cairo_close_path(cr);
        cairo_fill(cr);
        break;
    case DATASETTE_CONTROL_FORWARD:
        cairo_move_to(cr, x + 2.5*inset, y + 2.5*inset);
        cairo_line_to(cr, x + 2.5*inset, y + 7.5*inset);
        cairo_line_to(cr, x + 5*inset, y + 5*inset);
        cairo_close_path(cr);
        cairo_fill(cr);
        cairo_move_to(cr, x + 5*inset, y + 2.5*inset);
        cairo_line_to(cr, x + 5*inset, y + 7.5*inset);
        cairo_line_to(cr, x + 7.5*inset, y + 5*inset);
        cairo_close_path(cr);
        cairo_fill(cr);
        break;
    case DATASETTE_CONTROL_REWIND:
        cairo_move_to(cr, x + 5*inset, y + 2.5*inset);
        cairo_line_to(cr, x + 5*inset, y + 7.5*inset);
        cairo_line_to(cr, x + 2.5*inset, y + 5*inset);
        cairo_close_path(cr);
        cairo_fill(cr);
        cairo_move_to(cr, x + 7.5*inset, y + 2.5*inset);
        cairo_line_to(cr, x + 7.5*inset, y + 7.5*inset);
        cairo_line_to(cr, x + 5*inset, y + 5*inset);
        cairo_close_path(cr);
        cairo_fill(cr);
        break;
    case DATASETTE_CONTROL_RECORD:
        cairo_new_sub_path(cr);
        cairo_arc(cr, x + 5*inset, y + 5*inset, 2.5*inset, 0, 2 * G_PI);
        cairo_close_path(cr);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 1, 0, 0);
        cairo_new_sub_path(cr);
        cairo_arc(cr, x + 5*inset, y + 5*inset, 2*inset, 0, 2 * G_PI);
        cairo_close_path(cr);
        cairo_fill(cr);
        break;
    case DATASETTE_CONTROL_RESET:
    case DATASETTE_CONTROL_RESET_COUNTER:
    default:
        /* Things that aren't really controls look like we stop it. */
        /* TODO: Should RESET_COUNTER be wiped out by the time it gets here? */
        cairo_rectangle(cr, x + 2.5*inset, y + 2.5*inset, inset * 5, inset * 5);
        cairo_fill(cr);
    }

    return FALSE;
}


/** \brief  Handler for the 'activate' event of the 'Configure drives' item
 *
 * Opens the settings UI at the drive settings "page".
 *
 * \param[in]   widget  menu item triggering the event (unused)
 * \param[in]   data    extra event data (unused)
 */
static void on_drive_configure_activate(GtkWidget *widget, gpointer data)
{
    ui_settings_dialog_show("peripheral/drive");
}


/** \brief  Handler for the 'activate' event of the 'Reset drive \#X in ... mode' item
 *
 * Triggers a reset for drive ((int)data + 8).
 *
 * \param[in]   widget  menu item triggering the event (unused)
 * \param[in]   data    drive number (0-3)
 */
static void on_drive_reset_config_clicked(GtkWidget *widget, gpointer data)
{
    drive_cpu_trigger_reset_button(((GPOINTER_TO_INT(data)>>4)&15),
       GPOINTER_TO_INT(data) & 15 );
}

/** \brief Draw the LED associated with some drive's LED state.
 *
 *  \param widget  The drive LED GtkDrawingArea being drawn to.
 *  \param cr      The cairo context that handles the drawing.
 *  \param data    The index (0-3) of which drive this represents.
 *
 *  \return FALSE, telling GTK to continue event processing
 */
static gboolean draw_drive_led_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    int width, height;
    int unit;
    int drive;
    int i;
    double red = 0.0, green = 0.0, x, y, w, h;
    ui_sb_state_t *sb_state;

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);
    unit = GPOINTER_TO_INT(data) & 0xff;
    drive = GPOINTER_TO_INT(data) >> 8;
    sb_state = lock_sb_state();

    /* FIXME: this should display two LEDs some day, right now we combine the
       two LEDs of a drive into one that we display. */
    for (i = 0; i < DRIVE_LEDS_MAX; ++i) {
        int led_color = sb_state->drive_led_types[unit][drive][i];
        if (led_color) {
            green += sb_state->current_drive_leds[unit][drive][i] / 1000.0;
        } else {
            red += sb_state->current_drive_leds[unit][drive][i] / 1000.0;
        }
    }
    unlock_sb_state();

    /* Cairo clamps these for us */
    cairo_set_source_rgb(cr, red, green, 0);
    /* LED is half text height and aims for a 2x1 aspect ratio */
    h = height / 2.0;
    w = 2.0 * h;
    x = (width / 2.0) - h;
    y = height / 4.0;
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);
    return FALSE;
}


/** \brief Draw the current input status from a joyport.
 *
 *  This produces five squares arranged in a + shape, with directions
 *  represented as green squares when active and black when not. The
 *  fire buttons are represented by the central square, with red,
 *  green, and blue components representing the three possible
 *  buttons.
 *
 *  For traditional Commodore joysticks, there is only one fire button
 *  and this will be diplayed as a red square when the button is
 *  pressed.
 *
 *  \param widget  The joyport GtkDrawingArea being drawn to.
 *  \param cr      The cairo context that handles the drawing.
 *  \param data    The index (0-4) of which joyport this represents.
 *
 *  \return FALSE, telling GTK to continue event processing
 */
static gboolean draw_joyport_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    int width, height, val;
    double e, s, x, y;
    ui_sb_state_t *sb_state;

    /* FIXME This is called very often due to cpu/fps label updates
     * triggering a relayout/redraw */

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    sb_state = lock_sb_state();
    val = sb_state->current_joyports[GPOINTER_TO_INT(data)];
    unlock_sb_state();

    /* This widget "wants" to draw 6x6 squares inside a 20x20
     * space. We compute x and y offsets for a scaled square within
     * the real widget space, and then the actual widths for a square
     * edge (e) and the spaces between them (s). */

    if (width > height) {
        s = height / 20.0;
        x = (width - height) / 2.0;
        y = 0.0;
    } else {
        s = width / 20.0;
        y = (height - width) / 2.0;
        x = 0.0;
    }
    e = s * 5.0;

    /* Then we render the five squares. This seems like it could be
     * done more programatically, but enough changes each iteration
     * that we might as well unroll it. */

    /* Up: Bit 0 */
    cairo_set_source_rgb(cr, 0, (val&0x01) ? 1 : 0, 0);
    cairo_rectangle(cr, x + e + 2*s, y+s, e, e);
    cairo_fill(cr);
    /* Down: Bit 1 */
    cairo_set_source_rgb(cr, 0, (val&0x02) ? 1 : 0, 0);
    cairo_rectangle(cr, x + e + 2*s, y + 2*e + 3*s, e, e);
    cairo_fill(cr);
    /* Left: Bit 2 */
    cairo_set_source_rgb(cr, 0, (val&0x04) ? 1 : 0, 0);
    cairo_rectangle(cr, x + s, y + e + 2*s, e, e);
    cairo_fill(cr);
    /* Right: Bit 3 */
    cairo_set_source_rgb(cr, 0, (val&0x08) ? 1 : 0, 0);
    cairo_rectangle(cr, x + 2*e + 3*s, y + e + 2*s, e, e);
    cairo_fill(cr);
    /* Fire buttons: Bits 4-6. Each of the three notional fire buttons
     * controls the red, green, or blue color of the fire button
     * area. By default, we are using one-button joysticks and so this
     * region will be either black or red. */
    cairo_set_source_rgb(cr, (val&0x10) ? 1 : 0,
                             (val&0x20) ? 1 : 0,
                             (val&0x40) ? 1 : 0);
    cairo_rectangle(cr, x + e + 2*s, y + e + 2*s, e, e);
    cairo_fill(cr);

    return FALSE;
}


/** \brief Respond to mouse clicks on the tape status widget.
 *
 *  This displays the tape control popup menu.
 *
 *  \param widget  The GtkWidget that received the click. Ignored.
 *  \param event   The event representing the bottom operation.
 *  \param data    An integer representing which window's status bar was
 *                 clicked and thus where the popup window should go,
 *                 and tape unit number shifted 8 bits to the right.
 *
 *  \return TRUE if further event processing should be skipped.
 *
 *  \todo This callback and the way it is configured both will need to
 *        be significantly reworked to manage multiple tape drives.
 */
static gboolean ui_do_datasette_popup(GtkWidget *widget,
                                      GdkEvent *event,
                                      gpointer data)
{
    int bar_index = GPOINTER_TO_INT(data) & 0xff;
    int port = GPOINTER_TO_INT(data) >> 8;
    int tape_index = port - TAPEPORT_UNIT_1;

    mainlock_assert_is_not_vice_thread();

    if (((GdkEventButton *)event)->button == GDK_BUTTON_PRIMARY) {
        ui_statusbar_t bar;
        GtkWidget *tape_status;
        GtkWidget *tape_menu;

        bar = allocated_bars[bar_index];
        tape_status = bar.tape_status[tape_index];
        tape_menu = bar.tape_menu[tape_index];

        if (tape_status != NULL && tape_menu != NULL) {
            GList *children;
            GList *child;
            gint action_id;

            /* Set accelerators for attach/detach items
             *
             * Needs to happen here since the tape menus are created on status
             * bar initialization and the hotkeys can change in the mean time.
             */
            child = children = gtk_container_get_children(GTK_CONTAINER(tape_menu));

            action_id = port == 1 ? ACTION_TAPE_ATTACH_1 : ACTION_TAPE_ATTACH_2;
            vhk_gtk_set_menu_item_accel_label(GTK_WIDGET(child->data), action_id);

            child = child->next;
            action_id = port == 1 ? ACTION_TAPE_DETACH_1 : ACTION_TAPE_DETACH_2;
            vhk_gtk_set_menu_item_accel_label(GTK_WIDGET(child->data), action_id);

            g_list_free(children);

            ui_datasette_update_sensitive(tape_menu, port);
            gtk_menu_popup_at_widget(GTK_MENU(tape_menu),
                                     tape_status,
                                     GDK_GRAVITY_NORTH_EAST,
                                     GDK_GRAVITY_SOUTH_EAST,
                                     event);
        }
        return TRUE;
    } else if (((GdkEventButton *)event)->button == GDK_BUTTON_SECONDARY) {
        GtkWidget *dir_menu;

        dir_menu = dir_menu_popup_create(port,
                                         0,
                                         tapecontents_read,
                                         tape_dir_autostart_callback);
        gtk_menu_popup_at_widget(GTK_MENU(dir_menu),
                                 widget,
                                 GDK_GRAVITY_NORTH_EAST,
                                 GDK_GRAVITY_SOUTH_EAST,
                                 event);
        return TRUE;
    }
    return FALSE;
}


/** \brief  Respond to mouse clicks on a disk drive status widget.
 *
 *  This displays the drive control popup menu.
 *
 *  \param[in]  widget  The GtkWidget that received the click. Ignored.
 *  \param[in]  event   The event representing the bottom operation.
 *  \param[in]  data    Unit index (bits 0-7) and drive number (bit 8)
 *
 *  \return TRUE if further event processing should be skipped.
 */
static gboolean ui_do_drive_popup(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    GtkWidget *drive_menu;
    GtkWidget *drive_menu_item;
    GtkWidget *label;
    gchar buffer[256];
    GList *children;
    GList *child;
    int action;
    int i = GPOINTER_TO_INT(data) & 0xff;
    int drive = GPOINTER_TO_INT(data) >> 8;
    int buttons;

    mainlock_assert_is_not_vice_thread();

    drive_menu = allocated_bars[0].drive_menu[i][drive];

    /* set "Attach" item label based on dual-drive status */
    child = children = gtk_container_get_children(GTK_CONTAINER(drive_menu));
    if (child != NULL && child->data != NULL) {

        drive_menu_item = child->data;
        label = gtk_bin_get_child(GTK_BIN(drive_menu_item));

        if (drive_is_dualdrive_by_devnr(i + DRIVE_UNIT_MIN)) {
            g_snprintf(buffer, sizeof(buffer),
                       "Attach disk to drive #%d:%d...",
                       i + DRIVE_UNIT_MIN, drive);
        } else {
            g_snprintf(buffer, sizeof(buffer),
                       "Attach disk to drive #%d...",
                       i + DRIVE_UNIT_MIN);
        }
        gtk_label_set_text(GTK_LABEL(label), buffer);

        /* set hotkey, if any */
        action = ui_action_id_drive_attach(i + DRIVE_UNIT_MIN, drive);
        vhk_gtk_set_menu_item_accel_label(drive_menu_item, action);
    }

    /* set "Detach" item label based on dual-drive status */
    child = child->next;
    if (child != NULL && child->data != NULL) {

        drive_menu_item = child->data;
        label = gtk_bin_get_child(GTK_BIN(drive_menu_item));

        if (drive_is_dualdrive_by_devnr(i + DRIVE_UNIT_MIN)) {
            g_snprintf(buffer, sizeof(buffer),
                       "Detach disk from drive #%d:%d...",
                       i + DRIVE_UNIT_MIN, drive);
        } else {
            g_snprintf(buffer, sizeof(buffer),
                       "Detach disk from drive #%d...",
                       i + DRIVE_UNIT_MIN);
        }
        gtk_label_set_text(GTK_LABEL(label), buffer);

        /* set hotkey, if any */
        action = ui_action_id_drive_detach(i + DRIVE_UNIT_MIN, drive);
        vhk_gtk_set_menu_item_accel_label(drive_menu_item, action);
    }

    g_list_free(children);

    ui_populate_fliplist_menu(drive_menu, i + DRIVE_UNIT_MIN, 0);
    /* XXX: this code is a duplicate of the drive_menu creation code, so we
     *      should probably refactor this a bit
     */
    popup_menu_add_separator(drive_menu);

    drive_menu_item = gtk_menu_item_new_with_label("Configure drives ...");
    g_signal_connect(drive_menu_item, "activate",
                     G_CALLBACK(on_drive_configure_activate), NULL);
    gtk_container_add(GTK_CONTAINER(drive_menu), drive_menu_item);

    /*
     * Add drive reset item
     */
    g_snprintf(buffer, sizeof(buffer), "Reset drive #%d", i + DRIVE_UNIT_MIN);
    drive_menu_item = popup_menu_item_action_new(buffer,
                                                 ui_action_id_drive_reset(i + DRIVE_UNIT_MIN));
    gtk_container_add(GTK_CONTAINER(drive_menu), drive_menu_item);

    buttons = drive_has_buttons(i);

    /* Add reset to configuration mode for CMD HDs */
    if ((buttons & DRIVE_BUTTON_WRITE_PROTECT) == DRIVE_BUTTON_WRITE_PROTECT) {
        g_snprintf(buffer, sizeof(buffer),
                   "Reset drive #%d to Configuration Mode",
                   i + DRIVE_UNIT_MIN);
        drive_menu_item = gtk_menu_item_new_with_label(buffer);
        g_signal_connect(drive_menu_item, "activate",
                         G_CALLBACK(on_drive_reset_config_clicked),
                         GINT_TO_POINTER((i << 4) + DRIVE_BUTTON_WRITE_PROTECT));
        gtk_container_add(GTK_CONTAINER(drive_menu), drive_menu_item);
    }

    /* Add reset to installation mode for CMD HDs */
    if ((buttons & (DRIVE_BUTTON_SWAP_8|DRIVE_BUTTON_SWAP_9)) == (DRIVE_BUTTON_SWAP_8|DRIVE_BUTTON_SWAP_9)) {
        g_snprintf(buffer, sizeof(buffer),
                   "Reset drive #%d to Installation Mode",
                   i + DRIVE_UNIT_MIN);
        drive_menu_item = gtk_menu_item_new_with_label(buffer);
        g_signal_connect(drive_menu_item, "activate",
                         G_CALLBACK(on_drive_reset_config_clicked),
                         GINT_TO_POINTER((i << 4) + (DRIVE_BUTTON_SWAP_8|DRIVE_BUTTON_SWAP_9)));
        gtk_container_add(GTK_CONTAINER(drive_menu), drive_menu_item);
    }

    /* Add 'add image to fliplist' */
    gtk_container_add(GTK_CONTAINER(drive_menu), gtk_separator_menu_item_new());

    drive_menu_item = popup_menu_item_action_new(
            "Add current image to fliplist",
            ui_action_id_fliplist_add(i + DRIVE_UNIT_MIN, 0));
    gtk_widget_set_sensitive(drive_menu_item,
                             file_system_get_image(i + DRIVE_UNIT_MIN, 0) != NULL);
    gtk_container_add(GTK_CONTAINER(drive_menu), drive_menu_item);

    /* Add 'clear fliplist' */
    drive_menu_item = popup_menu_item_action_new(
            "Clear fliplist",
            ui_action_id_fliplist_clear(i + DRIVE_UNIT_MIN, 0));
    gtk_widget_set_sensitive(drive_menu_item,
                             fliplist_init_iterate(i + DRIVE_UNIT_MIN) != NULL);
    gtk_container_add(GTK_CONTAINER(drive_menu), drive_menu_item);

    gtk_widget_show_all(drive_menu);

    if (((GdkEventButton *)event)->button == GDK_BUTTON_PRIMARY) {
        /* show popup for attaching/detaching disk images */
        gtk_menu_popup_at_widget(GTK_MENU(drive_menu),
                                 widget,
                                 GDK_GRAVITY_NORTH_EAST,
                                 GDK_GRAVITY_SOUTH_EAST,
                                 event);
    } else if (((GdkEventButton *)event)->button == GDK_BUTTON_SECONDARY) {
        /* show popup to run file in currently attached image */
        GtkWidget *dir_menu = dir_menu_popup_create(i + DRIVE_UNIT_MIN,
                                                    drive,
                                                    diskcontents_filesystem_read,
                                                    disk_dir_autostart_callback);

        /* show popup for selecting file in currently attached image */
        gtk_menu_popup_at_widget(GTK_MENU(dir_menu),
                                 widget,
                                 GDK_GRAVITY_NORTH_EAST,
                                 GDK_GRAVITY_SOUTH_EAST,
                                 event);
    }

    return TRUE;
}


/** \brief  Handler for the enter/leave-notify events of the joysticks widget
 *
 * \param[in]   widget      widget triggering the event
 * \param[in]   event       event reference
 * \param[in]   user_data   extra event data (unused)
 *
 * \return  bool    (FALSE = keep propagating event, TRUE = stop)
 */
static gboolean on_joystick_widget_hover(GtkWidget *widget, GdkEvent *event,
                                         gpointer user_data)
{
    if (event != NULL) {
        GdkDisplay *display = gtk_widget_get_display(widget);
        GdkWindow *window = gtk_widget_get_window(widget);
        GdkCursor *cursor;

        if (display == NULL) {
            debug_gtk3("failed to retrieve GdkDisplay.");
            return FALSE;
        }
        if (window == NULL) {
            debug_gtk3("failed to retrieve GdkWindow.");
            return FALSE;
        }

        if (event->type == GDK_ENTER_NOTIFY) {
            if (joywidget_mouse_ptr == NULL) {
                joywidget_mouse_ptr = gdk_cursor_new_from_name(display, "pointer");
            }
            cursor = joywidget_mouse_ptr;

        } else {
            cursor = NULL;
        }
        gdk_window_set_cursor(window, cursor);
    }
    return FALSE;
}


/** \brief  Handler for button-press events of the joysticks widget
 *
 * \param[in]   widget      widget triggering the event
 * \param[in]   event       event reference
 * \param[in]   user_data   extra event data (unused)
 *
 * \return  TRUE to stop other handlers, FALSE to propagate event further
 */
static gboolean on_joystick_widget_button_press(GtkWidget *widget,
                                                GdkEvent *event,
                                                gpointer user_data)
{
    GdkEventButton *ev = (GdkEventButton *)event;

    mainlock_assert_is_not_vice_thread();

    if (ev->button == GDK_BUTTON_PRIMARY || ev->button == GDK_BUTTON_SECONDARY) {
        GtkWidget *menu = joystick_menu_popup_create();

        gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
                GDK_GRAVITY_NORTH_WEST, GDK_GRAVITY_SOUTH_WEST,
                event);
        return TRUE;
    }
    return FALSE;
}


/** Event handler for hovering over a clickable part of the status bar.
 *
 *  This will switch to or from the "hand" cursor as needed, creating
 *  it if necessary.
 *
 *  \param widget    The widget firing the event
 *  \param event     The GdkEventCross that caused the callback
 *  \param user_data The ui_statusbar_t object containing widget
 *
 *  \return TRUE if further event processing should be blocked.
 */
static gboolean ui_statusbar_cross_cb(GtkWidget *widget,
                                      GdkEvent *event,
                                      gpointer user_data)
{
    ui_statusbar_t *sb = (ui_statusbar_t *)user_data;

    if (event && event->type == GDK_ENTER_NOTIFY) {
        GdkDisplay *display;

        /* Sanity check arguments */
        if (sb == NULL) {
            /* Should be impossible */
            fprintf(stderr, "Error: ui_statusbar_t* is NULL.\n");
            return FALSE;
        }
        /* If the "hand" pointer hasn't been created yet, create it */
        display = gtk_widget_get_display(widget);
        if (display != NULL && sb->hand_ptr == NULL) {
            sb->hand_ptr = gdk_cursor_new_from_name(display, "pointer");
            if (sb->hand_ptr == NULL) {
                fprintf(stderr, "GTK3 CURSOR: Could not allocate custom"
                       " pointer for status bar\n");
            }
        }
        /* If the "hand" pointer is OK, use it */
        if (sb->hand_ptr != NULL) {
            GdkWindow *window = gtk_widget_get_window(widget);
            if (window) {
                gdk_window_set_cursor(window, sb->hand_ptr);
            }
        }
    } else {
        /* We're leaving the target widget, so change the pointer back
         * to default */
        GdkWindow *window = gtk_widget_get_window(widget);

        if (window) {
            gdk_window_set_cursor(window, NULL);
        }
    }
    return FALSE;
}


/** \brief  Widget destruction callback for status bars.
 *
 * \param[in]   sb      The status bar being destroyed. This should be
 *                      registered in some ui_statusbar_t structure as the
 *                      bar field.
 * \param[in]   index   status bar index
 */
static void destroy_statusbar_cb(GtkWidget *sb, gpointer index)
{
    ui_statusbar_t *bar;
    int w;
    int idx = GPOINTER_TO_INT(index);

    bar = &(allocated_bars[idx]);

    /* Invalidate all widget references. We need to do this so we can guard
     * against UI update requests after the UI has been destroyed.
     */
    bar->bar = NULL;
    bar->led_row_grid = NULL;
    bar->widget_row_grid = NULL;
    bar->warp_led = NULL;
    bar->pause_led = NULL;
    bar->shiftlock_led = NULL;
    bar->mode4080_led = NULL;
    bar->capslock_led = NULL;
    bar->diagnosticpin_led = NULL;
    bar->supercpu_turbo_led = NULL;
    bar->supercpu_jiffy_led = NULL;
    bar->speed = NULL;
    bar->msg = NULL;
    bar->record = NULL;
    bar->crt = NULL;
    bar->mixer = NULL;
    for (w = 0; w < TAPEPORT_MAX_PORTS; w++) {
        bar->tape_status[w] = NULL;
        bar->tape_menu[w] = NULL;
    }
    bar->joysticks = NULL;
    for (w = 0; w < NUM_DISK_UNITS; w++) {
        int d;

        bar->drive_unit[w] = NULL;
        for (d = 0; d < DRIVE_UNIT_DRIVE_MAX; d++) {
            bar->drive_menu[w][d] = NULL;
        }
    }
    bar->volume = NULL;
    bar->hand_ptr = NULL;
    bar->kbd_debug = NULL;
}


/** \brief  Handler for the 'toggled' event of the CRT controls checkbox
 *
 * Toggles the display state of the CRT controls
 *
 * \param[in]   widget  checkbox triggering the event
 * \param[in]   data    extra event data (unused
 */
static void on_crt_toggled(GtkWidget *widget, gpointer data)
{
    gboolean state;

    mainlock_assert_is_not_vice_thread();

    state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    ui_enable_crt_controls((gboolean)state);
}


/** \brief  Handler for the 'toggled' event of the mixer controls checkbox
 *
 * Toggles the display state of the mixer controls
 *
 * \param[in]   widget  checkbox triggering the event
 * \param[in]   data    extra event data (unused
 */
static void on_mixer_toggled(GtkWidget *widget, gpointer data)
{
    gboolean state;

    mainlock_assert_is_not_vice_thread();

    state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    ui_enable_mixer_controls((gboolean)state);
}


/** \brief  Handler for the 'value-changed' event of the volume control
 *
 * Updates the master volume
 *
 * \param[in]   widget  GtkVolumeButton control
 * \param[in]   value   new volume value (1.0 - 0.0)
 * \param[in]   data    extra event data (unused
 */
static void on_volume_value_changed(GtkScaleButton *widget,
                                    gdouble value,
                                    gpointer data)
{
    resources_set_int("SoundVolume", (int)(value * 100.0));
}


/*****************************************************************************
 *                          Private functions                                *
 ****************************************************************************/


/** \brief Extracts the list of enabled drives from the DriveType
 *         resources.
 *
 *  \return A bitmask value suitable for ui_sb_state_s::drives_enabled.
 */
static int compute_drives_enabled_mask(void)
{
    int unit, mask;
    int result = 0;
    for (unit = 0, mask = 1; unit < NUM_DISK_UNITS; ++unit, mask <<= 1) {
        int status = 0, value = 0;
        status = resources_get_int_sprintf("Drive%dType", &value, unit + DRIVE_UNIT_MIN);
        if (status == 0 && value != 0) {
            result |= mask;
        }
    }
    return result;
}


/** \brief Create a new drive widget for inclusion in the status bar.
 *
 *  \param unit The drive unit to create (0-3, indicating devices
 *              8-11)
 *
 *  \return The constructed widget. This widget will be a floating
 *          reference.
 */
static GtkWidget *ui_drive_widget_create(int unit, int bar_index)
{
    GtkWidget *grid;
    GtkWidget *number;
    GtkWidget *track;
    GtkWidget *led;
    GtkCssProvider *drive_css;
    int drive_num;

    mainlock_assert_is_not_vice_thread();

    grid = gtk_grid_new();
    gtk_widget_set_hexpand(grid, FALSE);
    gtk_widget_set_vexpand(grid, FALSE);
    /* create reusable CSS provider for the unit/drive and track labels */
    drive_css = vice_gtk3_css_provider_new(DRIVE_WIDGET_CSS);

    for (drive_num = 0; drive_num < DRIVE_UNIT_DRIVE_MAX; drive_num++) {
        GtkWidget *drive_grid;  /* grid for a single drive of a unit */
        GtkWidget *event_box;
        char drive_id[16];

        g_snprintf(drive_id,
                   sizeof(drive_id),
                   "%2d:%d",
                   unit + DRIVE_UNIT_MIN,
                   drive_num);
        number = gtk_label_new(drive_id);
        gtk_widget_set_halign(number, GTK_ALIGN_START);
        vice_gtk3_css_provider_add(number, drive_css);

        track = gtk_label_new(" 18.5");
        gtk_widget_set_hexpand(track, TRUE);
        gtk_widget_set_halign(track, GTK_ALIGN_END);
        vice_gtk3_css_provider_add(track, drive_css);

        led = gtk_drawing_area_new();
        gtk_widget_set_size_request(led, 30, 15);
        gtk_widget_set_no_show_all(led, TRUE);
        gtk_widget_set_has_window(led, TRUE);
        /* Labels will notice clicks by default, but drawing areas need to
         * be told to. */
        gtk_widget_add_events(led, GDK_BUTTON_PRESS_MASK|GDK_ENTER_NOTIFY_MASK|GDK_LEAVE_NOTIFY_MASK);
        g_signal_connect_unlocked(led,
                                  "draw",
                                  G_CALLBACK(draw_drive_led_cb),
                                  GINT_TO_POINTER(unit | (drive_num << 8)));
        /* for some reason we need to set the crossing event handlers on the led
         * itself, otherwise changing the mouse pointer on hover doesn't work. */
        g_signal_connect(led,
                         "enter-notify-event",
                         G_CALLBACK(ui_statusbar_cross_cb),
                         &allocated_bars[bar_index]);
        g_signal_connect(led,
                         "leave-notify-event",
                         G_CALLBACK(ui_statusbar_cross_cb),
                         &allocated_bars[bar_index]);

        /* wrap the widgets into a grid */
        drive_grid = gtk_grid_new();
        gtk_widget_set_hexpand(drive_grid, FALSE);
        gtk_widget_set_vexpand(drive_grid, FALSE);

        gtk_grid_attach(GTK_GRID(drive_grid), number, DRIVE_STATUS_NUMBER, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(drive_grid), track, DRIVE_STATUS_HEAD, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(drive_grid), led, DRIVE_STATUS_LED, 0, 1, 1);
        gtk_widget_show_all(drive_grid);


        event_box = gtk_event_box_new();
        gtk_event_box_set_visible_window(GTK_EVENT_BOX(event_box), FALSE);
        /* FIXME: unit number should be passed as-is, not as index! */
        g_signal_connect(event_box,
                         "button-press-event",
                         G_CALLBACK(ui_do_drive_popup),
                         GINT_TO_POINTER(unit | (drive_num << 8)));
        g_signal_connect(event_box,
                         "enter-notify-event",
                         G_CALLBACK(ui_statusbar_cross_cb),
                         &allocated_bars[bar_index]);
        g_signal_connect(event_box,
                         "leave-notify-event",
                         G_CALLBACK(ui_statusbar_cross_cb),
                         &allocated_bars[bar_index]);

        gtk_container_add(GTK_CONTAINER(event_box), drive_grid);
        gtk_widget_show_all(event_box);

        gtk_grid_attach(GTK_GRID(grid), event_box, 0, drive_num, 1, 1);
    }

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Get disk unit widget
 *
 * \param[in]   bar     status bar index (0 or 1)
 * \param[in]   unit    unit number (8-11)
 *
 * \return  GtkGrid
 */
static GtkWidget *drive_get_unit_widget(int bar, int unit)
{
    return allocated_bars[bar].drive_unit[unit - DRIVE_UNIT_MIN];
}


/** \brief  Get drive status widget
 *
 * \param[in]   bar     status bar index (0 or 1)
 * \param[in]   unit    unit number (8-11)
 * \param[in]   drive   drive number (0 or 1)
 *
 * \return  GtkEventBox containing the GtkGrid with the drive widgets
 *
 * \note    Perhaps this can be changed to return the contained GtkGrid if we
 *          find out we never need to work directly with the GtkEventBox.
 */
static GtkWidget *drive_get_drive_widget(int bar, int unit, int drive)
{
    GtkWidget *unit_widget;

    unit_widget = allocated_bars[bar].drive_unit[unit - DRIVE_UNIT_MIN];
    return gtk_grid_get_child_at(GTK_GRID(unit_widget), 0, drive);
}


/** \brief  Get child widget of a drive status widget
 *
 * \param[in]   bar     status bar index (0 or 1)
 * \param[in]   unit    unit number (8-11)
 * \param[in]   drive   drive number (0 or 1)
 * \param[in]   column  column, see #DRIVE_STATUS_NUMBER, #DRIVE_STATUS_HEAD,
 *                      #DRIVE_STATUS_LED
 *
 * \return  Child widget
 */
static GtkWidget *drive_get_child_widget(int bar, int unit, int drive, int column)
{
    GtkWidget *event_box;
    GtkWidget *grid;

    event_box = drive_get_drive_widget(bar, unit, drive);
    if (event_box != NULL) {
        grid = gtk_bin_get_child(GTK_BIN(event_box));
        if (grid != NULL) {
            return gtk_grid_get_child_at(GTK_GRID(grid), column, 0);
        }
    }
    return NULL;
}


/** \brief  Get widget displaying the unit[:drive] number
 *
 * \param[in]   bar     status bar index (0 or 1)
 * \param[in]   unit    unit number (8-11)
 * \param[in]   drive   drive number (0 or 1)
 *
 * \return  GtkLabel
 */
static GtkWidget *drive_get_number_widget(int bar, int unit, int drive)
{
    return drive_get_child_widget(bar, unit, drive, DRIVE_STATUS_NUMBER);
}


/** \brief  Get widget displaying the drive head position
 *
 * \param[in]   bar     status bar index (0 or 1)
 * \param[in]   unit    unit number (8-11)
 * \param[in]   drive   drive number (0 or 1)
 *
 * \return  GtkLabel
 */
static GtkWidget *drive_get_head_widget(int bar, int unit, int drive)
{
    return drive_get_child_widget(bar, unit, drive, DRIVE_STATUS_HEAD);
}


/** \brief  Get widget displaying the LED
 *
 * \param[in]   bar     status bar index (0 or 1)
 * \param[in]   unit    unit number (8-11)
 * \param[in]   drive   drive number (0 or 1)
 *
 * \return  GtkDrawingArea
 */
static GtkWidget *drive_get_led_widget(int bar, int unit, int drive)
{
    return drive_get_child_widget(bar, unit, drive, DRIVE_STATUS_LED);
}


/** \brief  Callback for the disk directory popup menu
 *
 * Autostarts the selected file in the directory
 *
 * \param[in]   image   image name
 * \param[in]   index   directory index of the file to start
 * \param[in]   device  device number (0-3)
 * \param[in]   drive   drive number (0 or 1) of device
 */
static void disk_dir_autostart_callback(const char *image,
                                        int index,
                                        int device,
                                        unsigned int drive)
{
    char *autostart_image;

    /* make a copy of the image name since autostart will reattach the disk
     * image, freeing memory used by the image name passed to us in the process
     */
    autostart_image = lib_strdup(image);
    autostart_disk(device + 8, drive, autostart_image, NULL, index + 1, AUTOSTART_MODE_RUN);
    lib_free(autostart_image);
}


/** \brief  Callback for the tape directory popup menu
 *
 * Autostarts the selected file in the directory
 *
 * \param[in]   image   image name
 * \param[in]   index   directory index of the file to start
 * \param[in]   device  device number (unused, but perhaps useful for PET)
 * \param[in]   drive   drive number (unused)
 */
static void tape_dir_autostart_callback(const char *image,
                                        int index,
                                        int device,
                                        unsigned int drive)
{
    char *autostart_image;

    /* make a copy of the image name since autostart will reattach the tape
     * image, freeing memory used by the image name passed to us in the process
     */
    autostart_image = lib_strdup(image);
    autostart_tape(autostart_image, NULL, index + 1, AUTOSTART_MODE_RUN, TAPEPORT_PORT_1 /* FIXME */);
    lib_free(autostart_image);
}


/** \brief Create a new tape widget for inclusion in the status bar.
 *
 * \param[in]   port    port number
 * \param[in]   bar     status bar index
 *
 *  \return The constructed widget. This widget will be a floating
 *          reference.
 */
static GtkWidget *ui_tape_widget_create(int port, int bar)
{
    GtkWidget *grid;
    GtkWidget *header;
    GtkWidget *counter;
    GtkWidget *motor;
    gchar title[256];

    mainlock_assert_is_not_vice_thread();

    grid = gtk_grid_new();
    gtk_widget_set_hexpand(grid, FALSE);
    gtk_widget_set_vexpand(grid, FALSE);

    if (machine_class == VICE_MACHINE_PET) {
        g_snprintf(title, sizeof(title), "Tape #%d:", port);
        header = gtk_label_new(title);
    } else {
        header = gtk_label_new("Tape:");
    }
    gtk_widget_set_hexpand(header, FALSE);
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_widget_set_margin_end(header, 8);

    counter = gtk_label_new("?");

    motor = gtk_drawing_area_new();
    gtk_widget_set_size_request(motor, 20, 20);
    /* Labels will notice clicks by default, but drawing areas need to
     * be told to. */
    gtk_widget_add_events(motor, GDK_BUTTON_PRESS_MASK|GDK_ENTER_NOTIFY_MASK|GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect(motor, "enter-notify-event",
            G_CALLBACK(ui_statusbar_cross_cb), &allocated_bars[bar]);
    g_signal_connect(motor, "leave-notify-event",
            G_CALLBACK(ui_statusbar_cross_cb), &allocated_bars[bar]);

    gtk_grid_attach(GTK_GRID(grid), header, TAPE_STATUS_COL_HEADER, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), counter, TAPE_STATUS_COL_COUNTER, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), motor, TAPE_STATUS_COL_MOTOR, 0, 1, 1);
    g_signal_connect_unlocked(motor,
                              "draw",
                              G_CALLBACK(draw_tape_icon_cb),
                              GINT_TO_POINTER(port - 1));

    return grid;
}


/** \brief Create a bitfield of enabled joyports for detecting changes
 */
static uint32_t build_active_joyport_mask(void)
{
    int active_joyport_mask = 0;
    int i;

    for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
        active_joyport_mask <<= 1;
        if (joyport_port_is_active(i)) {
            active_joyport_mask |= 1;
        }
    }

    return active_joyport_mask;
}

/** \brief Alter widget visibility within the joyport widget so that
 *         only currently existing joystick ports are displayed.
 */
static void update_joyport_layout(void)
{
    int i;
    int j;

    for (j = 0; j < MAX_STATUS_BARS; ++j) {
        GtkWidget *joyports_grid;
        GtkWidget *child;
        int active;

        if (allocated_bars[j].joysticks == NULL) {
            continue;
        }
        joyports_grid =  gtk_bin_get_child(GTK_BIN(allocated_bars[j].joysticks));

        /* Hide and show the joystick ports as required */
        active = 0;     /* count number of active joysticks */
        for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
            child = gtk_grid_get_child_at(GTK_GRID(joyports_grid),
                                          i + JOYSTICK_COL_STATUS, 0);
            if (child) {
                if (joyport_port_is_active(i)) {
                    gtk_widget_set_no_show_all(child, FALSE);
                    gtk_widget_show_all(child);
                    active++;
                } else {
                    gtk_widget_set_no_show_all(child, TRUE);
                    gtk_widget_hide(child);
                }
            }
        }
        /* Hide the label when no joysticks are active */
        child = gtk_grid_get_child_at(GTK_GRID(joyports_grid),
                                      JOYSTICK_COL_LABEL, 0);
        if (child != NULL) {
            if (active == 0) {
                /* hide the label */
                gtk_widget_hide(child);
            } else {
                gtk_widget_show(child);
            }
        }
    }
}


/** \brief  Create a master joyport widget for inclusion in the status bar.
 *
 *  Individual joyport representations are part of this widget and
 *  update functions will index the GtkGrid in the master widget to
 *  reach them.
 *
 *  \return The constructed widget. This widget will be a floating
 *          reference.
 */
static GtkWidget *ui_joystick_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *event_box;
    int i;

    mainlock_assert_is_not_vice_thread();

    grid = gtk_grid_new();
    gtk_orientable_set_orientation(GTK_ORIENTABLE(grid),
            GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_hexpand(grid, FALSE);
    label = gtk_label_new("Joysticks:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, FALSE);
    gtk_widget_set_margin_end(label, 8);
    gtk_container_add(GTK_CONTAINER(grid), label);
    /* Create all possible joystick displays */
    for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
        GtkWidget *joyport = gtk_drawing_area_new();
        /* add events it should respond to */
        gtk_widget_add_events(joyport,
                GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|
                GDK_ENTER_NOTIFY_MASK|GDK_LEAVE_NOTIFY_MASK);
        gtk_widget_set_size_request(joyport,20,20);
        gtk_container_add(GTK_CONTAINER(grid), joyport);
        g_signal_connect_unlocked(joyport, "draw", G_CALLBACK(draw_joyport_cb),
                GINT_TO_POINTER(i));
        g_signal_connect(joyport, "enter-notify-event",
                G_CALLBACK(on_joystick_widget_hover), NULL);
        g_signal_connect(joyport, "leave-notify-event",
                G_CALLBACK(on_joystick_widget_hover), NULL);
        gtk_widget_set_no_show_all(joyport, TRUE);
        gtk_widget_hide(joyport);
    }

    /*
     * Pack the joystick grid into an event box so we can have a popup menu and
     * also change the cursor shape to indicate to the user the joystick widget
     * is clickable.
     */
    event_box = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(event_box), FALSE);
    gtk_container_add(GTK_CONTAINER(event_box), grid);

    g_signal_connect(event_box, "button-press-event",
            G_CALLBACK(on_joystick_widget_button_press), NULL);
    g_signal_connect(event_box, "enter-notify-event",
            G_CALLBACK(on_joystick_widget_hover), NULL);
    g_signal_connect(event_box, "leave-notify-event",
            G_CALLBACK(on_joystick_widget_hover), NULL);

    return event_box;
}


/** \brief Lay out the disk drive widgets inside a status bar.
 *
 * Enable/disable unit, drive, LED widgets based on current configuration.
 *
 * \param[in]   bar_index   Which status bar to lay out.
 */
static void layout_statusbar_drives(ui_sb_state_t *state_snapshot, int bar_index)
{
    int unit;
    int state;
    int tde;
    GtkWidget *bar = allocated_bars[bar_index].bar;

    if (bar == NULL) {
        return;
    }

    state = state_snapshot->drives_enabled;
    tde = state_snapshot->drives_tde_enabled;

    for (unit = DRIVE_UNIT_MIN; unit <= DRIVE_UNIT_MAX; unit++) {
        GtkWidget *unit_widget;
        GtkWidget *drive0_widget;
        GtkWidget *drive1_widget;
        GtkWidget *led0_widget;
        GtkWidget *led1_widget;
        unsigned int dual;

        unit_widget = drive_get_unit_widget(bar_index, unit);
        drive0_widget = drive_get_drive_widget(bar_index, unit, DRIVE_UNIT_DRIVE_0);
        drive1_widget = drive_get_drive_widget(bar_index, unit, DRIVE_UNIT_DRIVE_1);
        led0_widget = drive_get_led_widget(bar_index, unit, DRIVE_UNIT_DRIVE_0);
        led1_widget = drive_get_led_widget(bar_index, unit, DRIVE_UNIT_DRIVE_1);
        dual = drive_check_dual(diskunit_context[unit - DRIVE_UNIT_MIN]->type);

        /* update unit and drives visibility */
        if (state & 1) {
            gtk_widget_show(unit_widget);
            gtk_widget_show(drive0_widget);
            if (dual) {
                gtk_widget_show(drive1_widget);
            } else {
                gtk_widget_hide(drive1_widget);
            }
        } else {
            gtk_widget_hide(unit_widget);
            gtk_widget_hide(drive0_widget);
            gtk_widget_hide(drive1_widget);
        }
        /* update LED visibility */
        if (tde & 1) {
            gtk_widget_show(led0_widget);
            gtk_widget_show(led1_widget);
        } else {
            gtk_widget_hide(led0_widget);
            gtk_widget_hide(led1_widget);
        }

        state >>= 1;
        tde >>= 1;
        dual >>= 1;
    }
}


/** \brief  Callback for the 'attach' disk popup menu item
 *
 * Trigger the proper 'attach' UI action for the given unit and drive.
 *
 * \param[in]   item    menu item (unused)
 * \param[in]   data    unit and drive number
 *
 * \return  `TRUE`
 */
static gboolean on_disk_attach(GtkWidget *item, gpointer data)
{
    gint unit;
    gint drive;

    unit = GPOINTER_TO_INT(data) >> 8;
    drive = GPOINTER_TO_INT(data) & 0xff;
    ui_action_trigger(ui_action_id_drive_attach(unit, drive));
    return TRUE;
}


/** \brief  Callback for the 'attach' disk popup menu item
 *
 * Trigger the proper 'detach' UI action for the given unit and drive.
 *
 * \param[in]   item    menu item (unused)
 * \param[in]   data    unit and drive number
 *
 * \return  `TRUE`
 */

static gboolean on_disk_detach(GtkWidget *item, gpointer data)
{
    gint unit;
    gint drive;

    unit = GPOINTER_TO_INT(data) >> 8;
    drive = GPOINTER_TO_INT(data) & 0xff;
    ui_action_trigger(ui_action_id_drive_detach(unit, drive));
    return TRUE;
}


/** \brief Create a popup menu to attach to a disk drive widget.
 *
 *  \param[in]  unit    unit number (8-11)
 *  \param[in]  drive   drive number (0 or 1)
 *
 *  \return The GtkMenu for use as a popup, as a floating reference.
 */
static GtkWidget *ui_drive_menu_create(gint unit, gint drive)
{
    GtkWidget *drive_menu;
    GtkWidget *drive_menu_item;

    mainlock_assert_is_not_vice_thread();

    drive_menu = gtk_menu_new();
    drive_menu_item = gtk_menu_item_new_with_label("Attach <fill-in-details>");
    g_signal_connect(drive_menu_item, "activate",
            G_CALLBACK(on_disk_attach),
            UNIT_DRIVE_TO_PTR(unit, drive));
    gtk_container_add(GTK_CONTAINER(drive_menu), drive_menu_item);

    drive_menu_item = gtk_menu_item_new_with_label("Detach <fill-in-details>");
    g_signal_connect(drive_menu_item, "activate",
                     G_CALLBACK(on_disk_detach),
                     UNIT_DRIVE_TO_PTR(unit, drive));
    gtk_container_add(GTK_CONTAINER(drive_menu), drive_menu_item);

    /* GTK2/GNOME UI put TDE and Read-only checkboxes here, but that
     * seems excessive or possibly too fine-grained, so skip that for
     * now. Also: make fliplist usable for drive 1. */
    ui_populate_fliplist_menu(drive_menu, unit, drive);
    gtk_container_add(GTK_CONTAINER(drive_menu),
            gtk_separator_menu_item_new());

    drive_menu_item = gtk_menu_item_new_with_label("Configure drives...");
    g_signal_connect(drive_menu_item, "activate",
            G_CALLBACK(on_drive_configure_activate), NULL);
    gtk_container_add(GTK_CONTAINER(drive_menu), drive_menu_item);

    gtk_widget_show_all(drive_menu);
    return drive_menu;
}


/** \brief  Create volume button for the status bar
 *
 * \return  GtkVolumeButton
 */
static GtkWidget *ui_volume_button_create(void)
{
    GtkWidget *volume;
    int sound_vol = 0;

    volume = gtk_volume_button_new();
    gtk_widget_set_can_focus(volume, FALSE);

    resources_get_int("SoundVolume", &sound_vol);
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(volume),
            (gdouble)sound_vol / 100.0);

    g_object_set(volume, "use-symbolic", TRUE, NULL);

    g_signal_connect(volume, "value-changed",
            G_CALLBACK(on_volume_value_changed), NULL);

    return volume;
}


/** \brief  Callback function for the Warp mode LED
 *
 * \param[in]   led     warp mode LED
 * \param[in]   active  new state of the LED
 */
static void warp_led_callback(GtkWidget *widget, gboolean active)
{
    /* this updates warp state throughout the UI */
    ui_action_trigger(ACTION_WARP_MODE_TOGGLE);
}

/** \brief  Create status bar LED for Warp mode
 *
 * \return  LED widget
 */
static GtkWidget *warp_led_create(void)
{
    GtkWidget *led = statusbar_led_widget_create("warp:", "#00ff00", "#000");

    statusbar_led_widget_set_toggleable(led, TRUE);
    statusbar_led_widget_set_toggle_callback(led, warp_led_callback);
    gtk_widget_show_all(led);
    return led;
}

/** \brief  Set Warp mode LED state
 *
 * \param[in]   bar     status bar index
 * \param[in]   active  LED status
 */
void warp_led_set_active(int bar, gboolean active)
{
    GtkWidget *led = allocated_bars[bar].warp_led;

    if (led != NULL) {
        statusbar_led_widget_set_active(led, active);
    }
}

/** \brief  Callback function for the Pause LED
 *
 * \param[in]   led     pause LED
 * \param[in]   active  new state of the LED
 */
static void pause_led_callback(GtkWidget *widget, gboolean active)
{
    /* this updates pause state throughout the UI */
    ui_action_trigger(ACTION_PAUSE_TOGGLE);
}


/** \brief  Create status bar LED for Pause
 *
 * \return  LED widget
 */
static GtkWidget *pause_led_create(void)
{
    GtkWidget *led = statusbar_led_widget_create("pause:", "#ff0000", "#000");

    statusbar_led_widget_set_toggleable(led, TRUE);
    statusbar_led_widget_set_toggle_callback(led, pause_led_callback);
    gtk_widget_show_all(led);
    return led;
}


/** \brief  Set Pause LED state
 *
 * \param[in]   bar     status bar index
 * \param[in]   active  LED status
 */
void pause_led_set_active(int bar, gboolean active)
{
    GtkWidget *led = allocated_bars[bar].pause_led;

    if (led != NULL) {
        statusbar_led_widget_set_active(led, active);
    }
}

/** \brief  Create status bar LED for shiftlock
 *
 * \return  LED widget
 */
static GtkWidget *shiftlock_led_create(void)
{
    GtkWidget *led = statusbar_led_widget_create("shift-lock:", "#ff0000", "#000");
    /*statusbar_led_widget_set_toggleable(led, TRUE);
      statusbar_led_widget_set_toggle_callback(led, shiftlock_led_callback);*/
    gtk_widget_show_all(led);
    return led;
}


/** \brief  Set shiftlock LED state
 *
 * \param[in]   bar     status bar index
 * \param[in]   active  LED status
 */
void shiftlock_led_set_active(int bar, gboolean active)
{
    GtkWidget *led = allocated_bars[bar].shiftlock_led;

    if (led != NULL) {
        statusbar_led_widget_set_active(led, active);
    }
}

/** \brief  Callback function for the 40/80 LED
 *
 * \param[in]   led     40/80 LED
 * \param[in]   active  new state of the LED
 */
static void mode4080_led_callback(GtkWidget *widget, gboolean active)
{
    keyboard_custom_key_toggle(KBD_CUSTOM_4080);
}

/** \brief  Create status bar LED for 40/80 key
 *
 * \return  LED widget
 */
static GtkWidget *mode4080_led_create(void)
{
    GtkWidget *led = statusbar_led_widget_create("80col:", "#00ff00", "#000");

    statusbar_led_widget_set_toggleable(led, TRUE);
    statusbar_led_widget_set_toggle_callback(led, mode4080_led_callback);
    gtk_widget_show_all(led);
    return led;
}

/** \brief  Set 40/80 LED state
 *
 * \param[in]   bar     status bar index
 * \param[in]   active  LED status
 */
void mode4080_led_set_active(int bar, gboolean active)
{
    GtkWidget *led = allocated_bars[bar].mode4080_led;

    if (led != NULL) {
        statusbar_led_widget_set_active(led, active);
    }
}

/** \brief  Callback function for the capslock LED
 *
 * \param[in]   led     capslock LED
 * \param[in]   active  new state of the LED
 */
static void capslock_led_callback(GtkWidget *widget, gboolean active)
{
    keyboard_custom_key_toggle(KBD_CUSTOM_CAPS);
}

/** \brief  Create status bar LED for capslock key
 *
 * \return  LED widget
 */
static GtkWidget *capslock_led_create(void)
{
    GtkWidget *led = statusbar_led_widget_create("caps:", "#00ff00", "#000");

    statusbar_led_widget_set_toggleable(led, TRUE);
    statusbar_led_widget_set_toggle_callback(led, capslock_led_callback);
    gtk_widget_show_all(led);
    return led;
}

/** \brief  Set capslock LED state
 *
 * \param[in]   bar     status bar index
 * \param[in]   active  LED status
 */
void capslock_led_set_active(int bar, gboolean active)
{
    GtkWidget *led = allocated_bars[bar].capslock_led;

    if (led != NULL) {
        statusbar_led_widget_set_active(led, active);
    }
}

/** \brief  Callback function for the PET diagnostic pin LED
 *
 * \param[in]   led     diagnostic pin LED
 * \param[in]   active  new state of the LED
 */
static void diagnosticpin_led_callback(GtkWidget *widget, gboolean active)
{
    resources_set_int("DiagPin", active ? 1 : 0);
}

static GtkWidget *diagnosticpin_led_create(void)
{
    GtkWidget *led = statusbar_led_widget_create("diag-pin:", "#ffa500", "#000");

    statusbar_led_widget_set_toggleable(led, TRUE);
    statusbar_led_widget_set_toggle_callback(led, diagnosticpin_led_callback);
    gtk_widget_show_all(led);
    return led;
}

/** \brief  Set PET userport diagnostic pin led state
 *
 * \param[in]   bar     status bar index
 * \param[in]   active  new state for led
 */
void diagnosticpin_led_set_active(int bar, gboolean active)
{
    GtkWidget *led = allocated_bars[bar].diagnosticpin_led;

    if (led != NULL) {
        statusbar_led_widget_set_active(led, active);
    }
}

void diagnosticpin_led_set_visible(int bar, gboolean active)
{
    GtkWidget *led = allocated_bars[bar].diagnosticpin_led;

    if (led != NULL) {
        gtk_widget_set_visible(led, active);
        gtk_widget_set_sensitive(led, active);
    }
}

/** \brief  Callback for the SuperCPU turbo LED
 *
 * Set resource "SpeedSwitch" to \a active.
 *
 * \param[in]   self    turbo LED (ignored)
 * \param[in]   active  new LED state
 */
static void supercpu_turbo_led_callback(GtkWidget *self, gboolean active)
{
    resources_set_int("SpeedSwitch", active ? 1 : 0);
}

/** \brief  Create SuperCPU "turbo" led widget
 *
 * \return  LED widget
 */
static GtkWidget *supercpu_turbo_led_create(void)
{
    GtkWidget *led;
    int        speed = 0;

    /* according to images of the actual SuperCPU it has a red LED to indicate
     * turbo mode */
    led = statusbar_led_widget_create("turbo:", "#ff0000", "#000");
    resources_get_int("SpeedSwitch", &speed);
    statusbar_led_widget_set_active(led, speed);
    statusbar_led_widget_set_toggleable(led, TRUE);
    statusbar_led_widget_set_toggle_callback(led, supercpu_turbo_led_callback);
    gtk_widget_show_all(led);
    return led;
}

/** \brief  Set SCPU64 turbo led state
 *
 * \param[in]   bar     status bar index
 * \param[in]   active  new state for led
 */
void supercpu_turbo_led_set_active(int bar, gboolean active)
{
    GtkWidget *led = allocated_bars[bar].supercpu_turbo_led;

    if (led != NULL) {
        statusbar_led_widget_set_active(led, active);
    }
}

/** \brief  Callback for the SuperCPU JiffyDOS LED
 *
 * Set resource "SpeedSwitch" to \a active.
 *
 * \param[in]   self    turbo LED (ignored)
 * \param[in]   active  new LED state
 */
static void supercpu_jiffy_led_callback(GtkWidget *self, gboolean active)
{
    resources_set_int("JiffySwitch", active ? 1 : 0);
}

/** \brief  Create SuperCPU JiffyDOS LED widget
 *
 * \return  LED widget
 */
static GtkWidget *supercpu_jiffy_led_create(void)
{
    GtkWidget *led;
    int        jiffy = 0;

    /* according to images of the actual SuperCPU it has a red LED to indicate
     * turbo mode */
    led = statusbar_led_widget_create("JiffyDOS:", "#00ff00", "#000");
    resources_get_int("JiffySwitch", &jiffy);
    statusbar_led_widget_set_active(led, jiffy);
    statusbar_led_widget_set_toggleable(led, TRUE);
    statusbar_led_widget_set_toggle_callback(led, supercpu_jiffy_led_callback);
    gtk_widget_show_all(led);
    return led;
}

/** \brief  Set SCPU64 JiffyDOS LED state
 *
 * \param[in]   bar     status bar index
 * \param[in]   active  new state for led
 */
void supercpu_jiffy_led_set_active(int bar, gboolean active)
{
    GtkWidget *led = allocated_bars[bar].supercpu_jiffy_led;

    if (led != NULL) {
        statusbar_led_widget_set_active(led, active);
    }
}


/** \brief  Get status bar index for \a window
 *
 * \param[in]   window  GtkWindow instance
 *
 * \return  index or -1 on error
 */
int ui_statusbar_index_for_window(GtkWidget *window)
{
    GtkWidget *bin;

    bin = gtk_bin_get_child(GTK_BIN(window));
    if (bin != NULL) {
        GtkWidget *bar = gtk_grid_get_child_at(GTK_GRID(bin), 0, 2);
        int i;

        for (i = 0; i < MAX_STATUS_BARS; i++) {
            if (allocated_bars[i].bar == bar) {
                return i;
            }
        }
    }
    return -1;
}


/** \brief  Append a widget to the LEDs row of a status bar
 *
 * Append \a led to status bar with index \a bar, optionally adding a
 * separator before the new widget.
 *
 * \param[in]   bar         status bar index
 * \param[in]   widget      widget to append
 * \param[in]   separator   append separator before appending the widget
 */
static void statusbar_append_led(int bar, GtkWidget *led, gboolean separator)
{
    GtkWidget *grid;
    int column;

    /* sanity check */
    if (bar < 0 || bar >= MAX_STATUS_BARS) {
        log_error(LOG_DEFAULT, "Invalid status bar index of %d.", bar);
        return;
    }

    grid = allocated_bars[bar].led_row_grid;
    column = allocated_bars[bar].led_row_column;

    if (separator && column > 0) {
        GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);

        gtk_grid_attach(GTK_GRID(grid), sep, column, 0, 1, 1);
        allocated_bars[bar].led_row_column = ++column;
    }

    gtk_grid_attach(GTK_GRID(grid), led, column, 0, 1, 1);
    allocated_bars[bar].led_row_column++;
}


/** \brief  Append a widget to the widgets row of a status bar
 *
 * Append \a widget to status bar with index \a bar, optionally adding a
 * separator before the new widget.
 *
 * \param[in]   bar         status bar index
 * \param[in]   widget      widget to append
 * \param[in]   separator   append separator before appending the widget
 */
static void statusbar_append_widget(int bar, GtkWidget *widget, gboolean separator)
{
    GtkWidget *grid;
    int column;

    /* sanity check */
    if (bar < 0 || bar >= MAX_STATUS_BARS) {
        log_error(LOG_DEFAULT, "Invalid status bar index of %d.", bar);
        return;
    }

    grid = allocated_bars[bar].widget_row_grid;
    column = allocated_bars[bar].widget_row_column;

    if (separator && column > 0) {
        GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);

        gtk_grid_attach(GTK_GRID(grid), sep, column, 0, 1, 1);
        allocated_bars[bar].widget_row_column = ++column;
    }

    gtk_grid_attach(GTK_GRID(grid), widget, column, 0, 1, 1);
    allocated_bars[bar].widget_row_column++;
}


/** \brief  Append a widget to the top row of a status bar at the end
 *
 * Append \a widget to status bar with index \a bar.
 *
 * \param[in]   bar         status bar index
 * \param[in]   widget      widget to append
 * \param[in]   separator   append separator before appending the widget
 */
static void statusbar_append_widget_end(int bar, GtkWidget *widget)
{
    GtkWidget *grid;
    int column;

    /* sanity check */
    if (bar < 0 || bar >= MAX_STATUS_BARS) {
        log_error(LOG_DEFAULT, "Invalid status bar index of %d.", bar);
        return;
    }

    grid = allocated_bars[bar].widget_row_grid;
    column = allocated_bars[bar].widget_row_column;

    gtk_widget_set_halign(widget, GTK_ALIGN_END);
    gtk_widget_set_hexpand(widget, TRUE);
    gtk_grid_attach(GTK_GRID(grid), widget, column, 0, 1, 1);
    allocated_bars[bar].widget_row_column++;
}



/*****************************************************************************
 *                              Public functions                             *
 ****************************************************************************/

/** \brief Initialize the status bar subsystem.
 *
 *  \warning This function _must_ be called before any call to
 *           ui_statusbar_create() and _must not_ be called after any
 *           call to it.
 */
void ui_statusbar_init(void)
{
    int i;
    ui_sb_state_t *sb_state;

    /* Most things need initialisation to zero and allocated_bars is
     * static, so not much to do here. */
    for (i = 0; i < MAX_STATUS_BARS; ++i) {
        GtkWidget *grid;


        grid = gtk_grid_new();
        gtk_widget_set_valign(grid, GTK_ALIGN_START);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
        gtk_grid_set_row_spacing(GTK_GRID(grid), 0);
        allocated_bars[i].led_row_grid = grid;
        allocated_bars[i].led_row_column = 0;

        grid = gtk_grid_new();
        gtk_widget_set_valign(grid, GTK_ALIGN_START);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
        gtk_grid_set_row_spacing(GTK_GRID(grid), 0);
        allocated_bars[i].widget_row_grid = grid;
        allocated_bars[i].widget_row_column = 0;

        allocated_bars[i].displayed_tape_counter[0] = -1;
        allocated_bars[i].displayed_tape_counter[1] = -1;
    }

    sb_state = lock_sb_state();
    /* Set an impossible number of joyports to enabled so that the status
     * is guarenteed to be updated. */
    sb_state->active_joyports = ~0;
    unlock_sb_state();
}


/** \brief Clean up any resources the statusbar system uses that
 *         weren't cleaned up when the status bars themselves were
 *         destroyed. */
void ui_statusbar_shutdown(void)
{
    mainlock_assert_is_not_vice_thread();
}


/** \brief  Create a new status bar.
 *
 *  This function should be called once as part of creating a new
 *  machine window.
 *
 *  \param[in]  window_identity window identity
 *
 *  \return A new status bar, as a floating reference, or NULL if all
 *          possible status bars have been allocated already.
 */
GtkWidget *ui_statusbar_create(int window_identity)
{
    GtkWidget *sb;

    /* LEDs */
    GtkWidget *warp_led;
    GtkWidget *pause_led;
    GtkWidget *shiftlock_led = NULL;
    GtkWidget *mode4080_led  = NULL;
    GtkWidget *capslock_led  = NULL;
    GtkWidget *diagpin_led   = NULL;
    GtkWidget *turbo_led     = NULL;
    GtkWidget *jiffy_led     = NULL;

    /* top row widgets/wrappers */
    GtkWidget *speed;
    GtkWidget *checkboxes;
    GtkWidget *tape_and_joy;
    GtkWidget *drive_units;
    GtkWidget *volume;

    GtkWidget *tape_wrapper;
    GtkWidget *tape_status;
    GtkWidget *tape_menu;
    GtkWidget *tape_events;
    GtkWidget *joysticks;
    GtkWidget *sep;
    GtkWidget *crt;
    GtkWidget *mixer;
    GtkWidget *message;
    GtkWidget *recording;
    GtkWidget *kbd_debug_widget;
    int i;
    int j;

    mainlock_assert_is_not_vice_thread();

    for (i = 0; i < MAX_STATUS_BARS; ++i) {
        if (allocated_bars[i].bar == NULL) {
            break;
        }
    }
    if (i == MAX_STATUS_BARS) {
        /* Fatal error (should never happen) */
        log_error(LOG_DEFAULT,
                  "Maxium number of status bars (%d) exceeded.",
                  MAX_STATUS_BARS);
        archdep_vice_exit(1);
    }

    allocated_bars[i].window_identity = window_identity;

    sb = vice_gtk3_grid_new_spaced(8, 0);
    gtk_widget_set_hexpand(sb, FALSE);
    g_signal_connect(sb, "destroy",
                     G_CALLBACK(destroy_statusbar_cb), GINT_TO_POINTER(i));
    allocated_bars[i].bar = sb;

    /* First row: LEDs */
    gtk_grid_attach(GTK_GRID(sb),
                    allocated_bars[i].led_row_grid,
                    SB_COL_LEDS, SB_ROW_LEDS, SB_COLUMN_COUNT, 1);

    /* Second row: horizontal separator */
#if 0
    sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach(GTK_GRID(sb),
                    sep,
                    SB_COL_LEDS, SB_ROW_LEDS_HSEP, SB_COLUMN_COUNT, 1);
#endif
    /* Third row: variable amount of widgets */
    gtk_grid_attach(GTK_GRID(sb),
                    allocated_bars[i].widget_row_grid,
                    SB_COL_WIDGETS, SB_ROW_WIDGETS, SB_COLUMN_COUNT, 1);

    /* Fourth row: horizontal separator */
#if 0
    sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach(GTK_GRID(sb),
                    sep,
                    SB_COL_WIDGETS, SB_ROW_WIDGETS_HSEP, SB_COLUMN_COUNT, 1);
#endif
    /* Fifth row: messages, separator and recording */
    message = gtk_label_new(NULL);
    gtk_widget_set_hexpand(message, TRUE);
    gtk_widget_set_halign(message, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(message), PANGO_ELLIPSIZE_END);
    gtk_widget_set_margin_start(message, 8);
    gtk_widget_set_margin_end(message, 8);
    allocated_bars[i].msg = message;
    gtk_grid_attach(GTK_GRID(sb),
                    message,
                    SB_COL_MESSAGES, SB_ROW_MESSAGES, 1, 1);
    /* add vertical separator */
    sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_grid_attach(GTK_GRID(sb),
                    sep,
                    SB_COL_MESSAGES_VSEP, SB_ROW_MESSAGES, 1, 1);
    /* Recording */
    recording = statusbar_recording_widget_create();
    gtk_widget_set_hexpand(recording, TRUE);
    allocated_bars[i].record = recording;
    gtk_grid_attach(GTK_GRID(sb),
                    recording,
                    SB_COL_RECORDING, SB_ROW_MESSAGES, 1, 1);

    /*
     * Add LEds
     */

    /* Warp mode */
    warp_led = warp_led_create();
    /* add a little margin */
    gtk_widget_set_margin_start(warp_led, 8);
    allocated_bars[i].warp_led = warp_led;
    statusbar_append_led(i, warp_led, FALSE);

    /* Pause */
    pause_led = pause_led_create();
    allocated_bars[i].pause_led = pause_led;
    statusbar_append_led(i, pause_led, FALSE);  /* no separator, for now */

    if (machine_class != VICE_MACHINE_VSID) {
        /* shiftlock */
        shiftlock_led = shiftlock_led_create();
        statusbar_append_led(i, shiftlock_led, FALSE);  /* no separator, for now */
    }
    allocated_bars[i].shiftlock_led = shiftlock_led;

    if (machine_class == VICE_MACHINE_C128) {
        /* 40/80 */
        mode4080_led = mode4080_led_create();
        statusbar_append_led(i, mode4080_led, FALSE);  /* no separator, for now */
        /* capslock */
        capslock_led = capslock_led_create();
        statusbar_append_led(i, capslock_led, FALSE);  /* no separator, for now */
    }
    allocated_bars[i].mode4080_led = mode4080_led;
    allocated_bars[i].capslock_led = capslock_led;

    if (machine_class == VICE_MACHINE_PET) {
        /* userport diagnostic pin */
        diagpin_led = diagnosticpin_led_create();
        statusbar_append_led(i, diagpin_led, FALSE);
    }
    allocated_bars[i].diagnosticpin_led = diagpin_led;

    if (machine_class == VICE_MACHINE_SCPU64) {
        /* SuperCPU SPEED switch (turbo/normal) */
        turbo_led = supercpu_turbo_led_create();
        statusbar_append_led(i, turbo_led, FALSE);
        /* SuperCPU JiffyDOS switch */
        jiffy_led = supercpu_jiffy_led_create();
        statusbar_append_led(i, jiffy_led, FALSE);
    }
    allocated_bars[i].supercpu_turbo_led = turbo_led;
    allocated_bars[i].supercpu_jiffy_led = jiffy_led;

    /*
     * Add widgets to the widgets row
     */
    speed = NULL;
    checkboxes = NULL;
    tape_and_joy = NULL;
    drive_units = NULL;
    volume = NULL;

    /* CPU/FPS - No FPS on VDC Window for now */
    speed = statusbar_speed_widget_create(&allocated_bars[i].speed_state);
    gtk_widget_set_margin_start(speed, 8);
    allocated_bars[i].speed = speed;

    /* CRT and Mixer controls */
    if (machine_class != VICE_MACHINE_VSID) {
        crt = gtk_check_button_new_with_label("CRT");
        gtk_widget_set_can_focus(crt, FALSE);
        gtk_widget_set_halign(crt, GTK_ALIGN_START);
        gtk_widget_set_valign(crt, GTK_ALIGN_START);
        gtk_widget_set_hexpand(crt, FALSE);
        gtk_widget_set_vexpand(crt, FALSE);
        gtk_widget_show_all(crt);
        g_signal_connect(crt, "toggled", G_CALLBACK(on_crt_toggled), NULL);

        mixer = gtk_check_button_new_with_label("Mixer");
        gtk_widget_set_can_focus(mixer, FALSE);
        gtk_widget_set_halign(mixer, GTK_ALIGN_START);
        gtk_widget_set_valign(mixer, GTK_ALIGN_START);
        gtk_widget_set_hexpand(mixer, FALSE);
        gtk_widget_set_vexpand(mixer, FALSE);
        gtk_widget_show_all(mixer);
        g_signal_connect(mixer, "toggled", G_CALLBACK(on_mixer_toggled), NULL);
    } else {
        crt = NULL;
        mixer = NULL;
    }
    allocated_bars[i].crt = crt;
    allocated_bars[i].mixer = mixer;

    /* Mixer/CRT checkboxes (all expect VSID) */
    if (machine_class != VICE_MACHINE_VSID) {
        /* wrap checkboxes in a grid to avoid extra vertical spacing */
        checkboxes = gtk_grid_new();

        gtk_grid_attach(GTK_GRID(checkboxes), crt, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(checkboxes), mixer, 0, 1, 1, 1);

    }

    /* Tape widget(s) and joysticks (neither for VSID) */
    tape_wrapper = NULL;
    joysticks = NULL;
    /* No datasette for DTV, SCPU or VSID */
    if ((machine_class != VICE_MACHINE_C64DTV) &&
            (machine_class != VICE_MACHINE_SCPU64) &&
            (machine_class != VICE_MACHINE_VSID)) {

        int ports = machine_class == VICE_MACHINE_PET ? 2 : 1;

        /* wrap tape widget(s) in a grid */
        tape_wrapper = gtk_grid_new();

        /* add widgets and event boxes */
        for (j = 0; j < ports; j++) {
            int port_number = j + TAPEPORT_UNIT_1;

            tape_status = ui_tape_widget_create(port_number, i);
            tape_menu = ui_create_datasette_control_menu(port_number);

            /* Clicking the tape status is supposed to pop up a window. This
             * requires a way to make sure events are captured by random
             * internal widgets; the GtkEventBox manages that task for us. */
            tape_events = gtk_event_box_new();
            gtk_event_box_set_visible_window(GTK_EVENT_BOX(tape_events), FALSE);
            gtk_container_add(GTK_CONTAINER(tape_events), tape_status);
            gtk_grid_attach(GTK_GRID(tape_wrapper), tape_events, 0, j, 1, 1);

            allocated_bars[i].tape_status[j] = tape_status;
            allocated_bars[i].tape_menu[j] = tape_menu;

            g_signal_connect(tape_events, "button-press-event",
                    G_CALLBACK(ui_do_datasette_popup),
                    GINT_TO_POINTER(i | (port_number << 8)));
            g_signal_connect(tape_events, "enter-notify-event",
                    G_CALLBACK(ui_statusbar_cross_cb), &allocated_bars[i]);
            g_signal_connect(tape_events, "leave-notify-event",
                    G_CALLBACK(ui_statusbar_cross_cb), &allocated_bars[i]);
        }
    }

    /* Joystick widgets: row below tape widget(s) */
    if (machine_class != VICE_MACHINE_VSID) {
        joysticks = ui_joystick_widget_create();
        gtk_widget_set_halign(joysticks, GTK_ALIGN_START);
        allocated_bars[i].joysticks = joysticks;
    }

    /* Append tape widgets and/or joysticks widget to top row */
    if (tape_wrapper != NULL || joysticks != NULL) {
        tape_and_joy = gtk_grid_new();
        int row = 0;

        if (tape_wrapper != NULL) {
            gtk_grid_attach(GTK_GRID(tape_and_joy), tape_wrapper, 0, row++, 1, 1);
        }
        if (joysticks != NULL) {
            gtk_grid_attach(GTK_GRID(tape_and_joy), joysticks, 0, row, 1, 1);
        }
    }


    /* Drive widgets (all expect VSID) */
    if (machine_class != VICE_MACHINE_VSID) {
        drive_units = gtk_grid_new();
        /* results in dual drives of a unit being grouped closer together than the
         * units: */
        gtk_grid_set_row_spacing(GTK_GRID(drive_units), 4);
        gtk_widget_set_hexpand(drive_units, FALSE);
        gtk_widget_set_vexpand(drive_units, FALSE);
        gtk_widget_set_halign(drive_units, GTK_ALIGN_START);
        gtk_widget_set_valign(drive_units, GTK_ALIGN_START);

        for (j = 0; j < NUM_DISK_UNITS; ++j) {
            GtkWidget *drive_unit;
            GtkWidget *drive_menu;
            int drive_num;

            drive_unit = ui_drive_widget_create(j, i);
            gtk_widget_set_hexpand(drive_unit, FALSE);
            allocated_bars[i].drive_unit[j] = drive_unit;
            for (drive_num = 0; drive_num < DRIVE_UNIT_DRIVE_MAX; drive_num++) {
                drive_menu = ui_drive_menu_create(j + DRIVE_UNIT_MIN, drive_num);
                allocated_bars[i].drive_menu[j][drive_num] = drive_menu;
            }
            gtk_grid_attach(GTK_GRID(drive_units), drive_unit, unit_cols[j], unit_rows[j], 1, 1);
        }
    }

    /*
     * Add volume control widget
     *
     * FIXME: The widget doesn't show on MacOS/Windows due to the rendering
     *        canvas somehow having z-index priority over the widget. This
     *        works fine on Linux (as far as we know).
     */
#if (!defined(WINDOWS_COMPILE)) && (!defined(MACOS_COMPILE))
    volume = ui_volume_button_create();
    gtk_widget_set_hexpand(volume, TRUE);
#else
    /* Windows or MacOS, only create the volume button for VSID */
    if (machine_class == VICE_MACHINE_VSID) {
        volume = ui_volume_button_create();
        gtk_widget_set_hexpand(volume, TRUE);
    }
#endif
    allocated_bars[i].volume = volume;

    /*
     * Add all valid widgets to the top row
     */
    if (speed != NULL) {
        statusbar_append_widget(i, speed, FALSE);
    }
    if (checkboxes != NULL) {
        statusbar_append_widget(i, checkboxes, TRUE);
    }
    if (tape_and_joy != NULL) {
        statusbar_append_widget(i, tape_and_joy, TRUE);
    }
    if (drive_units != NULL) {
        statusbar_append_widget(i, drive_units, TRUE);
    }
    if (volume != NULL) {
        statusbar_append_widget_end(i, volume);
    }

    /*
     * Add keyboard debugging widget (all except VSID)
     */
    if (machine_class != VICE_MACHINE_VSID) {
        sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_grid_attach(GTK_GRID(sb),
                        sep,
                        SB_COL_MESSAGES, SB_ROW_MESSAGES_HSEP, SB_COLUMN_COUNT, 1);

        kbd_debug_widget = kbd_debug_widget_create();
        allocated_bars[i].kbd_debug = kbd_debug_widget;
        gtk_grid_attach(GTK_GRID(sb),
                        kbd_debug_widget,
                        SB_COL_KBD_DEBUG, SB_ROW_KBD_DEBUG, SB_COLUMN_COUNT, 1);
    }

    return sb;
}


/** \brief Statusbar API function to register an elapsed time.
 *
 *  \param current The current time value in seconds
 *  \param total   The maximum time value in seconds
 *
 */
void ui_display_event_time(unsigned int current, unsigned int total)
{
    GtkWidget *widget;

    /* Ok to call from VICE thread */
    widget = allocated_bars[0].record;
    statusbar_recording_widget_set_time(widget, current, total);
}


/** \brief Statusbar API function to display playback status.
 *
 *  \param playback_status  Unknown.
 *  \param version          seems to be the VICE version major.minor during
 *                          playback and `NULL` when playback is done.
 *
 *  \todo This function is not implemented and its API is not
 *        understood.
 *
 * \note    Since the statusbar message display widget has been removed, we
 *          have some space to implement a widget to display information
 *          regarding playback/recording.
 */
void ui_display_playback(int playback_status, char *version)
{
    GtkWidget *widget = allocated_bars[0].record;

    /* Ok to call from VICE thread */
    statusbar_recording_widget_set_event_playback(widget, version);
}

/** \brief  Statusbar API function to display recording status.
 *
 *  \param  recording_status    seems to be bool indicating recording active
 *
 *  \todo   This function is not implemented and its API is not
 *          understood.
 *
 * \note    Since the statusbar message display widget has been removed, we
 *          have some space to implement a widget to display information
 *          regarding playback/recording.
 */
void ui_display_recording(int recording_status)
{
    GtkWidget *widget;
    DBG(("ui_display_recording: %d", recording_status));
    /* Ok to call from VICE thread */
    widget = allocated_bars[0].record;
    statusbar_recording_widget_set_recording_status(widget, recording_status);
}


/** \brief  Statusbar API function to display a message in the status bar.
 *
 *  \param  text    The text to display.
 *  \param  fadeout Erase the text after five* seconds unless it has already
 *                  been replaced.
 *
 * \note    Safe to call from VICE thread.
 * \see     #MESSAGE_TIMEOUT for the actual timeout in seconds
 */
void ui_display_statustext(const char *text, bool fadeout)
{
    ui_sb_state_t *sb_state = lock_sb_state();

    strncpy(sb_state->message_text, text, MESSAGE_TEXT_SIZE);
    /* strncpy() doesn't add a 0 when len(text) == buflen: */
    sb_state->message_text[MESSAGE_TEXT_SIZE - 1] = '\0';
    sb_state->message_fadeout = fadeout;
    sb_state->message_pending = true;

    unlock_sb_state();
}

/** \brief  Statusbar API function to display current volume
 *
 * This function is a NOP since the volume can be checked and altered via the
 * Mixer Controls via the statusbar.
 *
 * \param[in]   vol     new volume level
 */
void ui_display_volume(int vol)
{
    /* NOP */
}


/** \brief  Statusbar API function to display current joyport inputs.
 *  \param  joyport An array of bytes of size at least
 *                  JOYPORT_MAX_PORTS+1, with data regarding each
 *                  active joyport.
 *  \warning The joyport array is, for all practical purposes,
 *           _1-indexed_. joyport[0] is unused.
 *  \sa ui_sb_state_s::current_joyports Describes the format of the
 *      data encoded in the joyport array. Note that current_joyports
 *      is 0-indexed as is typical for C arrays.
 */

/* FIXME: during the joystick data extension the joyport type has become uint16_t,
   I did not change anything except the parameter of the function
 */
void ui_display_joyport(uint16_t *joyport)
{
    int i;
    ui_sb_state_t *sb_state;

    sb_state = lock_sb_state();

    for (i = 0; i < JOYPORT_MAX_PORTS; ++i) {
        /* Compare the new value to the current one, set the new
         * value, and queue a redraw if and only if there was a
         * change. And yes, the input joystick ports are 1-indexed. I
         * don't know either. */
        if (sb_state->current_joyports[i] != joyport[i+1]) {
            int j;
            sb_state->current_joyports[i] = joyport[i+1];
            for (j = 0; j < MAX_STATUS_BARS; ++j) {
                if (allocated_bars[j].joysticks) {
                    GtkWidget *grid;
                    GtkWidget *widget;

                    grid = gtk_bin_get_child(GTK_BIN(allocated_bars[j].joysticks));
                    widget = gtk_grid_get_child_at(GTK_GRID(grid), i + 1, 0);
                    if (widget) {
                        redraw_widget_on_ui_thread(widget);
                    }
                }
            }
        }
    }

    unlock_sb_state();
}


/** \brief  Get tape motor widget by tape port index
 *
 * \param[in]   bar     status bar index
 * \param[in]   port    tape port index
 *
 * \return  tape motor widget or NULL if not present
 */
static GtkWidget *tape_get_motor_widget(int bar, int port)
{
    GtkWidget *status;
    GtkWidget *motor = NULL;

    status = allocated_bars[bar].tape_status[port];
    if (status != NULL) {
        motor = gtk_grid_get_child_at(GTK_GRID(status), TAPE_STATUS_COL_MOTOR, 0);
    }
    return motor;
}


/** \brief  Statusbar API function to report changes in tape control status.
 *
 * \param[in]   port    tape port index (0 or 1)
 * \param[in]   control The new tape control. See the DATASETTE_CONTROL_*
 *                      constants in datasette.h for legal values of this
 *                      parameter.
 */
void ui_display_tape_control_status(int port, int control)
{
    ui_sb_state_t *sb_state;

    /* Ok to call from VICE thread */

    sb_state = lock_sb_state();

    if (control != sb_state->tape_control[port]) {
        int i;
        sb_state->tape_control[port] = control;

        for (i = 0; i < MAX_STATUS_BARS; ++i) {
            GtkWidget *motor = tape_get_motor_widget(i, port);
            if (motor != NULL) {
                redraw_widget_on_ui_thread(motor);
            }
        }
    }

    unlock_sb_state();
}

/** \brief  Statusbar API function to report changes in tape position.
 *
 * \param[in]   port    tape port index (0 or 1)
 * \param[in]   counter the new value of the position counter
 *
 *  \note   Only the last three digits of the counter will be displayed.
 */
void ui_display_tape_counter(int port, int counter)
{
    ui_sb_state_t *sb_state;
    int index = port;

    sb_state = lock_sb_state();
    sb_state->tape_counter[index] = counter;
    unlock_sb_state();
}


/** \brief  Statusbar API function to report changes in the tape motor.
 *
 * \param[in]   port    tape port index (0 or 1)
 * \param[in]   motor   Nonzero if the tape motor is now on.
 */
void ui_display_tape_motor_status(int port, int motor)
{
    ui_sb_state_t *sb_state;

    /* Ok to call from VICE thread */
    sb_state = lock_sb_state();

    if (motor != sb_state->tape_motor_status[port]) {
        int i;
        sb_state->tape_motor_status[port] = motor;

        for (i = 0; i < MAX_STATUS_BARS; ++i) {
            GtkWidget *widget = tape_get_motor_widget(i, port);

            if (widget != NULL) {
                redraw_widget_on_ui_thread(widget);
            }
        }
    }

    unlock_sb_state();
}


/** \brief  Statusbar API function to report changes in tape status.
 *  \param  tape_status The new tape status.
 *  \note   This function does nothing and its API is not
 *          understood. Furthermore, no other extant UIs appear to react
 *          to this call.
 */
void ui_set_tape_status(int port, int tape_status)
{
    /* Ok to call from VICE thread */
}


/** \brief  Statusbar API function to report mounting or unmounting of a tape
 *          image.
 *
 *  \param  image   The filename of the tape image (if mounted), or the
 *                  empty string or NULL (if unmounting).
 */
void ui_display_tape_current_image(int port, const char *image)
{
}


/** \brief  Statusbar API function to report changes in drive LED intensity.
 *
 * This function simply updates global state, rendering occurs in ui_update_statusbars().
 *
 *  \param  drive_number    The unit to update (0-3 for drives 8-11)
 *  \param  drive_base      Drive 0 or 1 of dualdrives
 *  \param  led_pwm1        The intensity of the first LED (0=off,
 *                          1000=maximum intensity)
 *  \param  led_pwm2        The intensity of the second LED (0=off,
 *                          1000=maximum intensity)
 *  \todo   Dual drive code doesn't have its separate Error LED.
 *          Its setting is mixed in in the led_status of drive 0.
 *          Also those error LEDs don't flash but are just on
 *          (or on some models, change the green/red LED to red).
 */
void ui_display_drive_led(unsigned int drive_number,
                          unsigned int drive_base,
                          unsigned int led_pwm1,
                          unsigned int led_pwm2)
{
    ui_sb_state_t *sb_state;

    /* Ok to call from VICE thread */

    if (drive_number > NUM_DISK_UNITS - 1) {
        /* TODO: Fatal error? */
        debug_gtk3("Error: illegal drive number %u.", drive_number);
        abort();
    }

    sb_state = lock_sb_state();
    sb_state->current_drive_leds[drive_number][drive_base][0] = led_pwm1;
    sb_state->current_drive_leds[drive_number][drive_base][1] = led_pwm2;
    sb_state->current_drive_leds_updated[drive_number][drive_base][0] = true;
    sb_state->current_drive_leds_updated[drive_number][drive_base][1] = true;
    unlock_sb_state();
}


/** \brief  Statusbar API function to report changes in drive head location.
 *
 * This function simply updates global state, rendering occurs in ui_update_statusbars().
 *
 *  \param  drive_number        The unit to update (0-3 for drives 8-11)
 *  \param  drive_base          Drive 0 or 1 of dualdrives
 *  \param  half_track_number   Twice the value of the head
 *                              location. 18.0 is 36, while 18.5 would be 37.
 *  \param  drive_side          drive side for dual-head drives (0 or 1)
 *
 *  \todo   The statusbar API does not yet support dual-unit disk
 *          drives. The drive_base argument will likely come into play
 *          once it does.
 */
void ui_display_drive_track(unsigned int drive_number,
                            unsigned int drive_base,
                            unsigned int half_track_number,
                            unsigned int drive_side)
{
    ui_sb_state_t *sb_state;
    int doubleside, dualdrive;

    /* Ok to call from VICE thread */

    if (drive_number > NUM_DISK_UNITS - 1) {
        /* TODO: Fatal error? */
        return;
    }

    sb_state = lock_sb_state();
    doubleside = drive_get_num_heads(sb_state->drives_type[drive_number]) == 2 ? 1 : 0;
    dualdrive = drive_check_dual(sb_state->drives_type[drive_number]);

    if (dualdrive) {
        snprintf(
            sb_state->current_drive_unit_str[drive_number][drive_base],
            DRIVE_UNIT_STR_MAX_LEN - 1,
            "%u:%u",
            drive_number + 8,
            drive_base);
    } else {
        snprintf(
            sb_state->current_drive_unit_str[drive_number][drive_base],
            DRIVE_UNIT_STR_MAX_LEN - 1,
            "%u",
            drive_number + 8);
    }
    sb_state->current_drive_unit_str[drive_number][drive_base][DRIVE_UNIT_STR_MAX_LEN - 1] = '\0';
    sb_state->current_drive_unit_str_updated[drive_number][drive_base] = true;

    if (doubleside) {
        snprintf(
            sb_state->current_drive_track_str[drive_number][drive_base],
            DRIVE_TRACK_STR_MAX_LEN - 1,
            " %u:%04.1lf",  /* space instead of 0 padding looks weird with the
                               drive side in front */
            drive_side,
            half_track_number / 2.0);
    } else {
        snprintf(
            sb_state->current_drive_track_str[drive_number][drive_base],
            DRIVE_TRACK_STR_MAX_LEN - 1,
            " %4.1lf",
            half_track_number / 2.0);
    }

    sb_state->current_drive_track_str[drive_number][drive_base][DRIVE_TRACK_STR_MAX_LEN - 1] = '\0';
    sb_state->current_drive_track_str_updated[drive_number][drive_base] = true;

    unlock_sb_state();
}


/** \brief Update information about each drive.
 *
 *  \param state           A bitmask int, where bits 0-3 indicate
 *                         whether or not drives 8-11 respectively are
 *                         being emulated carefully enough to provide
 *                         LED information.
 *  \param drive_led_color An array of size at least NUM_DISK_UNITS that
 *                         provides information about the LEDs on this
 *                         drive. An element of this array will only
 *                         be checked if the corresponding bit in
 *                         state is 1.
 *  \note Before calling this function, the drive configuration
 *        resources (Drive8Type, Drive9Type, etc) should all be set to
 *        the values you wish to display.
 *  \warning If a drive's LEDs are active when its LED values change,
 *           the UI will not reflect the LED type change until the
 *           next time the led's values are updated. This should not
 *           happen under normal circumstances.
 *  \sa compute_drives_enabled_mask() for how this function determines
 *      which drives are truly active
 *  \sa ui_sb_state_s::drive_led_types for the data in each element of
 *      drive_led_color
 */
void ui_enable_drive_status(ui_drive_enable_t state, int *drive_led_color)
{
    int unit, enabled, drive, i;
    ui_sb_state_t *sb_state;

    /* Ok to call from VICE thread */

    sb_state = lock_sb_state();

    /* Update the drive LEDs first, unconditionally. */
    enabled = state;
    for (unit = 0; unit < NUM_DISK_UNITS; ++unit) {
        for (drive = 0; drive < 2; drive++) {
            if (enabled & 1) {
                for (i = 0; i < DRIVE_LEDS_MAX; i++) {
                    sb_state->drive_led_types[unit][drive][i] = (drive_led_color[unit] >> i) & 1;
                    sb_state->current_drive_leds[unit][drive][i] = 0;
                }
            }
        }
        enabled >>= 1;
    }

    /* Determine drive types and determine dual-drive changes */
    sb_state->drives_dual = 0;
    for (unit = 0; unit < NUM_DISK_UNITS; unit++) {
        int curtype;
        bool old_dual;
        bool new_dual;

        if (resources_get_int_sprintf("Drive%dType", &curtype, unit + DRIVE_UNIT_MIN) < 0) {
            curtype = 0;
        }
        old_dual = (bool)drive_check_dual(sb_state->drives_type[unit]);
        new_dual = (bool)drive_check_dual(curtype);
        if (old_dual != new_dual) {
            sb_state->drives_dual |= 1 << unit;
        }

        /* update drive type */
        sb_state->drives_type[unit] = curtype;
    }

    /* Now give enabled its "real" value based on the drive
     * definitions. */
    enabled = compute_drives_enabled_mask();

    /* Now, if necessary, update the status bar layouts. We won't need
     * to do this if the only change was the kind of drives hooked up,
     * instead of the number */
    if ((state != sb_state->drives_tde_enabled)
            || (enabled != sb_state->drives_enabled)
            || (sb_state->drives_dual != 0)) {
        sb_state->drives_enabled = enabled;
        sb_state->drives_tde_enabled = state;
        sb_state->drives_layout_needed = true;
    }

    unlock_sb_state();
}

/** \brief  Statusbar API function to report mounting or unmounting of
 *          a disk image.
 *
 *  \param  unit_number     0-3 to represent disk units at device 8-11.
 *  \param  drive_number    0-1 to represent the drives in a unit
 *  \param  image           The filename of the disk image (if mounted),
 *                          or the empty string or NULL (if unmounting).
 *  \todo This API is insufficient to describe drives with two disk units.
 */
void ui_display_drive_current_image(unsigned int unit_number, unsigned int drive_number, const char *image)
{
}


/** \brief  Determine if the CRT controls widget is enabled in \a window
 *
 * \param[in]   window  GtkWindow instance
 *
 * \return  bool
 */
gboolean ui_statusbar_crt_controls_enabled(GtkWidget *window)
{
    int bar;

    mainlock_assert_is_not_vice_thread();

    bar = ui_statusbar_index_for_window(window);
    if (bar >= 0) {
        GtkWidget *crt = allocated_bars[bar].crt;

        if (crt != NULL) {
            return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(crt));
        }
    }
    return FALSE;
}


/** \brief  Determine if the mixer controls widget is enabled in \a window
 *
 * \param[in]   window  GtkWindow instance
 *
 * \return  bool
 */
gboolean ui_statusbar_mixer_controls_enabled(GtkWidget *window)
{
    int bar;

    mainlock_assert_is_not_vice_thread();

    bar = ui_statusbar_index_for_window(window);
    if (bar >= 0) {
        GtkWidget *mixer = allocated_bars[bar].mixer;

        if (mixer != NULL) {
            return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mixer));
        }
    }
    return FALSE;
}


/** \brief  Update status bar message widget
 *
 * \param[in]   state   statusbar state, obtained through lock_sb_state()
 */
static void statusbar_update_message(ui_sb_state_t *sb_state)
{
    if (sb_state->message_pending) {
        /* Only the primary window gets statusbar messages (?) */
        GtkWidget *msg = allocated_bars[0].msg;

       /* remove any previous fadout handler */
        if (timeout_id > 0) {
            g_source_remove(timeout_id);;
            timeout_id = 0;
        }

        /* set new text */
        /* debug_gtk3("New status text: '%s'.", sb_state->message_text); */
        gtk_label_set_text(GTK_LABEL(msg), sb_state->message_text);

        /* do we need a timeout? */
        if (sb_state->message_fadeout) {
            timeout_id = g_timeout_add_seconds(
                    MESSAGE_TIMEOUT,
                    message_timeout_handler,
                    msg);
        }
        /* we're done */
        sb_state->message_pending = false;
    }
}



/** \brief  Update VSID-specific status bar
 */
void ui_update_vsid_statusbar(void)
{
    GtkWidget *speed_widget;
    ui_statusbar_t *bar = &allocated_bars[0];
    ui_sb_state_t *sb_state = lock_sb_state();

    /* cpu/fps */
    speed_widget = bar->speed;
    if (speed_widget != NULL) {
        statusbar_speed_widget_update(speed_widget, &bar->speed_state, bar->window_identity);
    }

    /* messages */
    statusbar_update_message(sb_state);

    unlock_sb_state();
}


/** \brief  Update status bars for non-VSID machines
 */
void ui_update_statusbars(void)
{
    /* TODO: Don't call this for each top level window as it updates all statusbars */
    ui_statusbar_t *bar;
    GtkWidget *speed_widget;
    int i;
    int j;
    ui_sb_state_t *sb_state;
    ui_sb_state_t state_snapshot;
    uint32_t active_joyports;
    bool active_joyports_changed = false;
    int unit;
    sb_state = lock_sb_state();

    /* Have any joyports been enabled / disabled? */
    active_joyports = build_active_joyport_mask();
    if (active_joyports != sb_state->active_joyports) {
        active_joyports_changed = true;
        sb_state->active_joyports = active_joyports;
    }

    /* Take a safe copy of the sb_state so we don't hold the lock during display */
    state_snapshot = *sb_state;

    /* Reset any 'updated needed' flags */
    sb_state->drives_layout_needed = false;

    for (j = 0; j < NUM_DISK_UNITS; ++j) {
        sb_state->current_drive_track_str_updated[j][0] = false;
        sb_state->current_drive_track_str_updated[j][1] = false;
        sb_state->current_drive_unit_str_updated[j][0]  = false;
        sb_state->current_drive_unit_str_updated[j][1]  = false;
        for (int d = 0; d < 2; d++) {
            sb_state->current_drive_leds_updated[j][d][0] = false;
            sb_state->current_drive_leds_updated[j][d][1] = false;
        }
    }

    /* statusbar messages */
    statusbar_update_message(sb_state);

    /* we can release the lock now */
    unlock_sb_state();

    for (i = 0; i < MAX_STATUS_BARS; ++i) {
        bar = &allocated_bars[i];
        if (bar->bar == NULL) {
            continue;
        }

        /*
         * Emulation speed, fps, warp
         */

        speed_widget = bar->speed;
        if (speed_widget != NULL) {
            statusbar_speed_widget_update(speed_widget, &bar->speed_state, bar->window_identity);
        }

        /*
         * Update Tape
         */
        for (j = 0; j < TAPEPORT_MAX_PORTS; j++) {
            GtkWidget *tape_status = bar->tape_status[j];

            if (tape_status != NULL
                    && bar->displayed_tape_counter[j] != state_snapshot.tape_counter[j]) {
                GtkWidget *tape_counter;
                int count;

                tape_counter = gtk_grid_get_child_at(GTK_GRID(tape_status),
                                                     TAPE_STATUS_COL_COUNTER, 0);
                count = state_snapshot.tape_counter[j];

                if (tape_counter != NULL) {
                    char buffer[32];

                    g_snprintf(buffer, sizeof(buffer), "%03d", count % 1000);
                    gtk_label_set_text(GTK_LABEL(tape_counter), buffer);
                }
                bar->displayed_tape_counter[j] = count;
            }
        }

        /*
         * Joystick
         */

        if (active_joyports_changed) {
            update_joyport_layout();
        }

        /*
         * Drive track, half track, and led
         */

        if (state_snapshot.drives_layout_needed) {
            layout_statusbar_drives(&state_snapshot, i);
        }

        for (unit = DRIVE_UNIT_MIN; unit <= DRIVE_UNIT_MAX; unit++) {

            /* Only update the widgets if their state has changed .. */
            for (int drive = 0; drive < 2; drive++) {

                GtkWidget *number = drive_get_number_widget(i, unit, drive);
                GtkWidget *head = drive_get_head_widget(i, unit, drive);
                GtkWidget *led = drive_get_led_widget(i, unit, drive);

                if (state_snapshot.current_drive_track_str_updated[unit - DRIVE_UNIT_MIN][drive]) {
                    if (head != NULL) {
                        char *s = state_snapshot.current_drive_track_str[unit - DRIVE_UNIT_MIN][drive];
                        gtk_label_set_text(GTK_LABEL(head), s);
                    }
                }

                if (state_snapshot.current_drive_unit_str_updated[unit - DRIVE_UNIT_MIN][drive]) {
                    if (number != NULL) {
                        char *s = state_snapshot.current_drive_unit_str[unit - DRIVE_UNIT_MIN][drive];
                        gtk_label_set_text(GTK_LABEL(number), s);
                    }
                }

                /* Only draw the LEDs if they have changed */
                if (state_snapshot.current_drive_leds_updated[unit - DRIVE_UNIT_MIN][drive][0]) {
                    if (led != NULL) {
                        gtk_widget_queue_draw(led);
                    }
                }

                /* TODO: Another LED for dual drive */
            }
        }
    }
}


/** \brief  Show/hide the statusbar kdb debug widget of \a window
 *
 * \param[in,out]   window  GtkWindow instance
 * \param[in]       state   Display state
 *
 * \todo    Replace integer literals
 */
static void kbd_statusbar_widget_enable(GtkWidget *window, gboolean state)
{
    GtkWidget *main_grid;
    GtkWidget *statusbar;
    GtkWidget *kbd;

    mainlock_assert_is_not_vice_thread();

    main_grid = gtk_bin_get_child(GTK_BIN(window));
    if (main_grid != NULL) {
        statusbar = gtk_grid_get_child_at(GTK_GRID(main_grid), 0, 2);
        if (statusbar != NULL) {
            kbd = gtk_grid_get_child_at(GTK_GRID(statusbar), 0, SB_ROW_KBD_DEBUG);
            if (kbd != NULL) {
                if (state) {
                    gtk_widget_show_all(kbd);
                } else {
                    gtk_widget_hide(kbd);
                }
            }
        }
    }
}


/** \brief  Show/hide the keyboard debugging widget on the status bar
 *
 * Show/hide the keyboard debugging widget on the status bar of a single window.
 *
 * \param[in]   window  main window instance
 * \param[in]   state   visible state
 */
void ui_statusbar_set_kbd_debug_for_window(GtkWidget *window, gboolean state)
{
    mainlock_assert_is_not_vice_thread();

    kbd_statusbar_widget_enable(window, state);
}


/** \brief  Show/hide the keyboard debugging widget on the status bar
 *
 * Show/hide the keyboard debugging widget on the status bar (of both windows
 * in the case of x128).
 *
 * \param[in]   state   visible state
 *
 * \note    Only call from a finalized UI.
 */
void ui_statusbar_set_kbd_debug(gboolean state)
{
    GtkWidget *window;

    mainlock_assert_is_not_vice_thread();

    /* standard VIC/VICII/TED/CRTC window */
    window = ui_get_window_by_index(0);
    kbd_statusbar_widget_enable(window, state);

    /* C128: Handle the VDC */
    if (machine_class == VICE_MACHINE_C128) {
        window = ui_get_window_by_index(1); /* VDC */
        kbd_statusbar_widget_enable(window, state);
    }
}


/** \brief  Get active 'Recording' widget
 *
 * \return  recording widget
 */
GtkWidget *ui_statusbar_get_recording_widget(void)
{
    int w = ui_get_main_window_index();
    return allocated_bars[w].record;
}


/** \brief  Show reset on statusbar
 *
 * A device was reset, so we show it on the statusbar
 *
 * \param[in]   device  device number
 * \param[in]   mode    reset mode (soft(0) or hard(1) ,only for device 0)
 */
void ui_display_reset(int device, int mode)
{
}


/** \brief  Update status bar debug widget instances
 *
 * \param[in]   report  Gdk key event
 *
 * \note    Can only be called from the UI thread.
 */
void ui_statusbar_update_kbd_debug(GdkEvent *report)
{
    GtkWidget *widget;

    mainlock_assert_is_not_vice_thread();

    if (machine_class == VICE_MACHINE_VSID) {
        return;
    }

    /* update primary window debug widget */
    widget = allocated_bars[0].kbd_debug;
    if (widget != NULL) {
        kbd_debug_widget_update(widget, report);
    }
    if (machine_class == VICE_MACHINE_C128) {
        /* update secondary window debug widget */
        widget = allocated_bars[1].kbd_debug;
        if (widget != NULL) {
            kbd_debug_widget_update(widget, report);
        }
    }
}


/** \brief  Set visibility of the status bar for a main window
 *
 * \param[in]   window  main window
 * \param[in]   visible visibility of status bar for \a window
 *
 * \return  TRUE on success
 */
gboolean ui_statusbar_set_visible_for_window(GtkWidget *window, gboolean visible)
{
    GtkWidget *main_grid;
    GtkWidget *statusbar;

    main_grid = gtk_bin_get_child(GTK_BIN(window));
    if (main_grid == NULL) {
        return FALSE;
    }
    statusbar = gtk_grid_get_child_at(GTK_GRID(main_grid), 0, 2);
    if (statusbar == NULL) {
        return FALSE;
    }

    if (visible) {
        if (!ui_is_fullscreen() || ui_fullscreen_has_decorations()) {
            gtk_widget_show(statusbar);
        }
    } else {
        gtk_widget_hide(statusbar);
    }
    return TRUE;
}


/** \brief  Show/Hide statusbar of the **active** window
 *
 * Toggle visibility of statusbar based on "${chip}ShowStatusbar" resource.
 *
 * \return  TRUE (don't pass event along further)
 */
gboolean ui_action_toggle_show_statusbar(void)
{
    video_canvas_t *canvas = ui_get_active_canvas();
    if (canvas != NULL) {
        GtkWindow *window;
        int show = 0;
        const char *chip_name = canvas->videoconfig->chip_name;

        resources_get_int_sprintf("%sShowStatusbar", &show, chip_name);
        show = !show;
        resources_set_int_sprintf("%sShowStatusbar", show, chip_name);

        window = ui_get_active_window();
        ui_statusbar_set_visible_for_window(GTK_WIDGET(window), show);
        /* update menu item's toggled state! */
        vhk_gtk_set_check_item_blocked_by_action(ACTION_SHOW_STATUSBAR_TOGGLE,
                                                 show);
    }
    return TRUE;
}


/** \brief  Recreate CRT controls on the status bars
 *
 * Destroy the old CRT controls and create new ones. To be called whenever the
 * video standard changes.
 */
void ui_statusbar_recreate_crt_controls(void)
{
    int i;

    mainlock_assert_is_not_vice_thread();

    for (i = PRIMARY_WINDOW; i <= SECONDARY_WINDOW; i++) {
        GtkWidget      *window;
        GtkWidget      *grid;
        GtkWidget      *controls;
        video_canvas_t *canvas;
        const char     *chip;

        window   = ui_get_window_by_index(i);
        if (window == NULL) {
            continue;
        }
        canvas   = ui_get_canvas_for_window(i);
        chip     = canvas->videoconfig->chip_name;
        grid     = gtk_bin_get_child(GTK_BIN(window));
        controls = gtk_grid_get_child_at(GTK_GRID(grid), 0, 3);
        gtk_widget_destroy(controls);

        controls = crt_control_widget_create(NULL, chip, TRUE);
        gtk_grid_attach(GTK_GRID(grid), controls, 0, 3, 1, 1);

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(allocated_bars[i].crt))) {
            gtk_widget_show(controls);
        } else {
            gtk_widget_hide(controls);
        }
    }
}
