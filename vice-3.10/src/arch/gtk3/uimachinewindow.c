/**
 * \file    uimachinewindow.c
 * \brief   Native GTK3 main emulator window code.
 *
 * \author Marcus Sutton <loggedoubt@gmail.com>
 * \author Michael C. Martin <mcmartin@gmail.com>
 */

/* This file is part of VICE, the Versatile Commodore Emulator.
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

/* \note It should be possible to compile, link and run vsid while
 *       this entire file (amongst others) is contained inside an #if
 *       0 wrapper.
 */
#if 1

/* #define DEBUGPOINTER */

#include "vice.h"

#include <gtk/gtk.h>
#if !defined(MACOS_COMPILE) && !defined(WINDOWS_COMPILE)
#include <gdk/gdkx.h>
#endif
#include <string.h>
#ifdef MACOS_COMPILE
#import <CoreGraphics/CGEvent.h>
#elif defined(WINDOWS_COMPILE)
#include <windows.h>
#include <gdk/gdkwin32.h>
#endif

#ifdef WINDOWS_COMPILE
#include "directx_renderer.h"
#include "directx_renderer_impl.h"
#else
#include "opengl_renderer.h"
#endif

#include "lightpen.h"
#include "log.h"
#include "machine.h"
#ifdef MACOS_COMPILE
#include "macOS-util.h"
#endif
#include "mousedrv.h"
#include "resources.h"
#include "ui.h"
#include "uimachinemenu.h"
#include "videoarch.h"
#include "vsyncapi.h"

#include "uimachinewindow.h"


/* FIXME:   someone please add Doxygen docs for this, I can guess what it means
 *          but I'll probably get it wrong. --compyx
 */
#ifdef DEBUGPOINTER
#define VICE_EMPTY_POINTER  NULL
#else
#define VICE_EMPTY_POINTER  canvas->blank_ptr
#endif

log_t machine_window_log = LOG_DEFAULT;

static gboolean event_box_stillness_tick_cb(GtkWidget *widget,
                                            GdkFrameClock *clock,
                                            gpointer user_data);

/** \brief  Ignore the hide-mouse-cursor event handlers
 *
 * Used during dialogs
 */
static gboolean ignore_mouse_hide = FALSE;


/** \brief  Set mouse-hide-ignore state
 *
 * \param[in]   state   enable/disable ignoring the mouse pointer hiding
 */
void ui_set_ignore_mouse_hide(gboolean state)
{
    ignore_mouse_hide = state;
}

/** \brief Whether or not to capture mouse movements and warp back.
 */
static bool enable_capture = false;

/** \brief Host mouse deltas are calculated from this X value.
 *
 * After the delta is calculated, the mouse is warped back to this
 * location.
 */
static int host_delta_origin_x = 0.0;

/** \brief Host mouse deltas are calculated from this Y value.
 *
 * After the delta is calculated, the mouse is warped back to this
 * location.
 */
static int host_delta_origin_y = 0.0;

/** \brief Number of frames the mouse didn't move, used for pointer hiding
 */
static int _mouse_still_frames = 0;

/** \brief Mouse warp for each platform.
 */
static void warp(int x, int y)
{
#ifdef MACOS_COMPILE

    CGWarpMouseCursorPosition(CGPointMake(x, y));
    CGAssociateMouseAndMouseCursorPosition(true);

#elif defined(WINDOWS_COMPILE)

    if (SetCursorPos(x, y) == FALSE) {
        log_error(machine_window_log, "SetCursorPos(%d, %d) - %lu", x, y, GetLastError());
    }

    {
        DWORD dw;
        SetLastError(0);
        mouse_event(MOUSEEVENTF_ABSOLUTE, x, y, 0, 0);
        dw = GetLastError();
        if (dw != 0) {
            log_error(machine_window_log, "mouse_event(%d, %d) - %lu", x, y, dw);
        }
    }

#else /* xlib */

    GtkWidget *gtk_widget = ui_get_window_by_index(PRIMARY_WINDOW);
    GdkWindow *gdk_window = gtk_widget_get_window(gtk_widget);
    Display *display = GDK_DISPLAY_XDISPLAY(gdk_window_get_display(gdk_window));

    XWarpPointer(display, None, GDK_WINDOW_XID(gdk_window), 0, 0, 0, 0, x, y);

#endif
}

