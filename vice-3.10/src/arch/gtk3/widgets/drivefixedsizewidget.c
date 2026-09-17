/** \file   drivefixedsizewidget.c
 * \brief   Drive fixed size widget
 *
 * Used to set a fixed size for a CMDHD drive image
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES Drive8FixedSize     -vsid
 * $VICERES Drive9FixedSize     -vsid
 * $VICERES Drive10FixedSize    -vsid
 * $VICERES Drive11FixedSize    -vsid
 *
 *  (for more, see used widgets)
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

#include "vice_gtk3.h"
#include "resources.h"

#include "types.h"
#include "drivefixedsizewidget.h"


/** \brief  Create widget to set a fixed size for \a unit
 *
 * \param[in]   unit    drive unit number (8-11)
 *
 * \return  GtkEntry
 */
GtkWidget *drive_fixed_size_widget_create(int unit)
{
    GtkWidget *entry;
    char resource[1024];

    /* create entry for drive-specific resource */
    g_snprintf(resource, sizeof(resource), "Drive%dFixedSize", unit);
    entry = vice_gtk3_resource_numeric_string_new(resource);
    /* set limits */
    vice_gtk3_resource_numeric_string_set_limits(entry,
                                                 75 * 1024,
                                                 UINT64_MAX,
                                                 TRUE);
    /* set unit number */
    g_object_set_data(G_OBJECT(entry), "Unit", GINT_TO_POINTER(unit));
    /* add tooltip* */
    gtk_widget_set_tooltip_markup(entry,
            "Minimum size is 76800 bytes. Suffixes <b>K</b> (KiB), <b>M</b>"
            " (MiB) and <b>G</b> (GiB) are allowed.");
    return entry;
}
