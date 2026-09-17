/** \file   uidata.c
 * \brief   GTK3 binary resources handling
 *
 * Mostly a wrapper around the GResource API, which appears to be the way to
 * handle binary resources in Gtk3 applications.
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

#include "debug_gtk3.h"
#include "lib.h"
#include "log.h"
#include "sysfile.h"
#include "util.h"

#include "uidata.h"


/** \brief  Reference to the global GResource data
 */
static GResource *gresource = NULL;


/** \brief  Intialize the GResource binary blob handling
 *
 * \return  non-0 on success
 */
gboolean uidata_init(void)
{
    GError *err = NULL;
    gchar *path;
#if 0
#ifdef HAVE_DEBUG_GTK3UI
    char **files;
    int i;
#endif
#endif

    if (sysfile_locate(UIDATA_GRESOURCE_FILE, "common", &path) < 0) {
        log_error(LOG_DEFAULT,
                  "failed to find resource data '%s'.",
                  UIDATA_GRESOURCE_FILE);
        return FALSE;
    }

    gresource = g_resource_load(path, &err);
    if (gresource == NULL && err != NULL) {
        log_error(LOG_DEFAULT,
                  "failed to load resource data '%s': %s.",
                  path, err->message);
        g_clear_error(&err);
        lib_free(path);
        return FALSE;
    }
    lib_free(path);
    g_resources_register(gresource);

#if 0
    /* debugging: show files in the resource blob */
#ifdef HAVE_DEBUG_GTK3UI
    files = g_resource_enumerate_children(
            gresource,
            UIDATA_ROOT_PATH,
            G_RESOURCE_LOOKUP_FLAGS_NONE,
            &err);
    if (files == NULL && err != NULL) {
        debug_gtk3("couldn't enumerate children: %s.", err->message);
        g_clear_error(&err);
        return 0;
    }
    debug_gtk3("Listing files in the GResource file:");
    for (i = 0; files[i] != NULL; i++) {
        debug_gtk3("%d: %s.", i, files[i]);
    }
#endif
#endif
    return TRUE;
}


/** \brief  Clean up GResource data
 */
void uidata_shutdown(void)
{
    if (gresource != NULL) {
        g_free(gresource);
        gresource = NULL;
    }
}


/** \brief  Get a pixbuf from the GResource blob
 *
 * \param[in]   name    virtual path to the file
 *
 * \return  GdkPixbuf or `NULL` on error
 */
GdkPixbuf *uidata_get_pixbuf(const gchar *name)
{
    GdkPixbuf *buf;
    GError *err = NULL;
    gchar *path;

    path = util_concat(UIDATA_ROOT_PATH, "/", name, NULL);
    buf = gdk_pixbuf_new_from_resource(path, &err);
    lib_free(path);
    if (err) {
        log_error(LOG_DEFAULT, "Failed to obtain pixbuf for %s, Error: %s", name, err->message);
        g_clear_error(&err);
    }
    return buf;
}


/** \brief  Get a pixbuf from the GResource blob and scale it
 *
 * \param[in]   name            path in gresource
 * \param[in]   width           width of rescaled pixbuf
 * \param[in]   height          height of rescaled pixbuf
 * \param[in]   preserve_aspect preserve aspect ratio
 *
 * \return  pixbuf or `NULL` on error
 */
GdkPixbuf *uidata_get_pixbuf_at_scale(const gchar *name,
                                      gint width,
                                      gint height,
                                      gboolean preserve_aspect)
{
    GdkPixbuf *buf;
    GError *err = NULL;
    gchar *path;

    path = util_concat(UIDATA_ROOT_PATH, "/", name, NULL);
    buf = gdk_pixbuf_new_from_resource_at_scale(path, width, height,
                                                preserve_aspect, &err);
    lib_free(path);
    if (err) {
        log_error(LOG_DEFAULT, "Failed to obtain pixbuf for %s, Error: %s", name, err->message);
        g_clear_error(&err);
    }
    return buf;
}


/** \brief  Get a bytes from the GResource blob
 *
 * \param[in]   name    path in gresource
 *
 * \return  GBytes* or `NULL` on error
 */
GBytes *uidata_get_bytes(const gchar *name)
{
    GBytes *bytes;
    GError *err = NULL;
    gchar *path;

    path = util_concat(UIDATA_ROOT_PATH, "/", name, NULL);
    bytes = g_resource_lookup_data(gresource, path,
            G_RESOURCE_LOOKUP_FLAGS_NONE, &err);
    lib_free(path);
    if (bytes == NULL) {
        log_error(LOG_DEFAULT, "failed: %s.", err->message);
        g_clear_error(&err);
    }
    return bytes;
}
