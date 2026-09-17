/** \file   mixerwidget.c
 * \brief   GTK3 mixer widget for emus other than VSID
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES SoundVolume             -vsid
 * $VICERES SidResidPassband        -vsid
 * $VICERES SidResidGain            -vsid
 * $VICERES SidResidFilterBias      -vsid
 * $VICERES SidResid8580Passband    -vsid
 * $VICERES SidResid8580Gain        -vsid
 * $VICERES SidResid8580FilterBias  -vsid
 * $VICERES CB2Lowpass              xpet
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
#include "sid/sid.h"

#include "mixerwidget.h"


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
    "scale slider {\n" \
    "    min-width: 10px;\n" \
    "    min-height: 10px;\n" \
    "    margin: -3px;\n" \
    "}\n" \
    "scale {\n" \
    "    margin-top: -8px;\n" \
    "    margin-bottom: -8px;\n" \
    "}\n" \
    "scale value {\n" \
    "    min-width: 4em;\n" \
    "}\n"


/** \brief  CSS for the labels
 *
 * Make font smaller and reduce the vertical size the labels use
 *
 * Here Be Dragons!
 */
#define LABEL_CSS \
    "label {\n" \
    "    font-size: 80%;\n" \
    "    margin-top: -2px;\n" \
    "    margin-bottom: -2px;\n" \
    "}\n"


/** \brief  Main volume slider */
static GtkWidget *volume;

/** \brief  CB2 low pass filter setting (PETs only) */
static GtkWidget *lowpass;

#ifdef HAVE_RESID

/** \brief  ReSID 6581 passband slider */
static GtkWidget *passband6581;

/** \brief  ReSID 6581 gain slider */
static GtkWidget *gain6581;

/** \brief  ReSID 6581 filter bias slider */
static GtkWidget *bias6581;

/** \brief  ReSID 8580 passband slider */
static GtkWidget *passband8580;

/** \brief  ReSID 8580 gain slider */
static GtkWidget *gain8580;

/** \brief  ReSID 8580 filter bias slider */
static GtkWidget *bias8580;

/** \brief  ReSID 6581 passband label */
static GtkWidget *passband6581label;

/** \brief  ReSID 6581 gain label */
static GtkWidget *gain6581label;

/** \brief  ReSID 6581 filter bias label */
static GtkWidget *bias6581label;

/** \brief  ReSID 8580 passband label */
static GtkWidget *passband8580label;

/** \brief  ReSID 8580 gain label */
static GtkWidget *gain8580label;

/** \brief  ReSID 8580 filter bias label */
static GtkWidget *bias8580label;

#endif

/** \brief  CSS provider for labels
 */
static GtkCssProvider *label_css_provider;

/** \brief  CSS provider for scales (sliders)
 */
static GtkCssProvider *scale_css_provider;



/** \brief  Depending on what SID type is being used, show the right widgets
 */
