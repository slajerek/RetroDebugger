/** \file   vsidmixerwidget.c
 * \brief   GTK3 mixer widget for VSID
 *
 * Needs some way of switching between SID model to display the proper values.
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES SidResidPassband        vsid
 * $VICERES SidResidGain            vsid
 * $VICERES SidResidFilterBias      vsid
 * $VICERES SidResid8580Passband    vsid
 * $VICERES SidResid8580Gain        vsid
 * $VICERES SidResid8580FilterBias  vsid

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
#include <stdlib.h>
#include <gtk/gtk.h>

#include "vice_gtk3.h"
#include "debug.h"
#include "machine.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "sid.h"

#include "vsidmixerwidget.h"

#ifdef HAVE_RESID

/** \brief  CSS for the scales
 *
 * This makes the sliders take up less vertical space. The margin can be set
 * to a negative value (in px) to allow the slider to be larger than the scale
 * itself.
 *
 * Probably will require some testing/tweaking to get this to look acceptable
 * with various themes (and OSes).
 */
#define SLIDER_CSS \
    "scale {\n" \
    "  margin-top:    -2px;\n" \
    "  margin-bottom: -2px;\n" \
    "}\n" \
    "scale value {\n" \
    "  min-width: 6em;\n" \
    "}"

/** \brief  CSS for the labels
 *
 * Make font smaller and reduce the vertical size the labels use
 *
 * Here Be Dragons!
 */
#define LABEL_CSS \
   "label {\n" \
   "  margin-top:    -2px;\n" \
   "  margin-bottom: -2px;\n" \
   "}"


/** \brief  Number of GtkScale widgets used for the mixer */
#define NUM_SCALES  3

/** \brief  CSS provider for a label */
static GtkCssProvider *label_css_provider;
/** \brief  CSS provider for a scale  */
static GtkCssProvider *scale_css_provider;

static GtkWidget *main_grid;

/** \brief  Currently active scales
 *
 * Currently used scales, depending on SID engine and model. These references
 * are used to reset the scales to default with the "Reset" button.
 */
static GtkWidget *scale_widgets[NUM_SCALES];

static int old_sid_model = -1;
static int new_sid_model = -1;
static int old_sid_engine = -1;
static int new_sid_engine = -1;

/** \brief  Handler for the 'destroy' event of the mixer widget
 *
 * Unref CSS providers.
 *
 * \param[in]   self    mixer widget (ignored)
 * \param[in]   data    extra event data (ignored)
 */
static void on_destroy(GtkWidget *self, gpointer data)
{
    if (label_css_provider != NULL) {
        g_object_unref(label_css_provider);
        label_css_provider = NULL;
    }
    if (scale_css_provider != NULL) {
        g_object_unref(scale_css_provider);
        scale_css_provider = NULL;
    }
}

/** \brief  Handler for the 'clicked' event of the reset button
 *
 * Resets the slider to when the widget was created.
 *
 * \param[in]   widget  button (unused)
 * \param[in]   data    extra event data (unused)
 */
static void on_reset_clicked(GtkWidget *widget, gpointer data)
{
#ifndef HAVE_NEW_8580_FILTER
    int model = 0;
#endif
    int i;

    for (i = 0; i < NUM_SCALES; i++) {
        vice_gtk3_resource_scale_custom_factory(scale_widgets[i]);
    }

#ifndef HAVE_NEW_8580_FILTER
    resources_get_int("SidModel", &model);
    if ((model == SID_MODEL_8580) || (model == SID_MODEL_8580D)) {
        for (i = 0; i < NUM_SCALES; i++) {
            gtk_widget_set_sensitive(scale_widgets[i], FALSE);
        }
    }
#endif
}

/** \brief  Create a right-align label using Pango markup
 *
 * \param[in]   markup  Pango markup for the label
 *
 * \return  GtkLabel
 */
static GtkWidget *create_label(const char *markup)
{
    GtkWidget *label;

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}


/** \brief  Information on a scale for the ReSID filter settings
 */
