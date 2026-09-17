/** \file   resourcespinbutton.c
 * \brief   Spin buttons to control resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * This file contains spin buttons to control resources. The integer resource
 * spin button in its default state should be clear: alter integers. But the
 * "fake digits" might require some explanation:
 *
 * With "fake digits" I mean displaying an integer resource as if it were a
 * floating point value. There are some VICE resources that are used a integers
 * but actually represent a different 'scale'.
 *
 * An example is the `Drive[8-11]RPM` resource, this represents a drive's RPM
 * in hundredths of the actually resource value, ie `31234` means `312.34` RPM.
 *
 * So to display this value properly, one would set "fake digits to `2`. This
 * way the widget divides the actual value by 10^2 when displaying, and
 * multiplies the input value by 10^2 when setting the value through the widget.
 *
 * I've decided to support 1-5 digits, that ought to be enough. And since we're
 * displaying integer values divided by a power of 10, the "fake digits" number
 * also sets the displayed digits, since that's the maximum accuracy.
 *
 * That behaviour can be changed by calling gtk_spin_button_set_digits() on
 * the widget.
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
#include <math.h>

#include "basewidget_types.h"
#include "debug_gtk3.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "resourcehelpers.h"

#include "resourcespinbutton.h"


/** \brief  Get string resource value as a double
 *
 * Get value for \a resource and try to convert to double.
 * If \a text is not `NULL` the original string value will be stored there.
 *
 * \param[in]   resource    resource name
 * \param[out]  value       resource converted to double
 * \param[out]  text        original string resource value (optional)
 *
 * \return  `true` on success
 *
 * \note    on failure \a value and \a text are not touched
 */
static bool get_double_resource(const char  *resource,
                                double      *value,
                                const char **text)
{
    const char *current = NULL;

    if (resources_get_string(resource, &current) == 0) {
        double  result;
        char   *endptr;

        result = strtod(current, &endptr);
        if (*endptr == '\0') {
            *value = result;
            if (text != NULL) {
                *text = current;
            }
            return true;
        }
    }
    return false;
}

/** \brief  Set double in string resource
 *
 * \param[in]   resource    resource name
 * \param[in]   value       new value
 *
 * \return  `true` on success
 */
static bool set_double_resource(const char *resource,
                                double      value)
{
    char text[64];

    g_snprintf(text, sizeof text, "%16f", value);
    return resources_set_string(resource, text) == 0;
}

/** \brief  Handler for the 'destroy' event of a double resource spin button
 *
 * Frees memory used by resource name and resource original value of \a self.
 *
 * \param[in]   self    resource double spin button
 * \param[in]   data    extra event data (unused)
 */
static void on_spin_button_double_destroy(GtkWidget *self, gpointer data)
{
    resource_widget_free_string(self, "ResourceOrig");
}

/** \brief  Handler for the 'input' event of the \a spin button
 *
 * Checks and converts input of the \a spin button.
 *
 * \param[in,out]   spin        integer spin button
 * \param[out]      new_value   pointer to the value of the spin button
 * \param[in]       user_data   extra event data (unused)
 *
 * \return  TRUE when input is valid, GTK_INPUT_ERROR on invalid input
 */
static gint on_spin_button_input(GtkWidget *spin,
                                 gpointer new_value,
                                 gpointer user_data)
{
    const gchar *text;
    gchar *endptr;
    gdouble value;
    gdouble multiplier;
    int digits;

    digits = resource_widget_get_int(spin, "FakeDigits");
    multiplier = pow(10.0, (gdouble)digits);

    text = gtk_entry_get_text(GTK_ENTRY(spin));
    value = g_strtod(text, &endptr);
    if (*endptr != '\0') {
        return GTK_INPUT_ERROR;
    }

    *(gdouble *)new_value = value * multiplier;
    return TRUE;
}


/** \brief  Handler for the 'output' event of the \a spin button
 *
 * This outputs the spin button value as a floating point value, with digits
 * depending on the "fake digits" setting.
 *
 * \param[in,out]   spin        integer spin button
 * \param[in]       user_data   extra event data (unused)
 *
 * \return  TRUE
 */
