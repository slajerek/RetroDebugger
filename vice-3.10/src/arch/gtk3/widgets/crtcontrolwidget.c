/** \file   crtcontrolwidget.c
 * \brief   GTK3 CRT settings widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Provides a list of GtkScale widgets to alter CRT settings.
 *
 * Supported settings per chip/video mode:
 *
 * |Setting             |VICIIP|VICIIN|VDC|VICP|VICN|TEDP|TEDN|CRTC|
 * |--------------------|------|------|---|----|----|----|----|----|
 * |Brightness          | yes  | yes  |yes|yes |yes |yes |yes |yes |
 * |Contrast            | yes  | yes  |yes|yes |yes |yes |yes |yes |
 * |Saturation          | yes  | yes  |yes|yes |yes |yes |yes |yes |
 * |Tint                | yes  | yes  |yes|yes |yes |yes |yes |yes |
 * |Gamma               | yes  | yes  |yes|yes |yes |yes |yes |yes |
 * |Blur                | yes  | yes  |yes|yes |yes |yes |yes |yes |
 * |Scanline shade      | yes  | yes  |yes|yes |yes |yes |yes |yes |
 * |Odd lines phase     | yes  |  no  | no|yes | no |yes | no | no |
 * |Odd lines offset    | yes  |  no  | no|yes | no |yes | no | no |
 * |U-only delayline    | yes  |  no  | no|yes | no |yes | no | no |
 *
 * TODO:    Fix display of sliders when switching between PAL and NTSC
 */

/*
 * $VICERES CrtcColorBrightness     xpet xcbm2
 * $VICERES CrtcColorContrast       xpet xcbm2
 * $VICERES CrtcColorGamma          xpet xcbm2
 * $VICERES CrtcColorSaturation     xpet xcbm2
 * $VICERES CrtcColorTint           xpet xcbm2
 * $VICERES CrtcPALBlur             xpet xcbm2
 * $VICERES CrtcPALScanLineShade    xpet xcbm2
 *
 * $VICERES TEDColorBrightness      xplus4
 * $VICERES TEDColorContrast        xplus4
 * $VICERES TEDColorGamma           xplus4
 * $VICERES TEDColorSaturation      xplus4
 * $VICERES TEDColorTint            xplus4
 * $VICERES TEDPALBlur              xplus4
 * $VICERES TEDPALOddLineOffset     xplus4
 * $VICERES TEDPALOddLinePhase      xplus4
 * $VICERES TEDPALScanLineShade     xplus4
 * $VICERES TEDPALDelaylineType     xplus4
 *
 * $VICERES VDCColorBrightness      x128
 * $VICERES VDCColorContrast        x128
 * $VICERES VDCColorGamma           x128
 * $VICERES VDCColorSaturation      x128
 * $VICERES VDCColorTint            x128
 * $VICERES VDCPALBlur              x128
 * $VICERES VDCPALScanLineShade     x128
 *
 * $VICERES VICColorBrightness      xvic
 * $VICERES VICColorContrast        xvic
 * $VICERES VICColorGamma           xvic
 * $VICERES VICColorSaturation      xvic
 * $VICERES VICColorTint            xvic
 * $VICERES VICPALBlur              xvic
 * $VICERES VICPALOddLineOffset     xvic
 * $VICERES VICPALOddLinePhase      xvic
 * $VICERES VICPALScanLineShade     xvic
 * $VICERES VICPALDelaylineType     xvic
 *
 * $VICERES VICIIColorBrightness    x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIColorContrast      x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIColorGamma         x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIColorSaturation    x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIColorTint          x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIPALBlur            x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIPALOddLineOffset   x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIPALOddLinePhase    x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIPALScanLineShade   x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIPALDelaylineType   x64 x64sc x64dtv xscpu64 x128 xcbm5x0
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

#include <gtk/gtk.h>
#include <stdbool.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "crtcontrolwidget.h"


/** \brief  CSS for the scale widgets in the status bar CRT widget
 *
 * This makes the sliders take up less vertical space. The margin can be set
 * to a negative value (in px) to allow the slider to be larger than the scale
 * itself.
 *
 * Probably will require some testing/tweaking to get this to look acceptable
 * with various themes (and OSes).
 */
