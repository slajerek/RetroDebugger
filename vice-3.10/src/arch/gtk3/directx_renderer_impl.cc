/**
 * \file directx_renderer_impl.cc
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

#include "directx_renderer_impl.h"

#ifdef WINDOWS_COMPILE

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

extern "C"
{
#include "archdep.h"
#include "log.h"
#include "render_queue.h"
#include "resources.h"
#include "videoarch.h"
}

#define CANVAS_LOCK() pthread_mutex_lock(context->canvas_lock_ptr)
#define CANVAS_UNLOCK() pthread_mutex_unlock(context->canvas_lock_ptr)
#define RENDER_LOCK() pthread_mutex_lock(&context->render_lock)
#define RENDER_UNLOCK() pthread_mutex_unlock(&context->render_lock)

#define DX_RELEASE(x) if (x) { (x)->Release(); (x) = NULL; }

static void build_directx_resources(vice_directx_renderer_context_t *context)
{
    HRESULT result = S_OK;

    /*
     * Device dependent resources
     */

    /* Direct2D Factory for later */

    if (!context->d2d_factory) {
        D2D1_FACTORY_OPTIONS d2d_factory_options;
        ZeroMemory(&d2d_factory_options, sizeof(d2d_factory_options));

        result =
            D2D1CreateFactory(
                D2D1_FACTORY_TYPE_MULTI_THREADED,
                __uuidof(ID2D1Factory1),
                &d2d_factory_options,
                (void **)&context->d2d_factory
                );
        if (FAILED(result)) {
            vice_directx_impl_log_windows_error("D2D1CreateFactory1");
            vice_directx_destroy_context_impl(context);
            return;
        }
    }

    /* Create the Direct 3D device */

    D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };

    if (!context->d3d_device) {
        ID3D11Device *d3d_device;
        ID3D11DeviceContext *d3d_device_context;
        result =
            D3D11CreateDevice(
                nullptr,                            /* default gpu adapter */
                D3D_DRIVER_TYPE_HARDWARE,
                0,                                  /* no software renderer implementation DLL */
                D3D11_CREATE_DEVICE_BGRA_SUPPORT,   /* needed for Direct2D compatibility */
                featureLevels,
                ARRAYSIZE(featureLevels),
                D3D11_SDK_VERSION,
                &d3d_device,
                NULL,                               /* don't return resulting feature level */
                &d3d_device_context
                );
        if (FAILED(result))
        {
            vice_directx_impl_log_windows_error("D3D11CreateDevice");
            vice_directx_destroy_context_impl(context);
            return;
        }

        result = d3d_device->QueryInterface(__uuidof(ID3D11Device1), (void **)&context->d3d_device);
        if (FAILED(result))
        {
            vice_directx_impl_log_windows_error("ID3D11Device1");
            vice_directx_destroy_context_impl(context);
            return;
        }
        d3d_device->Release();

        result = d3d_device_context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void **)&context->d3d_device_context);
        if (FAILED(result))
        {
            vice_directx_impl_log_windows_error("ID3D11DeviceContext1");
            vice_directx_destroy_context_impl(context);
            return;
        }
        d3d_device_context->Release();
    }

    /* Get the underlying DXGI Device */

    if (!context->dxgi_device) {
        result = context->d3d_device->QueryInterface(__uuidof(IDXGIDevice), (void **)&context->dxgi_device);
        if (FAILED(result))
        {
            vice_directx_impl_log_windows_error("QueryInterface IDXGIDevice");
            vice_directx_destroy_context_impl(context);
            return;
        }
    }

    /* now get the Direct2D device and device context */

    if (!context->d2d_device) {
        result = context->d2d_factory->CreateDevice(context->dxgi_device, &context->d2d_device);
        if (FAILED(result))
        {
            vice_directx_impl_log_windows_error("CreateDevice ID2D1Device");
            vice_directx_destroy_context_impl(context);
            return;
        }

        result =
            context->d2d_device->CreateDeviceContext(
                D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                &context->d2d_device_context
                );
        if (FAILED(result))
        {
            vice_directx_impl_log_windows_error("CreateDeviceContext ID2D1DeviceContext");
            vice_directx_destroy_context_impl(context);
            return;
        }
    }

    /* Set up some image processing effects */

    if (!context->d2d_effect_strip_alpha) {
        /* Set alpha = 1, wonder if there's a more efficient way */
        D2D1_MATRIX_5X4_F strip_alpha_matrix =
            D2D1::Matrix5x4F(
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 0,
                0, 0, 0, 1
                );
        context->d2d_device_context->CreateEffect(CLSID_D2D1ColorMatrix, &context->d2d_effect_strip_alpha);
        context->d2d_effect_strip_alpha->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, strip_alpha_matrix);
    }

    if (!context->d2d_effect_premultiply_alpha) {
        /* VICE produces straight alpha images, Direct2D works with premultiplied alpha */
        context->d2d_device_context->CreateEffect(CLSID_D2D1Premultiply, &context->d2d_effect_premultiply_alpha);
    }

    if (!context->d2d_effect_combine) {
        /* Combine two bitmaps, used to stitch interlaced bitmaps */
        context->d2d_device_context->CreateEffect(CLSID_D2D1Composite, &context->d2d_effect_combine);
    }

    if (!context->d2d_effect_scale) {
        /* And scale up the final result */
        context->d2d_device_context->CreateEffect(CLSID_D2D1Scale, &context->d2d_effect_scale);
    }

    /*
     * Size dependent resources
     */

    /* Get the DXGI Adapter */

    if (!context->dxgi_adapter) {
        result = context->dxgi_device->GetAdapter(&context->dxgi_adapter);
        if (FAILED(result))
        {
            vice_directx_impl_log_windows_error("GetAdapter IDXGIAdapter");
            vice_directx_destroy_context_impl(context);
            return;
        }
    }

    /* Don't forget the DXGI Factory! You nearly forgot didn't you. */

    if (!context->dxgi_factory) {
        result = context->dxgi_adapter->GetParent(IID_PPV_ARGS(&context->dxgi_factory));
        if (FAILED(result))
        {
            vice_directx_impl_log_windows_error("dxgi_factory");
            vice_directx_destroy_context_impl(context);
            return;
        }
    }

    /* Now we need to create a swap chain for our hwnd */

    if (!context->d3d_swap_chain) {

        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = { 0 };
        swap_chain_desc.Width                 = 0;                                /* use automatic sizing */
        swap_chain_desc.Height                = 0;
        swap_chain_desc.Format                = DXGI_FORMAT_B8G8R8A8_UNORM;       /* this is the most common swapchain format */
        swap_chain_desc.Stereo                = false;
        swap_chain_desc.SampleDesc.Count      = 1;                                /* don't use multi-sampling */
        swap_chain_desc.SampleDesc.Quality    = 0;
        swap_chain_desc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.BufferCount           = 2;                                /* use double buffering to enable flip */
        swap_chain_desc.Scaling               = DXGI_SCALING_STRETCH;
        swap_chain_desc.SwapEffect            = DXGI_SWAP_EFFECT_DISCARD;
        swap_chain_desc.Flags                 = 0;

        result =
            context->dxgi_factory->CreateSwapChainForHwnd(
                context->d3d_device,
                context->window,
                &swap_chain_desc,
                NULL,                       /* Windowed swap chain */
                NULL,                       /* Don't restrict output device */
                &context->d3d_swap_chain
            );
        if (FAILED(result))
        {
            vice_directx_impl_log_windows_error("d3d_swap_chain");
            vice_directx_destroy_context_impl(context);
            return;
        }

        /* Ensure that DXGI doesn't queue more than one frame at a time. */
        context->dxgi_device->SetMaximumFrameLatency(1);
    }

    /*
     * Frame dependant resources
     */

    /* Get a DXGI surface representing the backbuffer */

    if (!context->dxgi_surface && context->d3d_swap_chain) {
        result = context->d3d_swap_chain->GetBuffer(0, IID_PPV_ARGS(&context->dxgi_surface));
        if (FAILED(result))
        {
            vice_directx_impl_log_windows_error("dxgi_surface");
            vice_directx_destroy_context_impl(context);
            return;
        }
    }

    /* Create a bitmap from the DXGI surface */
    FLOAT dpiX, dpiY;
    context->d2d_factory->GetDesktopDpi(&dpiX, &dpiY);

    if (!context->dxgi_bitmap) {
        D2D1_BITMAP_PROPERTIES1 bitmap_properties =
            D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                dpiX,
                dpiY
                );

        result =
            context->d2d_device_context->CreateBitmapFromDxgiSurface(
                context->dxgi_surface,
                bitmap_properties,
                &context->dxgi_bitmap
                );
        if (FAILED(result))
        {
            vice_directx_impl_log_windows_error("dxgi_bitmap");
            vice_directx_destroy_context_impl(context);
            return;
        }
    }
}

