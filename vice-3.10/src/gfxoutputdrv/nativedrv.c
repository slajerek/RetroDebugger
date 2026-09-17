/*
 * nativedrv.c - Common code for native screenshots.
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

/* #define DEBUGNATIVEDRV */

#ifdef  DEBUGNATIVEDRV
#define DBG(_x_)    printf _x_
#else
#define DBG(_x_)
#endif

#define VIC_NUM_COLORS      16
#define VICII_NUM_COLORS    16

/*
    TODO:

    disable debug output
    test vic screenshots, fix automatic positioning of the gfx window
    test vdc screenshots, fix automatic positioning of the gfx window
    test crts screenshots, fix automatic positioning of the gfx window
    when all is done, remove #if 0'ed code
*/

#if 0
void native_smooth_scroll_borderize_colormap(native_data_t *source, uint8_t bordercolor, uint8_t xcover, uint8_t ycover)
{
    int i, j, k;
    int xstart = 0;
    int xsize;
    int xendamount = 0;
    int ystart = 0;
    int ysize;
    int yendamount = 0;

    DBG(("native_smooth_scroll_borderize_colormap bordercolor: %d xcover: %d ycover: %d\n",
         bordercolor, xcover, ycover));

    if (xcover == 255) {
        xstart = 0;
        xsize = source->xsize;
        xendamount = 0;
    } else {
        xstart = 7 - xcover;
        xsize = source->xsize - 16;
        xendamount = 16 - xstart;
    }

    if (ycover == 255) {
        ystart = 0;
        ysize = source->ysize;
        yendamount = 0;
    } else {
        ystart = 7 - ycover;
        ysize = source->ysize - 8;
        yendamount = 8 - ystart;
    }

    k = 0;

    /* render top border if needed */
    for (i = 0; i < ystart; i++) {
        for (j = 0; j < source->xsize; j++) {
            source->colormap[k++] = bordercolor;
        }
    }

    for (i = 0; i < ysize; i++) {
        /* render left border if needed */
        for (j = 0; j < xstart; j++) {
            source->colormap[k++] = bordercolor;
        }

        /* skip screen data */
        k += xsize;

        /* render right border if needed */
        for (j = 0; j < xendamount; j++) {
            source->colormap[k++] = bordercolor;
        }
    }

    /* render bottom border if needed */
    for (i = 0; i < yendamount; i++) {
        for (j = 0; j < source->xsize; j++) {
            source->colormap[k++] = bordercolor;
        }
    }
}
#endif

native_data_t *native_borderize_colormap(native_data_t *source, uint8_t bordercolor, int xsize, int ysize)
{
    int i, j, k, l;
    int xstart = 0;
    int xendamount = 0;
    int ystart = 0;
    int yendamount = 0;
    native_data_t *dest = lib_malloc(sizeof(native_data_t));

    DBG(("native_borderize_colormap bordercolor: %d xsize: %d ysize: %d\n",
         bordercolor, xsize, ysize));

    dest->filename = source->filename;

    if (source->xsize < xsize) {
        dest->xsize = xsize;
        xstart = ((xsize - source->xsize) / 16) * 8;
        xendamount = xsize - xstart - source->xsize;
    } else {
        dest->xsize = source->xsize;
    }

    if (source->ysize < ysize) {
        dest->ysize = ysize;
        ystart = ((ysize - source->ysize) / 16) * 8;
        yendamount = ysize - ystart - source->ysize;
    } else {
        dest->ysize = source->ysize;
    }

    dest->colormap = lib_malloc(dest->xsize * dest->ysize);

    k = 0;
    l = 0;

    /* render top border if needed */
    for (i = 0; i < ystart; i++) {
        for (j = 0; j < dest->xsize; j++) {
            dest->colormap[k++] = bordercolor;
        }
    }

    for (i = 0; i < source->ysize; i++) {
        /* render left border if needed */
        for (j = 0; j < xstart; j++) {
            dest->colormap[k++] = bordercolor;
        }

        /* copy screen data */
        for (j = 0; j < source->xsize; j++) {
            dest->colormap[k++] = source->colormap[l++];
        }

        /* render right border if needed */
        for (j = 0; j < xendamount; j++) {
            dest->colormap[k++] = bordercolor;
        }
    }

    /* render bottom border if needed */
    for (i = 0; i < yendamount; i++) {
        for (j = 0; j < dest->xsize; j++) {
            dest->colormap[k++] = bordercolor;
        }
    }

    lib_free(source->colormap);
    lib_free(source);

    return dest;
}

native_data_t *native_crop_and_borderize_colormap(native_data_t *source, uint8_t bordercolor, int xsize, int ysize, int oversize_handling)
{
    int startx;
    int starty;
    int skipxstart = 0;
    int skipxend = 0;
    int skipystart = 0;
    int i, j, k, l;
    native_data_t *dest = lib_malloc(sizeof(native_data_t));

    DBG(("native_crop_and_borderize_colormap bordercolor: %d xsize: %d ysize: %d oversize handling: %d\n",
         bordercolor, xsize, ysize, oversize_handling));

    dest->filename = source->filename;

    startx = (xsize - source->xsize) / 2;
    starty = (ysize - source->ysize) / 2;

    if (source->xsize > xsize) {
        dest->xsize = xsize;
    } else {
        dest->xsize = source->xsize;
    }

    if (source->ysize > ysize) {
        dest->ysize = ysize;
    } else {
        dest->ysize = source->ysize;
    }

    dest->colormap = lib_malloc(dest->xsize * dest->ysize);

    if (startx < 0) {
        switch (oversize_handling) {
            default:
            case NATIVE_SS_OVERSIZE_CROP_LEFT_TOP:
            case NATIVE_SS_OVERSIZE_CROP_LEFT_CENTER:
            case NATIVE_SS_OVERSIZE_CROP_LEFT_BOTTOM:
                skipxend = source->xsize - xsize;
                break;
            case NATIVE_SS_OVERSIZE_CROP_CENTER_TOP:
            case NATIVE_SS_OVERSIZE_CROP_CENTER:
            case NATIVE_SS_OVERSIZE_CROP_CENTER_BOTTOM:
                skipxstart = 0 - startx;
                skipxend = source->xsize - xsize - skipxstart;
                break;
            case NATIVE_SS_OVERSIZE_CROP_RIGHT_TOP:
            case NATIVE_SS_OVERSIZE_CROP_RIGHT_CENTER:
            case NATIVE_SS_OVERSIZE_CROP_RIGHT_BOTTOM:
                skipxstart = source->xsize - xsize;
                break;
        }
        startx = 0;
    } else {
        startx = ((xsize - source->xsize) / 16) * 8;
    }

    if (starty < 0) {
        switch (oversize_handling) {
            default:
            case NATIVE_SS_OVERSIZE_CROP_LEFT_TOP:
            case NATIVE_SS_OVERSIZE_CROP_CENTER_TOP:
            case NATIVE_SS_OVERSIZE_CROP_RIGHT_TOP:
                break;
            case NATIVE_SS_OVERSIZE_CROP_LEFT_CENTER:
            case NATIVE_SS_OVERSIZE_CROP_CENTER:
            case NATIVE_SS_OVERSIZE_CROP_RIGHT_CENTER:
                skipystart = 0 - starty;
                break;
            case NATIVE_SS_OVERSIZE_CROP_LEFT_BOTTOM:
            case NATIVE_SS_OVERSIZE_CROP_CENTER_BOTTOM:
            case NATIVE_SS_OVERSIZE_CROP_RIGHT_BOTTOM:
                skipystart = source->ysize - ysize;
                break;
        }
        starty = 0;
    } else {
        starty = ((ysize - source->ysize) / 16) * 8;
    }

    k = 0;
    l = 0;

    /* skip top lines for cropping if needed */
    for (i = 0; i < skipystart; i++) {
        for (j = 0; j < source->ysize; j++) {
            l++;
        }
    }

    /* render top border if needed */
    for (i = 0; i < starty; i++) {
        for (j = 0; j < xsize; j++) {
            dest->colormap[k++] = bordercolor;
        }
    }

    for (i = starty; i < starty + dest->ysize; i++) {
        /* skip right part for cropping if needed */
        for (j = 0; j < skipxstart; j++) {
            l++;
        }

        /* render left border if needed */
        for (j = 0; j < startx; j++) {
            dest->colormap[k++] = bordercolor;
        }

        /* copy main body */
        for (j = startx; j < startx + dest->xsize; j++) {
            dest->colormap[k++] = source->colormap[l++];
        }

        /* render right border if needed */
        for (j = startx + dest->xsize; j < xsize; j++) {
            dest->colormap[k++] = bordercolor;
        }

        /* skip right part for cropping if needed */
        for (j = 0; j < skipxend; j++) {
            l++;
        }
    }

    /* render bottom border if needed */
    for (i = starty + dest->ysize; i < ysize; i++) {
        for (j = 0; j < xsize; j++) {
            dest->colormap[k++] = bordercolor;
        }
    }

    lib_free(source->colormap);
    lib_free(source);

    return dest;
}

native_data_t *native_scale_colormap(native_data_t *source, int xsize, int ysize)
{
    native_data_t *dest = lib_malloc(sizeof(native_data_t));
    int i, j;
    int xmult, ymult;

    DBG(("native_scale_colormap xsize: %d ysize: %d\n", xsize, ysize));
    DBG(("               source xsize: %d ysize: %d\n", source->xsize, source->ysize));

    dest->filename = source->filename;

    dest->xsize = xsize;
    dest->ysize = ysize;

    dest->colormap = lib_malloc(xsize * ysize);

    xmult = (source->xsize << 8) / xsize;
    ymult = (source->ysize << 8) / ysize;

    for (i = 0; i < ysize; i++) {
        for (j = 0; j < xsize; j++) {
            dest->colormap[(i * xsize) + j] =
                source->colormap[(((i * ymult) >> 8) * source->xsize) + ((j * xmult) >> 8)];
        }
    }

    lib_free(source->colormap);
    lib_free(source);

    return dest;
}

/* scale and/or crop and/or borderize according to the options */
native_data_t *native_resize_colormap(native_data_t *source, int xsize, int ysize, uint8_t bordercolor, int oversize_handling, int undersize_handling)
{
    native_data_t *data = source;
    int mc_data_present = source->mc_data_present;

    DBG(("native_crop_and_borderize_colormap bordercolor: %d xsize: %d ysize: %d oversize handling: %d undersize handling: %d\n",
         bordercolor, xsize, ysize, oversize_handling, undersize_handling));

    if (data->xsize > xsize) {
        if (oversize_handling == NATIVE_SS_OVERSIZE_SCALE) {
            data = native_scale_colormap(data, xsize, data->ysize);
        } else {
            data = native_crop_and_borderize_colormap(data, bordercolor, xsize, data->ysize, oversize_handling);
        }
    }

    if (data->xsize < xsize) {
        if (undersize_handling == NATIVE_SS_UNDERSIZE_SCALE) {
            data = native_scale_colormap(data, xsize, data->ysize);
        } else {
            data = native_borderize_colormap(data, bordercolor, xsize, data->ysize);
        }
    }

    if (data->ysize > ysize) {
        if (oversize_handling == NATIVE_SS_OVERSIZE_SCALE) {
            data = native_scale_colormap(data, xsize, ysize);
        } else {
            data = native_crop_and_borderize_colormap(data, bordercolor, xsize, ysize, oversize_handling);
        }
    }

    if (data->ysize < ysize) {
        if (undersize_handling == NATIVE_SS_UNDERSIZE_SCALE) {
            data = native_scale_colormap(data, xsize, ysize);
        } else {
            data = native_borderize_colormap(data, bordercolor, xsize, ysize);
        }
    }

    data->mc_data_present = mc_data_present;

    return data;
}

