/*
 * uifilereq.c - Common SDL file selection dialog functions.
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

#include "vice_sdl.h"
#include <string.h>

#include "archdep.h"
#include "lib.h"
#include "ui.h"
#include "uimenu.h"
#include "util.h"
#include "menu_common.h"

#include "uifilereq.h"


static menu_draw_t *menu_draw;
static char *last_selected_file = NULL;
/* static int last_selected_pos = 0; */ /* ? */
int last_selected_image_pos = 0;    /* FIXME: global variable. ugly. */

#define SDL_FILEREQ_META_FILE 0
#define SDL_FILEREQ_META_PATH 1

/* any platform that supports drive letters/names should
   define SDL_CHOOSE_DRIVES and provide the drive functions:
   char **archdep_list_drives(void);
   char *archdep_get_current_drive(void);
   void archdep_set_current_drive(const char *drive);
*/

#ifdef SDL_CHOOSE_DRIVES
#define SDL_FILEREQ_META_DRIVE 2
#define SDL_FILEREQ_META_NUM 3
#else
#define SDL_FILEREQ_META_NUM 2
#endif


/* ------------------------------------------------------------------ */
/* static functions */

static int sdl_ui_file_selector_recall_location(archdep_dir_t *directory)
{
    unsigned int i, j, k, count;
    int direction;

    if (!last_selected_file) {
        return 0;
    }

    /* the file list is sorted by ioutil, do binary search */
    i = 0;
    k = archdep_readdir_num_files(directory);

    /* ...but keep a failsafe in case the above assumption breaks */
    count = 50;

    while ((i < k) && (--count)) {
        j = i + ((k - i) >> 1);
        direction = strcmp(last_selected_file,
                           archdep_readdir_get_file(directory, j));

        if (direction > 0) {
            i = j + 1;
        } else if (direction < 0) {
            k = j;
        } else {
            return j + archdep_readdir_num_dirs(directory) + SDL_FILEREQ_META_NUM;
        }
    }

    return 0;
}

static const char* sdl_ui_get_file_selector_entry(archdep_dir_t *directory, int offset, int *isdir, ui_menu_filereq_mode_t mode)
{
    int num_dirs;

    *isdir = 0;

    if (offset >= archdep_readdir_num_entries(directory) + SDL_FILEREQ_META_NUM) {
        return NULL;
    }

    if (offset == SDL_FILEREQ_META_FILE) {
        switch (mode) {
            case FILEREQ_MODE_CHOOSE_FILE:
            case FILEREQ_MODE_CHOOSE_FILE_IN_IMAGE:
            case FILEREQ_MODE_SAVE_FILE:
                return "<enter filename>";

            case FILEREQ_MODE_CHOOSE_DIR:
                return "<choose current directory>";
        }
    }

    if (offset == SDL_FILEREQ_META_PATH) {
        return "<enter path>";
    }

#ifdef SDL_FILEREQ_META_DRIVE
    if (offset == SDL_FILEREQ_META_DRIVE) {
        return "<choose drive>";
    }
#endif

    num_dirs = archdep_readdir_num_dirs(directory);
    if (offset >= num_dirs + SDL_FILEREQ_META_NUM) {
        return archdep_readdir_get_file(directory, offset - (num_dirs + SDL_FILEREQ_META_NUM));
    } else {
        *isdir = 1;
        return archdep_readdir_get_dir(directory, offset - SDL_FILEREQ_META_NUM);
    }
}

#if (ARCHDEP_DIR_SEP_CHR == '\\')
static void sdl_ui_print_translate_seperator(const char *text, int x, int y)
{
    size_t len;
    size_t i;
    char *new_text = NULL;

    len = strlen(text);
    new_text = lib_strdup(text);

    for (i = 0; i < len; i++) {
        if (new_text[i] == '\\') {
            new_text[i] = '/';
        }
    }
    sdl_ui_print(new_text, x, y);
    lib_free(new_text);
}
#else
#define sdl_ui_print_translate_seperator(t, x, y) sdl_ui_print(t, x, y)
#endif