static void destroy_size_dependent_resources(vice_directx_renderer_context_t *context)
{
    DX_RELEASE(context->dxgi_bitmap);
    DX_RELEASE(context->dxgi_surface);
    DX_RELEASE(context->d3d_swap_chain);
    DX_RELEASE(context->dxgi_factory);
    DX_RELEASE(context->dxgi_adapter);
}

static void destroy_device_dependent_resources(vice_directx_renderer_context_t *context)
{
    DX_RELEASE(context->render_bitmap);
    DX_RELEASE(context->previous_frame_render_bitmap);

    DX_RELEASE(context->d2d_effect_scale);
    DX_RELEASE(context->d2d_effect_combine);
    DX_RELEASE(context->d2d_effect_premultiply_alpha);
    DX_RELEASE(context->d2d_effect_strip_alpha);
    DX_RELEASE(context->d2d_device_context);
    DX_RELEASE(context->d2d_device);
    DX_RELEASE(context->dxgi_device);
    DX_RELEASE(context->d3d_device_context);
    DX_RELEASE(context->d3d_device);
    DX_RELEASE(context->d2d_factory);
}

void vice_directx_destroy_context_impl(vice_directx_renderer_context_t *context)
{
    destroy_size_dependent_resources(context);
    destroy_device_dependent_resources(context);
}

