/** \file   debug_gtk3.h
 * \brief   Gtk3 port debugging code - header
 *
 * This module provides some macros to be used to print debugging output on
 * stdout/stderr in the Gtk3 port when configure was passed the
 * `--enable-debug-gtk3ui` flag.
 * Hopefully at some point the NOT_IMPLEMENTED/INCOMPLETE_IMPLEMENTATION stuff
 * won't be needed anymore.
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

#ifndef VICE_DEBUG_GTK3_H
# define VICE_DEBUG_GTK3_H

# include "vice.h"
# include <stdio.h>
# include <stdlib.h>



/* Doxygen docblocks go here to avoid having to write them two times for each
 * case for `HAVE_DEBUG_GTK3UI`.
 */

/** \def debug_gtk3
 * \brief  Print debugging info on stdout
 *
 * Works just like g_print() and printf(), except that every line is prefixed
 * with "[debug-gtk3] $FILE:$LINE::$FUNC(): ".
 * This macro outputs a newline. so the user should not provide one in the
 * message, unless an extra newline is preferred.
 */

/** \def NOT_IMPLEMENTED_WARN_ONLY
 * \brief  Not-implemented message with file, function and lineno, only warns
 */

/** \def NOT_IMPLEMENTED_WARN_X_TIMES
 * \brief  Not-implemented message, shown at most X times
 *
 * This macro limits the number of 'not implemented' messages appearing on
 * stderr, so the terminal debug output doesn't get flooded.
 *
 * Usage: declare a `static int foo_msgs` somewhere and then in the function you
 *        want to limit the number of messages it spits out, use this macro:
 *        NOT_IMPLEMENTED_WARN_X_TIMES(foo_msgs, 5);    (warn 5 times at most)
 *
 * \param[in,out]   C   counter variable (int)
 * \param[in]       X   maximum number of times to show the warning (int)
 */

/** \def NOT_IMPLEMENTED
 * \brief  Not-implemented message with file, function and lineno, calls exit(1)
 *
 * This one should NOT depend on HAVE_DEBUG_GTK3UI since it calls exit(1), to
 * avoid a 'mysterious' exit/crash without any information.
 */

/** \def INCOMPLETE_IMPLEMENTATION
 * \brief  Incomplete implementation message, only warns
 */

/** \def TEMPORARY_IMPLEMENTATION
 * \brief  Temporary implementation message, only warns
 */


/* HAVE_DEBUG_GTK3UI comes from configure */
# ifdef HAVE_DEBUG_GTK3UI
#  include <glib.h>

#  define debug_gtk3(...) \
    g_print("[debug-gtk3] %s:%d::%s(): ", \
            debug_gtk3_basename(__FILE__), __LINE__, __func__); \
    g_print(__VA_ARGS__); \
    g_print("\n");

#  define NOT_IMPLEMENTED_WARN_ONLY() \
    fprintf(stderr, \
            "%s:%d: warning: function %s() is not implemented yet, continuing\n", \
            __FILE__, __LINE__, __func__)

#  define NOT_IMPLEMENTED_WARN_X_TIMES(C, X) \
    if ((C)++ < (X)) { \
        fprintf(stderr, \
                "%s:%d: warning function %s() is not implemented yet, " \
                "warning %d/%d\n", \
                __FILE__, __LINE__, __func__, (C), (X)); \
    }

#  define INCOMPLETE_IMPLEMENTATION() \
    fprintf(stderr, \
            "%s:%d: warning: function %s() is not fully implemented yet, continuing\n", \
            __FILE__, __LINE__, __func__)


#  define TEMPORARY_IMPLEMENTATION() \
    fprintf(stderr, \
            "%s:%d: warning: function %s() contains a temporary implementation, continuing\n", \
            __FILE__, __LINE__, __func__)


# else  /* HAVE_DEBUG_GTK3UI */

/*  Empty placeholders */
#  define debug_gtk3(...)
#  define NOT_IMPLEMENTED_WARN_ONLY()
#  define NOT_IMPLEMENTED_WARN_X_TIMES(C, X)
#  define INCOMPLETE_IMPLEMENTATION()
#  define TEMPORARY_IMPLEMENTATION()

# endif /* HAVE DEBUG_GTK3UI */


# define NOT_IMPLEMENTED() \
    fprintf(stderr, \
            "%s:%d: error: function %s() is not implemented yet, exiting\n", \
            __FILE__, __LINE__, __func__); \
    exit(1)


const char *debug_gtk3_basename(const char *path);

#endif  /* VICE_DEBUG_GTK3_H */