static gboolean on_spin_button_output(GtkWidget *spin, gpointer user_data)
{
    GtkAdjustment *adj;
    gchar *text;
    gdouble value;
    gdouble divisor;
    int digits;
    /* silly, but avoids the 'format' is not const warnings */
    const char *fmt[] = {
        "%.1f", "%.2f", "%.3f","%.4f", "%.5f"
    };

    digits = resource_widget_get_int(spin, "FakeDigits");
    divisor = pow(10.0, (gdouble)digits);

    adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin));
    value = gtk_adjustment_get_value(adj);
    text = g_strdup_printf(fmt[digits - 1], value / divisor);
    gtk_entry_set_text(GTK_ENTRY(spin), text);
    g_free(text);

    return TRUE;
}


/** \brief  Handler for the 'value-changed' event of \a widget
 *
 * Updates the resource with the current value of the spin buttton
 *
 * \param[in]   spin        integer spin button
 * \param[in]   user_data   extra event data (unused)
 */
static void on_spin_button_value_changed(GtkWidget *spin, gpointer user_data)
{
    const char *res;
    int value;

    res = resource_widget_get_resource_name(spin);
    value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
    if (resources_set_int(res, value) < 0) {
        log_error(LOG_DEFAULT, "failed to set resource '%s' to %d\n", res, value);
    }
}

/******************************************************************************
 * Spin button for resources containing double integers                       *
 *****************************************************************************/

/** \brief  Create integer spin button to control a resource - helper
 *
 * \param[in,out]   spin    spin button
 *
 * \return  GtkSpinButton
 */
static GtkWidget *resource_spin_int_new_helper(GtkWidget *spin)
{
    const char *resource;
    int         current = 0;

    resource = resource_widget_get_resource_name(spin);

    /* set fake digits to 0 */
    g_object_set_data(G_OBJECT(spin), "FakeDigits", GINT_TO_POINTER(0));
    /* set real digits to 0 */
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), 0);

    if (resources_get_int(resource, &current) < 0) {
        log_error(LOG_DEFAULT,
                  "%s(): failed to get value for resource '%s'\n",
                  __func__, resource);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), (gdouble)current);
    g_signal_connect(spin,
                     "value-changed",
                     G_CALLBACK(on_spin_button_value_changed),
                     NULL);

    gtk_widget_show(spin);
    return spin;
}


/** \brief  Create integer spin button to control \a resource
 *
 * \param[in]   resource    resource name
 * \param[in]   lower       lowest value
 * \param[in]   upper       highest value
 * \param[in]   step        step for the +/- buttons
 *
 * \return  GtkSpinButton
 */
GtkWidget *vice_gtk3_resource_spin_int_new(
        const char *resource,
        int lower, int upper, int step)
{
    GtkWidget *spin;
    int current;

    spin = gtk_spin_button_new_with_range((gdouble)lower, (gdouble)upper,
            (gdouble)step);

    /* set a copy of the resource name */
    resource_widget_set_resource_name(spin, resource);

    /* store current resource value for use in reset() */
    if (resources_get_int(resource, &current) < 0) {
        log_error(LOG_DEFAULT,
                "failed to get current value for resource '%s', defaulting"
                " to 0.",
                resource);
        current = 0;
    }
    resource_widget_set_int(spin, "ResourceOrig", current);

    return resource_spin_int_new_helper(spin);
}


/** \brief  Create integer spin button to control \a resource
 *
 * Allows setting the resource name with a variable argument list
 *
 * \param[in]   fmt     resource name format string
 * \param[in]   lower   lowest value
 * \param[in]   upper   highest value
 * \param[in]   step    step for the +/- buttons
 *
 * \return  GtkSpinButton
 */
GtkWidget *vice_gtk3_resource_spin_int_new_sprintf(
        const char *fmt,
        int lower, int upper, int step,
        ...)
{
    GtkWidget *spin;
    va_list    args;

    spin = gtk_spin_button_new_with_range((gdouble)lower,
                                          (gdouble)upper,
                                          (gdouble)step);
    va_start(args, step);
    resource_widget_set_resource_name_valist(spin, fmt, args);
    va_end(args);

    return resource_spin_int_new_helper(spin);
}


/** \brief  Set "fake digits" for the integer spint button
 *
 * \param[in,out]   spin    integer spin button
 * \param[in]       digits  number of fake digits to display
 *
 */