static void build_render_bitmap(vice_directx_renderer_context_t *context, backbuffer_t *backbuffer)
{
    HRESULT result = S_OK;

    if (context->d2d_device_context) {
        if (backbuffer->interlace_field != context->current_interlace_field) {
            /* Retain the previous bitmaps to use in interlaced mode */
            ID2D1Bitmap *swap_bitmap              = context->previous_frame_render_bitmap;
            unsigned int swap_width               = context->previous_frame_bitmap_width;
            unsigned int swap_height              = context->previous_frame_bitmap_height;
            context->previous_frame_render_bitmap = context->render_bitmap;
            context->previous_frame_bitmap_width  = context->bitmap_width;
            context->previous_frame_bitmap_height = context->bitmap_height;
            context->render_bitmap                = swap_bitmap;
            context->bitmap_width                 = swap_width;
            context->bitmap_height                = swap_height;
            context->current_interlace_field      = backbuffer->interlace_field;
        }

        /* Is it sill the right size? */
        /* HACK - need to track each bitmap size */
        if (context->render_bitmap && (context->bitmap_width != backbuffer->width || context->bitmap_height != backbuffer->height)) {
            /* Nope, release it and let another be created */
            DX_RELEASE(context->render_bitmap);
        }

        /* Create bitmaps, if needed */
        if (!context->render_bitmap)
        {
            /*
             * Create the Direct2D bitmap used to get the emu display into the gpu
             */

            D2D1_BITMAP_PROPERTIES bitmap_properies;
            bitmap_properies.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
            context->d2d_factory->GetDesktopDpi(&bitmap_properies.dpiX, &bitmap_properies.dpiY);

            result =
                context->d2d_device_context->CreateBitmap(
                    D2D1::SizeU(backbuffer->width, backbuffer->height),
                    bitmap_properies,
                    &context->render_bitmap);

            if (FAILED(result)) {
                vice_directx_impl_log_windows_error("CreateBitmap1");
                return;
            }

            context->bitmap_width  = backbuffer->width;
            context->bitmap_height = backbuffer->height;
        }

        context->interlaced = backbuffer->interlaced;
        context->pixel_aspect_ratio = backbuffer->pixel_aspect_ratio;

        /* Copy the emulated screen to the Bitmap */
        D2D1_RECT_U bitmap_rect = D2D1::RectU(0, 0, backbuffer->width, backbuffer->height);

        result =
            context->render_bitmap->CopyFromMemory(
                &bitmap_rect,
                backbuffer->pixel_data,
                backbuffer->width * 4);
        if (FAILED(result)) {
            vice_directx_impl_log_windows_error("CopyFromMemory");
            DX_RELEASE(context->render_bitmap);
            return;
        }
    }
}