static void sdl_ui_display_path(const char *current_dir)
{
    int len;
    char *text = NULL;
    char *temp = NULL;
    int before = 0;
    int after = 0;
    int pos = 0;
    int amount = 0;
    int i;

    len = (int)strlen(current_dir);

    if (len > menu_draw->max_text_x) {
        text = lib_strdup(current_dir);

        temp = strchr(current_dir + 1, ARCHDEP_DIR_SEP_CHR);
        before = (int)(temp - current_dir + 1);

        while (temp != NULL) {
            amount++;
            temp = strchr(temp + 1, ARCHDEP_DIR_SEP_CHR);
        }

        while (text[len - after] != ARCHDEP_DIR_SEP_CHR) {
            after++;
        }

        if (amount > 1 && (before + after + 3) < menu_draw->max_text_x) {
            temp = strchr(current_dir + 1, ARCHDEP_DIR_SEP_CHR);
            while (((temp - current_dir + 1) < (menu_draw->max_text_x - after - 3)) && temp != NULL) {
                before = (int)(temp - current_dir + 1);
                temp = strchr(temp + 1, ARCHDEP_DIR_SEP_CHR);
            }
        } else {
            before = (menu_draw->max_text_x - 3) / 2;
            after = len - (len - menu_draw->max_text_x) - before - 3;
        }
        pos = len - after;
        text[before] = '.';
        text[before + 1] = '.';
        text[before + 2] = '.';
        for (i = 0; i < after; i++) {
            text[before + 3 + i] = text[pos + i];
        }
        text[before + 3 + after] = 0;
        sdl_ui_print_translate_seperator(text, 0, 2);
    } else {
        sdl_ui_print_translate_seperator(current_dir, 0, 2);
    }

    lib_free(text);
}

static void sdl_ui_file_selector_redraw(archdep_dir_t *directory, const char *title, const char *current_dir, int offset, int num_items, int more, ui_menu_filereq_mode_t mode, int cur_offset)
{
    int i, j, isdir = 0;
    char* title_string;
    const char* name;
    uint8_t oldbg = 0x0d;   /* This is the worst color, doesn't fit next to
                               3, 7 or F, only fits somewhat next to 5, but that
                               ramp is too high, or 1, but then the ramp down
                               kinda sucks
                             */

    title_string = lib_msprintf("%s %s", title, (offset) ? ((more) ? "(<- ->)" : "(<-)") : ((more) ? "(->)" : ""));

    sdl_ui_clear();
    sdl_ui_display_title(title_string);
    lib_free(title_string);
    sdl_ui_display_path(current_dir);

    for (i = 0; i < num_items; ++i) {
        if (i == cur_offset) {
            oldbg = sdl_ui_set_cursor_colors();
        }

        j = MENU_FIRST_X;
        name = sdl_ui_get_file_selector_entry(directory, offset + i, &isdir, mode);
        if (isdir) {
            j += sdl_ui_print("(D) ", MENU_FIRST_X, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);
        }
        j += sdl_ui_print(name, j, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);

        if (i == cur_offset) {
            sdl_ui_print_eol(j, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);
            sdl_ui_reset_cursor_colors(oldbg);
        }
    }
}

static void sdl_ui_file_selector_redraw_cursor(archdep_dir_t *directory, int offset, int num_items, ui_menu_filereq_mode_t mode, int cur_offset, int old_offset)
{
    int i, j, isdir = 0;
    const char *name;
    uint8_t oldbg = 0;

    for (i = 0; i < num_items; ++i) {
        if ((i == cur_offset) || (i == old_offset)){
            if (i == cur_offset) {
                oldbg = sdl_ui_set_cursor_colors();
            }
            j = MENU_FIRST_X;
            name = sdl_ui_get_file_selector_entry(directory, offset + i, &isdir, mode);
            if (isdir) {
                j += sdl_ui_print("(D) ", MENU_FIRST_X, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);
            }
            j += sdl_ui_print(name, j, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);

            sdl_ui_print_eol(j, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);
            if (i == cur_offset) {
                sdl_ui_reset_cursor_colors(oldbg);
            }
        }
    }
}