void vice_gtk3_resource_spin_int_set_fake_digits(
        GtkWidget *spin,
        int digits)
{
    if (digits <= 0 || digits > 5) {
        return;
    }
    resource_widget_set_int(spin, "FakeDigits", digits);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), digits);
    g_signal_connect_unlocked(spin, "input", G_CALLBACK(on_spin_button_input), NULL);
    g_signal_connect_unlocked(spin, "output", G_CALLBACK(on_spin_button_output), NULL);
}


/** \brief  Set spin button \a widget to \a value
 *
 * \param[in,out]   widget  integer spin button
 * \param[in]       value   new value for the spin button
 *
 * \return  TRUE
 */
gboolean vice_gtk3_resource_spin_int_set(GtkWidget *widget, int value)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), (gdouble)value);
    return TRUE;
}


/** \brief  Get spin button \a widget value
 *
 * \param[in]   widget  integer spin button
 * \param[out]  value   object to store value
 *
 * \return  TRUE if \a value was set
 */
gboolean vice_gtk3_resource_spin_int_get(GtkWidget *widget, int *value)
{
    const char *resource = resource_widget_get_resource_name(widget);
    if (resources_get_int(resource, value) < 0) {
        log_error(LOG_DEFAULT, "failed to get value for resource '%s'.", resource);
        return FALSE;
    }
    return TRUE;
}


/** \brief  Reset widget to its state when instanciated
 *
 * \param[in,out]   widget  integer spin button
 *
 * \return  TRUE if the widget was reset to its original value
 */
gboolean vice_gtk3_resource_spin_int_reset(GtkWidget *widget)
{
    int orig = resource_widget_get_int(widget, "ResourceOrig");
    return vice_gtk3_resource_spin_int_set(widget, orig);
}


/** \brief  Reset widget to its resources factory value
 *
 * \param[in,out]   widget  integer spin button
 *
 * \return  TRUE if the widget was set to its factory value
 */
gboolean vice_gtk3_resource_spin_int_factory(GtkWidget *widget)
{
    const char *resource;
    int factory;

    resource = resource_widget_get_resource_name(widget);
    if (resources_get_default_value(resource, &factory) < 0) {
        log_error(LOG_DEFAULT,
                "failed to get factory value for resource '%s'.",
                resource);
        return FALSE;
    }
    return vice_gtk3_resource_spin_int_set(widget, factory);
}


/** \brief  Synchronize \a widget with its associated resource
 *
 * \param[in,out]   widget  integer resource spinbutton
 *
 * \return  TRUE if the widget was sychronized
 */
gboolean vice_gtk3_resource_spin_int_sync(GtkWidget *widget)
{
    const char *resource_name;
    int widget_val;
    int resource_val;

    /* get widget state */
    if (!vice_gtk3_resource_spin_int_get(widget, &widget_val)) {
        log_error(LOG_DEFAULT, "failed to retrieve current value of widget");
        return FALSE;
    }
    /* get resource value */
    resource_name = resource_widget_get_resource_name(widget);
    if (resources_get_int(resource_name, &resource_val) < 0) {
        log_error(LOG_DEFAULT,
                "failed to retrieve value for resource '%s'",
                resource_name);
    }

    /*
     * Check current value against resource value to avoid triggering
     * an event connected to the resource
     */
    if (widget_val != resource_val) {
        return vice_gtk3_resource_spin_int_set(widget, resource_val);
    }
    return TRUE;
}


/******************************************************************************
 * Spin button for resources containing double values                         *
 *                                                                            *
 * Since VICE doesn't support true float/double resources we internally use a *
 * string resource.                                                           *
 *****************************************************************************/

/** \brief  Handler for the 'value-changed' event of a double resource spin button
 *
 * \param[in]   self    double resource spin button
 * \param[in]   data    extra event data (unused)
 */
static void on_spin_button_double_value_changed(GtkWidget *self,
                                                gpointer   data)
{
    const char *resource;
    double      value;

    value    = gtk_spin_button_get_value(GTK_SPIN_BUTTON(self));
    resource = resource_widget_get_resource_name(self);
    set_double_resource(resource, value);
}


