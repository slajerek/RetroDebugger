/** \file   resourcescale.c
 * \brief   GktScale widget to control resources
 *
 * \note    Only integer resources are supported, which should be fine since
 *          VICE resources are either int or string (or strings abused to store
 *          arbitraty data)
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
 *
 */

#include "vice.h"

#include <gtk/gtk.h>
#include <stdarg.h>
#include <math.h>

#include "basewidget_types.h"
#include "debug_gtk3.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "resourcehelpers.h"

#include "resourcescale.h"


/** \brief  Handler for the 'value-changed' event of the \a scale widget
 *
 * Updates resource value.
 *
 * \param[in]   widget      integer scale widget
 * \param[in]   user_data   extra event data (unused)
 */
static void on_scale_int_changed(GtkWidget *widget, gpointer user_data)
{
    const char *resource;
    int         old_val = 0;
    int         new_val;

    resource = resource_widget_get_resource_name(widget);
    if (resources_get_int(resource, &old_val) < 0) {
        log_error(LOG_DEFAULT,
                  "%s(): failed to get value for resource '%s'\n",
                  __func__, resource);
        return;
    }
    new_val = (int)gtk_range_get_value(GTK_RANGE(widget));
    /* only update resource when required */
    if (old_val != new_val) {
        resources_set_int(resource, new_val);
    }
}


/** \brief  Create a scale for an integer resource - helper
 *
 * \param[in,out]   widget      interger scale widget
 *
 * \return  GtkScale
 */
static GtkWidget *resource_scale_int_new_helper(GtkWidget *widget)
{
    int value;
    const char *resource;

    resource = resource_widget_get_resource_name(widget);

    gtk_scale_set_digits(GTK_SCALE(widget), 0);

    /* set current value */
    if (resources_get_int(resource, &value) < 0) {
        log_error(LOG_DEFAULT, "failed to get value for resource '%s'\n",
                resource);
        value = 0;
    }

    /* remember original value for reset() */
    resource_widget_set_int(widget, "ResourceOrig", value);

    gtk_range_set_value(GTK_RANGE(widget), (gdouble)value);

    g_signal_connect(widget,
                     "value-changed",
                     G_CALLBACK(on_scale_int_changed),
                     NULL);

    gtk_widget_show(widget);
    return widget;
}


/** \brief  Create a scale for an integer resource
 *
 * \param[in]   resource    resource name
 * \param[in]   orientation scale orientation (\see GtkOrientation)
 * \param[in]   low         lowest value for scale
 * \param[in]   high        highest value for scale
 * \param[in]   step        value to incr/decr value with cursor keys
 *
 * \return  GtkScale
 */
GtkWidget *vice_gtk3_resource_scale_int_new(const char *resource,
                                            GtkOrientation orientation,
                                            int low,
                                            int high,
                                            int step)
{
    GtkWidget *scale;

    scale = gtk_scale_new_with_range(orientation,
            (gdouble)low, (gdouble)high, (gdouble)step);
    /* store copy of resource name */
    resource_widget_set_resource_name(scale, resource);

    return resource_scale_int_new_helper(scale);
}


/** \brief  Create a scale for an integer resource
 *
 * \param[in]   fmt         resource name format string
 * \param[in]   orientation scale orientation (\see GtkOrientation)
 * \param[in]   low         lowest value for scale
 * \param[in]   high        highest value for scale
 * \param[in]   step        value to incr/decr value with cursor keys
 *
 * \return  GtkScale
 */
GtkWidget *vice_gtk3_resource_scale_int_new_sprintf(const char *fmt,
                                                    GtkOrientation orientation,
                                                    int low,
                                                    int high,
                                                    int step,
                                                    ...)
{
    GtkWidget *scale;
    va_list    args;

    scale = gtk_scale_new_with_range(orientation,
                                     (gdouble)low,
                                     (gdouble)high,
                                     (gdouble)step);

    va_start(args, step);
    resource_widget_set_resource_name_valist(scale, fmt, args);
    va_end(args);

    return resource_scale_int_new_helper(scale);
}


