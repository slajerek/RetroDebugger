/** \file   resourcewidgetmediator.h
 * \brief   Resource widget mediator - header
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 *
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

#ifndef VICE_RESOURCEWIDGETSTATE_H
#define VICE_RESOURCEWIDGETSTATE_H

#include <gtk/gtk.h>


/** \brief  Resource widget mediator object
 */
typedef struct mediator_s {
    /** \brief  Resource name */
    char      *name;

    /** \brief  Resource type */
    GType      type;

    /** \brief  Initial value of resource
     *
     * Used to restore resource to mediator on widget instanciation when calling
     * a reset() method.
     */
    GValue     initial;

    /** \brief  Current (valid) resource value
     *
     * Used to revert widget to its previous valid mediator if setting the resource
     * fails.
     */
    GValue     current;

    /** \brief  Reference to the widget */
    GtkWidget *widget;

    /** \brief  ID of the destroy signal handler set by mediator_new() */
    gulong     destroy;

    /** \brief  ID of the 'changed'/'toggled'/'set-state' signal of the widget
     *
     * ID of the signal handler that is used to react to widget changes to
     * update the resource. Can be used to have the mediator block and unblock
     * the handler.
     */
    gulong     handler;

    /** \brief  Additional callback for the user of the widget (optional) */
    union {
        void (*b)(GtkWidget*, gboolean);
        void (*i)(GtkWidget*, int);
        void (*s)(GtkWidget*, const char*);
    } callback;

    /** \brief  Object with additional data for the widget
     *
     * If a widget needs to keep additional data around this pointer can be
     * used to keep a reference to the additional data and free it on widget
     * destruction.
     */
    void      *data;

    /** \brief  Function to call to free additional data
     *
     * Function that frees \a data and its members on widget destruction.
     */
    void     (*data_free)(void*);
} mediator_t;


mediator_t *mediator_new                   (GtkWidget  *widget,
                                            const char *name,
                                            GType       type);
GtkWidget  *mediator_get_widget            (mediator_t *mediator);
mediator_t *mediator_for_widget            (GtkWidget *widget);
const char *mediator_get_name              (mediator_t *mediator);
const char *mediator_get_name_w            (GtkWidget *widget);

void        mediator_set_data              (mediator_t *mediator,
                                            void       *data,
                                            void (*data_free)(void*));
void       *mediator_get_data              (mediator_t *mediator);
void       *mediator_get_data_w            (GtkWidget *widget);

void        mediator_set_handler           (mediator_t *mediator, gulong handler);
void        mediator_handler_block         (mediator_t *mediator);
void        mediator_handler_unblock       (mediator_t *mediator);

gboolean    mediator_get_resource_boolean  (mediator_t *mediator);
gboolean    mediator_get_initial_boolean   (mediator_t *mediator);
gboolean    mediator_get_factory_boolean   (mediator_t *mediator);
gboolean    mediator_get_current_boolean   (mediator_t *mediator);
void        mediator_set_current_boolean   (mediator_t *mediator, gboolean value);
void        mediator_set_callback_boolean  (mediator_t *mediator,
                                            void (*callback)(GtkWidget*, gboolean));
void        mediator_set_callback_boolean_w(GtkWidget *widget,
                                            void (*callback)(GtkWidget*, gboolean));

gboolean    mediator_update_boolean        (mediator_t *mediator, gboolean value);
gboolean    mediator_update_boolean_w      (GtkWidget *widget, gboolean value);

int         mediator_get_resource_int      (mediator_t *mediator);
int         mediator_get_initial_int       (mediator_t *mediator);
int         mediator_get_factory_int       (mediator_t *mediator);
int         mediator_get_current_int       (mediator_t *mediator);
void        mediator_set_current_int       (mediator_t *mediator, int value);
void        mediator_set_callback_int      (mediator_t *mediator,
                                            void (*callback)(GtkWidget*, int));
gboolean    mediator_update_int            (mediator_t *mediator, int value);
gboolean    mediator_update_int_w          (GtkWidget *widget, int value);

#if 0
const char *mediator_get_initial_string    (mediator_t *mediator);
#endif
const char *mediator_get_resource_string   (mediator_t *mediator);
const char *mediator_get_initial_string    (mediator_t *mediator);
const char *mediator_get_factory_string    (mediator_t *mediator);
const char *mediator_get_current_string    (mediator_t *mediator);
void        mediator_set_current_string    (mediator_t *mediator, const char *value);
void        mediator_set_callback_string   (mediator_t *mediator,
                                            void (*callback)(GtkWidget*, const char*));
void        mediator_set_callback_string_w (GtkWidget *widget,
                                            void (*callback)(GtkWidget*, const char*));

gboolean    mediator_update_string         (mediator_t *mediator, const char *value);
gboolean    mediator_update_string_w       (GtkWidget *widget, const char *value);

#endif