#ifdef SDL_FILEREQ_META_DRIVE
static char *current_drive = NULL;
static int drive_set = 0;

static UI_MENU_CALLBACK(drive_callback)
{
    char *drive;

    drive = (char *)param;

    if (strcmp(drive, current_drive) == 0) {
        return sdl_menu_text_tick;
    } else if (activated) {
        current_drive = drive;
        drive_set = 1;
        return sdl_menu_text_tick;
    }

    return NULL;
}

static char * display_drive_menu(void)
{
    char **drives, **p;
    char *result = NULL;
    char *previous_drive;
    int drive_count = 1;
    ui_menu_entry_t *sub_menu, *pm;
    ui_menu_entry_t menu;

    drives = archdep_list_drives();

    p = drives;

    while (*p != NULL) {
        ++drive_count;
        ++p;
    }

    sub_menu = lib_malloc(sizeof(ui_menu_entry_t) * drive_count);
    pm = sub_menu;

    p = drives;

    while (*p != NULL) {
        pm->string = *p;
        pm->type = MENU_ENTRY_OTHER;
        pm->callback = drive_callback;
        pm->data = (ui_callback_data_t)*p;
        ++pm;
        ++p;
    }

    pm->string = NULL;
    pm->type = MENU_ENTRY_TEXT;
    pm->callback = NULL;
    pm->data = (ui_callback_data_t)NULL;

    menu.string = "Choose drive";
    menu.type = MENU_ENTRY_DYNAMIC_SUBMENU;
    menu.callback = NULL;
    menu.data = (ui_callback_data_t)sub_menu;

    previous_drive = archdep_get_current_drive();

    current_drive = previous_drive;
    drive_set = 0;
    sdl_ui_external_menu_activate(&menu);

    if (drive_set) {
        result = current_drive;
    }

    lib_free(previous_drive);

    lib_free(sub_menu);

    p = drives;

    while (*p != NULL) {
        if (*p != result) {
            lib_free(*p);
        }
        ++p;
    }
    lib_free(drives);

    return result;
}
#endif

/* ------------------------------------------------------------------ */
/* External UI interface */

#define SDL_FILESELECTOR_DIRMODE    ARCHDEP_OPENDIR_NO_HIDDEN_FILES