/** \brief  Add marks to integer \a scale widget at each \a step increment
 *
 * \param[in,out]   widget  integer scale widget
 * \param[in]       step    distance between marks
 */
void vice_gtk3_resource_scale_int_set_marks(GtkWidget *widget, int step)
{
    GtkAdjustment *adj;
    int lower;
    int upper;
    int i;

    adj = gtk_range_get_adjustment(GTK_RANGE(widget));
    lower = (int)gtk_adjustment_get_lower(adj);
    upper = (int)gtk_adjustment_get_upper(adj);

    for (i = lower; i <= upper; i += step) {
        gtk_scale_add_mark(GTK_SCALE(widget), (gdouble)i, GTK_POS_BOTTOM, NULL);
    }
}


/** \brief  Set the integer scale \a widget to \a value
 *
 * \param[in,out]   widget  integer scale widget
 * \param[in]       value   new value for \a scale
 *
 * \return  TRUE
 */
gboolean vice_gtk3_resource_scale_int_set(GtkWidget *widget, int value)
{
    gtk_range_set_value(GTK_RANGE(widget), (gdouble)value);
    return TRUE;
}


/** \brief  Get the value of the integer scale \a widget
 *
 * \param[in]   widget  integer scale widget
 * \param[out]  value   location to store value
 *
 * \return  TRUE
 */
gboolean vice_gtk3_resource_scale_int_get(GtkWidget *widget, int *value)
{
    *value = (int)gtk_range_get_value(GTK_RANGE(widget));
    return TRUE;
}


/** \brief  Restore the \a scale to the resource's value on instanciation
 *
 * Restores the widget to the resource value on instanciantion.
 *
 * \param[in,out]   widget  integer scale widget
 *
 * \return  TRUE if the widget was reset to its original value
 */
gboolean vice_gtk3_resource_scale_int_reset(GtkWidget *widget)
{
    int orig = resource_widget_get_int(widget, "ResourceOrig");

    return vice_gtk3_resource_scale_int_set(widget, orig);
}


/** \brief  Reset \a widget to the resource factory value
 *
 * \param[in,out]   widget  integer scale widget
 *
 * \return  TRUE if the widget was restored to its factory value
 */
gboolean vice_gtk3_resource_scale_int_factory(GtkWidget *widget)
{
    const char *resource;
    int factory;

    resource = resource_widget_get_resource_name(widget);
    if (resources_get_default_value(resource, &factory) < 0) {
        return FALSE;
    }
    return vice_gtk3_resource_scale_int_set(widget, factory);
}


/** \brief  Synchronize \a widget with its resource
 *
 * \param[in,out]   widget  integer scale widget
 *
 * \return  TRUE if the widget was synchronized with its resource
 */
gboolean vice_gtk3_resource_scale_int_sync(GtkWidget *widget)
{
    const char *resource;
    int current;

    resource = resource_widget_get_resource_name(widget);
    if (resources_get_int(resource, &current) < 0) {
        return FALSE;
    }
    return vice_gtk3_resource_scale_int_set(widget, current);
}


/* Custom scale widget: Displays values with a format string and allows mapping
 * an integer range to a (different) double range
 */

static int custom_get_resource_value(GtkWidget *self);
static gdouble custom_get_display_value(GtkWidget *self);
static void custom_get_params(GtkWidget *self,
                              gint *resource_low,
                              gint *resource_high,
                              gint *resource_range,
                              gdouble *display_low,
                              gdouble *display_high,
                              gdouble *display_range);
static int custom_display_to_resource(GtkWidget *self, gdouble value);
static gdouble custom_resource_to_display(GtkWidget *self, int value);


/** \brief  Handler for the 'destroy' event of the \a scale widget
 *
 * Frees memory used by the resource name and the display format string.
 *
 * \param[in,out]   widget      integer scale widget
 * \param[in]       user_data   extra event data (unused)
 */
static void on_scale_custom_destroy(GtkWidget *widget, gpointer user_data)
{
    resource_widget_free_string(widget, "DisplayFormat");
}


