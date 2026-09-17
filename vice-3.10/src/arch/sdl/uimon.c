/*
 * uimon.c - Monitor access interface.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
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

#include "vice.h"

#include "charset.h"
#include "console.h"
#if defined(UNIX_COMPILE)
#include "fullscreenarch.h"
#endif
#include "lib.h"
#include "machine.h"
#include "monitor.h"
#include "uimon.h"
#include "ui.h"
#include "uifonts.h"
#include "uimenu.h"
#include "videoarch.h"

static console_t mon_console = {
    40,
    25,
    0,
    0,
    NULL
};

static menu_draw_t *menu_draw = NULL;

static int x_pos = 0;

static console_t *console_log_local = NULL;

#ifdef ALLOW_NATIVE_MONITOR
static int using_ui_monitor = 0;
#else
static const int using_ui_monitor = 1;
#endif

void uimon_window_close(void)
{
    if (using_ui_monitor) {
        if (menu_draw) {
            sdl_ui_activate_post_action();
        }
        if (machine_class == VICE_MACHINE_VSID) {
            memset(sdl_active_canvas->draw_buffer_vsid->draw_buffer, 0, sdl_active_canvas->draw_buffer_vsid->draw_buffer_width * sdl_active_canvas->draw_buffer_vsid->draw_buffer_height);
            sdl_ui_refresh();
        }
    } else {
        native_console_close(console_log_local);
        console_log_local = NULL;
    }
}

console_t *uimon_window_open(bool display_now)
{
    /* TODO: something with display_now for SDL. It's set to false at startup when -moncommands is used. */

#ifdef ALLOW_NATIVE_MONITOR
    using_ui_monitor = !native_monitor || sdl_active_canvas->fullscreenconfig->enable;
#endif

    if (using_ui_monitor) {
        sdl_ui_activate_pre_action();
        sdl_ui_init_draw_params(sdl_active_canvas);
        sdl_ui_clear();
        menu_draw = sdl_ui_get_menu_param();
        mon_console.console_xres = menu_draw->max_text_x;
        mon_console.console_yres = menu_draw->max_text_y;
        x_pos = 0;
        return &mon_console;
    } else {
        console_log_local = native_console_open("Monitor");
        return console_log_local;
    }
}

void uimon_window_suspend(void)
{
    if (using_ui_monitor && menu_draw) {
        uimon_window_close();
    }
}

console_t *uimon_window_resume(void)
{
    if (using_ui_monitor && menu_draw) {
        return uimon_window_open(true);
    } else {
        return console_log_local;
    }
}

int uimon_out(const char *buffer)
{
    int rc = 0;

    char *buf = lib_strdup(buffer);

    if (using_ui_monitor && menu_draw) {
        int y = menu_draw->max_text_y - 1;
        char *p = buf;
        int i = 0;
        char c;

        sdl_ui_set_active_font(MENU_FONT_ASCII);

        while ((c = p[i]) != 0) {
            if (c == '\n') {
                p[i] = 0;
                sdl_ui_print(p, x_pos, y);
                sdl_ui_scroll_screen_up();
                x_pos = 0;
                p += i + 1;
                i = 0;
            } else if (c == '\t') {
                /* replace tabs with a single space, so weird 'i' chars don't
                 * show up for tabs (we don't have a lot of room in a 40-col
                 * display, so expanding these tabs won't do much good)
                 */
                p[i++] = ' ';
            } else {
                ++i;
            }
        }

        if (p[0] != 0) {
            x_pos += sdl_ui_print(p, x_pos, y);
        }

        /* sdl_ui_set_active_font(MENU_FONT_ASCII); */

    } else {
        if (console_log_local) {
            rc = native_console_out(console_log_local, "%s", buffer);
        }
    }

    lib_free(buf);

    return rc;
}

/* petscii layout:
 00 @abcdefghijklmnopqrstuvwxyz[ ]^     inverted (ctrl)
 20  !"#$%&'()*+,-./0123456789:;<=>?
 40 @abcdefghijklmnopqrstuvwxyz[ ]^
 60  !"#$%&'()*+,-./0123456789:;<=>?
 80  ABCDEFGHIJKLMNOPQRSTUVWXYZ.....    inverted (Ctrl)
 A0 ................................
 C0  ABCDEFGHIJKLMNOPQRSTUVWXYZ.....
 E0 ................................
*/
/* monitor charset:
 00 @abcdefghijklmnopqrstuvwxyz[ ]^     inverted
 20  !"#$%&'()*+,-./0123456789:;<=>?
 40 @abcdefghijklmnopqrstuvwxyz[ ]^
 60  ABCDEFGHIJKLMNOPQRSTUVWXYZ.....
 80  ABCDEFGHIJKLMNOPQRSTUVWXYZ.....    inverted
 A0 ................................
 C0  ABCDEFGHIJKLMNOPQRSTUVWXYZ.....
 E0 ................................
 */

