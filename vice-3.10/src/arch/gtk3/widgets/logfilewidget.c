/** \file   logfilewidget.c
 * \brief   Widget to set the log file
 *
 * /author  Bas Wassink <b.wassink@ziggo.nl>
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

/*
 * Resources manipulated by this widget:
 *
 * $VICERES LogFileName -vsid
 */

#include "vice.h"
#include <gtk/gtk.h>

#include "archdep.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "ui.h"
#include "vice_gtk3.h"

#include "logfilewidget.h"

/** \brief  Button to open a file manager in the logfile directory
 */
static GtkWidget *launcher = NULL;

/** \brief  "Log to file" checkbutton widget
 */
static GtkWidget  *file_check;

/** \brief  Handler for the 'changed' event of the entry
 *
 * Set \a check state depending on whether \a entry contains "-".
 *
 * \param[in]   entry   GtkEntry containing the LogFileName contents
 * \param[in]   check   GtkCheckButton for "log to stdout"
 */
static void on_logfile_entry_changed(GtkWidget *entry, gpointer check)
{
    const char *text    = gtk_entry_get_text(GTK_ENTRY(entry));
    bool        enabled = g_strcmp0(text, "-") == 0;

    if (enabled) {
        /* enable "log to stdout", disable "log to file" when "-" was entered as name */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), enabled);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(file_check), enabled ^ 1);
    }
}

/** \brief  Open directory containing the log file
 *
 * \param[in]   button  button (unused)
 * \param[in]   data    extra event data (unused)
 */
static void on_launcher_clicked(GtkWidget *button, gpointer data)
{
    const char *logfile = NULL;
    char       *path;
    char       *dir;
    char       *uri;
    GError     *error = NULL;
#ifdef WINDOWS_COMPILE
    char       *child_argv[3] = { "explorer.exe", NULL, NULL };
#endif

    resources_get_string("LogFileName", &logfile);
    if (g_strcmp0(logfile, "-") == 0) {
        /* stdout isn't in any directory ;) */
        return;
    }

    if (logfile == NULL || *logfile == '\0') {
        path = archdep_default_logfile();
    } else {
        path = lib_strdup(logfile);
    }

    /* get dirname */
    dir = g_path_get_dirname(path);
    lib_free(path);
    uri = g_strdup_printf("file://%s/", dir);
#if 0
    g_print("%s(): logfile location = %s\n", __func__, uri);
#endif

#ifdef WINDOWS_COMPILE
    /* Set directory as argument to explorer.exe, *DO NOT QUOTE*, GIO handles
       the quoting and unquoting mess for Windows: adding quotes around the
       directory actually results in explorer opening "Documents" for some
       obscene reason.
    */
    child_argv[1] = dir;
    if (!g_spawn_async(NULL,                /* working directory */
                       child_argv,          /* argv[], first element is executable */
                       NULL,                /* environment */
                       G_SPAWN_SEARCH_PATH, /* search PATH for executable */
                       NULL,                /* child setup function */
                       NULL,                /* user data for child setup */
                       NULL,                /* child process ID */
                       &error)) {
        log_error(LOG_DEFAULT,
                  "failed to launch file manager to view directory: %s",
                  error->message);
        g_error_free(error);
    }
#else
    if (!gtk_show_uri_on_window(ui_get_active_window(),
                                uri,
                                GDK_CURRENT_TIME,
                                &error)) {
        log_error(LOG_DEFAULT,
                  "failed to lauch file manager to view directory: %s",
                  error->message);
        g_error_free(error);
    }
#endif
    g_free(dir);
    g_free(uri);
}

/** \brief  Create left-aligned label with Pango markup
 *
 * \param[in]   text    label text, can contain Pango markup
 *
 * \return  GtkLabel
 */
static GtkWidget *label_helper(const char *text)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}

static const vice_gtk3_combo_entry_int_t log_limits[] = {
    { "Silent",  LOG_LIMIT_SILENT },
    { "Standard",  LOG_LIMIT_STANDARD },
    { "Verbose",  LOG_LIMIT_VERBOSE },
    { "Debug",  LOG_LIMIT_DEBUG },
    { NULL,       -1 }
};

