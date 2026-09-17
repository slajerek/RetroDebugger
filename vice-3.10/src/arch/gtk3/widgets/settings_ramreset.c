/** \file   settings_ramreset.c
 * \brief   Widget to control the RAM reset pattern settings
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 * \author  groepaz <groepaz@gmx.net>
 */

/*
 * $VICERES RAMInitStartValue           all
 * $VICERES RAMInitValueInvert          all
 * $VICERES RAMInitPatternInvert        all
 * $VICERES RAMInitValueOffset          all
 * $VICERES RAMInitPatternInvertValue   all
 * $VICERES RAMInitStartRandom          all
 * $VICERES RAMInitRepeatRandom         all
 * $VICERES RAMInitRandomChance         all
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 `*
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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vice_gtk3.h"
#include "ram.h"
#include "resources.h"
#include "lib.h"

#include "settings_ramreset.h"


/** \brief  Number of bytes to show for the preview
 */
#define PREVIEWPATTERNBYTES 0x10000

/** \brief  Size of the text buffer for the preview
 */
#define PREVIEWTEXTBYTES    (PREVIEWPATTERNBYTES * 4)

/** \brief  CSS for the preview of the pattern
 *
 * Since Gtk's CSS doesn't allow using colors from the current theme, we make
 * the widget green text on black, like old terminals.
 * That avoids making the widget look odd with themes other than Adwaita.
 */
#define PREVIEW_CSS \
    "text {\n" \
    "    font-family: \"Monospace\";\n" \
    "    background-color: black;\n" \
    "    color: limegreen;\n" \
    "}\n"

/** \brief  List of powers of two used for the widgets
 *
 * Yes, this looks silly, but allows me to use vice-gtk3 widgets.
 */
static const vice_gtk3_combo_entry_int_t powers_of_two[] = {
    { "0 bytes",         0 },
    { "1 byte",          1 },
    { "2 bytes",         2 },
    { "4 bytes",         4 },
    { "8 bytes",         8 },
    { "16 bytes",       16 },
    { "32 bytes",       32 },
    { "64 bytes",       64 },
    { "128 bytes",     128 },
    { "256 bytes",     256 },
    { "512 bytes",     512 },
    { "1024 bytes",   1024 },
    { "2048 bytes",   2048 },
    { "4096 bytes",   4096 },
    { "8192 bytes",   8192 },
    { "16384 bytes", 16384 },
    { "32768 bytes", 32768 },
    { NULL,             -1 }
};


/** \brief  Buffer used to print the raw text of the hexdump
 */
static char *printbuffer = NULL;


/** \brief  Handler for the 'value-changed' event of the widgets in this dialog
 *
 * Updates the preview widget.
 *
 * \param[in]   widget  widget triggering the event (unused)
 * \param[in]   data    label for the preview
 */
static void on_value_changed(GtkWidget *widget, gpointer data)
{
    GtkTextBuffer *buffer;
    GtkTextIter start;
    GtkTextIter end;

    /* FIXME: right now we allocate the buffer once on first use and never
              free/release it */
    if (printbuffer == NULL) {
        printbuffer = lib_malloc(PREVIEWTEXTBYTES);
    }
    ram_init_print_pattern(printbuffer, PREVIEWPATTERNBYTES, "\n");

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data));
    gtk_text_buffer_set_text(buffer, "", -1);
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gtk_text_buffer_insert_with_tags_by_name(buffer,
                                             &end,
                                             printbuffer,
                                             -1,
                                             "hexdump",
                                             NULL);
}

/** \brief  Create textview for the RAM init preview hexdump
 *
 * We use a textview here and not a label since a textview communicates with
 * its scrolled winded to determine what to render and what not. We used to
 * render inside a label, which had the advantage of keeping the scrolled
 * window at the position the user had scrolled to when updating the contents.
 * The downside is that the view now scrolls back to the top when changing
 * contents. But keep in mind it took 5-8 seconds on a Win10 VM with 2 cores
 * at 3GHz to render, making the UI unresponsive during this time.
 *
 * \return  GtkTextView
 */
static GtkWidget *create_preview_widget(void)
{
    GtkWidget *view_widget;
    GtkTextBuffer *buffer;

    view_widget = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view_widget), FALSE);
    gtk_text_view_set_input_hints(GTK_TEXT_VIEW(view_widget),
                                  GTK_INPUT_HINT_NO_EMOJI);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view_widget), GTK_WRAP_WORD_CHAR);
    gtk_widget_set_can_focus(view_widget, FALSE);

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view_widget));
    gtk_text_buffer_create_tag(buffer,
                               "hexdump",
                               "family", "monospace",
                               "left-margin", 4,
                               "right-margin", 4,
                               "background", "black",
                               "foreground", "limegreen",
                               NULL);
    return view_widget;
}