void mixer_widget_sid_type_changed(void)
{
#ifdef HAVE_RESID
# ifdef HAVE_NEW_8580_FILTER
#  define FILTER_8580 TRUE
# else
#  define FILTER_8580 FALSE
# endif

    int model  = 0;
    int engine = 0;

    if (machine_class == VICE_MACHINE_VSID) {
        /* VSID has its own mechanism to toggle SID models and their widgets */
        return;
    }

    resources_get_int("SidModel",  &model);
    resources_get_int("SidEngine", &engine);

    if ((model == SID_MODEL_8580) || (model == SID_MODEL_8580D)) {
        gtk_widget_hide(passband6581);
        gtk_widget_hide(gain6581);
        gtk_widget_hide(bias6581);
        gtk_widget_show(passband8580);
        gtk_widget_show(gain8580);
        gtk_widget_show(bias8580);
        gtk_widget_hide(passband6581label);
        gtk_widget_hide(gain6581label);
        gtk_widget_hide(bias6581label);
        gtk_widget_show(passband8580label);
        gtk_widget_show(gain8580label);
        gtk_widget_show(bias8580label);
    } else {
        gtk_widget_hide(passband8580);
        gtk_widget_hide(gain8580);
        gtk_widget_hide(bias8580);
        gtk_widget_show(passband6581);
        gtk_widget_show(gain6581);
        gtk_widget_show(bias6581);
        gtk_widget_hide(passband8580label);
        gtk_widget_hide(gain8580label);
        gtk_widget_hide(bias8580label);
        gtk_widget_show(passband6581label);
        gtk_widget_show(gain6581label);
        gtk_widget_show(bias6581label);
    }

    /* enable/disable 8580 filter controls based on --enable-new8580filter */
    gtk_widget_set_sensitive(passband8580, FILTER_8580);
    gtk_widget_set_sensitive(gain8580,     FILTER_8580);
    gtk_widget_set_sensitive(bias8580,     FILTER_8580);

    if (engine == SID_ENGINE_FASTSID) {
        gtk_widget_set_sensitive(passband6581, FALSE);
        gtk_widget_set_sensitive(gain6581,     FALSE);
        gtk_widget_set_sensitive(bias6581,     FALSE);
        gtk_widget_set_sensitive(passband8580, FALSE);
        gtk_widget_set_sensitive(gain8580,     FALSE);
        gtk_widget_set_sensitive(bias8580,     FALSE);
    }
# undef FILTER_8580
#endif  /* HAVE_RESID */
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
    int value = 0;
#ifdef HAVE_RESID
    int model = 0;
#endif

    mixer_widget_sid_type_changed();
    resources_get_default_value("SoundVolume", &value);
    gtk_range_set_value(GTK_RANGE(volume), (gdouble)value);
    if (lowpass) {
        resources_get_default_value("CB2Lowpass", &value);
        vice_gtk3_resource_exp_range_set_value(GTK_RANGE(lowpass), (gdouble)value);
    }
#ifdef HAVE_RESID

    resources_get_int("SidModel", &model);
    if ((model == SID_MODEL_8580) || (model == SID_MODEL_8580D)) {
        vice_gtk3_resource_scale_custom_factory(passband8580);
        vice_gtk3_resource_scale_custom_factory(gain8580);
        vice_gtk3_resource_scale_custom_factory(bias8580);
    } else if (model == SID_MODEL_6581) {
        vice_gtk3_resource_scale_custom_factory(passband6581);
        vice_gtk3_resource_scale_custom_factory(gain6581);
        vice_gtk3_resource_scale_custom_factory(bias6581);
    }
#endif
}


/** \brief  Create a horizontally aligned label
 *
 * \param[in]   text        text for the label
 * \param[in]   minimal     use CSS to reduce size of use as statusbar widget
 * \param[in]   alignment   label alignment (see `GtkAlingn` enum`)
 *
 * \return  GtkLabel
 */
static GtkWidget *create_label(const char *text,
                               gboolean minimal,
                               GtkAlign alignment)
{
    GtkWidget *label;

    label = gtk_label_new(text);
    gtk_widget_set_halign(label, alignment);

    if (minimal) {
        vice_gtk3_css_provider_add(label, label_css_provider);
    }
    return label;
}


/** \brief  Create a customized GtkScale for \a resource
 *
 * \param[in]   resource    resource name
 * \param[in]   low         lower bound
 * \param[in]   high        upper bound
 * \param[in]   reslow      resource lower bound
 * \param[in]   reshigh     resource upper bound
 * \param[in]   step        step used to increase/decrease slider value
 * \param[in]   minimal     reduce slider size to be used in the statusbar
 *
 * \return  GtkScale
 */