static void mouse_host_capture(int warp_x, int warp_y)
{
    enable_capture = true;

    warp(warp_x, warp_y);

    /* future mouse moments will be captured relative from here */
    host_delta_origin_x = warp_x;
    host_delta_origin_y = warp_y;
}

static void mouse_host_uncapture(void)
{
    enable_capture = false;
}

/** \brief Calculate mouse movement delta and warp back to the origin.
 */
static void mouse_host_moved(float x, float y)
{
    float delta_x, delta_y;

    if (!enable_capture) {
        return;
    }

    delta_x = x - host_delta_origin_x;
    delta_y = y - host_delta_origin_y;

    if (delta_x || delta_y) {
        mouse_move(delta_x, delta_y);
        warp(host_delta_origin_x, host_delta_origin_y);
    }
}

/** \brief Callback for handling mouse motion events over the emulated
 *         screen.
 *
 *  Mouse motion events influence three different subsystems: the
 *  light-pen (if any), the emulated mouse (if any), and the UI-level
 *  routines that hide the mouse pointer if it comes to rest over the
 *  machine's screen.
 *
 *  Moving the mouse pointer resets the number of frames the mouse was
 *  held still.
 *
 *  Light pen position information is computed based on the new mouse
 *  position and what part of the machine window is actually in use
 *  based on current scaling and aspect ratio settings.
 *
 *  Mouse information is computed based on the difference between the
 *  current mouse location and the last recorded mouse location. If
 *  the mouse has been captured by the emulator, this also then warps
 *  the mouse pointer back to the middle of the emulated
 *  screen. (These warps will trigger an additional call to this
 *  function, but an additional flag will prevent them from being
 *  processed as true input.)
 *
 *  Information relevant to these processes is cached in the
 *  video_canvas_s structure for use as needed.
 *
 *  \param widget    The widget that sent the event.
 *  \param event     The GdkEventMotion event itself.
 *  \param user_data The video canvas data structure associated with
 *                   this machine window.
 *  \return TRUE if no further event processing is necessary.
 *
 *  \todo Information involving mouse-warping is not cached with the
 *        canvas yet, and should be for cleaner C128 support.
 *
 *  \todo Pointer warping does not work on Wayland. GTK3 and its GDK
 *        substrate simply do not provide an implementation for
 *        gdk_device_warp(), and Wayland's window model doesn't really
 *        support pointer warping the way GDK envisions. Wayland's
 *        window model envisions using pointer constraints to confine
 *        the mouse pointer within a target window, and then using
 *        relative mouse motion events to capture additional attempts
 *        at motion outside of it. SDL2 implements this and it may
 *        provide a useful starting point for this alternative
 *        implementation.
 *
 * \sa event_box_mouse_button_cb Further light pen and mouse button
 *     handling.
 * \sa event_box_scroll_cb Further mouse button handling.
 * \sa event_box_stillness_tick_cb More of the hide-idle-mouse-pointer
 *     logic.
 * \sa event_box_cross_cb More of the hide-idle-mouse-pointer logic.
 */
