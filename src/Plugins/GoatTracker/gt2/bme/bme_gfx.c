//
// BME (Blasphemous Multimedia Engine) graphics main module
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "SDL.h"

#include "bme_main.h"
#include "bme_cfg.h"
#include "bme_win.h"
#include "bme_io.h"
#include "bme_err.h"

// Prototypes

int gfx_init(unsigned xsize, unsigned ysize, unsigned framerate, unsigned flags);
int gfx_reinit(void);
void gfx_uninit(void);
int gfx_lock(void);
void gfx_unlock(void);
void gfx_flip(void);
void gfx_setclipregion(unsigned left, unsigned top, unsigned right, unsigned bottom);
void gfx_setmaxspritefiles(int num);
void gfx_setmaxcolors(int num);
int gfx_loadpalette(char *name);
void gfx_calcpalette(int fade, int radd, int gadd, int badd);
void gfx_setpalette(void);
int gfx_loadblocks(char *name);
int gfx_loadsprites(int num, char *name);
void gfx_freesprites(int num);

void gfx_drawsprite(int x, int y, unsigned num);
void gfx_getspriteinfo(unsigned num);

int gfx_initted = 0;
int gfx_redraw = 0;
int gfx_fullscreen = 0;
int gfx_scanlinemode = 0;
int gfx_preventswitch = 0;
int gfx_virtualxsize;
int gfx_virtualysize;
int gfx_windowxsize;
int gfx_windowysize;
int gfx_blockxsize = 16;
int gfx_blockysize = 16;
int spr_xsize = 0;
int spr_ysize = 0;
int spr_xhotspot = 0;
int spr_yhotspot = 0;
unsigned gfx_nblocks = 0;
Uint8 gfx_palette[MAX_COLORS * 3] = {0};
SDL_Surface *gfx_screen = NULL;

// Static variables

static int gfx_initexec = 0;
static unsigned gfx_last_xsize;
static unsigned gfx_last_ysize;
static unsigned gfx_last_framerate;
static unsigned gfx_last_flags;
static int gfx_cliptop;
static int gfx_clipbottom;
static int gfx_clipleft;
static int gfx_clipright;
static int gfx_maxcolors = MAX_COLORS;
static int gfx_maxspritefiles = 0;
static SPRITEHEADER **gfx_spriteheaders = NULL;
static Uint8 **gfx_spritedata = NULL;
static unsigned *gfx_spriteamount = NULL;
SDL_Color gfx_sdlpalette[MAX_COLORS];
static int gfx_locked = 0;

int gfx_init(unsigned xsize, unsigned ysize, unsigned framerate, unsigned flags)
{
	LOGD("gfx_init");
	
//    int sdlflags = SDL_HWSURFACE;

    // Prevent re-entry (by window procedure)
    if (gfx_initexec) return BME_OK;
    gfx_initexec = 1;

    gfx_last_xsize = xsize;
    gfx_last_ysize = ysize;
    gfx_last_framerate = framerate;
    gfx_last_flags = flags & ~(GFX_FULLSCREEN | GFX_WINDOW);

    // Store the options contained in the flags

    gfx_scanlinemode = flags & (GFX_SCANLINES | GFX_DOUBLESIZE);

//    if (flags & GFX_NOSWITCHING) gfx_preventswitch = 1;
//        else gfx_preventswitch = 0;
//    if (win_fullscreen) sdlflags |= SDL_FULLSCREEN;
//    if (flags & GFX_FULLSCREEN) sdlflags |= SDL_FULLSCREEN;
//    if (flags & GFX_WINDOW) sdlflags &= ~SDL_FULLSCREEN;
//    if (sdlflags & SDL_FULLSCREEN) gfx_fullscreen = 1;
//        else gfx_fullscreen = 0;

    // Calculate virtual window size

    gfx_virtualxsize = xsize;
    gfx_virtualxsize /= 16;
    gfx_virtualxsize *= 16;
    gfx_virtualysize = ysize;

    if ((!gfx_virtualxsize) || (!gfx_virtualysize))
    {
        gfx_initexec = 0;
        gfx_uninit();
        bme_error = BME_ILLEGAL_CONFIG;
        return BME_ERROR;
    }

    // Calculate actual window size (for scanline mode & doublesize mode
    // this is double the virtual)

    gfx_windowxsize = gfx_virtualxsize;
    gfx_windowysize = gfx_virtualysize;
    if (gfx_scanlinemode)
    {
        gfx_windowxsize <<= 1;
        gfx_windowysize <<= 1;
    }

    gfx_setclipregion(0, 0, gfx_virtualxsize, gfx_virtualysize);

    // Colors 0 & 255 are always black & white
    gfx_sdlpalette[0].r = 0;
    gfx_sdlpalette[0].g = 0;
    gfx_sdlpalette[0].b = 0;
    gfx_sdlpalette[255].r = 255;
    gfx_sdlpalette[255].g = 255;
    gfx_sdlpalette[255].b = 255;

//    gfx_screen = SDL_SetVideoMode(gfx_windowxsize, gfx_windowysize, 8, sdlflags);
    gfx_initexec = 0;
	
//    if (gfx_screen)
//    {
        gfx_initted = 1;
        gfx_redraw = 1;
//        gfx_setpalette();
//        win_setmousemode(win_mousemode);
        return BME_OK;
//    }
//    else
	
//	 return BME_ERROR;
}

