#ifndef GTABLE_H
#define GTABLE_H

#define MST_NOFINEVIB 0
#define MST_FINEVIB 1
#define MST_FUNKTEMPO 2
#define MST_PORTAMENTO 3
#define MST_RAW 4

#ifndef GTABLE_C
extern int etview[MAX_TABLES];
extern int etnum;
extern int etpos;
extern int etcolumn;
extern int etlock;
extern int etmarknum;
extern int etmarkstart;
extern int etmarkend;
#endif

void tablecommands(void);
void tableup(void);
void tabledown(void);
void inserttable(int num, int pos, int mode);
void deletetable(int num, int pos);
int makespeedtable(unsigned data, int mode, int makenew);
void optimizetable(int num);
void deleteinstrtable(int i);
int gettablelen(int num);
int gettablepartlen(int num, int pos);
void gototable(int num, int pos);
void settableview(int num, int pos);
void settableviewfirst(int num, int pos);
void validatetableview(void);
void exectable(int num, int ptr);
int findfreespeedtable(void);

#endif
