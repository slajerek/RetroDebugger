/*
 * minipaintdrv.c - Create a vic20 minipaint type file.
 *
 * Written by
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

/* #define DEBUGMINIPAINT */

#ifdef  DEBUGMINIPAINT
#define DBG(_x_)    printf _x_
#else
#define DBG(_x_)
#endif

/*
    Minipaint format saver, this uses a standard vic20 screen with 16 pixel
    high characters in the following configuration:

    900f bit3 (inverted mode) = 1 (normal video)

    multicolor blocks:

    00    bgcolor     900f bit 7-4                (colors 0...15)
    01    bordercolor 900f bit 2-0                (colors 0..7)
    10    charcolor   cram bit 2-0   bit 3 = 1    (colors 0..7)
    11    auxcolor    900e bit 6-4                (colors 0...15)

    hires blocks:

     0    bgcolor     900f bit 7-4                (colors 0...15)
     1    charcolor   cram bit 2-0   bit 3 = 0    (colors 0..7)

    colors (all)        (bg and aux only)

        0 - black         8 - orange
        1 - white         9 - light orange
        2 - red          10 - pink
        3 - cyan         11 - light cyan
        4 - magenta      12 - light magenta
        5 - green        13 - light green
        6 - blue         14 - light blue
        7 - yellow       15 - light yellow

    TODO:
    - hires blocks are not handled yet
    - the algorithm for finding the best shared colors could be improved
*/

#define VIC_NUM_COLORS_BG_AUX   16

#define MINIPAINT_SCREEN_PIXEL_WIDTH   160
#define MINIPAINT_SCREEN_PIXEL_HEIGHT  192

#define MINIPAINT_BLOCK_WIDTH   8
#define MINIPAINT_BLOCK_HEIGHT  16

#define MINIPAINT_SCREEN_CHARS_WIDTH    (MINIPAINT_SCREEN_PIXEL_WIDTH / MINIPAINT_BLOCK_WIDTH)
#define MINIPAINT_SCREEN_CHARS_HEIGHT   (MINIPAINT_SCREEN_PIXEL_HEIGHT / MINIPAINT_BLOCK_HEIGHT)

/* define offsets in the minipaint file */
#define AUXCOLOR_OFFSET     15
#define BGCOLOR_OFFSET      16
#define BITMAP_OFFSET       17
#define VIDEORAM_OFFSET     3857
#define DISPLAYER_OFFSET    3977
#define MINIPAINT_SIZE      4097

static gfxoutputdrv_t minipaint_drv;

static unsigned char headerdata[17] = {
    0xf1, 0x10, 0x0c, 0x12, 0xd8, 0x07, 0x9e, 0x20, 0x38, 0x35, 0x38, 0x34, 0x00, 0x00, 0x00, 0x60, 0x0d
};
static unsigned char displaycode[120] = {
    0x18, 0xA9, 0x10, 0xA8, 0x99, 0xF0, 0x0F, 0x69, 0x0C, 0x90, 0x02, 0xE9, 0xEF, 0xC8, 0xD0, 0xF4,
    0xA0, 0x05, 0x18, 0xB9, 0xE4, 0xED, 0x79, 0xFA, 0x21, 0x99, 0x00, 0x90, 0x88, 0x10, 0xF3, 0xAD,
    0x0E, 0x90, 0x29, 0x0F, 0x0D, 0x0E, 0x12, 0x8D, 0x0E, 0x90, 0xAD, 0x0F, 0x12, 0x8D, 0x0F, 0x90,
    0xA9, 0x10, 0x85, 0xFB, 0xA9, 0x12, 0x85, 0xFC, 0xA9, 0x00, 0x85, 0xFD, 0xA9, 0x11, 0x85, 0xFE,
    0xA2, 0x0F, 0xA0, 0x00, 0xB1, 0xFB, 0x91, 0xFD, 0xC8, 0xD0, 0xF9, 0xE6, 0xFC, 0xE6, 0xFE, 0xCA,
    0xD0, 0xF2, 0xA2, 0x00, 0xA0, 0x00, 0xBD, 0x10, 0x21, 0xE8, 0x99, 0x00, 0x94, 0xC8, 0x4A, 0x4A,
    0x4A, 0x4A, 0x99, 0x00, 0x94, 0xC8, 0xC0, 0xF0, 0xD0, 0xEC, 0x20, 0xE4, 0xFF, 0xF0, 0xFB, 0x4C,
    0x32, 0xFD, 0x02, 0xFE, 0xFE, 0xEB, 0x00, 0x0C
};