char* sdl_ui_file_selection_dialog(const char* title, ui_menu_filereq_mode_t mode)
{
    int total, dirs, files, menu_max;
    int active = 1;
    int offset = 0;
    int redraw = 1;
    char *retval = NULL;
    int cur = 0, cur_old = -1;
    archdep_dir_t *directory;
    char current_dir[ARCHDEP_PATH_MAX];
    char backup_dir[ARCHDEP_PATH_MAX];
    char *inputstring;

    last_selected_image_pos = 0;

    menu_draw = sdl_ui_get_menu_param();

    archdep_getcwd(current_dir, ARCHDEP_PATH_MAX);
    memcpy(backup_dir, current_dir, sizeof(backup_dir));

    directory = archdep_opendir(current_dir, SDL_FILESELECTOR_DIRMODE);
    if (directory == NULL) {
        return NULL;
    }

    dirs = archdep_readdir_num_dirs(directory);
    files = archdep_readdir_num_files(directory);
    total = dirs + files + SDL_FILEREQ_META_NUM;
    menu_max = menu_draw->max_text_y - (MENU_FIRST_Y + SDL_FILEREQ_META_NUM);

    if ((mode == FILEREQ_MODE_CHOOSE_FILE) || (mode == FILEREQ_MODE_CHOOSE_FILE_IN_IMAGE)) {
        offset = sdl_ui_file_selector_recall_location(directory);
    }

    while (active) {
        if (redraw) {
            sdl_ui_file_selector_redraw(directory, title, current_dir, offset,
                (total - offset > menu_max) ? menu_max : total - offset,
                (total - offset > menu_max) ? 1 : 0, mode, cur);
            redraw = 0;
        } else {
            sdl_ui_file_selector_redraw_cursor(directory, offset,
                    (total - offset > menu_max) ? menu_max : total - offset,
                    mode, cur, cur_old);
        }
        sdl_ui_refresh();

        switch (sdl_ui_menu_poll_input()) {
            case MENU_ACTION_HOME:
                cur_old = cur;
                cur = 0;
                offset = 0;
                redraw = 1;
                break;

            case MENU_ACTION_END:
                cur_old = cur;
                if (total < (menu_max - 1)) {
                    cur = total - 1;
                    offset = 0;
                } else {
                    cur = menu_max - 1;
                    offset = total - menu_max;
                }
                redraw = 1;
                break;

            case MENU_ACTION_UP:
                if (cur > 0) {
                    cur_old = cur;
                    --cur;
                } else {
                    if (offset > 0) {
                        offset--;
                        redraw = 1;
                    }
                }
                break;

            case MENU_ACTION_PAGEUP:
            case MENU_ACTION_LEFT:
                offset -= menu_max;
                if (offset < 0) {
                    offset = 0;
                    cur_old = -1;
                    cur = 0;
                }
                redraw = 1;
                break;

            case MENU_ACTION_DOWN:
                if (cur < (menu_max - 1)) {
                    if ((cur + offset) < total - 1) {
                        cur_old = cur;
                        ++cur;
                    }
                } else {
                    if (offset < (total - menu_max)) {
                        offset++;
                        redraw = 1;
                    }
                }
                break;

            case MENU_ACTION_PAGEDOWN:
            case MENU_ACTION_RIGHT:
                offset += menu_max;
                if (offset >= total) {
                    offset = total - 1;
                    cur_old = -1;
                    cur = 0;
                } else if ((cur + offset) >= total) {
                    cur_old = -1;
                    cur = total - offset - 1;
                }
                redraw = 1;
                break;

            case MENU_ACTION_SELECT:
                switch (offset + cur) {
                    case SDL_FILEREQ_META_FILE:
                        if ((mode == FILEREQ_MODE_CHOOSE_FILE) || (mode == FILEREQ_MODE_CHOOSE_FILE_IN_IMAGE) || (mode == FILEREQ_MODE_SAVE_FILE)) {
                            inputstring = sdl_ui_text_input_dialog("Enter filename", NULL);
                            if (inputstring == NULL) {
                                redraw = 1;
                            } else {
                                if (!archdep_path_is_relative(inputstring) && (strchr(inputstring, ARCHDEP_DIR_SEP_CHR) != NULL)) {
                                    retval = inputstring;
                                } else {
                                    retval = util_concat(current_dir, ARCHDEP_DIR_SEP_STR, inputstring, NULL);
                                    lib_free(inputstring);
                                }
                            }
                        } else {
                            retval = lib_strdup(current_dir);
                        }
                        active = 0;
                        break;

                    case SDL_FILEREQ_META_PATH:
                        inputstring = sdl_ui_text_input_dialog("Enter path", NULL);
                        if (inputstring != NULL) {
                            archdep_chdir(inputstring);
                            lib_free(inputstring);
                            archdep_closedir(directory);
                            archdep_getcwd(current_dir, ARCHDEP_PATH_MAX);
                            directory = archdep_opendir(current_dir, SDL_FILESELECTOR_DIRMODE);
                            offset = 0;
                            cur_old = -1;
                            cur = 0;
                            dirs = archdep_readdir_num_dirs(directory);
                            files = archdep_readdir_num_files(directory);
                            total = dirs + files + SDL_FILEREQ_META_NUM;
                        }
                        redraw = 1;
                        break;

#ifdef SDL_FILEREQ_META_DRIVE
                    case SDL_FILEREQ_META_DRIVE:
                        inputstring = display_drive_menu();
                        if (inputstring != NULL) {
                            archdep_set_current_drive(inputstring);
                            lib_free(inputstring);
                            archdep_closedir(directory);
                            archdep_getcwd(current_dir, ARCHDEP_PATH_MAX);
                            directory = archdep_opendir(current_dir, SDL_FILESELECTOR_DIRMODE);
                            offset = 0;
                            cur_old = -1;
                            cur = 0;
                            dirs = archdep_readdir_num_dirs(directory);
                            files = archdep_readdir_num_files(directory);
                            total = dirs + files + SDL_FILEREQ_META_NUM;
                        }
                        redraw = 1;
                        break;
#endif
                    default:
                        if (offset + cur < (dirs + SDL_FILEREQ_META_NUM)) {
                            /* enter subdirectory */
                            archdep_chdir(archdep_readdir_get_dir(directory, offset + cur - SDL_FILEREQ_META_NUM));
                            archdep_closedir(directory);
                            archdep_getcwd(current_dir, ARCHDEP_PATH_MAX);
                            directory = archdep_opendir(current_dir, SDL_FILESELECTOR_DIRMODE);
                            offset = 0;
                            cur_old = -1;
                            cur = 0;
                            dirs = archdep_readdir_num_dirs(directory);
                            files = archdep_readdir_num_files(directory);
                            total = dirs + files + SDL_FILEREQ_META_NUM;
                            redraw = 1;
                        } else {
                            const char *selected_file = archdep_readdir_get_file(directory, offset + cur - dirs - SDL_FILEREQ_META_NUM);
                            if ((mode == FILEREQ_MODE_CHOOSE_FILE) || (mode == FILEREQ_MODE_CHOOSE_FILE_IN_IMAGE) || (mode == FILEREQ_MODE_CHOOSE_FILE_IN_IMAGE)) {
                                lib_free(last_selected_file);
                                last_selected_file = lib_strdup(selected_file);
                            }
                            retval = util_concat(current_dir, ARCHDEP_DIR_SEP_STR, selected_file, NULL);

                            if (mode == FILEREQ_MODE_CHOOSE_FILE_IN_IMAGE) {
                                /* browse image */
                                int retval2;
                                retval2 = sdl_ui_image_file_selection_dialog(retval, mode);
                                if (retval2 != -1) {
                                    active = 0;
                                }
                                last_selected_image_pos = retval2;
                                redraw = 1;
                            } else {
                                active = 0;
                                last_selected_image_pos = 0;
                            }
                        }
                        break;
                }
                break;

            case MENU_ACTION_CANCEL:
            case MENU_ACTION_EXIT:
                retval = NULL;
                active = 0;
                archdep_chdir(backup_dir);
                break;

            default:
                SDL_Delay(10);
                break;
        }
    }
    archdep_closedir(directory);

    return retval;
}