native_color_sort_t *native_sort_colors_colormap(native_data_t *source, int color_amount)
{
    int i, j;
    uint8_t color;
    int highest;
    int amount;
    int highestindex = 0;
    native_color_sort_t *colors = lib_malloc(sizeof(native_color_sort_t) * color_amount);

    for (i = 0; i < color_amount; i++) {
        colors[i].color = i;
        colors[i].amount = 0;
    }

    /* count the colors used */
    for (i = 0; i < (source->xsize * source->ysize); i++) {
        colors[source->colormap[i]].amount++;
    }

    /* sort colors from highest to lowest */
    for (i = 0; i < color_amount; i++) {
        highest = 0;
        for (j = i; j < color_amount; j++) {
            if (colors[j].amount >= highest) {
                highest = colors[j].amount;
                highestindex = j;
            }
        }
        color = colors[i].color;
        amount = colors[i].amount;
        colors[i].color = colors[highestindex].color;
        colors[i].amount = colors[highestindex].amount;
        colors[highestindex].color = color;
        colors[highestindex].amount = amount;
    }
    return colors;
}

/* returns 1 if any 8x8 cell contains more than two colors */
int native_is_colormap_multicolor(native_data_t *source)
{
    int blocksx = source->xsize / 8;
    int blocksy = source->ysize / 8;
    int i, j, k, l;
    int multicolor = 0;
    native_data_t *dest = lib_malloc(sizeof(native_data_t));
    native_color_sort_t *colors = NULL;

    dest->xsize = 8;
    dest->ysize = 8;
    dest->colormap = lib_malloc(8 * 8);

    for (i = 0; (i < blocksy) && (multicolor == 0); i++) {
        for (j = 0; (j < blocksx) && (multicolor == 0); j++) {
            /* get block */
            for (k = 0; k < 8; k++) {
                for (l = 0; l < 8; l++) {
                    dest->colormap[(k * 8) + l] =
                    source->colormap[(i * 8 * source->xsize) + (j * 8)
                                        + (k * source->xsize) + l];
                }
            }
            colors = native_sort_colors_colormap(dest, 16);
            if (colors[2].amount > 0) {
                multicolor = 1;
            }
            lib_free(colors);
        }
    }
    lib_free(dest->colormap);
    lib_free(dest);
    return multicolor;
}

static uint8_t vicii_color_bw_translate[VICII_NUM_COLORS] = {
    0,    /* vicii black       (0) -> vicii black (0) */
    1,    /* vicii white       (1) -> vicii white (1) */
    0,    /* vicii red         (2) -> vicii black (0) */
    1,    /* vicii cyan        (3) -> vicii white (1) */
    1,    /* vicii purple      (4) -> vicii white (1) */
    0,    /* vicii green       (5) -> vicii black (0) */
    0,    /* vicii blue        (6) -> vicii black (0) */
    1,    /* vicii yellow      (7) -> vicii white (1) */
    0,    /* vicii orange      (8) -> vicii black (0) */
    0,    /* vicii brown       (9) -> vicii black (0) */
    1,    /* vicii light red   (A) -> vicii white (1) */
    0,    /* vicii dark gray   (B) -> vicii black (0) */
    1,    /* vicii medium gray (C) -> vicii white (1) */
    1,    /* vicii light green (D) -> vicii white (1) */
    1,    /* vicii light blue  (E) -> vicii white (1) */
    1     /* vicii light gray  (F) -> vicii white (1) */
};

static inline uint8_t vicii_color_to_bw(uint8_t color)
{
    return vicii_color_bw_translate[color];
}

void vicii_color_to_vicii_bw_colormap(native_data_t *source)
{
    int i, j;

    for (i = 0; i < 200; i++) {
        for (j = 0; j < 320; j++) {
            source->colormap[(i * 320) + j] = vicii_color_to_bw(source->colormap[(i * 320) + j]);
        }
    }
}

static uint8_t vicii_color_gray_translate[VICII_NUM_COLORS] = {
    0x0,    /* vicii black       (0) -> vicii black       (0) */
    0xF,    /* vicii white       (1) -> vicii light gray  (F) */
    0xB,    /* vicii red         (2) -> vicii dark gray   (B) */
    0xC,    /* vicii cyan        (3) -> vicii medium gray (C) */
    0xC,    /* vicii purple      (4) -> vicii medium gray (C) */
    0xB,    /* vicii green       (5) -> vicii dark gray   (B) */
    0xB,    /* vicii blue        (6) -> vicii dark gray   (B) */
    0xC,    /* vicii yellow      (7) -> vicii medium gray (C) */
    0xC,    /* vicii orange      (8) -> vicii medium gray (C) */
    0xB,    /* vicii brown       (9) -> vicii dark gray   (B) */
    0xC,    /* vicii light red   (A) -> vicii medium gray (C) */
    0xB,    /* vicii dark gray   (B) -> vicii dark gray   (B) */
    0xC,    /* vicii medium gray (C) -> vicii medium gray (C) */
    0xF,    /* vicii light green (D) -> vicii light gray  (F) */
    0xC,    /* vicii light blue  (E) -> vicii medium gray (C) */
    0xF     /* vicii light gray  (F) -> vicii light gray  (F) */
};

static inline uint8_t vicii_color_to_gray(uint8_t color)
{
    return vicii_color_gray_translate[color];
}

void vicii_color_to_vicii_gray_colormap(native_data_t *source)
{
    int i, j;

    for (i = 0; i < 200; i++) {
        for (j = 0; j < 320; j++) {
            source->colormap[(i * 320) + j] = vicii_color_to_gray(source->colormap[(i * 320) + j]);
        }
    }
}

/* most similar -> least similar color */
static uint8_t vicii_closest_color[VICII_NUM_COLORS][VICII_NUM_COLORS] = {
    /* vicii black (0) */
    { 0, 9, 11, 2, 6, 8, 5, 12, 4, 10, 14, 3, 13, 15, 7, 1 },
    /* vicii white (1) */
    { 1, 15, 13, 7, 3, 10, 14, 12, 4, 5, 11, 8, 6, 2, 9, 0 },
    /* vicii red (2) */
    { 2, 8, 9, 11, 0, 10, 12, 5, 4, 6, 7, 14, 15, 3, 13, 1 },
    /* vicii cyan (3) */
    { 3, 13, 14, 15, 12, 10, 5, 7, 4, 11, 1, 6, 8, 9, 2, 0 },
    /* vicii purple (4) */
    { 4, 10, 12, 11, 15, 14, 6, 8, 2, 3, 13, 9, 7, 5, 1, 0 },
    /* vicii green (5) */
    { 5, 11, 12, 8, 9, 3, 10, 2, 13, 7, 14, 15, 0, 4, 6, 1 },
    /* vicii blue (6) */
    { 6, 11, 9, 0, 4, 12, 14, 2, 8, 10, 3, 5, 13, 15, 7, 1 },
    /* vicii yellow (7) */
    { 7, 13, 15, 10, 3, 12, 1, 5, 8, 4, 14, 11, 2, 9, 6, 0 },
    /* vicii orange (8) */
    { 8, 2, 9, 11, 10, 5, 12, 4, 0, 7, 6, 15, 3, 14, 13, 1 },
    /* vicii brown (9) */
    { 9, 11, 2, 0, 8, 6, 5, 12, 4, 10, 14, 3, 15, 13, 7, 1 },
    /* vicii light red (10) */
    { 10, 12, 4, 15, 7, 8, 3, 11, 13, 14, 2, 5, 9, 1, 6, 0 },
    /* vicii dark gray (11) */
    { 11, 9, 12, 6, 2, 8, 5, 0, 4, 10, 14, 3, 15, 13, 7, 1 },
    /* vicii medium gray (12) */
    { 12, 10, 4, 3, 14, 11, 15, 5, 13, 8, 9, 6, 7, 2, 1, 0 },
    /* vicii light green (13) */
    { 13, 3, 15, 7, 12, 15, 1, 10, 5, 4, 11, 8, 9, 2, 6, 0 },
    /* vicii light blue (14) */
    { 14, 3, 12, 11, 4, 13, 6, 11, 10, 5, 9, 1, 7, 8, 2, 0 },
    /* vicii light gray (15) */
    { 15, 13, 3, 12, 14, 10, 7, 1, 4, 5, 11, 8, 6, 9, 2, 0 }
};
/*
    0 - black         8 - orange
    1 - white         9 - light orange  (brown)
    2 - red          10 - pink
    3 - cyan         11 - light cyan    (dark gray)
    4 - magenta      12 - light magenta (medium gray)
    5 - green        13 - light green
    6 - blue         14 - light blue
    7 - yellow       15 - light yellow  (light gray)

    FIXME: this table needs tweaking/fixing
*/
/* most similar -> least similar color */
static uint8_t vic_closest_color[VIC_NUM_COLORS][VIC_NUM_COLORS] = {
    /* vicii black (0) */
    { 0, 11, 2, 6, 8, 5, 12, 4, 10, 9, 14, 3, 13, 15, 7, 1 },
    /* vicii white (1) */
    { 1, 15, 9, 13, 7, 3, 10, 14, 12, 4, 5, 11, 8, 6, 2, 0 },
    /* vicii red (2) */
    { 2, 8, 9, 11, 0, 10, 12, 5, 4, 6, 7, 14, 15, 3, 13, 1 },
    /* vicii cyan (3) */
    { 3, 13, 14, 15, 12, 10, 5, 7, 4, 11, 1, 6, 8, 9, 2, 0 },
    /* vicii purple (4) */
    { 4, 10, 12, 11, 15, 14, 6, 8, 2, 3, 13, 9, 7, 5, 1, 0 },
    /* vicii green (5) */
    { 5, 11, 12, 8, 9, 3, 10, 2, 13, 7, 14, 15, 0, 4, 6, 1 },
    /* vicii blue (6) */
    { 6, 11, 9, 0, 4, 12, 14, 2, 8, 10, 3, 5, 13, 15, 7, 1 },
    /* vicii yellow (7) */
    { 7, 13, 15, 10, 3, 12, 1, 5, 8, 4, 14, 11, 2, 9, 6, 0 },
    /* vicii orange (8) */
    { 8, 2, 9, 11, 10, 5, 12, 4, 0, 7, 6, 15, 3, 14, 13, 1 },
    /* vicii brown (9) */
    { 9, 11, 2, 0, 8, 6, 5, 12, 4, 10, 14, 3, 15, 13, 7, 1 },
    /* vicii light red (10) */
    { 10, 12, 4, 15, 7, 8, 3, 11, 13, 14, 2, 5, 9, 1, 6, 0 },
    /* vicii dark gray (11) */
    { 11, 9, 12, 6, 2, 8, 5, 0, 4, 10, 14, 3, 15, 13, 7, 1 },
    /* vicii medium gray (12) */
    { 12, 10, 4, 3, 14, 11, 15, 5, 13, 8, 9, 6, 7, 2, 1, 0 },
    /* vicii light green (13) */
    { 13, 3, 15, 7, 12, 15, 1, 10, 5, 4, 11, 8, 9, 2, 6, 0 },
    /* vicii light blue (14) */
    { 14, 3, 12, 11, 4, 13, 6, 11, 10, 5, 9, 1, 7, 8, 2, 0 },
    /* vicii light yellow (15) */
    { 15, 13, 3, 7, 1, 12, 14, 10, 4, 5, 11, 8, 6, 9, 2, 0 }
};