#define SCALE_CSS_STATUSBAR \
    "scale slider {\n" \
    "  min-width: 10px;\n" \
    "  min-height: 10px;\n" \
    "  margin: -3px;\n" \
    "}\n\n" \
    "scale value {\n" \
    "  font-family: monospace;\n" \
    "  font-size: 80%;\n" \
    "}\n" \
    "scale {\n" \
    "  margin-top: -8px;\n" \
    "  margin-bottom: -8px;\n" \
    "}"

/** \brief  CSS used to tweak looks of CRT sliders in the settings dialog
 *
 * We use monospace for the value labels to keep them aligned on the decimal
 * point and to keep the width of the sliders consistent.
 */
#define SCALE_CSS_DIALOG \
    "scale value {\n" \
    "  font-family: monospace;\n" \
    "}"

/** \brief  CSS for the "U-only delayline" check button on the status bar
 */
#define CHECKBUTTON_CSS_STATUSBAR \
    "checkbutton {\n" \
    "  font-size: 80%;\n" \
    "}\n" \
    "checkbutton check {\n" \
    "  min-width: 12px;\n" \
    "  min-height: 12px;\n" \
    "}"

/** \brief  CSS for the reset button when used on the status bar
 */
#define BUTTON_CSS_STATUSBAR \
    "button {\n" \
    "  min-width: 12px;\n" \
    "  min-height: 12px;\n" \
    "  padding-top: 2px;\n " \
    "  padding-left: 2px;\n " \
    "  padding-bottom: 2px;\n " \
    "  padding-right: 2px;\n " \
    "}"

/** \brief  CSS for the labels
 *
 * Make font smaller and reduce the vertical size the labels use
 *
 * Here Be Dragons!
 */
#define LABEL_CSS \
    "label {\n" \
    "  font-size: 80%;\n" \
    "  margin-top: -2px;\n" \
    "  margin-bottom: -2px;\n" \
    "}"

/** \brief  Size of array required for all resources */
#define RESOURCE_COUNT  9

/** \brief  CRT resource info
 */
typedef struct crt_control_info_s {
    const char *label;      /**< displayed name (label) */
    const char *name;       /**< resource name excluding CHIP prefix */
    bool        ntsc;       /**< resource is valid for NTSC */
    int         low;        /**< lowest value for resource */
    int         high;       /**< highest value for resource */
    int         neutral;    /**< neutral value for resource */
    int         step;       /**< stepping for the spin button */
    double      disp_low;   /**< display low */
    double      disp_high;  /**< display high */
    double      disp_step;  /**< stepping for the displayed values */
    const char *disp_fmt;   /**< format string for the displayed value */
    int         emu_mask;   /**< mask indicating which emus expose the resource */
} crt_control_info_t;

/** \brief  Object holding internal state of a CRT control widget
 *
 * Since we can have two video chips (C128's VICII+VDC), we cannot use static
 * references to widgets and need to allocate memory for the references and
 * clean that memory up once the widget is destroyed.
 */
typedef struct crt_control_data_s {
    char           chip[16];                /**< video chip name */
    GtkWidget     *scales[RESOURCE_COUNT];  /**< sliders */
    GtkWidget     *delayline;               /**< U-delay line widget */
} crt_control_data_t;


/** \brief  List of CRT emulation resources
 */
