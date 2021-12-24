#ifndef GPLAY_H
#define GPLAY_H

#define PLAY_PLAYING 0x00
#define PLAY_BEGINNING 0x01
#define PLAY_POS 0x02
#define PLAY_PATTERN 0x03
#define PLAY_STOP 0x04
#define PLAY_STOPPED 0x80

typedef struct
{
  unsigned char trans;
  unsigned char instr;
  unsigned char note;
  unsigned char lastnote;
  unsigned char newnote;
  unsigned pattptr;
  unsigned char pattnum;
  unsigned char songptr;
  unsigned char repeat;
  unsigned short freq;
  unsigned char gate;
  unsigned char wave;
  unsigned short pulse;
  unsigned char ptr[2];
  unsigned char pulsetime;
  unsigned char wavetime;
  unsigned char vibtime;
  unsigned char vibdelay;
  unsigned char command;
  unsigned char cmddata;
  unsigned char newcommand;
  unsigned char newcmddata;
  unsigned char tick;
  unsigned char tempo;
  unsigned char mute;
  unsigned char advance;
  unsigned char gatetimer;
} CHN;

#ifndef GPLAY_C
extern CHN chn[MAX_CHN];
extern unsigned char masterfader;
extern unsigned char freqtbllo[];
extern unsigned char freqtblhi[];
extern int lastsonginit;
#endif

void initchannels(void);
void initsong(int num, int playmode);
void initsongpos(int num, int playmode, int pattpos);
void stopsong(void);
void rewindsong(void);
void playtestnote(int note, int ins, int chnnum);
void releasenote(int chnnum);
void mutechannel(int chnnum);
int isplaying(void);
void playroutine(void);

#endif
