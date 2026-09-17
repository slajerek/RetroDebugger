/** \file   uimon.c
 * \brief   Native GTK3 UI monitor stuff
 *
 * \author  Fabrizio Gennari <fabrizio.ge@tiscali.it>
 * \author  David Hogan <david.q.hogan@gmail.com>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * TODO:    Properly document this, please.
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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include "novte/novte.h"

#define vte_terminal_new novte_terminal_new
#define vte_terminal_feed novte_terminal_feed
#define vte_terminal_get_column_count novte_terminal_get_column_count
#define vte_terminal_copy_clipboard novte_terminal_copy_clipboard
#define vte_terminal_copy_clipboard_format novte_terminal_copy_clipboard_format
#define vte_terminal_get_row_count novte_terminal_get_row_count
#define vte_terminal_set_scrollback_lines novte_terminal_set_scrollback_lines
#define vte_terminal_set_scroll_on_output novte_terminal_set_scroll_on_output
#define vte_terminal_get_char_width novte_terminal_get_char_width
#define vte_terminal_get_char_height novte_terminal_get_char_height

#define VTE_TERMINAL(x) NOVTE_TERMINAL(x)
#define VteTerminal NoVteTerminal

#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include "archdep.h"
#include "charset.h"
#include "console.h"
#include "debug_gtk3.h"
#include "machine.h"
#include "monitor.h"
#include "mainlock.h"
#include "resources.h"
#include "lib.h"
#include "log.h"
#include "ui.h"
#include "linenoise.h"
#include "unicodehelpers.h"
#include "uimon.h"
#include "uimon-fallback.h"
#include "mon_command.h"
#include "vsync.h"
#include "vsyncapi.h"
#include "widgethelpers.h"
#include "uidata.h"

/*#define DEBUG_UIMON*/

#ifdef DEBUG_UIMON
#define DBG(x)  log_printf
#else
#define DBG(x)
#endif

static gboolean uimon_window_open_impl(gpointer user_data);
static gboolean uimon_window_resume_impl(gpointer user_data);

#define VTE_CSS \
    "vte-terminal { font-size: 300%; }"

/** \brief  Monitor console window object
 *
 * Again, guess work. Someone, not me, should have documented this.
 */
static struct console_private_s {
    pthread_mutex_t lock;

    GtkWidget *window;  /**< windows */
    GtkWidget *term;    /**< could be a VTE instance? */
    char *input_buffer;
    char *output_buffer;
    size_t output_buffer_allocated_size;
    size_t output_buffer_used_size;
} fixed;

#define DEFAULT_COLUMNS     80
#define DEFAULT_ROWS        50

static console_t vte_console = { DEFAULT_COLUMNS, DEFAULT_ROWS, 0, 1, NULL }; /* bare minimum */
static linenoiseCompletions command_lc = {0, NULL};
static linenoiseCompletions need_filename_lc = {0, NULL};

#define FONT_TYPE_ASCII     0
#define FONT_TYPE_C64PRO    1
#define FONT_TYPE_PETME     2 /* https://www.kreativekorp.com/software/fonts/c64/ */
#define FONT_TYPE_PETME64   3 /* https://www.kreativekorp.com/software/fonts/c64/ */
static int font_type = FONT_TYPE_ASCII;

static log_t monui_log = LOG_DEFAULT;

/* FIXME: this should perhaps be done using some function from archdep */
static int is_dir(struct dirent *de)
{
    return 0;
}

static int native_monitor(void)
{
    int res = 0;
    resources_get_int("NativeMonitor", &res);
    return res;
}

static gboolean write_to_terminal(gpointer _)
{
    pthread_mutex_lock(&fixed.lock);

    if (!fixed.term) {
        /* Terminal hasn't been created yet, queue up the write for now. */
        goto done;
    }

    if (fixed.output_buffer) {
        vte_terminal_feed(VTE_TERMINAL(fixed.term), fixed.output_buffer, fixed.output_buffer_used_size);

        lib_free(fixed.output_buffer);
        fixed.output_buffer = NULL;
        fixed.output_buffer_allocated_size = 0;
        fixed.output_buffer_used_size = 0;
    }

done:
    pthread_mutex_unlock(&fixed.lock);

    return FALSE;
}

void uimon_write_to_terminal(struct console_private_s *t,
                             const char *data,
                             glong length)
{
    size_t output_buffer_required_size;
    bool write_scheduled = false;

    pthread_mutex_lock(&fixed.lock);

    /*
     * If the output buffer exists, we've already scheduled a write
     * so we just append to the existing buffer.
     */

    output_buffer_required_size = fixed.output_buffer_used_size + length;

    if (output_buffer_required_size > fixed.output_buffer_allocated_size) {
        output_buffer_required_size += 4096;

        if (fixed.output_buffer) {
            write_scheduled = true;
            fixed.output_buffer = lib_realloc(fixed.output_buffer, output_buffer_required_size);
        } else {
            fixed.output_buffer = lib_malloc(output_buffer_required_size);
        }

        fixed.output_buffer_allocated_size = output_buffer_required_size;
    }

    memcpy(fixed.output_buffer + fixed.output_buffer_used_size, data, length);
    fixed.output_buffer_used_size += length;

    if (!write_scheduled) {
        /* schedule a call on the ui thread */
        gdk_threads_add_timeout(0, write_to_terminal, NULL);
    }

    pthread_mutex_unlock(&fixed.lock);
}

int uimon_out(const char *buffer)
{
    const char *line;
    const char *line_end;

    if (native_monitor()) {
        return uimonfb_out(buffer);
    }

    /* Substitute \n for \r\n when feeding the terminal */

    line = buffer;
    while (*line != '\0') {
        line_end = strchr(line, '\n');

        if (line_end == NULL) {
            /* buffer ends without a \n */
            uimon_write_to_terminal(&fixed, line, strlen(line));
            break;
        }

        uimon_write_to_terminal(&fixed, line, line_end - line);
        uimon_write_to_terminal(&fixed, "\r\n", 2);

        line = line_end + 1;
    }

    return 0;
}

