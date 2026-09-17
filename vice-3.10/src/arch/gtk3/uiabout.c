/** \file   uiabout.c
 * \brief   GTK3 about dialog
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 * \author  Greg King <gregdk@users.sf.net>
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
#include <string.h>

#include "archdep.h"
#include "debug_gtk3.h"
#include "info.h"
#include "lib.h"
#ifdef USE_SVN_REVISION
# include "svnversion.h"
#endif
#include "ui.h"
#include "uiactions.h"
#include "uidata.h"
#include "version.h"
#include "vicedate.h"

#include "uiabout.h"


/** \brief  Maximum length of generated version string
 */
#define VERSION_STRING_MAX 8192


/** \brief  List of current team members
 *
 * This list is allocated in create_current_team_list() and deallocated in
 * the "destroy" callback of the About dialog
 */
static char **authors;


/** \brief  Create list of current team members
 *
 * \return  heap-allocated list of strings
 */
static char **create_current_team_list(void)
{
    char **list;
    size_t i;

    /* get proper size of list (sizeof doesn't work here) */
    for (i = 0; core_team[i].name != NULL; i++) {
        /* NOP */
    }
    list = lib_malloc(sizeof *list * (i + 1));

    /* create list of current team members */
    for (i = 0; core_team[i].name != NULL; i++) {
        list[i] = core_team[i].name;
    }
    list[i] = NULL;
    return list;
}


/** \brief  Deallocate current team list
 *
 * \param[in,out]   list    list of team member names
 */
static void destroy_current_team_list(char **list)
{
    lib_free(list);
}


/** \brief  Create VICE logo
 *
 * \return  GdkPixbuf instance
 */
static GdkPixbuf *get_vice_logo(void)
{
    return uidata_get_pixbuf("vice-logo-black.svg");
}


/** \brief  Handler for the "destroy" event
 *
 * \param[in,out]   widget      widget triggering the event (unused)
 * \param[in]       user_data   data for the event (unused)
 */
static void about_destroy_callback(GtkWidget *widget, gpointer user_data)
{
    destroy_current_team_list(authors);
    ui_action_finish(ACTION_HELP_ABOUT);
}


/** \brief  Handler for the "response" event
 *
 * This handles the "response" event, which is triggered for various standard
 * buttons, although which buttons trigger this is a little unclear at the
 * moment.
 *
 * \param[in,out]   widget      widget triggering the event (the dialog)
 * \param[in]       response_id response ID
 * \param[in]       user_data   extra data (unused)
 */
static void about_response_callback(GtkWidget *widget, gint response_id,
                                    gpointer user_data)
{
    /* The GTK_RESPONSE_DELETE_EVENT is sent when the user clicks 'Close',
     * but also when the user clicks the 'X' gadget.
     */
    switch (response_id) {
        case GTK_RESPONSE_DELETE_EVENT:
            gtk_widget_destroy(widget);
            break;
        default:
            debug_gtk3("Warning: Unsupported response ID %d", response_id);
            break;
    }
}


/** \brief  Show the about dialog
 */
void ui_about_dialog_show(void)
{
    archdep_runtime_info_t runtime_info;
    char version[VERSION_STRING_MAX];
    const char *model;
    char buffer[256];
    GtkWidget *about = gtk_about_dialog_new();
    GdkPixbuf *logo = get_vice_logo();


    /* set toplevel window, Gtk doesn't like dialogs without parents */
    gtk_window_set_transient_for(GTK_WINDOW(about), ui_get_active_window());

    /* generate team members list */
    authors = create_current_team_list();

    /* set window title */
    gtk_window_set_title(GTK_WINDOW(about), "About VICE");

    /* set version string */
#ifdef USE_SVN_REVISION
    g_snprintf(version, sizeof(version),
            "%s r%s\n(GTK3 %d.%d.%d, GLib %d.%d.%d, Cairo %s, Pango %s)",
            VERSION, VICE_SVN_REV_STRING,
            GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION,
            GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION,
            cairo_version_string(),
            pango_version_string());
#else
    g_snprintf(version, sizeof(version),
            "%s\n(GTK3 %d.%d.%d, GLib %d.%d.%d, Cairo %s, Pango %s)",
            VERSION,
            GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION,
            GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION,
            cairo_version_string(),
            pango_version_string());
#endif
#undef VICE_VERSION_STRING
    if (archdep_get_runtime_info(&runtime_info)) {
        size_t v = strlen(version);
        g_snprintf(version + v, VERSION_STRING_MAX - v - 1UL,
                "\n\n%s %s\n"
                "%s\n"
                "%s",
                runtime_info.os_name,
                runtime_info.os_release,
                runtime_info.os_version,
                runtime_info.machine);
    }
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), version);

    /* Describe the program */
    switch (machine_class) {
        default:                    /* fall through */ /* fix warning */
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_VSID:
            model = "Commodore 64";
            break;
        case VICE_MACHINE_C64DTV:
            model = "C64 DTV";
            break;
        case VICE_MACHINE_C128:
            model = "Commodore 128";
            break;
        case VICE_MACHINE_SCPU64:
            model = "Commodore 64 with SuperCPU";
            break;
        case VICE_MACHINE_VIC20:
            model = "Commodore VIC-20";
            break;
        case VICE_MACHINE_PLUS4:
            model = "Commodore 16/116 and Plus/4";
            break;
        case VICE_MACHINE_PET:
            model = "Commodore PET and SuperPET";
            break;
        case VICE_MACHINE_CBM5x0:
            model = "Commodore CBM-II 510 (P500)";
            break;
        case VICE_MACHINE_CBM6x0:
            model = "Commodore CBM-II 6x0 and 7x0";
            break;
    }
    g_snprintf(buffer, sizeof(buffer), "The %s Emulator", model);
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about), buffer);

    /* set license */
    gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(about), GTK_LICENSE_GPL_2_0);
    /* set website link and title */
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about),
                                 "http://vice-emu.sourceforge.net/");
    gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about),
                                       "http://vice-emu.sourceforge.net/");
    /* set list of current team members */
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), (const gchar **)authors);
    /* set copyright string */
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about),
            "Copyright 1996-" VICEDATE_YEAR_STR ", VICE team");

    /* set logo */
    if (logo != NULL) {
        gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about), logo);
        g_object_unref(logo);
    }

    /*
     * hook up event handlers
     */

    /* destroy callback, called when the dialog is closed through the 'X',
     * but NOT when clicking 'Close' */
    g_signal_connect_unlocked(about,
                              "destroy",
                              G_CALLBACK(about_destroy_callback),
                              NULL);

    /* set up a generic handler for various buttons, this makes sure the
     * 'Close' button is handled properly */
    g_signal_connect_unlocked(about,
                              "response",
                              G_CALLBACK(about_response_callback),
                              NULL);

    /* make the about dialog modal */
    gtk_window_set_modal(GTK_WINDOW(about), TRUE);

    /* ... and show the dialog finally */
    gtk_widget_show(about);
}
