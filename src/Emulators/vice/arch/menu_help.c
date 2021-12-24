/*
 * menu_help.c - SDL help menu functions.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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
#include "types.h"

#include <stdlib.h>
#include "vice_sdl.h"

#include "cmdline.h"
#include "info.h"
#include "lib.h"
#include "menu_common.h"
#include "menu_help.h"
#include "platform_discovery.h"
#include "ui.h"
#include "uimenu.h"
#include "util.h"
#include "version.h"

#ifdef WINMIPS
static char *concat_all(char **text)
{
    int i;
    char *new_text;
    char *old_text = lib_stralloc("\n");

    for (i = 0; text[i] != NULL; i++) {
        new_text = util_concat(old_text, text[i], NULL);
        lib_free(old_text);
        old_text = new_text;
    }
    return new_text;
}
#endif

static void make_n_cols(char *text, int len, int cols)
{
    int i = cols;

    while (i < len) {
        while (text[i] != ' ' && (i > 0)) {
            i--;
        }

        if (i == 0) {
            /* line with >40 chars and no spaces;
               abort and let sdl_ui_print truncate */
            return;
        }

        text[i] = '\n';
        text += i + 1;
        len -= i + 1;
        i = cols;
    }
}

static char *convert_cmdline_to_n_cols(const char *text, int cols)
{
    char *new_text;
    int num_options;
    int current_line;
    int i, j, index;

    num_options = cmdline_get_num_options();
    new_text = lib_malloc(strlen(text) + num_options);

    new_text[0] = '\n';
    index = 1;
    current_line = 1;
    for (i = 0; i < num_options; i++) {
        for (j = 0; text[current_line + j] != '\n'; j++) {
            new_text[index] = text[current_line + j];
            index++;
        }
        new_text[index] = '\n';
        index++;
        current_line += j + 2;
        for (j = 0; text[current_line + j] != '\n'; j++) {
            new_text[index + j] = text[current_line + j];
        }

        new_text[index + j] = '\n';

        if (j > cols) {
            make_n_cols(&(new_text[index]), j, cols);
        }

        current_line += j + 1;
        index += j + 1;
        new_text[index] = '\n';
        index++;
    }
    return new_text;
}

static char *contrib_convert(char *text, int cols)
{
    char *new_text;
    char *pos;
    unsigned int i = 0;
    unsigned int j = 0;
    int single = 0;
    int len;
    size_t size;

    size = strlen(text);
    new_text = lib_malloc(size);
    while (i < size) {
        if (text[i] == ' ' && text[i + 1] == ' ' && text[i - 1] == '\n') {
            i += 2;
        } else {
            if ((text[i] == ' ' || text[i] == '\n') && text[i + 1] == '<') {
                while (text[i] != '>') {
                   i++;
                }
                i++;
            } else {
                new_text[j] = text[i];
                j++;
                i++;
            }
        }
    }
    new_text[j] = 0;

    i = 0;
    j = strlen(new_text);

    while (i < j) {
        if (new_text[i] == '\n') {
            if (new_text[i + 1] == '\n') {
                if (single) {
                    single = 0;
                }
                if (new_text[i - 1] == ':' && new_text[i - 2] == 'e') {
                    single = 1;
                }
                new_text[i + 1] = 0;
                i++;
            } else {
                if (!single) {
                    new_text[i] = ' ';
                }
            }
        }
        i++;
    }

    pos = new_text;
    while (*pos != 0) {
        len = strlen(pos);
        make_n_cols(pos, len, cols);
        pos += len + 1;
    }

    for (i = 0; i < j; i++) {
        if (new_text[i] == 0) {
            new_text[i] = '\n';
        }
    }

    return new_text;
}

static unsigned int scroll_up(const char *text, int first_line, int amount)
{
    int line = first_line;

    while ((amount--) && (line > 0)) {
        int i = line - 2;

        while ((i >= 0) && (text[i] != '\n')) {
            --i;
        }

        line = i + 1;
    }

    return line;
}