int uimon_petscii_out(const char *buffer, int len)
{
    unsigned char *utf = NULL;
    uint8_t b;
    int n = 0;
    uint8_t c;

    if (native_monitor()) {
        return uimonfb_petscii_out(buffer, len);
    }

    while (n < len) {
        c = buffer[n];

        if (font_type == FONT_TYPE_ASCII) {
            /* regular ASCII font */
            c = charset_p_toascii(c, CONVERT_WITH_CTRLCODES);
        } else if (font_type == FONT_TYPE_C64PRO) {
            /* regular PETSCII capable ("C64 Pro") font */
#if 0
            /* c0-df   AB...|}~ */
            } else if ((c >= 0xc0) && (c <= 0xdf)) {
                if (c == 0xc0) {
                    c = '@';
                } else if (c == 0xdb) {
                    c = '[';
                } else if (c == 0xdd) {
                    c = ']';
#endif
            /* 20-3f  !"#...=>? */
            if (((c >= 0x20) && (c <= 0x2f)) ||
                ((c >= 0x30) && (c <= 0x3f))
            ) {
                /* exclude some ranges from utf conversion */
            /* 40-5a  @ab... */
            } else if (c == 0x40) {
                    c = '@';
            } else if ((c >= 0x41) && (c <= 0x5a)) {
                c += 0x20;
            } else if (c == 0x5b) {
                c = '[';
            } else if (c == 0x5d) {
                c = ']';
            /* 60-7f   AB...|}~ */
            } else if ((c >= 0x61) && (c <= 0x7a)) {
                c -= 0x20;
            } else {
                /* convert the rest to utf8 */
                if (c == 0) {
                    /* 0 is a petscii control code, which we display as inverted @ */
                    c = '@';
                    b = c;
                    utf = vice_gtk3_petscii_to_utf8(&b, true, true);
                    uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
                } else {
                    b = c;
                    utf = vice_gtk3_petscii_to_utf8(&b, false, true);
                    uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
                }
            }
        } else if (font_type == FONT_TYPE_PETME) {
            if (c == 0) {
                /* 0 is a petscii control code, which we display as inverted @ */
                c = '@';
                b = c;
                utf = vice_gtk3_petscii_to_utf8_petme(&b, true, true);
                uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
            } else {
                b = c;
                utf = vice_gtk3_petscii_to_utf8_petme(&b, false, true);
                uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
            }
        } else if (font_type == FONT_TYPE_PETME64) {
            if (c == 0) {
                /* 0 is a petscii control code, which we display as inverted @ */
                c = '@';
                b = c;
                utf = vice_gtk3_petscii_to_utf8_petme64(&b, true, true);
                uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
            } else {
                b = c;
                utf = vice_gtk3_petscii_to_utf8_petme64(&b, false, true);
                uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
            }
        }

        if (utf) {
            lib_free(utf);
            utf = NULL;
        } else {
            uimon_write_to_terminal(&fixed, (const char*)&c, 1);
        }
        n++;
    }

    return 0;
}

int uimon_petscii_upper_out(const char *buffer, int len)
{
    unsigned char *utf = NULL;
    uint8_t b;
    int n = 0;
    uint8_t c;

    if (native_monitor()) {
        return uimonfb_petscii_upper_out(buffer, len);
    }

    while (n < len) {
        c = buffer[n];

        if (font_type == FONT_TYPE_ASCII) {
            /* regular ASCII font */
            c = buffer[n];
            if ((c >= 0x01) && (c <= 0x1a)) {
                c += 0x60; /* lower */
            } else if ((c >= 0x41) && (c <= 0x5a)) {
                c += 0x20; /* upper */
            }
            c = charset_p_toascii(c, CONVERT_WITH_CTRLCODES);
        } else if (font_type == FONT_TYPE_C64PRO) {
            /* regular PETSCII capable ("C64 Pro") font */

            /* 20-3f  !"#...=>? */
            if (((c >= 0x20) && (c <= 0x2f)) ||
                ((c >= 0x30) && (c <= 0x3f))
            ) {
                /* exclude some ranges from utf conversion */
            /* 40-5a  @ab... */
            } else if (c == 0x40) {
                c = '@';
            } else if ((c >= 0x41) && (c <= 0x5a)) {
                c += 0x20;
            } else if (c == 0x5b) {
                c = '[';
            } else if (c == 0x5d) {
                c = ']';
            /* 51-7a  AB... */
            } else if ((c >= 0x61) && (c <= 0x7a)) {
                c -= 0x20;
            } else {
                /* convert the rest to utf8 */
                if (c == 0) {
                    /* 0 is a petscii control code, which we display as inverted @ */
                    c = '@';
                    b = c;
                    utf = vice_gtk3_petscii_upper_to_utf8(&b, true);
                    uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
                } else {
                    b = c;
                    utf = vice_gtk3_petscii_upper_to_utf8(&b, false);
                    uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
                }
            }
        } else if (font_type == FONT_TYPE_PETME) {
            /* FIXME */
            if (c == 0) {
                /* 0 is a petscii control code, which we display as inverted @ */
                c = '@';
                b = c;
                utf = vice_gtk3_petscii_upper_to_utf8_petme(&b, true);
                uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
            } else {
                b = c;
                utf = vice_gtk3_petscii_upper_to_utf8_petme(&b, false);
                uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
            }
        } else if (font_type == FONT_TYPE_PETME64) {
            /* FIXME */
            if (c == 0) {
                /* 0 is a petscii control code, which we display as inverted @ */
                c = '@';
                b = c;
                utf = vice_gtk3_petscii_upper_to_utf8_petme64(&b, true);
                uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
            } else {
                b = c;
                utf = vice_gtk3_petscii_upper_to_utf8_petme64(&b, false);
                uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
            }
        }

        if (utf) {
            lib_free(utf);
            utf = NULL;
        } else {
            uimon_write_to_terminal(&fixed, (const char*)&c, 1);
        }
        n++;
    }

    return 0;
}

/* output screencode, lowercase */
int uimon_scrcode_out(const char *buffer, int len)
{
    int n = 0;
    unsigned char *utf = NULL;
    uint8_t b;
    uint8_t c;

    if (native_monitor()) {
        return uimonfb_scrcode_out(buffer, len);
    }

    while (n < len) {
        c = buffer[n];

        if (font_type == FONT_TYPE_ASCII) {
            /* regular ASCII font */
            c = charset_screencode_to_petscii(c);
            c = charset_p_toascii(c, CONVERT_WITH_CTRLCODES);
        } else if (font_type == FONT_TYPE_C64PRO) {
            /* regular PETSCII capable ("C64 Pro") font */
            b = c;
            if (c == 0x00) {
                b = '@';    /* Hack, this way we sneak the 0 in there */
                utf = vice_gtk3_petscii_to_utf8(&b, c & 0x80 ? true : false, true);
            } else {
                utf = vice_gtk3_scrcode_to_utf8(&b, c & 0x80 ? true : false, true);
            }
            uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
        } else if (font_type == FONT_TYPE_PETME) {
            /* FIXME: regular PETSCII capable ("PET Me") font */
            int ch = c & 0x7f;
            b = ch;
            if (ch == 0x00) {
                b = '@';    /* Hack, this way we sneak the 0 in there */
                utf = vice_gtk3_petscii_to_utf8_petme(&b, c & 0x80 ? true : false, true);
            } else {
                utf = vice_gtk3_scrcode_to_utf8_petme(&b, c & 0x80 ? true : false, true);
            }
            uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
        } else if (font_type == FONT_TYPE_PETME64) {
            /* FIXME: regular PETSCII capable ("PET Me") font */
            int ch = c & 0x7f;
            b = ch;
            if (ch == 0x00) {
                b = '@';    /* Hack, this way we sneak the 0 in there */
                utf = vice_gtk3_petscii_to_utf8_petme64(&b, c & 0x80 ? true : false, true);
            } else {
                utf = vice_gtk3_scrcode_to_utf8_petme64(&b, c & 0x80 ? true : false, true);
            }
            uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
        }

        if (utf) {
            lib_free(utf);
            utf = NULL;
        } else {
            uimon_write_to_terminal(&fixed, (const char*)&c, 1);
        }
        n++;
    }
    return 0;
}

/* output screencode, uppercase */
int uimon_scrcode_upper_out(const char *buffer, int len)
{
    int n = 0;
    unsigned char *utf = NULL;
    uint8_t b;
    uint8_t c;

    if (native_monitor()) {
        return uimonfb_scrcode_out(buffer, len);
    }

    while (n < len) {
        c = buffer[n];

        if (font_type == FONT_TYPE_ASCII) {
            /* regular ASCII font */
            c = charset_screencode_to_petscii(c);
            c = charset_p_toascii(c, CONVERT_WITH_CTRLCODES);
        } else if (font_type == FONT_TYPE_C64PRO) {
            /* regular PETSCII capable ("C64 Pro") font */
            b = c;
            if (c == 0x00) {
                b = '@';    /* Hack, this way we sneak the 0 in there */
                utf = vice_gtk3_petscii_upper_to_utf8(&b, c & 0x80 ? true : false);
            } else {
                utf = vice_gtk3_scrcode_upper_to_utf8(&b, c & 0x80 ? true : false);
            }
            uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
        } else if (font_type == FONT_TYPE_PETME) {
            /* FIXME: regular PETSCII capable ("PET Me") font */
            int ch = c & 0x7f;
            b = ch;
            if (ch == 0x00) {
                b = '@';    /* Hack, this way we sneak the 0 in there */
                utf = vice_gtk3_petscii_upper_to_utf8_petme(&b, c & 0x80 ? true : false);
            } else {
                utf = vice_gtk3_scrcode_upper_to_utf8_petme(&b, c & 0x80 ? true : false);
            }
            uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
        } else if (font_type == FONT_TYPE_PETME64) {
            /* FIXME: regular PETSCII capable ("PET Me") font */
            int ch = c & 0x7f;
            b = ch;
            if (ch == 0x00) {
                b = '@';    /* Hack, this way we sneak the 0 in there */
                utf = vice_gtk3_petscii_upper_to_utf8_petme64(&b, c & 0x80 ? true : false);
            } else {
                utf = vice_gtk3_scrcode_upper_to_utf8_petme64(&b, c & 0x80 ? true : false);
            }
            uimon_write_to_terminal(&fixed, (const char*)&utf[0], 3);
        }

        if (utf) {
            lib_free(utf);
            utf = NULL;
        } else {
            uimon_write_to_terminal(&fixed, (const char*)&c, 1);
        }
        n++;
    }
    return 0;
}

int uimon_get_columns(struct console_private_s *t)
{
    if(t->term) {
        return (int)vte_terminal_get_column_count(VTE_TERMINAL(t->term));
    }
    return DEFAULT_COLUMNS;
}

static char* append_char_to_input_buffer(char *old_input_buffer, char new_char)
{
    char* new_input_buffer = lib_msprintf("%s%c",
        old_input_buffer ? old_input_buffer : "",
        new_char);
    lib_free(old_input_buffer);
    return new_input_buffer;
}

static char* append_string_to_input_buffer(char *old_input_buffer, GtkWidget *terminal, GdkAtom clipboard_to_use)
{
    GtkClipboard *clipboard = gtk_widget_get_clipboard(terminal, clipboard_to_use);
    gchar *new_string = gtk_clipboard_wait_for_text(clipboard);

    if (new_string != NULL) {
        char *new_input_buffer = lib_realloc(old_input_buffer, strlen(old_input_buffer) + strlen(new_string) + 1);
        char *char_in, *char_out = new_input_buffer + strlen(new_input_buffer);

        for (char_in = new_string; *char_in; char_in++) {
#if CHAR_MIN < 0
            if (*char_in < 0 || *char_in >= 32) {
#else
            /* char is unsigned on raspberry Pi 2B with GCC */
            if (*char_in >= 32) {
#endif
                *char_out++ = *char_in;
            } else if (*char_in == 10) {
                *char_out++ = 13;
            }
        }
        *char_out = 0;
        g_free(new_string);

        return new_input_buffer;
    }
    return old_input_buffer;
}

static gboolean plain_key_pressed(char **input_buffer, guint keyval)
{
    switch (keyval) {
        default:
            if(keyval >= GDK_KEY_space && keyval <= GDK_KEY_ydiaeresis) {
                *input_buffer = append_char_to_input_buffer(*input_buffer, (char)keyval);
                return TRUE;
            }
            if(keyval >= GDK_KEY_KP_0 && keyval <= GDK_KEY_KP_9) {
                *input_buffer =
                    append_char_to_input_buffer(
                        *input_buffer,
                        (char)keyval - GDK_KEY_KP_0 + 48);
                return TRUE;
            }
            return FALSE;
        case GDK_KEY_Home:
        case GDK_KEY_KP_Home:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 1);
            return TRUE;
        case GDK_KEY_Left:
        case GDK_KEY_KP_Left:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 2);
            return TRUE;
        case GDK_KEY_Delete:
        case GDK_KEY_KP_Delete:
            /* We use Ctrl+W here to signal linenoise to not exit the monitor
             * when pressing Delete on an empty line, see comment at
             * src/arch/gtk3/linenoise.c:308 --compyx */
            *input_buffer = append_char_to_input_buffer(*input_buffer, 23);
            return TRUE;
        case GDK_KEY_End:
        case GDK_KEY_KP_End:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 5);
            return TRUE;
        case GDK_KEY_Right:
        case GDK_KEY_KP_Right:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 6);
            return TRUE;
        case GDK_KEY_Tab:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 9);
            return TRUE;
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 13);
            return TRUE;
        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 14);
            return TRUE;
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 16);
            return TRUE;
        case GDK_KEY_dead_diaeresis:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 34);
            return TRUE;
        case GDK_KEY_dead_acute:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 39);
            return TRUE;
        case GDK_KEY_KP_Multiply:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 42);
            return TRUE;
        case GDK_KEY_KP_Add:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 43);
            return TRUE;
        case GDK_KEY_KP_Subtract:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 45);
            return TRUE;
        case GDK_KEY_KP_Decimal:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 46);
            return TRUE;
        case GDK_KEY_KP_Divide:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 47);
            return TRUE;
        case GDK_KEY_dead_circumflex:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 94);
            return TRUE;
        case GDK_KEY_dead_grave:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 96);
            return TRUE;
        case GDK_KEY_dead_tilde:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 126);
            return TRUE;
        case GDK_KEY_BackSpace:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 127);
            return TRUE;
    }
}

