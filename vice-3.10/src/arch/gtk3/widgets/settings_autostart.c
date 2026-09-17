/** \file   settings_autostart.c
 * \brief   GTK3 autostart settings central widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES AutostartDelay                      -vsid
 * $VICERES AutostartDelayRandom                -visd
 * $VICERES AutostartPrgMode                    -vsid
 * $VICERES AutostartPrgDiskImage               -vsid
 * $VICERES AutostartRunWithColon               -vsid
 * $VICERES AutostartBasicLoad                  -vsid
 * $VICERES AutostartTapeBasicLoad              -vsid -xpet -xcbm2 -xcbm5x0
 * $VICERES AutostartWarp                       -vsid
 * $VICERES AutostartHandleTrueDriveEmulation   -vsid
 * $VICERES AutostartOnDoubleClick              -vsid
 * $VICERES AutostartDropMode                   -vsid
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

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "vice_gtk3.h"
#include "machine.h"
#include "resources.h"
#include "autostart.h"
#include "autostart-prg.h"
#include "uisettings.h"

#include "settings_autostart.h"


/** \brief  Autostart modes for PRG files
 */
static const vice_gtk3_radiogroup_entry_t autostart_modes[] = {
    { "Virtual filesystem", AUTOSTART_PRG_MODE_VFS },
    { "Inject into RAM",    AUTOSTART_PRG_MODE_INJECT },
    { "Copy to disk",       AUTOSTART_PRG_MODE_DISK },
    { NULL,                 -1 }
};

/** \brief  Autostart behaviour when dropping image on the emulator window */
static const vice_gtk3_radiogroup_entry_t autostart_drop_modes[] = {
    { "Attach image only",          AUTOSTART_DROP_MODE_ATTACH },
    { "Attach image and LOAD",      AUTOSTART_DROP_MODE_LOAD },
    { "Attach image, LOAD and RUN", AUTOSTART_DROP_MODE_RUN },
    { NULL,                         -1 }
};


/*
 * Layout helpers
 */

/** \brief  Add widgets to set autostart dleay-related resources
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to start adding widgets
 * \param[in]   columns columns in \a grid to use for proper widget column spans
 *
 * \return  row in \a grid for more widgets
 */
static int create_delay_layout(GtkWidget *grid, int row, int columns)
{
    GtkWidget *label;
    GtkWidget *random_delay;
    GtkWidget *fixed_delay;
    GtkWidget *info_label;

    /* bold header */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Autostart delay settings</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(label, 16);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, columns, 1);
    row++;

    /* random delay check button */
    random_delay = vice_gtk3_resource_check_button_new("AutostartDelayRandom",
                                                       "Add random delay");
    gtk_grid_attach(GTK_GRID(grid), random_delay, 0, row, columns, 1);
    row++;

    /* fixed delay: label, spin button and info label */
    label = gtk_label_new("Fixed delay (seconds)");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    fixed_delay = vice_gtk3_resource_spin_int_new("AutostartDelay", 0, 1000, 1);
    gtk_widget_set_hexpand(fixed_delay, FALSE);
    gtk_widget_set_vexpand(fixed_delay, FALSE);
    info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(info_label),
                        "0 = machine-specific delay for <i>stock</i> KERNAL boot");
    gtk_widget_set_margin_start(info_label, 8); /* bit of extra margin */
    gtk_widget_set_halign(info_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label,       0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), fixed_delay, 1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), info_label,  2, row, 1, 1);
    row++;

    return row;
}

/** \brief  Add widgets to set PRG-related resources
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to start adding widgets
 * \param[in]   columns columns in \a grid to use for proper widget column spans
 *
 * \return  row in \a grid for more widgets
 */