void sdl_ui_file_selection_dialog_shutdown(void)
{
    lib_free(last_selected_file);
    last_selected_file = NULL;
}


/* FIXME: dead code? */
#if 0
static char* sdl_ui_get_slot_selector_entry(ui_menu_slots *slots, int offset, ui_menu_slot_mode_t mode)
{
    if (offset >= slots->number_of_elements) {
        return NULL;
    }
    if (slots->entries[offset].used) {
        return slots->entries[offset].slot_string;
    } else {
        return "< empty slot >";
    }
}

static void sdl_ui_slot_selector_redraw(ui_menu_slots *slots, const char *title, const char *current_dir, int offset, int num_items, int more, ui_menu_slot_mode_t mode)
{
    int i, j;
    char* title_string;
    char* name;

    title_string = lib_msprintf("%s  : ", title);

    sdl_ui_clear();
    sdl_ui_display_title(title_string);
    lib_free(title_string);
    sdl_ui_display_path(current_dir);

    for (i = 0; i < num_items; ++i) {
        j = MENU_FIRST_X;
        name = sdl_ui_get_slot_selector_entry(slots, offset + i, mode);
        sdl_ui_print(name, j, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);
    }
}

static void sdl_ui_slot_selector_redraw_cursor(ui_menu_slots *slots, int offset, int num_items, ui_menu_slot_mode_t mode, int cur_offset, int old_offset)
{
    int i, j;
    char* name;

    for (i = 0; i < num_items; ++i) {
        if ((i == cur_offset) || (i == old_offset)) {
            j = MENU_FIRST_X;
            name = sdl_ui_get_slot_selector_entry(slots, offset + i, mode);
            j += sdl_ui_print(name, j, i + MENU_FIRST_Y + SDL_FILEREQ_META_NUM);
            sdl_ui_print_eol(MENU_FIRST_X + j, MENU_FIRST_Y + i);
        }
    }
}