static void recalculate_layout(video_canvas_t *canvas, vice_directx_renderer_context_t *context)
{
    float scale_x;
    float scale_y;
    D2D1_SIZE_U  viewport_size_ddp;
    D2D1_SIZE_U  bitmap_size_ddp;

    if (!context->dxgi_bitmap || !context->render_bitmap) {
        return;
    }

    viewport_size_ddp   = context->dxgi_bitmap->GetPixelSize();
    bitmap_size_ddp     = context->render_bitmap->GetPixelSize();

    if (canvas->videoconfig->aspect_mode != VIDEO_ASPECT_MODE_NONE) {
        float viewport_aspect;
        float emulated_aspect;

        viewport_aspect = (float)viewport_size_ddp.width / viewport_size_ddp.height;
        emulated_aspect = (float)bitmap_size_ddp.width   / bitmap_size_ddp.height;

        if (canvas->videoconfig->aspect_mode == VIDEO_ASPECT_MODE_TRUE) {
            emulated_aspect *= context->pixel_aspect_ratio;
        } else {
            emulated_aspect *= canvas->videoconfig->aspect_ratio;
        }

        if (emulated_aspect < viewport_aspect) {
            scale_x = emulated_aspect / viewport_aspect;
            scale_y = 1.0f;
        } else {
            scale_x = 1.0f;
            scale_y = viewport_aspect / emulated_aspect;
        }
    } else {
        scale_x = 1.0f;
        scale_y = 1.0f;
    }

    /* Calculate the minimum drawing area size to be enforced by gtk */
    if (canvas->videoconfig->aspect_mode == VIDEO_ASPECT_MODE_TRUE) {
        context->window_min_width = ceil((float)context->bitmap_width * context->pixel_aspect_ratio);
        context->window_min_height = context->bitmap_height;
    } else {
        context->window_min_width = context->bitmap_width;
        context->window_min_height = context->bitmap_height;
    }

    /* These values aren't used anywhere other than here in this renderer */
    canvas->screen_display_w = (viewport_size_ddp.width  * scale_x);
    canvas->screen_display_h = (viewport_size_ddp.height * scale_y);
    canvas->screen_origin_x = ((viewport_size_ddp.width  - canvas->screen_display_w) / 2.0);
    canvas->screen_origin_y = ((viewport_size_ddp.height - canvas->screen_display_h) / 2.0);

#if 0
    /*
     * Direct2D thinks in terms of device indepenent pixels,
     * but GTK gives us pixel sizes in device dependent pixel sizes.
     *
     * When rendering, we convert from DDP to DIP, this ensures that
     * our filter choice is applied rather than whatever D2D is doing
     * under the hood when you set dpi on the bitmap itself.
     */

    context->d2d_factory->GetDesktopDpi(&dpi_x, &dpi_y);

    /* Update the detination rect used directly by the renderer */
    context->render_dest_rect.left   =  canvas->screen_origin_x                             * 96.0f / dpi_x;
    context->render_dest_rect.right  = (canvas->screen_origin_x + canvas->screen_display_w) * 96.0f / dpi_x;
    context->render_dest_rect.top    =  canvas->screen_origin_y                             * 96.0f / dpi_y;
    context->render_dest_rect.bottom = (canvas->screen_origin_y + canvas->screen_display_h) * 96.0f / dpi_y;
#endif

    /* Update the detination rect used directly by the renderer */
    context->render_dest_rect.left   = canvas->screen_origin_x;
    context->render_dest_rect.right  = canvas->screen_origin_x + canvas->screen_display_w;
    context->render_dest_rect.top    = canvas->screen_origin_y;
    context->render_dest_rect.bottom = canvas->screen_origin_y + canvas->screen_display_h;
}