/** \brief  Create spin button for double resource
 *
 * Create a resource-bound spin button interpreting a string resource to double.
 *
 * \param[in]   resource    resource name
 * \param[in]   lower       lower bound of value
 * \param[in]   upper       upper bound of value
 * \param[in]   step        stepping to use when clicking +/-
 *
 * \return  GtkSpinButton
 */
GtkWidget *vice_gtk3_resource_spin_double_new(const char *resource,
                                              double      lower,
                                              double      upper,
                                              double      step)
{
    GtkWidget  *spin;
    double      current = 0.0;
    const char *curstr = "0.0";

    get_double_resource(resource, &current, &curstr);
    spin = gtk_spin_button_new_with_range(lower, upper, step);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), current);
    resource_widget_set_resource_name(spin, resource);
    /* we can't store a double with g_object_set() so we keep the string */
    resource_widget_set_string(spin, "ResourceOrig", curstr);

    g_signal_connect_unlocked(G_OBJECT(spin),
                              "destroy",
                              G_CALLBACK(on_spin_button_double_destroy),
                              NULL);
    g_signal_connect(G_OBJECT(spin),
                     "value-changed",
                     G_CALLBACK(on_spin_button_double_value_changed),
                     NULL);
    return spin;
}


/** \brief  Create spin button for double resource
 *
 * Create a resource-bound spin button interpreting a string resource to double.
 * This version uses string formatting to create the resource name.
 *
 * \param[in]   resource    resource name
 * \param[in]   lower       lower bound of value
 * \param[in]   upper       upper bound of value
 * \param[in]   step        stepping to use when clicking +/-
 * \param[in]   ...         arguments to \a fmt
 *
 * \return  GtkSpinButton
 */
GtkWidget *vice_gtk3_resource_spin_double_new_sprintf(const char *fmt,
                                                      double      lower,
                                                      double      upper,
                                                      double      step,
                                                      ...)
{
    char    resource[256];
    va_list args;

    va_start(args, step);
    g_vsnprintf(resource, sizeof resource, fmt, args);
    va_end(args);
    return vice_gtk3_resource_spin_double_new(resource, lower, upper, step);
}


/** \brief  Set new value for resource double spin button
 *
 * \param[in]   spin    resource double spin button
 * \param[in]   value   new value for \a spin
 *
 * \return  `TRUE` on success
 */
gboolean vice_gtk3_resource_spin_double_set(GtkWidget *spin, double value)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), value);
    return TRUE;
}


/** \brief  Get value of resource double spin button
 *
 * \param[in]   spin    resource double spin button
 * \param[out]  value   current value of \a spin
 *
 * \return  `TRUE` on success
 */
gboolean vice_gtk3_resource_spin_double_get(GtkWidget *spin, double *value)
{
    *value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
    return TRUE;
}


/** \brief  Reset resource double spin button to the value on construction
 *
 * Reset \a spin to the value the bound resource contained when \a spin was
 * constructed.
 *
 * \param[in]   spin    resource double spin button
 *
 * \return  `TRUE` on success
 */
gboolean vice_gtk3_resource_spin_double_reset(GtkWidget *spin)
{
    const char *orig;
    double      value;
    char       *endptr;

    orig = resource_widget_get_string(spin, "ResourceOrig");
    if (orig != NULL) {
        value = strtod(orig, &endptr);
        if (*endptr == '\0') {
            return vice_gtk3_resource_spin_double_set(spin, value);
        }
    }
    return FALSE;
}

/** \brief  Restore resource double spin button to factory value
 *
 * \param[in]   spin    resource double spin button
 *
 * \return  `TRUE` on success
 */
gboolean vice_gtk3_resource_spin_double_factory(GtkWidget *spin)
{
    const char  *resource;
    const char  *factory = NULL;
    char        *endptr;
    double       value;

    resource = resource_widget_get_resource_name(spin);
    if (resources_get_default_value(resource, &factory) < 0) {
        log_error(LOG_DEFAULT,
                  "failed to get factory value for resource %s'.",
                  resource);
        return FALSE;
    }
    value = strtod(factory, &endptr);
    if (*endptr != '\0') {
        log_error(LOG_DEFAULT,
                  "factory value '%s' of resource '%s' could not be converted"
                  " to double.",
                  factory, resource);
        return FALSE;
    }
    return vice_gtk3_resource_spin_double_set(spin, value);
}


