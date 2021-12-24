// BME main definitions header file
#include <SDL.h>

#define GFX_SCANLINES 1
#define GFX_DOUBLESIZE 2
#define GFX_USE1PAGE 0
#define GFX_USE2PAGES 4
#define GFX_USE3PAGES 8
#define GFX_WAITVBLANK 16
#define GFX_FULLSCREEN 32
#define GFX_WINDOW 64
#define GFX_NOSWITCHING 128
#define GFX_USEDIBSECTION 256

#define MOUSE_ALWAYS_VISIBLE 0
#define MOUSE_FULLSCREEN_HIDDEN 1
#define MOUSE_ALWAYS_HIDDEN 2

#define MOUSEB_LEFT 1
#define MOUSEB_RIGHT 2
#define MOUSEB_MIDDLE 4

#define JOY_LEFT 1
#define JOY_RIGHT 2
#define JOY_UP 4
#define JOY_DOWN 8
#define JOY_FIRE1 16
#define JOY_FIRE2 32
#define JOY_FIRE3 64
#define JOY_FIRE4 128

#define LEFT 0
#define MIDDLE 128
#define RIGHT 255

#define B_OFF 0
#define B_SOLID 1
#define B_NOTSOLID 2

#define MONO 0
#define STEREO 1
#define EIGHTBIT 0
#define SIXTEENBIT 2

#define VM_OFF 0
#define VM_ON 1
#define VM_ONESHOT 0
#define VM_LOOP 2
#define VM_16BIT 4