/** \brief  Handler for the 'value-changed' event of the custom scale widget
 *
 * \param[in]   self    custom scale widget
 * \param[in]   data    extra event data (unused)
 */
static void on_custom_changed(GtkWidget *self, gpointer data)
{
    const char *resource;
    int old_val;
    int new_val;

    resource = resource_widget_get_resource_name(self);
    if (resources_get_int(resource, &old_val) < 0) {
        log_error(LOG_DEFAULT, "failed to get value for resource '%s'\n",
                resource);
        return;
    }
    new_val = custom_display_to_resource(self, gtk_range_get_value(GTK_RANGE(self)));
    if (old_val != new_val) {
        resources_set_int(resource, new_val);
    }
}


static gchar *on_custom_format(GtkScale *self, gdouble value)
{
    const char *format = g_object_get_data(G_OBJECT(self), "DisplayFormat");

    return g_strdup_printf(format, gtk_range_get_value(GTK_RANGE(self)));
}


/** \brief  Get parameters required to convert between resource and display value
 *
 * \param[in]   self            custom scale widget
 * \param[out]  resource_low    resource value lower bound
 * \param[out]  resource_high   resource value upper bound
 * \param[out]  resource_range  resource value range
 * \param[out]  display_low     display value lower bound
 * \param[out]  display_high    display value upper bound
 * \param[out]  display_range   display value range
 */
static void custom_get_params(GtkWidget *self,
                              gint *resource_low,
                              gint *resource_high,
                              gint *resource_range,
                              gdouble *display_low,
                              gdouble *display_high,
                              gdouble *display_range)
{
    GtkAdjustment *adjustment = gtk_range_get_adjustment(GTK_RANGE(self));

    *resource_low = resource_widget_get_int(self, "ResourceLow");
    *resource_high = resource_widget_get_int(self, "ResourceHigh");
    *resource_range = *resource_high - *resource_low;
    *display_low = gtk_adjustment_get_lower(adjustment);
    *display_high = gtk_adjustment_get_upper(adjustment);
    *display_range = *display_high - *display_low;
}


/** \brief  Convert display value to resource value
 *
 * \param[in]   self    custom scale widget
 * \param[in]   value   custom display value
 *
 * \return  resource value
 *
 * \note    Doesn't touch the resource
 */
static int custom_display_to_resource(GtkWidget *self, gdouble value)
{
    gint resource_low;
    gint resource_high;
    gint resource_range;
    gint resource;
    gdouble display_low;
    gdouble display_high;
    gdouble display_range;
    gdouble factor = 0.0;

    custom_get_params(self,
                      &resource_low, &resource_high, &resource_range,
                      &display_low, &display_high, &display_range);

    if (display_range != 0) {
        factor = (gdouble)(resource_range) / display_range;
    }
    resource = round(factor * (value - display_low)) + resource_low;
    return resource;
}


/** \brief  Convert resource value to display value
 *
 * \param[in]   self    custom scale widget
 * \param[in]   value   raw resource value
 *
 * \return  custom display value
 *
 * \note    Doesn't touch the resource
 */
static gdouble custom_resource_to_display(GtkWidget *self, int value)
{
    gint resource_low;
    gint resource_high;
    gint resource_range;
    gdouble display_low;
    gdouble display_high;
    gdouble display_range;
    gdouble factor = 0.0;
    gdouble display;

    custom_get_params(self,
                      &resource_low, &resource_high, &resource_range,
                      &display_low, &display_high, &display_range);
    if (resource_range != 0) {
        factor = display_range / (gdouble)resource_range;
    }
    display = (factor * (gdouble)(value - resource_low)) + display_low;
    return display;
}


/** \brief  Convert current display value to integer resource value
 *
 * \param[in]   self    custom scale widget
 *
 * \return  current display value scaled and offset for setting the resource
 */
static int custom_get_resource_value(GtkWidget *self)
{
    gdouble display_value;

    display_value = gtk_range_get_value(GTK_RANGE(self));
    return custom_display_to_resource(self, display_value);
}