static int create_prg_layout(GtkWidget *grid, int row, int columns)
{
    GtkWidget  *label;
    GtkWidget  *colon;
    GtkWidget  *tapebasic;
    GtkWidget  *diskbasic;
    GtkWidget  *group;
    GtkWidget  *image;
    char        buffer[256];
    /* FIXME: proper extension per machine (d64 for C64, d71 for C128, d80?
     *        for PET/CBM-II */
    const char *patterns[] = { "*.d6[47]", "*.d71", "*.d8[012]", NULL };

    /* bold header */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Autostart PRG settings</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(label, 16);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, columns, 1);
    row++;

    /* "RUN:" */
    colon = vice_gtk3_resource_check_button_new("AutostartRunWithColon",
                                                "Use ':' with RUN");
    gtk_grid_attach(GTK_GRID(grid), colon, 0, row, columns, 1);
    row++;

    /* BASIC start load for tape */
    if ((machine_class != VICE_MACHINE_CBM5x0) &&
        (machine_class != VICE_MACHINE_CBM6x0) &&
        (machine_class != VICE_MACHINE_PET)) {
        tapebasic = vice_gtk3_resource_check_button_new("AutostartTapeBasicLoad",
                                                        "Load to BASIC start (Tape)");
        gtk_grid_attach(GTK_GRID(grid), tapebasic, 0, row, columns, 1);
        row++;
    }

    /* BASIC start load for disk */
    diskbasic = vice_gtk3_resource_check_button_new("AutostartBasicLoad",
                                                    "Load to BASIC start (Disk)");
    gtk_grid_attach(GTK_GRID(grid), diskbasic, 0, row, columns, 1);
    row++;

    /* autostart mode for PRGs */
    label = gtk_label_new("Autostart mode");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_START);
    group = vice_gtk3_resource_radiogroup_new("AutostartPrgMode",
                                              autostart_modes,
                                              GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_top(group, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1,           1);
    gtk_grid_attach(GTK_GRID(grid), group, 1, row, columns - 1, 1);
    row++;

    /* autostart drop mode */
    label = gtk_label_new("Drag and Drop mode");
    gtk_widget_set_valign(label, GTK_ALIGN_START);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    group = vice_gtk3_resource_radiogroup_new("AutostartDropMode",
                                              autostart_drop_modes,
                                              GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_top(group, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1,           1);
    gtk_grid_attach(GTK_GRID(grid), group, 1, row, columns - 1, 1);
    row++;

    /* temporary disk image for AUTO_START_PRG_MODE_DISK */
    label = gtk_label_new("Autostart disk image");
    image = vice_gtk3_resource_filechooser_new("AutostartPrgDiskImage",
                                               GTK_FILE_CHOOSER_ACTION_SAVE);
    g_snprintf(buffer, sizeof buffer,
               "Select or create %s autostart disk image",
               machine_name);
    vice_gtk3_resource_filechooser_set_custom_title(image, buffer);
    vice_gtk3_resource_filechooser_set_filter(image,
                                              "Disk images",
                                              patterns,
                                              TRUE);
    gtk_widget_set_margin_top(label, 16);
    gtk_widget_set_margin_top(image, 16);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1,           1);
    gtk_grid_attach(GTK_GRID(grid), image, 1, row, columns - 1, 1);

    return row + 1;
}


/** \brief  Create widget to use in the settings dialog for autostart resources
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  grid
 */
GtkWidget *settings_autostart_widget_create(GtkWidget *parent)
{
#define NUM_COLS    3
    GtkWidget *grid;
    GtkWidget *tde;
    GtkWidget *warp;
    GtkWidget *doubleclick;
    int        row = 0;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

    tde = vice_gtk3_resource_check_button_new("AutostartHandleTrueDriveEmulation",
                                              "Handle True Drive Emulation on autostart");
    gtk_grid_attach(GTK_GRID(grid), tde, 0, row, NUM_COLS, 1);
    row++;

    warp = vice_gtk3_resource_check_button_new("AutostartWarp",
                                               "Warp on autostart");
    gtk_grid_attach(GTK_GRID(grid), warp, 0, row, NUM_COLS, 1);
    row++;

    doubleclick = vice_gtk3_resource_check_button_new("AutostartOnDoubleClick",
                                                      "Double click for autostart");
    gtk_grid_attach(GTK_GRID(grid), doubleclick, 0, row, NUM_COLS, 1);
    row++;

    row = create_delay_layout(grid, row, NUM_COLS);
    row = create_prg_layout  (grid, row, NUM_COLS);
#undef NUM_COLS
    gtk_widget_show_all(grid);
    return grid;
}