typedef struct mixer_scale_s {
    const char     *label;              /**< label in front of the scale */
    const char     *resource_name;      /**< resource name */
    const char     *display_format;     /**< format string for the custom value */
    gint            resource_low;       /**< resource value lower bound*/
    gint            resource_high;      /**< resource value upper bound */
    gdouble         display_low;        /**< displayed value lower bound */
    gdouble         display_high;       /**< displayed value upper bound */
    gdouble         display_step;       /**< stepping for the scale */
} mixer_scale_t;

/** \brief  Custom scale configuration for 6581 */
static const mixer_scale_t scales_6581[NUM_SCALES] = {
    { "Passband", "SidResidPassband",   "%3.0f%%",
      RESID_6581_PASSBAND_MIN,   RESID_6581_PASSBAND_MAX,
      RESID_6581_PASSBAND_MIN,   RESID_6581_PASSBAND_MAX,     1.0f },
    { "Gain",     "SidResidGain",       "%3.0f%%",
      RESID_6581_FILTER_GAIN_MIN, RESID_6581_FILTER_GAIN_MAX,
      RESID_6581_FILTER_GAIN_MIN, RESID_6581_FILTER_GAIN_MAX, 1.0f },
    { "Bias",     "SidResidFilterBias", "%+1.2fmV",
      RESID_6581_FILTER_BIAS_MIN, RESID_6581_FILTER_BIAS_MAX,
      (RESID_6581_FILTER_BIAS_MIN / RESID_6581_FILTER_BIAS_ONE),
      (RESID_6581_FILTER_BIAS_MAX / RESID_6581_FILTER_BIAS_ONE),
      0.01f }
};

/** \brief  Custom scale configuration for 8580[D] */
static const mixer_scale_t scales_8580[NUM_SCALES] = {
    { "Passband", "SidResid8580Passband",   "%3.0f%%",
      RESID_8580_PASSBAND_MIN,   RESID_8580_PASSBAND_MAX,
      RESID_8580_PASSBAND_MIN,   RESID_8580_PASSBAND_MAX,     1.0f },
    { "Gain",     "SidResid8580Gain",       "%3.0f%%",
      RESID_8580_FILTER_GAIN_MIN, RESID_8580_FILTER_GAIN_MAX,
      RESID_8580_FILTER_GAIN_MIN, RESID_8580_FILTER_GAIN_MAX, 1.0f },
    { "Bias",     "SidResid8580FilterBias", "%+1.2fmV",
      RESID_8580_FILTER_BIAS_MIN, RESID_8580_FILTER_BIAS_MAX,
      (RESID_8580_FILTER_BIAS_MIN / RESID_8580_FILTER_BIAS_ONE),
      (RESID_8580_FILTER_BIAS_MAX / RESID_8580_FILTER_BIAS_ONE),
      0.01f }
};


/** \brief  Add custom resource-bound GtkScales to the grid
 *
 * \param[in]   grid    main grid
 * \param[in]   row     row in \a grid to start adding widgets
 * \param[in]   model   SID model
 *
 * \return  row in \a grid for additional widgets
 */
static int add_resid_scales(GtkWidget *grid, int row, int model)
{
    const mixer_scale_t *scales;
    int                  i;

    if (model == SID_MODEL_6581) {
        scales = scales_6581;
    } else {
        scales = scales_8580;
    }

    for (i = 0; i < NUM_SCALES; i++) {
        GtkWidget *label;
        GtkWidget *scale;

        /* destroy old widgets, if present */
        label = gtk_grid_get_child_at(GTK_GRID(grid), 0, row + i);
        if (label != NULL) {
            gtk_widget_destroy(label);
        }
        scale = gtk_grid_get_child_at(GTK_GRID(grid), 1, row + i);
        if (scale != NULL) {
            gtk_widget_destroy(scale);
        }

        /* create new widgets */
        debug_gtk3("adding scale %s at row %d", scales[i].resource_name, row + i);
        label = create_label(scales[i].label);
        scale = vice_gtk3_resource_scale_custom_new(scales[i].resource_name,
                                                    GTK_ORIENTATION_HORIZONTAL,
                                                    scales[i].resource_low,
                                                    scales[i].resource_high,
                                                    scales[i].display_low,
                                                    scales[i].display_high,
                                                    scales[i].display_step,
                                                    scales[i].display_format);
        gtk_widget_set_hexpand(scale, TRUE);
        gtk_widget_set_halign(scale, GTK_ALIGN_FILL);
        gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
        gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);

        /* use CSS to customize appearance a bit: make sure the custom formatted
         * values don't result in different widths for the scales themselves */
        vice_gtk3_css_provider_add(label, label_css_provider);
        vice_gtk3_css_provider_add(scale, scale_css_provider);

        gtk_grid_attach(GTK_GRID(grid), label, 0, row + i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), scale, 1, row + i, 1, 1);

        /* the "old" 8580 filter implementation doesn't have customizable
         * filter settings, so we disable the sliders if "old" and 8580 */