int gfx_reinit(void)
{
    return gfx_init(gfx_last_xsize, gfx_last_ysize, gfx_last_framerate, gfx_last_flags);
}

void gfx_uninit(void)
{
    gfx_initted = 0;
    return;
}

int gfx_lock(void)
{
    if (gfx_locked) return 1;
    if (!gfx_initted) return 0;
//    if (!SDL_LockSurface(gfx_screen))
    {
        gfx_locked = 1;
        return 1;
    }
//    else return 0;
}

void gfx_unlock(void)
{
    if (gfx_locked)
    {
//        SDL_UnlockSurface(gfx_screen);
        gfx_locked = 0;
    }
}

void gfx_flip()
{
	LOGD("gfx_flip");
//    SDL_Flip(gfx_screen);
    gfx_redraw = 0;
}

void gfx_setmaxcolors(int num)
{
    gfx_maxcolors = num;
}

int gfx_loadpalette(char *name)
{
    int handle;

    handle = io_open(name);
    if (handle == -1)
    {
        bme_error = BME_OPEN_ERROR;
        return BME_ERROR;
    }
    if (io_read(handle, gfx_palette, sizeof gfx_palette) != sizeof gfx_palette)
    {
        bme_error = BME_READ_ERROR;
        io_close(handle);
        return BME_ERROR;
    }

    io_close(handle);
    gfx_calcpalette(64, 0, 0, 0);
    bme_error = BME_OK;
    return BME_OK;
}

void gfx_calcpalette(int fade, int radd, int gadd, int badd)
{
    Uint8  *sptr = &gfx_palette[3];
    int c, cl;
    if (radd < 0) radd = 0;
    if (gadd < 0) gadd = 0;
    if (badd < 0) badd = 0;

    for (c = 1; c < 255; c++)
    {
        cl = *sptr;
        cl *= fade;
        cl >>= 6;
        cl += radd;
        if (cl > 63) cl = 63;
        if (cl < 0) cl = 0;
        gfx_sdlpalette[c].r = (cl << 2) | (cl & 3);
        sptr++;

        cl = *sptr;
        cl *= fade;
        cl >>= 6;
        cl += gadd;
        if (cl > 63) cl = 63;
        if (cl < 0) cl = 0;
        gfx_sdlpalette[c].g = (cl << 2) | (cl & 3);
        sptr++;

        cl = *sptr;
        cl *= fade;
        cl >>= 6;
        cl += badd;
        if (cl > 63) cl = 63;
        if (cl < 0) cl = 0;
        gfx_sdlpalette[c].b = (cl << 2) | (cl & 3);
        sptr++;
    }
}

void gfx_setpalette(void)
{
    if (!gfx_initted) return;

	LOGD("TODO:     gfx_setpalette");
//    SDL_SetColors(gfx_screen, &gfx_sdlpalette[0], 0, gfx_maxcolors);
}

