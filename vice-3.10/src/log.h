/*
 * log.h - Logging facility.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_LOG_H
#define VICE_LOG_H

#include <stdio.h>

/* values passed into the log helper (log_out->log_helper) */
#define LOG_LEVEL_NONE      0x00
#define LOG_LEVEL_FATAL     0x20
#define LOG_LEVEL_ERROR     0x40
#define LOG_LEVEL_WARNING   0x60
#define LOG_LEVEL_INFO      0x80
#define LOG_LEVEL_VERBOSE   0xa0
#define LOG_LEVEL_DEBUG     0xc0
#define LOG_LEVEL_ALL       0xff

/* values used to set the log level (log_set_limit, log_set_limit_early) */

/* errors only */
#define LOG_LIMIT_SILENT    (LOG_LEVEL_WARNING - 1)
/* all messages, except verbose+debug */
#define LOG_LIMIT_STANDARD  (LOG_LEVEL_VERBOSE - 1)
/* all messages, except debug */
#define LOG_LIMIT_VERBOSE   (LOG_LEVEL_DEBUG - 1)
/* all messages */
#define LOG_LIMIT_DEBUG     (LOG_LEVEL_ALL)

int log_set_limit_early(int n);
int log_early_init(int argc, char **argv);

int log_set_limit(int n);
int log_get_limit(void);

int log_resources_init(void);
void log_resources_shutdown(void);
int log_cmdline_options_init(void);

/* init/open the log file */
int log_init(void);
int log_init_with_fd(FILE *f);

/* for individual log streams */
typedef signed int log_t;
#define LOG_DEFAULT ((log_t)-1)

log_t log_open(const char *id);
int log_close(log_t log);
void log_close_all(void);

/* actual log functions */

/* foreground colors */
#define LOG_COL_BLACK    "\x1B[30;40m"
#define LOG_COL_RED      "\x1B[31;40m"
#define LOG_COL_GREEN    "\x1B[32;40m"
#define LOG_COL_YELLOW   "\x1B[33;40m"
#define LOG_COL_BLUE     "\x1B[34;40m"
#define LOG_COL_MAGENTA  "\x1B[35;40m"
#define LOG_COL_CYAN     "\x1B[36;40m"
#define LOG_COL_WHITE    "\x1B[37;40m"
#define LOG_COL_LBLACK   "\x1B[90;40m"
#define LOG_COL_LRED     "\x1B[91;40m"
#define LOG_COL_LGREEN   "\x1B[92;40m"
#define LOG_COL_LYELLOW  "\x1B[93;40m"
#define LOG_COL_LBLUE    "\x1B[94;40m"
#define LOG_COL_LMAGENTA "\x1B[95;40m"
#define LOG_COL_LCYAN    "\x1B[96;40m"
#define LOG_COL_LWHITE   "\x1B[97;40m"

#define LOG_COL_OFF      "\x1B[0m"

int log_out(log_t log, unsigned int level, const char *format, ...) VICE_ATTR_PRINTF3;

int log_debug(log_t log, const char *format, ...) VICE_ATTR_PRINTF2;
int log_verbose(log_t log, const char *format, ...) VICE_ATTR_PRINTF2;
int log_message(log_t log, const char *format, ...) VICE_ATTR_PRINTF2;
int log_warning(log_t log, const char *format, ...) VICE_ATTR_PRINTF2;
int log_error(log_t log, const char *format, ...) VICE_ATTR_PRINTF2;
int log_fatal(log_t log, const char *format, ...) VICE_ATTR_PRINTF2;

/* simple way to print to the default log, at debug level */
int log_printf(const char *format, ...) VICE_ATTR_PRINTF;

#endif