static gboolean event_box_motion_cb(GtkWidget *widget,
                                    GdkEvent *event, gpointer user_data)
{
    video_canvas_t *canvas = (video_canvas_t *)user_data;

    _mouse_still_frames = 0;

    if (event->type != GDK_MOTION_NOTIFY) {
        return FALSE;
    }

    pthread_mutex_lock(&canvas->lock);

    /* GDK_ENTER_NOTIFY isn't reliable on fullscreen transitions, so we reenable this here too */
    if (canvas->still_frame_callback_id == 0) {
        canvas->still_frame_callback_id =
            gtk_widget_add_tick_callback(
                 canvas->event_box,
                 event_box_stillness_tick_cb,
                 canvas, NULL);
    }

    GdkEventMotion *motion = (GdkEventMotion *)event;

    /* printf("mouse move %f, %f  (%f, %f)\n", motion->x, motion->y, motion->x_root, motion->y_root); fflush(stdout); */

    if (enable_capture) {

        /* mouse movement, translate motion into window coordinates */
        int widget_x, widget_y;
        gtk_widget_translate_coordinates(widget, gtk_widget_get_toplevel(widget), 0, 0, &widget_x, &widget_y);

#ifdef MACOS_COMPILE
        CGRect native_frame, content_rect;

        vice_macos_get_widget_frame_and_content_rect(widget, &native_frame, &content_rect);

        /* macOS CoreGraphics coordinates origin is bottom-left of primary display */
        size_t main_display_height = CGDisplayPixelsHigh(CGMainDisplayID());

        mouse_host_moved(
                                  native_frame.origin.x + widget_x + motion->x,
            main_display_height - native_frame.origin.y - content_rect.size.height + widget_y + motion->y
            );

#elif defined(WINDOWS_COMPILE)

        /* scale is a uniform scaling applied to eg high dpi widgets by gtk */
        int scale = gtk_widget_get_scale_factor(widget);
        POINT pt;
        /* mouse_host_moved(motion->x_root * scale, motion->y_root * scale); */
        if (GetCursorPos(&pt) == FALSE) {
            log_error(machine_window_log, "GetCursorPos failed (%ld, %ld)\n", pt.x, pt.y);
            /*printf("GetCursorPos failed (%ld, %ld)\n", pt.x, pt.y); fflush(stdout);*/
        } else {
            /* log_message(machine_window_log, "mouse move %f, %f GetCursorPos: %ld, %ld", motion->x, motion->y, pt.x, pt.y); */
            /*printf("mouse move %f, %f GetCursorPos: %ld, %ld\n", motion->x, motion->y, pt.x, pt.y); fflush(stdout);*/
            mouse_host_moved(pt.x * scale, pt.y * scale);
        }

#else /* Xlib, warp is relative to window */

        /* scale is a uniform scaling applied to eg high dpi widgets by gtk */
        int scale = gtk_widget_get_scale_factor(widget);
        mouse_host_moved(
            (widget_x + motion->x) * scale,
            (widget_y + motion->y) * scale);

#endif

        pthread_mutex_unlock(&canvas->lock);
        return FALSE;
    }

    /*
     * Mouse isn't captured, so we update the pen position.
     */
#if 0
    /* FIXME: this value is wrong in (at least) xvic, it is always the full
              width of produced video, not just the width of the shown window */
    double render_w = canvas->geometry->screen_size.width;
#else
    /* get width from render context instead */
#ifdef WINDOWS_COMPILE
    vice_directx_renderer_context_t *context = (vice_directx_renderer_context_t *)canvas->renderer_context;
    double render_w = context->bitmap_width;
    if (canvas->videoconfig->double_size_enabled) {
        render_w /= 2.0f;
    }
#else
    vice_opengl_renderer_context_t *context = (vice_opengl_renderer_context_t *)canvas->renderer_context;
    double render_w = context->current_frame_width;
    if (canvas->videoconfig->double_size_enabled) {
        render_w /= 2.0f;
    }
#endif
    /* sanity check */
    if (render_w != canvas->geometry->screen_size.width) {
        static int once = 0;
        if (!once) {
            log_warning(LOG_DEFAULT, "geometry->screen_size.width (%u) does not match actual rendered width (%f)",
                        canvas->geometry->screen_size.width, render_w);
            once++;
        }
    }
#endif
    double render_h = canvas->geometry->last_displayed_line - canvas->geometry->first_displayed_line + 1;

    /* There might be some sweet off-by-0.5 bugs here */
    int pen_x = (motion->x - canvas->screen_origin_x) * render_w / canvas->screen_display_w;
    int pen_y = (motion->y - canvas->screen_origin_y) * render_h / canvas->screen_display_h;

    if (pen_x < 0 || pen_y < 0 || pen_x >= render_w || pen_y >= render_h) {
        /* Mouse pointer is offscreen, so the light pen is disabled. */
        canvas->pen_x = -1;
        canvas->pen_y = -1;
        canvas->pen_buttons = 0;
    } else {
        canvas->pen_x = pen_x;
        canvas->pen_y = pen_y;
    }

    pthread_mutex_unlock(&canvas->lock);
    return FALSE;
}