int uimon_petscii_out(const char *buffer, int num)
{
    int rc = 0;

    if (using_ui_monitor) {
        int y = menu_draw->max_text_y - 1;
        const char *p = buffer;
        int i = 0;
        uint8_t c;

        sdl_ui_set_active_font(MENU_FONT_MONITOR);

        /* CAUTION: there is another level of indirection going on in uimenu.c */
        while (i < num) {
            c = p[i];
            /* 00-1f control codes 00-1f (shown as inverted 00-1f, ie 80-9f */
            /* 20-3f  !"#...=>? */
            /* 40-5f  @ab...\]  */
            /* 60-7f   AB...|}~ */
            if ((c >= 0x60) && (c <= 0x7f)) {
            /* 80-9f control codes 80-9f (shown as inverted 60-7f, ie e0-ff */
            /* a0-bf  gfx chars */
            } else if ((c >= 0xa0) && (c <= 0xbf)) {
            /* c0-df   AB...|}~ */
            } else if ((c >= 0xc0) && (c <= 0xdf)) {
                c -= 0x60;
            /* e0-ff -> a0 -> 60-7f gfx chars */
            } else if ((c >= 0xe0) && (c <= 0xfe)) {
                c -= 0x40;
            } else if (c == 0xff) {
                c = 0x7e;
            }
            sdl_ui_putchar(c, x_pos, y);
            ++x_pos;
            ++i;
        }

        sdl_ui_set_active_font(MENU_FONT_ASCII);

    } else {
        if (console_log_local) {
            rc = native_console_petscii_out(num, console_log_local, "%s", buffer);
        }
    }

    return rc;
}

int uimon_petscii_upper_out(const char *buffer, int num)
{
    int rc = 0;

    if (using_ui_monitor) {
        int y = menu_draw->max_text_y - 1;
        const char *p = buffer;
        int i = 0;
        uint8_t c;

        sdl_ui_set_active_font(MENU_FONT_IMAGES);

        /* CAUTION: there is another level of indirection going on in uimenu.c */
        while (i < num) {
            c = p[i];
            /* 00-1f control codes 00-1f (shown as inverted 00-1f, ie 80-9f */
            /* 20-3f  !"#...=>? */
            /* 40-5f  @ab...\]  */
            /* 60-7f   AB...|}~ */
            if ((c >= 0x60) && (c <= 0x7f)) {
            /* 80-9f control codes 80-9f (shown as inverted 60-7f, ie e0-ff */
            /* a0-bf  gfx chars */
            } else if ((c >= 0xa0) && (c <= 0xbf)) {
            /* c0-df   AB...|}~ */
            } else if ((c >= 0xc0) && (c <= 0xdf)) {
                c -= 0x60;
            /* e0-ff -> a0 -> 60-7f gfx chars */
            } else if ((c >= 0xe0) && (c <= 0xfe)) {
                c -= 0x40;
            } else if (c == 0xff) {
                c = 0x7e;
            }
            sdl_ui_putchar(c, x_pos, y);
            ++x_pos;
            ++i;
        }

        sdl_ui_set_active_font(MENU_FONT_ASCII);

    } else {
        if (console_log_local) {
            rc = native_console_petscii_upper_out(num, console_log_local, "%s", buffer);
        }
    }

    return rc;
}


/* screencode layout:
 00 @abcdefghijklmnopqrstuvwxyz[ ]^
 20  !"#$%&'()*+,-./0123456789:;<=>?
 40  ABCDEFGHIJKLMNOPQRSTUVWXYZ.....
 60 ................................
 80-FF inverted version of the above
 */

/* monitor charset:
 00 @abcdefghijklmnopqrstuvwxyz[ ]^     inverted
 20  !"#$%&'()*+,-./0123456789:;<=>?
 40 @abcdefghijklmnopqrstuvwxyz[ ]^
 60  ABCDEFGHIJKLMNOPQRSTUVWXYZ.....
 80  ABCDEFGHIJKLMNOPQRSTUVWXYZ.....    inverted
 A0 ................................

 */

