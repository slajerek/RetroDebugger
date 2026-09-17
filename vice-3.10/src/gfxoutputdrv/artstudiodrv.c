/*
 * artstudiodrv.c - Create a c64 artstudio type file.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "vice.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "archdep.h"
#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "mem.h"
#include "gfxoutput.h"
#include "nativedrv.h"
#include "palette.h"
#include "resources.h"
#include "screenshot.h"
#include "types.h"
#include "uiapi.h"
#include "util.h"
#include "vsync.h"

/*
    TODO:

    when all is done, remove #if 0'ed code
*/

#define ARTSTUDIO_SCREEN_PIXEL_WIDTH   320
#define ARTSTUDIO_SCREEN_PIXEL_HEIGHT  200

#define ARTSTUDIO_SCREEN_BYTE_WIDTH    ARTSTUDIO_SCREEN_PIXEL_WIDTH / 8
#define ARTSTUDIO_SCREEN_BYTE_HEIGHT   ARTSTUDIO_SCREEN_PIXEL_HEIGHT / 8

/* define offsets in the artstudio file */
#define VIDEORAM_OFFSET 0x1f42
#define BITMAP_OFFSET   2
#define ARTSTUDIO_SIZE  9002 + 7

static gfxoutputdrv_t artstudio_drv;

/* ------------------------------------------------------------------------ */

static int oversize_handling;
static int undersize_handling;
static int multicolor_handling;
static int ted_lum_handling;
#if 0
static int crtc_text_color;
static uint8_t crtc_fgcolor;
#endif

static int set_oversize_handling(int val, void *param)
{
    switch (val) {
        case NATIVE_SS_OVERSIZE_SCALE:
        case NATIVE_SS_OVERSIZE_CROP_LEFT_TOP:
        case NATIVE_SS_OVERSIZE_CROP_CENTER_TOP:
        case NATIVE_SS_OVERSIZE_CROP_RIGHT_TOP:
        case NATIVE_SS_OVERSIZE_CROP_LEFT_CENTER:
        case NATIVE_SS_OVERSIZE_CROP_CENTER:
        case NATIVE_SS_OVERSIZE_CROP_RIGHT_CENTER:
        case NATIVE_SS_OVERSIZE_CROP_LEFT_BOTTOM:
        case NATIVE_SS_OVERSIZE_CROP_CENTER_BOTTOM:
        case NATIVE_SS_OVERSIZE_CROP_RIGHT_BOTTOM:
            break;
        default:
            return -1;
    }

    oversize_handling = val;

    return 0;
}

static int set_undersize_handling(int val, void *param)
{
    switch (val) {
        case NATIVE_SS_UNDERSIZE_SCALE:
        case NATIVE_SS_UNDERSIZE_BORDERIZE:
            break;
        default:
            return -1;
    }

    undersize_handling = val;

    return 0;
}

static int set_multicolor_handling(int val, void *param)
{
    switch (val) {
        case NATIVE_SS_MC2HR_BLACK_WHITE:
        case NATIVE_SS_MC2HR_2_COLORS:
        case NATIVE_SS_MC2HR_4_COLORS:
        case NATIVE_SS_MC2HR_GRAY:
        case NATIVE_SS_MC2HR_DITHER:
            break;
        default:
            return -1;
    }

    multicolor_handling = val;

    return 0;
}

static int set_ted_lum_handling(int val, void *param)
{
    switch (val) {
        case NATIVE_SS_TED_LUM_IGNORE:
        case NATIVE_SS_TED_LUM_DITHER:
            break;
        default:
            return -1;
    }

    ted_lum_handling = val;

    return 0;
}

#if 0
static int set_crtc_text_color(int val, void *param)
{
    switch (val) {
        case NATIVE_SS_CRTC_WHITE:
            crtc_fgcolor = 1;
            break;
        case NATIVE_SS_CRTC_AMBER:
            crtc_fgcolor = 8;
            break;
        case NATIVE_SS_CRTC_GREEN:
            crtc_fgcolor = 5;
            break;
        default:
            return -1;
    }

    crtc_text_color = val;

    return 0;
}
#endif

