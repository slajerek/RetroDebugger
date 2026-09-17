/** \file   kernalrevisionwidget.c
 * \brief   GTK3 KERNAL revision widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 * \author  Groepaz <groepaz@gmx.de>
 */

/*
 * $VICERES KernalRev   x64 x64sc
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

#include "vice_gtk3.h"
#include "types.h"
#include "c64rom.h"
#include "lib.h"
#include "resources.h"
#include "vsync.h"

#include "kernalrevisionwidget.h"


/** \brief  List of KERNAL revisions for the C64
 *
 * Taken from scr/c64/c64-resources.h
 */
static const vice_gtk3_radiogroup_entry_t revisions[] = {
    { "Revision 1",         C64_KERNAL_REV1 },
    { "Revision 2",         C64_KERNAL_REV2 },
    { "Revision 3",         C64_KERNAL_REV3 },
    { "SX-64",              C64_KERNAL_SX64 },
    { "C64 GS",             C64_KERNAL_GS64 },
    { "PET64/Educator64",   C64_KERNAL_4064 },
    { "Japanese",           C64_KERNAL_JAP },
    { NULL,                 -1 }
};


/** \brief  Optional extra callback function
 */
static void (*widget_callback)(int) = NULL;


/** \brief  Look up index of revision ID \a rev
 *
 * \param[in]   rev     revision ID
 *
 * \return  index in `revisions` array or -1 when not found
 */
static int get_revision_index(int rev)
{
    return vice_gtk3_radiogroup_get_list_index(revisions, rev);
}

/** \brief  Handler for the "toggled" event of the revision radio buttons
 *
 * \param[in]   widget      radio button triggering the event
 * \param[in]   user_data   revision ID (int)
 */
static void on_revision_toggled(GtkWidget *widget, gpointer user_data)
{
    int rev = GPOINTER_TO_INT(user_data);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        resources_set_int("KernalRev", rev);
        if (widget_callback != NULL) {
            widget_callback(rev);
        }

    }
}


/** \brief  Create KERNAL revision widget (c64/c64sc/c64dtv only)
 *
 * \return  GtkGrid
 */
GtkWidget *kernal_revision_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *radio;
    GtkWidget *last = NULL;
    GSList    *group = NULL;
    int        rev = 0;
    int        index;
    int        i;

    resources_get_int("KernalRev", &rev);
    index = get_revision_index(rev);

    grid = vice_gtk3_grid_new_spaced_with_label(16, 0, "KERNAL revision", 1);
    vice_gtk3_grid_set_title_margin(grid, 8);

    /* 'unknown' radio button (only used when using a custom KERNAL, cannot
     * be selected through the UI, only set through code */
    radio = gtk_radio_button_new_with_label(group, "Unknown");
    gtk_widget_set_margin_start(radio, 8);
    gtk_widget_set_sensitive(radio, FALSE);
    gtk_grid_attach(GTK_GRID(grid), radio, 0, 1, 1, 1);

    last = radio;
    for (i = 0; revisions[i].name != NULL; i++) {
        radio = gtk_radio_button_new_with_label(group, revisions[i].name);
        gtk_widget_set_margin_start(radio, 8);
        gtk_radio_button_join_group(GTK_RADIO_BUTTON(radio),
                                    GTK_RADIO_BUTTON(last));

        /* check if we got the right revision */
        if (index == i) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
        }

        g_signal_connect(radio,
                         "toggled",
                         G_CALLBACK(on_revision_toggled),
                         GINT_TO_POINTER(revisions[i].id));
        gtk_grid_attach(GTK_GRID(grid), radio, 0, i + 2, 1, 1);
        last = radio;
    }

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Update KERNAL revision widget
 *
 * \param[in,out]   widget      KERNAL revision widget
 * \param[in]       revision    new revision (\see src/c64/c64-resources.h)
 */
void kernal_revision_widget_update(GtkWidget *widget, int revision)
{
    int old_rev;

    resources_get_int("KernalRev", &old_rev);
    if (old_rev != revision) {
        GtkWidget *radio;
        int index = get_revision_index(revision);
        /* when index == -1 the 'Unknown' radio button is activated */
        radio = gtk_grid_get_child_at(GTK_GRID(widget), 0, index + 2);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
    }
}


/** \brief  Synchronize the KERNAL widget with its current value
 *
 * \param[in,out]   widget  KERNAL version widget
 */
void kernal_revision_widget_sync(GtkWidget *widget)
{
    int revision, index;
    GtkWidget *radio;

    resources_get_int("KernalRev", &revision);
    index = get_revision_index(revision);
    /* when index == -1 the 'Unknown' radio button is activated */
    radio = gtk_grid_get_child_at(GTK_GRID(widget), 0, index + 2);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
}


/** \brief  Add extra callback
 *
 * \param[in]   callback    extra callback for widget changes
 */
void kernal_revision_widget_add_callback(void (*callback)(int))
{
    widget_callback = callback;
}
