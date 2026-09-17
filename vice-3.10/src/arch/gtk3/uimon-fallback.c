/** \file   uimon-fallback.c
 * \brief   Fallback implementation for the ML-Monitor for when the VTE library
 *          is not available
 *
 * \author  Fabrizio Gennari <fabrizio.ge@tiscali.it>
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

/* FIXME: this code is simple enough to be merged back into uimon.c */

#include "vice.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "console.h"
#include "debug_gtk3.h"
#include "log.h"
#include "monitor.h"
#include "uimon.h"

#include "uimon-fallback.h"


/** \brief  Reference to the log file
 */
static console_t *console_log_local = NULL;


/** \brief  NOP
 *
 * \return  0
 */
int consolefb_close_all(void)
{
    /* This is a no-op on GNOME, should be fine here too */
    return native_console_close_all();
}


/** \brief  NOP
 *
 * \return  0
 */
int consolefb_init(void)
{
    return native_console_init();
}


/** \brief  Output message on native console
 *
 * \param[in]   log     log (unused, console_log_local is used)
 * \param[in]   format  format string
 * \param[in]   ...     arguments for \a format
 *
 * \return 0
 */
int consolefb_out(console_t *log, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    native_console_out(console_log_local, format, ap);
    va_end(ap);

    return 0;
}


/** \brief  Open console window
 *
 * \return  console reference
 */
console_t *uimonfb_window_open(void)
{
    console_log_local = native_console_open(0);
    if (console_log_local == NULL) {
        return NULL;
    }
    /* partially implemented */
    /* INCOMPLETE_IMPLEMENTATION(); */
#ifdef HAVE_MOUSE
    /* ui_restore_mouse(); */
#endif
    /* ui_focus_monitor(); */
    return console_log_local;
}


/** \brief  Close console window
 *
 */
void uimonfb_window_close(void)
{
    if (console_log_local != NULL) {
        native_console_close(console_log_local);
    }

    uimon_window_suspend();
}


/** \brief  Suspend console window
 *
 * Unused.
 */
void uimonfb_window_suspend(void)
{
    /* ui_restore_focus(); */
#ifdef HAVE_MOUSE
    /* ui_check_mouse_cursor(); */
#endif
    NOT_IMPLEMENTED_WARN_ONLY();
}


/** \brief  Resume console window
 *
 * \return  console
 */
console_t *uimonfb_window_resume(void)
{
    if (console_log_local) {
        /* partially implemented */
        INCOMPLETE_IMPLEMENTATION();
#ifdef HAVE_MOUSE
        /* ui_restore_mouse(); */
#endif
        /* ui_focus_monitor(); */
        return console_log_local;
    }
    log_error(LOG_DEFAULT, "uimonfb_window_resume: log was not opened.");
    return uimon_window_open(true);
}


/** \brief  Output string \a buffer to \c console_log_local
 *
 * \param[in]   buffer  message
 *
 * \return 0
 */
int uimonfb_out(const char *buffer)
{
    return native_console_out(console_log_local, "%s", buffer);
}

int uimonfb_petscii_out(const char *buffer, int len)
{
    return native_console_petscii_out(len, console_log_local, "%s", buffer);
}

int uimonfb_petscii_upper_out(const char *buffer, int len)
{
    return native_console_petscii_upper_out(len, console_log_local, "%s", buffer);
}

int uimonfb_scrcode_out(const char *buffer, int len)
{
    return native_console_scrcode_out(len, console_log_local, "%s", buffer);
}

/** \brief  Read a string
 *
 * \param       ppchCommandLine ? (unused)
 * \param[in]   prompt          prompt to display
 *
 * \return  string
 */
char *uimonfb_get_in(char **ppchCommandLine, const char *prompt)
{
    return native_console_in(console_log_local, prompt);
}


/** \brief  NOP
 */
void uimonfb_notify_change(void)
{
}


/** \brief  NOP
 *
 * \param[in]   monitor_interface_init  ? (unused)
 * \param[in]   count                   ? (unused)
 */
void uimonfb_set_interface(struct monitor_interface_s **monitor_interface_init,
                           int count)
{
}