/** \brief Callback for handling mouse button events over the emulated
 *         screen.
 *
 *  This forwards any button press or release events on to the light
 *  pen and mouse subsystems.
 *
 *  \param widget    The widget that sent the event.
 *  \param event     The GdkEventButton event itself.
 *  \param user_data The video canvas data structure associated with
 *                   this machine window.
 *  \return TRUE if no further event processing is necessary.
 *
 *  \sa event_box_mouse_motion_cb Further handling of light pen and
 *      mouse events.
 *  \sa event_box_scroll_cb Further handling of mouse button events.
 */
static gboolean event_box_mouse_button_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    video_canvas_t *canvas = (video_canvas_t *)user_data;

    if (event->type == GDK_BUTTON_PRESS) {
        int button = ((GdkEventButton *)event)->button;

        pthread_mutex_lock(&canvas->lock);
        if (button == 1) {
            /* Left mouse button */
            canvas->pen_buttons |= LP_HOST_BUTTON_LEFT;
        } else if (button == 3) {
            /* Right mouse button */
            canvas->pen_buttons |= LP_HOST_BUTTON_RIGHT;
        }
        pthread_mutex_unlock(&canvas->lock);

        /* Don't send button events if the mouse isn't captured */
        if (_mouse_enabled) {
            mouse_button(button - 1, 1);
        }
    } else if (event->type == GDK_BUTTON_RELEASE) {
        int button = ((GdkEventButton *)event)->button;

        pthread_mutex_lock(&canvas->lock);
        if (button == 1) {
            /* Left mouse button */
            canvas->pen_buttons &= ~LP_HOST_BUTTON_LEFT;
        } else if (button == 3) {
            /* Right mouse button */
            canvas->pen_buttons &= ~LP_HOST_BUTTON_RIGHT;
        }
        pthread_mutex_unlock(&canvas->lock);

        /* Don't send button events if the mouse isn't captured */
        if (_mouse_enabled) {
            mouse_button(button - 1, 0);
        }
    }

    /* Ignore all other mouse button events, though we'll be sent
     * things like double- and triple-click. */
    return FALSE;
}

/** \brief Callback for handling mouse scroll wheel events over the
 *         emulated screen.
 *
 *  GTK generates these by translating button presses on buttons 4 and
 *  5 into scroll events; we convert them back and forward them on to
 *  the mouse subsystem.
 *
 *  "Smooth scroll" events are also processed, interpreted as "up
 *  scroll" or "down scroll" based on the vertical component of the
 *  smooth-scroll event.
 *
 *  \param widget    The widget that sent the event.
 *  \param event     The GdkEventScroll event itself.
 *  \param user_data The video canvas data structure associated with
 *                   this machine window.
 *  \return TRUE if no further event processing is necessary.
 *
 *  \sa event_box_scroll_cb Further handling of mouse button events.
 */
static gboolean event_box_scroll_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    GdkScrollDirection dir = ((GdkEventScroll *)event)->direction;
    gdouble smooth_x = 0.0, smooth_y = 0.0;

    /* Don't send button events if the mouse isn't captured */
    if (!_mouse_enabled) {
        return FALSE;
    }

    switch (dir) {
    case GDK_SCROLL_UP:
        mouse_button(3, 1);
        break;
    case GDK_SCROLL_DOWN:
        mouse_button(4, 1);
        break;
    case GDK_SCROLL_SMOOTH:
        /* Isolate the Y component of a smooth scroll */
        if (gdk_event_get_scroll_deltas(event, &smooth_x, &smooth_y)) {
            if (smooth_y < 0) {
                mouse_button(3, 1);
            } else if (smooth_y > 0) {
                mouse_button(4, 1);
            }
        }
        break;
    default:
        /* Ignore left and right scroll */
        break;
    }
    return FALSE;
}