static void show_text(const char *text)
{
    int first_line = 0;
    int next_line = 0;
    int next_page = 0;
    unsigned int current_line = 0;
    unsigned int len;
    int x, y, z;
    int active = 1;
    int active_keys;
    char *string;
    menu_draw_t *menu_draw;

    menu_draw = sdl_ui_get_menu_param();

    string = lib_malloc(128);
    len = strlen(text);

    while (active) {
        sdl_ui_clear();
        first_line = current_line;
        for (y = 0; (y < menu_draw->max_text_y) && (current_line < len); y++) {
            z = 0;
            for (x = 0; text[current_line + x] != '\n'; x++) {
                switch (text[current_line + x]) {
                    case '`':
                        string[x + z] = '\'';
                        break;
                    case 'ä':
                        string[x + z] = 'a';
                        break;
                    case '~':
                        string[x + z] = '-';
                        break;
                    case 'é':
                    case 'è':
                        string[x + z] = 'e';
                        break;
                    case 'Ö':
                        string[x + z] = 'O';
                        break;
                    case 'ö':
                        string[x + z] = 'o';
                        break;
                    case 'å':
                        string[x + z] = 'a';
                        break;
                    case '\t':
                        string[x + z] = ' ';
                        string[x + z + 1] = ' ';
                        string[x + z + 2] = ' ';
                        string[x + z + 3] = ' ';
                        z += 3;
                        break;
                    default:
                       string[x + z] = text[current_line + x];
                       break;
                }
            }
            if (x != 0) {
                string[x + z] = 0;
                sdl_ui_print(string, 0, y);
            }
            if (y == 0) {
                next_line = current_line + x + 1;
            }
            current_line += x + 1;
        }
        next_page = current_line;
        active_keys = 1;
        sdl_ui_refresh();

        while (active_keys) {
            switch(sdl_ui_menu_poll_input()) {
                case MENU_ACTION_CANCEL:
                case MENU_ACTION_EXIT:
                case MENU_ACTION_SELECT:
                    active_keys = 0;
                    active = 0;
                    break;
                case MENU_ACTION_RIGHT:
                    active_keys = 0;
                    current_line = next_page;
                    break;
                case MENU_ACTION_DOWN:
                    active_keys = 0;
                    current_line = next_line;
                    break;
                case MENU_ACTION_LEFT:
                    active_keys = 0;
                    current_line = scroll_up(text, first_line, menu_draw->max_text_y);
                    break;
                case MENU_ACTION_UP:
                    active_keys = 0;
                    current_line = scroll_up(text, first_line, 1);
                    break;
                default:
                    //SDL_Delay(10);
                    break;
            }
        }
    }
    lib_free(string);
}