static gboolean ctrl_plus_key_pressed(char **input_buffer, guint keyval, GtkWidget *terminal)
{
    switch (keyval) {
        default:
            return FALSE;
        case GDK_KEY_h:
        case GDK_KEY_H:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 127);
            return TRUE;
        case GDK_KEY_b:
        case GDK_KEY_B:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 2);
            return TRUE;
        case GDK_KEY_f:
        case GDK_KEY_F:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 6);
            return TRUE;
        case GDK_KEY_p:
        case GDK_KEY_P:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 16);
            return TRUE;
        case GDK_KEY_n:
        case GDK_KEY_N:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 14);
            return TRUE;
        case GDK_KEY_t:
        case GDK_KEY_T:
            *input_buffer = append_char_to_input_buffer(*input_buffer, 20);
            return TRUE;
        case GDK_KEY_d:
        case GDK_KEY_D:
            /* ctrl-d, remove char at right of cursor */
            *input_buffer = append_char_to_input_buffer(*input_buffer, 4);
            return TRUE;
        case GDK_KEY_u:
        case GDK_KEY_U:
            /* Ctrl+u, delete the whole line. */
            *input_buffer = append_char_to_input_buffer(*input_buffer, 21);
            return TRUE;
        case GDK_KEY_k:
        case GDK_KEY_K:
            /* Ctrl+k, delete from current to end of line. */
            *input_buffer = append_char_to_input_buffer(*input_buffer, 11);
            return TRUE;
        case GDK_KEY_a:
        case GDK_KEY_A:
            /* Ctrl+a, go to the start of the line */
            *input_buffer = append_char_to_input_buffer(*input_buffer, 1);
            return TRUE;
        case GDK_KEY_e:
        case GDK_KEY_E:
            /* ctrl+e, go to the end of the line */
            *input_buffer = append_char_to_input_buffer(*input_buffer, 5);
            return TRUE;
