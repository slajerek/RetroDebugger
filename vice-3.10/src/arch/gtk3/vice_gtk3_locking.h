/** \file   vice_gtk3_locking.h
 * \brief   GTK3 signal handling helper functions
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
 */

#ifndef VICE_VICE_GTK3_LOCKING_H
#define VICE_VICE_GTK3_LOCKING_H

#include "vice.h"
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

gulong vice_locking_g_signal_connect(
    gpointer instance,
    const gchar *detailed_signal,
    GCallback c_handler,
    gpointer data,
    const char *signal_handler_name);

gulong vice_locking_g_signal_connect_swapped(gpointer     instance,
                                             const gchar *detailed_signal,
                                             GCallback    c_handler,
                                             gpointer     data,
                                             const char  *signal_handler_name);

void vice_locking_gtk_accel_group_connect(
    GtkAccelGroup *accel_group,
    guint accel_key,
    GdkModifierType accel_mods,
    GtkAccelFlags accel_flags,
    GClosure *closure);

#ifdef __cplusplus
}
#endif

/*
 * Replace the standard g_signal_connect macro with one that calls our vice locking version.
 */
#undef g_signal_connect
#define g_signal_connect(instance, detailed_signal, callback, data) \
    vice_locking_g_signal_connect((instance), (detailed_signal), callback, data, #detailed_signal"["#callback"]")

/*
 * Replace the standard g_signal_connect_swapped macro with one that calls our vice locking version.
 */
#undef g_signal_connect_swapped
#define g_signal_connect_swapped(instance, detailed_signal, callback, data) \
    vice_locking_g_signal_connect_swapped((instance), (detailed_signal), callback, data, #detailed_signal"["#callback"]")

/*
 * g_signal_connect_unlocked is a copy of the original implementation of g_signal_connect (no locking).
 *
 * This is useful in controlled circumstances where we know the handler takes significant time to run
 * and we also know that the operations it performs are threadsafe.
 *
 * Primarily used to connect the "render" signal for the opengl_renderer.
 */
#define g_signal_connect_unlocked(instance, detailed_signal, callback, data) \
    g_signal_connect_data((instance), (detailed_signal), (G_CALLBACK(callback)), (data), NULL, (GConnectFlags) 0)


/** \brief  Normal g_signal_connect_swapped() implementation as defined in gsignal.h
 *
 * Pretty much verbatim copy of g_signal_connect_swapped in GLib's `gobject/gsignal.h`.
 */
#define g_signal_connect_swapped_unlocked(instance, detailed_signal, c_handler, data) \
    g_signal_connect_data((instance), (detailed_signal), (c_handler), (data), NULL, G_CONNECT_SWAPPED)

#endif
