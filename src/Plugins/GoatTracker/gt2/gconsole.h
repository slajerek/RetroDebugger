#ifndef GCONSOLE_H
#define GCONSOLE_H

#define MAX_COLUMNS 100
#define MAX_ROWS 37
#define HOLDDELAY 24
#define DOUBLECLICKDELAY 15
#define MOUSESIZEX 11
#define MOUSESIZEY 20

int initscreen(void);
void closescreen(void);
void clearscreen(void);
void fliptoscreen(void);
void printtext(int x, int y, int color, const char *text);
void printtextc(int y, int color, const char *text);
void printtextcp(int cp, int y, int color, const char *text);
void printblank(int x, int y, int length);
void printblankc(int x, int y, int color, int length);
void drawbox(int x, int y, int color, int sx, int sy);
void printbg(int x, int y, int color, int length);
void getkey(void);

#ifndef GCONSOLE_C
extern int key, rawkey, shiftpressed, cursorflashdelay, altpressed;
extern int mouseb, prevmouseb;
extern int mouseheld;
extern int mousex, mousey;
#endif

#endif