static const crt_control_info_t control_info[RESOURCE_COUNT] = {
    { "Brightness",     "ColorBrightness",  true,  0, 2000, 1000, 100,   0.0, 200.0,  0.1, "%5.1f%%",      VICE_MACHINE_ALL },
    { "Contrast",       "ColorContrast",    true,  0, 2000, 1000, 100,   0.0, 200.0,  0.1, "%5.1f%%",      VICE_MACHINE_ALL },
    { "Saturation",     "ColorSaturation",  true,  0, 2000, 1000, 100,   0.0, 200.0,  0.1, "%5.1f%%",      VICE_MACHINE_ALL },
    { "Tint",           "ColorTint",        true,  0, 2000, 1000, 100, -25.0,  25.0,  0.1, "%+5.1f%%",     VICE_MACHINE_ALL },
    { "Gamma",          "ColorGamma",       true,  0, 4000, 1000, 200,   0.0,   4.0, 0.01, "%6.2f",        VICE_MACHINE_ALL },
    { "Blur",           "PALBlur",          true,  0, 1000,    0,  50,   0.0, 100.0,  0.1, "%5.1f%%",      VICE_MACHINE_ALL },
    { "Scanline shade", "PALScanLineShade", true,  0, 1000, 1000,  50,   0.0, 100.0,  0.1, "%5.1f%%",      VICE_MACHINE_ALL },
    { "Oddline phase",  "PALOddLinePhase",  false, 0, 2000, 1000, 100, -25.0,  25.0,  0.1, "%+5.1f\u00b0", VICE_MACHINE_ALL^VICE_MACHINE_CBM6x0^VICE_MACHINE_PET },
    { "Oddline offset", "PALOddLineOffset", false, 0, 2000, 1000, 100, -50.0,  50.0,  0.1, "%+4.1f%%",     VICE_MACHINE_ALL^VICE_MACHINE_CBM6x0^VICE_MACHINE_PET }
};


/** \brief  CSS provider for labels */
static GtkCssProvider *label_css = NULL;

/** \brief  CSS provider for the reset button on the status bar */
static GtkCssProvider *button_css = NULL;

/** \brief  CSS provider for the U-delayline check button on the status bar */
static GtkCssProvider *checkbutton_css = NULL;

/** \brief  CSS provider for scales in the CRT widget for the statusbar */
static GtkCssProvider *scale_css_statusbar = NULL;

/** \brief  CSS provider for scales in the settings dialog */
static GtkCssProvider *scale_css_dialog = NULL;

/** \brief  MachineVideoStandard resource value on construction */
static int standard = 0;


/** \brief  Determine if the PAL-specific controls must be enabled
 *
 * Check video standard and \a chip for PAL support.
 *
 * \param[in]   chip    video chip name
 *
 * \return  true if PAL controls must be enabled
 */
static bool is_pal(const char *chip)
{
    if ((standard == MACHINE_SYNC_PAL || standard == MACHINE_SYNC_PALN) &&
        (chip[0] != 'C') /* not Crtc */ &&
        (!(chip[0] == 'V' && chip[1] == 'D'))) /* not VDC */ {
        return true;
    }
    return false;
}

/** \brief  Reset all sliders to their factory value
 *
 * \param[in]   data    crt control data
 */
static void reset_sliders(crt_control_data_t *data)
{
    int i;

    for (i = 0; i < RESOURCE_COUNT; i++) {
        GtkWidget *scale = data->scales[i];

        if (scale != NULL) {
            vice_gtk3_resource_scale_custom_factory(scale);
        }
    }
}

/** \brief  Reset all sliders to their neutral value
 *
 * \param[in]   data    crt control data
 */
static void reset_sliders_neutral(crt_control_data_t *data)
{
    int i;

    for (i = 0; i < RESOURCE_COUNT; i++) {
        GtkWidget *scale = data->scales[i];
        if (scale != NULL) {
            vice_gtk3_resource_scale_custom_set(scale, control_info[i].neutral);
        }
    }
}

/** \brief  Reset all sliders to their factory value
 *
 * \param[in]   button  reset button
 * \param[in]   data    crt control data
 */
static void on_reset_clicked(GtkWidget *button, gpointer data)
{
    reset_sliders(data);
}

static gboolean on_reset_right_clicked (GtkWidget* self, GdkEventButton *event, gpointer data) {
    guint button = event->button;
    GdkModifierType state = event->state;
    if ((button == 3) || (state & GDK_BUTTON3_MASK) ||
        (state & GDK_SHIFT_MASK) || (state & GDK_CONTROL_MASK)) {
        /* right button, or click+shift/control */
        reset_sliders_neutral(data);
        return TRUE;
    }
    return FALSE; /* propagate further, on_reset_clicked() handles regular left clicks */
}