/** \brief  Synchronize double spin button with its resource
 *
 * \param[in]   spin    resource double spin button
 *
 * \return  `TRUE` on success
 */
gboolean vice_gtk3_resource_spin_double_sync(GtkWidget *spin)
{
    const char *resource;
    double      value = 0.0;

    resource = resource_widget_get_resource_name(spin);
    if (get_double_resource(resource, &value, NULL)) {
        return vice_gtk3_resource_spin_double_set(spin, value);
    }
    return FALSE;
}


/** \brief   Set the precision to be displayed by a resource double spin button
 *
 * \param[in]   spin    resource double spin button
 * \param[in]   digits  number of digits
 *
 * \note    currently just calls gtk_spin_button_set_digits(\a spin), but the
 *          internal structure of \a spin might change, so this function is
 *          preferred over the gtk one.
 */
void vice_gtk3_resource_spin_double_set_digits(GtkWidget *spin, guint digits)
{
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), digits);
}



/******************************************************************************
 *   Spin button for displaying custom values based on alternate ranges and   *
 *   using format strings to display affixes.                                 *
 *****************************************************************************/

/** \brief  Size of buffer for formatting the display value
 *
 * Three chars for '-0.', 53 for mantissa and 1023 for exponent for the smallest
 * negative value of an IEEE 64-bit float, 1 for terminating '\0', and a random
 * number of characters for the custom formatting such as a prefix or suffix.
 */
#define CS_BUFSIZE  (3 + DBL_MANT_DIG + (-DBL_MIN_EXP) + 42 + 1)


/** \brief  Custom spin button state object
 *
 * Provides the ranges, stepping, resource name and display format string for
 * the events/methods of the custom spin button.
 */
typedef struct cs_state_s {
    gchar   *res_name;  /**< resource name */
    gint     res_min;   /**< resource minimum value */
    gint     res_max;   /**< resource maximum value */
    gdouble  disp_min;  /**< displayed minimum value */
    gdouble  disp_max;  /**< displayed maximum value */
    gdouble  disp_step; /**< stepping for displayed value */
    gchar   *disp_fmt;  /**< format string for displayed value */
} cs_state_t;


/** \brief  Create internal state object for custom spin button
 *
 * \param[in]   resource_name   resource name
 * \param[in]   resource_min    resource minimum value
 * \param[in]   resource_max    resource maximum value
 * \param[in]   display_min     displayed minimum value
 * \param[in]   display_max     displayed maximum value
 * \param[in]   display_step    displayed value stepping
 * \param[in]   display_format  format string for displayed value
 *
 * \return  new custom spin state
 */
static cs_state_t *cs_state_new(const gchar *resource_name,
                                gint         resource_min,
                                gint         resource_max,
                                gdouble      display_min,
                                gdouble      display_max,
                                gdouble      display_step,
                                const gchar *display_format)
{
    cs_state_t *state = g_malloc(sizeof *state);

    state->res_name  = g_strdup(resource_name);
    state->res_min   = resource_min;
    state->res_max   = resource_max;
    state->disp_min  = display_min;
    state->disp_max  = display_max;
    state->disp_step = display_step;
    state->disp_fmt  = g_strdup(display_format);

    return state;
}

/** \brief  Free custom spin state object and its members
 *
 * \param[in]   state   custom spin state
 */
static void cs_state_free(cs_state_t *state)
{
    g_free(state->res_name);
    g_free(state->disp_fmt);
    g_free(state);
}

/** \brief  Determine ratio of resource range:display range
 *
 * \param[in]   state   custom spin state
 *
 * \return  ratio to use when translating between resource and display values
 */
static gdouble cs_state_ratio(const cs_state_t *state)
{
    /* incorrect: should be +1 for the integer range */
    return ((gdouble)(state->res_max) - (gdouble)(state->res_min)) /
           (state->disp_max - state->disp_min);
}

/** \brief  Calculate display value from resource value
 *
 * \param[in]   value   resource value
 * \param[in]   state   custom spin state
 *
 * \return  resource value
 */