void gfx_setclipregion(unsigned left, unsigned top, unsigned right, unsigned bottom)
{
    if (left >= right) return;
    if (top >= bottom) return;
    if (left >= gfx_virtualxsize) return;
    if (top >= gfx_virtualysize) return;
    if (right > gfx_virtualxsize) return;
    if (bottom > gfx_virtualysize) return;

    gfx_clipleft = left;
    gfx_clipright = right;
    gfx_cliptop = top;
    gfx_clipbottom = bottom;
}

void gfx_setmaxspritefiles(int num)
{
    if (num <= 0) return;

    if (gfx_spriteheaders) return;

    gfx_spriteheaders = malloc(num * sizeof(Uint8 *));
    gfx_spritedata = malloc(num * sizeof(Uint8 *));
    gfx_spriteamount = malloc(num * sizeof(unsigned));
    if ((gfx_spriteheaders) && (gfx_spritedata) && (gfx_spriteamount))
    {
        int c;

        gfx_maxspritefiles = num;
        for (c = 0; c < num; c++)
        {
            gfx_spriteamount[c] = 0;
            gfx_spritedata[c] = NULL;
            gfx_spriteheaders[c] = NULL;
        }
    }
    else gfx_maxspritefiles = 0;
}

int gfx_loadsprites(int num, char *name)
{
    int handle, size, c;
    int datastart;

    if (!gfx_spriteheaders)
    {
        gfx_setmaxspritefiles(DEFAULT_MAX_SPRFILES);
    }

    bme_error = BME_OPEN_ERROR;
    if (num >= gfx_maxspritefiles) return BME_ERROR;

    gfx_freesprites(num);

    handle = io_open(name);
    if (handle == -1) return BME_ERROR;

    size = io_lseek(handle, 0, SEEK_END);
    io_lseek(handle, 0, SEEK_SET);

    gfx_spriteamount[num] = io_readle32(handle);

    gfx_spriteheaders[num] = malloc(gfx_spriteamount[num] * sizeof(SPRITEHEADER));

    if (!gfx_spriteheaders[num])
    {
        bme_error = BME_OUT_OF_MEMORY;    
        io_close(handle);
        return BME_ERROR;
    }

    for (c = 0; c < gfx_spriteamount[num]; c++)
    {
        SPRITEHEADER *hptr = gfx_spriteheaders[num] + c;

        hptr->xsize = io_readle16(handle);
        hptr->ysize = io_readle16(handle);
        hptr->xhot = io_readle16(handle);
        hptr->yhot = io_readle16(handle);
        hptr->offset = io_readle32(handle);
    }

    datastart = io_lseek(handle, 0, SEEK_CUR);
    gfx_spritedata[num] = malloc(size - datastart);
    if (!gfx_spritedata[num])
    {
        bme_error = BME_OUT_OF_MEMORY;    
        io_close(handle);
        return BME_ERROR;
    }
    io_read(handle, gfx_spritedata[num], size - datastart);
    io_close(handle);
    bme_error = BME_OK;
    return BME_OK;
}

void gfx_freesprites(int num)
{
    if (num >= gfx_maxspritefiles) return;

    if (gfx_spritedata[num])
    {
        free(gfx_spritedata[num]);
        gfx_spritedata[num] = NULL;
    }
    if (gfx_spriteheaders[num])
    {
        free(gfx_spriteheaders[num]);
        gfx_spriteheaders[num] = NULL;
    }
}

void gfx_copyscreen8(Uint8  *destaddress, Uint8  *srcaddress, unsigned pitch)
{
    int c, d;

    switch(gfx_scanlinemode)
    {
        default:
        for (c = 0; c < gfx_virtualysize; c++)
        {
            memcpy(destaddress, srcaddress, gfx_virtualxsize);
            destaddress += pitch;
            srcaddress += gfx_virtualxsize;
        }
        break;

        case GFX_SCANLINES:
        for (c = 0; c < gfx_virtualysize; c++)
        {
            d = gfx_virtualxsize;
            while (d--)
            {
                *destaddress = *srcaddress;
                destaddress++;
                *destaddress = *srcaddress;
                destaddress++;
                srcaddress++;
            }
            destaddress += pitch*2 - (gfx_virtualxsize << 1);
        }
        break;

        case GFX_DOUBLESIZE:
        for (c = 0; c < gfx_virtualysize; c++)
        {
            d = gfx_virtualxsize;
            while (d--)
            {
                *destaddress = *srcaddress;
                destaddress++;
                *destaddress = *srcaddress;
                destaddress++;
                srcaddress++;
            }
            destaddress += pitch - (gfx_virtualxsize << 1);
            srcaddress -= gfx_virtualxsize;
            d = gfx_virtualxsize;
            while (d--)
            {
                *destaddress = *srcaddress;
                destaddress++;
                *destaddress = *srcaddress;
                destaddress++;
                srcaddress++;
            }
            destaddress += pitch - (gfx_virtualxsize << 1);
        }
        break;
    }
}