/** \brief  Create widget to control RAM init settings
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_ramreset_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *start_value_widget;
    GtkWidget *value_invert_widget;
    GtkWidget *value_offset_widget;
    GtkWidget *pattern_invert_widget;
    GtkWidget *pattern_invert_value_widget;
    GtkWidget *start_random_widget;
    GtkWidget *repeat_random_widget;
    GtkWidget *chance_random_widget;
    GtkWidget *scrolled;
    GtkWidget *view;

    grid = vice_gtk3_grid_new_spaced_with_label(8, 0, "RAM reset pattern", 2);

    label = gtk_label_new("Value of first byte");
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    start_value_widget = vice_gtk3_resource_spin_int_new(
            "RAMInitStartValue", 0, 255, 1);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), start_value_widget, 1, 1, 1, 1);

    label = gtk_label_new("First byte offset");
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    value_offset_widget = vice_gtk3_resource_combo_int_new(
            "RAMInitValueOffset", powers_of_two);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), value_offset_widget, 1, 2, 1, 1);

    label = gtk_label_new("Invert first byte every");
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    value_invert_widget = vice_gtk3_resource_combo_int_new(
            "RAMInitValueInvert", powers_of_two);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), value_invert_widget, 1, 3, 1, 1);

    label = gtk_label_new("Value of second byte");
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    pattern_invert_value_widget = vice_gtk3_resource_spin_int_new(
            "RAMInitPatternInvertValue", 0, 255, 1);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), pattern_invert_value_widget, 1, 4, 1, 1);

    label = gtk_label_new("Invert with second byte every");
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    pattern_invert_widget = vice_gtk3_resource_combo_int_new(
            "RAMInitPatternInvert", powers_of_two);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), pattern_invert_widget, 1, 5, 1, 1);

    label = gtk_label_new("Length of random pattern");
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    start_random_widget = vice_gtk3_resource_combo_int_new(
            "RAMInitStartRandom", powers_of_two);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), start_random_widget, 1, 6, 1, 1);

    label = gtk_label_new("Repeat random pattern every");
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    repeat_random_widget = vice_gtk3_resource_combo_int_new(
            "RAMInitRepeatRandom", powers_of_two);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 7, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), repeat_random_widget, 1, 7, 1, 1);

    label = gtk_label_new("Global random chance");
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
#if 0
    chance_random_widget = vice_gtk3_resource_spin_int_new(
            "RAMInitRandomChance", 0, RAM_INIT_RANDOM_CHANCE_MAX, 1);
#else
    chance_random_widget = vice_gtk3_resource_spin_custom_new("RAMInitRandomChance",
                                                       0, RAM_INIT_RANDOM_CHANCE_MAX,
                                                       0.0, 100.0, 0.1,
                                                       "%3.2f%%");
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(chance_random_widget), 2);
#endif
    gtk_grid_attach(GTK_GRID(grid), label, 0, 8, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), chance_random_widget, 1, 8, 1, 1);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Preview:</b>");
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 9, 2, 1);

    /* Create the preview using a label
     *
     * Using a GtkTextView failed due to updating the buffer triggering the
     * scrolled window to scroll back to the top. I spent a few hours trying
     * to get it working, using all sort of trickery and the gtk devs on #gtk
     * also couldn't help me out.
     *
     * UPDATE:
     * Switched back to textview, rendering in a label is horrendously slow,
     * see the comments for create_preview_widget().
     */
#if 0
    view = gtk_label_new(NULL);
#endif
    view = create_preview_widget();
    vice_gtk3_css_add(view, PREVIEW_CSS);

    /* trigger setting the preview text */
    on_value_changed(NULL, view);

    scrolled = gtk_scrolled_window_new(NULL, NULL);
    /* TODO:    Look into setting the size based on the contents/font size
     *          --compyx
     */
    gtk_widget_set_size_request(scrolled, 512, 160);
    gtk_container_add(GTK_CONTAINER(scrolled), view);
    gtk_widget_set_margin_start(scrolled, 8);
    gtk_grid_attach(GTK_GRID(grid), scrolled, 0, 10, 2, 1);

    g_signal_connect(start_value_widget,
                     "value-changed",
                     G_CALLBACK(on_value_changed),
                     view);
    g_signal_connect(value_offset_widget,
                     "changed",
                     G_CALLBACK(on_value_changed),
                     view);
    g_signal_connect(value_invert_widget,
                     "changed",
                     G_CALLBACK(on_value_changed),
                     view);
    g_signal_connect(pattern_invert_widget,
                     "changed",
                     G_CALLBACK(on_value_changed),
                     view);
    g_signal_connect(pattern_invert_value_widget,
                     "value-changed",
                     G_CALLBACK(on_value_changed),
                     view);
    g_signal_connect(start_random_widget,
                     "changed",
                     G_CALLBACK(on_value_changed),
                     view);
    g_signal_connect(repeat_random_widget,
                     "changed",
                     G_CALLBACK(on_value_changed),
                     view);
    g_signal_connect(chance_random_widget,
                     "value-changed",
                     G_CALLBACK(on_value_changed),
                     view);

    gtk_widget_show_all(grid);
    return grid;
}