/* altcolors[n].color == 255 marks the end of the list */
static inline uint8_t vicii_color_to_nearest_color(uint8_t color, native_color_sort_t *altcolors)
{
    int i, j;

    for (i = 0; i < VICII_NUM_COLORS; i++) {
        for (j = 0; altcolors[j].color != 255; j++) {
            if (vicii_closest_color[color][i] == altcolors[j].color) {
                return vicii_closest_color[color][i];
            }
        }
    }
    return 0;
}

void vicii_color_to_nearest_vicii_color_colormap(native_data_t *source, native_color_sort_t *colors)
{
    int i, j;

    for (i = 0; i < source->ysize; i++) {
        for (j = 0; j < source->xsize; j++) {
            source->colormap[(i * source->xsize) + j] =
                vicii_color_to_nearest_color(source->colormap[(i * source->xsize) + j], colors);
        }
    }
}

/* altcolors[n].color == 255 marks the end of the list */
static inline uint8_t vic_color_to_nearest_color(uint8_t color, native_color_sort_t *altcolors)
{
    int i, j;

    for (i = 0; i < VIC_NUM_COLORS; i++) {
        for (j = 0; altcolors[j].color != 255; j++) {
            if (vic_closest_color[color][i] == altcolors[j].color) {
                return vic_closest_color[color][i];
            }
        }
    }
    return 0;
}

void vic_color_to_nearest_vic_color_colormap(native_data_t *source, native_color_sort_t *colors)
{
    int i, j;

    for (i = 0; i < source->ysize; i++) {
        for (j = 0; j < source->xsize; j++) {
            source->colormap[(i * source->xsize) + j] =
                vic_color_to_nearest_color(source->colormap[(i * source->xsize) + j], colors);
        }
    }
}

/* ------------------------------------------------------------------------ */

static native_data_t *native_generic_render(screenshot_t *screenshot, const char *filename, int xsize, int ysize, int xstep)
{
    native_data_t *data = lib_malloc(sizeof(native_data_t));
    int i, j;
    int leftborder, topborder;
    uint8_t *linebuffer;

    DBG(("native_generic_render xsize: %d ysize: %d xstep: %d\n", xsize, ysize, xstep));
    data->filename = filename;

    /* size of the native picture */
    data->xsize = xsize;
    data->ysize = ysize;
    data->colormap = lib_malloc(data->xsize * data->ysize);

    linebuffer = lib_malloc(screenshot->width * screenshot->height);

    DBG(("screenshot->width: %u\n", screenshot->width));
    DBG(("screenshot->height: %u\n", screenshot->height));
    DBG(("screenshot->max_width: %u\n", screenshot->max_width));
    DBG(("screenshot->x_offset: %u\n", screenshot->x_offset));
    DBG(("screenshot->first_displayed_line: %u\n", screenshot->first_displayed_line));
    DBG(("screenshot->gfx_position.x %u\n", screenshot->gfx_position.x));
    DBG(("screenshot->gfx_position.y %u\n", screenshot->gfx_position.y));

    leftborder = screenshot->gfx_position.x;
    topborder = screenshot->gfx_position.y - screenshot->first_displayed_line;

    DBG(("                      leftborder: %d\n", leftborder));
    DBG(("                      topborder: %d\n", topborder));

    /* get screenshot data in palette format */
    for (i = 0; i < data->ysize; i++) {
        (screenshot->convert_line)(screenshot,
                                   linebuffer + (i * screenshot->width),
                                   i + topborder, SCREENSHOT_MODE_PALETTE);
    }

    /* create picture in the native size of the videochip, without border */
    for (i = 0; i < data->ysize; i++) {
        for (j = 0; j < data->xsize; j++) {
            data->colormap[(i * data->xsize) + (j)] =
            linebuffer[(i * screenshot->width) + ((j * xstep) + leftborder)];
        }
    }
    data->mc_data_present = native_is_colormap_multicolor(data);
    DBG(("                      mc_data_present: %d\n", data->mc_data_present));

    return data;
}

native_data_t *native_vicii_render(screenshot_t *screenshot, const char *filename)
{
    native_data_t *data = native_generic_render(screenshot, filename, 320, 200, 1);
#if 0
    uint8_t *regs = screenshot->video_regs;
    if (((regs[0x16] & 8) == 0) || ((regs[0x11] & 8) == 0)) {
        native_smooth_scroll_borderize_colormap(data, (uint8_t)(regs[0x20] & 0xf), (uint8_t)((regs[0x16] & 8) ? 255 : regs[0x16] & 7), (uint8_t)((regs[0x11] & 8) ? 255 : regs[0x11] & 7));
    }
#endif
    return data;
}

native_data_t *native_ted_render(screenshot_t *screenshot, const char *filename)
{
    native_data_t *data = native_generic_render(screenshot, filename, 320, 200, 1);
#if 0
    uint8_t *regs = screenshot->video_regs;
    if (((regs[0x07] & 8) == 0) || ((regs[0x06] & 8) == 0)) {
        native_smooth_scroll_borderize_colormap(data, brdrcolor, (uint8_t)((regs[0x07] & 8) ? 255 : regs[0x07] & 7), (uint8_t)((regs[0x06] & 8) ? 255 : regs[0x06] & 7));
    }
#endif
    return data;
}

native_data_t *native_vic_render(screenshot_t *screenshot, const char *filename)
{
    native_data_t *data;
    uint8_t *regs = screenshot->video_regs;
    int xsize, ysize;
    xsize = (regs[0x02] & 0x7f) * 8; /* columns */
    ysize = ((regs[0x03] & 0x7e) >> 1) * 8; /* rows */
    if (regs[0x03] & 1) {
        ysize <<= 1; /* 16 pixel high chars */
    }
    DBG(("native_vic_render columns: %d rows: %d char height: %d\n",
         regs[0x02] & 0x7f, (regs[0x03] & 0x7e) >> 1, (regs[0x03] & 1) ? 16 : 8));
    data = native_generic_render(screenshot, filename, xsize, ysize, 2);
    return data;
}

/* FIXME:
   screenshot->gfx_position.x/y does not match actual black border

   "hre" and "petdww_ram" is broken
*/
native_data_t *native_crtc_render(screenshot_t *screenshot, const char *filename)
{
    native_data_t *data;
    uint8_t *regs = screenshot->video_regs;
    uint8_t *petdww_ram = screenshot->bitmap_ptr;
    int xsize;
    int ysize;
    uint8_t invert;
    uint8_t charheight, charwidth;
    int hre = 0;
#if 0
    int base;
    int chars = 1;
    int col80;
#endif

    DBG(("screenshot->bitmap_low_ptr[0]: %d\n", screenshot->bitmap_low_ptr[0]));
    DBG(("screenshot->bitmap_high_ptr[0]: %d\n", screenshot->bitmap_high_ptr[0]));
    DBG(("1: %d\n", regs[0x01]));
    DBG(("2: %d\n", regs[0x02]));
    DBG(("3: %d\n", regs[0x03]));
    DBG(("4: %d\n", regs[0x04]));
    DBG(("5: %d\n", regs[0x05]));
    DBG(("6: %d\n", regs[0x06]));
    DBG(("7: %d\n", regs[0x07]));
    DBG(("8: %d\n", regs[0x08]));
    DBG(("9: %d\n", regs[0x09]));

    xsize = regs[0x01];
    if (screenshot->bitmap_low_ptr[0] == 80) {
        xsize <<= 1;
    }

    ysize = regs[0x06];
    invert = (regs[0x0c] & 0x10) >> 4;

    if (!invert) {      /* On 8296 only! */
        hre = 1;
#if 0
        chars = 0;
#endif
        invert = 1;
    }

    /* charheight = screenshot->bitmap_high_ptr[0]; */
    charwidth = 8; /* is this correct? */
    charheight = regs[0x09] + 1; /* FIXME: handle inter-character gap */

    DBG(("chars x: %d w: %d\n", xsize, charwidth));
    DBG(("chars y: %d h: %d\n", ysize, charheight));

    if (hre) {
        xsize = 512;
        ysize = 256;
    } else {
        xsize = xsize * charwidth;
        ysize = ysize * charheight;
    }

    if (petdww_ram) {
        /* FIXME:
        * If we're in an 80 column screen we need to
        * horizontally double all pixels, since DWW
        * only has 40 characters worth of pixels.
        */
    }
    DBG(("xsize: %d\n", xsize));
    DBG(("ysize: %d\n", ysize));

    /* FIXME: this should be defined elsewhere in the crtc code */
    if (regs[0x01] == 40) {
        screenshot->gfx_position.y = 41;
    } else {
        screenshot->gfx_position.y = 28;
    }
    screenshot->gfx_position.x = 33;

    data = native_generic_render(screenshot, filename, xsize, ysize, 1);
    return data;
}

/* FIXME:
   screenshot->gfx_position.x/y does not match actual black border
*/
native_data_t *native_vdc_render(screenshot_t *screenshot, const char *filename)
{
    native_data_t *data;
    uint8_t *regs = screenshot->video_regs;
    int xsize, ysize;
    uint8_t displayed_chars_h = regs[1];
    uint8_t displayed_chars_v = regs[6];
    uint8_t scanlines_per_char = (regs[9] & 0x1f) + 1;
    /* uint8_t char_h_size_alloc = regs[22] & 0xf; */
    /* uint8_t char_h_size_displayed = (regs[22] & 0xf0) >> 4; */
    uint8_t char_h_size = (regs[22] & 0x0f);
    uint8_t char_h_double_pixel = ((regs[25] & 0x10) ? 2 : 1);
    uint8_t top_border_chars_h;

    DBG(("native_vdc_render displayed_chars_h: %u\n", displayed_chars_h));
    DBG(("                  displayed_chars_v: %u\n", displayed_chars_v));
    DBG(("                  char_h_size_displayed: %u\n", (regs[22] & 0xf0) >> 4));
    DBG(("                  char_h_size: %u\n", char_h_size));
    DBG(("                  scanlines_per_char: %u\n", scanlines_per_char));
    DBG(("                  char_h_double_pixel: %u\n", char_h_double_pixel));
    DBG(("                  3: %u\n", regs[3]));
    DBG(("                  4: %u\n", regs[4]));
    DBG(("                  5: %u\n", regs[5]));
    DBG(("                  6: %u\n", regs[6]));

    xsize = displayed_chars_h * char_h_size * char_h_double_pixel;
    ysize = displayed_chars_v * scanlines_per_char;

    DBG(("                  xsize: %d\n", xsize));
    DBG(("                  ysize: %d\n", ysize));

    /* FIXME: this should be set elsewhere in the vdc code */
    top_border_chars_h = ((regs[4] - displayed_chars_v) / 2) - 1;   /* FIXME: is this correct? */
    screenshot->gfx_position.y = top_border_chars_h * scanlines_per_char;

    data = native_generic_render(screenshot, filename, xsize, ysize, 1);
    return data;
}