static GtkWidget *create_slider(
        const char *resource,
        const char *resfmt,
        int low, int high,
        int reslow, int reshigh, float step,
        gboolean minimal)
{
    GtkWidget *scale;

    scale = vice_gtk3_resource_scale_custom_new_printf("%s",
                                                       GTK_ORIENTATION_HORIZONTAL,
                                                       reslow,
                                                       reshigh,
                                                       low,
                                                       high,
                                                       step,
                                                       resfmt,
                                                       resource);
    gtk_widget_set_hexpand(scale, TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
    /* vice_gtk3_resource_scale_int_set_marks(scale, step); */

    if (minimal) {
        /* use CSS to make sliders use less space */
        vice_gtk3_css_provider_add(scale, scale_css_provider);
    }

    /*    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE); */

    return scale;
}

/** \brief  Create an exponential GtkScale for \a resource
 *
 * \param[in]   resource    resource name without the video \a chip name prefix
 * \param[in]   power       exponent
 * \param[in]   low         lower bound
 * \param[in]   high        upper bound
 * \param[in]   step        step used to increase/decrease slider value
 * \param[in]   minimal     reduce slider size to be used in the statusbar
 *
 * \return  GtkScale
 */
static GtkWidget *create_exp_slider(
        const char *resource,
        double power,
        int low, int high, int step,
        gboolean minimal)
{
    GtkWidget *scale;

    scale = vice_gtk3_resource_scale_exp_new(resource,
            GTK_ORIENTATION_HORIZONTAL, power, low, high, step, "%d");
    gtk_widget_set_hexpand(scale, TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
    /* vice_gtk3_resource_scale_int_set_marks(scale, step); */

    if (minimal) {
        /* use CSS to make sliders use less space */
        vice_gtk3_css_provider_add(scale, scale_css_provider);
    }

    /*    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE); */
    return scale;
}

/* Commented out for now, the idea here is to allow more fine-grained control
 * over ReSID filter resources, which is still a TODO
 */
#if 0
static GtkWidget *create_spin(
        const char *resource,
        int low, int high, int step)
{
    GtkWidget *spin;

    spin = vice_gtk3_resource_spin_int_new(resource, low, high, step);
    return spin;
}
#endif


/** \brief  Create slider for main volume
 *
 * \param[in]   minimal resize slider to minimal size
 *
 * \return  GtkScale
 */
static GtkWidget *create_volume_widget(gboolean minimal)
{
    return create_slider("SoundVolume", "%3.0f%%", 0, MASTER_VOLUME_MAX, 0, MASTER_VOLUME_MAX, 1, minimal);
}


/** \brief  Create slider for CB2 low pass filter
 *
 * \param[in]   minimal resize slider to minimal size
 *
 * \return  GtkScale
 */
static GtkWidget *create_lowpass_widget(gboolean minimal)
{
    /*
     * 3.8118 is chosen so that the default setting of 16000
     * (1/3 of the full range) is at 75% on the slider:
     * (0.75)^3.8118 ~ 0.33.
     */
    return create_exp_slider("CB2Lowpass", 3.81184, 1, 48000, 1000, minimal);
}


#ifdef HAVE_RESID

/** \brief  Create slider for ReSID passband
 *
 * \param[in]   minimal resize slider to minimal size
 *
 * \return  GtkScale
 */
static GtkWidget *create_passband6581_widget(gboolean minimal)
{
    return create_slider("SidResidPassBand", "%3.0f%%",
                         RESID_6581_PASSBAND_MIN, RESID_6581_PASSBAND_MAX,
                         RESID_6581_PASSBAND_MIN, RESID_6581_PASSBAND_MAX, 1, minimal);
}


/** \brief  Create slider for ReSID gain
 *
 * \param[in]   minimal resize slider to minimal size
 *
 * \return  GtkScale
 */
static GtkWidget *create_gain6581_widget(gboolean minimal)
{
    return create_slider("SidResidGain", "%3.0f%%",
                         RESID_6581_FILTER_GAIN_MIN, RESID_6581_FILTER_GAIN_MAX,
                         RESID_6581_FILTER_GAIN_MIN, RESID_6581_FILTER_GAIN_MAX, 1, minimal);
}


/** \brief  Create slider for ReSID filter bias
 *
 * \param[in]   minimal resize slider to minimal size
 *
 * \return  GtkScale
 */
static GtkWidget *create_bias6581_widget(gboolean minimal)
{
    return create_slider("SidResidFilterBias", "%+1.2fmV",
                         (RESID_6581_FILTER_BIAS_MIN / RESID_6581_FILTER_BIAS_ONE),
                         (RESID_6581_FILTER_BIAS_MAX / RESID_6581_FILTER_BIAS_ONE),
                         RESID_6581_FILTER_BIAS_MIN, RESID_6581_FILTER_BIAS_MAX, 0.01f, minimal);
}

/** \brief  Create slider for ReSID 8580 passband
 *
 * \param[in]   minimal resize slider to minimal size
 *
 * \return  GtkScale
 */
static GtkWidget *create_passband8580_widget(gboolean minimal)
{
    return create_slider("SidResid8580PassBand", "%3.0f%%",
                         RESID_8580_PASSBAND_MIN, RESID_8580_PASSBAND_MAX,
                         RESID_8580_PASSBAND_MIN, RESID_8580_PASSBAND_MAX, 1, minimal);
}


/** \brief  Create slider for ReSID 8580 gain
 *
 * \param[in]   minimal resize slider to minimal size
 *
 * \return  GtkScale
 */
static GtkWidget *create_gain8580_widget(gboolean minimal)
{
    return create_slider("SidResid8580Gain", "%3.0f%%",
                         RESID_8580_FILTER_GAIN_MIN, RESID_8580_FILTER_GAIN_MAX,
                         RESID_8580_FILTER_GAIN_MIN, RESID_8580_FILTER_GAIN_MAX, 1, minimal);
}


/** \brief  Create slider for ReSID 8580 filter bias
 *
 * \param[in]   minimal resize slider to minimal size
 *
 * \return  GtkScale
 */
static GtkWidget *create_bias8580_widget(gboolean minimal)
{
    return create_slider("SidResid8580FilterBias", "%+1.2fmV",
                         (RESID_8580_FILTER_BIAS_MIN / RESID_8580_FILTER_BIAS_ONE),
                         (RESID_8580_FILTER_BIAS_MAX / RESID_8580_FILTER_BIAS_ONE),
                         RESID_8580_FILTER_BIAS_MIN, RESID_8580_FILTER_BIAS_MAX, 0.01f, minimal);
}
#endif  /* ifdef HAVE_RESID */


/** \brief  Create mixer widget
 *
 * \param[in]   minimal     minimize side of sliders and labels
 * \param[in]   alignment   alignment of labels
 *
 * \return  GtkGrid
 *
 */
GtkWidget *mixer_widget_create(gboolean minimal, GtkAlign alignment)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *button;
    int row = 0;
    int model = 0;
#ifdef HAVE_RESID
    gboolean sid_present = TRUE;
    int tmp;

    if (machine_class == VICE_MACHINE_PET
            || machine_class == VICE_MACHINE_VIC20
            || machine_class == VICE_MACHINE_PLUS4) {
        /* check for presence of SidCart */
        if (resources_get_int("SidCart", &tmp) < 0) {
            log_error(LOG_DEFAULT,
                    "failed to get value for resource SidCart, bailing!");
            return NULL;
        }
        sid_present = (gboolean)tmp;
    }
#endif

    grid = vice_gtk3_grid_new_spaced(16, 0);
    gtk_widget_set_margin_start(grid, 8);
    gtk_widget_set_margin_end(grid, 8);
    gtk_widget_set_hexpand(grid, TRUE);

    /* create reusable CSS providers */
    label_css_provider = vice_gtk3_css_provider_new(LABEL_CSS);
    if (label_css_provider == NULL) {
        return NULL;
    }
    scale_css_provider = vice_gtk3_css_provider_new(SLIDER_CSS);
    if (scale_css_provider == NULL) {
        return NULL;
    }

    if (minimal) {
        /*
         * 'minimal' is used when this widget is used under the statusbar,
         * in which case we add a label to make the difference between the
         * CRT and the mixer controls more clear
         */
        label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label),
                "<b><small>Mixer settings</small></b>");
        gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
    }


    button = gtk_button_new_with_label("Reset");
    gtk_grid_attach(GTK_GRID(grid), button, 1, 0, 1, 1);
    gtk_widget_set_halign(button, GTK_ALIGN_END);
    gtk_widget_set_hexpand(button, FALSE);
    g_signal_connect(button, "clicked", G_CALLBACK(on_reset_clicked), NULL);
    row++;

    label = create_label("Volume", minimal, alignment);
    volume = create_volume_widget(minimal);
    gtk_widget_set_hexpand(volume, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), volume, 1,row, 1, 1);
    row++;

    /* Only make this slider when the resource is available, i.e. on PETs */
    if (resources_query_type("CB2Lowpass") == RES_INTEGER) {
        label = create_label("CB2 Low Pass", minimal, alignment);
        lowpass = create_lowpass_widget(minimal);
        gtk_widget_set_hexpand(lowpass, TRUE);
        gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), lowpass, 1,row, 1, 1);
        row++;
    } else {
        lowpass = NULL;
    }

    if (resources_get_int("SidModel", &model) < 0) {
        log_error(LOG_DEFAULT, "failed to get SidModel resource");
        return NULL;
    }

