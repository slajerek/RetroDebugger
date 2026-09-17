/**
 * \file directx_renderer.h
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
#ifndef VICE_DIRECTX_RENDERER_IMPL_H
#define VICE_DIRECTX_RENDERER_IMPL_H

#include "vice.h"

#ifdef WINDOWS_COMPILE

#include <windows.h>

#include <d3d11_1.h>
#include <d2d1_1.h>
#include <glib.h>
#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "render_thread.h"

/** \brief Rendering context for the DirectX backend.
 *  \sa video_canvas_s::renderer_context */
typedef struct vice_directx_renderer_context_s {
    /** \brief needed to coordinate access to the context between vice and main threads */
    pthread_mutex_t *canvas_lock_ptr;

    /** \brief used to coordinate access to native rendering resources */
    pthread_mutex_t render_lock;

    /** \brief A 'pool' of one thread used to render backbuffers via directx */
    render_thread_t render_thread;

    /** \brief A queue of backbuffers ready for painting to the widget */
    void *render_queue;

    /** \brief native child window for DirectX to draw on */
    HWND window;

    /** \brief minimum size for the drawing area, based on emu and aspect ratio settings */
    unsigned int window_min_width;

    /** \brief minimum size for the drawing area, based on emu and aspect ratio settings */
    unsigned int window_min_height;

    /* So many DirectX */
    ID3D11Device1 *d3d_device;
    ID3D11DeviceContext1 *d3d_device_context;
    IDXGIDevice1 *dxgi_device;
    ID2D1Factory1 *d2d_factory;
    ID2D1Device *d2d_device;
    ID2D1DeviceContext *d2d_device_context;
    ID2D1Effect *d2d_effect_strip_alpha;
    ID2D1Effect *d2d_effect_premultiply_alpha;
    ID2D1Effect *d2d_effect_combine;
    ID2D1Effect *d2d_effect_scale;

    IDXGIAdapter *dxgi_adapter;
    IDXGIFactory2 *dxgi_factory;
    IDXGISwapChain1 *d3d_swap_chain;
    IDXGISurface *dxgi_surface;
    ID2D1Bitmap1 *dxgi_bitmap;

    /** \brief Direct2D bitmap used to get emu bitmap into the GPU */
    ID2D1Bitmap *render_bitmap;

    /** \brief size of the current gpu bitmap in pixels */
    unsigned int bitmap_width;

    /** \brief size of the current gpu bitmap in pixels */
    unsigned int bitmap_height;

    /** \brief Direct2D bitmap used to retain the last frame */
    ID2D1Bitmap *previous_frame_render_bitmap;

    /** \brief size of the current gpu bitmap in pixels */
    unsigned int previous_frame_bitmap_width;

    /** \brief size of the current gpu bitmap in pixels */
    unsigned int previous_frame_bitmap_height;

    /** \brief Where emu bitmap gets placed on the target surface */
    D2D1_RECT_F render_dest_rect;

    /** \brief Background colour */
    D2D1_COLOR_F render_bg_colour;

    /** \brief location of the directx viewport in window pixels */
    unsigned int viewport_x;

    /** \brief size of the directx viewport in window pixels */
    unsigned int viewport_y;

    /** \brief size of the directx viewport in window pixels */
    unsigned int viewport_width;

    /** \brief size of the directx viewport in window pixels */
    unsigned int viewport_height;

    /** \brief used to signal that the viewport has resized */
    bool resized;

    /** \brief aspect ratio of each pixel in the current gpu bitmap */
    float pixel_aspect_ratio;

    /** \brief size of the next emulated frame */
    unsigned int emulated_width_next;

    /** \brief size of the NEXT emulated frame */
    unsigned int emulated_height_next;

    /** \brief pixel aspect ratio of the next emulated frame */
    float pixel_aspect_ratio_next;

    /** \brief if the next render should use interlaced mode */
    bool interlaced;

    /** \brief the even/odd of the most recent interlaced field */
    int current_interlace_field;

} vice_directx_renderer_context_t;

void vice_directx_impl_log_windows_error(const char *prefix);

void vice_directx_destroy_context_impl(vice_directx_renderer_context_t *context);

void vice_directx_impl_async_render(void *pool_data, void *job_data);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* #ifdef WINDOWS_COMPILE */

#endif /* #ifndef VICE_DIRECTX_RENDERER_IMPL_H */