/** \brief  Clean up memory used by the internal state of \a widget
 *
 * \param[in]   widget  widget
 * \param[in]   data    crt control dat
 */
static void on_widget_destroy(GtkWidget *widget, gpointer data)
{
    lib_free(data);
}


/** \brief  Create reusable CSS providers for the widgets
 *
 * Create CSS providers once when needed and only unref on emulator shutdown.
 * Keeping them around after a widget using them has been destroyed avoids
 * having to recreate them every time a dialog/widget is recreated that uses
 * said widget.
 *
 * \return  TRUE when all CSS was successfully parsed
 */
static gboolean create_css_providers(void)
{
    if (label_css == NULL) {
        label_css = vice_gtk3_css_provider_new(LABEL_CSS);
        if (label_css == NULL) {
            return FALSE;
        }
    }
    if (button_css == NULL) {
        button_css = vice_gtk3_css_provider_new(BUTTON_CSS_STATUSBAR);
        if (button_css == NULL) {
            return FALSE;
        }
    }
    if (checkbutton_css == NULL) {
        checkbutton_css = vice_gtk3_css_provider_new(CHECKBUTTON_CSS_STATUSBAR);
        if (checkbutton_css == NULL) {
            return FALSE;
        }
    }

    if (scale_css_statusbar == NULL) {
        scale_css_statusbar = vice_gtk3_css_provider_new(SCALE_CSS_STATUSBAR);
        if (scale_css_statusbar == NULL) {
            return FALSE;
        }
    }
    if (scale_css_dialog == NULL) {
        scale_css_dialog = vice_gtk3_css_provider_new(SCALE_CSS_DIALOG);
        if (scale_css_dialog == NULL) {
            return FALSE;
        }
    }
    return TRUE;
}

/** \brief  Create right-aligned label with a smaller font
 *
 * \param[in]   text    label text
 * \param[in]   minimal reduce label to minimum size
 *
 * \return  GtkLabel
 */
static GtkWidget *create_label(const char *text, gboolean minimal)
{
    GtkWidget *label;

    label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_END);

    if (minimal) {
        vice_gtk3_css_provider_add(label, label_css);
    }
    return label;
}

/** \brief  Create a customized GtkScale for \a resource
 *
 * \param[in]   resource        resource name without the \a chip prefix
 * \param[in]   chip            video chip name
 * \param[in]   resource_low    resource value lower bound
 * \param[in]   resource_high   resource value upper bound
 * \param[in]   display_low     display value lower bound
 * \param[in]   display_high    display value upper bound
 * \param[in]   display_step    display value stepping
 * \param[in]   display_format  format string for displaying value
 * \param[in]   minimal         reduced size (for the statusbar widget)
 *
 * \return  GtkScale
 */
static GtkWidget *create_slider(const char *resource,
                                const char *chip,
                                int         resource_low,
                                int         resource_high,
                                double      display_low,
                                double      display_high,
                                double      display_step,
                                const char *display_format,
                                gboolean    minimal)
{
    GtkWidget *scale;

    scale = vice_gtk3_resource_scale_custom_new_printf("%s%s",
                                                       GTK_ORIENTATION_HORIZONTAL,
                                                       resource_low,
                                                       resource_high,
                                                       display_low,
                                                       display_high,
                                                       display_step,
                                                       display_format,
                                                       chip,
                                                       resource);
    gtk_widget_set_hexpand(scale, TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);

    /* set up custom CSS to make the scale take up less space */
    if (minimal) {
        vice_gtk3_css_provider_add(scale, scale_css_statusbar);
    } else {
        vice_gtk3_css_provider_add(scale, scale_css_dialog);
    }

    return scale;
}

/** \brief  Create "U-only delayline" check button
 *
 * \param[in]   chip    video chip name
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_delayline_widget(const char *chip)
{
    return vice_gtk3_resource_check_button_new_sprintf("%sPALDelaylineType",
                                                       "U-only Delayline",
                                                       chip);
}

/** \brief  Add GtkScale sliders to \a grid
 *
 * \param[in,out]   grid    grid to add widgets to
 * \param[in,out]   data    internal data of the main widget
 * \param[in]       minimal minimize size of the slider, no spinboxes
 *
 * \return  row number of last widget added
 */