static gdouble cs_state_resource_to_display(int value, const cs_state_t *state)
{
    return ((gdouble)(value - state->res_min) / cs_state_ratio(state)) + state->disp_min;
}

/** \brief  Calculate resource value from display value
 *
 * \param[in]   value   display value
 * \param[in]   state   custom spin state
 *
 * \return  resource value
 */
static int cs_state_display_to_resource(gdouble value, const cs_state_t *state)
{
    return round((value - state->disp_min) * cs_state_ratio(state)) + state->res_min;
}

/** \brief  Handler for the 'destroy' event of a custom spin button
 *
 * Frees the internal state object.
 *
 * \param[in]   self    custom spin button
 * \param[in]   data    custom spin state
 */
static void on_custom_spin_destroy(GtkWidget *self, gpointer data)
{
    if (data != NULL) {
        cs_state_free(data);
    }
}

/** \brief  Handler for the 'output' event of a custom spin button
 *
 * Format the value according to the \c display_format provided in the widget's
 * constructor.
 *
 * \param[in]   self    custom spin button
 * \param[in]   data    custom spin state
 */
static gboolean on_custom_spin_output(GtkSpinButton *self, gpointer data)
{
    GtkAdjustment *adjustment;
    gchar          buffer[CS_BUFSIZE];
    cs_state_t    *state = data;
    gdouble        value;

    adjustment = gtk_spin_button_get_adjustment(self);
    value      = gtk_adjustment_get_value(adjustment);
    g_snprintf(buffer, sizeof(buffer), state->disp_fmt, value);
#if 0
    debug_gtk3("Setting spin button display to '%s'\n", buffer);
#endif
    gtk_entry_set_text(GTK_ENTRY(self), buffer);
    return TRUE;
}

/** \brief  Handler for the 'input' event of a custom spin button
 *
 * Convert string entered by user to double and store in \a value.
 *
 * \param[in]   self    custom spin button
 * \param[out]  value   converted value
 * \param[in]   data    custom spin state (unused)
 */
static gint on_custom_spin_input(GtkSpinButton *self, gdouble *value, gpointer data)
{
    const char *text;
    gdouble     result;
    char       *endptr = NULL;

    text   = gtk_entry_get_text(GTK_ENTRY(self));
    result = strtod(text, &endptr);
#if 0
    debug_gtk3("Spin button text: '%s', result = %f\n", text, result);
#endif
    *value = result;
    return TRUE;
}

/** \brief  Handler for the 'value-changed' event of a custom spin button
 *
 * Convert displayed value to resource value and update resource.
 *
 * \param[in]   self    custom spin button
 * \param[in]   data    custom spin state
 */
static void on_custom_spin_value_changed(GtkSpinButton *self, gpointer data)
{
    cs_state_t *state = data;
    gdouble     dvalue; /* displayed value */
    int         rvalue; /* resource value */

    dvalue = gtk_spin_button_get_value(self);
    rvalue = cs_state_display_to_resource(dvalue, state);
#if 0
    debug_gtk3("Display value: %4.1f, resource value: %d", dvalue, rvalue);
#endif
    if (rvalue < state->res_min) {
        rvalue = state->res_min;
    } else if (rvalue > state->res_max) {
        rvalue = state->res_max;
    }
    resources_set_int(state->res_name, rvalue);
}

/** \brief  Get minimum width required for the largest values of the widget
 *
 * Determine the minimum width in characters required for the displayed values.
 *
 * \param[in]   state   custom spin state
 *
 * \return  width in characters
 */
static int cs_get_minimum_width(const cs_state_t *state)
{
    char buffer[CS_BUFSIZE];
    int  len_minval;
    int  len_maxval;

    /* print minimum and maximum values into buffer to see which one provides
     * the largest string */
    len_minval = g_snprintf(buffer, sizeof(buffer), state->disp_fmt, state->disp_min);
    len_maxval = g_snprintf(buffer, sizeof(buffer), state->disp_fmt, state->disp_max);
    /* return largest value of the two */
    return len_maxval > len_minval ? len_maxval : len_minval;
}