/** \brief Create a reusable cursor that may be used as part of this
 *         widget.
 *
 *  GDK cursors are tied to specific displays, so they need to be
 *  created for each machine window individually.
 *
 *  \param widget The widget that will be using this cursor.
 *  \param name   The name of the cursor to create.
 *  \return A new, non-floating, GdkCursor reference, or NULL on
 *          failure.
 *
 *  \note Users coming to this code from the more X11-centric GTK2
 *        will notice that the array of guaranteed-available cursors
 *        is much smaller. Please continue to only use the cursors
 *        listed in the documentation for gdk_cursor_new_from_name()
 *        here.
 */
static GdkCursor *make_cursor(GtkWidget *widget, const char *name)
{
    GdkDisplay *display = gtk_widget_get_display(widget);
    GdkCursor *result = NULL;

    if (display) {
        result = gdk_cursor_new_from_name(display, name);
        if (result != NULL) {
            g_object_ref_sink(G_OBJECT(result));
        }
    }
    return result;
}


/** \brief Frame-advance callback for the hide-mouse-when-idle logic.
 *
 *  This function is called as the "tick callback" whenever the mouse
 *  is hovering over the machine's screen. Its job is primarily to
 *  manage the mouse cursor:
 *
 *  - If the light pen is active, the cursor is always visible and is
 *    shaped like a crosshair.
 *  - If the mouse is grabbed, the cursor is never visible.
 *  - Otherwise, the cursor is visible as a normal mouse pointer as
 *    long as it's been 60 or fewer ticks since the last time the
 *    mouse moved.
 *
 *  \param widget    The widget that sent the event.
 *  \param clock     The GdkFrameClock that's managing our ticks.
 *  \param user_data The video canvas data structure associated with
 *                   this machine window.
 *  \return TRUE if no further event processing is necessary.
 *
 *  \sa event_box_cross_cb  Manages the lifecycle of this tick
 *      callback.
 *  \sa event_box_motion_cb Manages the "ticks since the last time the
 *      mouse moved" counter.
 */
static gboolean event_box_stillness_tick_cb(GtkWidget *widget, GdkFrameClock *clock, gpointer user_data)
{
    video_canvas_t *canvas = (video_canvas_t *)user_data;
    GdkWindow *window = gtk_widget_get_window(widget);

    _mouse_still_frames++;

    if (ignore_mouse_hide) {
        if (window != NULL) {
            gdk_window_set_cursor(window, NULL);
        }
    } else if (_mouse_enabled || (!lightpen_enabled && _mouse_still_frames > 60)) {
        if (canvas->blank_ptr == NULL) {
            canvas->blank_ptr = make_cursor(widget, "none");
        }
        if (canvas->blank_ptr != NULL) {
            if (window) {
                gdk_window_set_cursor(window, VICE_EMPTY_POINTER);
            }
        }
    } else {
        if (canvas->pen_ptr == NULL) {
            canvas->pen_ptr = make_cursor(widget, "crosshair");
        }
        if (window) {
            if (lightpen_enabled && canvas->pen_ptr) {
                gdk_window_set_cursor(window, canvas->pen_ptr);
            } else {
                gdk_window_set_cursor(window, NULL);
            }
        }
    }

    return G_SOURCE_CONTINUE;
}

/** \brief Callback for managing the hide-pointer-on-idle timings.
 *
 *  This callback fires whenever the machine window's canvas gains or
 *  loses focus over the mouse pointer. It manages the logic that
 *  hides the mouse pointer after inactivity. Entering the window will
 *  start the timer, and leaving it will stop it.
 *
 *  Leaving the window entirely will also be interpreted as removing
 *  the light pen from the screen.
 *
 *  \param widget    The widget that sent the event.
 *  \param event     The GdkEventCrossing event itself.
 *  \param user_data The video canvas data structure associated with
 *                   this machine window.
 *  \return TRUE if no further event processing is necessary.
 *
 *  \sa event_box_stillness_tick_cb The timer managed by this function.
 */