void gfx_getspriteinfo(unsigned num)
{
    unsigned sprf = num >> 16;
    unsigned spr = (num & 0xffff) - 1;
    SPRITEHEADER *hptr;

    if ((sprf >= gfx_maxspritefiles) || (!gfx_spriteheaders[sprf]) ||
        (spr >= gfx_spriteamount[sprf])) hptr = NULL;
    else hptr = gfx_spriteheaders[sprf] + spr;

    if (!hptr)
    {
        spr_xsize = 0;
        spr_ysize = 0;
        spr_xhotspot = 0;
        spr_yhotspot = 0;
        return;
    }

    spr_xsize = hptr->xsize;
    spr_ysize = hptr->ysize;
    spr_xhotspot = hptr->xhot;
    spr_yhotspot = hptr->yhot;
}

void gfx_drawsprite(int x, int y, unsigned num)
{
	/*
    unsigned sprf = num >> 16;
    unsigned spr = (num & 0xffff) - 1;
    SPRITEHEADER *hptr;

    Uint8 *sptr;
    Uint8 *dptr;
    int cx;

    if (!gfx_initted) return;
    if (!gfx_locked) return;

    if ((sprf >= gfx_maxspritefiles) || (!gfx_spriteheaders[sprf]) ||
        (spr >= gfx_spriteamount[sprf]))
    {
        spr_xsize = 0;
        spr_ysize = 0;
        spr_xhotspot = 0;
        spr_yhotspot = 0;
        return;
    }
    else hptr = gfx_spriteheaders[sprf] + spr;

    sptr = gfx_spritedata[sprf] + hptr->offset;
    spr_xsize = hptr->xsize;
    spr_ysize = hptr->ysize;
    spr_xhotspot = hptr->xhot;
    spr_yhotspot = hptr->yhot;

    x -= spr_xhotspot;
    y -= spr_yhotspot;

    if (x >= gfx_clipright) return;
    if (y >= gfx_clipbottom) return;
    if (x + spr_xsize <= gfx_clipleft) return;
    if (y + spr_ysize <= gfx_cliptop) return;

    while (y < gfx_cliptop)
    {
        int dec = *sptr++;
        if (dec == 255)
        {
            if (!(*sptr)) return;
            y++;
        }
        else
        {
            if (dec < 128)
            {
                sptr += dec;
            }
        }
    }
    while (y < gfx_clipbottom)
    {
        int dec;
        cx = x;
        dptr = gfx_screen->pixels + y * gfx_screen->pitch + x;

        for (;;)
        {
            dec = *sptr++;

            if (dec == 255)
            {
                if (!(*sptr)) return;
                y++;
                break;
            }
            if (dec < 128)
            {
                if ((cx + dec <= gfx_clipleft) || (cx >= gfx_clipright))
                {
                    goto SKIP;
                }
                if (cx < gfx_clipleft)
                {
                    dec -= (gfx_clipleft - cx);
                    sptr += (gfx_clipleft - cx);
                    dptr += (gfx_clipleft - cx);
                    cx = gfx_clipleft;
                }
                while ((cx < gfx_clipright) && (dec))
                {
                    *dptr = *sptr;
                    cx++;
                    sptr++;
                    dptr++;
                    dec--;
                }
                SKIP:
                cx += dec;
                sptr += dec;
                dptr += dec;
            }
            else
            {
                cx += (dec & 0x7f);
                dptr += (dec & 0x7f);
            }
        }
    }
	*/
}