static UI_MENU_CALLBACK(about_callback)
{
    int active = 1;

    if (activated) {
        sdl_ui_clear();
        sdl_ui_print_center("VICE", 0);
        sdl_ui_print_center("Versatile Commodore Emulator", 1);
        sdl_ui_print_center("Version " VERSION, 2);
        sdl_ui_print_center("SDL " PLATFORM_CPU " " PLATFORM_OS " " PLATFORM_COMPILER, 3);
        sdl_ui_print_center("The VICE Team", 5);
        sdl_ui_print_center("(C) 1998-2012 Dag Lem", 6);
        sdl_ui_print_center("(C) 1999-2012 Andreas Matthies", 7);
        sdl_ui_print_center("(C) 1999-2012 Martin Pottendorfer", 8);
        sdl_ui_print_center("(C) 2005-2012 Marco van den Heuvel", 9);
        sdl_ui_print_center("(C) 2006-2012 Christian Vogelgsang", 10);
        sdl_ui_print_center("(C) 2007-2012 Fabrizio Gennari", 11);
        sdl_ui_print_center("(C) 2007-2012 Daniel Kahlin", 12);
        sdl_ui_print_center("(C) 2008-2012 Antti S. Lankila", 13);
        sdl_ui_print_center("(C) 2009-2012 Groepaz", 14);
        sdl_ui_print_center("(C) 2009-2012 Ingo Korb", 15);
        sdl_ui_print_center("(C) 2009-2012 Errol Smith", 16);
        sdl_ui_print_center("(C) 2010-2012 Olaf Seibert", 17);
        sdl_ui_print_center("(C) 2011-2012 Marcus Sutton", 18);
        sdl_ui_print_center("(C) 2011-2012 Ulrich Schulz", 19);
        sdl_ui_print_center("(C) 2011-2012 Stefan Haubenthal", 20);
        sdl_ui_print_center("(C) 2011-2012 Thomas Giesel", 21);
        sdl_ui_print_center("(C) 2011-2012 Kajtar Zsolt", 22);
        sdl_ui_print_center("(C) 2012-2012 Benjamin 'BeRo' Rosseaux", 23);
        sdl_ui_refresh();

        while (active) {
            switch(sdl_ui_menu_poll_input()) {
                case MENU_ACTION_CANCEL:
                case MENU_ACTION_EXIT:
                case MENU_ACTION_SELECT:
                    active = 0;
                    break;
                default:
                    //SDL_Delay(10);
                    break;
            }
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(cmdline_callback)
{
    menu_draw_t *menu_draw;
    char *options;
    char *options_n;

    if (activated) {
        menu_draw = sdl_ui_get_menu_param();
        options = cmdline_options_string();
        options_n = convert_cmdline_to_n_cols(options, menu_draw->max_text_x);
        lib_free(options);
        show_text((const char *)options_n);
        lib_free(options_n);
    }
    return NULL;
}

static UI_MENU_CALLBACK(contributors_callback)
{
	/*
    menu_draw_t *menu_draw;
    char *info_contrib_text_n;
#ifdef WINMIPS
    char *new_text = NULL;
#endif

    if (activated) {
        menu_draw = sdl_ui_get_menu_param();
#ifdef WINMIPS
        new_text = concat_all(info_contrib_text);
        info_contrib_text_n = contrib_convert(new_text, menu_draw->max_text_x);
        lib_free(new_text);
#else
        info_contrib_text_n = contrib_convert((char *)info_contrib_text, menu_draw->max_text_x);
#endif
        show_text((const char *)info_contrib_text_n);
        lib_free(info_contrib_text_n);
    }
	*/
    return NULL;
}

static UI_MENU_CALLBACK(license_callback)
{
	/*
    menu_draw_t *menu_draw;
#ifdef WINMIPS
    char *new_text = NULL;
#endif

    if (activated) {
        menu_draw = sdl_ui_get_menu_param();
        if (menu_draw->max_text_x > 60) {
#ifdef WINMIPS
            new_text = concat_all(info_license_text);
            show_text(new_text);
            lib_free(new_text);
#else
            show_text(info_license_text);
#endif
        } else {
#ifdef WINMIPS
            new_text = concat_all(info_license_text40);
            show_text(new_text);
            lib_free(new_text);
#else
            show_text(info_license_text40);
#endif
        }
    }*/
    return NULL;
}

static UI_MENU_CALLBACK(warranty_callback)
{
	/*
    menu_draw_t *menu_draw;

    if (activated) {
        menu_draw = sdl_ui_get_menu_param();
        if (menu_draw->max_text_x > 60) {
            show_text(info_warranty_text);
        } else {
            show_text(info_warranty_text40);
        }
    }*/
    return NULL;
}

#ifdef SDL_DEBUG
static UI_MENU_CALLBACK(show_font_callback)
{
    int active = 1;
    int i, j;
    char fontchars[] = "0x 0123456789abcdef";

    if (activated) {
        sdl_ui_clear();
        sdl_ui_print_center("   0123456789ABCDEF", 0);
        sdl_ui_print_center("0x \xff\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f", 1);
        for (j = 1; j < 16; ++j) {
            for (i = 0; i < 16; ++i) {
                fontchars[3 + i] = (char)(j * 16 + i);
            }
            fontchars[0] = "0123456789ABCDEF"[j];
            sdl_ui_print_center(fontchars, 1 + j);
        }
        sdl_ui_refresh();
        while (active) {
            switch(sdl_ui_menu_poll_input()) {
                case MENU_ACTION_CANCEL:
                case MENU_ACTION_EXIT:
                    active = 0;
                    break;
                default:
                    //SDL_Delay(10);
                    break;
            }
        }
    }
    return NULL;
}
#endif

const ui_menu_entry_t help_menu[] = {
    { "About",
      MENU_ENTRY_DIALOG,
      about_callback,
      NULL },
    { "Command-line options",
      MENU_ENTRY_DIALOG,
      cmdline_callback,
      NULL },
    { "Contributors",
      MENU_ENTRY_DIALOG,
      contributors_callback,
      NULL },
    { "License",
      MENU_ENTRY_DIALOG,
      license_callback,
      NULL },
    { "Warranty",
      MENU_ENTRY_DIALOG,
      warranty_callback,
      NULL },
#ifdef SDL_DEBUG
    { "Show font",
      MENU_ENTRY_DIALOG,
      show_font_callback,
      NULL },
#endif
    SDL_MENU_LIST_END
};
