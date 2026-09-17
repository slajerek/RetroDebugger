/** \file   kbddebugwidget.c
 * \brief   GTK3 keyboard debugging widget
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

#include <string.h>
#include <gtk/gtk.h>

#include "keyboard.h"
#include "log.h"
#include "vice_gtk3.h"

#include "kbddebugwidget.h"


/** \brief  Number of lines of key event logging */
#define LINES   3

/** \brief  Buffer size for g_snprintf() calls */
#define BUFSIZE 128

/** \brief  Column indexes of the various widgets
 */
enum {
    COL_TITLE = 0,      /**< title */
    COL_KEYTYPE,        /**< type of event */
    COL_KEYVAL,         /**< raw key value */
    COL_KEYSYM,         /**< key symbol as string */
    COL_KEYMOD          /**< modifiers */
};

/** \brief  CSS for the labels with debugging text */
#define LABEL_CSS \
    "label {\n" \
    "  font-family: \"monospace\";\n" \
    "}"


/** \brief  Buffers for key type label text */
static gchar keytype_buffer[LINES][BUFSIZE];

/** \brief  Buffers for key value label text */
static gchar keyval_buffer[LINES][BUFSIZE];

/** \brief  Buffers for key symbol label text */
static gchar keysym_buffer[LINES][BUFSIZE];

/** \brief  Buffers for modifiers label text */
static gchar keymod_buffer[LINES][BUFSIZE];

/** \brief  Instance being created is the first instance
 *
 * Used to avoid updating the text buffers twice on x128 for a single event.
 *
 */
static gboolean primary_instance = TRUE;

/** \brief  CSS provider for the labels
 *
 * We only need a single instance since all labels get the same style.
 */
static GtkCssProvider *css_provider = NULL;


/** \brief  Create Gtk3 keyboard debugging widget
 *
 * \return  GtkGrid
 */
GtkWidget *kbd_debug_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    int line;

    /* only one provider is required */
    if (css_provider == NULL) {
        css_provider = vice_gtk3_css_provider_new(LABEL_CSS);
    }

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);
    gtk_widget_set_margin_start(grid, 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 1);

    label = gtk_label_new("KBD debug:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, COL_TITLE, 0, 1, 1);

    for (line = 0; line < LINES; line++) {
        label = gtk_label_new("-");
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        vice_gtk3_css_provider_add(label, css_provider);
        gtk_grid_attach(GTK_GRID(grid), label, COL_KEYTYPE, line, 1, 1);

        label = gtk_label_new("-");
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        vice_gtk3_css_provider_add(label, css_provider);
        gtk_grid_attach(GTK_GRID(grid), label, COL_KEYVAL, line, 1, 1);

        label = gtk_label_new("-");
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        vice_gtk3_css_provider_add(label, css_provider);
        gtk_grid_attach(GTK_GRID(grid), label, COL_KEYSYM, line, 1, 1);

        label = gtk_label_new("-----");
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        vice_gtk3_css_provider_add(label, css_provider);
        gtk_grid_attach(GTK_GRID(grid), label, COL_KEYMOD, line, 1, 1);
    }

    /* mark the first widget created as the primary instance so we can avoid
     * g_snprintf() calls in kbd_debug_widget_update() when we have two instances
     * in x128
     */
    g_object_set_data(G_OBJECT(grid), "PrimaryInstance", GINT_TO_POINTER(primary_instance));
    if (primary_instance) {
        primary_instance = FALSE;
    }

    return grid;
}


/** \brief  Update keyboard debug widget with \a event
 *
 * Updates the labels in \a widget with information from \a event.
 * Only if \a widget is the primary instance will the text buffers be updated
 * and scrolled. For all instances the label texts will be updated with the
 * text buffers' contents.
 *
 * \param[in]   widget  keyboard debug widget instance
 * \param[in]   event   GDK event structure
 */
void kbd_debug_widget_update(GtkWidget *widget, GdkEvent *event)
{
    GtkWidget *label;
    int line;
    gboolean primary;

    /* early exit if we are not visible */
    if (!gtk_widget_is_visible(widget)) {
        return;
    }

    /* only update text buffers when the widget is the primary instance */
    primary = (gboolean)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "PrimaryInstance"));