#ifndef MACOS_COMPILE
        case GDK_KEY_c:
            vte_terminal_copy_clipboard_format(VTE_TERMINAL(terminal), VTE_FORMAT_ASCII);
            return TRUE;
        case GDK_KEY_C:
            vte_terminal_copy_clipboard_format(VTE_TERMINAL(terminal), VTE_FORMAT_TEXT);
            return TRUE;
        case GDK_KEY_v:
        case GDK_KEY_V:
            *input_buffer = append_string_to_input_buffer(*input_buffer, terminal, GDK_SELECTION_CLIPBOARD);
            return TRUE;
#endif
    }
}

static gboolean on_window_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data);

#ifdef MACOS_COMPILE
static gboolean cmd_plus_key_pressed(char **input_buffer, guint keyval, GtkWidget *terminal)
{
    switch (keyval) {
        default:
            return FALSE;
        case GDK_KEY_c:
            vte_terminal_copy_clipboard_format(VTE_TERMINAL(terminal), VTE_FORMAT_ASCII);
            return TRUE;
        case GDK_KEY_C:
            vte_terminal_copy_clipboard_format(VTE_TERMINAL(terminal), VTE_FORMAT_TEXT);
            return TRUE;
        case GDK_KEY_v:
        case GDK_KEY_V:
            *input_buffer = append_string_to_input_buffer(*input_buffer, terminal, GDK_SELECTION_CLIPBOARD);
            return TRUE;
        case GDK_KEY_w:
        case GDK_KEY_W:
            on_window_delete_event(terminal, NULL, NULL);
            return TRUE;
    }
}
#endif

/** \brief  Handler for the 'key-press-event' event of the the VTE terminal
 *
 * \param[in]   widget      VTE terminal
 * \param[in]   event       event information
 * \param[in]   user_data   extra event data (unused)
 *
 * \return  \c TRUE to stop propagation of event, or \c FALSE to propagate further
 */
static gboolean on_term_key_press_event(GtkWidget   *widget,
                                        GdkEventKey *event,
                                        gpointer     user_data)
{
    GdkModifierType state = 0;
    gboolean retval = FALSE;

    gdk_event_get_state((GdkEvent*)event, &state);

    pthread_mutex_lock(&fixed.lock);

    if (event->type == GDK_KEY_PRESS) {
        /*printf("keyval: %04x state:%04x\n", event->keyval, state);fflush(stdout);*/
#ifdef WINDOWS_COMPILE
        /* extra check for ALT-GR */
        if ((state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)) == (GDK_CONTROL_MASK | GDK_MOD1_MASK)) {
            retval = plain_key_pressed(&fixed.input_buffer, event->keyval);
            goto done;
        }
#endif
        if (state & GDK_CONTROL_MASK) {
            retval = ctrl_plus_key_pressed(&fixed.input_buffer, event->keyval, widget);
            goto done;
        }
#ifdef MACOS_COMPILE
        if (state & GDK_MOD2_MASK) {
            retval = cmd_plus_key_pressed(&fixed.input_buffer, event->keyval, widget);
            goto done;
        }
#endif
        retval = plain_key_pressed(&fixed.input_buffer, event->keyval);
        goto done;
    }

done:
    pthread_mutex_unlock(&fixed.lock);

    return retval;
}

/** \brief  Handler for the 'button-press-event' event of the the VTE terminal
 *
 * \param[in]   widget      VTE terminal
 * \param[in]   event       event information
 * \param[in]   user_data   extra event data (unused)
 *
 * \return  \c TRUE to stop propagation of event, or \c FALSE to propagate further
 */
static gboolean on_term_button_press_event(GtkWidget *widget,
                                           GdkEvent  *event,
                                           gpointer   user_data)
{
    GdkEventButton *button_event = (GdkEventButton*)event;

    if (button_event->button != 2
     || button_event->type   != GDK_BUTTON_PRESS) {
        return FALSE;
    }

    pthread_mutex_lock(&fixed.lock);
    fixed.input_buffer = append_string_to_input_buffer(fixed.input_buffer, widget, GDK_SELECTION_PRIMARY);
    pthread_mutex_unlock(&fixed.lock);

    return TRUE;
}

/** \brief  Handler for the 'delete-event' event of monitor window
 *
 * \param[in]   window      monitor window
 * \param[in]   event       event information (unused)
 * \param[in]   user_data   extra event data (unused)
 *
 * \return  \c TRUE to stop propagating the event further
 */
static gboolean on_window_delete_event(GtkWidget *window,
                                       GdkEvent  *event,
                                       gpointer   user_data)
{
    pthread_mutex_lock(&fixed.lock);

    lib_free(fixed.input_buffer);
    fixed.input_buffer = NULL;

    pthread_mutex_unlock(&fixed.lock);

    gtk_widget_hide(window);
    return TRUE;
}

/* \brief Block until some monitor input event happens */
int uimon_get_string(struct console_private_s *t, char* string, int string_len)
{
    int retval=0;
    while(retval<string_len) {
        int i;

        pthread_mutex_lock(&t->lock);

        if (!t->input_buffer) {
            /* TODO: Not sure if this check makes sense anymore, needs testing without */
            pthread_mutex_unlock(&t->lock);
            return -1;
        }

        if (strlen(t->input_buffer) == 0) {
            /* There's no input yet, so have a little sleep and look again. */
            pthread_mutex_unlock(&t->lock);
            mainlock_yield_and_sleep(tick_per_second() / 60);
            continue;
        }

        for (i = 0; i < strlen(t->input_buffer) && retval < string_len; i++, retval++) {
            string[retval]=t->input_buffer[i];
        }
        memmove(t->input_buffer, t->input_buffer + i, strlen(t->input_buffer) + 1 - i);
        pthread_mutex_unlock(&t->lock);
    }
    return retval;
}

static void get_terminal_size_in_chars(VteTerminal *terminal,
                           glong *width,
                           glong *height)
{
    *width = vte_terminal_get_column_count(terminal);
    *height = vte_terminal_get_row_count(terminal);
}

