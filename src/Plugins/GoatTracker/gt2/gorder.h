#ifndef GORDER_H
#define GORDER_H

#ifndef GORDER_C
extern int espos[MAX_CHN];
extern int esend[MAX_CHN];
extern int eseditpos;
extern int esview;
extern int escolumn;
extern int eschn;
extern int esnum;
extern int esmarkchn;
extern int esmarkstart;
extern int esmarkend;
extern int enpos;
#endif

void updateviewtopos(void);
void orderlistcommands(void);
void namecommands(void);
void nextsong(void);
void prevsong(void);
void songchange(void);
void orderleft(void);
void orderright(void);
void deleteorder(void);
void insertorder(unsigned char value);

#endif