#if 0
    debug_gtk3("primary instance = %s.", primary ? "TRUE" : "FALSE");
#endif
    if (primary) {
        guint keyval = event->key.keyval;
        guint mods = event->key.state;
        int scancode = gdk_event_get_scancode(event);
        GdkDisplay *display = gdk_display_get_default();
        GdkKeymap *keymap = gdk_keymap_get_for_display(display);
        int capslock = gdk_keymap_get_caps_lock_state(keymap);
#if 0
        debug_gtk3("Updating text buffers.");
#endif
        for (line = 0; line < (LINES - 1); line++) {
            memcpy(keytype_buffer[line], keytype_buffer[line + 1], BUFSIZE);
            memcpy(keyval_buffer[line], keyval_buffer[line + 1], BUFSIZE);
            memcpy(keysym_buffer[line], keysym_buffer[line + 1], BUFSIZE);
            memcpy(keymod_buffer[line], keymod_buffer[line + 1], BUFSIZE);
        }

        switch(event->type) {
            case GDK_KEY_PRESS:
                /* trailing spaces are used to align the log_message() output */
                g_snprintf(keytype_buffer[LINES - 1], BUFSIZE, "press  ");
                break;
            case GDK_KEY_RELEASE:
                g_snprintf(keytype_buffer[LINES - 1], BUFSIZE, "release");
                break;
            default:
                g_snprintf(keytype_buffer[LINES - 1], BUFSIZE, "unknown");
                break;
        }

        g_snprintf(keyval_buffer[LINES - 1], BUFSIZE, "%5u/0x%04x  0x%08x",
                   keyval, keyval, (unsigned int)scancode);
        g_snprintf(keysym_buffer[LINES - 1], BUFSIZE, "%s", gdk_keyval_name(keyval));
        g_snprintf(keymod_buffer[LINES - 1], BUFSIZE, "%c%c%c %c%c%c%c%c %c%c",
                mods & GDK_SHIFT_MASK ? 'S' : '-',    /* shift (left or right) */
                mods & GDK_LOCK_MASK ? 'L' : '-',     /* shift-lock */
                mods & GDK_CONTROL_MASK ? 'C' : '-',  /* control */
                                                      /* the additional ones seem to be different on */
                                                      /* windows        vs linux                     */
                mods & GDK_MOD1_MASK ? '1' : '-',     /* alt left          alt-left                  */
                mods & GDK_MOD2_MASK ? '2' : '-',     /* alt-gr (Alt_R)    num-lock                  */
                mods & GDK_MOD3_MASK ? '3' : '-',
                mods & GDK_MOD4_MASK ? '4' : '-',     /*                   windows right             */
                mods & GDK_MOD5_MASK ? '5' : '-',     /*                   alt-gr (ISO_Level3_Shift) */
                capslock ? 'L' : '-',                 /* host keyboard shift lock */
                keyboard_get_shiftlock() ? 'L' : '-'  /* emulated keyboard shift lock */
                );

        log_message(LOG_DEFAULT, "%s %s %s %s",
                    keytype_buffer[LINES - 1],
                    keyval_buffer[LINES - 1],
                    keymod_buffer[LINES - 1],
                    keysym_buffer[LINES - 1]);
    }

    /* update instance widgets */
#if 0
    debug_gtk3("Updating label texts.");
#endif
    for (line = 0; line < LINES; line++) {
        label = gtk_grid_get_child_at(GTK_GRID(widget), COL_KEYTYPE, line);
        gtk_label_set_text(GTK_LABEL(label), keytype_buffer[line]);

        label = gtk_grid_get_child_at(GTK_GRID(widget), COL_KEYVAL, line);
        gtk_label_set_text(GTK_LABEL(label), keyval_buffer[line]);

        label = gtk_grid_get_child_at(GTK_GRID(widget), COL_KEYSYM, line);
        gtk_label_set_text(GTK_LABEL(label), keysym_buffer[line]);

        label = gtk_grid_get_child_at(GTK_GRID(widget), COL_KEYMOD, line);
        gtk_label_set_text(GTK_LABEL(label), keymod_buffer[line]);
    }
}