static void printfontinfo(const PangoFontDescription* desc, const char *name)
{
#if PANGO_VERSION_CHECK(1, 42, 0)
    const char *variations = pango_font_description_get_variations(desc);
#else
    const char *variations = "UNKNOWN(pre-1.42)";
#endif
    log_message(monui_log, "using font '%s' (Family:%s, Size:%d, Variations:%s) PETSCII:%s Terminal Scale:%f",
                name,
                pango_font_description_get_family(desc),
                pango_font_description_get_size(desc) / PANGO_SCALE,
                variations ? variations : "-",
                font_type != FONT_TYPE_ASCII ? "yes" : "no",
                vte_terminal_get_font_scale(VTE_TERMINAL(fixed.term)));
}

static void scale_terminal_set(gdouble scale)
{
    /* use vte scaling */
    if (scale < ((3 * 100.0f) / PANGO_SCALE)) {
        scale = ((3 * 100.0f) / PANGO_SCALE);
    } else if (scale > ((100 * 100.0f) / PANGO_SCALE)) {
        scale = ((100 * 100.0f) / PANGO_SCALE);
    }
    vte_terminal_set_font_scale(VTE_TERMINAL(fixed.term), scale);
}

static void scale_terminal(gdouble delta)
{
    gdouble curr_scaling = vte_terminal_get_font_scale(VTE_TERMINAL(fixed.term));
#if 0
    /* change the font size */
    static int size = -1;
    desc_tmp = vte_terminal_get_font(VTE_TERMINAL(fixed.term));
    desc = pango_font_description_copy_static(desc_tmp);
    if (size == -1) {
        size = (pango_font_description_get_size(desc) / PANGO_SCALE);
        if (size <= 0) {
            size = 11;  /* default fallback size */
        }
    }

    size -= scrollevent.delta_y;
    if (size < 3) {
        size = 3;
    } else if (size > 100) {
        size = 100;
    }

    pango_font_description_set_size(desc, size * PANGO_SCALE);
    vte_terminal_set_font(VTE_TERMINAL(fixed.term), desc);
#else
    /* use vte scaling */
    curr_scaling -= delta;
    if (curr_scaling < ((3 * 100.0f) / PANGO_SCALE)) {
        curr_scaling = ((3 * 100.0f) / PANGO_SCALE);
    } else if (curr_scaling > ((100 * 100.0f) / PANGO_SCALE)) {
        curr_scaling = ((100 * 100.0f) / PANGO_SCALE);
    }
    vte_terminal_set_font_scale(VTE_TERMINAL(fixed.term), curr_scaling);
#endif
}

static gboolean on_term_scrolled(VteTerminal *terminal, GdkEvent  *event,
                                 gpointer      user_data)
{


    const PangoFontDescription *desc_tmp;
    PangoFontDescription* desc;
    char *using_font = NULL;
    GdkEventScroll scrollevent = *((GdkEventScroll*)event);

    if (scrollevent.state & GDK_CONTROL_MASK) {
        /* with control pressed, mouse wheel will scale the terminal/font */
        scale_terminal((scrollevent.delta_y * (100.0f / 3.0f)) / PANGO_SCALE);

        desc_tmp = vte_terminal_get_font(VTE_TERMINAL(fixed.term));
        desc = pango_font_description_copy_static(desc_tmp);

        using_font = pango_font_description_to_string(desc);
        printfontinfo(desc, using_font);

        g_free(using_font);
        return FALSE;
    } else {
        /* mouse wheel scrolls the terminal (scrollback) forth/back */
        GtkAdjustment* vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(fixed.term));
        gdouble value = gtk_adjustment_get_value(vadj);
        value += scrollevent.delta_y * 1;
        gtk_adjustment_set_value(vadj, value);
        gtk_scrollable_set_vadjustment(GTK_SCROLLABLE(fixed.term), vadj);

        return FALSE;
    }

    return TRUE;
}

/** \brief  Handler for the 'text-modified event of the VTE terminal
 *
 * \param[in]   terminal    VTE terminal
 * \param[in]   user_data   extra event data (unused)
 */
static void on_term_text_modified(VteTerminal *terminal,
                                 gpointer      user_data)
{
    glong width, height;
    get_terminal_size_in_chars(terminal, &width, &height);
    vte_console.console_xres = (unsigned int)width;
    vte_console.console_yres = (unsigned int)height;
}

/** \brief  Handler for the 'configure-event' event of the monitor window
 *
 * Resize the terminal when the window is resized, and store monitor window
 * geometry in resources.
 *
 * \param[in]   window      terminal window (unused)
 * \param[in]   event       event information (unused)
 * \param[in]   user_data   extra event data (unused
 */
static void on_window_configure_event(GtkWidget *window,
                                      GdkEvent  *event,
                                      gpointer   user_data)
{
    gint width, height;
    gint xpos;
    gint ypos;
    glong cwidth, cheight;
    glong newwidth, newheight;

    if (!gtk_widget_get_visible(window)) {
        /* bail out, otherwise Gdk will somehow restore the window position
         * the way it was when the window was created and thus we'll get the
         * Xpos/Ypos values from vicerc restored and the changed position will
         * never be saved in the settings.
         */
        return;
    }

    gtk_window_get_size (GTK_WINDOW(fixed.window), &width, &height);
    gtk_window_get_position(GTK_WINDOW(fixed.window), &xpos, &ypos);
    cwidth = vte_terminal_get_char_width (VTE_TERMINAL(fixed.term));
    cheight = vte_terminal_get_char_height (VTE_TERMINAL(fixed.term));

    newwidth = width / cwidth;
    newheight = height / cheight;
    if (newwidth < 1) {
        newwidth = 1;
    }
    if (newheight < 1) {
        newheight = 1;
    }

    if (xpos >= 0 && ypos >= 0) {
        resources_set_int("MonitorXPos", xpos);
        resources_set_int("MonitorYPos", ypos);
    }
    if (width > 0 && height > 0) {
        resources_set_int("MonitorWidth", width);
        resources_set_int("MonitorHeight", height);
    }

    vte_terminal_set_size(VTE_TERMINAL(fixed.term), newwidth, newheight);
    /* printf("on_window_configure_event %lix%li\n", newwidth, newheight); */
    /* update the console size */
    vte_console.console_xres = (unsigned int)newwidth;
    vte_console.console_yres = (unsigned int)newheight;
}

/** \brief  Create an icon by loading it from the vice.gresource file
 *
 * \return  Current emulator's icon
 *
 * \note    If we want something else, we should ask whoever created the current
 *          icon set.
 */
static GdkPixbuf *get_default_icon(void)
{
    char buffer[1024];

    g_snprintf(buffer, sizeof(buffer), "%s.svg", machine_name);
    return uidata_get_pixbuf(buffer);
}

console_t *uimonfb_window_open(void);


/** \brief  Try to set VTE monitor font
 *
 * \return  boolean
 */

static PangoFontDescription* getfontdesc(const char *name)
{
    PangoFontDescription* desc = pango_font_description_from_string(name);

    if (desc == NULL) {
        log_warning(monui_log, "Failed to parse Pango font description '%s'", name);
        return NULL;
    }

    return desc;
}