/** \brief  Create a resource-bound spin button with custom range and formatting
 *
 * Create spin button that displays values between \a display_min and
 * \a display_max but internally uses \a resource_min and \a resource_max for
 * the resource.
 * The \a display_format string is used to format the displayed value, and while
 * the resource is an \c int the displayed value is a \c double, so the format
 * string should reflect this. When displaying and setting the resource, scaling
 * is applied to translate between the resource range and value and its displayed
 * value and range.
 *
 * \param[in]   resource_name   resource name
 * \param[in]   resource_min    resource minimum value
 * \param[in]   resource_max    resource maximum value
 * \param[in]   display_min     displayed minimum value
 * \param[in]   display_max     displayed maximum value
 * \param[in]   display_step    displayed value stepping
 * \param[in]   display_format  format string for displayed value
 *
 * \return  GtkSpinButton
 */
GtkWidget *vice_gtk3_resource_spin_custom_new(const gchar *resource_name,
                                              gint         resource_min,
                                              gint         resource_max,
                                              gdouble      display_min,
                                              gdouble      display_max,
                                              gdouble      display_step,
                                              const gchar *display_format)
{
    GtkWidget  *spin;
    cs_state_t *state;
    int         rvalue = 0; /* resource value */
    gdouble     dvalue = 0; /* resource value converted to display value */

    spin  = gtk_spin_button_new_with_range(display_min, display_max, display_step);
    state = cs_state_new(resource_name, resource_min, resource_max,
                         display_min, display_max, display_step, display_format);

    /* No resource mediator here, we have to convert between displayed and
     * resource values so most of the mediator's methods won't work. */
    resources_get_int(resource_name, &rvalue);
    dvalue = cs_state_resource_to_display(rvalue, state);
#if 0
    debug_gtk3("Setting \"%s\" to %d (resource), %4.1f (display)",
               resource_name, rvalue, dvalue);
#endif
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), dvalue);
    /* Without this any custom formatting of output will fail and result in an
     * empty widget! */
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), FALSE);
    /* set minimum size of the entry to account for the custom formatting */
    gtk_entry_set_width_chars(GTK_ENTRY(spin), cs_get_minimum_width(state));

    /* connect signal handlers, most don't touch the resource and can be connected
     * "unlocked" */
    g_signal_connect_unlocked(G_OBJECT(spin),
                              "destroy",
                              G_CALLBACK(on_custom_spin_destroy),
                              (gpointer)state);
    g_signal_connect(G_OBJECT(spin),
                     "output",
                     G_CALLBACK(on_custom_spin_output),
                     (gpointer)state);
    g_signal_connect(G_OBJECT(spin),
                     "input",
                     G_CALLBACK(on_custom_spin_input),
                    (gpointer)state);
    g_signal_connect(G_OBJECT(spin),
                     "value-changed",
                     G_CALLBACK(on_custom_spin_value_changed),
                     (gpointer)state);

    return spin;
}


/** \brief  Create a resource-bound spin button with custom range and formatting
 *
 * This version allows using a printf-style format string for the resource name.
 * See the non-printf vice_gtk3_resource_spin_custom_new() for documentation.
 *
 * \param[in]   resource_format resource name format string
 * \param[in]   resource_min    resource minimum value
 * \param[in]   resource_max    resource maximum value
 * \param[in]   display_min     displayed minimum value
 * \param[in]   display_max     displayed maximum value
 * \param[in]   display_step    displayed value stepping
 * \param[in]   display_format  format string for displayed value
 * \param[in]   ...             arguments for the \a resource_format
 *
 * \return  GtkSpinButton
 */
GtkWidget *vice_gtk3_resource_spin_custom_new_sprintf(const gchar *resource_format,
                                                      gint         resource_min,
                                                      gint         resource_max,
                                                      gdouble      display_min,
                                                      gdouble      display_max,
                                                      gdouble      display_step,
                                                      const gchar *display_format,
                                                      ...)
{
    char    resource_name[256];
    va_list args;

    va_start(args, display_format);
    g_vsnprintf(resource_name, sizeof resource_name, resource_format, args);
    va_end(args);

    return vice_gtk3_resource_spin_custom_new(resource_name,
                                              resource_min,
                                              resource_max,
                                              display_min,
                                              display_max,
                                              display_step,
                                              display_format);
}


/*
 * No setters/getters or sync/reset methods, not required at the moment.
 */
