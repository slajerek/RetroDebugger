// BME graphics module header file
#include "bme_cfg.h"

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

extern int gfx_initted;
extern int gfx_scanlinemode;
extern int gfx_preventswitch;
extern int gfx_fullscreen;
extern int gfx_redraw;
extern unsigned gfx_windowxsize;
extern unsigned gfx_windowysize;
extern unsigned gfx_virtualxsize;
extern unsigned gfx_virtualysize;
extern unsigned gfx_nblocks;
extern int gfx_blockxsize;
extern int gfx_blockysize;
extern int spr_xsize;
extern int spr_ysize;
extern int spr_xhotspot;
extern int spr_yhotspot;
extern Uint8 *gfx_vscreen;
extern Uint8 *gfx_blocks;
extern Uint8 gfx_palette[];
extern SDL_Surface *gfx_screen;

extern SDL_Color gfx_sdlpalette[MAX_COLORS];