void vice_directx_impl_async_render(void *job_data, void *pool_data)
{
    render_job_t job = (render_job_t)vice_ptr_to_int(job_data);
    video_canvas_t *canvas = (video_canvas_t *)pool_data;
    vice_directx_renderer_context_t *context = (vice_directx_renderer_context_t *)canvas->renderer_context;
    backbuffer_t *backbuffer;
    HRESULT result = S_OK;
    bool interlaced;
    int vsync = canvas->videoconfig->vsync;
    int filter = canvas->videoconfig->glfilter;
    int flipidx = canvas->videoconfig->flipx | (canvas->videoconfig->flipy << 1);
    DXGI_PRESENT_PARAMETERS present_parameters = { 0 };

    if (job == render_thread_init) {
        archdep_thread_init();

        /* Make sure the render thread wakes up and does its thing asap. */
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

        log_verbose(LOG_DEFAULT, "Render thread initialised");
        return;
    }

    if (job == render_thread_shutdown) {
        archdep_thread_shutdown();
        log_verbose(LOG_DEFAULT, "Render thread shutdown");
        return;
    }

    CANVAS_LOCK();

    if (context->resized) {
        destroy_size_dependent_resources(context);
        context->resized = false;
    }

    RENDER_LOCK();

    /*
     * Update the bitmaps. All need to be processed for correct interlace rendering,
     * especially when the monitor is open and stepping through code.
     */

    backbuffer = render_queue_dequeue_for_display(context->render_queue);
    if (backbuffer) {
        build_render_bitmap(context, backbuffer);
        render_queue_return_to_pool(context->render_queue, backbuffer);
    }

    build_directx_resources(context);

    recalculate_layout(canvas, context);

    interlaced = context->interlaced;

    CANVAS_UNLOCK();

    if (!context->d2d_device_context) {
        log_message(LOG_DEFAULT, "no render target, not rendering this frame");
        RENDER_UNLOCK();
        return;
    }

    /* Each frame, set the backbuffer bitmap as the Direct2D render target. */
    context->d2d_device_context->SetTarget(context->dxgi_bitmap);

    context->d2d_device_context->BeginDraw();
    context->d2d_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
    context->d2d_device_context->Clear(&context->render_bg_colour);

    if (interlaced && context->previous_frame_render_bitmap && context->render_bitmap) {
        /* Render the previous frame ignoring alpha */
        context->d2d_effect_strip_alpha->SetInput(0, context->previous_frame_render_bitmap);

        /* Premultiply the alpha on the new frame */
        context->d2d_effect_premultiply_alpha->SetInput(0, context->render_bitmap);

        /* Combine the two processed bitmaps */
        context->d2d_effect_combine->SetInputEffect(0, context->d2d_effect_strip_alpha);
        context->d2d_effect_combine->SetInputEffect(1, context->d2d_effect_premultiply_alpha);

        context->d2d_effect_scale->SetInputEffect(0, context->d2d_effect_combine);
    } else {
        /* Render the current frame ignoring alpha */
        context->d2d_effect_strip_alpha->SetInput(0, context->render_bitmap);

        context->d2d_effect_scale->SetInputEffect(0, context->d2d_effect_strip_alpha);
    }

    context->d2d_effect_scale->SetValue(
        2, /* D2D1_SCALE_PROP_INTERPOLATION_MODE, For some reason this enum value isn't picked up on msys */
        filter == 0
            ? 0   /* D2D1_SCALE_INTERPOLATION_MODE_NEAREST_NEIGHBOR */
            : filter == 2
                ? 5 /* D2D1_SCALE_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC */
                : 1 /* D2D1_SCALE_INTERPOLATION_MODE_LINEAR */
        );
    context->d2d_effect_scale->SetValue(
        0, /* D2D1_SCALE_PROP_SCALE, For some reason this enum value isn't picked up on msys */
        D2D1::Vector2F(
            (float)(context->render_dest_rect.right - context->render_dest_rect.left) / context->bitmap_width,
            (float)(context->render_dest_rect.bottom - context->render_dest_rect.top) / context->bitmap_height
            )
        );

    /* FIXME: add support for rotate */

    switch (flipidx) {
        default:
        case 0:
            context->d2d_device_context->SetTransform(
                D2D1::Matrix3x2F::Translation(
                    context->render_dest_rect.left,
                    context->render_dest_rect.top
                    )
            );
            break;
        case 1:
            context->d2d_device_context->SetTransform(
                D2D1::Matrix3x2F(-1,0,0,1,0,0) *
                D2D1::Matrix3x2F::Translation(
                    context->render_dest_rect.right,
                    context->render_dest_rect.top
                    )
            );
            break;
        case 2:
            context->d2d_device_context->SetTransform(
                D2D1::Matrix3x2F(1,0,0,-1,0,0) *
                D2D1::Matrix3x2F::Translation(
                    context->render_dest_rect.left,
                    context->render_dest_rect.bottom
                    )
            );
            break;
        case 3:
            context->d2d_device_context->SetTransform(
                D2D1::Matrix3x2F(-1,0,0,-1,0,0) *
                D2D1::Matrix3x2F::Translation(
                    context->render_dest_rect.right,
                    context->render_dest_rect.bottom
                    )
            );
            break;
    }
    context->d2d_device_context->DrawImage(context->d2d_effect_scale);

    result = context->d2d_device_context->EndDraw();

    if (result == D2DERR_RECREATE_TARGET) {
        printf("Must recreate resources\n");
        destroy_size_dependent_resources(context);
        destroy_device_dependent_resources(context);
    } else {
        context->d3d_swap_chain->Present1(vsync ? 1 : 0, 0, &present_parameters);
    }

    RENDER_UNLOCK();
}

/*
 * Implemented here because we can't include the vice headers in a cpp file
 */
void vice_directx_impl_log_windows_error(const char *prefix)
{
    char *error_message;
    DWORD last_error = GetLastError();

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        last_error,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        (LPSTR)&error_message,
        0,
        NULL);

    log_error(LOG_DEFAULT, "%s: %s", prefix, error_message);

    LocalFree(error_message);
}

#endif /* #ifdef WINDOWS_COMPILE */
