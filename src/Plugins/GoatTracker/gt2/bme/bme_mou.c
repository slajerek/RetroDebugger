//
// BME (Blasphemous Multimedia Engine) mouse module
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

#include "bme_main.h"
#include "bme_cfg.h"
#include "bme_win.h"
#include "bme_gfx.h"
#include "bme_io.h"
#include "bme_err.h"

void mou_init(void);
void mou_uninit(void);
void mou_getpos(unsigned *x, unsigned *y);
//void mou_getmove(int *dx, int *dy);
unsigned mou_getbuttons(void);

void mou_init(void)
{
    win_mousebuttons = 0;
}

void mou_uninit(void)
{
}

void mou_getpos(unsigned *x, unsigned *y)
{
    if (!gfx_initted)
    {
        *x = win_mousexpos;
        *y = win_mouseypos;
    }
    else
    {
        *x = win_mousexpos * gfx_virtualxsize / gfx_windowxsize;
        *y = win_mouseypos * gfx_virtualysize / gfx_windowysize;
    }
}

//void mou_getmove(int *dx, int *dy)
//{
//    *dx = win_mousexrel;
//    *dy = win_mouseyrel;
//    win_mousexrel = 0;
//    win_mouseyrel = 0;
//
//}

unsigned mou_getbuttons(void)
{
    return win_mousebuttons;
}