static const resource_int_t resources_int[] = {
    { "OCPOversizeHandling", NATIVE_SS_OVERSIZE_SCALE, RES_EVENT_NO, NULL,
      &oversize_handling, set_oversize_handling, NULL },
    { "OCPUndersizeHandling", NATIVE_SS_UNDERSIZE_BORDERIZE, RES_EVENT_NO, NULL,
      &undersize_handling, set_undersize_handling, NULL },
    { "OCPMultiColorHandling", NATIVE_SS_MC2HR_DITHER, RES_EVENT_NO, NULL,
      &multicolor_handling, set_multicolor_handling, NULL },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_plus4[] = {
    { "OCPTEDLumHandling", NATIVE_SS_TED_LUM_DITHER, RES_EVENT_NO, NULL,
      &ted_lum_handling, set_ted_lum_handling, NULL },
    RESOURCE_INT_LIST_END
};

#if 0
static const resource_int_t resources_int_crtc[] = {
    { "OCPCRTCTextColor", NATIVE_SS_CRTC_WHITE, RES_EVENT_NO, NULL,
      &crtc_text_color, set_crtc_text_color, NULL },
    RESOURCE_INT_LIST_END
};
#endif

static int artstudiodrv_resources_init(void)
{
    if (machine_class == VICE_MACHINE_PLUS4) {
        if (resources_register_int(resources_int_plus4) < 0) {
            return -1;
        }
    }
#if 0
    if (machine_class == VICE_MACHINE_PET || machine_class == VICE_MACHINE_CBM6x0) {
        if (resources_register_int(resources_int_crtc) < 0) {
            return -1;
        }
    }
#endif
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-ocpoversize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "OCPOversizeHandling", NULL,
      "<method>", "Select the way the oversized input should be handled,"
      " (0: scale down, 1: crop left top, 2: crop center top,  3: crop right top,"
      " 4: crop left center, 5: crop center, 6: crop right center, 7: crop left bottom,"
      " 8: crop center bottom, 9:  crop right bottom)" },
    { "-ocpundersize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "OCPUndersizeHandling", NULL,
      "<method>", "Select the way the undersized input should be handled,"
      " (0: scale up, 1: borderize)" },
    { "-ocpmc", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "OCPMultiColorHandling", NULL,
      "<method>", "Select the way the multicolor to hires should be handled,"
      " (0: b&w, 1: 2 colors, 2: 4 colors, 3: gray scale,  4: best cell colors)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t cmdline_options_plus4[] =
{
    { "-ocptedlum", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "OCPTEDLumHandling", NULL,
      "<method>", "Select the way the TED luminosity should be handled, (0: ignore, 1: dither)" },
    CMDLINE_LIST_END
};

#if 0
static const cmdline_option_t cmdline_options_crtc[] =
{
    { "-ocpcrtctextcolor", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "OCPCRTCTextColor", NULL,
      "<color>", "Select the CRTC text color (0: white, 1: amber, 2: green)" },
    CMDLINE_LIST_END
};
#endif

static int artstudiodrv_cmdline_options_init(void)
{
    if (machine_class == VICE_MACHINE_PLUS4) {
        if (cmdline_register_options(cmdline_options_plus4) < 0) {
            return -1;
        }
    }
#if 0
    if (machine_class == VICE_MACHINE_PET || machine_class == VICE_MACHINE_CBM6x0) {
        if (cmdline_register_options(cmdline_options_crtc) < 0) {
            return -1;
        }
    }
#endif
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------ */

static void artstudio_check_and_correct_cell(native_data_t *source)
{
    native_data_t *dest = lib_malloc(sizeof(native_data_t));
    int i, j, k, l;
    native_color_sort_t *colors = NULL;

    dest->xsize = 8;
    dest->ysize = 8;
    dest->colormap = lib_malloc(8 * 8);

    for (i = 0; i < ARTSTUDIO_SCREEN_BYTE_HEIGHT; i++) {
        for (j = 0; j < ARTSTUDIO_SCREEN_BYTE_WIDTH; j++) {
            /* get block */
            for (k = 0; k < 8; k++) {
                for (l = 0; l < 8; l++) {
                    dest->colormap[(k * 8) + l] =
                        source->colormap[(i * 8 * ARTSTUDIO_SCREEN_PIXEL_WIDTH) + (j * 8)
                                            + (k * ARTSTUDIO_SCREEN_PIXEL_WIDTH) + l];
                }
            }
            /* if there are more than 2 colors in the block, re-render it using
               the two most used colors in the block */
            colors = native_sort_colors_colormap(dest, 16);
            if (colors[2].amount != 0) {
                colors[2].color = 255;
                vicii_color_to_nearest_vicii_color_colormap(dest, colors);
                for (k = 0; k < 8; k++) {
                    for (l = 0; l < 8; l++) {
                        source->colormap[(i * 8 * ARTSTUDIO_SCREEN_PIXEL_WIDTH) + (j * 8)
                                            + (k * ARTSTUDIO_SCREEN_PIXEL_WIDTH) + l] =
                            dest->colormap[(k * 8) + l];
                    }
                }
            }
            lib_free(colors);
        }
    }
    lib_free(dest->colormap);
    lib_free(dest);
}

static int artstudio_render_and_save(native_data_t *source)
{
    FILE *fd;
    char *filename_ext = NULL;
    uint8_t *filebuffer = NULL;
    uint8_t *result = NULL;
    int i, j, k, l;
    int m = 0;
    int n = 0;
    int retval = 0;
    uint8_t fgcolor = 0;
    uint8_t bgcolor;
    uint8_t colorbyte;

    /* allocate file buffer */
    filebuffer = lib_malloc(ARTSTUDIO_SIZE);

    /* clear filebuffer */
    memset(filebuffer, 0, ARTSTUDIO_SIZE);

    /* set load addy */
    filebuffer[0] = 0x00;
    filebuffer[1] = 0x20;

    for (i = 0; i < ARTSTUDIO_SCREEN_BYTE_HEIGHT; i++) {
        for (j = 0; j < ARTSTUDIO_SCREEN_BYTE_WIDTH; j++) {
            fgcolor = bgcolor = 255;
            for (k = 0; k < 8; k++) {
                filebuffer[BITMAP_OFFSET + m] = 0;
                for (l = 0; l < 8; l++) {
                    colorbyte = source->colormap[(i * ARTSTUDIO_SCREEN_PIXEL_WIDTH * 8)
                                    + (j * 8) + (k * ARTSTUDIO_SCREEN_PIXEL_WIDTH) + l];
                    if ((colorbyte != fgcolor) && (fgcolor == 255)) {
                        fgcolor = colorbyte;
                    } else if ((colorbyte != bgcolor) && (colorbyte != fgcolor) && (bgcolor == 255)) {
                        bgcolor = colorbyte;
                    }
                    if (colorbyte == fgcolor) {
                        filebuffer[BITMAP_OFFSET + m] |= (1 << (7 - l));
                    }
                }
                m++;
            }
            filebuffer[VIDEORAM_OFFSET + n++] = ((fgcolor & 0xf) << 4) | (bgcolor & 0xf);
        }
    }

    filename_ext = util_add_extension_const(source->filename, artstudio_drv.default_extension);

    fd = fopen(filename_ext, MODE_WRITE);
    if (fd == NULL) {
        retval = -1;
    }

    if (retval != -1) {
        if (fwrite(filebuffer, ARTSTUDIO_SIZE, 1, fd) < 1) {
            retval = -1;
        }
    }

    if (fd != NULL) {
        fclose(fd);
    }

    lib_free(source->colormap);
    lib_free(source);
    lib_free(filename_ext);
    lib_free(filebuffer);
    lib_free(result);

    return retval;
}

/* ------------------------------------------------------------------------ */

static int artstudio_multicolor_render(native_data_t *data)
{
    native_color_sort_t *color_order = NULL;

    switch (multicolor_handling) {
        case NATIVE_SS_MC2HR_BLACK_WHITE:
            vicii_color_to_vicii_bw_colormap(data);
            break;
        case NATIVE_SS_MC2HR_GRAY:
            vicii_color_to_vicii_gray_colormap(data);
            artstudio_check_and_correct_cell(data);
            break;
        case NATIVE_SS_MC2HR_2_COLORS:
            color_order = native_sort_colors_colormap(data, 16);
            color_order[2].color = 255;
            vicii_color_to_nearest_vicii_color_colormap(data, color_order);
            lib_free(color_order);
            artstudio_check_and_correct_cell(data);
            break;
        case NATIVE_SS_MC2HR_4_COLORS:
            color_order = native_sort_colors_colormap(data, 16);
            color_order[4].color = 255;
            vicii_color_to_nearest_vicii_color_colormap(data, color_order);
            lib_free(color_order);
            artstudio_check_and_correct_cell(data);
            break;
        case NATIVE_SS_MC2HR_DITHER:
            color_order = native_sort_colors_colormap(data, 16);
            vicii_color_to_nearest_vicii_color_colormap(data, color_order);
            lib_free(color_order);
            artstudio_check_and_correct_cell(data);
            break;
        default:
            return -1;
            break;
    }
    return 0;
}

static int artstudio_vicii_save(screenshot_t *screenshot, const char *filename)
{
    native_data_t *data = NULL;
#if 0
    uint8_t *regs = screenshot->video_regs;
    uint8_t mc;
    uint8_t eb;
    uint8_t bm;
    uint8_t blank;

    mc = (regs[0x16] & 0x10) >> 4;
    eb = (regs[0x11] & 0x40) >> 6;
    bm = (regs[0x11] & 0x20) >> 5;

    blank = (regs[0x11] & 0x10) >> 4;

    if (!blank) {
        ui_error("Screen is blanked, no picture to save");
        return -1;
    }

    switch (mc << 2 | eb << 1 | bm) {
        case 0:    /* normal text mode */
            data = native_vicii_text_mode_render(screenshot, filename);
            break;
        case 1:    /* hires bitmap mode */
            data = native_vicii_hires_bitmap_mode_render(screenshot, filename);
            break;
        case 2:    /* extended background mode */
            data = native_vicii_extended_background_mode_render(screenshot, filename);
            break;
        case 4:    /* multicolor text mode */
            data = native_vicii_multicolor_text_mode_render(screenshot, filename);
            break;
        case 5:    /* multicolor bitmap mode */
            data = native_vicii_multicolor_bitmap_mode_render(screenshot, filename);
            break;
        default:   /* illegal modes (3, 6 and 7) */
            ui_error("Illegal mode, no saving will be done");
            return -1;
            break;
    }
#endif
    data = native_vicii_render(screenshot, filename);
    if (data == NULL) {
        return -1;
    }
    if (data->mc_data_present) {
        if (artstudio_multicolor_render(data) != 0) {
            return -1;
        }
    }
    return artstudio_render_and_save(data);
}

/* ------------------------------------------------------------------------ */

static int artstudio_ted_save(screenshot_t *screenshot, const char *filename)
{
    native_data_t *data = NULL;
#if 0
    uint8_t *regs = screenshot->video_regs;
    uint8_t mc;
    uint8_t eb;
    uint8_t bm;

    mc = (regs[0x07] & 0x10) >> 4;
    eb = (regs[0x06] & 0x40) >> 6;
    bm = (regs[0x06] & 0x20) >> 5;

    switch (mc << 2 | eb << 1 | bm) {
        case 0:    /* normal text mode */
            data = native_ted_text_mode_render(screenshot, filename);
            break;
        case 1:    /* hires bitmap mode */
            data = native_ted_hires_bitmap_mode_render(screenshot, filename);
            break;
        case 2:    /* extended background mode */
            data = native_ted_extended_background_mode_render(screenshot, filename);
            break;
        case 4:    /* multicolor text mode */
            ui_error("This screen saver is a WIP, it doesn't support multicolor text mode (yet)");
            return -1;
            break;
        case 5:    /* multicolor bitmap mode */
            data = native_ted_multicolor_bitmap_mode_render(screenshot, filename);
            break;
        default:   /* illegal modes (3, 6 and 7) */
            ui_error("Illegal mode, no saving will be done");
            return -1;
            break;
    }
#endif
    data = native_ted_render(screenshot, filename);
    if (data == NULL) {
        return -1;
    }
    ted_color_to_vicii_color_colormap(data, ted_lum_handling);
    if (data->mc_data_present) {
        if (artstudio_multicolor_render(data) != 0) {
            return -1;
        }
    }
    return artstudio_render_and_save(data);
}

/* ------------------------------------------------------------------------ */

static int artstudio_vic_save(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    native_data_t *data = native_vic_render(screenshot, filename);
    native_color_sort_t *color_order = NULL;
    uint8_t bordercolor = regs[0xf] & 7;

    if (data == NULL) {
        return -1;
    }

    vic_color_to_vicii_color_colormap(data);

    if (data->xsize != ARTSTUDIO_SCREEN_PIXEL_WIDTH || data->ysize != ARTSTUDIO_SCREEN_PIXEL_HEIGHT) {
        data = native_resize_colormap(data, ARTSTUDIO_SCREEN_PIXEL_WIDTH, ARTSTUDIO_SCREEN_PIXEL_HEIGHT,
                                        bordercolor, oversize_handling, undersize_handling);
    }

    if (data->mc_data_present) {
        switch (multicolor_handling) {
            case NATIVE_SS_MC2HR_BLACK_WHITE:
                vicii_color_to_vicii_bw_colormap(data);
                break;
            case NATIVE_SS_MC2HR_GRAY:
                vicii_color_to_vicii_gray_colormap(data);
                artstudio_check_and_correct_cell(data);
                break;
            case NATIVE_SS_MC2HR_2_COLORS:
                color_order = native_sort_colors_colormap(data, 16);
                color_order[2].color = 255;
                vicii_color_to_nearest_vicii_color_colormap(data, color_order);
                lib_free(color_order);
                artstudio_check_and_correct_cell(data);
                break;
            case NATIVE_SS_MC2HR_4_COLORS:
                color_order = native_sort_colors_colormap(data, 16);
                color_order[4].color = 255;
                vicii_color_to_nearest_vicii_color_colormap(data, color_order);
                lib_free(color_order);
                artstudio_check_and_correct_cell(data);
                break;
            case NATIVE_SS_MC2HR_DITHER:
                color_order = native_sort_colors_colormap(data, 16);
                vicii_color_to_nearest_vicii_color_colormap(data, color_order);
                lib_free(color_order);
                artstudio_check_and_correct_cell(data);
                break;
            default:
                return -1;
                break;
        }
    }
    return artstudio_render_and_save(data);
}

/* ------------------------------------------------------------------------ */

static int artstudio_crtc_save(screenshot_t *screenshot, const char *filename)
{
    native_data_t *data = native_crtc_render(screenshot, filename);

    if (data == NULL) {
        return -1;
    }

    if (data->xsize != ARTSTUDIO_SCREEN_PIXEL_WIDTH || data->ysize != ARTSTUDIO_SCREEN_PIXEL_HEIGHT) {
        data = native_resize_colormap(data, ARTSTUDIO_SCREEN_PIXEL_WIDTH, ARTSTUDIO_SCREEN_PIXEL_HEIGHT,
                                        0, oversize_handling, undersize_handling);
    }
    return artstudio_render_and_save(data);
}

/* ------------------------------------------------------------------------ */

static int artstudio_vdc_save(screenshot_t *screenshot, const char *filename)
{
    native_data_t *data = NULL;
#if 0
    uint8_t *regs = screenshot->video_regs;

    if (regs[25] & 0x80) {
        ui_error("VDC bitmap mode screenshot saving not implemented yet");
        return -1;
    }
#endif
    data = native_vdc_render(screenshot, filename);
    if (data == NULL) {
        return -1;
    }
    vdc_color_to_vicii_color_colormap(data);
    if (data->xsize != ARTSTUDIO_SCREEN_PIXEL_WIDTH || data->ysize != ARTSTUDIO_SCREEN_PIXEL_HEIGHT) {
        data = native_resize_colormap(data, ARTSTUDIO_SCREEN_PIXEL_WIDTH, ARTSTUDIO_SCREEN_PIXEL_HEIGHT,
                                        0, oversize_handling, undersize_handling);
    }
    return artstudio_render_and_save(data);
}

/* ------------------------------------------------------------------------ */

static int artstudiodrv_save(screenshot_t *screenshot, const char *filename)
{
    if (!(strcmp(screenshot->chipid, "VICII"))) {
        return artstudio_vicii_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "VDC"))) {
        return artstudio_vdc_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "CRTC"))) {
        return artstudio_crtc_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "TED"))) {
        return artstudio_ted_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "VIC"))) {
        return artstudio_vic_save(screenshot, filename);
    }
    ui_error("Unknown graphics chip");
    return -1;
}

static gfxoutputdrv_t artstudio_drv =
{
    GFXOUTPUTDRV_TYPE_SCREENSHOT_NATIVE,
    "ARTSTUDIO",
    "OCP Artstudio screenshot",
    "ocp",
    NULL, /* formatlist */
    NULL,
    NULL,
    NULL,
    NULL,
    artstudiodrv_save,
    NULL,
    NULL,
    artstudiodrv_resources_init,
    artstudiodrv_cmdline_options_init
#ifdef FEATURE_CPUMEMHISTORY
    , NULL
#endif
};

void gfxoutput_init_artstudio(int help)
{
    gfxoutput_register(&artstudio_drv);
}
