#ifndef GSONG_H
#define GSONG_H

#ifndef GSONG_C
extern INSTR ginstr[MAX_INSTR];
extern unsigned char ltable[MAX_TABLES][MAX_TABLELEN];
extern unsigned char rtable[MAX_TABLES][MAX_TABLELEN];
extern unsigned char songorder[MAX_SONGS][MAX_CHN][MAX_SONGLEN+2];
extern unsigned char pattern[MAX_PATT][MAX_PATTROWS*4+4];
extern char songname[MAX_STR];
extern char authorname[MAX_STR];
extern char copyrightname[MAX_STR];
extern int pattlen[MAX_PATT];
extern int songlen[MAX_SONGS][MAX_CHN];
extern int highestusedpattern;
extern int highestusedinstr;
#endif

void loadsong(void);
void mergesong(void);
void loadinstrument(void);
int savesong(void);
int saveinstrument(void);
void clearsong(int cs, int cp, int ci, int cf, int cn);
void countpatternlengths(void);
void countthispattern(void);
void clearpattern(int p);
int insertpattern(int p);
void deletepattern(int p);
void findusedpatterns(void);
void findduplicatepatterns(void);
void optimizeeverything(int oi, int ot);

#endif
