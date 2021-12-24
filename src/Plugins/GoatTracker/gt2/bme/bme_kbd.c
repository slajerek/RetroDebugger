//
// BME (Blasphemous Multimedia Engine) keyboard module
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "bme_main.h"
#include "bme_cfg.h"
#include "bme_win.h"
#include "bme_io.h"
#include "bme_err.h"

int kbd_init(void);
void kbd_uninit(void);
int kbd_waitkey(void);
int kbd_getkey(void);
int kbd_checkkey(int rawcode);
char *kbd_getkeyname(int rawcode);

static KEY keyname[] = {{KEY_BACKSPACE, "BACKSPACE"},
    {KEY_CAPSLOCK, "CAPSLOCK"},
    {KEY_ENTER, "ENTER"},
    {KEY_ESC, "ESC"},
    {KEY_NUMLOCK, "NUMLOCK"},
    {KEY_LEFTSHIFT, "SHIFT"},
    {KEY_RIGHTSHIFT, "SHIFT"},
    {KEY_SCROLLLOCK, "SCR. LOCK"},
    {KEY_SPACE, "SPACE"},
    {KEY_TAB, "TAB"},
    {KEY_F1, "F1"},
    {KEY_F2, "F2"},
    {KEY_F3, "F3"},
    {KEY_F4, "F4"},
    {KEY_F5, "F5"},
    {KEY_F6, "F6"},
    {KEY_F7, "F7"},
    {KEY_F8, "F8"},
    {KEY_F9, "F9"},
    {KEY_F10, "F10"},
    {KEY_F11, "F11"},
    {KEY_F12, "F12"},
    {KEY_A, "A"},
    {KEY_N, "N"},
    {KEY_B, "B"},
    {KEY_O, "O"},
    {KEY_C, "C"},
    {KEY_P, "P"},
    {KEY_D, "D"},
    {KEY_Q, "Q"},
    {KEY_E, "E"},
    {KEY_R, "R"},
    {KEY_F, "F"},
    {KEY_S, "S"},
    {KEY_G, "G"},
    {KEY_T, "T"},
    {KEY_H, "H"},
    {KEY_U, "U"},
    {KEY_I, "I"},
    {KEY_V, "V"},
    {KEY_J, "J"},
    {KEY_W, "W"},
    {KEY_K, "K"},
    {KEY_X, "X"},
    {KEY_L, "L"},
    {KEY_Y, "Y"},
    {KEY_M, "M"},
    {KEY_Z, "Z"},
    {KEY_1, "1"},
    {KEY_2, "2"},
    {KEY_3, "3"},
    {KEY_4, "4"},
    {KEY_5, "5"},
    {KEY_6, "6"},
    {KEY_7, "7"},
    {KEY_8, "8"},
    {KEY_9, "9"},
    {KEY_0, "0"},
    {KEY_MINUS, "-"},
    {KEY_EQUAL, "="},
    {KEY_BRACKETL, "["},
    {KEY_BRACKETR, "]"},
    {KEY_SEMICOLON, ";"},
    {KEY_APOST1, "'"},
    {KEY_APOST2, "`"},
    {KEY_COMMA, ","},
    {KEY_COLON, "."},
    {KEY_SLASH, "/"},
    {KEY_BACKSLASH, "\\"},
    {KEY_ALT, "ALT"},
    {KEY_CTRL, "CTRL"},
    {KEY_DEL, "DELETE"},
    {KEY_DOWN, "CRS. DOWN"},
    {KEY_END, "END"},
    {KEY_HOME, "HOME"},
    {KEY_INS, "INSERT"},
    {KEY_LEFT, "CRS. LEFT"},
    {KEY_PGDN, "PAGE DOWN"},
    {KEY_PGUP, "PAGE UP"},
    {KEY_RIGHT, "CRS. RIGHT"},
    {KEY_UP, "CRS. UP"},
    {KEY_WINDOWSL, "WINDOWS KEY"},
    {KEY_WINDOWSR, "WINDOWS KEY"},
    {KEY_MENU, "MENU KEY"},
    {0xff, "?"}};

int kbd_init(void)
{
    return BME_OK;
}

void kbd_uninit(void)
{
}

int kbd_waitkey(void)
{
	LOGD("kbd_waitkey");
	
    int index;

    win_asciikey = 0;
    for (;;)
    {
        win_checkmessages();

        for (index = 0; index < MAX_KEYS; index++)
        {
            if (win_keytable[index])
            {
                win_keytable[index] = 0;
                return index;
            }
        }

        SDL_Delay(15);
    }
}

int kbd_getkey(void)
{
    int index;

    for (index = 0; index < MAX_KEYS; index++)
    {
        if (win_keytable[index])
        {
            win_keytable[index] = 0;
            return index;
        }
    }
    return 0;
}

int kbd_checkkey(int rawcode)
{
    if (rawcode >= MAX_KEYS) return 0;
    if (win_keytable[rawcode])
    {
        win_keytable[rawcode] = 0;
        return 1;
    }
    return 0;
}

char *kbd_getkeyname(int rawcode)
{
    KEY *ptr = &keyname[0];

    while (ptr->rawcode != 255)
    {
        if (ptr->rawcode == rawcode)
        {
            return ptr->name;
        }
        ptr++;
    }
    return ptr->name;
}