/** \brief  Convert current resource integer value to gdouble for display
 *
 * \param[in]   self    custom scale widget
 *
 * \return  current resource value scaled and offset for display
 */
static gdouble custom_get_display_value(GtkWidget *self)
{
    const char *resource_name;
    int resource_value;

    resource_name = resource_widget_get_resource_name(self);
    if (resources_get_int(resource_name, &resource_value) < 0) {
        log_error(LOG_DEFAULT,
                  "%s(): failed to get value for resource '%s'.",
                  __func__, resource_name);
        resource_value = 0;
    }
    return custom_resource_to_display(self, resource_value);
}




/** \brief  Helper function to create the custom resource-bound scale widget
 *
 * \param[in]   self            scale widget
 * \param[in]   resource_low    resource lower bound (inclusive)
 * \param[in]   resource_high   resource upper bound (inclusive)
 * \param[in]   display_fmt     display format string
 *
 * \return  \a self
 */
static GtkWidget *custom_new_helper(GtkWidget *self,
                                    gint resource_low,
                                    gint resource_high,
                                    const gchar *display_fmt)
{
    gdouble display_value;
    gulong changed_handler;
    gulong format_handler;
    const char *resource_name;
    int resource_value;

    resource_name = resource_widget_get_resource_name(self);
    resources_get_int(resource_name, &resource_value);
    resource_widget_set_int(self, "ResourceOrig", resource_value);
    resource_widget_set_int(self, "ResourceLow", resource_low);
    resource_widget_set_int(self, "ResourceHigh", resource_high);
    resource_widget_set_string(self, "DisplayFormat", display_fmt);

    display_value = custom_get_display_value(self);
    gtk_range_set_value(GTK_RANGE(self), display_value);

    changed_handler = g_signal_connect(self,
                                       "value-changed",
                                       G_CALLBACK(on_custom_changed),
                                       NULL);
    format_handler = g_signal_connect_unlocked(self,
                                               "format-value",
                                               G_CALLBACK(on_custom_format),
                                               NULL);
    g_signal_connect_unlocked(self,
                              "destroy",
                              G_CALLBACK(on_scale_custom_destroy),
                              NULL);
    g_object_set_data(G_OBJECT(self),
                     "ChangedHandler",
                     GULONG_TO_POINTER(changed_handler));
    g_object_set_data(G_OBJECT(self),
                     "FormatHandler",
                     GULONG_TO_POINTER(format_handler));
    return self;
}


/** \brief  Create scale widget displaying custom value bound to resource
 *
 * Create a scale widget that maps an integer resource's range to a custom
 * range using double with a custom format string.
 *
 * \param[in]   resource        resource name
 * \param[in]   orientation     GtkScale orientation
 * \param[in]   resource_low    resource lower bound (inclusive)
 * \param[in]   resource_high   resource upper bound (inclusive)
 * \param[in]   display_low     display lower bound (inclusive)
 * \param[in]   display_high    display upper bound (inclusive)
 * \param[in]   display_step    display stepping
 * \param[in]   display_fmt     display format string
 *
 * \return  new custom resource-bound scale widget
 */
GtkWidget *vice_gtk3_resource_scale_custom_new(
        const gchar *resource,
        GtkOrientation orientation,
        gint resource_low, gint resource_high,
        gdouble display_low, gdouble display_high, gdouble display_step,
        const gchar *display_fmt)
{
    GtkWidget *scale;

    scale = gtk_scale_new_with_range(orientation,
                                     display_low,
                                     display_high,
                                     display_step);
    /* store copy of resource name */
    resource_widget_set_resource_name(scale, resource);

    return custom_new_helper(scale, resource_low, resource_high, display_fmt);
}


/** \brief  Create scale widget displaying custom value bound to resource
 *
 * Create a scale widget that maps an integer resource's range to a custom
 * range using double with a custom format string.
 * This version uses \a resource as a printf format string to generate the
 * resource name.
 *
 * \param[in]   resource        resource name format string
 * \param[in]   orientation     GtkScale orientation
 * \param[in]   resource_low    resource lower bound (inclusive)
 * \param[in]   resource_high   resource upper bound (inclusive)
 * \param[in]   display_low     display lower bound (inclusive)
 * \param[in]   display_high    display upper bound (inclusive)
 * \param[in]   display_step    display stepping
 * \param[in]   display_fmt     display format string
 * \param[in]   ...             resource name format string arguments
 *
 * \return  new custom resource-bound scale widget
 */