static int add_sliders(GtkGrid             *grid,
                        crt_control_data_t *data,
                        gboolean            minimal)
{
    GtkWidget                *label;
    GtkWidget                *scale;
    const crt_control_info_t *info;
    const char               *chip;
    int                       row;
    size_t                    i;

    chip = data->chip;
    row = 1;

    if (!minimal) {
        for (i = 0; i < RESOURCE_COUNT; i ++) {
            info = &(control_info[i]);
            if (!(info->emu_mask & machine_class)) {
                continue;
            }
            if (!(is_pal(chip)) && !(info->ntsc)) {
                continue;
            }

            label   = create_label(info->label, minimal);

            gtk_grid_attach(grid, label, 0, row, 1, 1);
            scale = create_slider(info->name,
                                  chip,
                                  info->low,
                                  info->high,
                                  info->disp_low,
                                  info->disp_high,
                                  info->disp_step,
                                  info->disp_fmt,
                                  minimal);
            gtk_grid_attach(grid, scale, 1, row, 1, 1);
            data->scales[i] = scale;
            row++;
        }
    } else {
        /* Add sliders for statusbar popup */
        for (i = 0; i < RESOURCE_COUNT; i ++) {
            int column;

            info = &(control_info[i]);
            if (!(info->emu_mask & machine_class)) {
                continue;
            }
            if (!(is_pal(chip)) && !(info->ntsc)) {
                continue;
            }


            column = (i % 2) * 2;
            label  = create_label(info->label, minimal);

            gtk_grid_attach(grid, label, column + 0, row, 1, 1);
            scale = create_slider(info->name,
                                  chip,
                                  info->low,
                                  info->high,
                                  info->disp_low,
                                  info->disp_high,
                                  info->disp_step,
                                  info->disp_fmt,
                                  minimal);
            gtk_grid_attach(grid, scale, column + 1, row, 1, 1);
            data->scales[i] = scale;
            if (column > 0) {
                row++;
            }
        }
    }

    return row + 1;
}

/** \brief  Create heap-allocated CRT controls state object
 *
 * We need this since we share this code with the CRT controls available via
 * the statusbar.
 *
 * \param[in]   chip    video chip name
 *
 * \return  heap-allocated CRT controls state object
 */
static crt_control_data_t *create_control_data(const char *chip)
{
    crt_control_data_t *data;
    size_t              index;

    data = lib_malloc(sizeof *data);

    strncpy(data->chip, chip, sizeof data->chip - 1u);
    data->chip[sizeof data->chip - 1u] = '\0';

    for (index = 0; index < RESOURCE_COUNT; index++) {
        data->scales[index] = NULL;
    }
    data->delayline = NULL;
    return data;
}

/** \brief  Create header with label and reset button
 *
 * Create grid with label mentioning \a chip and reset button, adjust size
 * depending on whether \a minimal is `TRUE`.
 *
 * \param[in]   chip    video chip name
 * \param[in]   minimal reduce size of widgets for use on the status bar
 * \param[in]   data    CRT controls runtime data object
 *
 * \return  GtkGrid
 */