#ifndef HAVE_NEW_8580_FILTER
        if (model == SID_MODEL_8580 || model == SID_MODEL_8580D) {
            gtk_widget_set_sensitive(scale, FALSE);
        }
#endif
        gtk_widget_show(label);
        gtk_widget_show(scale);
        scale_widgets[i] = scale;
    }

    return row + i;
}

/** \brief  Create VSID mixer widget
 *
 * \return  GtkGrid
 */
GtkWidget *vsid_mixer_widget_create(void)
{
    GtkWidget *grid;
    GtkWidget *button;
    int        engine;
    int        row = 0;
    GtkWidget *label;

    label_css_provider = vice_gtk3_css_provider_new(LABEL_CSS);
    scale_css_provider = vice_gtk3_css_provider_new(SLIDER_CSS);

    resources_get_int("SidEngine", &engine);
    resources_get_int("SidModel", &new_sid_model);
    old_sid_model = new_sid_model;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_widget_set_hexpand(grid, TRUE);

    label = create_label("<b>ReSID settings</b>");
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 2, 1);
    row++;

    row = add_resid_scales(grid, row, new_sid_model);

    /* FIXME: does this make sense for non-ReSID? */
    button = gtk_button_new_with_label("Reset to defaults");
    gtk_grid_attach(GTK_GRID(grid), button, 0, row, 2, 1);

    g_signal_connect(G_OBJECT(button),
                     "clicked",
                     G_CALLBACK(on_reset_clicked),
                     NULL);

    g_signal_connect_unlocked(G_OBJECT(grid),
                              "destroy",
                              G_CALLBACK(on_destroy),
                              NULL);

    gtk_widget_show_all(grid);
    main_grid = grid;

    return grid;
}
#else
GtkWidget *vsid_mixer_widget_create(void)
{
    GtkWidget *grid;
    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_widget_set_hexpand(grid, TRUE);

    return grid;
}

#endif  /* ifdef HAVE_RESID */


/** \brief  Update mixer widget
 *
 * Update the ReSID filter scales if the model has changed.
 */
void vsid_mixer_widget_update(void)
{
#ifdef HAVE_RESID
    resources_get_int("SidEngine", &new_sid_engine);
    resources_get_int("SidModel", &new_sid_model);
    debug_gtk3("old engine = %d, new engine = %d", old_sid_engine, new_sid_engine);
    debug_gtk3("old model = %d, new model = %d", old_sid_model, new_sid_model);
    if ((new_sid_engine != old_sid_engine) ||
        (new_sid_model != old_sid_model)) {
        debug_gtk3("engine or model has changed: updating scale widgets");
        add_resid_scales(main_grid, 1, new_sid_model);
        gtk_widget_set_sensitive(main_grid, (new_sid_engine == SID_ENGINE_RESID));
#if 0
        if (new_sid_engine == SID_ENGINE_RESID) {
            gtk_widget_show_all(main_grid);
        } else {
            gtk_widget_hide(main_grid);
        }
#endif
    }
    old_sid_engine = new_sid_engine;
    old_sid_model = new_sid_model;
#endif
}
