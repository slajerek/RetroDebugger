/**
 * \file directx_renderer.c
 * \brief   DirectX-based renderer for the GTK3 backend.
 *
 * \author David Hogan <david.q.hogan@gmail.com>
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

#include "vice.h"

#ifdef WINDOWS_COMPILE

#include "directx_renderer.h"
#include "directx_renderer_impl.h"

#include <assert.h>
#include <gtk/gtk.h>
#include <gdk/gdkwin32.h>
#include <math.h>
#include <windows.h>
#include <strsafe.h>

#include "lib.h"
#include "log.h"
#include "palette.h"
#include "render_queue.h"
#include "resources.h"
#include "ui.h"
#include "uistatusbar.h"

#define CANVAS_LOCK() pthread_mutex_lock(&canvas->lock)
#define CANVAS_UNLOCK() pthread_mutex_unlock(&canvas->lock)
#define RENDER_LOCK() pthread_mutex_lock(&context->render_lock)
#define RENDER_UNLOCK() pthread_mutex_unlock(&context->render_lock)

typedef vice_directx_renderer_context_t context_t;

#define VICE_DIRECTX_WINDOW_CLASS "VICE_DIRECTX_WINDOW_CLASS"

static WNDCLASS window_class;

static void on_widget_realized(GtkWidget *widget, gpointer data);
static void on_widget_unrealized(GtkWidget *widget, gpointer data);
static void on_widget_resized(GtkWidget *widget, GdkRectangle *allocation, gpointer data);

static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_NCHITTEST:
        /* We don't want mouse events - send them to the gdk parent window */
        return HTTRANSPARENT;

    case WM_PAINT:
        /* We need to repaint the current bitmap, which we do in the render thread */
        {
            video_canvas_t *canvas = (video_canvas_t *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            context_t *context = canvas->renderer_context;

            /* Prevent this event from being reposted continually */
            ValidateRect(hwnd, NULL);

            CANVAS_LOCK();
            /* If there is no pending render job, schedule one */
            if (!render_queue_length(context->render_queue)) {
                render_thread_push_job(context->render_thread, render_thread_render);
            }
            CANVAS_UNLOCK();
        }
        return 0;

    case WM_DISPLAYCHANGE:
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

static void vice_directx_initialise_canvas(video_canvas_t *canvas)
{
    context_t *context;

    /* First create the context_t that we'll need everywhere */
    context = lib_calloc(1, sizeof(context_t));

    context->canvas_lock_ptr = &canvas->lock;
    pthread_mutex_init(&context->render_lock, NULL);
    canvas->renderer_context = context;

    g_signal_connect (canvas->event_box, "realize", G_CALLBACK (on_widget_realized), canvas);
    g_signal_connect (canvas->event_box, "unrealize", G_CALLBACK (on_widget_unrealized), canvas);
    g_signal_connect_unlocked(canvas->event_box, "size-allocate", G_CALLBACK(on_widget_resized), canvas);
}

static void vice_directx_destroy_context(video_canvas_t *canvas)
{
    context_t *context;

    CANVAS_LOCK();

    context = canvas->renderer_context;

    if (context) {
        pthread_mutex_destroy(&context->render_lock);
        lib_free(context);
        canvas->renderer_context = NULL;
    }

    CANVAS_UNLOCK();
}

static void on_widget_realized(GtkWidget *widget, gpointer data)
{
    video_canvas_t *canvas = data;
    context_t *context = canvas->renderer_context;

    /* Register the native window class if that hasn't happened yet */
    if (!window_class.lpszClassName) {
        window_class.lpszClassName = VICE_DIRECTX_WINDOW_CLASS;
        window_class.hInstance = GetModuleHandle(NULL);
        window_class.lpfnWndProc = WindowProcedure;
        window_class.style = CS_HREDRAW | CS_VREDRAW;
        window_class.cbWndExtra = sizeof(context_t *);

        if (!RegisterClass(&window_class)) {
            vice_directx_impl_log_windows_error("RegisterClass");
            return;
        }
    }

    /* Create a native child window for DirectX to render onto */
    if (!context->window) {
        context->window =
            CreateWindowEx(
                0,
                VICE_DIRECTX_WINDOW_CLASS,
                NULL,
                WS_CHILD,
                0, 0, 1, 1, /* we resize it when the underlying event_box gets resized */
                gdk_win32_window_get_handle(gtk_widget_get_window(gtk_widget_get_toplevel(widget))),
                NULL,
                GetModuleHandle(NULL),
                NULL);

        if (!context->window) {
            vice_directx_impl_log_windows_error("CreateWindowEx");
            return;
        }

        /* Make the context available to the windowproc */
        SetWindowLongPtr(context->window, GWLP_USERDATA, (LONG_PTR)canvas);

        ShowWindow(context->window, SW_SHOW);
    }

    context->render_queue = render_queue_create();
    context->render_bg_colour.a = 1.0f;

    /* Create an exclusive single thread 'pool' for executing render jobs */
    context->render_thread = render_thread_create(vice_directx_impl_async_render, canvas);
}

static void on_widget_unrealized(GtkWidget *widget, gpointer data)
{
    video_canvas_t *canvas = data;
    context_t *context = canvas->renderer_context;

    CANVAS_LOCK();

    vice_directx_destroy_context_impl(context);

    if (context->window) {
        DestroyWindow(context->window);
        context->window = NULL;
    }

    render_queue_destroy(context->render_queue);
    context->render_queue = NULL;

    CANVAS_UNLOCK();
}

/** The underlying GtkDrawingArea has changed size (possibly before being realised) */
static void on_widget_resized(GtkWidget *widget, GdkRectangle *allocation, gpointer data)
{
    video_canvas_t *canvas = data;
    context_t *context;
    gint viewport_x, viewport_y;
    gint gtk_scale = gtk_widget_get_scale_factor(widget);

    CANVAS_LOCK();

    context = canvas->renderer_context;
    if (!context) {
        CANVAS_UNLOCK();
        return;
    }

    gtk_widget_translate_coordinates(widget, gtk_widget_get_toplevel(widget), 0, 0, &viewport_x, &viewport_y);

    context->viewport_x      = viewport_x         * gtk_scale;
    context->viewport_y      = viewport_y         * gtk_scale;
    context->viewport_width  = allocation->width  * gtk_scale;
    context->viewport_height = allocation->height * gtk_scale;

    /* Set the background colour */
    if (ui_is_fullscreen_from_canvas(canvas)) {
        context->render_bg_colour.r = 0.0f;
        context->render_bg_colour.g = 0.0f;
        context->render_bg_colour.b = 0.0f;
    } else {
        context->render_bg_colour.r = 0.5f;
        context->render_bg_colour.g = 0.5f;
        context->render_bg_colour.b = 0.5f;
    }

    /* Update the size of the native child window to match the gtk drawing area */
    if (context->window) {
        MoveWindow(context->window, context->viewport_x, context->viewport_y, context->viewport_width, context->viewport_height, TRUE);
        if (!render_queue_length(context->render_queue)) {
            render_thread_push_job(context->render_thread, render_thread_render);
        }
        context->resized = true;
    }

    CANVAS_UNLOCK();
}

/******/

/** \brief The emulated screen size or aspect ratio has changed */
static void vice_directx_update_context(video_canvas_t *canvas, unsigned int width, unsigned int height)
{
    context_t *context;

    CANVAS_LOCK();

    context = canvas->renderer_context;

    context->emulated_width_next = width;
    context->emulated_height_next = height;
    context->pixel_aspect_ratio_next = canvas->geometry->pixel_aspect_ratio;

    CANVAS_UNLOCK();
}

/** \brief It's time to draw a complete emulated frame */
static void vice_directx_refresh_rect(video_canvas_t *canvas,
                                     unsigned int xs, unsigned int ys,
                                     unsigned int xi, unsigned int yi,
                                     unsigned int w, unsigned int h)
{
    context_t *context;
    backbuffer_t *backbuffer;
    int pixel_data_size_bytes;

    CANVAS_LOCK();

    context = canvas->renderer_context;
    if (!context || !context->render_queue) {
        CANVAS_UNLOCK();
        return;
    }

    /* Obtain an unused backbuffer to render to */
    pixel_data_size_bytes = context->emulated_width_next * context->emulated_height_next * 4;
    backbuffer = render_queue_get_from_pool(context->render_queue, pixel_data_size_bytes);

    if (!backbuffer) {
        CANVAS_UNLOCK();
        return;
    }

    backbuffer->width = context->emulated_width_next;
    backbuffer->height = context->emulated_height_next;
    backbuffer->pixel_aspect_ratio = context->pixel_aspect_ratio_next;
    backbuffer->interlaced = canvas->videoconfig->interlaced;
    backbuffer->interlace_field = canvas->videoconfig->interlace_field;

    CANVAS_UNLOCK();

    video_canvas_render(canvas, backbuffer->pixel_data, w, h, xs, ys, xi, yi, backbuffer->width * 4);

    CANVAS_LOCK();
    render_queue_enqueue_for_display(context->render_queue, backbuffer);
    render_thread_push_job(context->render_thread, render_thread_render);
    CANVAS_UNLOCK();
}

static void vice_directx_on_ui_frame_clock(GdkFrameClock *clock, video_canvas_t *canvas)
{
    context_t *context = canvas->renderer_context;

    ui_update_statusbars();

    CANVAS_LOCK();

    /* TODO we really shouldn't be setting this every frame! */
    gtk_widget_set_size_request(canvas->event_box, context->window_min_width, context->window_min_height);

    CANVAS_UNLOCK();
}

static void vice_directx_set_palette(video_canvas_t *canvas)
{
    int i;
    video_render_color_tables_t *color_tables = &canvas->videoconfig->color_tables;
    struct palette_s *palette = canvas ? canvas->palette : NULL;

    if (!palette) {
        return;
    }

    for (i = 0; i < palette->num_entries; i++) {
        palette_entry_t color = palette->entries[i];
        uint32_t color_code = color.red | (color.green << 8) | (color.blue << 16) | (0xffU << 24);
        video_render_setphysicalcolor(canvas->videoconfig, i, color_code, 32);
    }

    for (i = 0; i < 256; i++) {
        video_render_setrawrgb(color_tables, i, i, i << 8, i << 16);
    }
    video_render_setrawalpha(color_tables, 0xffU << 24);
    video_render_initraw(canvas->videoconfig);
}

vice_renderer_backend_t vice_directx_backend = {
    vice_directx_initialise_canvas,
    vice_directx_update_context,
    vice_directx_destroy_context,
    vice_directx_refresh_rect,
    vice_directx_on_ui_frame_clock,
    vice_directx_set_palette
};

#endif