/* ------------------------------------------------------------------ */
/* External UI interface */

char* sdl_ui_slot_selection_dialog(const char* title, ui_menu_slot_mode_t mode)
{
    int total;
    int active = 1;
    int offset = 0;
    int redraw = 1;
    char *retval = NULL;
    int cur = 0, cur_old = -1;
    char *current_dir;
    unsigned int maxpathlen;
    ui_menu_slots *slots;
    char *temp_name;
    char *progname;
    int i;

    menu_draw = sdl_ui_get_menu_param();
    maxpathlen = archdep_maxpathlen();

    /* workaround to get the "home" directory of the emulator*/
    current_dir = archdep_default_resource_file_name();
    if (current_dir) {
        temp_name = strrchr(current_dir, ARCHDEP_DIR_SEP_CHR);
        if (temp_name) {
            *temp_name = 0;
        } else {
            lib_free(current_dir);
            current_dir = NULL;
        }
    }
    /* workaound end */
    if (!current_dir) {
        current_dir = lib_malloc(maxpathlen);
        archdep_getcwd(current_dir, maxpathlen);
    }

    total = menu_draw->max_text_y - (MENU_FIRST_Y + SDL_FILEREQ_META_NUM);

    slots = lib_malloc(sizeof(ui_menu_slots));
    slots->entries = lib_malloc(sizeof(ui_menu_slot_entry) * total);

    progname = archdep_program_name();
    temp_name = strchr(progname, '.');
    if (temp_name) {
        *temp_name = 0;
    }
    for (i = 0; i < total; ++i) {
        size_t len;
        unsigned int isdir;

        slots->entries[i].slot_name = lib_msprintf("snapshot_%s_%02d.vsf", progname, i + 1);
        slots->entries[i].slot_string = lib_msprintf(" SLOT %2d", i + 1);
        temp_name = util_concat(current_dir, ARCHDEP_DIR_SEP_STR, slots->entries[i].slot_name, NULL);
        if (archdep_stat(temp_name, &len, &isdir) == 0) {
            slots->entries[i].used = (isdir == 0);
        } else {
            slots->entries[i].used = 0;
        }
        lib_free(temp_name);
    }
/*    lib_free(progname); */
    slots->number_of_elements = total;
    if (mode == SLOTREQ_MODE_CHOOSE_SLOT) {
        cur = last_selected_pos;
    }

    while (active) {
        if (redraw) {
            sdl_ui_slot_selector_redraw(slots, title, current_dir, offset, total, 0, mode);
            redraw = 0;
        } else {
            sdl_ui_slot_selector_redraw_cursor(slots, offset, total, mode, cur, cur_old);
        }
        sdl_ui_refresh();

        switch (sdl_ui_menu_poll_input()) {
            case MENU_ACTION_UP:
                if (cur > 0) {
                    cur--;
                    redraw = 1;
                }
                break;
            case MENU_ACTION_DOWN:
                if (cur < total - 1) {
                    cur++;
                    redraw = 1;
                }
                break;
            case MENU_ACTION_SELECT:
                if ((slots->entries[cur].used) || (mode == SLOTREQ_MODE_SAVE_SLOT)) {
                    last_selected_pos = cur;
                    retval = util_concat(current_dir, ARCHDEP_DIR_SEP_STR, slots->entries[cur].slot_name, NULL);
                } else {
                    retval = NULL;
                }
                active = 0;
                break;
            case MENU_ACTION_CANCEL:
            case MENU_ACTION_EXIT:
                retval = NULL;
                active = 0;
                break;
            default:
                SDL_Delay(10);
                break;
        }
    }
    for (i = 0; i < total; ++i) {
        lib_free(slots->entries[i].slot_name);
        lib_free(slots->entries[i].slot_string);
    }
    lib_free(slots->entries);
    lib_free(slots);
    lib_free(current_dir);
    return retval;
}
#endif
