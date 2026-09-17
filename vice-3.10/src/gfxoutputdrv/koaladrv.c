/*
 * koaladrv.c - Create a c64 koala type file.
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

/* TODO:

    when all is done, remove #if 0'ed code
*/

#define KOALA_SCREEN_PIXEL_WIDTH   320
#define KOALA_SCREEN_PIXEL_HEIGHT  200

#define KOALA_SCREEN_BYTE_WIDTH    KOALA_SCREEN_PIXEL_WIDTH / 8
#define KOALA_SCREEN_BYTE_HEIGHT   KOALA_SCREEN_PIXEL_HEIGHT / 8

/* define offsets in the koala file */
#define BITMAP_OFFSET 2
#define SCREENRAM_OFFSET 8002
#define VIDEORAM_OFFSET 9002
#define BGCOLOR_OFFSET 10002
#define KOALA_SIZE 10003

static gfxoutputdrv_t koala_drv;

/* ------------------------------------------------------------------------ */

static int oversize_handling;
static int undersize_handling;
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
    { "KoalaOversizeHandling", NATIVE_SS_OVERSIZE_SCALE, RES_EVENT_NO, NULL,
      &oversize_handling, set_oversize_handling, NULL },
    { "KoalaUndersizeHandling", NATIVE_SS_UNDERSIZE_BORDERIZE, RES_EVENT_NO, NULL,
      &undersize_handling, set_undersize_handling, NULL },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_plus4[] = {
    { "KoalaTEDLumHandling", NATIVE_SS_TED_LUM_DITHER, RES_EVENT_NO, NULL,
      &ted_lum_handling, set_ted_lum_handling, NULL },
    RESOURCE_INT_LIST_END
};

#if 0
static const resource_int_t resources_int_crtc[] = {
    { "KoalaCRTCTextColor", NATIVE_SS_CRTC_WHITE, RES_EVENT_NO, NULL,
      &crtc_text_color, set_crtc_text_color, NULL },
    RESOURCE_INT_LIST_END
};
#endif