GtkWidget *vice_gtk3_resource_scale_custom_new_printf(
        const gchar *resource,
        GtkOrientation orientation,
        gint resource_low, gint resource_high,
        gdouble display_low, gdouble display_high, gdouble display_step,
        const gchar *display_fmt,
        ...)
{
    GtkWidget *scale;
    char *resource_name;
    va_list args;
#if 0
    debug_gtk3("display low: %g, display high: %g\n", display_low, display_high);
#endif
    scale = gtk_scale_new_with_range(orientation,
                                     display_low,
                                     display_high,
                                     display_step);
    /* create resource name */
    va_start(args, display_fmt);
    resource_name = lib_mvsprintf(resource, args);
    va_end(args);
#if 0
    debug_gtk3("Formatted resource name: '%s'.", resource_name);
#endif
    resource_widget_set_resource_name(scale, resource_name);
    lib_free(resource_name);

    return custom_new_helper(scale, resource_low, resource_high, display_fmt);
}


/** \brief  Set the raw resource-bound value
 *
 * Set the bound resource's new value and update the widget.
 *
 * \param[in]   widget  custom scale widget
 * \param[in]   value   new value for \a widget
 *
 * \return  true on success
 */
gboolean vice_gtk3_resource_scale_custom_set(GtkWidget *widget, int value)
{
    const char *resource;
    gulong changed_handler;
    gboolean result;
    gdouble display_value;

    resource = resource_widget_get_resource_name(widget);
    display_value = custom_resource_to_display(widget, value);

    /* block the changed handler so we don't update the resource twice, but
     * keep the format handler active to update the displayed value */
    changed_handler = GPOINTER_TO_ULONG(g_object_get_data(G_OBJECT(widget),
                                                          "ChangedHandler"));
    g_signal_handler_block(widget, changed_handler);
    result = resources_set_int(resource, value) == 0 ? TRUE : FALSE;
    gtk_range_set_value(GTK_RANGE(widget), display_value);
    g_signal_handler_unblock(widget, changed_handler);

    return result;
}


/** \brief  Get the raw resource-bound value
 *
 * Get the bound resource's current value.
 *
 * \param[in]   widget  custom scale widget
 * \param[out]  value   current value of the resource
 *
 * \return  TRUE
 */
gboolean vice_gtk3_resource_scale_custom_get(GtkWidget *widget, int *value)
{
    *value = custom_get_resource_value(widget);
    return TRUE;
}


/** \brief  Reset widget to the state it had when instanciated
 *
 * Reset the widget to its resource value when the widget was instanciated.
 *
 * \param[in]   widget  custom scale widget
 *
 * \return  TRUE on success
 */
gboolean vice_gtk3_resource_scale_custom_reset(GtkWidget *widget)
{
    int orig;

    orig = resource_widget_get_int(widget, "ResourceOrig");
    return vice_gtk3_resource_scale_custom_set(widget, orig);
}


/** \brief  Set widget to the factory value of its resource
 *
 * Reset the widget to the value its resource had when the widget was instanciated.
 *
 * \param[in]   widget  custom scale widget
 *
 * \return  TRUE on success
 */
gboolean vice_gtk3_resource_scale_custom_factory(GtkWidget *widget)
{
    const char *resource;
    int factory;

    resource = resource_widget_get_resource_name(widget);
    if (resources_get_default_value(resource, &factory) != 0) {
        return FALSE;
    }
    return vice_gtk3_resource_scale_custom_set(widget, factory);
}


/** \brief  Synchronize the widget state with its resource
 *
 * Update the widget's state in case its resource was altered through other means
 * than this widget.
 *
 * \param[in]   widget  custom scale widget
 *
 * \return  TRUE on success
 */
