#ifndef GINSTR_H
#define GINSTR_H

#ifndef GINSTR_C
extern int einum;
extern int eipos;
extern int eicolumn;
extern INSTR instrcopybuffer;
#endif

void instrumentcommands(void);
void nextinstr(void);
void previnstr(void);
void clearinstr(int num);
void gotoinstr(int i);
void showinstrtable(void);

#endif