static gboolean event_box_cross_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    video_canvas_t *canvas = (video_canvas_t *)user_data;
    GdkEventCrossing *crossing = (GdkEventCrossing *)event;

    if (!canvas || !event ||
        (event->type != GDK_ENTER_NOTIFY && event->type != GDK_LEAVE_NOTIFY) ||
        crossing->mode != GDK_CROSSING_NORMAL) {
        /* Spurious event. Most likely, this is an event fired because
         * clicking the canvas altered grab status. */
        return FALSE;
    }

    if (crossing->type == GDK_ENTER_NOTIFY) {
        _mouse_still_frames = 0;
        if (canvas->still_frame_callback_id == 0) {
            canvas->still_frame_callback_id = gtk_widget_add_tick_callback(canvas->event_box,
                                                                           event_box_stillness_tick_cb,
                                                                           canvas, NULL);
        }
        return FALSE;
    }

    if (crossing->type == GDK_LEAVE_NOTIFY) {
        if (_mouse_enabled && gtk_window_has_toplevel_focus(GTK_WINDOW(gtk_widget_get_toplevel(canvas->event_box)))) {
            /* Warp the mouse back */
            ui_mouse_grab_pointer();
        } else {
            GdkWindow *window = gtk_widget_get_window(canvas->event_box);

            if (window) {
                gdk_window_set_cursor(window, NULL);
            }
            if (canvas->still_frame_callback_id != 0) {
                gtk_widget_remove_tick_callback(canvas->event_box, canvas->still_frame_callback_id);
                canvas->still_frame_callback_id = 0;
            }
            pthread_mutex_lock(&canvas->lock);
            canvas->pen_x = -1;
            canvas->pen_y = -1;
            canvas->pen_buttons = 0;
            pthread_mutex_unlock(&canvas->lock);
        }
    }

    return FALSE;
}

/** \brief Create a new machine window.
 *
 *  A machine window is a GtkGrid that has a menu bar on top, a status
 *  bar on the bottom, and a renderer-backend specific drawing area in
 *  the middle. The canvas argument has its relevant fields populated
 *  by this process.
 *
 *  \param canvas The video canvas to populate.
 *
 *  \todo At the moment, the renderer backend is selected at compile
 *        time and cannot be changed. It would be nice to be able to
 *        fall back to simpler backends if more specialized ones
 *        fail. This is difficult at present because we cannot know if
 *        OpenGL is available until long after the window is created
 *        and realized.
 */