int uimon_scrcode_out(const char *buffer, int num)
{
    int rc = 0;

    if (using_ui_monitor) {
        int y = menu_draw->max_text_y - 1;
        const char *p = buffer;
        int i = 0;
        uint8_t c;

        sdl_ui_set_active_font(MENU_FONT_MONITOR);

        /* CAUTION: there is another level of indirection going on in uimenu.c */
        while (i < num) {
            c = p[i];
            /* 00-1f -> 40-5f */
            if (/*(c >= 0x00) &&*/ (c <= 0x1f)) {
                c += 0x40;
            /* 20-3f -> 20-3f */
            /* 40-5f -> 60-7f */
            } else if ((c >= 0x40) && (c <= 0x5f)) {
                c += 0x20;
            /* 60-7f -> a0-bf */
            } else if ((c >= 0x60) && (c <= 0x7f)) {
                c += 0x40;
            /* 80-9f -> 00-1f */
            } else if ((c >= 0x80) && (c <= 0x9f)) {
                c -= 0x80;
            /* a0-bf -> c0-df */
            } else if ((c >= 0xa0) && (c <= 0xbf)) {
                c += 0x20;
            /* c0-df -> 80-9f */
            } else if ((c >= 0xc0) && (c <= 0xdf)) {
                c -= 0x40;
            }
            /* e0-ff -> e0-ff */
            sdl_ui_putchar(c, x_pos, y);
            ++x_pos;
            ++i;
        }

        sdl_ui_set_active_font(MENU_FONT_ASCII);

    } else {
        if (console_log_local) {
            rc = native_console_scrcode_out(num, console_log_local, "%s", buffer);
        }
    }

    return rc;
}

int uimon_scrcode_upper_out(const char *buffer, int num)
{
    int rc = 0;

    if (using_ui_monitor) {
        int y = menu_draw->max_text_y - 1;
        const char *p = buffer;
        int i = 0;
        uint8_t c;

        sdl_ui_set_active_font(MENU_FONT_IMAGES);

        /* CAUTION: there is another level of indirection going on in uimenu.c */
        while (i < num) {
            c = p[i];
            /* 00-1f -> 40-5f */
            if (/*(c >= 0x00) &&*/ (c <= 0x1f)) {
                c += 0x40;
            /* 20-3f -> 20-3f */
            /* 40-5f -> 60-7f */
            } else if ((c >= 0x40) && (c <= 0x5f)) {
                c += 0x20;
            /* 60-7f -> a0-bf */
            } else if ((c >= 0x60) && (c <= 0x7f)) {
                c += 0x40;
            /* 80-9f -> 00-1f */
            } else if ((c >= 0x80) && (c <= 0x9f)) {
                c -= 0x80;
            /* a0-bf -> c0-df */
            } else if ((c >= 0xa0) && (c <= 0xbf)) {
                c += 0x20;
            /* c0-df -> 80-9f */
            } else if ((c >= 0xc0) && (c <= 0xdf)) {
                c -= 0x40;
            }
            /* e0-ff -> e0-ff */
            sdl_ui_putchar(c, x_pos, y);
            ++x_pos;
            ++i;
        }

        sdl_ui_set_active_font(MENU_FONT_ASCII);

    } else {
        if (console_log_local) {
            rc = native_console_scrcode_out(num, console_log_local, "%s", buffer);
        }
    }

    return rc;
}

char *uimon_get_in(char **ppchCommandLine, const char *prompt)
{
    if (using_ui_monitor) {
        int y, x_off;
        char *input;

        y = menu_draw->max_text_y - 1;
        x_pos = 0;

        x_off = sdl_ui_print(prompt, 0, y);
        input = sdl_ui_readline(NULL, x_off, y);
        sdl_ui_scroll_screen_up();

        return input;
    } else {
        return native_console_in(console_log_local, prompt);
    }
}

void uimon_notify_change(void)
{
    if (using_ui_monitor && menu_draw) {
        sdl_ui_refresh();
    }
}

void uimon_set_interface(monitor_interface_t **monitor_interface_init, int count)
{
}

int console_init(void)
{
#if 0
    if (using_ui_monitor) {
    } else {
        return native_console_init();
    }
#endif
    return native_console_init();
}

int console_close_all(void)
{
#if 0
    if (using_ui_monitor) {
    } else {
        return native_console_close_all();
    }
#endif
    native_console_close_all();
    console_log_local = NULL;
    return 0;
}