enum GoatTrackerKey
{
       //   0
       KEY_BACKSPACE = SDLK_BACKSPACE,
       KEY_CAPSLOCK = 0x100,
       KEY_ENTER = SDLK_RETURN,
       KEY_ESC = SDLK_ESCAPE,
       KEY_ALT = 0x101,
       KEY_CTRL = 0x102,
       KEY_LEFTCTRL = 0x103,
       KEY_RIGHTALT = 0x104,
       KEY_RIGHTCTRL = 0x105,
       KEY_LEFTSHIFT = 0x106, //  10
       KEY_RIGHTSHIFT = 0x107,
       KEY_NUMLOCK = 0x108,
       KEY_SCROLLLOCK = 0x109,
       KEY_SPACE = SDLK_SPACE,
       KEY_TAB = SDLK_TAB,
       KEY_F1 = 0x10A,
       KEY_F2 = 0x10B,
       KEY_F3 = 0x10C,
       KEY_F4 = 0x10D,
       KEY_F5 = 0x10E, //  20
       KEY_F6 = 0x10F,
       KEY_F7 = 0x110,
       KEY_F8 = 0x111,
       KEY_F9 = 0x112,
       KEY_F10 = 0x113,
       KEY_F11 = 0x114,
       KEY_F12 = 0x115,
       KEY_A = SDLK_a,
       KEY_N = SDLK_n,
       KEY_B = SDLK_b, //  30
       KEY_O = SDLK_o,
       KEY_C = SDLK_c,
       KEY_P = SDLK_p,
       KEY_D = SDLK_d,
       KEY_Q = SDLK_q,
       KEY_E = SDLK_e,
       KEY_R = SDLK_r,
       KEY_F = SDLK_f,
       KEY_S = SDLK_s,
       KEY_G = SDLK_g, //  40
       KEY_T = SDLK_t,
       KEY_H = SDLK_h,
       KEY_U = SDLK_u,
       KEY_I = SDLK_i,
       KEY_V = SDLK_v,
       KEY_J = SDLK_j,
       KEY_W = SDLK_w,
       KEY_K = SDLK_k,
       KEY_X = SDLK_x,
       KEY_L = SDLK_l, //  50
       KEY_Y = SDLK_y,
       KEY_M = SDLK_m,
       KEY_Z = SDLK_z,
       KEY_1 = SDLK_1,
       KEY_2 = SDLK_2,
       KEY_3 = SDLK_3,
       KEY_4 = SDLK_4,
       KEY_5 = SDLK_5,
       KEY_6 = SDLK_6,
       KEY_7 = SDLK_7, //  60
       KEY_8 = SDLK_8,
       KEY_9 = SDLK_9,
       KEY_0 = SDLK_0,
       KEY_MINUS = SDLK_MINUS,
       KEY_EQUAL = SDLK_EQUALS,
       KEY_BRACKETL = SDLK_LEFTBRACKET,
       KEY_BRACKETR = SDLK_RIGHTBRACKET,
       KEY_SEMICOLON = SDLK_SEMICOLON,
       KEY_APOST1 = SDLK_QUOTE,
       KEY_APOST2 = SDLK_BACKQUOTE, //  70
       KEY_COMMA = SDLK_COMMA,
       KEY_COLON = SDLK_PERIOD,
       KEY_PERIOD = SDLK_PERIOD,
       KEY_SLASH = SDLK_SLASH,
       KEY_BACKSLASH = SDLK_BACKSLASH,
       KEY_DEL = SDLK_DELETE,
       KEY_DOWN = 0x116,
       KEY_END = 0x117,
       KEY_HOME = 0x118,
       KEY_INS = 0x119, //  80
       KEY_LEFT = 0x11A,
       KEY_PGDN = 0x11B,
       KEY_PGUP = 0x11C,
       KEY_RIGHT = 0x11D,
       KEY_UP = 0x11E,
       KEY_WINDOWSL = 0x11F,
       KEY_WINDOWSR = 0x120,
       KEY_MENU = 0x121,
       KEY_PAUSE = 0x122,
       KEY_KPDIVIDE = 0x123, //  90
       KEY_KPMULTIPLY = 0x124,
       KEY_KPPLUS = 0x125,
       KEY_KPMINUS = 0x126,
       KEY_KP0 = 0x127,
       KEY_KP1 = 0x128,
       KEY_KP2 = 0x129,
       KEY_KP3 = 0x12A,
       KEY_KP4 = 0x12B,
       KEY_KP5 = 0x12C,
       KEY_KP6 = 0x12D, // 100
       KEY_KP7 = 0x12E,
       KEY_KP8 = 0x12F,
       KEY_KP9 = 0x130,
       KEY_KPUP = 0x131,
       KEY_KPDOWN = 0x132,
       KEY_KPLEFT = 0x133,
       KEY_KPRIGHT = 0x134,
       KEY_KPENTER = 0x135,
       KEY_KPEQUALS = 0x136,
       KEY_KPPERIOD = 0x137
       // 110
};


extern unsigned int gtSdlKeyMapping[110];


typedef struct
{
	Sint8 *start;
	Sint8 *repeat;
	Sint8 *end;
	unsigned char voicemode;
} SAMPLE;

typedef struct
{
	volatile Sint8 *pos;
	Sint8 *repeat;
	Sint8 *end;
	SAMPLE *smp;
	unsigned freq;
	volatile unsigned fractpos;
	int vol;
	int mastervol;
	unsigned panning;
	volatile unsigned voicemode;
} CHANNEL;

typedef struct
{
  unsigned rawcode;
  char *name;
} KEY;

typedef struct
{
  Sint16 xsize;
  Sint16 ysize;
  Sint16 xhot;
  Sint16 yhot;
  Uint32 offset;
} SPRITEHEADER;

typedef struct
{
  Uint32 type;
  Uint32 offset;
} BLOCKHEADER;

typedef struct
{
  Uint8 blocksname[13];
  Uint8 palettename[13];
} MAPHEADER;

typedef struct
{
  Sint32 xsize;
  Sint32 ysize;
  Uint8 xdivisor;
  Uint8 ydivisor;
  Uint8 xwrap;
  Uint8 ywrap;
} LAYERHEADER;

extern int bme_error;
