/** \file   opengl_renderer_macos.m
 *
 * \author  David Hogan <david.q.hogan@gmail.com>
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

#import "vice.h"

#import "opengl_renderer.h"

#import <Cocoa/Cocoa.h>
#import <stdbool.h>

#import <gtk/gtk.h>
#import <gdk/gdkquartz.h>

#import "render_queue.h"

#define CANVAS_LOCK() pthread_mutex_lock(context->canvas_lock_ptr)
#define CANVAS_UNLOCK() pthread_mutex_unlock(context->canvas_lock_ptr)
#define RENDER_LOCK() pthread_mutex_lock(&context->render_lock)
#define RENDER_UNLOCK() pthread_mutex_unlock(&context->render_lock)

/* For some reason this isn't in the GDK quartz headers */
NSView *gdk_quartz_window_get_nsview(GdkWindow *window);

@interface ViceOpenGLView : NSOpenGLView
{
@public
    vice_opengl_renderer_context_t *context;
}

- (id)initWithFrame: (NSRect)frameRect context: (vice_opengl_renderer_context_t *)context;

@end

@implementation ViceOpenGLView

- (id)initWithFrame: (NSRect)frameRect context: (vice_opengl_renderer_context_t *)_context
{
    self = [super initWithFrame: frameRect];

    if (self == nil) {
        return nil;
    }

    self->context = _context;

    /* Request OpenGL 3.2 */
    NSOpenGLPixelFormatAttribute pixel_format_attributes[] = { NSOpenGLPFADoubleBuffer, NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core, 0 };
    NSOpenGLPixelFormat *pixel_format = [[NSOpenGLPixelFormat alloc] initWithAttributes: pixel_format_attributes];
    NSOpenGLContext *opengl_context = [[NSOpenGLContext alloc] initWithFormat: pixel_format shareContext: nil];

    [self setWantsBestResolutionOpenGLSurface: YES];
    [self setPixelFormat: pixel_format];
    [self setOpenGLContext: opengl_context];

    [pixel_format release];
    [opengl_context release];

    return self;
}

- (void)update
{
    CANVAS_LOCK();
    /* Disable rendering until any pending resize is handled */
    context->render_skip = true;
    CANVAS_UNLOCK();

    RENDER_LOCK();
    [super update];
    RENDER_UNLOCK();

    CANVAS_LOCK();
    /* glViewport co-ordinates use the backing layer resolution, which can change on drag between screens */
    NSSize backing_layer_size = [self convertSizeToBacking: CGSizeMake(context->native_view_width, context->native_view_height)];

    context->gl_backing_layer_width = backing_layer_size.width;
    context->gl_backing_layer_height = backing_layer_size.height;

    /* Re-enable rendering */
    context->render_skip = false;
    CANVAS_UNLOCK();
}

@end

/**/

void vice_opengl_renderer_make_current(vice_opengl_renderer_context_t *context)
{
    ViceOpenGLView *opengl_view = context->native_view;

    [[opengl_view openGLContext] makeCurrentContext];
}

void vice_opengl_renderer_set_viewport(vice_opengl_renderer_context_t *context)
{
    /* Appears to be handled automatically */
}

void vice_opengl_renderer_set_vsync(vice_opengl_renderer_context_t *context, bool enable_vsync)
{
    ViceOpenGLView *opengl_view = context->native_view;
    GLint gl_int = enable_vsync ? 1 : 0;

    [[opengl_view openGLContext] setValues: &gl_int forParameter: NSOpenGLCPSwapInterval];
}

void vice_opengl_renderer_present_backbuffer(vice_opengl_renderer_context_t *context)
{
    ViceOpenGLView *opengl_view = context->native_view;

    [[opengl_view openGLContext] flushBuffer];
}

void vice_opengl_renderer_clear_current(vice_opengl_renderer_context_t *context)
{
    [NSOpenGLContext clearCurrentContext];
}

/**/

static NSRect getLayout(NSView *window_nsview, vice_opengl_renderer_context_t *context)
{
    NSRect layout;

    layout.origin.x     = context->native_view_x;
    layout.size.width   = context->native_view_width;
    layout.size.height  = context->native_view_height;

    /*
     * It seems that GTK 3.24.35, they un-flipped the native macOS Y co-ordinate system.
     */

    if ([window_nsview isFlipped]) {
        layout.origin.y = context->native_view_y;
    } else {
        layout.origin.y = [window_nsview frame].size.height - context->native_view_y - context->native_view_height;
    }

    return layout;
}

void vice_opengl_renderer_create_child_view(GtkWidget *widget, vice_opengl_renderer_context_t *context)
{
    NSView *window_nsview = gdk_quartz_window_get_nsview(gtk_widget_get_window(widget));
    ViceOpenGLView *opengl_view = [[ViceOpenGLView alloc] initWithFrame: getLayout(window_nsview, context)
                                                                context: context];

    context->native_view = opengl_view;

    /* Add the openglk view as a child of the gdk window */
    [window_nsview addSubview: opengl_view];

    vice_opengl_renderer_make_current(context);

    /* make sure OpenGL extension pointers are loaded */
    glewInit();

    vice_opengl_renderer_clear_current(context);
}

void vice_opengl_renderer_resize_child_view(GtkWidget *widget, vice_opengl_renderer_context_t *context)
{
    ViceOpenGLView *opengl_view = (ViceOpenGLView *)context->native_view;
    NSView *window_nsview;

    if (!opengl_view)
        return;

    window_nsview = gdk_quartz_window_get_nsview(gtk_widget_get_window(widget));
    [opengl_view setFrame: getLayout(window_nsview, context)];
}

void vice_opengl_renderer_destroy_child_view(vice_opengl_renderer_context_t *context)
{
    ViceOpenGLView *opengl_view = (ViceOpenGLView *)context->native_view;

    [opengl_view removeFromSuperview]; /* implicit release */
    context->native_view = NULL;
}