gboolean vice_gtk3_resource_scale_custom_sync(GtkWidget *widget)
{
    const char *resource;
    int current;

    resource = resource_widget_get_resource_name(widget);
    if (resources_get_int(resource, &current) != 0) {
        return FALSE;
    }
    return vice_gtk3_resource_scale_custom_set(widget, current);
}

/* Custom exponential widget: allows mapping an integer range to
 * the slider in an exponential way.
 * It differs from the "custom" slider in that the value which is shown
 * to the user is also stored in the resource.  Just which position on
 * the slider matches to which value is different.
 */

static void exponential_get_params(GtkWidget *self,
                                   gdouble *power,
                                   gint *display_low,
                                   gint *display_high,
                                   gint *display_range);
static gint exp_slider_to_resource(GtkWidget *self, gint value);
static gint exp_resource_to_slider(GtkWidget *self, gint value);

/** \brief  Handler for the 'destroy' event of the \a exp widget
 *
 * Frees memory used by the resource name and the display format string.
 *
 * \param[in,out]   widget      integer exponential scale widget
 * \param[in]       user_data   extra event data (unused)
 */
static void on_scale_exp_destroy(GtkWidget *widget, gpointer user_data)
{
    resource_widget_free_string(widget, "DisplayFormat");
}

/** \brief  Handler for the 'value-changed' event of the exp scale widget
 *
 * \param[in]   self    custom scale widget
 * \param[in]   data    extra event data (unused)
 */
static void on_exp_changed(GtkWidget *self, gpointer data)
{
    const char *resource;
    int old_val;
    int new_val;

    resource = resource_widget_get_resource_name(self);
    if (resources_get_int(resource, &old_val) < 0) {
        log_error(LOG_DEFAULT, "failed to get value for resource '%s'\n",
                resource);
        return;
    }
    new_val = exp_slider_to_resource(self, gtk_range_get_value(GTK_RANGE(self)));
    if (old_val != new_val) {
        resources_set_int(resource, new_val);
    }
}

static gchar *on_exp_format(GtkScale *self, gdouble value)
{
    const char *format = g_object_get_data(G_OBJECT(self), "DisplayFormat");

    int new_val = exp_slider_to_resource(GTK_WIDGET(self), value);

    return g_strdup_printf(format, new_val);
}

/** \brief  Get parameters required to convert between resource value and slider position
 *
 * \param[in]   self            custom scale widget
 * \param[out]  power           exponent
 * \param[out]  resource_low    resource value lower bound
 * \param[out]  resource_high   resource value upper bound
 * \param[out]  resource_range  resource value range
 */
static void exponential_get_params(GtkWidget *self,
                              gdouble *power,
                              gint *resource_low,
                              gint *resource_high,
                              gint *resource_range)
{
    *power = resource_widget_get_int(self, "Power") / 10000.0;
    *resource_low = resource_widget_get_int(self, "ResourceLow");
    *resource_low = resource_widget_get_int(self, "ResourceLow");
    *resource_high = resource_widget_get_int(self, "ResourceHigh");
    *resource_range = *resource_high - *resource_low + 1;
}

/** \brief  Convert slider value to resource value
 *
 * \param[in]   self    exponential scale widget
 * \param[in]   value   linear slider value
 *
 * \return  resource value
 *
 * \note    Doesn't touch the resource
 */
static int exp_slider_to_resource(GtkWidget *self, gint value)
{
    gint resource_low;
    gint resource_high;
    gint resource_range;
    gint resource;
    gdouble power;

    exponential_get_params(self,
                           &power,
                           &resource_low, &resource_high, &resource_range);

    /* Normalize value to [0, 1] */
    gdouble v = (gdouble)value / 1000.0;

    /* Apply the power of exponents ;-) */
    v = pow(v, power);

    /* Scale to the range of the resource */
    resource = resource_low + v * resource_range;

    return resource;
}


/** \brief  Convert resource value to slider value
 *
 * \param[in]   self    exponential scale widget
 * \param[in]   value   linear slider value
 *
 * \return  resource value
 *
 * \note    Doesn't touch the resource
 */