bool uimon_set_font(void)
{
    const PangoFontDescription *desc_tmp;
    PangoFontDescription* desc;
    const char *monitor_font = NULL;
    char *using_font = NULL;
    GList *widgets;
    GList *box;
    const char *bg;
    const char *fg;
    GdkRGBA color;

    font_type = FONT_TYPE_ASCII;

    if (resources_get_string("MonitorFont", &monitor_font) < 0) {
        log_error(monui_log, "Failed to read 'MonitorFont' resource.");
        return false;
    }

    if (fixed.term == NULL) {
        log_error(monui_log, "No monitor instance found.");
        return false;
    }

    /* NOTE: the Pango API is kindof of weird, in that it never lets us know if
             a font does actually exist, or if a font description (string)
             matches whatever. We can only guess... */

    /* try to set monitor font */
    desc = getfontdesc(monitor_font);
    if (desc == NULL) {
        /* fall back */

        /* if we didn't try to set the default C64 font, try it now */
        if (!strcmp("C64 Pro", monitor_font)) {
            desc = getfontdesc("C64 Pro Mono Regular 9");
        }

        if (desc == NULL) {
            /* last resort, some Monospace 11pt */
            desc_tmp = vte_terminal_get_font(VTE_TERMINAL(fixed.term));
            desc = pango_font_description_copy_static(desc_tmp);
            pango_font_description_set_family(desc, "Mono");
            pango_font_description_set_size(desc, 11 * PANGO_SCALE);
        }
    }
    vte_terminal_set_font(VTE_TERMINAL(fixed.term), desc);

    desc_tmp = vte_terminal_get_font(VTE_TERMINAL(fixed.term));
    using_font = pango_font_description_to_string(desc_tmp);
    if(!strncasecmp("c64 pro", using_font, 7)) {
        log_message(monui_log, "'C64 Pro*' font found, enabling PETSCII output.");
        font_type = FONT_TYPE_C64PRO;
    } else if(!strncasecmp("pet me 64", using_font, 9) ||
              !strncasecmp("pet me 128", using_font, 10)) {
        log_message(monui_log, "'PET Me 64/128*' font found, enabling PETSCII output.");
        font_type = FONT_TYPE_PETME64;
    } else if(!strncasecmp("pet me", using_font, 6)) {
        log_message(monui_log, "'PET Me*' font found, enabling PETSCII output.");
        font_type = FONT_TYPE_PETME;
    }
    scale_terminal_set(1.0f);
    printfontinfo(desc_tmp, using_font);

    if (resources_set_string("MonitorFont", using_font) < 0) {
        log_error(monui_log, "Failed to set 'MonitorFont' resource.");
        return false;
    }

    g_free(using_font);

    pango_font_description_free(desc);

    /* try background color */
    if (resources_get_string("MonitorBG", &bg) < 0) {
        bg = NULL;
    }
    if (gdk_rgba_parse(&color, bg)) {
        vte_terminal_set_color_background(VTE_TERMINAL(fixed.term), &color);
    }

    /* try foreground color */
    if (resources_get_string("MonitorFG", &fg) < 0) {
        fg = NULL;
    }
    if (gdk_rgba_parse(&color, fg)) {
        vte_terminal_set_color_foreground(VTE_TERMINAL(fixed.term), &color);
    }

    gtk_widget_set_size_request(GTK_WIDGET(fixed.window), -1, -1);
    gtk_widget_set_size_request(GTK_WIDGET(fixed.term), -1, -1);

    /* get GtkBox */
    widgets = gtk_container_get_children(GTK_CONTAINER(fixed.window));
    box = g_list_first(widgets);

    gtk_widget_set_size_request(GTK_WIDGET(box->data), -1 , -1);
    return true;
}


/** \brief  Set foreground color
 *
 * Set VTE terminal foreground color to \a color.
 *
 * \param[in]   color   Gdk RGBA color string
 *
 * \return  bool
 *
 * \see     https://docs.gtk.org/gdk3/method.RGBA.parse.html
 */
bool uimon_set_foreground_color(const char *color)
{
    if (fixed.term != NULL) {
        GdkRGBA rgba;

        if (gdk_rgba_parse(&rgba, color)) {
            vte_terminal_set_color_foreground(VTE_TERMINAL(fixed.term), &rgba);
            return true;
        }
    }
    return false;
}


/** \brief  Set foreground color
 *
 * Set VTE terminal foreground color to \a color.
 *
 * \param[in]   color   Gdk RGBA color string
 *
 * \return  bool
 *
 * \see     https://docs.gtk.org/gdk3/method.RGBA.parse.html
 */
bool uimon_set_background_color(const char *color)
{
    if (fixed.term != NULL) {
        GdkRGBA rgba;

        if (gdk_rgba_parse(&rgba, color)) {
            vte_terminal_set_color_background(VTE_TERMINAL(fixed.term), &rgba);
            return true;
        }
    }
    return false;
}

static void mon_set_pos(int xpos, int ypos)
{
    DBG(("mon_set_pos window xpos:%3d ypos:%3d", xpos, ypos));
    if ((xpos == INT_MIN) || (ypos == INT_MIN)) {
        /* Only center if we didn't get either a previous position or
            * the position was set via the command line.
            */
        gtk_window_set_position(GTK_WINDOW(fixed.window), GTK_WIN_POS_CENTER);
    } else {
        gtk_window_move(GTK_WINDOW(fixed.window), xpos, ypos);
    }
}

static void mon_set_size(int width, int height)
{
    DBG(("mon_set_size window resize width:%3d height:%3d", width, height));
    if ((width > 0) && (height > 0)) {
        gtk_window_resize(GTK_WINDOW(fixed.window), width, height);
    }
}

