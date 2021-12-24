#ifndef GDISPLAY_H
#define GDISPLAY_H

#define CNORMAL 8
#define CMUTE 3
#define CEDIT 10
#define CPLAYING 12
#define CCOMMAND 7
#define CTITLE 15

void printmainscreen(void);
void displayupdate(void);
void printstatus(void);
void resettime(void);
void incrementtime(void);

#endif