#if 0
native_data_t *native_vicii_text_mode_render(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    uint8_t bitmap;
    uint8_t fgcolor;
    uint8_t bgcolor;
    int i, j, k, l;
    native_data_t *data = lib_malloc(sizeof(native_data_t));

    data->filename = filename;
    data->mc_data_present = 0;

    data->xsize = 320;
    data->ysize = 200;
    data->colormap = lib_malloc(320 * 200);

    bgcolor = regs[0x21] & 0xf;

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            fgcolor = screenshot->color_ram_ptr[(i * 40) + j] & 0xf;
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->chargen_ptr[(screenshot->screen_ptr[(i * 40) + j] * 8) + k];
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = fgcolor;
                    } else {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = bgcolor;
                    }
                }
            }
        }
    }
    if (((regs[0x16] & 8) == 0) || ((regs[0x11] & 8) == 0)) {
        native_smooth_scroll_borderize_colormap(data, (uint8_t)(regs[0x20] & 0xf), (uint8_t)((regs[0x16] & 8) ? 255 : regs[0x16] & 7), (uint8_t)((regs[0x11] & 8) ? 255 : regs[0x11] & 7));
    }
    return data;
}
#endif

#if 0
native_data_t *native_vicii_extended_background_mode_render(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    uint8_t bitmap;
    uint8_t fgcolor;
    uint8_t bgcolor;
    int i, j, k, l;
    native_data_t *data = lib_malloc(sizeof(native_data_t));

    data->filename = filename;
    data->mc_data_present = 0;

    data->xsize = 320;
    data->ysize = 200;
    data->colormap = lib_malloc(320 * 200);

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            fgcolor = screenshot->color_ram_ptr[(i * 40) + j] & 0xf;
            bgcolor = regs[0x21 + ((screenshot->screen_ptr[(i * 40) + j] & 0xc0) >> 6)] & 0xf;
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->chargen_ptr[((screenshot->screen_ptr[(i * 40) + j] & 0x3f) * 8) + k];
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = fgcolor;
                    } else {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = bgcolor;
                    }
                }
            }
        }
    }
    if (((regs[0x16] & 8) == 0) || ((regs[0x11] & 8) == 0)) {
        native_smooth_scroll_borderize_colormap(data, (uint8_t)(regs[0x20] & 0xf), (uint8_t)((regs[0x16] & 8) ? 255 : regs[0x16] & 7), (uint8_t)((regs[0x11] & 8) ? 255 : regs[0x11] & 7));
    }
    return data;
}
#endif

#if 0
native_data_t *native_vicii_multicolor_text_mode_render(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    uint8_t bitmap;
    uint8_t color0;
    uint8_t color1;
    uint8_t color2;
    uint8_t color3;
    int i, j, k, l;
    native_data_t *data = lib_malloc(sizeof(native_data_t));

    data->filename = filename;
    data->mc_data_present = 0;

    data->xsize = 320;
    data->ysize = 200;
    data->colormap = lib_malloc(320 * 200);

    color0 = regs[0x21] & 0xf;
    color1 = regs[0x22] & 0xf;
    color2 = regs[0x23] & 0xf;

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            color3 = screenshot->color_ram_ptr[(i * 40) + j] & 0xf;
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->chargen_ptr[(screenshot->screen_ptr[(i * 40) + j] * 8) + k];
                if (color3 & 8) {
                    for (l = 0; l < 4; l++) {
                        data->mc_data_present = 1;
                        switch ((bitmap & (3 << ((3 - l) * 2))) >> ((3 - l) * 2)) {
                            case 0:
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color0;
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color0;
                                break;
                            case 1:
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color1;
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color1;
                                break;
                            case 2:
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color2;
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color2;
                                break;
                            case 3:
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color3 & 7;
                                data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color3 & 7;
                                break;
                        }
                    }
                } else {
                    for (l = 0; l < 8; l++) {
                        if (bitmap & (1 << (7 - l))) {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = color3;
                        } else {
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = color0;
                        }
                    }
                }
            }
        }
    }
    if (((regs[0x16] & 8) == 0) || ((regs[0x11] & 8) == 0)) {
        native_smooth_scroll_borderize_colormap(data, (uint8_t)(regs[0x20] & 0xf), (uint8_t)((regs[0x16] & 8) ? 255 : regs[0x16] & 7), (uint8_t)((regs[0x11] & 8) ? 255 : regs[0x11] & 7));
    }
    return data;
}
#endif

#if 0
native_data_t *native_vicii_hires_bitmap_mode_render(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    uint8_t bitmap;
    uint8_t fgcolor;
    uint8_t bgcolor;
    int i, j, k, l;
    native_data_t *data = lib_malloc(sizeof(native_data_t));

    data->filename = filename;
    data->mc_data_present = 0;

    data->xsize = 320;
    data->ysize = 200;
    data->colormap = lib_malloc(320 * 200);

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            fgcolor = (screenshot->screen_ptr[(i * 40) + j] & 0xf0) >> 4;
            bgcolor = screenshot->screen_ptr[(i * 40) + j] & 0xf;
            for (k = 0; k < 8; k++) {
                if (((i * 40 * 8) + (j * 8) + k) < 4096) {
                    bitmap = screenshot->bitmap_low_ptr[(i * 40 * 8) + (j * 8) + k];
                } else {
                    bitmap = screenshot->bitmap_high_ptr[((i * 40 * 8) + (j * 8) + k) - 4096];
                }
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = fgcolor;
                    } else {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = bgcolor;
                    }
                }
            }
        }
    }
    if (((regs[0x16] & 8) == 0) || ((regs[0x11] & 8) == 0)) {
        native_smooth_scroll_borderize_colormap(data, (uint8_t)(regs[0x20] & 0xf), (uint8_t)((regs[0x16] & 8) ? 255 : regs[0x16] & 7), (uint8_t)((regs[0x11] & 8) ? 255 : regs[0x11] & 7));
    }
    return data;
}
#endif

#if 0
native_data_t *native_vicii_multicolor_bitmap_mode_render(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    uint8_t bitmap;
    uint8_t color0;
    uint8_t color1;
    uint8_t color2;
    uint8_t color3;
    int i, j, k, l;
    native_data_t *data = lib_malloc(sizeof(native_data_t));

    data->filename = filename;
    data->mc_data_present = 1;

    data->xsize = 320;
    data->ysize = 200;
    data->colormap = lib_malloc(320 * 200);

    color0 = regs[0x21] & 0xf;
    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            color1 = (screenshot->screen_ptr[(i * 40) + j] & 0xf0) >> 4;
            color2 = screenshot->screen_ptr[(i * 40) + j] & 0xf;
            color3 = screenshot->color_ram_ptr[(i * 40) + j] & 0xf;
            for (k = 0; k < 8; k++) {
                if (((i * 40 * 8) + (j * 8) + k) < 4096) {
                    bitmap = screenshot->bitmap_low_ptr[(i * 40 * 8) + (j * 8) + k];
                } else {
                    bitmap = screenshot->bitmap_high_ptr[((i * 40 * 8) + (j * 8) + k) - 4096];
                }
                for (l = 0; l < 4; l++) {
                    switch ((bitmap & (3 << ((3 - l) * 2))) >> ((3 - l) * 2)) {
                        case 0:
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color0;
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color0;
                            break;
                        case 1:
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color1;
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color1;
                            break;
                        case 2:
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color2;
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color2;
                            break;
                        case 3:
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color3;
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color3;
                            break;
                    }
                }
            }
        }
    }
    if (((regs[0x16] & 8) == 0) || ((regs[0x11] & 8) == 0)) {
        native_smooth_scroll_borderize_colormap(data, (uint8_t)(regs[0x20] & 0xf), (uint8_t)((regs[0x16] & 8) ? 255 : regs[0x16] & 7), (uint8_t)((regs[0x11] & 8) ? 255 : regs[0x11] & 7));
    }
    return data;
}
#endif

/* ------------------------------------------------------------------------ */

static uint8_t ted_vicii_translate[16] = {
    0x0,    /* ted black        (0) -> vicii black       (0) */
    0x1,    /* ted white        (1) -> vicii white       (1) */
    0x2,    /* ted red          (2) -> vicii red         (2) */
    0x3,    /* ted cyan         (3) -> vicii cyan        (3) */
    0x4,    /* ted purple       (4) -> vicii purple      (4) */
    0x5,    /* ted green        (5) -> vicii green       (5) */
    0x6,    /* ted blue         (6) -> vicii blue        (6) */
    0x7,    /* ted yellow       (7) -> vicii yellow      (7) */
    0x8,    /* ted orange       (8) -> vicii orange      (8) */
    0x9,    /* ted brown        (9) -> vicii brown       (9) */
    0xD,    /* ted yellow-green (A) -> vicii light green (D) */
    0xA,    /* ted pink         (B) -> vicii light red   (A) */
    0xE,    /* ted blue-green   (C) -> vicii light blue  (E) */
    0xE,    /* ted light blue   (D) -> vicii light blue  (E) */
    0x6,    /* ted dark blue    (E) -> vicii blue        (6) */
    0xD     /* ted light green  (F) -> vicii light green (D) */
};

static uint8_t ted_vic_translate[16] = {
    0x0,    /* ted black        (0) -> vic black       (0) */
    0x1,    /* ted white        (1) -> vic white       (1) */
    0x2,    /* ted red          (2) -> vic red         (2) */
    0x3,    /* ted cyan         (3) -> vic cyan        (3) */
    0x4,    /* ted purple       (4) -> vic purple      (4) */
    0x5,    /* ted green        (5) -> vic green       (5) */
    0x6,    /* ted blue         (6) -> vic blue        (6) */
    0x7,    /* ted yellow       (7) -> vic yellow      (7) */
    0x8,    /* ted orange       (8) -> vic orange      (8) */
    0x9,    /* ted brown        (9) -> vic brown       (9) */
    0xF,    /* ted yellow-green (A) -> vic light yellow(F) */
    0xA,    /* ted pink         (B) -> vic light red   (A) */
    0xB,    /* ted blue-green   (C) -> vic light cyan  (B) */
    0xE,    /* ted light blue   (D) -> vic light blue  (E) */
    0x6,    /* ted dark blue    (E) -> vic blue        (6) */
    0xD     /* ted light green  (F) -> vic light green (D) */
};

static uint8_t ted_to_vicii_color(uint8_t color)
{
    return ted_vicii_translate[color];
}

static uint8_t ted_to_vic_color(uint8_t color)
{
    return ted_vic_translate[color];
}