static gboolean uimon_window_open_impl(gpointer user_data)
{
    bool display_now = (bool)user_data;
    GtkWidget *scrollbar, *horizontal_container;
    GdkGeometry hints;
    GdkPixbuf *icon;
    int sblines;
    int xpos = INT_MIN;
    int ypos = INT_MIN;
    int height = 0;
    int width = 0;

    pthread_mutex_lock(&fixed.lock);

    resources_get_int("MonitorScrollbackLines", &sblines);

    if (fixed.window == NULL) {
        fixed.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(fixed.window), "VICE monitor");

        resources_get_int("MonitorXPos", &xpos);
        resources_get_int("MonitorYPos", &ypos);
        resources_get_int("MonitorHeight", &height);
        resources_get_int("MonitorWidth",  &width);
        DBG(("uimon_window_open_impl window xpos:%3d ypos:%3d width:%3d height:%3d",
             xpos, ypos, width, height));

        mon_set_pos(xpos, ypos);
        mon_set_size(width, height);

        /* Set the gravity so that gtk doesn't over-compensate for the
         * window's border width when saving/restoring its position. */
        gtk_window_set_gravity(GTK_WINDOW(fixed.window), GDK_GRAVITY_STATIC);
        gtk_widget_set_app_paintable(fixed.window, TRUE);
        gtk_window_set_deletable(GTK_WINDOW(fixed.window), TRUE);

        /* set a default C= icon for now */
        icon = get_default_icon();
        if (icon != NULL) {
            gtk_window_set_icon(GTK_WINDOW(fixed.window), icon);
        }

        fixed.term = vte_terminal_new();
        vte_terminal_set_scrollback_lines (VTE_TERMINAL(fixed.term), sblines);
        vte_terminal_set_scroll_on_output (VTE_TERMINAL(fixed.term), TRUE);

        /* allowed window widths are base_width + width_inc * N
         * allowed window heights are base_height + height_inc * N
         */
        hints.width_inc = (gint)vte_terminal_get_char_width (VTE_TERMINAL(fixed.term));
        hints.height_inc = (gint)vte_terminal_get_char_height (VTE_TERMINAL(fixed.term));
        /* min size should be multiple of .._inc, else we get funky effects */
        hints.min_width = hints.width_inc;
        hints.min_height = hints.height_inc;
        /* base size should be multiple of .._inc, else we get funky effects */
        hints.base_width = hints.width_inc;
        hints.base_height = hints.height_inc;
        DBG(("uimon_window_open_impl font width:%3d height:%3d", hints.width_inc, hints.height_inc));
        gtk_window_set_geometry_hints (GTK_WINDOW (fixed.window),
                                     fixed.term,
                                     &hints,
                                     GDK_HINT_RESIZE_INC |
                                     GDK_HINT_MIN_SIZE |
                                     GDK_HINT_BASE_SIZE |
                                     GDK_HINT_USER_POS |
                                     GDK_HINT_USER_SIZE);

        if ((xpos == INT_MIN) || (ypos == INT_MIN)) {
            DBG(("uimon_window_open_impl set vte size to defaults"));
            vte_terminal_set_size(VTE_TERMINAL(fixed.term), DEFAULT_COLUMNS, DEFAULT_ROWS);
        }

        scrollbar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL,
                gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(fixed.term)));

        horizontal_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_container_add(GTK_CONTAINER(fixed.window), horizontal_container);

        gtk_box_pack_start(GTK_BOX(horizontal_container), fixed.term,
                TRUE, TRUE, 0);
        gtk_box_pack_end(GTK_BOX(horizontal_container), scrollbar,
                FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(fixed.window),
                        "delete-event",
                        G_CALLBACK(on_window_delete_event),
                        NULL);

        g_signal_connect_unlocked(G_OBJECT(fixed.term),
                                  "key-press-event",
                                  G_CALLBACK(on_term_key_press_event),
                                  NULL);

        g_signal_connect_unlocked(G_OBJECT(fixed.term),
                                  "button-press-event",
                                  G_CALLBACK(on_term_button_press_event),
                                  NULL);

        g_signal_connect_unlocked(G_OBJECT(fixed.term),
                                  "text-modified",
                                  G_CALLBACK(on_term_text_modified),
                                  NULL);

        g_signal_connect_unlocked(G_OBJECT(fixed.term),
                                  "scroll-event",
                                  G_CALLBACK(on_term_scrolled),
                                  NULL);

        /* can this actually be connected unlocked, we're setting resources here? */
        g_signal_connect_unlocked(G_OBJECT(fixed.window),
                                  "configure-event",
                                  G_CALLBACK(on_window_configure_event),
                                  NULL);

        vte_console.console_can_stay_open = 1;
        vte_console.console_cannot_output = 0;

        uimon_set_font();
    } else {
        vte_terminal_set_scrollback_lines (VTE_TERMINAL(fixed.term), sblines);
    }

    pthread_mutex_unlock(&fixed.lock);

    if (display_now) {
        uimon_window_resume_impl(NULL);
    }

    /* Ensure any queued monitor output is displayed */
    gdk_threads_add_timeout(0, write_to_terminal, NULL);
    return FALSE;
}

static gboolean uimon_window_resume_impl(gpointer user_data)
{
    int xpos = INT_MIN;
    int ypos = INT_MIN;
    int height = 0;
    int width = 0;

    resources_get_int("MonitorXPos",   &xpos);
    resources_get_int("MonitorYPos",   &ypos);
    resources_get_int("MonitorHeight", &height);
    resources_get_int("MonitorWidth",  &width);
    DBG(("uimon_window_resume_impl window xpos:%3d ypos:%3d width:%3d height:%3d",
         xpos, ypos, width, height));

    gtk_widget_show_all(fixed.window);
    on_term_text_modified(VTE_TERMINAL(fixed.term), NULL);

    mon_set_pos(xpos, ypos);

#if 0
    /* BUG: this makes it hang first time the monitor window is opened */
    mon_set_size(height, width);
#endif

    /*
     * Make the monitor window appear on top of the active emulated machine
     * window. This makes the monitor window show when the emulated machine
     * window is in fullscreen mode. (only tested on Windows 10)
     */
    gtk_window_present(GTK_WINDOW(fixed.window));
    return FALSE;
}

static gboolean uimon_window_suspend_impl(gpointer user_data)
{
    if (fixed.window != NULL) {
        int keep_open = 0;

        /* do need to keep the monitor window open? */
        resources_get_int("KeepMonitorOpen", &keep_open);
        if (!keep_open) {
            gtk_widget_hide(fixed.window);
        } else {
            /* move monitor window behind the emu window */
            GtkWidget *window = ui_get_window_by_index(ui_get_main_window_index());
            gtk_window_present(GTK_WINDOW(window));
        }
    }

    return FALSE;
}

static gboolean uimon_window_close_impl(gpointer user_data)
{
    /* Flush any queued writes */
    write_to_terminal(NULL);

    /* only close window if there is one: this avoids a GTK_CRITICAL warning
     * when using a remote monitor */
    if (fixed.window != NULL) {
        gtk_widget_hide(fixed.window);
    }

    return FALSE;
}

console_t *uimon_window_open(bool display_now)
{
    if (monui_log == LOG_DEFAULT) {
        monui_log = log_open("Monitor UI");
    }

    if (native_monitor()) {
        return uimonfb_window_open();
    }

    /* call from ui thread */
    gdk_threads_add_timeout(0, uimon_window_open_impl, (gpointer)display_now);

    return &vte_console;
}

console_t *uimon_window_resume(void)
{
    if (native_monitor()) {
        return uimonfb_window_resume();
    }

    /* call from ui thread */
    gdk_threads_add_timeout(0, uimon_window_resume_impl, NULL);

    return &vte_console;
}

void uimon_window_suspend(void)
{
    if (native_monitor()) {
        uimonfb_window_suspend();
        return;
    }

    /* call from ui thread */
    gdk_threads_add_timeout(0, uimon_window_suspend_impl, NULL);
}

void uimon_window_close(void)
{
    if (native_monitor()) {
        uimonfb_window_close();
        return;
    }

    /* call from ui thread */
    gdk_threads_add_timeout(0, uimon_window_close_impl, NULL);
}

void uimon_notify_change(void)
{
    if (native_monitor()) {
        uimonfb_notify_change();
        return;
    }
}

void uimon_set_interface(struct monitor_interface_s **interf, int i)
{
    if (native_monitor()) {
        uimonfb_set_interface(interf, i);
        return;
    }
}

