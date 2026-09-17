/** \file   opengl_renderer_unix.c
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

#include "vice.h"

#include "archdep.h"

#include <string.h>

#include "opengl_renderer.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "log.h"
#include "render_queue.h"

/* for testing, undefine to use legacy renderer even with 3.2+ */
/*#define FORCE_LEGACY_RENDERER*/

extern log_t opengl_log;

#define CANVAS_LOCK() pthread_mutex_lock(context->canvas_lock_ptr)
#define CANVAS_UNLOCK() pthread_mutex_unlock(context->canvas_lock_ptr)
#define RENDER_LOCK() pthread_mutex_lock(&context->render_lock)
#define RENDER_UNLOCK() pthread_mutex_unlock(&context->render_lock)

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, Bool, const int *);

static bool isExtensionSupported(const char *extList, const char *extension);

/**/

void vice_opengl_renderer_make_current(vice_opengl_renderer_context_t *context)
{
    glXMakeCurrent(context->x_display, context->x_overlay_window, context->gl_context);
}

void vice_opengl_renderer_set_viewport(vice_opengl_renderer_context_t *context)
{
    glViewport(0, 0, context->gl_backing_layer_width, context->gl_backing_layer_height);
}

void vice_opengl_renderer_present_backbuffer(vice_opengl_renderer_context_t *context)
{
    glXSwapBuffers(context->x_display, context->x_overlay_window);
}

void vice_opengl_renderer_clear_current(vice_opengl_renderer_context_t *context)
{
    glXMakeCurrent(context->x_display, None, NULL);
}

static int catch_x_error(Display *display, XErrorEvent *event)
{
    return 0;
}

/**/