#ifdef HAVE_RESID

    /*
     * 6581 ReSID resources
     */

    passband6581label = create_label("ReSID 6581 Passband", minimal, alignment);
    passband6581 = create_passband6581_widget(minimal);
    gtk_widget_set_sensitive(passband6581, sid_present);
    gtk_widget_set_hexpand(passband6581, TRUE);
    gtk_grid_attach(GTK_GRID(grid), passband6581label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), passband6581, 1, row, 1, 1);
    row++;

    gain6581label = create_label("ReSID 6581 Gain", minimal, alignment);
    gain6581 = create_gain6581_widget(minimal);
    gtk_widget_set_sensitive(gain6581, sid_present);
    gtk_widget_set_hexpand(gain6581, TRUE);
    gtk_grid_attach(GTK_GRID(grid), gain6581label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gain6581, 1, row, 1, 1);
    row++;

    bias6581label = create_label("ReSID 6581 Filter Bias", minimal, alignment);
    bias6581 = create_bias6581_widget(minimal);
    gtk_widget_set_hexpand(bias6581, TRUE);
    gtk_widget_set_sensitive(bias6581, sid_present);
    gtk_grid_attach(GTK_GRID(grid), bias6581label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), bias6581, 1, row, 1, 1);
    row++;

    /*
     * 8580 ReSID resources
     */

    passband8580label = create_label("ReSID 8580 Passband", minimal, alignment);
    passband8580 = create_passband8580_widget(minimal);
    gtk_widget_set_sensitive(passband8580, sid_present);
    gtk_widget_set_hexpand(passband8580, TRUE);
    gtk_grid_attach(GTK_GRID(grid), passband8580label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), passband8580, 1, row, 1, 1);
    row++;

    gain8580label = create_label("ReSID 8580 Gain", minimal, alignment);
    gain8580 = create_gain8580_widget(minimal);
    gtk_widget_set_sensitive(gain8580, sid_present);
    gtk_widget_set_hexpand(gain8580, TRUE);
    gtk_grid_attach(GTK_GRID(grid), gain8580label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gain8580, 1, row, 1, 1);
    row++;

    bias8580label = create_label("ReSID 8580 Filter Bias", minimal, alignment);
    bias8580 = create_bias8580_widget(minimal);
    gtk_widget_set_hexpand(bias8580, TRUE);
    gtk_widget_set_sensitive(bias8580, sid_present);
    gtk_grid_attach(GTK_GRID(grid), bias8580label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), bias8580, 1, row, 1, 1);
    row++;
#endif

    gtk_widget_show_all(grid);
    mixer_widget_sid_type_changed();
    return grid;
}
