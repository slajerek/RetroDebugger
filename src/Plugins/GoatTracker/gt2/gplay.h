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
  unsigned char arpcolnotes[MAX_ARP_COLS]; // Per-column active note (0=off)
  unsigned char arpnotes[MAX_ARP_COLS+1];  // Active note set for cycling (including base)
  unsigned char arpcount;                   // Number of active notes in arpnotes[]
  unsigned char arppos;                     // Current position in arp cycle
  unsigned char arpbase;                    // 1 when arpnotes[0] is the base note
} CHN;

#ifndef GPLAY_C
extern CHN chn[MAX_CHN];
extern unsigned char masterfader;
extern unsigned char freqtbllo[];
extern unsigned char freqtblhi[];
extern int lastsonginit;
extern int songinit;
extern int gt2LoopCurrentPattern;
#endif

void rebuildarp(CHN *cptr);
void initchannels(void);
void initsong(int num, int playmode);
void initsongpos(int num, int playmode, int pattpos);
void triggerpatternrow(int pattpos);
void stopsong(void);
void rewindsong(void);
void playtestnote(int note, int ins, int chnnum);

/* Last note passed to playtestnote() and the channel it went to; note is 0 if
   nothing has been auditioned yet. */
extern int gt2LastPreviewNote;
extern int gt2LastPreviewChannel;

/* playroutine() refuses to play a song whose current instrument has a
   gatetimer longer than the tick interval, and stock GT2 stops dead without
   a word -- play just parks on row 0 in silence. These carry the reason out
   to the editor, which reports it once and clears the flag. */
extern volatile int gt2GatetimerStopPending;
extern volatile int gt2GatetimerStopGatetimer;
extern volatile int gt2GatetimerStopTick;
extern volatile int gt2GatetimerStopInstr;

/* Largest gatetimer that guard will accept at the current song tempo. */
int gt2MaxSafeGatetimer(void);

/* Non-zero makes loadinstrument() skip its stopsong(), so an instrument can be
   auditioned without cutting the song off. Only the ImGui-side loaders set it;
   every native GT2 path leaves it at 0. */
extern int gt2KeepPlayingOnInstrumentLoad;
void releasenote(int chnnum);
void mutechannel(int chnnum);
int isplaying(void);
void playroutine(void);

#endif