static gint exp_resource_to_slider(GtkWidget *self, gint value)
{
    gint resource_low;
    gint resource_high;
    gint resource_range;
    gdouble result;
    gdouble power;

    exponential_get_params(self,
                           &power,
                           &resource_low, &resource_high, &resource_range);

    /* Normalize the resource value to [0, 1] */
    result = (double)(value - resource_low) / resource_range;

    /*                                 power   ______
     * Invert the power of exponents:        |/result
     * = result ^ (1/power)
     */
    result = pow(result, 1.0 / power);

    /* Scale to the range of the slider [0, 1000] */
    result *= 1000.0;

    return result + 0.5;
}

void vice_gtk3_resource_exp_range_set_value(GtkRange *self, gdouble value)
{
    gint slider_value = exp_resource_to_slider(GTK_WIDGET(self), value);
    gtk_range_set_value(self, slider_value);
}



/** \brief  Helper function to create the exponential resource-bound scale
 *          widget
 *
 * \param[in]   self            scale widget
 * \param[in]   power           the exponent
 * \param[in]   resource_low    resource lower bound (inclusive)
 * \param[in]   resource_high   resource upper bound (inclusive)
 * \param[in]   display_fmt     display format string
 *
 * \return  \a self
 */
static GtkWidget *exp_new_helper(GtkWidget *self,
                                 gdouble power,
                                 gint resource_low,
                                 gint resource_high,
                                 const gchar *display_fmt)
{
    gulong changed_handler;
    gulong format_handler;
    const char *resource_name;
    int resource_value;

    resource_name = resource_widget_get_resource_name(self);
    resources_get_int(resource_name, &resource_value);
    resource_widget_set_int(self, "Power", (int)(power * 10000.0 + 0.5));
    resource_widget_set_int(self, "ResourceOrig", resource_value);
    resource_widget_set_int(self, "ResourceLow", resource_low);
    resource_widget_set_int(self, "ResourceHigh", resource_high);
    resource_widget_set_string(self, "DisplayFormat", display_fmt);

    vice_gtk3_resource_exp_range_set_value(GTK_RANGE(self), resource_value);

    changed_handler = g_signal_connect(self,
                                       "value-changed",
                                       G_CALLBACK(on_exp_changed),
                                       NULL);
    format_handler = g_signal_connect_unlocked(self,
                                               "format-value",
                                               G_CALLBACK(on_exp_format),
                                               NULL);
    g_signal_connect_unlocked(self,
                              "destroy",
                              G_CALLBACK(on_scale_exp_destroy),
                              NULL);
    g_object_set_data(G_OBJECT(self),
                     "ChangedHandler",
                     GULONG_TO_POINTER(changed_handler));
    g_object_set_data(G_OBJECT(self),
                     "FormatHandler",
                     GULONG_TO_POINTER(format_handler));

    return self;
}


/** \brief  Create exponential scale widget displaying value bound to resource
 *
 * Create a scale widget that displays an integer resource's range to a
 * slider which reacts exponentially.
 *
 * \param[in]   resource        resource name
 * \param[in]   orientation     GtkScale orientation
 * \param[in]   power           exponent
 * \param[in]   resource_low    lower bound (inclusive)
 * \param[in]   resource_high   upper bound (inclusive)
 * \param[in]   display_step    stepping
 * \param[in]   display_fmt     display format string
 *
 * \return  new exponential resource-bound scale widget
 */
GtkWidget *vice_gtk3_resource_scale_exp_new(
        const gchar *resource,
        GtkOrientation orientation,
        gdouble power,
        gint resource_low, gint resource_high, gint display_step,
        const gchar *display_fmt)
{
    GtkWidget *scale;

    scale = gtk_scale_new_with_range(orientation,
                                     0,
                                     1000,
                                     display_step);
    /* store copy of resource name */
    resource_widget_set_resource_name(scale, resource);

    return exp_new_helper(scale, power, resource_low, resource_high, display_fmt);
}