static uint8_t ted_lum_vicii_translate[16 * 8] = {
    0x0,    /* ted black L0        (0) -> vicii black     (0) */
    0x9,    /* ted white L0        (1) -> vicii brown     (9) */
    0x2,    /* ted red L0          (2) -> vicii red       (2) */
    0xB,    /* ted cyan L0         (3) -> vicii dark gray (B) */
    0x6,    /* ted purple L0       (4) -> vicii blue      (6) */
    0x0,    /* ted green L0        (5) -> vicii black     (0) */
    0x6,    /* ted blue L0         (6) -> vicii blue      (6) */
    0x9,    /* ted yellow L0       (7) -> vicii brown     (9) */
    0x9,    /* ted orange L0       (8) -> vicii brown     (9) */
    0x9,    /* ted brown L0        (9) -> vicii brown     (9) */
    0x9,    /* ted yellow-green L0 (A) -> vicii brown     (9) */
    0x9,    /* ted pink L0         (B) -> vicii brown     (9) */
    0x0,    /* ted blue-green L0   (C) -> vicii black     (0) */
    0x6,    /* ted light blue L0   (D) -> vicii blue      (6) */
    0x6,    /* ted dark blue L0    (E) -> vicii blue      (6) */
    0x9,    /* ted light green L0  (F) -> vicii brown     (9) */

    0x0,    /* ted black L1        (0) -> vicii black     (0) */
    0xB,    /* ted white L1        (1) -> vicii dark gray (B) */
    0x2,    /* ted red L1          (2) -> vicii red       (2) */
    0xB,    /* ted cyan L1         (3) -> vicii dark gray (B) */
    0x6,    /* ted purple L1       (4) -> vicii blue      (6) */
    0x9,    /* ted green L1        (5) -> vicii brown     (9) */
    0x6,    /* ted blue L1         (6) -> vicii blue      (6) */
    0x9,    /* ted yellow L1       (7) -> vicii brown     (9) */
    0x2,    /* ted orange L1       (8) -> vicii red       (2) */
    0x9,    /* ted brown L1        (9) -> vicii brown     (9) */
    0x9,    /* ted yellow-green L1 (A) -> vicii brown     (9) */
    0xB,    /* ted pink L1         (B) -> vicii dark gray (B) */
    0xB,    /* ted blue-green L1   (C) -> vicii dark gray (B) */
    0x6,    /* ted light blue L1   (D) -> vicii blue      (6) */
    0x6,    /* ted dark blue L1    (E) -> vicii blue      (6) */
    0x9,    /* ted light green L1  (F) -> vicii brown     (9) */

    0x0,    /* ted black L2        (0) -> vicii black     (0) */
    0xB,    /* ted white L2        (1) -> vicii dark gray (B) */
    0x2,    /* ted red L2          (2) -> vicii red       (2) */
    0xB,    /* ted cyan L2         (3) -> vicii dark gray (B) */
    0x4,    /* ted purple L2       (4) -> vicii purple    (4) */
    0x9,    /* ted green L2        (5) -> vicii brown     (9) */
    0x6,    /* ted blue L2         (6) -> vicii blue      (6) */
    0x9,    /* ted yellow L2       (7) -> vicii brown     (9) */
    0x2,    /* ted orange L2       (8) -> vicii red       (2) */
    0x9,    /* ted brown L2        (9) -> vicii brown     (9) */
    0x9,    /* ted yellow-green L2 (A) -> vicii brown     (9) */
    0xB,    /* ted pink L2         (B) -> vicii dark gray (B) */
    0xB,    /* ted blue-green L2   (C) -> vicii dark gray (B) */
    0x6,    /* ted light blue L2   (D) -> vicii blue      (6) */
    0x6,    /* ted dark blue L2    (E) -> vicii blue      (6) */
    0x9,    /* ted light green L2  (F) -> vicii brown     (9) */

    0x0,    /* ted black L3        (0) -> vicii black     (0) */
    0xB,    /* ted white L3        (1) -> vicii dark gray (B) */
    0x2,    /* ted red L3          (2) -> vicii red       (2) */
    0xB,    /* ted cyan L3         (3) -> vicii dark gray (B) */
    0x4,    /* ted purple L3       (4) -> vicii purple    (4) */
    0x9,    /* ted green L3        (5) -> vicii brown     (9) */
    0x6,    /* ted blue L3         (6) -> vicii blue      (6) */
    0x9,    /* ted yellow L3       (7) -> vicii brown     (9) */
    0x8,    /* ted orange L3       (8) -> vicii orange    (8) */
    0x8,    /* ted brown L3        (9) -> vicii orange    (8) */
    0x9,    /* ted yellow-green L3 (A) -> vicii brown     (9) */
    0x4,    /* ted pink L3         (B) -> vicii purple    (4) */
    0xB,    /* ted blue-green L3   (C) -> vicii dark gray (B) */
    0x6,    /* ted light blue L3   (D) -> vicii blue      (6) */
    0x6,    /* ted dark blue L3    (E) -> vicii blue      (6) */
    0x9,    /* ted light green L3  (F) -> vicii brown     (9) */

    0x0,    /* ted black L4        (0) -> vicii black       (0) */
    0xC,    /* ted white L4        (1) -> vicii medium gray (C) */
    0xA,    /* ted red L4          (2) -> vicii light red   (A) */
    0xE,    /* ted cyan L4         (3) -> vicii light blue  (E) */
    0x4,    /* ted purple L4       (4) -> vicii purple      (4) */
    0x5,    /* ted green L4        (5) -> vicii green       (5) */
    0xE,    /* ted blue L4         (6) -> vicii light blue  (E) */
    0x5,    /* ted yellow L4       (7) -> vicii green       (5) */
    0xA,    /* ted orange L4       (8) -> vicii light red   (A) */
    0x8,    /* ted brown L4        (9) -> vicii orange      (8) */
    0x5,    /* ted yellow-green L4 (A) -> vicii green       (5) */
    0x4,    /* ted pink L4         (B) -> vicii purple      (4) */
    0xC,    /* ted blue-green L4   (C) -> vicii medium gray (C) */
    0xE,    /* ted light blue L4   (D) -> vicii light blue  (E) */
    0xE,    /* ted dark blue L4    (E) -> vicii light blue  (E) */
    0x5,    /* ted light green L4  (F) -> vicii green       (5) */

    0x0,    /* ted black L5        (0) -> vicii black       (0) */
    0xC,    /* ted white L5        (1) -> vicii medium gray (C) */
    0xA,    /* ted red L5          (2) -> vicii light red   (A) */
    0x3,    /* ted cyan L5         (3) -> vicii cyan        (3) */
    0xF,    /* ted purple L5       (4) -> vicii light gray  (F) */
    0x5,    /* ted green L5        (5) -> vicii green       (5) */
    0xE,    /* ted blue L5         (6) -> vicii light blue  (E) */
    0x5,    /* ted yellow L5       (7) -> vicii green       (5) */
    0xA,    /* ted orange L5       (8) -> vicii light red   (A) */
    0xA,    /* ted brown L5        (9) -> vicii light red   (A) */
    0x5,    /* ted yellow-green L5 (A) -> vicii green       (5) */
    0xA,    /* ted pink L5         (B) -> vicii light red   (A) */
    0x3,    /* ted blue-green L5   (C) -> vicii cyan        (3) */
    0xE,    /* ted light blue L5   (D) -> vicii light blue  (E) */
    0xE,    /* ted dark blue L5    (E) -> vicii light blue  (E) */
    0x5,    /* ted light green L5  (F) -> vicii green       (5) */

    0x0,    /* ted black L6        (0) -> vicii black       (0) */
    0xF,    /* ted white L6        (1) -> vicii light gray  (F) */
    0xA,    /* ted red L6          (2) -> vicii light red   (A) */
    0x3,    /* ted cyan L6         (3) -> vicii cyan        (3) */
    0xF,    /* ted purple L6       (4) -> vicii light gray  (F) */
    0xD,    /* ted green L6        (5) -> vicii light green (D) */
    0xF,    /* ted blue L6         (6) -> vicii light gray  (F) */
    0x7,    /* ted yellow L6       (7) -> vicii yellow      (7) */
    0xA,    /* ted orange L6       (8) -> vicii light red   (A) */
    0xA,    /* ted brown L6        (9) -> vicii light red   (A) */
    0x7,    /* ted yellow-green L6 (A) -> vicii yellow      (7) */
    0xF,    /* ted pink L6         (B) -> vicii light gray  (F) */
    0x3,    /* ted blue-green L6   (C) -> vicii cyan        (3) */
    0xF,    /* ted light blue L6   (D) -> vicii light gray  (F) */
    0xF,    /* ted dark blue L6    (E) -> vicii light gray  (F) */
    0x7,    /* ted light green L6  (F) -> vicii yellow      (7) */

    0x0,    /* ted black L7        (0) -> vicii black       (0) */
    0x1,    /* ted white L7        (1) -> vicii white       (1) */
    0x1,    /* ted red L7          (2) -> vicii white       (1) */
    0x1,    /* ted cyan L7         (3) -> vicii white       (1) */
    0x1,    /* ted purple L7       (4) -> vicii white       (1) */
    0xD,    /* ted green L7        (5) -> vicii light green (D) */
    0x1,    /* ted blue L7         (6) -> vicii white       (1) */
    0x7,    /* ted yellow L7       (7) -> vicii yellow      (7) */
    0xF,    /* ted orange L7       (8) -> vicii light gray  (F) */
    0x7,    /* ted brown L7        (9) -> vicii yellow      (7) */
    0x7,    /* ted yellow-green L7 (A) -> vicii yellow      (7) */
    0x1,    /* ted pink L7         (B) -> vicii white       (1) */
    0xD,    /* ted blue-green L7   (C) -> vicii light green (D) */
    0x1,    /* ted light blue L7   (D) -> vicii white       (1) */
    0x1,    /* ted dark blue L7    (E) -> vicii white       (1) */
    0xD     /* ted light green L7  (F) -> vicii light green (D) */
};

