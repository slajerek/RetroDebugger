//
// BME (Blasphemous Multimedia Engine) main module
//
#include <SDL.h>
#include "bme_err.h"
#include "bme_cfg.h"
#include "bme_main.h"

int bme_error = BME_OK;

unsigned int gtSdlKeyMapping[110] =
{
       //  0
       SDLK_BACKSPACE, SDLK_CAPSLOCK, SDLK_RETURN, SDLK_ESCAPE, SDLK_LALT, SDLK_LCTRL, SDLK_LCTRL, SDLK_RALT, SDLK_RCTRL, SDLK_LSHIFT,
       // 10
       SDLK_RSHIFT, SDL_SCANCODE_NUMLOCKCLEAR, SDLK_SCROLLLOCK, SDLK_SPACE, SDLK_TAB, SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5,
       // 20
       SDLK_F6, SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12, SDLK_a, SDLK_n, SDLK_b,
       // 30
       SDLK_o, SDLK_c, SDLK_p, SDLK_d, SDLK_q, SDLK_e, SDLK_r, SDLK_f, SDLK_s, SDLK_g,
       // 40
       SDLK_t, SDLK_h, SDLK_u, SDLK_i, SDLK_v, SDLK_j, SDLK_w, SDLK_k, SDLK_x, SDLK_l,
       // 50
       SDLK_y, SDLK_m, SDLK_z, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7,
       // 60
       SDLK_8, SDLK_9, SDLK_0, SDLK_MINUS, SDLK_EQUALS, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET, SDLK_SEMICOLON, SDLK_QUOTE, SDLK_BACKQUOTE,
       // 70
       SDLK_COMMA, SDLK_PERIOD, SDLK_PERIOD, SDLK_SLASH, SDLK_BACKSLASH, SDLK_DELETE, SDLK_DOWN, SDLK_END, SDLK_HOME, SDLK_INSERT,
       // 80
       SDLK_LEFT, SDLK_PAGEDOWN, SDLK_PAGEUP, SDLK_RIGHT, SDLK_UP,     SDLK_LGUI, SDLK_RGUI, SDLK_MENU, SDLK_PAUSE, SDLK_KP_DIVIDE,
       // 90
       SDLK_KP_MULTIPLY, SDLK_KP_PLUS, SDLK_KP_MINUS, SDLK_KP_0, SDLK_KP_1, SDLK_KP_2, SDLK_KP_3, SDLK_KP_4, SDLK_KP_5, SDLK_KP_6,
       // 100
       SDLK_KP_7, SDLK_KP_8, SDLK_KP_9, SDLK_KP_8, SDLK_KP_2, SDLK_KP_4, SDLK_KP_6, SDLK_KP_ENTER, SDLK_KP_EQUALS, SDLK_KP_PERIOD
       // 110
};

unsigned int gtBmeKeyMapping[110] =
{
       //   0
       KEY_BACKSPACE, KEY_CAPSLOCK, KEY_ENTER, KEY_ESC, KEY_ALT, KEY_CTRL, KEY_LEFTCTRL, KEY_RIGHTALT, KEY_RIGHTCTRL, KEY_LEFTSHIFT,
       //  10
       KEY_RIGHTSHIFT, KEY_NUMLOCK,
       KEY_SCROLLLOCK,
       KEY_SPACE,
       KEY_TAB,
       KEY_F1,
       KEY_F2,
       KEY_F3,
       KEY_F4,
       KEY_F5,
       //  20
       KEY_F6,
       KEY_F7,
       KEY_F8,
       KEY_F9,
       KEY_F10,
       KEY_F11,
       KEY_F12,
       KEY_A,
       KEY_N,
       KEY_B,
       //  30
       KEY_O,
       KEY_C,
       KEY_P,
       KEY_D,
       KEY_Q,
       KEY_E,
       KEY_R,
       KEY_F,
       KEY_S,
       KEY_G,
       //  40
       KEY_T,
       KEY_H,
       KEY_U,
       KEY_I,
       KEY_V,
       KEY_J,
       KEY_W,
       KEY_K,
       KEY_X,
       KEY_L,
       //  50
       KEY_Y,
       KEY_M,
       KEY_Z,
       KEY_1,
       KEY_2,
       KEY_3,
       KEY_4,
       KEY_5,
       KEY_6,
       KEY_7,
       //  60
       KEY_8,
       KEY_9,
       KEY_0,
       KEY_MINUS,
       KEY_EQUAL,
       KEY_BRACKETL,
       KEY_BRACKETR,
       KEY_SEMICOLON,
       KEY_APOST1,
       KEY_APOST2,
       //  70
       KEY_COMMA,
       KEY_COLON,
       KEY_PERIOD,
       KEY_SLASH,
       KEY_BACKSLASH,
       KEY_DEL,
       KEY_DOWN,
       KEY_END,
       KEY_HOME,
       KEY_INS,
       //  80
       KEY_LEFT,
       KEY_PGDN,
       KEY_PGUP,
       KEY_RIGHT,
       KEY_UP,
       KEY_WINDOWSL,
       KEY_WINDOWSR,
       KEY_MENU,
       KEY_PAUSE,
       KEY_KPDIVIDE,
       //  90
       KEY_KPMULTIPLY,
       KEY_KPPLUS,
       KEY_KPMINUS,
       KEY_KP0,
       KEY_KP1,
       KEY_KP2,
       KEY_KP3,
       KEY_KP4,
       KEY_KP5,
       KEY_KP6,
       // 100
       KEY_KP7,
       KEY_KP8,
       KEY_KP9,
       KEY_KPUP,
       KEY_KPDOWN,
       KEY_KPLEFT,
       KEY_KPRIGHT,
       KEY_KPENTER,
       KEY_KPEQUALS,
       KEY_KPPERIOD
       // 110
};


unsigned int mapSdlKeyToBmeKey(unsigned int sdlKey)
{
	LOGD("mapSdlKeyToBmeKey sdlKey=%d", sdlKey);
	for (int i = 0; i < 110; i++)
	{
		if (sdlKey == gtSdlKeyMapping[i])
		{
			LOGD("...found i=%d bme=%d", i, gtBmeKeyMapping[i]);
			return gtBmeKeyMapping[i];
		}
	}
	
	return -1;
}