static char* concat_strings(const char *string1, int nchars, const char *string2)
{
    char *ret = lib_malloc(nchars + strlen(string2) + 1);
    memcpy(ret, string1, nchars);
    strcpy(ret + nchars, string2);
    return ret;
}

static void fill_completions(const char *string_so_far, int initial_chars, int token_len, const linenoiseCompletions *possible_lc, linenoiseCompletions *lc)
{
    int word_index;

    lc->len = 0;
    for(word_index = 0; word_index < possible_lc->len; word_index++) {
        int i;
        for(i = 0; i < token_len; i++) {
            if (string_so_far[initial_chars + i] != possible_lc->cvec[word_index][i]) {
                break;
            }
        }
        if (i == token_len && possible_lc->cvec[word_index][token_len] != 0) {
            char *string_to_append = concat_strings(string_so_far, initial_chars, possible_lc->cvec[word_index]);
            vte_linenoiseAddCompletion(lc, string_to_append);
            lib_free(string_to_append);
        }
    }
}

static void find_next_token(const char *string_so_far, int start_of_search, int *start_of_token, int *token_len)
{
    for(*start_of_token = start_of_search; string_so_far[*start_of_token] && isspace((unsigned char)(string_so_far[*start_of_token])); (*start_of_token)++);
    for(*token_len = 0; string_so_far[*start_of_token + *token_len] && !isspace((unsigned char)(string_so_far[*start_of_token + *token_len])); (*token_len)++);
}

static gboolean is_token_in(const char *string_so_far, int token_len, const linenoiseCompletions *lc)
{
    int i;
    for(i = 0; i < lc->len; i++) {
        if(strlen(lc->cvec[i]) == token_len && !strncmp(string_so_far, lc->cvec[i], token_len)) {
            return TRUE;
        }
    }
    return FALSE;
}

static void monitor_completions(const char *string_so_far, linenoiseCompletions *lc)
{
    int start_of_token, token_len;
    char *help_commands[] = {"help", "?"};
    const linenoiseCompletions help_lc = {
         sizeof(help_commands)/sizeof(*help_commands),
         help_commands
    };

    find_next_token(string_so_far, 0, &start_of_token, &token_len);
    if (!string_so_far[start_of_token + token_len]) {
         fill_completions(string_so_far, start_of_token, token_len, &command_lc, lc);
         return;
    }
    if (is_token_in(string_so_far + start_of_token, token_len, &help_lc)) {
        find_next_token(string_so_far, start_of_token + token_len, &start_of_token, &token_len);
        if (!string_so_far[start_of_token + token_len]){
             fill_completions(string_so_far, start_of_token, token_len, &command_lc, lc);
             return;
        }
    }
    if (is_token_in(string_so_far + start_of_token, token_len, &need_filename_lc)) {
        int start_of_path;
        DIR* dir;
        struct dirent *direntry;
        struct linenoiseCompletions files_lc = {0, NULL};
        int i;

        for (start_of_token += token_len; string_so_far[start_of_token] && isspace((unsigned char)(string_so_far[start_of_token])); start_of_token++);
        if (string_so_far[start_of_token] != '"') {
            char *string_to_append = concat_strings(string_so_far, start_of_token, "\"");
            vte_linenoiseAddCompletion(lc, string_to_append);
            lib_free(string_to_append);
            return;
        }
        for (start_of_path = ++start_of_token, token_len = 0; string_so_far[start_of_token + token_len]; token_len++) {
            if(string_so_far[start_of_token + token_len] == '"'
            && string_so_far[start_of_token + token_len - 1] != '\\') {
                return;
            }
            if(string_so_far[start_of_token + token_len] == '/') {
                start_of_token += token_len + 1;
                token_len = -1;
            }
        }
        if (start_of_token == start_of_path) {
            dir = opendir(".");
        } else {
            char *path = concat_strings(string_so_far + start_of_path, start_of_token - start_of_path, "");
            dir = opendir(path);
            lib_free(path);
        }
        if (dir) {
            for (direntry = readdir(dir); direntry; direntry = readdir(dir)) {
                if (strcmp(direntry->d_name, ".") && strcmp(direntry->d_name, "..")) {
                    char *entryname = lib_msprintf("%s%s", direntry->d_name, is_dir(direntry) ? "/" : "\"");
                    vte_linenoiseAddCompletion(&files_lc, entryname);
                    lib_free(entryname);
                }
            }
            fill_completions(string_so_far, start_of_token, token_len, &files_lc, lc);
            for(i = 0; i < files_lc.len; i++) {
                free(files_lc.cvec[i]);
            }
            closedir(dir);
            return;
        }
    }
}

char *uimon_get_in(char **ppchCommandLine, const char *prompt)
{
    char *p, *ret_string;

    if (native_monitor()) {
        return uimonfb_get_in(ppchCommandLine, prompt);
    }

    pthread_mutex_lock(&fixed.lock);
    if (!fixed.input_buffer) {
        fixed.input_buffer = lib_strdup("");
    }
    pthread_mutex_unlock(&fixed.lock);

    vte_linenoiseSetCompletionCallback(monitor_completions);
    p = vte_linenoise(prompt, &fixed);
    if (p) {
        if (*p) {
            vte_linenoiseHistoryAdd(p);
        }
        ret_string = lib_strdup(p);
        free(p);
    } else {
        ret_string = NULL;
    }

    return ret_string;
}

int console_init(void)
{
    int i = 0;
    char *full_name;
    char *short_name;
    int takes_filename_as_arg;
    pthread_mutexattr_t lock_attributes;

    /* our console lock needs to be recursive */
    pthread_mutexattr_init(&lock_attributes);
    pthread_mutexattr_settype(&lock_attributes, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&fixed.lock, &lock_attributes);

    if (native_monitor()) {
        return consolefb_init();
    }

    while (mon_get_nth_command(i++,
                &full_name,
                &short_name,
                &takes_filename_as_arg)) {
        if (strlen(full_name)) {
            vte_linenoiseAddCompletion(&command_lc, full_name);
            if (strlen(short_name)) {
                vte_linenoiseAddCompletion(&command_lc, short_name);
            }
            if (takes_filename_as_arg) {
                vte_linenoiseAddCompletion(&need_filename_lc, full_name);
                if (strlen(short_name)) {
                    vte_linenoiseAddCompletion(&need_filename_lc, short_name);
                }
            }
        }
    }
    return 0;
}

int console_close_all(void)
{
    int i;

    pthread_mutex_lock(&fixed.lock);

    if (fixed.input_buffer) {
        /* This happens if the application exits with the monitor open, as the VICE thread
         * exits while the monitor is waiting for user input into this buffer.
         */
        lib_free(fixed.input_buffer);
        fixed.input_buffer = NULL;
    }

    pthread_mutex_unlock(&fixed.lock);

    if (native_monitor()) {
        return consolefb_close_all();
    }

    for(i = 0; i < command_lc.len; i++) {
        free(command_lc.cvec[i]);
    }
    for(i = 0; i < need_filename_lc.len; i++) {
        free(need_filename_lc.cvec[i]);
    }

    return 0;
}