static int koaladrv_resources_init(void)
{
    if (machine_class == VICE_MACHINE_PLUS4) {
        if (resources_register_int(resources_int_plus4) < 0) {
            return -1;
        }
    }
#if 0
    if (machine_class == VICE_MACHINE_CBM6x0 || machine_class == VICE_MACHINE_PET) {
        if (resources_register_int(resources_int_crtc) < 0) {
            return -1;
        }
    }
#endif
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-koalaoversize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KoalaOversizeHandling", NULL,
      "<method>", "Select the way the oversized input should be handled,"
      " (0: scale down, 1: crop left top, 2: crop center top,  3: crop right top,"
      " 4: crop left center, 5: crop center, 6: crop right center, 7: crop left bottom,"
      " 8: crop center bottom, 9:  crop right bottom)" },
    { "-koalaundersize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KoalaUndersizeHandling", NULL,
      "<method>", "Select the way the undersized input should be handled,"
      " (0: scale up, 1: borderize)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t cmdline_options_plus4[] =
{
    { "-koalatedlum", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KoalaTEDLumHandling", NULL,
      "<method>", "Select the way the TED luminosity should be handled,"
      " (0: ignore, 1: dither)" },
    CMDLINE_LIST_END
};

#if 0
static const cmdline_option_t cmdline_options_crtc[] =
{
    { "-koalacrtctextcolor", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KoalaCRTCTextColor", NULL,
      "<color>", "Select the CRTC text color (0: white, 1: amber, 2: green)" },
    CMDLINE_LIST_END
};
#endif

static int koaladrv_cmdline_options_init(void)
{
    if (machine_class == VICE_MACHINE_PLUS4) {
        if (cmdline_register_options(cmdline_options_plus4) < 0) {
            return -1;
        }
    }
#if 0
    if (machine_class == VICE_MACHINE_CBM6x0 || machine_class == VICE_MACHINE_PET) {
        if (cmdline_register_options(cmdline_options_crtc) < 0) {
            return -1;
        }
    }
#endif
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------ */

/* make all pixels double pixels */
static void koala_multicolorize_colormap(native_data_t *source)
{
    int i, j;

    for (i = 0; i < KOALA_SCREEN_PIXEL_HEIGHT; i++) {
        for (j = 0; j < (KOALA_SCREEN_PIXEL_WIDTH / 2); j++) {
            source->colormap[(i * KOALA_SCREEN_PIXEL_WIDTH) + (j * 2) + 1] =
            source->colormap[(i * KOALA_SCREEN_PIXEL_WIDTH) + (j * 2)];
        }
    }
}

/* find the best background color. for this we pick the color that appears
   most often in blocks that contain 4, or more, colors */
static uint8_t koala_find_bgcolor(native_data_t *source)
{
    native_data_t *dest = lib_malloc(sizeof(native_data_t));
    native_color_sort_t *blockcolors = NULL;
    native_color_sort_t bgcolors[16];
    int i, j, k, l, c;
    uint8_t bgcolor, amount;

    /* color map for one block */
    dest->xsize = 8;
    dest->ysize = 8;
    dest->colormap = lib_malloc(8 * 8);

    for (c = 0; c < 16; c++) {
        bgcolors[c].amount = 0;
        bgcolors[c].color = c;
    }

    for (i = 0; i < KOALA_SCREEN_BYTE_HEIGHT; i++) {
        for (j = 0; j < KOALA_SCREEN_BYTE_WIDTH; j++) {
            /* get block */
            for (k = 0; k < 8; k++) {
                for (l = 0; l < 8; l++) {
                    dest->colormap[(k * 8) + l] =
                    source->colormap[(i * 8 * KOALA_SCREEN_PIXEL_WIDTH) + (j * 8)
                                        + (k * KOALA_SCREEN_PIXEL_WIDTH) + l];
                }
            }
            blockcolors = native_sort_colors_colormap(dest, 16);
            if (blockcolors[3].amount != 0) {
                /* 4 or more colors in this block, add them to the pool of
                    colors we pick the background color from */
                for (c = 0; c < 16; c++) {
                    if (blockcolors[c].amount != 0) {
                        bgcolors[blockcolors[c].color].amount++;
                    }
                }
            }
            lib_free(blockcolors);
        }
    }

    /* pick the most used color from the pool */
    amount = bgcolors[0].amount;
    bgcolor = 0;
    for (c = 1; c < 16; c++) {
        if (amount < bgcolors[c].amount) {
            amount = bgcolors[c].amount;
            bgcolor = c;
        }
    }

    lib_free(dest->colormap);
    lib_free(dest);
    return bgcolor;
}

/* fix/re-render the picture according to the multicolor restrictions, one
   common background color, plus 3 colors per block */
static void koala_check_and_correct_cell(native_data_t *source, uint8_t bgcolor)
{
    native_data_t *dest = lib_malloc(sizeof(native_data_t));
    int i, j, k, l;
    native_color_sort_t *colors = NULL;
    native_color_sort_t cellcolors[16];

    dest->xsize = 8;
    dest->ysize = 8;
    dest->colormap = lib_malloc(8 * 8);

    for (i = 0; i < KOALA_SCREEN_BYTE_HEIGHT; i++) {
        for (j = 0; j < KOALA_SCREEN_BYTE_WIDTH; j++) {
            /* get block */
            for (k = 0; k < 8; k++) {
                for (l = 0; l < 8; l++) {
                    dest->colormap[(k * 8) + l] =
                        source->colormap[(i * 8 * KOALA_SCREEN_PIXEL_WIDTH) + (j * 8)
                                            + (k * KOALA_SCREEN_PIXEL_WIDTH) + l];
                }
            }
            colors = native_sort_colors_colormap(dest, 16);
            /* the first color is expected to be the background color in the
               next step. so put it first, followed by the other colors */
            cellcolors[0].amount = 8000;
            cellcolors[0].color = bgcolor;
            for (l = 0, k = 1; l < 16; l++) {
                if (colors[l].color != bgcolor) {
                    cellcolors[k].color = colors[l].color;
                    cellcolors[k].amount = colors[l].amount;
                    k++;
                }
            }
            /* re-render the block using the background color and the 3 most
               used color in that block */
            cellcolors[4].color = 255; /* mark end of list */
            vicii_color_to_nearest_vicii_color_colormap(dest, cellcolors);
            for (k = 0; k < 8; k++) {
                for (l = 0; l < 8; l++) {
                    source->colormap[(i * 8 * KOALA_SCREEN_PIXEL_WIDTH) + (j * 8)
                                        + (k * KOALA_SCREEN_PIXEL_WIDTH) + l] =
                        dest->colormap[(k * 8) + l];
                }
            }
            lib_free(colors);
        }
    }
    lib_free(dest->colormap);
    lib_free(dest);
}

static int koala_render_and_save(native_data_t *source)
{
    FILE *fd;
    char *filename_ext = NULL;
    uint8_t *filebuffer = NULL;
    uint8_t *result = NULL;
    int i, j, k, l;
    int m = 0;
    int n = 0;
    int retval = 0;
    uint8_t color1;
    uint8_t color2;
    uint8_t color3;
    uint8_t colorbyte;
    uint8_t bgcolor;
    uint8_t gfxbits;

    /* allocate file buffer */
    filebuffer = lib_malloc(KOALA_SIZE);

    /* clear filebuffer */
    memset(filebuffer, 0, KOALA_SIZE);

    /* set load addy */
    filebuffer[0] = 0x00;
    filebuffer[1] = 0x60;

    /* make all pixels multicolor */
    koala_multicolorize_colormap(source);

    /* find out bgcolor */
    bgcolor = koala_find_bgcolor(source);

    /* check and correct cells */
    koala_check_and_correct_cell(source, bgcolor);

    for (i = 0; i < KOALA_SCREEN_BYTE_HEIGHT; i++) {
        for (j = 0; j < KOALA_SCREEN_BYTE_WIDTH; j++) {
            /* one block */
            color1 = color2 = color3 = 255;
            for (k = 0; k < 8; k++) {
                gfxbits = 0;
                for (l = 0; l < 4; l++) {
                    gfxbits <<= 2;
                    colorbyte = source->colormap[(i * KOALA_SCREEN_PIXEL_WIDTH * 8) + (j * 8)
                                                    + (k * KOALA_SCREEN_PIXEL_WIDTH) + (l * 2)];
                    /* remember the used colors in this block. so we can assign
                       the bits */
                    if (colorbyte != bgcolor) {
                        if (color1 == 255) {
                            color1 = colorbyte;
                        } else if ((color1 != colorbyte) && (color2 == 255)) {
                            color2 = colorbyte;
                        } else if ((color1 != colorbyte) && (color2 != colorbyte) && (color3 == 255)) {
                            color3 = colorbyte;
                        }
                    }
                    /* assign the bits */
                    if (colorbyte == color1) {
                        gfxbits |= 1;
                    } else if (colorbyte == color2) {
                        gfxbits |= 2;
                    } else if (colorbyte == color3) {
                        gfxbits |= 3;
                    }
                }
                filebuffer[BITMAP_OFFSET + m] = gfxbits;
                m++;
            }
            /* put the colors into video ram and color ram buffers */
            filebuffer[SCREENRAM_OFFSET + n] = ((color1 & 0xf) << 4) | (color2 & 0xf);
            filebuffer[VIDEORAM_OFFSET + n] = color3 & 0xf;
            n++;
        }
    }
    filebuffer[BGCOLOR_OFFSET] = bgcolor;

    filename_ext = util_add_extension_const(source->filename, koala_drv.default_extension);

    fd = fopen(filename_ext, MODE_WRITE);
    if (fd == NULL) {
        retval = -1;
    }

    if (retval != -1) {
        if (fwrite(filebuffer, KOALA_SIZE, 1, fd) < 1) {
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

static int koala_vicii_save(screenshot_t *screenshot, const char *filename)
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
    return koala_render_and_save(data);
}

/* ------------------------------------------------------------------------ */

static int koala_ted_save(screenshot_t *screenshot, const char *filename)
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
    return koala_render_and_save(data);
}

/* ------------------------------------------------------------------------ */

static int koala_vic_save(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    native_data_t *data = native_vic_render(screenshot, filename);
    uint8_t bordercolor = regs[0xf] & 7;

    if (data == NULL) {
        return -1;
    }

    vic_color_to_vicii_color_colormap(data);

    if ((data->xsize != KOALA_SCREEN_PIXEL_WIDTH) || (data->ysize != KOALA_SCREEN_PIXEL_HEIGHT)) {
        data = native_resize_colormap(data, KOALA_SCREEN_PIXEL_WIDTH, KOALA_SCREEN_PIXEL_HEIGHT,
                                        bordercolor, oversize_handling, undersize_handling);
    }

    return koala_render_and_save(data);
}

/* ------------------------------------------------------------------------ */

static int koala_crtc_save(screenshot_t *screenshot, const char *filename)
{
    native_data_t *data = native_crtc_render(screenshot, filename);
    if (data == NULL) {
        return -1;
    }

    if (data->xsize != KOALA_SCREEN_PIXEL_WIDTH || data->ysize != KOALA_SCREEN_PIXEL_HEIGHT) {
        data = native_resize_colormap(data, KOALA_SCREEN_PIXEL_WIDTH, KOALA_SCREEN_PIXEL_HEIGHT,
                                        0, oversize_handling, undersize_handling);
    }
    return koala_render_and_save(data);
}

/* ------------------------------------------------------------------------ */

static int koala_vdc_save(screenshot_t *screenshot, const char *filename)
{
    native_data_t *data = NULL;
#if 0
    uint8_t *regs = screenshot->video_regs;

    if (regs[25] & 0x80) {
        ui_error("VDC bitmap mode screenshot saving not implemented yet");
        return -1;
    }
    data = native_vdc_text_mode_render(screenshot, filename);
#endif
    data = native_vdc_render(screenshot, filename);
    if (data == NULL) {
        return -1;
    }

    vdc_color_to_vicii_color_colormap(data);
    if (data->xsize != KOALA_SCREEN_PIXEL_WIDTH || data->ysize != KOALA_SCREEN_PIXEL_HEIGHT) {
        data = native_resize_colormap(data, KOALA_SCREEN_PIXEL_WIDTH, KOALA_SCREEN_PIXEL_HEIGHT,
                                        0, oversize_handling, undersize_handling);
    }
    return koala_render_and_save(data);
}

/* ------------------------------------------------------------------------ */

static int koaladrv_save(screenshot_t *screenshot, const char *filename)
{
    if (!(strcmp(screenshot->chipid, "VICII"))) {
        return koala_vicii_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "VDC"))) {
        return koala_vdc_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "CRTC"))) {
        return koala_crtc_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "TED"))) {
        return koala_ted_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "VIC"))) {
        return koala_vic_save(screenshot, filename);
    }
    ui_error("Unknown graphics chip");
    return -1;
}

static gfxoutputdrv_t koala_drv =
{
    GFXOUTPUTDRV_TYPE_SCREENSHOT_NATIVE,
    "KOALA",
    "Koalapainter screenshot",
    "koa",
    NULL, /* formatlist */
    NULL,
    NULL,
    NULL,
    NULL,
    koaladrv_save,
    NULL,
    NULL,
    koaladrv_resources_init,
    koaladrv_cmdline_options_init
#ifdef FEATURE_CPUMEMHISTORY
    , NULL
#endif
};

void gfxoutput_init_koala(int help)
{
    gfxoutput_register(&koala_drv);
}