/* TODO: this table needs more tweaking */
static uint8_t ted_lum_vic_translate[16 * 8] = {
    0x0,    /* ted black L0        (0) -> vic black     (0) */
    0x0,    /* ted white L0        (1) -> vic black     (0) */
    0x2,    /* ted red L0          (2) -> vic red       (2) */
    0x3,    /* ted cyan L0         (3) -> vic cyan      (3) */
    0x6,    /* ted purple L0       (4) -> vic blue      (6) */
    0x0,    /* ted green L0        (5) -> vic black     (0) */
    0x6,    /* ted blue L0         (6) -> vic blue      (6) */
    0x0,    /* ted yellow L0       (7) -> vic black     (0) */
    0x0,    /* ted orange L0       (8) -> vic black     (0) */
    0x0,    /* ted brown L0        (9) -> vic black     (0) */
    0x0,    /* ted yellow-green L0 (A) -> vic black     (0) */
    0x0,    /* ted pink L0         (B) -> vic black     (0) */
    0x0,    /* ted blue-green L0   (C) -> vic black     (0) */
    0x6,    /* ted light blue L0   (D) -> vic blue      (6) */
    0x6,    /* ted dark blue L0    (E) -> vic blue      (6) */
    0x0,    /* ted light green L0  (F) -> vic black     (0) */

    0x0,    /* ted black L1        (0) -> vic black     (0) */
    0x0,    /* ted white L1        (1) -> vic black     (0) */
    0x2,    /* ted red L1          (2) -> vic red       (2) */
    0x3,    /* ted cyan L1         (3) -> vic cyan      (C) */
    0x6,    /* ted purple L1       (4) -> vic blue      (6) */
    0x0,    /* ted green L1        (5) -> vic black     (0) */
    0x6,    /* ted blue L1         (6) -> vic blue      (6) */
    0x0,    /* ted yellow L1       (7) -> vic black     (0) */
    0x2,    /* ted orange L1       (8) -> vic red       (2) */
    0x0,    /* ted brown L1        (9) -> vic black     (0) */
    0x0,    /* ted yellow-green L1 (A) -> vic black     (0) */
    0x2,    /* ted pink L1         (B) -> vic red       (2) */
    0x6,    /* ted blue-green L1   (C) -> vic blue      (6) */
    0x6,    /* ted light blue L1   (D) -> vic blue      (6) */
    0x6,    /* ted dark blue L1    (E) -> vic blue      (6) */
    0x5,    /* ted light green L1  (F) -> vic green     (5) */

    0x0,    /* ted black L2        (0) -> vic black     (0) */
    0x0,    /* ted white L2        (1) -> vic black     (0) */
    0x2,    /* ted red L2          (2) -> vic red       (2) */
    0x3,    /* ted cyan L2         (3) -> vic cyan      (3) */
    0x4,    /* ted purple L2       (4) -> vic purple    (4) */
    0x9,    /* ted green L2        (5) -> vic black     (0) */
    0x6,    /* ted blue L2         (6) -> vic blue      (6) */
    0x0,    /* ted yellow L2       (7) -> vic black     (0) */
    0x2,    /* ted orange L2       (8) -> vic red       (2) */
    0x0,    /* ted brown L2        (9) -> vic black     (0) */
    0x0,    /* ted yellow-green L2 (A) -> vic black     (0) */
    0x2,    /* ted pink L2         (B) -> vic red       (2) */
    0x6,    /* ted blue-green L2   (C) -> vic blue      (6) */
    0x6,    /* ted light blue L2   (D) -> vic blue      (6) */
    0x6,    /* ted dark blue L2    (E) -> vic blue      (6) */
    0x5,    /* ted light green L2  (F) -> vic green     (5) */

    0x0,    /* ted black L3        (0) -> vic black       (0) */
    0x0,    /* ted white L3        (1) -> vic black       (0) */
    0x2,    /* ted red L3          (2) -> vic red         (2) */
    0x3,    /* ted cyan L3         (3) -> vic cyan        (3) */
    0x4,    /* ted purple L3       (4) -> vic purple      (4) */
    0x0,    /* ted green L3        (5) -> vic black       (0) */
    0x6,    /* ted blue L3         (6) -> vic blue        (6) */
    0x0,    /* ted yellow L3       (7) -> vic black       (0) */
    0x8,    /* ted orange L3       (8) -> vic orange      (8) */
    0x8,    /* ted brown L3        (9) -> vic orange      (8) */
    0x0,    /* ted yellow-green L3 (A) -> vic black       (0) */
    0x4,    /* ted pink L3         (B) -> vic purple      (4) */
    0x6,    /* ted blue-green L3   (C) -> vic blue        (6) */
    0x6,    /* ted light blue L3   (D) -> vic blue        (6) */
    0x6,    /* ted dark blue L3    (E) -> vic blue        (6) */
    0x5,    /* ted light green L3  (F) -> vic green       (5) */

    0x0,    /* ted black L4        (0) -> vic black       (0) */
    0x1,    /* ted white L4        (1) -> vic white       (1) */
    0xA,    /* ted red L4          (2) -> vic pink        (A) */
    0xE,    /* ted cyan L4         (3) -> vic light blue  (E) */
    0x4,    /* ted purple L4       (4) -> vic purple      (4) */
    0x5,    /* ted green L4        (5) -> vic green       (5) */
    0xE,    /* ted blue L4         (6) -> vic light blue  (E) */
    0x5,    /* ted yellow L4       (7) -> vic green       (5) */
    0xA,    /* ted orange L4       (8) -> vic pink        (A) */
    0x8,    /* ted brown L4        (9) -> vic orange      (8) */
    0x5,    /* ted yellow-green L4 (A) -> vic green       (5) */
    0x4,    /* ted pink L4         (B) -> vic purple      (4) */
    0x3,    /* ted blue-green L4   (C) -> vic cyan        (3) */
    0xE,    /* ted light blue L4   (D) -> vic light blue  (E) */
    0xE,    /* ted dark blue L4    (E) -> vic light blue  (E) */
    0x5,    /* ted light green L4  (F) -> vic green       (5) */

    0x0,    /* ted black L5        (0) -> vic black       (0) */
    0x1,    /* ted white L5        (1) -> vic white       (1) */
    0xA,    /* ted red L5          (2) -> vic pink        (A) */
    0x3,    /* ted cyan L5         (3) -> vic cyan        (3) */
    0xC,    /* ted purple L5       (4) -> vic light purple(C) */
    0x5,    /* ted green L5        (5) -> vic green       (5) */
    0xE,    /* ted blue L5         (6) -> vic light blue  (E) */
    0x5,    /* ted yellow L5       (7) -> vic green       (5) */
    0xA,    /* ted orange L5       (8) -> vic pink        (A) */
    0xA,    /* ted brown L5        (9) -> vic pink        (A) */
    0x5,    /* ted yellow-green L5 (A) -> vic green       (5) */
    0xA,    /* ted pink L5         (B) -> vic pink        (A) */
    0x3,    /* ted blue-green L5   (C) -> vic cyan        (3) */
    0xE,    /* ted light blue L5   (D) -> vic light blue  (E) */
    0xE,    /* ted dark blue L5    (E) -> vic light blue  (E) */
    0x5,    /* ted light green L5  (F) -> vic green       (5) */

    0x0,    /* ted black L6        (0) -> vic black       (0) */
    0x1,    /* ted white L6        (1) -> vic white       (1) */
    0xA,    /* ted red L6          (2) -> vic pink        (A) */
    0x3,    /* ted cyan L6         (3) -> vic cyan        (3) */
    0xC,    /* ted purple L6       (4) -> vic light purple(C) */
    0xD,    /* ted green L6        (5) -> vic light green (D) */
    0xE,    /* ted blue L6         (6) -> vic light blue  (E) */
    0x7,    /* ted yellow L6       (7) -> vic yellow      (7) */
    0xA,    /* ted orange L6       (8) -> vic pink        (A) */
    0xA,    /* ted brown L6        (9) -> vic pink        (A) */
    0x7,    /* ted yellow-green L6 (A) -> vic yellow      (7) */
    0xA,    /* ted pink L6         (B) -> vic pink        (A) */
    0x3,    /* ted blue-green L6   (C) -> vic cyan        (3) */
    0xE,    /* ted light blue L6   (D) -> vic light blue  (E) */
    0xE,    /* ted dark blue L6    (E) -> vic light blue  (E) */
    0x7,    /* ted light green L6  (F) -> vic yellow      (7) */

    0x0,    /* ted black L7        (0) -> vic black       (0) */
    0x1,    /* ted white L7        (1) -> vic white       (1) */
    0x1,    /* ted red L7          (2) -> vic white       (1) */
    0x1,    /* ted cyan L7         (3) -> vic white       (1) */
    0x1,    /* ted purple L7       (4) -> vic white       (1) */
    0xD,    /* ted green L7        (5) -> vic light green (D) */
    0x1,    /* ted blue L7         (6) -> vic white       (1) */
    0x7,    /* ted yellow L7       (7) -> vic yellow      (7) */
    0x9,    /* ted orange L7       (8) -> vic light orange(9) */
    0x7,    /* ted brown L7        (9) -> vic yellow      (7) */
    0x7,    /* ted yellow-green L7 (A) -> vic yellow      (7) */
    0x1,    /* ted pink L7         (B) -> vic white       (1) */
    0xD,    /* ted blue-green L7   (C) -> vic light green (D) */
    0x1,    /* ted light blue L7   (D) -> vic white       (1) */
    0x1,    /* ted dark blue L7    (E) -> vic white       (1) */
    0xD     /* ted light green L7  (F) -> vic light green (D) */
};

static uint8_t ted_lum_to_vicii_color(uint8_t color, uint8_t lum)
{
    return ted_lum_vicii_translate[(lum * 16) + color];
}

void ted_color_to_vicii_color_colormap(native_data_t *source, int ted_lum_handling)
{
    int i, j;
    uint8_t colorbyte;

    for (i = 0; i < source->ysize; i++) {
        for (j = 0; j < source->xsize; j++) {
            colorbyte = source->colormap[(i * source->xsize) + j];
            if (ted_lum_handling == NATIVE_SS_TED_LUM_DITHER) {
                source->colormap[(i * source->xsize) + j] =
                    ted_lum_to_vicii_color((uint8_t)(colorbyte & 0xf), (uint8_t)(colorbyte >> 4));
            } else {
                source->colormap[(i * source->xsize) + j] =
                    ted_to_vicii_color((uint8_t)(colorbyte & 0xf));
            }
        }
    }
}

static uint8_t ted_lum_to_vic_color(uint8_t color, uint8_t lum)
{
    return ted_lum_vic_translate[(lum * 16) + color];
}

void ted_color_to_vic_color_colormap(native_data_t *source, int ted_lum_handling)
{
    int i, j;
    uint8_t colorbyte;

    for (i = 0; i < source->ysize; i++) {
        for (j = 0; j < source->xsize; j++) {
            colorbyte = source->colormap[(i * source->xsize) + j];
            if (ted_lum_handling == NATIVE_SS_TED_LUM_DITHER) {
                source->colormap[(i * source->xsize) + j] =
                    ted_lum_to_vic_color((uint8_t)(colorbyte & 0xf), (uint8_t)(colorbyte >> 4));
            } else {
                source->colormap[(i * source->xsize) + j] =
                    ted_to_vic_color((uint8_t)(colorbyte & 0xf));
            }
        }
    }
}

#if 0
native_data_t *native_ted_text_mode_render(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    uint8_t bitmap;
    uint8_t fgcolor;
    uint8_t bgcolor;
    uint8_t brdrcolor;
    int i, j, k, l;
    native_data_t *data = lib_malloc(sizeof(native_data_t));

    data->filename = filename;
    data->mc_data_present = 0;

    data->xsize = 320;
    data->ysize = 200;

    data->colormap = lib_malloc(320 * 200);

    bgcolor = regs[0x15] & 0x7f;

    brdrcolor = regs[0x19] & 0x7f;

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            fgcolor = screenshot->color_ram_ptr[(i * 40) + j] & 0x7f;
            for (k = 0; k < 8; k++) {
                if (regs[0x07] & 0x80) {
                    bitmap = screenshot->chargen_ptr[(screenshot->screen_ptr[(i * 40) + j] * 8) + k];
                } else {
                    bitmap = screenshot->chargen_ptr[((screenshot->screen_ptr[(i * 40) + j] & 0x7f) * 8) + k];
                    if (screenshot->screen_ptr[(i * 40) + j] & 0x80) {
                        bitmap = ~bitmap;
                    }
                }
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = fgcolor;
                    } else {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = bgcolor;
                    }
                }
            }
        }
    }
    if (((regs[0x07] & 8) == 0) || ((regs[0x06] & 8) == 0)) {
        native_smooth_scroll_borderize_colormap(data, brdrcolor, (uint8_t)((regs[0x07] & 8) ? 255 : regs[0x07] & 7), (uint8_t)((regs[0x06] & 8) ? 255 : regs[0x06] & 7));
    }
    return data;
}
#endif