void vice_opengl_renderer_create_child_view(GtkWidget *widget, vice_opengl_renderer_context_t *context)
{
    /*
     * OpenGL context initialisation adapted from
     * https://www.khronos.org/opengl/wiki/Tutorial:_OpenGL_3.0_Context_Creation_(GLX)
     */

    /* we need to create a child window compatible with the type of OpenGL stuff we want */
    static int visual_attribs[] =
        {
            GLX_X_RENDERABLE    , True,
            GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
            GLX_RENDER_TYPE     , GLX_RGBA_BIT,
            GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
            GLX_RED_SIZE        , 8,
            GLX_GREEN_SIZE      , 8,
            GLX_BLUE_SIZE       , 8,
            GLX_ALPHA_SIZE      , 8,
            GLX_DOUBLEBUFFER    , True,
            None
        };

    int glx_major;
    int glx_minor;
    int fbcount;
    const char *glx_extensions;
    int major = -1;
    int minor = -1;

    /* use the same connection to the x server that GDK is using */
    context->x_display = GDK_WINDOW_XDISPLAY(gtk_widget_get_window(widget));

    if (!glXQueryVersion(context->x_display, &glx_major, &glx_minor)) {
        log_error(opengl_log, "Failed to get GLX version\n");
        archdep_vice_exit(1);
    }

    log_verbose(opengl_log, "GLX version: %d.%d", glx_major, glx_minor);

    /* FBConfigs were added in GLX version 1.3. */
    if (glx_major < 1 || (glx_major == 1 && glx_minor < 3)) {
        log_error(opengl_log, "At least GLX 1.3 is required\n");
        archdep_vice_exit(1);
    }

    log_verbose(opengl_log, "Getting matching framebuffer configs" );

    PFNGLXCHOOSEFBCONFIGPROC vice_glXChooseFBConfig = (PFNGLXCHOOSEFBCONFIGPROC)glXGetProcAddressARB((const GLubyte *)"glXChooseFBConfig");
    GLXFBConfig *framebuffer_configs = vice_glXChooseFBConfig(context->x_display, DefaultScreen(context->x_display), visual_attribs, &fbcount);
    if (!framebuffer_configs) {
        log_error(opengl_log, "Failed to retrieve a framebuffer config\n");
        archdep_vice_exit(1);
    }
    log_verbose(opengl_log, "Found %d matching FB configs.", fbcount);

    /* Just pick the first one I guess. */
    GLXFBConfig framebuffer_config = framebuffer_configs[0];
    XFree(framebuffer_configs);

    /* Get a visual */
    PFNGLXGETVISUALFROMFBCONFIGPROC vice_glXGetVisualFromFBConfig = (PFNGLXGETVISUALFROMFBCONFIGPROC)glXGetProcAddressARB((const GLubyte *)"glXGetVisualFromFBConfig");
    XVisualInfo *x_visual = vice_glXGetVisualFromFBConfig(context->x_display, framebuffer_config);

    XSetWindowAttributes x_set_window_attributes;

    x_set_window_attributes.colormap =
        XCreateColormap(
            context->x_display,
            GDK_WINDOW_XID(gtk_widget_get_window(widget)),
            x_visual->visual,
            AllocNone);
    x_set_window_attributes.background_pixmap = None ;
    x_set_window_attributes.border_pixel      = 0;
    x_set_window_attributes.event_mask        = 0;

    context->x_overlay_window =
        XCreateWindow(
            context->x_display,
            GDK_WINDOW_XID(gtk_widget_get_window(widget)),
            context->native_view_x, context->native_view_y, context->gl_backing_layer_width, context->gl_backing_layer_height,
            0,
            x_visual->depth,
            InputOutput,
            x_visual->visual,
            CWColormap | CWBackPixmap | CWBorderPixel | CWEventMask,
            &x_set_window_attributes);

    XMapWindow(context->x_display, context->x_overlay_window);

    /* Done with the visual info data */
    XFree(x_visual);

    /* Get the screen's GLX extension list */
    glx_extensions = glXQueryExtensionsString(context->x_display, DefaultScreen(context->x_display));

    PFNGLXCREATECONTEXTATTRIBSARBPROC vice_glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

    /*
     * Check for the GLX_ARB_create_context extension string and the function.
     * If either is not present, use GLX 1.3 context creation method.
     */
    if (!isExtensionSupported(glx_extensions, "GLX_ARB_create_context") || !vice_glXCreateContextAttribsARB) {
        /* Legacy context -- TODO, actually support using this */
        PFNGLXCREATENEWCONTEXTPROC vice_glXCreateNewContext = (PFNGLXCREATENEWCONTEXTPROC)glXGetProcAddressARB((const GLubyte *)"glXCreateNewContext");
        context->gl_context = vice_glXCreateNewContext(context->x_display, framebuffer_config, GLX_RGBA_TYPE, NULL, True);
    } else {
        XErrorHandler oldHandler;
        int context_attribs[] =
            {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
#ifdef FORCE_LEGACY_RENDERER
                GLX_CONTEXT_MINOR_VERSION_ARB, 1,
#else
                GLX_CONTEXT_MINOR_VERSION_ARB, 2,
#endif
                None
            };

        oldHandler = XSetErrorHandler(catch_x_error);

        context->gl_context = vice_glXCreateContextAttribsARB(context->x_display, framebuffer_config, NULL, True, context_attribs);

        /* Sync to ensure any errors generated are processed. */
        XSync(context->x_display, False);
        XSetErrorHandler(oldHandler);

        if (!context->gl_context) {
            log_error(opengl_log, "Failed to obtain an OpenGL 3.2 context, requesting a legacy context\n" );

            /*
             * Couldn't create GL 3.2 context.  Fall back to old-style 2.x context.
             * When a context version below 3.0 is requested, implementations will
             * return the newest context version compatible with OpenGL versions less
             * than version 3.0.
             */

            context_attribs[1] = 1;
            context_attribs[3] = 0;

            context->gl_context = vice_glXCreateContextAttribsARB(context->x_display, framebuffer_config, NULL, True, context_attribs);
        }
    }

    /* Sync to ensure any errors generated are processed. */
    XSync(context->x_display, False);

    /* make sure OpenGL extension pointers are loaded for the general renderer code */
    vice_opengl_renderer_make_current(context);
    glewInit();

#ifdef FORCE_LEGACY_RENDERER
    major = 3;
    minor = 1;
#else
    sscanf((const char *)glGetString(GL_VERSION), "%d.%d", &major, &minor);
#endif

    /* Anything less than OpenGL 3.2 will use the legacy renderer */
    context->gl_context_is_legacy = major < 3 || (major == 3 && minor < 2);

    if (log_get_limit() <= LOG_LIMIT_STANDARD) {
        log_message(opengl_log, "Obtained OpenGL %d.%d context %s %s",
            major,
            minor,
            context->gl_context_is_legacy ? "Legacy" : "",
            glXIsDirect(context->x_display, context->gl_context) ? "" : "GLX indirect"
        );
    }

    log_verbose(opengl_log, "Obtained OpenGL %d.%d context\n Vendor:   %s\n Renderer: %s\n Version:  %s\n Legacy:   %s",
        major,
        minor,
        glGetString(GL_VENDOR),
        glGetString(GL_RENDERER),
        glGetString(GL_VERSION),
        context->gl_context_is_legacy ? "yes" : "no");

    /* Not sure if an indirect context will work but lets leave some useful output for bug reports */
    if (!glXIsDirect(context->x_display, context->gl_context)) {
        log_warning(opengl_log, "Indirect GLX rendering context obtained - please let us know if this works!");
    } else {
        log_verbose(opengl_log, "Direct GLX rendering context obtained");
    }

    log_verbose(opengl_log, "Swap control support. glXSwapIntervalMESA: %d glXSwapIntervalEXT: %d glXSwapIntervalSGI: %d", !!glXSwapIntervalMESA, !!glXSwapIntervalEXT, !!glXSwapIntervalSGI);

    vice_opengl_renderer_clear_current(context);
}