static unsigned char hiresmap[MINIPAINT_SCREEN_CHARS_HEIGHT][MINIPAINT_SCREEN_CHARS_WIDTH];
static int containshires;

/* ------------------------------------------------------------------------ */

static int oversize_handling;
static int undersize_handling;
static int ted_lum_handling;

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

static const resource_int_t resources_int[] = {
    { "MiniPaintOversizeHandling", NATIVE_SS_OVERSIZE_SCALE, RES_EVENT_NO, NULL,
      &oversize_handling, set_oversize_handling, NULL },
    { "MiniPaintUndersizeHandling", NATIVE_SS_UNDERSIZE_BORDERIZE, RES_EVENT_NO, NULL,
      &undersize_handling, set_undersize_handling, NULL },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_int_plus4[] = {
    { "MiniPaintTEDLumHandling", NATIVE_SS_TED_LUM_DITHER, RES_EVENT_NO, NULL,
      &ted_lum_handling, set_ted_lum_handling, NULL },
    RESOURCE_INT_LIST_END
};

static int minipaintdrv_resources_init(void)
{
    if (machine_class == VICE_MACHINE_PLUS4) {
        if (resources_register_int(resources_int_plus4) < 0) {
            return -1;
        }
    }
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-minipaintoversize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MiniPaintOversizeHandling", NULL,
      "<method>", "Select the way the oversized input should be handled,"
      " (0: scale down, 1: crop left top, 2: crop center top,  3: crop right top,"
      " 4: crop left center, 5: crop center, 6: crop right center, 7: crop left bottom,"
      " 8: crop center bottom, 9:  crop right bottom)" },
    { "-minipaintundersize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MiniPaintUndersizeHandling", NULL,
      "<method>", "Select the way the undersized input should be handled,"
      " (0: scale up, 1: borderize)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t cmdline_options_plus4[] =
{
    { "-minipainttedlum", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MiniPaintTEDLumHandling", NULL,
      "<method>", "Select the way the TED luminosity should be handled,"
      " (0: ignore, 1: dither)" },
    CMDLINE_LIST_END
};

static int minipaintdrv_cmdline_options_init(void)
{
    if (machine_class == VICE_MACHINE_PLUS4) {
        if (cmdline_register_options(cmdline_options_plus4) < 0) {
            return -1;
        }
    }
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------ */

#define UNASSIGNED_COLOR    16

/* a block is considered a hires block if not all odd and even pixels are the
   same color, we have exactly two colors in this block, and no color index
   is > 7 */
static void minipaint_find_hires_colors(native_data_t *source, uint8_t *bgcolor)
{
    native_data_t *dest = lib_malloc(sizeof(native_data_t));
    native_color_sort_t *blockcolors = NULL;
    native_color_sort_t bgcolors[VIC_NUM_COLORS_BG_AUX];
    int screeny, screenx, blocky, blockx, c, c1, c2, ishires;
    uint8_t amount;

    /* color map for one block */
    dest->xsize = MINIPAINT_BLOCK_WIDTH;
    dest->ysize = MINIPAINT_BLOCK_HEIGHT;
    dest->colormap = lib_malloc(MINIPAINT_BLOCK_WIDTH * MINIPAINT_BLOCK_HEIGHT);

    for (c = 0; c < VIC_NUM_COLORS_BG_AUX; c++) {
        bgcolors[c].amount = 0;
        bgcolors[c].color = c;
    }

    for (screeny = 0; screeny < MINIPAINT_SCREEN_CHARS_HEIGHT; screeny++) {
        for (screenx = 0; screenx < MINIPAINT_SCREEN_CHARS_WIDTH; screenx++) {
            /* get block */
            for (blocky = 0; (blocky < MINIPAINT_BLOCK_HEIGHT); blocky++) {
                for (blockx = 0; (blockx < MINIPAINT_BLOCK_WIDTH); blockx++) {
                    dest->colormap[(blocky * MINIPAINT_BLOCK_WIDTH) + blockx] =
                        source->colormap[(screeny * MINIPAINT_BLOCK_HEIGHT * MINIPAINT_SCREEN_PIXEL_WIDTH) +
                                         (screenx * MINIPAINT_BLOCK_WIDTH) +
                                         (blocky * MINIPAINT_SCREEN_PIXEL_WIDTH) + blockx];
                }
            }

            /* check all pixels in the block, if we find an odd and even pixel
               with different color, then this can be a hires block */
            ishires = 0;
            for (blocky = 0; (blocky < MINIPAINT_BLOCK_HEIGHT) && (ishires == 0); blocky++) {
                for (blockx = 0; (blockx < MINIPAINT_BLOCK_WIDTH) && (ishires == 0); blockx++) {
                    c1 = dest->colormap[(blocky * MINIPAINT_BLOCK_WIDTH) + blockx];
                    blockx++;
                    c2 = dest->colormap[(blocky * MINIPAINT_BLOCK_WIDTH) + blockx];
                    if (c1 != c2) {
                        ishires = 1;
                    }
                }
            }

            if (ishires) {
                blockcolors = native_sort_colors_colormap(dest, VIC_NUM_COLORS_BG_AUX);
                if ((blockcolors[1].amount > 0) && (blockcolors[2].amount == 0)) {
                    /* exactly two colors in this block */

                    /* if both colors are > 7, then this can not be a hires block */
                    if (!((blockcolors[1].color > 7) && (blockcolors[0].color > 7))) {
                        containshires = 1;
                        hiresmap[screeny][screenx] = (blockcolors[1].color << 4) | blockcolors[0].color;

                        /* add them to the pool of colors we pick the background color from */
                        for (c = 0; c < VIC_NUM_COLORS_BG_AUX; c++) {
                            /* skip if the color was already assigned in a previous pass */
                            if ((*bgcolor != UNASSIGNED_COLOR) && (blockcolors[c].color == *bgcolor)){
                                continue;
                            }
                            if (blockcolors[c].amount != 0) {
                                bgcolors[blockcolors[c].color].amount++;
                            }
                        }
                    }
                }
                lib_free(blockcolors);
            }
        }
    }

    /* pick the most used colors from the pool */
    if (*bgcolor == UNASSIGNED_COLOR) {
        amount = 0;
        for (c = 0; c < VIC_NUM_COLORS_BG_AUX; c++) {
            if (amount < bgcolors[c].amount) {
                amount = bgcolors[c].amount;
                *bgcolor = c;
            } else if ((amount == bgcolors[c].amount) && (c > *bgcolor)) {
                /* if we find another color that has been used exactly
                   as often, prefer the color with higher color index -
                   this will pick colors > 7 for background in this case */
                *bgcolor = c;
            }
        }
        if (*bgcolor != UNASSIGNED_COLOR) {
            bgcolors[*bgcolor].amount = 0;
        }
    }

   DBG(("selected hires bg: %d\n", *bgcolor));

    lib_free(dest->colormap);
    lib_free(dest);
}

/* find the best shared colors. for this we pick the colors that appear
   most often in blocks that contain (pass), or more, colors */
static void minipaint_find_shared_colors_pass(native_data_t *source,
    uint8_t *bgcolor, uint8_t *auxcolor, uint8_t *bordercolor, int pass, int auxbg)
{
    native_data_t *dest = lib_malloc(sizeof(native_data_t));
    native_color_sort_t *blockcolors = NULL;
    native_color_sort_t bgcolors[VIC_NUM_COLORS_BG_AUX];
    int i, j, k, l, c;
    uint8_t amount;

    /* color map for one block */
    dest->xsize = MINIPAINT_BLOCK_WIDTH;
    dest->ysize = MINIPAINT_BLOCK_HEIGHT;
    dest->colormap = lib_malloc(MINIPAINT_BLOCK_WIDTH * MINIPAINT_BLOCK_HEIGHT);

    for (c = 0; c < VIC_NUM_COLORS_BG_AUX; c++) {
        bgcolors[c].amount = 0;
        bgcolors[c].color = c;
    }

    for (i = 0; i < MINIPAINT_SCREEN_CHARS_HEIGHT; i++) {
        for (j = 0; j < MINIPAINT_SCREEN_CHARS_WIDTH; j++) {
            if (hiresmap[i][j] == 0) {
                /* get block */
                for (k = 0; k < MINIPAINT_BLOCK_HEIGHT; k++) {
                    for (l = 0; l < MINIPAINT_BLOCK_WIDTH; l++) {
                        dest->colormap[(k * MINIPAINT_BLOCK_WIDTH) + l] =
                        source->colormap[(i * MINIPAINT_BLOCK_HEIGHT * MINIPAINT_SCREEN_PIXEL_WIDTH) +
                                         (j * MINIPAINT_BLOCK_WIDTH) +
                                         (k * MINIPAINT_SCREEN_PIXEL_WIDTH) + l];
                    }
                }
                blockcolors = native_sort_colors_colormap(dest, VIC_NUM_COLORS_BG_AUX);
                if (blockcolors[pass - 1].amount != 0) {
                    /* (pass-1) or more colors in this block, add them to the pool of
                        colors we pick the background color from */
                    for (c = 0; c < VIC_NUM_COLORS_BG_AUX; c++) {
                        /* skip if the color was already assigned in a previous pass */
                        if (((*bordercolor != UNASSIGNED_COLOR) && (blockcolors[c].color == *bordercolor)) ||
                            ((*bgcolor != UNASSIGNED_COLOR) && (blockcolors[c].color == *bgcolor)) ||
                            ((*auxcolor != UNASSIGNED_COLOR) && (blockcolors[c].color == *auxcolor))){
                            continue;
                        }
                        /* if we are looking for an aux/bg color, skip if the color
                        index is less than 8 */
                        if (auxbg && (blockcolors[c].color < 8)) {
                            continue;
                        }
                        if (blockcolors[c].amount != 0) {
                            bgcolors[blockcolors[c].color].amount++;
                        }
                    }
                }
                lib_free(blockcolors);
            }
        }
    }
#ifdef DEBUGMINIPAINT
    for (c = 0; c < VIC_NUM_COLORS_BG_AUX; c++) {
        DBG(("%d col: %d amount :%d\n", c, bgcolors[c].color, bgcolors[c].amount));
    }
#endif
    /* pick the most used colors from the pool */
    if (*bgcolor == UNASSIGNED_COLOR) {
        amount = 0;
        for (c = 0; c < VIC_NUM_COLORS_BG_AUX; c++) {
            if (amount < bgcolors[c].amount) {
                amount = bgcolors[c].amount;
                *bgcolor = c;
            }
        }
        if (*bgcolor != UNASSIGNED_COLOR) {
            bgcolors[*bgcolor].amount = 0;
        }
    }

    if (*auxcolor == UNASSIGNED_COLOR) {
        amount = 0;
        for (c = 0; c < VIC_NUM_COLORS_BG_AUX; c++) {
            if (amount < bgcolors[c].amount) {
                amount = bgcolors[c].amount;
                *auxcolor = c;
            }
        }
        if (*auxcolor != UNASSIGNED_COLOR) {
            bgcolors[*auxcolor].amount = 0;
        }
    }

    if (*bordercolor == UNASSIGNED_COLOR) {
        amount = 0;
        for (c = 0; c < VIC_NUM_COLORS_BG_AUX; c++) {
            if (amount < bgcolors[c].amount) {
                amount = bgcolors[c].amount;
                *bordercolor = c;
            }
        }
        if (*bordercolor != UNASSIGNED_COLOR) {
            bgcolors[*bordercolor].amount = 0;
        }
    }

    DBG(("pass %d - border: %d bg: %d aux: %d\n", pass, *bordercolor, *bgcolor, *auxcolor));

    lib_free(dest->colormap);
    lib_free(dest);
}

static void minipaint_find_shared_colors(native_data_t *source, uint8_t *bgcolor, uint8_t *auxcolor, uint8_t *bordercolor)
{
    uint8_t color0, color1, color2;
    color0 = color1 = color2 = UNASSIGNED_COLOR;

    /* test for hires blocks and bg color */
    minipaint_find_hires_colors(source, &color0);
    /* test blocks for colors > 8 first to find bg- and aux color */
    minipaint_find_shared_colors_pass(source, &color0, &color1, &color2, 4, 1);
    minipaint_find_shared_colors_pass(source, &color0, &color1, &color2, 3, 1);
    minipaint_find_shared_colors_pass(source, &color0, &color1, &color2, 2, 1);
    minipaint_find_shared_colors_pass(source, &color0, &color1, &color2, 1, 1);
    /* now test all remaining colors */
    color2 = UNASSIGNED_COLOR;
    minipaint_find_shared_colors_pass(source, &color0, &color1, &color2, 4, 0);
    minipaint_find_shared_colors_pass(source, &color0, &color1, &color2, 3, 0);
    minipaint_find_shared_colors_pass(source, &color0, &color1, &color2, 2, 0);
    minipaint_find_shared_colors_pass(source, &color0, &color1, &color2, 1, 0);

    *bgcolor = color0 & 0x0f;
    *auxcolor = color1 & 0x0f;
    *bordercolor = color2 & 0x07;
}

/* fix/re-render the picture according to the multicolor restrictions,
   (three shared colors, plus one colors per block) or in hires */

static void minipaint_check_and_correct_cell(native_data_t *source, uint8_t bgcolor, uint8_t auxcolor, uint8_t bordercolor)
{
    native_data_t *dest = lib_malloc(sizeof(native_data_t));
    int i, j, k, l;
    native_color_sort_t *colors = NULL;
    native_color_sort_t cellcolors[VIC_NUM_COLORS_BG_AUX];

    dest->xsize = MINIPAINT_BLOCK_WIDTH;
    dest->ysize = MINIPAINT_BLOCK_HEIGHT;
    dest->colormap = lib_malloc(MINIPAINT_BLOCK_WIDTH * MINIPAINT_BLOCK_HEIGHT);

    for (i = 0; i < MINIPAINT_SCREEN_CHARS_HEIGHT; i++) {
        for (j = 0; j < MINIPAINT_SCREEN_CHARS_WIDTH; j++) {
            /* get block */
            for (k = 0; k < MINIPAINT_BLOCK_HEIGHT; k++) {
                for (l = 0; l < MINIPAINT_BLOCK_WIDTH; l++) {
                    dest->colormap[(k * MINIPAINT_BLOCK_WIDTH) + l] =
                        source->colormap[(i * MINIPAINT_BLOCK_HEIGHT * MINIPAINT_SCREEN_PIXEL_WIDTH) +
                                         (j * MINIPAINT_BLOCK_WIDTH) +
                                         (k * MINIPAINT_SCREEN_PIXEL_WIDTH) + l];
                }
            }
            colors = native_sort_colors_colormap(dest, VIC_NUM_COLORS_BG_AUX);

            if (hiresmap[i][j]) {
                /* the first color is expected to be the background color in the
                next step. so put it first, followed by the other colors */
                cellcolors[0].amount = MINIPAINT_SCREEN_PIXEL_WIDTH * MINIPAINT_SCREEN_PIXEL_HEIGHT;
                cellcolors[0].color = bgcolor;

                for (l = 0, k = 1; k < VIC_NUM_COLORS_BG_AUX; l++) {
                    if (colors[l].color != bgcolor) {
                        cellcolors[k].color = colors[l].color;
                        cellcolors[k].amount = colors[l].amount;
                        k++;
                    }
                }
                /* re-render the block using the background color and the most
                used color in that block */
                cellcolors[2].color = 255; /* mark end of list */
            } else {
                /* the first color is expected to be the background color in the
                next step. so put it first, followed by the other colors */
                cellcolors[0].amount = MINIPAINT_SCREEN_PIXEL_WIDTH * MINIPAINT_SCREEN_PIXEL_HEIGHT;
                cellcolors[0].color = bgcolor;
                cellcolors[1].amount = MINIPAINT_SCREEN_PIXEL_WIDTH * MINIPAINT_SCREEN_PIXEL_HEIGHT;
                cellcolors[1].color = auxcolor;
                cellcolors[2].amount = MINIPAINT_SCREEN_PIXEL_WIDTH * MINIPAINT_SCREEN_PIXEL_HEIGHT;
                cellcolors[2].color = bordercolor;

                for (l = 0, k = 3; k < VIC_NUM_COLORS_BG_AUX; l++) {
                    if ((colors[l].color != bordercolor) &&
                        (colors[l].color != bgcolor) &&
                        (colors[l].color != auxcolor)) {
                        cellcolors[k].color = colors[l].color;
                        cellcolors[k].amount = colors[l].amount;
                        k++;
                    }
                }
                /* re-render the block using the background color and the 3 most
                used color in that block */
                cellcolors[4].color = 255; /* mark end of list */
            }

            /* do the rendering */
            vic_color_to_nearest_vic_color_colormap(dest, cellcolors);
            for (k = 0; k < MINIPAINT_BLOCK_HEIGHT; k++) {
                for (l = 0; l < MINIPAINT_BLOCK_WIDTH; l++) {
                    source->colormap[(i * MINIPAINT_BLOCK_HEIGHT * MINIPAINT_SCREEN_PIXEL_WIDTH) +
                                        (j * MINIPAINT_BLOCK_WIDTH) +
                                        (k * MINIPAINT_SCREEN_PIXEL_WIDTH) + l] =
                        dest->colormap[(k * MINIPAINT_BLOCK_WIDTH) + l];
                }
            }
            lib_free(colors);
        }
    }
    lib_free(dest->colormap);
    lib_free(dest);
}

static int minipaint_render_and_save(native_data_t *source, int original_bordercolor)
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
    uint8_t colorbyte;
    uint8_t bgcolor;
    uint8_t gfxbits;
    uint8_t auxcolor;
    uint8_t bordercolor;

    /* allocate file buffer */
    filebuffer = lib_malloc(MINIPAINT_SIZE);

    /* clear filebuffer */
    memset(filebuffer, 0, MINIPAINT_SIZE);

    memset(hiresmap, 0, MINIPAINT_SCREEN_CHARS_HEIGHT * MINIPAINT_SCREEN_CHARS_WIDTH);
    containshires = 0;

    /* find best shared colors to use */
    minipaint_find_shared_colors(source, &bgcolor, &auxcolor, &bordercolor);

    /* when its safe to do so, use the original bordercolor. since we are trying
       a best effort generic conversion, we can't directly use the real border
       color and only use it as a hint. */
    DBG(("bordercolor: %d original bordercolor: %d\n", bordercolor, original_bordercolor));
    if (original_bordercolor != bordercolor) {
        if (auxcolor == original_bordercolor) {
            auxcolor = bordercolor;
            bordercolor = original_bordercolor;
            DBG(("auxcolor == original_bordercolor, aux: %d border: %d\n", auxcolor, bordercolor));
        } else if (bgcolor == original_bordercolor) {
            /* if the picture contains hires blocks, then we can not change the bgcolor */
            if (containshires == 0) {
                bgcolor = bordercolor;
                bordercolor = original_bordercolor;
                DBG(("bgcolor == original_bordercolor, bgcolor: %d border: %d\n", bgcolor, bordercolor));
            }
        }
    }
    DBG(("using bg: %d border: %d aux: %d\n", bgcolor, bordercolor, auxcolor));

    /* check and correct cells */
    minipaint_check_and_correct_cell(source, bgcolor, auxcolor, bordercolor);

    for (j = 0; j < MINIPAINT_SCREEN_CHARS_WIDTH; j++) {
        for (i = 0; i < MINIPAINT_SCREEN_CHARS_HEIGHT; i++) {
            /* one block */
            n = (i * MINIPAINT_SCREEN_CHARS_WIDTH) + j;
            color1 = 0;
            if (hiresmap[i][j]) {
                for (k = 0; k < MINIPAINT_BLOCK_HEIGHT; k++) {
                    gfxbits = 0;
                    for (l = 0; l < MINIPAINT_BLOCK_WIDTH; l++) {
                        gfxbits <<= 1;
                        colorbyte = source->colormap[(i * MINIPAINT_SCREEN_PIXEL_WIDTH * MINIPAINT_BLOCK_HEIGHT) +
                                                     (j * MINIPAINT_BLOCK_WIDTH) +
                                                     (k * MINIPAINT_SCREEN_PIXEL_WIDTH) + l];
                        /* assign the bits */
                        if (colorbyte != bgcolor) {
                            gfxbits |= 1;
                            color1 = colorbyte;
                        }
                    }
                    filebuffer[BITMAP_OFFSET + m] = gfxbits;
                    m++;
                }
            } else {
                for (k = 0; k < MINIPAINT_BLOCK_HEIGHT; k++) {
                    gfxbits = 0;
                    for (l = 0; l < (MINIPAINT_BLOCK_WIDTH / 2); l++) {
                        gfxbits <<= 2;
                        colorbyte = source->colormap[(i * MINIPAINT_SCREEN_PIXEL_WIDTH * MINIPAINT_BLOCK_HEIGHT) +
                                                     (j * MINIPAINT_BLOCK_WIDTH) +
                                                     (k * MINIPAINT_SCREEN_PIXEL_WIDTH) + (l * 2)];
                        /* assign the bits */
                        if (colorbyte == bordercolor) {
                            gfxbits |= 1;
                        } else if (colorbyte == auxcolor) {
                            gfxbits |= 3;
                        } else if (colorbyte != bgcolor) {
                            gfxbits |= 2;
                            color1 = colorbyte;
                        }
                    }
                    filebuffer[BITMAP_OFFSET + m] = gfxbits;
                    m++;
                }
            }
            if (n & 1) {
                filebuffer[VIDEORAM_OFFSET + (n >> 1)] |= ((color1 & 0x7) << 4) | (hiresmap[i][j] ? 0 : 0x80);
            } else {
                filebuffer[VIDEORAM_OFFSET + (n >> 1)] |= ((color1 & 0x7) << 0) | (hiresmap[i][j] ? 0 : 0x08);
            }
        }
    }

    memcpy(filebuffer, headerdata, 17);
    memcpy(&filebuffer[DISPLAYER_OFFSET], displaycode, 120);
    filebuffer[AUXCOLOR_OFFSET] = auxcolor << 4;
    filebuffer[BGCOLOR_OFFSET] = (bgcolor << 4) | bordercolor | (1 << 3);

    filename_ext = util_add_extension_const(source->filename, minipaint_drv.default_extension);

    fd = fopen(filename_ext, MODE_WRITE);
    if (fd == NULL) {
        retval = -1;
    }

    if (retval != -1) {
        if (fwrite(filebuffer, MINIPAINT_SIZE, 1, fd) < 1) {
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

static int minipaint_vicii_save(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    native_data_t *data = NULL;
    uint8_t bordercolor = regs[0x20] & 0xf;
    data = native_vicii_render(screenshot, filename);
    if (data == NULL) {
        return -1;
    }
    vicii_color_to_vic_color_colormap(data);
    if ((data->xsize != MINIPAINT_SCREEN_PIXEL_WIDTH) || (data->ysize != MINIPAINT_SCREEN_PIXEL_HEIGHT)) {
        data = native_resize_colormap(data, MINIPAINT_SCREEN_PIXEL_WIDTH, MINIPAINT_SCREEN_PIXEL_HEIGHT,
                                        bordercolor, oversize_handling, undersize_handling);
    }
    return minipaint_render_and_save(data, 0);
}

/* ------------------------------------------------------------------------ */

static int minipaint_ted_save(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    native_data_t *data = NULL;
    uint8_t bordercolor = regs[0x19];
    data = native_ted_render(screenshot, filename);
    if (data == NULL) {
        return -1;
    }
    ted_color_to_vic_color_colormap(data, ted_lum_handling);
    if ((data->xsize != MINIPAINT_SCREEN_PIXEL_WIDTH) || (data->ysize != MINIPAINT_SCREEN_PIXEL_HEIGHT)) {
        data = native_resize_colormap(data, MINIPAINT_SCREEN_PIXEL_WIDTH, MINIPAINT_SCREEN_PIXEL_HEIGHT,
                                        bordercolor, oversize_handling, undersize_handling);
    }
    return minipaint_render_and_save(data, 0);
}

/* ------------------------------------------------------------------------ */

static int minipaint_vic_save(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    native_data_t *data;
    uint8_t bordercolor = regs[0xf] & 7;

    DBG(("original VIC colors: background: %d border: %d aux: %d\n", regs[0xf] >> 4, regs[0xf] & 7, regs[0xe] >> 4));

    data = native_vic_render(screenshot, filename);
    if (data == NULL) {
        return -1;
    }

    if ((data->xsize != MINIPAINT_SCREEN_PIXEL_WIDTH) || (data->ysize != MINIPAINT_SCREEN_PIXEL_HEIGHT)) {
        data = native_resize_colormap(data, MINIPAINT_SCREEN_PIXEL_WIDTH, MINIPAINT_SCREEN_PIXEL_HEIGHT,
                                        bordercolor, oversize_handling, undersize_handling);
    }
    return minipaint_render_and_save(data, bordercolor);
}

/* ------------------------------------------------------------------------ */

static int minipaint_crtc_save(screenshot_t *screenshot, const char *filename)
{
    native_data_t *data = native_crtc_render(screenshot, filename);
    if (data == NULL) {
        return -1;
    }

    if (data->xsize != MINIPAINT_SCREEN_PIXEL_WIDTH || data->ysize != MINIPAINT_SCREEN_PIXEL_HEIGHT) {
        data = native_resize_colormap(data, MINIPAINT_SCREEN_PIXEL_WIDTH, MINIPAINT_SCREEN_PIXEL_HEIGHT,
                                        0, oversize_handling, undersize_handling);
    }
    return minipaint_render_and_save(data, 0);
}

/* ------------------------------------------------------------------------ */

static int minipaint_vdc_save(screenshot_t *screenshot, const char *filename)
{
    native_data_t *data = NULL;
    data = native_vdc_render(screenshot, filename);
    if (data == NULL) {
        return -1;
    }

    vdc_color_to_vic_color_colormap(data);
    if (data->xsize != MINIPAINT_SCREEN_PIXEL_WIDTH || data->ysize != MINIPAINT_SCREEN_PIXEL_HEIGHT) {
        data = native_resize_colormap(data, MINIPAINT_SCREEN_PIXEL_WIDTH, MINIPAINT_SCREEN_PIXEL_HEIGHT,
                                        0, oversize_handling, undersize_handling);
    }
    return minipaint_render_and_save(data, 0);
}

/* ------------------------------------------------------------------------ */

static int minipaintdrv_save(screenshot_t *screenshot, const char *filename)
{
    if (!(strcmp(screenshot->chipid, "VICII"))) {
        return minipaint_vicii_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "VDC"))) {
        return minipaint_vdc_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "CRTC"))) {
        return minipaint_crtc_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "TED"))) {
        return minipaint_ted_save(screenshot, filename);
    }
    if (!(strcmp(screenshot->chipid, "VIC"))) {
        return minipaint_vic_save(screenshot, filename);
    }
    ui_error("Unknown graphics chip");
    return -1;
}

static gfxoutputdrv_t minipaint_drv =
{
    GFXOUTPUTDRV_TYPE_SCREENSHOT_NATIVE,
    "MINIPAINT",
    "MiniPaint screenshot",
    "prg",
    NULL, /* formatlist */
    NULL,
    NULL,
    NULL,
    NULL,
    minipaintdrv_save,
    NULL,
    NULL,
    minipaintdrv_resources_init,
    minipaintdrv_cmdline_options_init
#ifdef FEATURE_CPUMEMHISTORY
    , NULL
#endif
};

void gfxoutput_init_minipaint(int help)
{
    gfxoutput_register(&minipaint_drv);
}