#if 0
native_data_t *native_ted_extended_background_mode_render(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    uint8_t bitmap;
    uint8_t fgcolor;
    uint8_t bgcolor;
    uint8_t brdrcolor;
    int i, j, k, l;
    native_data_t *data = lib_malloc(sizeof(native_data_t));

    data->filename = filename;
    data->mc_data_present = 0;

    data->xsize = 320;
    data->ysize = 200;

    data->colormap = lib_malloc(320 * 200);

    brdrcolor = regs[0x19] & 0x7f;

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            fgcolor = screenshot->color_ram_ptr[(i * 40) + j] & 0x7f;
            bgcolor = regs[0x15 + ((screenshot->screen_ptr[(i * 40) + j] & 0xc0) >> 6)] & 0x7f;
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->chargen_ptr[((screenshot->screen_ptr[(i * 40) + j] & 0x3f) * 8) + k];
                if ((regs[0x07] & 0x80) && (screenshot->screen_ptr[(i * 40) + j] & 0x80)) {
                    bitmap = ~bitmap;
                }
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = fgcolor;
                    } else {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = bgcolor;
                    }
                }
            }
        }
    }
    if (((regs[0x07] & 8) == 0) || ((regs[0x06] & 8) == 0)) {
        native_smooth_scroll_borderize_colormap(data, brdrcolor, (uint8_t)((regs[0x07] & 8) ? 255 : regs[0x07] & 7), (uint8_t)((regs[0x06] & 8) ? 255 : regs[0x06] & 7));
    }
    return data;
}
#endif

#if 0
native_data_t *native_ted_hires_bitmap_mode_render(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    uint8_t bitmap;
    uint8_t fgcolor;
    uint8_t bgcolor;
    uint8_t brdrcolor;
    int i, j, k, l;
    native_data_t *data = lib_malloc(sizeof(native_data_t));

    data->filename = filename;
    data->mc_data_present = 0;

    data->xsize = 320;
    data->ysize = 200;

    data->colormap = lib_malloc(320 * 200);

    brdrcolor = regs[0x19] & 0x7f;

    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            fgcolor = (screenshot->screen_ptr[(i * 40) + j] & 0xf0) >> 4;
            fgcolor |= (screenshot->screen_ptr[(i * 40) + j] & 0x70);
            bgcolor = screenshot->screen_ptr[(i * 40) + j] & 0xf;
            bgcolor |= ((screenshot->screen_ptr[(i * 40) + j] & 0x7) << 4);
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->bitmap_ptr[(i * 40 * 8) + j + (k * 40)];
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = fgcolor;
                    } else {
                        data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = bgcolor;
                    }
                }
            }
        }
    }
    if (((regs[0x07] & 8) == 0) || ((regs[0x06] & 8) == 0)) {
        native_smooth_scroll_borderize_colormap(data, brdrcolor, (uint8_t)((regs[0x07] & 8) ? 255 : regs[0x07] & 7), (uint8_t)((regs[0x06] & 8) ? 255 : regs[0x06] & 7));
    }
    return data;
}
#endif

#if 0
native_data_t *native_ted_multicolor_bitmap_mode_render(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    uint8_t bitmap;
    uint8_t color0;
    uint8_t color1;
    uint8_t color2;
    uint8_t color3;
    uint8_t brdrcolor;
    int i, j, k, l;
    native_data_t *data = lib_malloc(sizeof(native_data_t));

    data->filename = filename;
    data->mc_data_present = 1;

    data->xsize = 320;
    data->ysize = 200;
    data->colormap = lib_malloc(320 * 200);

    brdrcolor = regs[0x19] & 0x7f;

    color0 = regs[0x15];
    color3 = regs[0x16];
    for (i = 0; i < 25; i++) {
        for (j = 0; j < 40; j++) {
            color1 = (screenshot->screen_ptr[(i * 40) + j] & 0xf0) >> 4;
            color1 |= (screenshot->screen_ptr[(i * 40) + j] & 0x70);
            color2 = screenshot->screen_ptr[(i * 40) + j] & 0xf;
            color2 |= ((screenshot->screen_ptr[(i * 40) + j] & 0x7) << 4);
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->bitmap_ptr[(i * 40 * 8) + j + (k * 40)];
                for (l = 0; l < 4; l++) {
                    switch ((bitmap & (3 << ((3 - l) * 2))) >> ((3 - l) * 2)) {
                        case 0:
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color0;
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color0;
                            break;
                        case 1:
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color1;
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color1;
                            break;
                        case 2:
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color2;
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color2;
                            break;
                        case 3:
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color3;
                            data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color3;
                            break;
                    }
                }
            }
        }
    }
    if (((regs[0x07] & 8) == 0) || ((regs[0x06] & 8) == 0)) {
        native_smooth_scroll_borderize_colormap(data, brdrcolor, (uint8_t)((regs[0x07] & 8) ? 255 : regs[0x07] & 7), (uint8_t)((regs[0x06] & 8) ? 255 : regs[0x06] & 7));
    }
    return data;
}
#endif

/* ------------------------------------------------------------------------ */

static uint8_t vic_vicii_translate[VIC_NUM_COLORS] = {
    0x0,    /* vic black        (0) -> vicii black       (0) */
    0x1,    /* vic white        (1) -> vicii white       (1) */
    0x2,    /* vic red          (2) -> vicii red         (2) */
    0x3,    /* vic cyan         (3) -> vicii cyan        (3) */
    0x4,    /* vic purple       (4) -> vicii purple      (4) */
    0x5,    /* vic green        (5) -> vicii green       (5) */
    0x6,    /* vic blue         (6) -> vicii blue        (6) */
    0x7,    /* vic yellow       (7) -> vicii yellow      (7) */
    0x8,    /* vic orange       (8) -> vicii orange      (8) */
    0x8,    /* vic light orange (9) -> vicii orange      (8) */
    0xa,    /* vic pink         (A) -> vicii light red   (a) */
    0xD,    /* vic light cyan   (B) -> vicii light green (D) */
    0x4,    /* vic light purple (C) -> vicii purple      (4) */
    0xD,    /* vic light green  (D) -> vicii light green (D) */
    0xE,    /* vic light blue   (E) -> vicii light blue  (E) */
    0xF     /* vic light yellow (F) -> vicii light grey  (F) */
};

static uint8_t vicii_vic_translate[VICII_NUM_COLORS] = {
    0x0,    /* vicii black       (0) -> vic black        (0) */
    0x1,    /* vicii white       (1) -> vic white        (1) */
    0x2,    /* vicii red         (2) -> vic red          (2) */
    0x3,    /* vicii cyan        (3) -> vic cyan         (3) */
    0x4,    /* vicii purple      (4) -> vic purple       (4) */
    0x5,    /* vicii green       (5) -> vic green        (5) */
    0x6,    /* vicii blue        (6) -> vic blue         (6) */
    0x7,    /* vicii yellow      (7) -> vic yellow       (7) */
    0x9,    /* vicii orange      (8) -> vic light orange (9) */
    0x8,    /* vicii brown       (9) -> vic orange       (8) */
    0xa,    /* vicii light red   (A) -> vic pink         (a) */
    0x2,    /* vicii dark gray   (B) -> vic red          (2) */
    0x8,    /* vicii medium gray (C) -> vic orange       (8) */
    0xd,    /* vicii light green (D) -> vic light green  (d) */
    0xe,    /* vicii light blue  (E) -> vic light blue   (e) */
    0xf     /* vicii light gray  (F) -> vic light yellow (f) */
};

static inline uint8_t vic_to_vicii_color(uint8_t color)
{
    return vic_vicii_translate[color];
}

void vic_color_to_vicii_color_colormap(native_data_t *source)
{
    int i, j;

    for (i = 0; i < source->ysize; i++) {
        for (j = 0; j < source->xsize; j++) {
            source->colormap[(i * source->xsize) + j] =
                vic_to_vicii_color(source->colormap[(i * source->xsize) + j]);
        }
    }
}

static inline uint8_t vicii_to_vic_color(uint8_t color)
{
    return vicii_vic_translate[color];
}

void vicii_color_to_vic_color_colormap(native_data_t *source)
{
    int i, j;

    for (i = 0; i < source->ysize; i++) {
        for (j = 0; j < source->xsize; j++) {
            source->colormap[(i * source->xsize) + j] =
                vicii_to_vic_color(source->colormap[(i * source->xsize) + j]);
        }
    }
}

#if 0
native_data_t *native_vic_render(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    uint8_t bitmap;
    uint8_t fgcolor;
    uint8_t bgcolor;
    uint8_t brdrcolor;
    uint8_t auxcolor;
    int i, j, k, l;
    native_data_t *data;
    uint8_t xsize;
    uint8_t ysize;

    xsize = regs[0x02] & 0x7f;
    ysize = (regs[0x03] & 0x7e) >> 1;

    if (xsize == 0 || ysize == 0) {
        ui_error("Screen is blank, no save will be done");
        return NULL;
    }

    if (screenshot->chargen_ptr == NULL) {
        ui_error("Character generator memory is illegal");
        return NULL;
    }

    data = lib_malloc(sizeof(native_data_t));

    data->filename = filename;
    data->mc_data_present = 0;

    data->xsize = xsize * 8;
    data->ysize = ysize * 8;

    data->colormap = lib_malloc(data->xsize * data->ysize);

    bgcolor = (regs[0xf] & 0xf0) >> 4;
    auxcolor = (regs[0xe] & 0xf0) >> 4;
    brdrcolor = regs[0xf] & 3;
    for (i = 0; i < ysize; i++) {
        for (j = 0; j < xsize; j++) {
            fgcolor = screenshot->color_ram_ptr[(i * xsize) + j] & 7;
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->chargen_ptr[(screenshot->screen_ptr[(i * xsize) + j] * 8) + k];
                if (!(regs[0xf] & 8)) {
                    bitmap = ~bitmap;
                }
                if (screenshot->color_ram_ptr[(i * xsize) + j] & 8) {
                    for (l = 0; l < 4; l++) {
                        data->mc_data_present = 1;
                        switch ((bitmap & (3 << ((3 - l) * 2))) >> ((3 - l) * 2)) {
                            case 0:
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2)] = bgcolor;
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2) + 1] = bgcolor;
                                break;
                            case 1:
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2)] = brdrcolor;
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2) + 1] = brdrcolor;
                                break;
                            case 2:
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2)] = fgcolor;
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2) + 1] = fgcolor;
                                break;
                            case 3:
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2)] = auxcolor;
                                data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + (l * 2) + 1] = auxcolor;
                                break;
                        }
                    }
                } else {
                    for (l = 0; l < 8; l++) {
                        if (bitmap & (1 << (7 - l))) {
                            data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + l] = fgcolor;
                        } else {
                            data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + l] = bgcolor;
                        }
                    }
                }
            }
        }
    }
    return data;
}
#endif
/* ------------------------------------------------------------------------ */
#if 0

