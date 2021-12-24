#ifndef GCOMMON_H
#define GCOMMON_H

#define CMD_DONOTHING 0
#define CMD_PORTAUP 1
#define CMD_PORTADOWN 2
#define CMD_TONEPORTA 3
#define CMD_VIBRATO 4
#define CMD_SETAD 5
#define CMD_SETSR 6
#define CMD_SETWAVE 7
#define CMD_SETWAVEPTR 8
#define CMD_SETPULSEPTR 9
#define CMD_SETFILTERPTR 10
#define CMD_SETFILTERCTRL 11
#define CMD_SETFILTERCUTOFF 12
#define CMD_SETMASTERVOL 13
#define CMD_FUNKTEMPO 14
#define CMD_SETTEMPO 15

#define WTBL 0
#define PTBL 1
#define FTBL 2
#define STBL 3

#define MAX_FILT 64
#define MAX_STR 32
#define MAX_INSTR 64
#define MAX_CHN 3
#define MAX_PATT 208
#define MAX_TABLES 4
#define MAX_TABLELEN 255
#define MAX_INSTRNAMELEN 16
#define MAX_PATTROWS 128
#define MAX_SONGLEN 254
#define MAX_SONGS 32
#define MAX_NOTES 96

#define REPEAT 0xd0
#define TRANSDOWN 0xe0
#define TRANSUP 0xf0
#define LOOPSONG 0xff

#define ENDPATT 0xff
#define INSTRCHG 0x00
#define FX 0x40
#define FXONLY 0x50
#define FIRSTNOTE 0x60
#define LASTNOTE 0xbc
#define REST 0xbd
#define KEYOFF 0xbe
#define KEYON 0xbf
#define OLDKEYOFF 0x5e
#define OLDREST 0x5f

#define WAVEDELAY 0x1
#define WAVELASTDELAY 0xf
#define WAVESILENT 0xe0
#define WAVELASTSILENT 0xef
#define WAVECMD 0xf0
#define WAVELASTCMD 0xfe

typedef struct
{
  unsigned char ad;
  unsigned char sr;
  unsigned char ptr[MAX_TABLES];
  unsigned char vibdelay;
  unsigned char gatetimer;
  unsigned char firstwave;
  char name[MAX_INSTRNAMELEN];
} INSTR;

#endif