void vice_opengl_renderer_set_vsync(vice_opengl_renderer_context_t *context, bool enable_vsync)
{
    static bool set_vsync_failed;

    GLint gl_int = enable_vsync ? 1 : 0;
    int result = 0;

    /* WTF opengl. */
    if (GLX_MESA_swap_control && glXSwapIntervalMESA) {
        result = glXSwapIntervalMESA(gl_int);
        if (result) {
            if (!set_vsync_failed) {
                log_error(opengl_log, "glXSwapIntervalMESA(%d) failed with error: %d (suppressing futher errors)", gl_int, result);
                set_vsync_failed = true;
            }
        } else {
            if (set_vsync_failed) {
                log_message(opengl_log, "glXSwapIntervalMESA(%d) started working again", gl_int);
                set_vsync_failed = false;
            }
        }
    } else if (GLX_EXT_swap_control && glXSwapIntervalEXT) {
        glXSwapIntervalEXT(context->x_display, glXGetCurrentDrawable(), gl_int);
    } else if (GLX_SGI_swap_control && glXSwapIntervalSGI) {
        result = glXSwapIntervalSGI(gl_int);
        if (result) {
            if (!set_vsync_failed) {
                log_error(opengl_log, "glXSwapIntervalSGI(%d) failed with error: %d (suppressing futher errors)", gl_int, result);
                set_vsync_failed = true;
            }
        } else {
            if (set_vsync_failed) {
                log_message(opengl_log, "glXSwapIntervalSGI(%d) started working again", gl_int);
                set_vsync_failed = false;
            }
        }
    }
}

void vice_opengl_renderer_resize_child_view(GtkWidget *widget, vice_opengl_renderer_context_t *context)
{
    Display *x_display;
    Window x_overlay_window;
    unsigned int gl_backing_layer_width;
    unsigned int gl_backing_layer_height;

    if (!context->x_overlay_window) {
        return;
    }

    CANVAS_LOCK();

    x_display               = context->x_display;
    x_overlay_window        = context->x_overlay_window;
    gl_backing_layer_width  = context->gl_backing_layer_width;
    gl_backing_layer_height = context->gl_backing_layer_height;

    CANVAS_UNLOCK();

    /*
     * Getting the render lock here causes a deadlock within glFinish().
     * Thankfully, it seems to be ok to call XMoveResizeWindow during a render
     * without any glitches or crashes, unlike on macOS.
     */

    XMoveResizeWindow(
        x_display,
        x_overlay_window,
        0,
        0,
        gl_backing_layer_width,
        gl_backing_layer_height);
}

void vice_opengl_renderer_destroy_child_view(vice_opengl_renderer_context_t *context)
{
    XDestroyWindow(context->x_display, context->x_overlay_window);
}

static bool isExtensionSupported(const char *extList, const char *extension)
{
    const char *start;
    const char *where, *terminator;

    /* Extension names should not have spaces. */
    where = strchr(extension, ' ');
    if (where || *extension == '\0') {
        return false;
    }

    /* It takes a bit of care to be fool-proof about parsing the
    OpenGL extensions string. Don't be fooled by sub-strings,
    etc. */
    for (start=extList;;) {
        where = strstr(start, extension);

        if (!where) {
            break;
        }

        terminator = where + strlen(extension);

        if (where == start || *(where - 1) == ' ') {
            if (*terminator == ' ' || *terminator == '\0') {
                return true;
            }
        }

        start = terminator;
    }

    return false;
}