#define MA_WIDTH        64
#define MA_LO           (MA_WIDTH - 1)          /* 6 bits */
#define MA_HI           (~MA_LO)

native_data_t *native_crtc_render(screenshot_t *screenshot, const char *filename, int crtc_fgcolor)
{
    uint8_t *regs = screenshot->video_regs;
    uint8_t *petdww_ram = screenshot->bitmap_ptr;
    uint8_t bitmap;
    uint8_t fgcolor;
    uint8_t bgcolor;
    int x, y, k, l;
    native_data_t *data;
    uint8_t xsize;
    uint8_t ysize;
    uint8_t invert;
    uint8_t charheight;
    int base;
    int shiftand;
    int chars = 1;
    int hre = 0;
    int col80;
    int scr_rel;

    switch (screenshot->bitmap_low_ptr[0]) {
        default:
        case 40:
            xsize = regs[0x01];
            base = ((regs[0x0c] & 3) << 8) + regs[0x0d];
            shiftand = 0x3ff;
            col80 = 0;
            break;
        case 60:
            xsize = regs[0x01];
            base = ((regs[0x0c] & 3) << 8) + regs[0x0d];
            shiftand = 0x7ff;
            col80 = 0;
            break;
        case 80:
            xsize = regs[0x01] << 1;
            base = (((regs[0x0c] & 3) << 9) + regs[0x0d]) << 1;
            shiftand = 0x7ff;
            col80 = 1;
            break;
    }

    ysize = regs[0x06];
    invert = (regs[0x0c] & 0x10) >> 4;

    if (!invert) {      /* On 8296 only! */
        hre = 1;
        chars = 0;
        invert = 1;
    }

    if (!hre && (xsize == 0 || ysize == 0)) {
        ui_error("Screen is blank, no save will be done");
        return NULL;
    }

    charheight = screenshot->bitmap_high_ptr[0];

    data = lib_malloc(sizeof(native_data_t));

    data->filename = filename;
    data->mc_data_present = 0;

    if (hre) {
        data->xsize = 512;
        data->ysize = 256;
    } else {
        data->xsize = xsize * 8;
        data->ysize = ysize * charheight;
    }

    data->colormap = lib_malloc(data->xsize * data->ysize);

    bitmap = 0;
    bgcolor = 0;
    fgcolor = crtc_fgcolor;
    scr_rel = base;

    if (hre) {
        int ma_hi = scr_rel & MA_HI;    /* MA<9...6> */
        int ma_lo = scr_rel & MA_LO;    /* MA<5...0> */
        /* Form <MA 9-6><RA 2-0><MA 5-0> */
        int addr = ((ma_hi << 3) + ma_lo) >> 1;

        for (l = 0; l < 16384; l++) {
            bitmap = screenshot->screen_ptr[addr + l];
            for (k = 0; k < 8; k++) {
                if (bitmap & (1 << (7 - k))) {
                    data->colormap[(l * 8) + k] = fgcolor;
                } else {
                    data->colormap[(l * 8) + k] = bgcolor;
                }
            }
        }
    } else {
        for (y = 0; y < ysize; y++) {
            for (x = 0; x < xsize; x++) {
                for (k = 0; k < charheight; k++) {
                    bitmap = 0;
                    if (chars) {
                        uint8_t chr = screenshot->screen_ptr[scr_rel & shiftand];
                        bitmap = screenshot->chargen_ptr[(chr * 16) + k];
                    }
                    if (petdww_ram && k < 8) {
                        int addr = (k * 1024) + ((scr_rel >> col80) & 0x3FF);
                        uint8_t b = petdww_ram[addr];
                        /*
                         * If we're in an 80 column screen we need to
                         * horizontally double all pixels, since DWW
                         * only has 40 characters worth of pixels.
                         * Fetch the same byte for odd and even matrix
                         * addresses (above), then double the bits from
                         * first the left half (low nybble) and then the
                         * right half (high nybble).
                         */
                        if (col80) {
                            if (x & 1) {
                                b >>= 4;        /* show right half */
                            } else {
                                b &= 0x0F;      /* show left half */
                            }
                            /*
                             * Double the bits by interleaving with self
                             * http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
                             */
                            b = (b | (b << 2)) & 0x33;
                            b = (b | (b << 1)) & 0x55;
                            b |= b << 1;
                        }
                        /*
                        * Now reverse the bits...
                        * http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith32Bits
                        */
                        b = ((b * 0x0802U & 0x22110U) | (b * 0x8020U & 0x88440U)) * 0x10101U >> 16;
                        bitmap |= b;
                    }
                }
                if (!invert) {
                    bitmap = ~bitmap;
                }
                for (l = 0; l < 8; l++) {
                    int color;

                    if (bitmap & (1 << (7 - l))) {
                        color = fgcolor;
                    } else {
                        color = bgcolor;
                    }
                    data->colormap[(y * data->xsize * charheight) + (x * 8) + (k * data->xsize) + l] = color;
                }
            }
            scr_rel++;
        }
    }
    return data;
}
#endif
/* ------------------------------------------------------------------------ */

static uint8_t vdc_vicii_translate[16] = {
    0x0,    /* vdc black        (0) -> vicii black       (0) */
    0xB,    /* vdc dark gray    (1) -> vicii dark gray   (B) */
    0x6,    /* vdc dark blue    (2) -> vicii blue        (6) */
    0xE,    /* vdc light blue   (3) -> vicii light blue  (E) */
    0x5,    /* vdc dark green   (4) -> vicii green       (5) */
    0xD,    /* vdc light green  (5) -> vicii light green (D) */
    0x6,    /* vdc dark cyan    (6) -> vicii blue        (6) */
    0x3,    /* vdc light cyan   (7) -> vicii cyan        (3) */
    0x2,    /* vdc dark red     (8) -> vicii red         (2) */
    0x8,    /* vdc light red    (9) -> vicii orange      (8) */
    0x4,    /* vdc dark purple  (A) -> vicii purple      (4) */
    0xA,    /* vdc light purple (B) -> vicii light red   (A) */
    0x7,    /* vdc dark yellow  (C) -> vicii yellow      (7) */
    0x7,    /* vdc light yellow (D) -> vicii yellow      (7) */
    0xF,    /* vdc light gray   (E) -> vicii light gray  (F) */
    0x1     /* vdc white        (F) -> vicii white       (1) */
};

static uint8_t vdc_vic_translate[16] = {
    0x0,    /* vdc black        (0) -> vic black       (0) */
    0x0,    /* vdc dark gray    (1) -> vic black       (0) */
    0x6,    /* vdc dark blue    (2) -> vic blue        (6) */
    0xe,    /* vdc light blue   (3) -> vic light blue  (e) */
    0x5,    /* vdc dark green   (4) -> vic green       (5) */
    0xd,    /* vdc light green  (5) -> vic light green (d) */
    0x3,    /* vdc dark cyan    (6) -> vic cyan        (3) */
    0xb,    /* vdc light cyan   (7) -> vic light cyan  (b) */
    0x2,    /* vdc dark red     (8) -> vic red         (2) */
    0xa,    /* vdc light red    (9) -> vic pink        (a) */
    0x4,    /* vdc dark purple  (A) -> vic purple      (4) */
    0xc,    /* vdc light purple (B) -> vic light purple(c) */
    0x7,    /* vdc dark yellow  (C) -> vic yellow      (7) */
    0xe,    /* vdc light yellow (D) -> vic light yellow(e) */
    0xe,    /* vdc light gray   (E) -> vic light yellow(e) */
    0x1     /* vdc white        (F) -> vic white       (1) */
};

static inline uint8_t vdc_to_vicii_color(uint8_t color)
{
    return vdc_vicii_translate[color];
}

void vdc_color_to_vicii_color_colormap(native_data_t *source)
{
    int i, j;

    for (i = 0; i < source->ysize; i++) {
        for (j = 0; j < source->xsize; j++) {
            source->colormap[(i * source->xsize) + j] =
                vdc_to_vicii_color(source->colormap[(i * source->xsize) + j]);
        }
    }
}

static inline uint8_t vdc_to_vic_color(uint8_t color)
{
    return vdc_vic_translate[color];
}

void vdc_color_to_vic_color_colormap(native_data_t *source)
{
    int i, j;

    for (i = 0; i < source->ysize; i++) {
        for (j = 0; j < source->xsize; j++) {
            source->colormap[(i * source->xsize) + j] =
                vdc_to_vic_color(source->colormap[(i * source->xsize) + j]);
        }
    }
}

#if 0
native_data_t *native_vdc_text_mode_render(screenshot_t *screenshot, const char *filename)
{
    uint8_t *regs = screenshot->video_regs;
    uint8_t displayed_chars_h = regs[1];
    uint8_t displayed_chars_v = regs[6];
    /* BYTE scanlines_per_char = (regs[9] & 0x1f) + 1;
    BYTE char_h_size_alloc = regs[22] & 0xf;
    BYTE char_h_size_displayed = (regs[22] & 0xf0) >> 4; */
    uint8_t bitmap;
    uint8_t fgcolor;
    uint8_t bgcolor;
    int i, j, k, l;
    native_data_t *data = lib_malloc(sizeof(native_data_t));

    data->filename = filename;
    data->mc_data_present = 0;

    /* X size calculation is not completely correct,
       char x and double pixel is not taken into account yet,
       correct calculation will become : data->xsize = displayed_chars_h * char_h_size_displayed * ((regs[25] & 0x10) ? 2 : 1);
     */
    data->xsize = displayed_chars_h * 8;

    /* Y size calculation is not completely correct,
       scanlines per char is not taken into account yet,
       correct calculation will become : data->ysize = displayed_chars_v * scanlines_per_char;
     */
    data->ysize = displayed_chars_v * 8;

    data->colormap = lib_malloc(data->xsize * data->ysize);

    fgcolor = 1;
    bgcolor = regs[0x26] & 0xf;

    if (!(regs[25] & 0x40)) {
        fgcolor = (regs[26] & 0xf0) >> 4;
    }

    /* bitmap filling is not completely correct,
      char x allocation, char x display, skipped chars, smooth scrolling and
      blanked borders are not taken into account yet.
     */
    for (i = 0; i < data->ysize / 8; i++) {
        for (j = 0; j < data->xsize / 8; j++) {
            if (regs[25] & 0x40) {
                fgcolor = screenshot->color_ram_ptr[(i * (data->xsize / 8)) + j] & 0x7f;
            }
            for (k = 0; k < 8; k++) {
                bitmap = screenshot->chargen_ptr[(screenshot->screen_ptr[(i * data->xsize / 8) + j] * 16) + k];
                for (l = 0; l < 8; l++) {
                    if (bitmap & (1 << (7 - l))) {
                        data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + l] = fgcolor;
                    } else {
                        data->colormap[(i * data->xsize * 8) + (j * 8) + (k * data->xsize) + l] = bgcolor;
                    }
                }
            }
        }
    }
    return data;
}
#endif