static GtkWidget *create_header(const char         *chip,
                                gboolean            minimal,
                                crt_control_data_t *data)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *button;
    GtkWidget *image;
    char       buffer[256];

    grid = gtk_grid_new();
    gtk_widget_set_hexpand(grid, TRUE);

    /* header label */
    if (minimal) {
        g_snprintf(buffer, sizeof buffer,
                   "<small><b>CRT settings (%s)</b></small>", chip);
    } else {
        g_snprintf(buffer, sizeof buffer, "<b>CRT settings (%s)</b>", chip);
    }
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), buffer);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);

    /* reset button */
    button = gtk_button_new();
    image  = gtk_image_new_from_icon_name("edit-undo-symbolic",
                                           GTK_ICON_SIZE_BUTTON);
    if (minimal) {
        gtk_image_set_pixel_size(GTK_IMAGE(image), 8);
        /* We have to use CSS here: none of the Gtk size/margin/alignment
         * setters work to reduce size from what Gtk considers minimal. */
        vice_gtk3_css_provider_add(button, button_css);
        gtk_widget_set_margin_top(button, 2);
        gtk_widget_set_margin_start(button, 2);
        gtk_widget_set_margin_end(button, 2);
        gtk_widget_set_margin_bottom(button, 2);
    }
    gtk_button_set_image(GTK_BUTTON(button), image);
    gtk_widget_set_halign(button, GTK_ALIGN_END);

    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(on_reset_clicked),
                     (gpointer)data);
    g_signal_connect(button,
                     "button-release-event",
                     G_CALLBACK(on_reset_right_clicked),
                     (gpointer)data);

    gtk_grid_attach(GTK_GRID(grid), label,  0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button, 1, 0, 1, 1);
    return grid;
}


/** \brief  Create CRT control widget for \a chip
 *
 * \param[in]   parent  parent widget (unused)
 * \param[in]   chip    video chip name (all caps)
 * \param[in]   minimal reduce slider sizes for use in the status bar
 *
 * \todo    Add a way to force update of all sliders and their sensitivity when
 *          MachineVideoStandard is changed (ie CRT sliders in the statusbar
 *          are shown and the user switches from PAL to NTSC)
 *
 * \return  GtkGrid
 */
GtkWidget *crt_control_widget_create(GtkWidget  *parent,
                                     const char *chip,
                                     gboolean    minimal)
{
    GtkWidget          *grid;
    GtkWidget          *header;
    crt_control_data_t *data;
    int                 row;

    /* sanity check */
    if (RESOURCE_COUNT != sizeof control_info / sizeof control_info[0]) {
        log_error(LOG_DEFAULT,
                  "RESOURCE_COUNT doesn't match element count of control_info[]");
        return NULL;
    }

    /* create reusable CSS providers */
    if (!create_css_providers()) {
        return NULL;
    }

    /* this avoids obtaining the vice lock in each is_pal() call */
    resources_get_int("MachineVideoStandard", &standard);
    data = create_control_data(chip);

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), minimal ? 4 : 8);

    /* header */
    header = create_header(chip, minimal, data);
    gtk_grid_attach(GTK_GRID(grid), header, 0, 0, minimal ? 4 : 2, 1);

    /* add scales and spin buttons */
    row = add_sliders(GTK_GRID(grid), data, minimal);

    /* add U-only delayline check button */
    if ((machine_class != VICE_MACHINE_CBM6x0) &&
        (machine_class != VICE_MACHINE_PET) &&
        (is_pal(chip))) {
        data->delayline = create_delayline_widget(chip);
        if (minimal) {
            vice_gtk3_css_provider_add(data->delayline, checkbutton_css);
            gtk_grid_attach(GTK_GRID(grid), data->delayline, 2, row - 1, 2, 1);
        } else {
            gtk_widget_set_margin_top(data->delayline, 8);
            gtk_grid_attach(GTK_GRID(grid), data->delayline, 0, row, 3, 1);
        }
        row++;
    }

    g_object_set_data(G_OBJECT(grid), "InternalState", (gpointer)data);
    g_signal_connect_unlocked(grid,
                              "destroy",
                              G_CALLBACK(on_widget_destroy),
                              (gpointer)data);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Free memory used by CSS providers for the sliders
 *
 * Call this function on UI shutdown.
 */
void crt_control_widget_shutdown(void)
{
    if (label_css != NULL) {
        g_object_unref(label_css);
    }
    if (button_css != NULL) {
        g_object_unref(button_css);
    }
    if (checkbutton_css != NULL) {
        g_object_unref(checkbutton_css);
    }
    if (scale_css_statusbar != NULL) {
        g_object_unref(scale_css_statusbar);
    }
    if (scale_css_dialog != NULL) {
        g_object_unref(scale_css_dialog);
    }
}