/** \brief  Create widget to set the LogFileName resource
 *
 * \return  GtkGrid
 */
GtkWidget *logfile_widget_create(void)
{
    GtkWidget  *grid;
    GtkWidget  *header_label;
    GtkWidget  *logfile_chooser;
    GtkWidget  *stdout_check;
    GtkWidget  *mon_check;
    GtkWidget  *col_check;
    GtkWidget  *button_box;
    GtkWidget  *limits_widget;
    GtkWidget  *limits_label;
    const char *logfilename = NULL;
    char       *logfile_default;
    int         row = 0;

    resources_get_string("LogFileName", &logfilename);

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    /* header */
    header_label = label_helper("<b>VICE log</b>");
    gtk_grid_attach(GTK_GRID(grid), header_label, 0, row, 2, 1);
    row++;

    file_check = vice_gtk3_resource_check_button_new("LogToFile", "Log to file");
    gtk_grid_attach(GTK_GRID(grid), file_check, 0, row, 1, 1);

    /* logfile resource filechooser */
    logfile_default = archdep_default_logfile();
    logfile_chooser = vice_gtk3_resource_filechooser_new("LogFileName",
                                                         GTK_FILE_CHOOSER_ACTION_SAVE);
    vice_gtk3_resource_filechooser_set_custom_title(logfile_chooser,
                                                    "Select or create log file");
    if (logfilename == NULL || *logfilename == '\0') {
        vice_gtk3_resource_filechooser_set_directory(logfile_chooser,
                                                     archdep_user_state_path());
    }
    /* use the default log file path as a placeholder text */
    gtk_entry_set_placeholder_text(GTK_ENTRY(logfile_chooser), logfile_default);
    lib_free(logfile_default);
    gtk_grid_attach(GTK_GRID(grid), logfile_chooser, 1, row, 3, 1);
    row++;


    limits_label = label_helper("Log limit");
    gtk_widget_set_halign(limits_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), limits_label, 1, row, 1, 1);

    limits_widget = vice_gtk3_resource_combo_int_new("LogLimit",
                                                              log_limits);
    gtk_grid_attach(GTK_GRID(grid), limits_widget, 2, row, 1, 1);

    /* "Open directory" button in button box, next to the stdout check button */
    button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_set_spacing(GTK_BOX(button_box), 8);
    launcher = gtk_button_new_with_label("Open directory containing log file");
    gtk_box_pack_start(GTK_BOX(button_box), launcher, FALSE, FALSE, 0);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), button_box, 3, row, 1, 1);

    row++;

    /* check button for "log to stdout" */
    stdout_check = vice_gtk3_resource_check_button_new("LogToStdout", "Log to stdout");
    logfilename = gtk_entry_get_text(GTK_ENTRY(logfile_chooser));
    if (g_strcmp0(logfilename, "-") == 0) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stdout_check), TRUE);
    }
    gtk_grid_attach(GTK_GRID(grid), stdout_check, 0, row, 1, 1);

    /* check button for "log to monitor" */
    mon_check = vice_gtk3_resource_check_button_new("LogToMonitor", "Log to monitor");
    gtk_grid_attach(GTK_GRID(grid), mon_check, 1, row, 1, 1);

    /* check button for "Colorize Log" */
    col_check = vice_gtk3_resource_check_button_new("LogColorize", "Colorize Log");
    gtk_grid_attach(GTK_GRID(grid), col_check, 2, row, 1, 1);


    g_signal_connect(G_OBJECT(launcher),
                     "clicked",
                     G_CALLBACK(on_launcher_clicked),
                     NULL);
    g_signal_connect_unlocked(G_OBJECT(logfile_chooser),
                              "changed",
                              G_CALLBACK(on_logfile_entry_changed),
                              (gpointer)stdout_check);

    gtk_widget_show_all(grid);
    return grid;
}