static void machine_window_create(video_canvas_t *canvas)
{
    GtkWidget *new_event_box;
    GtkWidget *menu_bar;
    char *backend_label;
    unsigned w, h, vstretch = 0, hstretch = 0;
    gint window_id = PRIMARY_WINDOW;

    machine_window_log = log_open("Window");

    /* hack to determine window index for use in UI action handling later */
    if (strcmp(canvas->videoconfig->chip_name, "VDC") == 0) {
        window_id = SECONDARY_WINDOW;
    }

#ifdef WINDOWS_COMPILE
    canvas->renderer_backend = &vice_directx_backend;
    backend_label = "DirectX";
#else
    canvas->renderer_backend = &vice_opengl_backend;
    backend_label = "OpenGL";
#endif

    log_message(machine_window_log, "Chip '%s' using GTK3 backend '%s'.",
                canvas->videoconfig->chip_name , backend_label);

    new_event_box = gtk_event_box_new();

    gtk_widget_set_hexpand(new_event_box, TRUE);
    gtk_widget_set_vexpand(new_event_box, TRUE);

    canvas->event_box = new_event_box;
    canvas->renderer_backend->initialise(canvas);

    gtk_widget_add_events(new_event_box, GDK_POINTER_MOTION_MASK);
    gtk_widget_add_events(new_event_box, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(new_event_box, GDK_BUTTON_RELEASE_MASK);
    gtk_widget_add_events(new_event_box, GDK_SCROLL_MASK);

    g_signal_connect_unlocked(new_event_box, "enter-notify-event", G_CALLBACK(event_box_cross_cb), canvas);
    g_signal_connect_unlocked(new_event_box, "leave-notify-event", G_CALLBACK(event_box_cross_cb), canvas);

    /* Important mouse event handling to bypass the lock and be immediately visible to the emulator */
    g_signal_connect_unlocked(new_event_box, "motion-notify-event", G_CALLBACK(event_box_motion_cb), canvas);
    g_signal_connect_unlocked(new_event_box, "button-press-event", G_CALLBACK(event_box_mouse_button_cb), canvas);
    g_signal_connect_unlocked(new_event_box, "button-release-event", G_CALLBACK(event_box_mouse_button_cb), canvas);
    g_signal_connect_unlocked(new_event_box, "scroll-event", G_CALLBACK(event_box_scroll_cb), canvas);

    /* I'm pretty sure when running x128 we get two menu instances, so this
     * should go somewhere else: call ui_menu_bar_create() once and attach the
     * result menu to each GtkWindow instance
     *
     *   Won't work since a GtkWidget cannot have two parents. I already tried
     *   this, Gtk hade a huge fit. --compyx
     */
    menu_bar = ui_machine_menu_bar_create(window_id);

    gtk_container_add(GTK_CONTAINER(canvas->grid), menu_bar);
    gtk_container_add(GTK_CONTAINER(canvas->grid), new_event_box);

    /* Crazy hack to make the initial window open at the right place of the screen. The Problem here is, that
       if we don't explicitly set a window position (eg because we start with -default), then the window would
       be initially created smaller than it will be when initialization finished. The window manager will take
       that small size for determining the initial position, and if it then resizes after that, the window may
       no more be centered, or even partially off screen.

       To prevent this from happening, we try to set the correct final size here.

       CAUTION: since the resources are not properly initialized yet, this is tricky and we must use some dirty
       tricks and assumptions. the following also shows shortcomings / problems in other parts of the code.
    */
#if 1
    /*log_verbose(machine_window_log, "chip_name: %s", canvas->videoconfig->chip_name);*/
    log_verbose(machine_window_log, " screen_size: %u x %u", canvas->geometry->screen_size.width, canvas->geometry->screen_size.height);
    /*log_verbose(machine_window_log, " first/lastline: %u x %u", canvas->viewport->first_line, canvas->viewport->last_line);*/
    log_verbose(machine_window_log, " gfx_size: %u x %u", canvas->geometry->gfx_size.width, canvas->geometry->gfx_size.height);
    log_verbose(machine_window_log, " gfx_position: %u x %u", canvas->geometry->gfx_position.x, canvas->geometry->gfx_position.y);
    log_verbose(machine_window_log, " first/last displayed line: %u x %u", canvas->geometry->first_displayed_line, canvas->geometry->last_displayed_line);
    log_verbose(machine_window_log, " extra offscreen border left/right: %u x %u", canvas->geometry->extra_offscreen_border_left, canvas->geometry->extra_offscreen_border_right);
    /*log_verbose(machine_window_log, " screen_display_wh: %f x %f", (float)canvas->screen_display_w, (float)canvas->screen_display_h);*/
    /*log_verbose(machine_window_log, " canvas_physical_wh: %u x %u", canvas->draw_buffer->canvas_physical_width, canvas->draw_buffer->canvas_physical_width);*/
    log_verbose(machine_window_log, " scalexy: %d x %d sizexy: %u x %u",
              canvas->videoconfig->scalex, canvas->videoconfig->scaley,
              canvas->videoconfig->cap->single_mode.sizex, canvas->videoconfig->cap->single_mode.sizey);
    log_verbose(machine_window_log, " rmode: %u", canvas->videoconfig->cap->single_mode.rmode);
    log_verbose(machine_window_log, " aspect ratio: %f", (float)canvas->geometry->pixel_aspect_ratio);
#endif
    /* find out if we have a videochip that uses vertical stretching. since the resources are not
       initialized, assume it always is stretched (this is the default) */
    if (!strcmp("Crtc", canvas->videoconfig->chip_name)) {
        /* resources_get_int("CrtcStretchVertical", &vstretch); */
        if (machine_class == VICE_MACHINE_PET) {
            vstretch = 1; /* HACK: doing it for xcbm2 gives wrong result */
            hstretch = 1; /* HACK: compensate for missing scalex setup */
        }
    } else if (!strcmp("VDC", canvas->videoconfig->chip_name)) {
        /* resources_get_int("VDCStretchVertical", &vstretch); */
        /* vstretch = 1; */ /* HACK: for some reason that doesn't give the wanted result */
    }
#if 1
    log_verbose(machine_window_log, " hstretch: %u vstretch: %u", hstretch, vstretch);
#endif
    /* calculate the initial size from the values we have
       WARNING: terrible hacks coming up
    */
    w = (canvas->geometry->screen_size.width - canvas->geometry->gfx_position.x)
        * canvas->videoconfig->scalex * (hstretch ? 2 : 1);
    if (machine_class == VICE_MACHINE_VIC20) {
        w = canvas->geometry->gfx_size.width
            * canvas->videoconfig->scalex * (hstretch ? 2 : 1);
    }

    h = (canvas->geometry->last_displayed_line - canvas->geometry->first_displayed_line)
        * canvas->videoconfig->scaley * (vstretch ? 2 : 1);
    if (machine_class != VICE_MACHINE_PLUS4) {
        h = (unsigned)(((double)h) * canvas->geometry->pixel_aspect_ratio);
    }
#if 1
    log_verbose(machine_window_log, " initializing with width, height: %u x %u", w, h);
#endif
    /* finally set the size. use -1 for width and height to compensate for single pixel errors. this
       will be corrected by the resize that will happen at the end of initialization */
    gtk_widget_set_size_request (new_event_box, w - 1, h - 1);
    /* finally trigger a resize of the window to adjust to smallest size */
    gtk_window_resize(GTK_WINDOW(gtk_widget_get_parent(canvas->grid)), 1, 1);
    return;
}


/** \brief  Set up any resources needed to create new machine windows */
void ui_machine_window_init(void)
{
    ui_set_create_window_func(machine_window_create);
    return;
}


/** \brief  Grab the mouse pointer when mouse emulation is enabled */
void ui_mouse_grab_pointer(void)
{
    GtkWidget *window;
    float warp_x, warp_y;

    if (!_mouse_enabled) {
        return;
    }

    window = ui_get_window_by_index(PRIMARY_WINDOW);

    if (!window) {
        /* Probably mouse grab via config or command line, we'll grab it later via on_focus_in_event(). */
        return;
    }

    /*
     * We warp the mouse to the center of the primary window and move it back there
     * each time we detect mouse movement.
     */

#ifdef MACOS_COMPILE

    CGRect native_frame, content_rect;
    vice_macos_get_widget_frame_and_content_rect(window, &native_frame, &content_rect);

    /* macOS CoreGraphics coordinates origin is bottom-left of primary display */
    size_t main_display_height = CGDisplayPixelsHigh(CGMainDisplayID());

    warp_x =                       native_frame.origin.x + (int)(content_rect.size.width  / 2.0f);
    warp_y = main_display_height - native_frame.origin.y - (int)(content_rect.size.height / 2.0f);

#else

    /* First calculate destination relative to window */
    int window_width, window_height;
    gtk_window_get_size(GTK_WINDOW(window), &window_width, &window_height);

    int scale = gtk_widget_get_scale_factor(window);

    warp_x = (float)window_width  / 2.0f * scale;
    warp_y = (float)window_height / 2.0f * scale;

#ifdef WINDOWS_COMPILE

    /*
     * Windows uses SetCursorPos, which needs screen co-ordinates
     * Use native window API to get these, as it deals with multi-monitor
     * negative co-ordinates which are possible with some screen layouts.
     *
     * If you change this code, make sure you test with more than one monitor,
     * and test all layouts where the second monitor is left/above/below/right.
     *
     * If the code is wrong, you'll see the mouse pointer 'warp' to weird
     * screen locations instead of to the middle of the emu window.
     */

    GdkWindow *gdk_window = gtk_widget_get_window(window);
    HWND hWnd = gdk_win32_window_get_impl_hwnd(gdk_window);
    RECT rect;

    GetWindowRect(hWnd, &rect);

    warp_x += rect.left;
    warp_y += rect.top;

#endif
#endif

    mouse_host_capture(warp_x, warp_y);
}

/** \brief  Ungrab the mouse pointer when it was grabbed before */
void ui_mouse_ungrab_pointer(void)
{
    mouse_host_uncapture();

    /* Make the mouse appear as though it had been moved */
    _mouse_still_frames = 0;
}

#endif
