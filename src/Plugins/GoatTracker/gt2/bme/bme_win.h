// BME windows, input & timing module header file

int win_openwindow(char *appname, char *icon);
void win_closewindow(void);
void win_messagebox(char *string);
void win_checkmessages(void);
int win_getspeed(int framerate);
void win_setmousemode(int mode);

extern int win_windowinitted;
extern int win_quitted;
extern int win_fullscreen;
extern unsigned char win_keytable[MAX_KEYS];
extern unsigned char win_keystate[MAX_KEYS];
extern unsigned char win_asciikey;
extern volatile unsigned int win_mousexpos;
extern volatile unsigned int win_mouseypos;
//extern unsigned win_mousexrel;
//extern unsigned win_mouseyrel;
extern volatile unsigned int win_mousebuttons;
extern int win_mousemode;
//SDL_Joystick *gtjoy[MAX_JOYSTICKS];
//extern Sint16 joyx[MAX_JOYSTICKS];
//extern Sint16 joyy[MAX_JOYSTICKS];
//extern Uint32 joybuttons[MAX_JOYSTICKS];

void gt2SetMousePosition(unsigned int x, unsigned int y);
