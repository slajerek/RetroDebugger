//
// GOATTRACKER v2 packer/relocator
//

#define GRELOC_C

#include "goattrk2.h"
#include "gt-membuf.h"
#include "gt-parse.h"

char *playeroptname[] =
{
  "Buffered SID-writes",
  "Sound effect support",
  "Volume change support",
  "Store author-info",
  "Use zeropage ghostregs",
  "Disable optimization",
  "Full SID buffering"
};

char *tableleftname[] = {
  "mt_wavetbl",
  "mt_pulsetimetbl",
  "mt_filttimetbl",
  "mt_speedlefttbl"};

char *tablerightname[] = {
  "mt_notetbl",
  "mt_pulsespdtbl",
  "mt_filtspdtbl",
  "mt_speedrighttbl"};

unsigned char chnused[MAX_CHN];
unsigned char pattused[MAX_PATT];
unsigned char pattmap[MAX_PATT];
unsigned char instrused[MAX_INSTR];
unsigned char instrmap[MAX_INSTR];
unsigned char tableused[MAX_TABLES][MAX_TABLELEN+1];
unsigned char tablemap[MAX_TABLES][MAX_TABLELEN+1];
int pattoffset[MAX_PATT];
int pattsize[MAX_PATT];
int songoffset[MAX_SONGS][MAX_CHN];
int songsize[MAX_SONGS][MAX_CHN];
int tableerror;
int channels;
int fixedparams;
int simplepulse;
int firstnote;
int lastnote;
int patternlastnote;
int gnofilter;
int nofiltermod;
int nopulse;
int nopulsemod;
int nowavedelay;
int norepeat;
int notrans;
int noportamento;
int notoneporta;
int novib;
int noinsvib;
int nosetad;
int nosetsr;
int nosetwave;
int nosetwaveptr;
int nosetpulseptr;
int nosetfiltptr;
int nosetfiltcutoff;
int nosetfiltctrl;
int nosetmastervol;
int nofunktempo;
int noglobaltempo;
int nochanneltempo;
int nogate;
int noeffects;
int nowavecmd;
int nofirstwavecmd;
int nocalculatedspeed;
int nonormalspeed;
int nozerospeed;

struct membuf src = STATIC_MEMBUF_INIT;
struct membuf dest = STATIC_MEMBUF_INIT;

#ifdef GT2RELOC
#ifdef __WIN32__
extern FILE *STDOUT, *STDERR;
#else
#define STDOUT stdout
#define STDERR stderr
#endif
extern char packedsongname[MAX_PATHNAME];
#define clearscreen()
#define fliptoscreen()
#define waitkeynoupdate()
#define printtextc(x, y, b) fputs(b, STDERR)
#define printmainscreen()
#endif

void relocator(void)
{
#ifndef GT2RELOC
  char packedsongname[MAX_FILENAME];
  char packedfilter[MAX_FILENAME];
#endif
  unsigned char *packeddata = NULL;
  char *playername = "player.s";

  int tableerrortype = TYPE_NONE;
  int tableerrorcause = CAUSE_NONE;
  int tableerrorsource1 = 0;
  int tableerrorsource2 = 0;
  int patterns = 0;
  int songs = 0;
  int instruments = 0;
  int numlegato = 0;
  int numnohr = 0;
  int numnormal = 0;
  int freenormal;
  int freenohr;
  int freelegato;
  int transuprange = 0;
  int transdownrange = 0;
  int pattdatasize = 0;
  int patttblsize = 0;
  int songdatasize = 0;
  int songtblsize = 0;
  int instrsize = 0;
  int wavetblsize = 0;
  int pulsetblsize = 0;
  int filttblsize = 0;
  int speedtblsize = 0;
  int playersize = 0;
  int packedsize = 0;

  FILE *songhandle = NULL;
  int selectdone;
  int opt = 0;
  unsigned char speedcode[] = {0xa2,0x00,0x8e,0x04,0xdc,0xa2,0x00,0x8e,0x05,0xdc};

  int c,d,e;

  unsigned char patttemp[512];
  unsigned char *songwork = NULL;
  unsigned char *pattwork = NULL;
  unsigned char *instrwork = NULL;

  channels = 3;
  fixedparams = 1;
  simplepulse = 1;
  firstnote = MAX_NOTES-1;
  lastnote = 0;
  patternlastnote = 0;
  noeffects = 1;
  nogate = 1;
  gnofilter = 1;
  nofiltermod = 1;
  nopulse = 1;
  nopulsemod = 1;
  nowavedelay = 1;
  nowavecmd = 1;
  norepeat = 1;
  notrans = 1;
  noportamento = 1;
  notoneporta = 1;
  novib = 1;
  noinsvib = 1;
  nosetad = 1;
  nosetsr = 1;
  nosetwave = 1;
  nosetwaveptr = 1;
  nosetpulseptr = 1;
  nosetfiltptr = 1;
  nosetfiltcutoff = 1;
  nosetfiltctrl = 1;
  nosetmastervol = 1;
  nofunktempo = 1;
  noglobaltempo = 1;
  nochanneltempo = 1;
  nofirstwavecmd = 1;
  nocalculatedspeed = 1;
  nonormalspeed = 1;
  nozerospeed = 1;

  stopsong();

  memset(pattused, 0, sizeof pattused);
  memset(instrused, 0, sizeof instrused);
  memset(chnused, 0, sizeof chnused);
  memset(tableused, 0, sizeof tableused);
  memset(tablemap, 0, sizeof tablemap);
  tableerror = 0;

  membuf_free(&src);
  membuf_free(&dest);

  // Process song-orderlists
  countpatternlengths();
  // Calculate amount of songs with nonzero length
  for (c = 0; c < MAX_SONGS; c++)
  {
    if ((songlen[c][0]) &&
        (songlen[c][1]) &&
        (songlen[c][2]))
    {
      // See which patterns are used in this song
      for (d = 0; d < MAX_CHN; d++)
      {
        songdatasize += songlen[c][d]+2;
        for (e = 0; e < songlen[c][d]; e++)
        {
          if (songorder[c][d][e] < REPEAT)
          {
            int f;
            int num = songorder[c][d][e];

            pattused[num] = 1;
            for (f = 0; f < pattlen[num]; f++)
            {
              if ((pattern[num][f*4] != REST) || (pattern[num][f*4+1]) || (pattern[num][f*4+2]))
                chnused[d] = 1;
            }
          }
          else
          {
            if (songorder[c][d][e] >= TRANSDOWN)
            {
              notrans = 0;
              if (songorder[c][d][e] < TRANSUP)
              {
                int newtransdownrange = -(songorder[c][d][e] - TRANSUP);
                if (newtransdownrange > transdownrange) transdownrange = newtransdownrange;
              }
              else
              {
                int newtransuprange = songorder[c][d][e] - TRANSUP;
                if (newtransuprange > transuprange) transuprange = newtransuprange;
              }
            }
            else norepeat = 0;
          }
        }
        if (songorder[c][d][songlen[c][d]+1] >= songlen[c][d])
        {
          sprintf(textbuffer, "ILLEGAL SONG RESTART POSITION! (SUBTUNE %02X, CHANNEL %d)", c, d+1);
          clearscreen();
          printtextc(MAX_ROWS/2, 15, textbuffer);
          fliptoscreen();
          waitkeynoupdate();
          goto PRCLEANUP;
        }
      }
      songs++;
    }
  }

  // Optimize amount of used channels
  if (!chnused[2])
    channels = 2;
  if ((!chnused[1]) && (!chnused[2]))
    channels = 1;

  if (!songs)
  {
    clearscreen();
    printtextc(MAX_ROWS/2, CTITLE, "NO SONGS, NO DATA TO SAVE!");
    fliptoscreen();
    waitkeynoupdate();
    goto PRCLEANUP;
  }

  // Build the pattern-mapping
  // Instrument 1 is always used
  instrused[1] = 1;
  for (c = 0; c < MAX_PATT; c++)
  {
    if (pattused[c])
    {
      pattmap[c] = patterns;
      patterns++;

      // See which instruments/tablecommands are used
      for (d = 0; d < pattlen[c]; d++)
      {
        tableerror = 0;

        if ((pattern[c][d*4] == KEYOFF) || (pattern[c][d*4] == KEYON))
          nogate = 0;
        if (pattern[c][d*4+1])
          instrused[pattern[c][d*4+1]] = 1;
        if (pattern[c][d*4+2])
          noeffects = 0;
        if ((pattern[c][d*4+2] >= CMD_SETWAVEPTR) && (pattern[c][d*4+2] <= CMD_SETFILTERPTR))
          exectable(pattern[c][d*4+2] - CMD_SETWAVEPTR, pattern[c][d*4+3]);
        if ((pattern[c][d*4+2] >= CMD_PORTAUP) && (pattern[c][d*4+2] <= CMD_VIBRATO))
        {
          exectable(STBL, pattern[c][d*4+3]);
          calcspeedtest(pattern[c][d*4+3]);
        }
        if (pattern[c][d*4+2] == CMD_FUNKTEMPO)
          exectable(STBL, pattern[c][d*4+3]);
        if (pattern[c][d*4+2] == CMD_FUNKTEMPO)
        {
          nofunktempo = 0;
          noglobaltempo = 0;
        }
        if ((pattern[c][d*4+2] == CMD_SETTEMPO) && ((pattern[c][d*4+3] & 0x7f) < 3)) nofunktempo = 0;

        // See, which are the highest/lowest notes used
        if ((pattern[c][d*4] >= FIRSTNOTE) && (pattern[c][d*4] <= LASTNOTE))
        {
          int newfirstnote = pattern[c][d*4] - FIRSTNOTE - transdownrange;
          int newlastnote = pattern[c][d*4] - FIRSTNOTE + transuprange;
          if (newfirstnote < 0) newfirstnote = 0;
          if (newlastnote > MAX_NOTES-1) newlastnote = MAX_NOTES-1;

          if (newfirstnote < firstnote) firstnote = newfirstnote;
          if (newlastnote > lastnote)
          {
            patternlastnote = newlastnote;
            lastnote = newlastnote;
          }
          if (newfirstnote > lastnote)
          {
            patternlastnote = newfirstnote;
            lastnote = newfirstnote;
          }
        }
        if ((tableerror) && (!tableerrortype))
        {
          tableerrortype = tableerror;
          tableerrorcause = CAUSE_PATTERN;
          tableerrorsource1 = c;
          tableerrorsource2 = d;
        }
      }
    }
  }

  // Count amount of normal, nohr, and legato instruments
  // Also see if special first wave parameters are used
  for (c = 0; c < MAX_INSTR; c++)
  {
    if (instrused[c])
    {
      if (ginstr[c].gatetimer & 0x40) numlegato++;
      else
      {
        if (ginstr[c].gatetimer & 0x80) numnohr++;
        else numnormal++;
      }
      if ((!ginstr[c].firstwave) || (ginstr[c].firstwave >= 0xfe))
        nofirstwavecmd = 0;
    }
  }
  freenormal = 1;
  freenohr = freenormal + numnormal;
  freelegato = freenohr + numnohr;

  // Build the instrument-mapping
  for (c = 0; c < MAX_INSTR; c++)
  {
    if (instrused[c])
    {
      if (ginstr[c].gatetimer & 0x40) instrmap[c] = freelegato++;
      else
      {
        if (ginstr[c].gatetimer & 0x80) instrmap[c] = freenohr++;
        else instrmap[c] = freenormal++;
      }
      instruments++;
      for (d = 0; d < MAX_TABLES; d++)
      {
        tableerror = 0;
        exectable(d, ginstr[c].ptr[d]);
        if (d == STBL) calcspeedtest(ginstr[c].ptr[d]);
        if ((tableerror) && (!tableerrortype))
        {
          tableerrortype = tableerror;
          tableerrorcause = CAUSE_INSTRUMENT;
          tableerrorsource1 = c;
          tableerrorsource2 = d;
        }
      }
    }
  }

  // Execute tableprograms invoked from wavetable commands
  for (c = 0; c < MAX_TABLELEN; c++)
  {
    if (tableused[WTBL][c+1])
    {
      if ((ltable[WTBL][c] >= WAVECMD) && (ltable[WTBL][c] <= WAVELASTCMD))
      {
        d = -1;
        tableerror = 0;

        switch(ltable[WTBL][c] - WAVECMD)
        {
          case CMD_PORTAUP:
          case CMD_PORTADOWN:
          case CMD_TONEPORTA:
          case CMD_VIBRATO:
          d = STBL;
          calcspeedtest(rtable[WTBL][c]);
          break;

          case CMD_SETPULSEPTR:
          d = PTBL;
          nopulse = 0;
           break;
           
           case CMD_SETFILTERPTR:
           d = FTBL;
          gnofilter = 0;
          break;

          case CMD_DONOTHING:
          case CMD_SETWAVEPTR:
          case CMD_FUNKTEMPO:
          sprintf(textbuffer, "ILLEGAL WAVETABLE COMMAND (ROW %02X, COMMAND %X)", c+1, ltable[WTBL][c] - WAVECMD);
          clearscreen();
          printtextc(MAX_ROWS/2, 15, textbuffer);
          fliptoscreen();
          waitkeynoupdate();
          goto PRCLEANUP;
        }

        if (d != -1) exectable(d, rtable[WTBL][c]);

        if ((tableerror) && (!tableerrortype))
        {
          tableerrortype = tableerror;
          tableerrorcause = CAUSE_WAVECMD;
          tableerrorsource1 = c+1;
          tableerrorsource2 = d;
        }
      }
    }
  }

  // Build the table-mapping
  for (c = 0; c < MAX_TABLES; c++)
  {
    int e = 1;
    for (d = 0; d < MAX_TABLELEN; d++)
    {
      if (tableused[c][d+1])
      {
        tablemap[c][d+1] = e;
        e++;
      }
    }
  }

  // Check for table errors
  if (tableerrorcause)
  {
    clearscreen();
    switch(tableerrortype)
    {
      case TYPE_JUMP:
      sprintf(textbuffer, "TABLE POINTER POINTS TO A JUMP! ");
      break;

      case TYPE_OVERFLOW:
      sprintf(textbuffer, "TABLE EXECUTION OVERFLOWS! ");
      break;
    }
    switch (tableerrorcause)
    {
      case CAUSE_PATTERN:
      sprintf(textbuffer + strlen(textbuffer), "(PATTERN %02X, ROW %02d)", tableerrorsource1, tableerrorsource2);
      break;

      case CAUSE_WAVECMD:
      sprintf(textbuffer + strlen(textbuffer), "WAVETABLE CMD (ROW %02X, ", tableerrorsource1);
      goto TABLETYPE;

      case CAUSE_INSTRUMENT:
      sprintf(textbuffer + strlen(textbuffer), "(INSTRUMENT %02X, ", tableerrorsource1);
      TABLETYPE:
      switch (tableerrorsource2)
      {
        case WTBL:
        strcat(textbuffer, "WAVE");
        break;

        case PTBL:
        strcat(textbuffer, "PULSE");
        break;

        case FTBL:
        strcat(textbuffer, "FILTER");
        break;
      }
      strcat(textbuffer, ")");
      break;
    }
    printtextc(MAX_ROWS/2, 15, textbuffer);

    fliptoscreen();
    waitkeynoupdate();
    goto PRCLEANUP;
  }

  // Find duplicate ranges in tables
  for (c = 0; c < MAX_TABLES; c++)
    findtableduplicates(c);

  // Select playroutine options
#ifndef GT2RELOC
  clearscreen();
  printblankc(0, 0, 15+16, MAX_COLUMNS);
  if (!strlen(loadedsongfilename))
    sprintf(textbuffer, "%s Packer/Relocator", programname);
  else
    sprintf(textbuffer, "%s Packer/Relocator - %s", programname, loadedsongfilename);
  textbuffer[MAX_COLUMNS] = 0;
  printtext(0, 0, 15+16, textbuffer);
  printtext(1, 2, CTITLE, "SELECT PLAYROUTINE OPTIONS: (CURSORS=MOVE/CHANGE, ENTER=ACCEPT, ESC=CANCEL)");
  selectdone = 0;
  while (!selectdone)
  {
    for (c = 0; c < MAX_OPTIONS; c++)
    {
      int color = CNORMAL;
      if (opt == c) color = CEDIT;

      printtext(1, 3+c, color, playeroptname[c]);
      if (playerversion & (PLAYER_BUFFERED << c))
        printtext(24, 3+c, color, "Yes");
      else
        printtext(24, 3+c, color, "No ");
    }
    fliptoscreen();
    waitkeynoupdate();

    if (win_quitted)
    {
      exitprogram = 1;
      goto PRCLEANUP;
    }

    switch(rawkey)
    {
      case KEY_LEFT:
      case KEY_RIGHT:
      case KEY_SPACE:
      playerversion ^= (PLAYER_BUFFERED << opt);
      if (opt)
      {
        if ((playerversion & PLAYER_SOUNDEFFECTS) || (playerversion & PLAYER_ZPGHOSTREGS) || (playerversion & PLAYER_FULLBUFFERED))
          playerversion |= PLAYER_BUFFERED;
      }
      else
      {
        if (!(playerversion & PLAYER_BUFFERED))
        {
          playerversion &= ~PLAYER_SOUNDEFFECTS;
          playerversion &= ~PLAYER_ZPGHOSTREGS;
          playerversion &= ~PLAYER_FULLBUFFERED;
        }
      }
      break;

      case KEY_UP:
      opt--;
      if (opt < 0) opt = MAX_OPTIONS-1;
      break;

      case KEY_DOWN:
      opt++;
      if (opt >= MAX_OPTIONS) opt = 0;
      break;

      case KEY_ESC:
      selectdone = -1;
      break;

      case KEY_ENTER:
      selectdone = 1;
      break;
    }
  }
  if (selectdone == -1) goto PRCLEANUP;
#endif

  // Disable optimizations if necessary
  if (playerversion & PLAYER_NOOPTIMIZATION)
  {
    fixedparams = 0;
    if (!numlegato) numlegato++;

    simplepulse = 0;
    firstnote = 0;
    lastnote = MAX_NOTES-1;
    nogate = 0;
    noeffects = 0;
    gnofilter = 0;
    nofiltermod = 0;
    nopulse = 0;
    nopulsemod = 0;
    nowavedelay = 0;
    nowavecmd = 0;
    norepeat = 0;
    notrans = 0;
    noportamento = 0;
    notoneporta = 0;
    novib = 0;
    noinsvib = 0;
    nosetad = 0;
    nosetsr = 0;
    nosetwave = 0;
    nosetwaveptr = 0;
    nosetpulseptr = 0;
    nosetfiltptr = 0;
    nosetfiltcutoff = 0;
    nosetfiltctrl = 0;
    nosetmastervol = 0;
    nofunktempo = 0;
    noglobaltempo = 0;
    nochanneltempo = 0;
    nofirstwavecmd = 0;
    nocalculatedspeed = 0;
    nonormalspeed = 0;
    nozerospeed = 0;
  }

  // Make sure buffering is used if it is needed
  if ((playerversion & PLAYER_SOUNDEFFECTS) || (playerversion & PLAYER_ZPGHOSTREGS) || (playerversion & PLAYER_FULLBUFFERED))
    playerversion |= PLAYER_BUFFERED;

  // Sound effect or ghostreg players always use full 3 channels
  if ((playerversion & PLAYER_SOUNDEFFECTS) || (playerversion & PLAYER_FULLBUFFERED) || (playerversion & PLAYER_ZPGHOSTREGS))
    channels = 3;

  // Allocate memory for song-orderlists
  songtblsize = songs*6;
  songwork = (unsigned char*)malloc(songdatasize);
  if (!songwork)
  {
    clearscreen();
    printtextc(MAX_ROWS/2, CTITLE, "OUT OF MEMORY IN PACKER/RELOCATOR!");
    fliptoscreen();
    waitkeynoupdate();
    goto PRCLEANUP;
  }

  // Generate songorderlists & songtable
  songdatasize = 0;
  for (c = 0; c < songs; c++)
  {
    if ((songlen[c][0]) &&
        (songlen[c][1]) &&
        (songlen[c][2]))
    {
      for (d = 0; d < MAX_CHN; d++)
      {
        songoffset[c][d] = songdatasize;
        songsize[c][d] = songlen[c][d] + 2;

        for (e = 0; e < songlen[c][d]; e++)
        {
          // Pattern
          if (songorder[c][d][e] < REPEAT)
            songwork[songdatasize++] = pattmap[songorder[c][d][e]];
          else
          {
            // Transpose
            if (songorder[c][d][e] >= TRANSDOWN)
            {
              songwork[songdatasize++] = songorder[c][d][e];
            }
            // Repeat sequence: must be swapped
            else
            {
              // See that repeat amount is more than 1
              if (songorder[c][d][e] > REPEAT)
              {
                // Insanity check that a pattern indeed follows
                if (songorder[c][d][e+1] < REPEAT)
                {
                  songwork[songdatasize++] = pattmap[songorder[c][d][e+1]];
                  songwork[songdatasize++] = songorder[c][d][e];
                  e++;
                }
                else
                  songwork[songdatasize++] = songorder[c][d][e];
              }
            }
          }
        }
        // Endmark & repeat position
        songwork[songdatasize++] = songorder[c][d][e++];
        songwork[songdatasize++] = songorder[c][d][e++];
      }
    }
    else
    {
      for (d = 0; d < MAX_CHN; d++)
      {
        songoffset[c][d] = songdatasize;
        songsize[c][d] = 0;
      }
    }
  }

  // Calculate total size of patterns
  for (c = 0; c < MAX_PATT; c++)
  {
    if (pattused[c])
    {
      int result = packpattern(patttemp, pattern[c], pattlen[c]);

      if (result < 0)
      {
        clearscreen();
        sprintf(textbuffer, "PATTERN %02X IS TOO COMPLEX (OVER 256 BYTES PACKED)!", c);
        printtextc(MAX_ROWS/2, 15, textbuffer);
        fliptoscreen();
        waitkeynoupdate();
        goto PRCLEANUP;
      }
      pattdatasize += result;
    }
  }

  patttblsize = patterns*2;
  pattwork = (unsigned char*)malloc(pattdatasize);
  if (!pattwork)
  {
    clearscreen();
    printtextc(MAX_ROWS/2, CTITLE, "OUT OF MEMORY IN PACKER/RELOCATOR!");
    fliptoscreen();
    waitkeynoupdate();
    goto PRCLEANUP;
  }

  // This time pack the patterns for real
  pattdatasize = 0;
  d = 0;
  for (c = 0; c < MAX_PATT; c++)
  {
    if (pattused[c])
    {
      pattoffset[d] = pattdatasize;
      pattsize[d] = packpattern(&pattwork[pattdatasize], pattern[c], pattlen[c]);
      pattdatasize += pattsize[d];
      d++;
    }
  }

  // Then process instruments
  instrsize = instruments*9;
  instrwork = (unsigned char*)malloc(instrsize);
  if (!instrwork)
  {
    clearscreen();
    printtextc(MAX_ROWS/2, CTITLE, "OUT OF MEMORY IN PACKER/RELOCATOR!");
    fliptoscreen();
    waitkeynoupdate();
    goto PRCLEANUP;
  }

  for (c = 1; c < MAX_INSTR; c++)
  {
    if (instrused[c])
    {
      d = instrmap[c] - 1;
      instrwork[d] = ginstr[c].ad;
      instrwork[d+instruments] = ginstr[c].sr;
      instrwork[d+instruments*2] = tablemap[WTBL][ginstr[c].ptr[WTBL]];
      instrwork[d+instruments*3] = tablemap[PTBL][ginstr[c].ptr[PTBL]];
      instrwork[d+instruments*4] = tablemap[FTBL][ginstr[c].ptr[FTBL]];
      if (ginstr[c].vibdelay)
      {
        instrwork[d+instruments*5] = tablemap[STBL][ginstr[c].ptr[STBL]];
        instrwork[d+instruments*6] = ginstr[c].vibdelay - 1;
      }
      else
      {
        instrwork[d+instruments*5] = 0;
        instrwork[d+instruments*6] = 0;
      }
      instrwork[d+instruments*7] = ginstr[c].gatetimer & 0x3f;
      instrwork[d+instruments*8] = ginstr[c].firstwave;

      if (ginstr[c].ptr[STBL])
      {
        novib = 0;
        noinsvib = 0;
      }
      if (ginstr[c].ptr[PTBL])
        nopulse = 0;
      if (ginstr[c].ptr[FTBL])
        gnofilter = 0;

      // See if all instruments use same gatetimer & firstwave parameters
      if ((ginstr[c].gatetimer != ginstr[1].gatetimer) ||
          (ginstr[c].firstwave != ginstr[1].firstwave))
        fixedparams = 0;
      // or if special firstwave commands are in use
      if ((!ginstr[c].firstwave) || (ginstr[c].firstwave >= 0xfe))
        fixedparams = 0;
    }
  }

  // Disable sameparam optimization for multispeed stability
  if (multiplier > 1)
  {
    fixedparams = 0;
    numlegato++;
    numnohr++;
  }

  if (fixedparams) instrsize -= instruments*2;
  if (noinsvib) instrsize -= instruments*2;
  if (nopulse) instrsize -= instruments;
  if (gnofilter) instrsize -= instruments;

  // Process tables
  for (c = 0; c < MAX_TABLELEN; c++)
  {
    if (tableused[WTBL][c+1])
    {
      wavetblsize += 2;
      if ((ltable[WTBL][c] >= WAVEDELAY) && (ltable[WTBL][c] <= WAVELASTDELAY)) nowavedelay = 0;
      if ((ltable[WTBL][c] >= WAVECMD) && (ltable[WTBL][c] <= WAVELASTCMD))
      {
        nowavecmd = 0;
        noeffects = 0;
        switch (ltable[WTBL][c] - WAVECMD)
        {
          case CMD_PORTAUP:
          case CMD_PORTADOWN:
          noportamento = 0;
          break;

          case CMD_TONEPORTA:
          notoneporta = 0;
          break;

          case CMD_VIBRATO:
          novib = 0;
          break;

          case CMD_SETAD:
          nosetad = 0;
          break;

          case CMD_SETSR:
          nosetsr = 0;
          break;

          case CMD_SETWAVE:
          nosetwave = 0;
          break;

          case CMD_SETPULSEPTR:
          nosetpulseptr = 0;
          break;

          case CMD_SETFILTERPTR:
          nosetfiltptr = 0;
          break;

          case CMD_SETFILTERCUTOFF:
          nosetfiltcutoff = 0;
          break;

          case CMD_SETFILTERCTRL:
          nosetfiltctrl = 0;
          break;

          case CMD_SETMASTERVOL:
          nosetmastervol = 0;
          break;
        }
      }
      if (ltable[WTBL][c] < WAVECMD)
      {
        if (rtable[WTBL][c] <= 0x80)
        {
          int newlastnote = rtable[WTBL][c] + patternlastnote;
          if (newlastnote > MAX_NOTES - 1) newlastnote = MAX_NOTES - 1;
          if (rtable[WTBL][c] >= 0x20) firstnote = 0;
           if (newlastnote > lastnote) lastnote = newlastnote;
        }
        else
        {
          int newfirstnote = rtable[WTBL][c] & 0x7f;
          int newlastnote = rtable[WTBL][c] & 0x7f;
          if (newlastnote > MAX_NOTES - 1) newlastnote = MAX_NOTES - 1;
          if (newfirstnote < firstnote) firstnote = newfirstnote;
          if (newlastnote > lastnote) lastnote = newlastnote;
        }
      }
    }
  }
  for (c = 0; c < MAX_TABLELEN; c++)
  {
    if (tableused[PTBL][c+1])
    {
      pulsetblsize += 2;
      if ((ltable[PTBL][c] >= 0x80) && (ltable[PTBL][c] != 0xff))
      {
        if (rtable[PTBL][c] & 0xf) simplepulse = 0;
      }
      if (ltable[PTBL][c] < 0x80)
      {
        nopulsemod = 0;
        if (rtable[PTBL][c] & 0xf) simplepulse = 0;
      }
    }
  }
  for (c = 0; c < MAX_TABLELEN; c++)
  {
    if (tableused[FTBL][c+1])
    {
      filttblsize += 2;
      if (ltable[FTBL][c] < 0x80) nofiltermod = 0;
    }
  }
  for (c = 0; c < MAX_TABLELEN; c++)
  {
    if (tableused[STBL][c+1]) speedtblsize += 2;
  }
  // Zero entry of speedtable
  if ((!novib) || (!nofunktempo) || (!noportamento) || (!notoneporta))
    speedtblsize += 2;

  if (nopulse) pulsetblsize = 0;
  if (gnofilter) filttblsize = 0;

#ifdef GT2RELOC
  fprintf(STDOUT, "Player address:   $%04X\n", playeradr);
  fprintf(STDOUT, "Zeropage address: $%04X\n", zeropageadr);
#else
  sprintf(textbuffer, "SELECT START ADDRESS: (CURSORS=MOVE, ENTER=ACCEPT, ESC=CANCEL)");
  printtext(1, 11, 15, textbuffer);

  selectdone = 0;
  while (!selectdone)
  {
    sprintf(textbuffer, "$%04X", playeradr);
    printtext(1, 12, 10, textbuffer);

    fliptoscreen();
    waitkeynoupdate();

    if (win_quitted)
    {
      exitprogram = 1;
      goto PRCLEANUP;
    }

    switch(rawkey)
    {
      case KEY_LEFT:
      playeradr -= 0x0400;
      playeradr &= 0xff00;
      break;

      case KEY_UP:
      playeradr += 0x0100;
      playeradr &= 0xff00;
      break;

      case KEY_RIGHT:
      playeradr += 0x0400;
      playeradr &= 0xff00;
      break;

      case KEY_DOWN:
      playeradr -= 0x0100;
      playeradr &= 0xff00;
      break;

      case KEY_ESC:
      selectdone = -1;
      break;

      case KEY_ENTER:
      selectdone = 1;
      break;
    }
  }

  if (selectdone == -1) goto PRCLEANUP;

  sprintf(textbuffer, "SELECT ZEROPAGE ADDRESS: (CURSORS=MOVE, ENTER=ACCEPT, ESC=CANCEL)");
  printtext(1, 14, 15, textbuffer);

  selectdone = 0;
  while (!selectdone)
  {
    if (playerversion & PLAYER_ZPGHOSTREGS)
    {
      if (zeropageadr < 0x02) zeropageadr = 0xe5;
      if (zeropageadr > 0xe5) zeropageadr = 0x02;
    }
    else
    {
      if (zeropageadr < 0x02) zeropageadr = 0xfe;
      if (zeropageadr > 0xfe) zeropageadr = 0x02;
    }

    if (!(playerversion & PLAYER_ZPGHOSTREGS))
    {
      if (zeropageadr < 0x90)
        sprintf(textbuffer, "$%02X-$%02X (Used by BASIC interpreter)    ", zeropageadr, zeropageadr+1);
      if ((zeropageadr >= 0x90) && (zeropageadr < 0xfb))
        sprintf(textbuffer, "$%02X-$%02X (Used by KERNAL routines)      ", zeropageadr, zeropageadr+1);
      if ((zeropageadr >= 0xfb) && (zeropageadr < 0xfe))
        sprintf(textbuffer, "$%02X-$%02X (Unused)                       ", zeropageadr, zeropageadr+1);
      if (zeropageadr >= 0xfe)
        sprintf(textbuffer, "$%02X-$%02X ($FF used by BASIC interpreter)", zeropageadr, zeropageadr+1);
    }
    else
    {
      sprintf(textbuffer, "$%02X-$%02X (ghostregs start at %02X)", zeropageadr, zeropageadr+26, zeropageadr);
    }

    printtext(1, 15, 10, textbuffer);

    fliptoscreen();
    waitkeynoupdate();

    if (win_quitted)
    {
      exitprogram = 1;
      goto PRCLEANUP;
    }

    switch(rawkey)
    {
      case KEY_LEFT:
      zeropageadr -= 0x10;
      break;

      case KEY_UP:
      zeropageadr++;
      break;

      case KEY_RIGHT:
      zeropageadr += 0x10;
      break;

      case KEY_DOWN:
      zeropageadr--;
      break;

      case KEY_ESC:
      selectdone = -1;
      break;

      case KEY_ENTER:
      selectdone = 1;
      break;
    }
  }

  if (selectdone == -1) goto PRCLEANUP;
#endif

  // Validate frequencytable parameters
  if (lastnote < firstnote)
    lastnote = firstnote;
  if (firstnote < 0) firstnote = 0;
  if (!nocalculatedspeed)
    lastnote++; // Calculated speeds need the next frequency value
  if (lastnote > MAX_NOTES-1) lastnote = MAX_NOTES-1;
  // For sound effect support, always use the full table
  if (playerversion & PLAYER_SOUNDEFFECTS)
  {
    firstnote = 0;
    lastnote = MAX_NOTES-1;
  }

  // Insert baseaddresses
  insertdefine("base", playeradr);
  insertdefine("zpbase", zeropageadr);
  insertdefine("SIDBASE", sidaddress);

  // Insert conditionals
  insertdefine("SOUNDSUPPORT", (playerversion & PLAYER_SOUNDEFFECTS) ? 1 : 0);
  insertdefine("VOLSUPPORT", (playerversion & PLAYER_VOLUME) ? 1 : 0);
  insertdefine("BUFFEREDWRITES", (playerversion & PLAYER_BUFFERED) ? 1 : 0);
  insertdefine("GHOSTREGS", (playerversion & (PLAYER_ZPGHOSTREGS|PLAYER_FULLBUFFERED)) ? 1 : 0);
  insertdefine("ZPGHOSTREGS", (playerversion & PLAYER_ZPGHOSTREGS) ? 1 : 0);
  insertdefine("FIXEDPARAMS", fixedparams);
  insertdefine("SIMPLEPULSE", simplepulse);
  insertdefine("PULSEOPTIMIZATION", optimizepulse);
  insertdefine("REALTIMEOPTIMIZATION", optimizerealtime);
  insertdefine("NOAUTHORINFO", (playerversion & PLAYER_AUTHORINFO) ? 0 : 1);
  insertdefine("NOEFFECTS", noeffects);
  insertdefine("NOGATE", nogate);
  insertdefine("NOFILTER", gnofilter);
  insertdefine("NOFILTERMOD", nofiltermod);
  insertdefine("NOPULSE", nopulse);
  insertdefine("NOPULSEMOD", nopulsemod);
  insertdefine("NOWAVEDELAY", nowavedelay);
  insertdefine("NOWAVECMD", nowavecmd);
  insertdefine("NOREPEAT", norepeat);
  insertdefine("NOTRANS", notrans);
  insertdefine("NOPORTAMENTO", noportamento);
  insertdefine("NOTONEPORTA", notoneporta);
  insertdefine("NOVIB", novib);
  insertdefine("NOINSTRVIB", noinsvib);
  insertdefine("NOSETAD", nosetad);
  insertdefine("NOSETSR", nosetsr);
  insertdefine("NOSETWAVE", nosetwave);
  insertdefine("NOSETWAVEPTR", nosetwaveptr);
  insertdefine("NOSETPULSEPTR", nosetpulseptr);
  insertdefine("NOSETFILTPTR", nosetfiltptr);
  insertdefine("NOSETFILTCTRL", nosetfiltctrl);
  insertdefine("NOSETFILTCUTOFF", nosetfiltcutoff);
  insertdefine("NOSETMASTERVOL", nosetmastervol);
  insertdefine("NOFUNKTEMPO", nofunktempo);
  insertdefine("NOGLOBALTEMPO", noglobaltempo);
  insertdefine("NOCHANNELTEMPO", nochanneltempo);
  insertdefine("NOFIRSTWAVECMD", nofirstwavecmd);
  insertdefine("NOCALCULATEDSPEED", nocalculatedspeed);
  insertdefine("NONORMALSPEED", nonormalspeed);
  insertdefine("NOZEROSPEED", nozerospeed);

  // Insert parameters
  insertdefine("NUMCHANNELS", channels);
  insertdefine("NUMSONGS", songs);
  insertdefine("FIRSTNOTE", firstnote);
  insertdefine("FIRSTNOHRINSTR", numnormal + 1);
  insertdefine("FIRSTLEGATOINSTR", numnormal + numnohr + 1);
  insertdefine("NUMHRINSTR", numnormal);
  insertdefine("NUMNOHRINSTR", numnohr);
  insertdefine("NUMLEGATOINSTR", numlegato);
  insertdefine("ADPARAM", adparam >> 8);
  insertdefine("SRPARAM", adparam & 0xff);
  if ((ginstr[MAX_INSTR-1].ad >= 2) && (!(ginstr[MAX_INSTR-1].ptr[WTBL])))
    insertdefine("DEFAULTTEMPO", ginstr[MAX_INSTR-1].ad - 1);
  else
    insertdefine("DEFAULTTEMPO", multiplier ? (multiplier*6-1) : 5);

  // Fixed firstwave & gatetimer
  if (fixedparams)
  {
    insertdefine("FIRSTWAVEPARAM", ginstr[1].firstwave);
    insertdefine("GATETIMERPARAM", ginstr[1].gatetimer & 0x3f);
  }

  // Insert source code of player
  if (adparam >= 0xf000)
    playername = "altplayer.s";
    
  if (!insertfile(playername))
  {
    clearscreen();
    printtextc(MAX_ROWS/2, CTITLE, "COULD NOT OPEN PLAYROUTINE!");
    fliptoscreen();
    waitkeynoupdate();
    goto PRCLEANUP;
  }
  
  // Modify ghostregs to not be zeropage if needed
  if ((playerversion & PLAYER_FULLBUFFERED) && (playerversion & PLAYER_ZPGHOSTREGS) == 0)
  {
    int bufsize = membuf_get_size(&src);
    char* bufdata = (char*)membuf_get(&src);
    int c;
    for (c = 0; c < bufsize; c++)
    {
      if (bufdata[c] == '<')
      {
        if (memcmp(bufdata + c + 1, "ghost", 5) == 0)
          bufdata[c] = ' ';
      }
    }
  }

  // Insert frequencytable
  insertlabel("mt_freqtbllo");
  insertbytes(&freqtbllo[firstnote], lastnote-firstnote+1);
  insertlabel("mt_freqtblhi");
  insertbytes(&freqtblhi[firstnote], lastnote-firstnote+1);

  // Insert songtable
  insertlabel("mt_songtbllo");
  for (c = 0; c < songs*3; c++)
  {
    sprintf(textbuffer, "mt_song%d", c);
    insertaddrlo(textbuffer);
  }
  insertlabel("mt_songtblhi");
  for (c = 0; c < songs*3; c++)
  {
    sprintf(textbuffer, "mt_song%d", c);
    insertaddrhi(textbuffer);
  }

  // Insert patterntable
  insertlabel("mt_patttbllo");
  for (c = 0; c < patterns; c++)
  {
    sprintf(textbuffer, "mt_patt%d", c);
    insertaddrlo(textbuffer);
  }
  insertlabel("mt_patttblhi");
  for (c = 0; c < patterns; c++)
  {
    sprintf(textbuffer, "mt_patt%d", c);
    insertaddrhi(textbuffer);
  }

  // Insert instruments
  insertlabel("mt_insad");
  insertbytes(&instrwork[0], instruments);
  insertlabel("mt_inssr");
  insertbytes(&instrwork[instruments], instruments);
  insertlabel("mt_inswaveptr");
  insertbytes(&instrwork[instruments*2], instruments);
  if (!nopulse)
  {
    insertlabel("mt_inspulseptr");
    insertbytes(&instrwork[instruments*3], instruments);
  }
  if (!gnofilter)
  {
    insertlabel("mt_insfiltptr");
    insertbytes(&instrwork[instruments*4], instruments);
  }
  if (!noinsvib)
  {
    insertlabel("mt_insvibparam");
    insertbytes(&instrwork[instruments*5], instruments);
    insertlabel("mt_insvibdelay");
    insertbytes(&instrwork[instruments*6], instruments);
  }
  if (!fixedparams)
  {
    insertlabel("mt_insgatetimer");
    insertbytes(&instrwork[instruments*7], instruments);
    insertlabel("mt_insfirstwave");
    insertbytes(&instrwork[instruments*8], instruments);
  }

  // Insert tables
  for (c = 0; c < MAX_TABLES; c++)
  {
    if ((c == PTBL) && (nopulse)) goto SKIPTABLE;
    if ((c == FTBL) && (gnofilter)) goto SKIPTABLE;

    // Write table left side
    // Extra zero for speedtable
    if ((c == STBL) && ((!novib) || (!nofunktempo) || (!noportamento) || (!notoneporta))) insertbyte(0);
    // Table label
    insertlabel(tableleftname[c]);

    // Table data
    for (d = 0; d < MAX_TABLELEN; d++)
    {
      if (tableused[c][d+1])
      {
        switch (c)
        {
          // In wavetable, convert waveform values for the playroutine
          case WTBL:
          {
            unsigned char wave = ltable[c][d];
            if ((ltable[c][d] >= WAVESILENT) && (ltable[c][d] <= WAVELASTSILENT)) wave &= 0xf;
            if ((ltable[c][d] > WAVELASTDELAY) && (ltable[c][d] <= WAVELASTSILENT) && (!nowavedelay)) wave += 0x10;
            insertbyte(wave);
          }
          break;

          case PTBL:
          if ((simplepulse) && (ltable[c][d] != 0xff) && (ltable[c][d] > 0x80))
            insertbyte(0x80);
          else
            insertbyte(ltable[c][d]);
          break;

          // In filtertable, modify passband bits
          case FTBL:
          if ((ltable[c][d] != 0xff) && (ltable[c][d] > 0x80))
            insertbyte(((ltable[c][d] & 0x70) >> 1) | 0x80);
          else
            insertbyte(ltable[c][d]);
          break;

          default:
          insertbyte(ltable[c][d]);
          break;
        }
      }
    }

    // Write table right side, remapping jumps as necessary
    // Extra zero for speedtable
    if ((c == STBL) && ((!novib) || (!nofunktempo) || (!noportamento) || (!notoneporta))) insertbyte(0);
    // Table label
    insertlabel(tablerightname[c]);

    for (d = 0; d < MAX_TABLELEN; d++)
    {
      if (tableused[c][d+1])
      {
        if ((ltable[c][d] != 0xff) || (c == STBL))
        {
          switch(c)
          {
            case WTBL:
            if ((ltable[c][d] >= WAVECMD) && (ltable[c][d] <= WAVELASTCMD))
            {
              // Remap table-referencing commands
              switch (ltable[c][d] - WAVECMD)
              {
                case CMD_PORTAUP:
                case CMD_PORTADOWN:
                case CMD_TONEPORTA:
                case CMD_VIBRATO:
                insertbyte(tablemap[STBL][rtable[c][d]]);
                break;

                case CMD_SETPULSEPTR:
                insertbyte(tablemap[PTBL][rtable[c][d]]);
                break;

                case CMD_SETFILTERPTR:
                insertbyte(tablemap[FTBL][rtable[c][d]]);
                break;

                default:
                insertbyte(rtable[c][d]);
                break;
              }
            }
            else
            {
              // For normal notes, reverse all right side high bits
              insertbyte(rtable[c][d] ^ 0x80);
            }
            break;

            case PTBL:
            if (simplepulse)
            {
              if (ltable[c][d] >= 0x80)
                insertbyte((ltable[c][d] & 0x0f) | (rtable[c][d] & 0xf0));
              else
              {
                int pulsespeed = rtable[c][d] >> 4;
                if (rtable[c][d] & 0x80)
                {
                  pulsespeed |= 0xf0;
                  pulsespeed--;
                }
                pulsespeed = swapnybbles(pulsespeed);
                insertbyte(pulsespeed);
              }
            }
            else
              insertbyte(rtable[c][d]);
            break;

            default:
            insertbyte(rtable[c][d]);
            break;
          }
        }
        else
          insertbyte(tablemap[c][rtable[c][d]]);
      }
    }

    SKIPTABLE: ;
  }

  // Insert orderlists
  for (c = 0; c < songs; c++)
  {
    for (d = 0; d < MAX_CHN; d++)
    {
      sprintf(textbuffer, "mt_song%d", c*3+d);
      insertlabel(textbuffer);
      insertbytes(&songwork[songoffset[c][d]], songsize[c][d]);
    }
  }

  // Insert patterns
  for (c = 0; c < patterns; c++)
  {
    sprintf(textbuffer, "mt_patt%d", c);
    insertlabel(textbuffer);
    insertbytes(&pattwork[pattoffset[c]], pattsize[c]);
  }

  /* {
    FILE *handle = fopen("debug.s", "wb");
    fwrite(membuf_get(&src), membuf_memlen(&src), 1, handle);
    fclose(handle);
  } */

  // Assemble; on error fail in a rude way (the parser does so too)
  if (assemble(&src, &dest)) exit(1);

  packeddata = (unsigned char*)membuf_get(&dest);
  packedsize = membuf_memlen(&dest);
  playersize = packedsize - songtblsize - songdatasize - patttblsize - pattdatasize - instrsize - wavetblsize - pulsetblsize - filttblsize - speedtblsize;

  // Copy author info
  if (playerversion & PLAYER_AUTHORINFO)
  {
    for (c = 0; c < 32; c++)
    {
      packeddata[32+c] = authorname[c];
      // Convert 0 to space
      if (packeddata[32+c] == 0) packeddata[32+c] = 0x20;
    }
  }

  // Print results
#ifdef GT2RELOC
  fprintf(STDOUT, "packing results:\n");
  fprintf(STDOUT, "Playroutine:     %d bytes\n", playersize);
  fprintf(STDOUT, "Songtable:       %d bytes\n", songtblsize);
  fprintf(STDOUT, "Song-orderlists: %d bytes\n", songdatasize);
  fprintf(STDOUT, "Patterntable:    %d bytes\n", patttblsize);
  fprintf(STDOUT, "Patterns:        %d bytes\n", pattdatasize);
  fprintf(STDOUT, "Instruments:     %d bytes\n", instrsize);
  fprintf(STDOUT, "Tables:          %d bytes\n", wavetblsize+pulsetblsize+filttblsize+speedtblsize);
  fprintf(STDOUT, "Total size:      %d bytes\n", packedsize);

  songhandle = fopen(packedsongname, "wb");
  if (!songhandle) 
  {
      fprintf(STDERR, "error: could not open output file '%s'.\n", packedsongname);
      goto PRCLEANUP;
  }

#else
  clearscreen();
  printblankc(0, 0, 15+16, MAX_COLUMNS);
  if (!strlen(loadedsongfilename))
    sprintf(textbuffer, "%s Packer/Relocator", programname);
  else
    sprintf(textbuffer, "%s Packer/Relocator - %s", programname, loadedsongfilename);
  textbuffer[80] = 0;
  printtext(0, 0, 15+16, textbuffer);

  sprintf(textbuffer, "PACKING RESULTS:");
  printtext(1, 2, 15, textbuffer);

  sprintf(textbuffer, "Playroutine:     %d bytes", playersize);
  printtext(1, 3, 7, textbuffer);
  sprintf(textbuffer, "Songtable:       %d bytes", songtblsize);
  printtext(1, 4, 7, textbuffer);
  sprintf(textbuffer, "Song-orderlists: %d bytes", songdatasize);
  printtext(1, 5, 7, textbuffer);
  sprintf(textbuffer, "Patterntable:    %d bytes", patttblsize);
  printtext(1, 6, 7, textbuffer);
  sprintf(textbuffer, "Patterns:        %d bytes", pattdatasize);
  printtext(1, 7, 7, textbuffer);
  sprintf(textbuffer, "Instruments:     %d bytes", instrsize);
  printtext(1, 8, 7, textbuffer);
  sprintf(textbuffer, "Tables:          %d bytes", wavetblsize+pulsetblsize+filttblsize+speedtblsize);
  printtext(1, 9, 7, textbuffer);
  sprintf(textbuffer, "Total size:      %d bytes", packedsize);
  printtext(1, 11, 7, textbuffer);
  fliptoscreen();


  // Now ask for fileformat
  printtext(1, 13, CTITLE, "SELECT FORMAT TO SAVE IN: (CURSORS=MOVE, ENTER=ACCEPT, ESC=CANCEL)");

  selectdone = 0;

  while (!selectdone)
  {
    switch(fileformat)
    {
      case FORMAT_SID:
      printtext(1, 14, CEDIT, "SID - SIDPlay music file format          ");
      strcpy(packedfilter, "*.sid");
      break;

      case FORMAT_PRG:
      printtext(1, 14, CEDIT, "PRG - C64 native format                  ");
      strcpy(packedfilter, "*.prg");
      break;

      case FORMAT_BIN:
      printtext(1, 14, CEDIT, "BIN - Raw binary format (no startaddress)");
      strcpy(packedfilter, "*.bin");
      break;
    }

    fliptoscreen();
    waitkeynoupdate();

    if (win_quitted)
    {
      exitprogram = 1;
      goto PRCLEANUP;
    }

    switch(rawkey)
    {
      case KEY_LEFT:
      case KEY_DOWN:
      fileformat--;
      if (fileformat < FORMAT_SID) fileformat = FORMAT_BIN;
      break;

      case KEY_RIGHT:
      case KEY_UP:
      fileformat++;
      if (fileformat > FORMAT_BIN) fileformat = FORMAT_SID;
      break;

      case KEY_ESC:
      selectdone = -1;
      break;

      case KEY_ENTER:
      selectdone = 1;
      break;
    }
  }
  if (selectdone == -1) goto PRCLEANUP;

  // By default, copy loaded song name up to the extension
  memset(packedsongname, 0, sizeof packedsongname);
  for (c = 0; c < strlen(loadedsongfilename); c++)
  {
    if (loadedsongfilename[c] == '.') break;
    packedsongname[c] = loadedsongfilename[c];
  }
  switch (fileformat)
  {
    case FORMAT_PRG:
    strcat(packedsongname, ".prg");
    break;

    case FORMAT_BIN:
    strcat(packedsongname, ".bin");
    break;

    case FORMAT_SID:
    strcat(packedsongname, ".sid");
    break;
  }

  // Now ask for filename, retry if unsuccessful
  while (!songhandle)
  {
    if (!fileselector(packedsongname, packedpath, packedfilter, "Save Music+Playroutine", 3))
      goto PRCLEANUP;

    if (strlen(packedsongname) < MAX_FILENAME-4)
    {
      int extfound = 0;
      for (c = strlen(packedsongname)-1; c >= 0; c--)
      {
        if (packedsongname[c] == '.') extfound = 1;
      }
      if (!extfound)
      {
        switch (fileformat)
        {
          case FORMAT_PRG:
          strcat(packedsongname, ".prg");
          break;

          case FORMAT_BIN:
          strcat(packedsongname, ".bin");
          break;

          case FORMAT_SID:
          strcat(packedsongname, ".sid");
          break;
        }
      }
    }
    songhandle = fopen(packedsongname, "wb");
  }
#endif

  if (fileformat == FORMAT_PRG)
  {
    fwritele16(songhandle, playeradr);
  }
  if (fileformat == FORMAT_SID)
  {
    unsigned char ident[] = {'P', 'S', 'I', 'D', 0x00, 0x02, 0x00, 0x7c};
    unsigned char byte;
    // Identification
    fwrite(ident, sizeof ident, 1, songhandle);

    // Load address
    byte = 0x00;
    fwrite8(songhandle, byte);
    fwrite8(songhandle, byte);

    // Init address
    if ((multiplier > 1) || (!multiplier))
    {
      unsigned speedvalue;
      byte = (playeradr-10) >> 8;
      fwrite8(songhandle, byte);
      byte = (playeradr-10) & 0xff;
      fwrite8(songhandle, byte);

      if (multiplier)
      {
        if (ntsc) speedvalue = 0x42c6/multiplier;
        else speedvalue = 0x4cc7/multiplier;
      }
      else
      {
        if (ntsc) speedvalue = 0x42c6*2;
        else speedvalue = 0x4cc7*2;
      }
      speedcode[1] = speedvalue & 0xff;
      speedcode[6] = speedvalue >> 8;
    }
    else
    {
      byte = (playeradr) >> 8;
      fwrite8(songhandle, byte);
      byte = (playeradr) & 0xff;
      fwrite8(songhandle, byte);
    }

    // Play address
    byte = (playeradr+3) >> 8;
    fwrite8(songhandle, byte);
    byte = (playeradr+3) & 0xff;
    fwrite8(songhandle, byte);

    // Number of subtunes
    byte = 0x00;
    fwrite8(songhandle, byte);
    byte = songs;
    fwrite8(songhandle, byte);

    // Default subtune
    byte = 0x00;
    fwrite8(songhandle, byte);
    byte = 0x01;
    fwrite8(songhandle, byte);

    // Song speed bits
    byte = 0x00;
    if ((ntsc) || (multiplier > 1) || (!multiplier)) byte = 0xff;
    fwrite8(songhandle, byte);
    fwrite8(songhandle, byte);
    fwrite8(songhandle, byte);
    fwrite8(songhandle, byte);

    // Songname etc.
    fwrite(songname, sizeof songname, 1, songhandle);
    fwrite(authorname, sizeof authorname, 1, songhandle);
    fwrite(copyrightname, sizeof copyrightname, 1, songhandle);

    // Flags
    byte = 0x00;
    fwrite8(songhandle, byte);
    if (ntsc) byte = 8;
      else byte = 4;
    if (sidmodel) byte |= 32;
      else byte |= 16;
    fwrite8(songhandle, byte);

    // Reserved longword
    byte = 0x00;
    fwrite8(songhandle, byte);
    fwrite8(songhandle, byte);
    fwrite8(songhandle, byte);
    fwrite8(songhandle, byte);

    // Load address
    if ((multiplier > 1) || (!multiplier))
    {
      byte = (playeradr-10) & 0xff;
      fwrite8(songhandle, byte);
      byte = (playeradr-10) >> 8;
      fwrite8(songhandle, byte);
    }
    else
    {
      byte = (playeradr) & 0xff;
      fwrite8(songhandle, byte);
      byte = (playeradr) >> 8;
      fwrite8(songhandle, byte);
    }
    if ((multiplier > 1) || (!multiplier)) fwrite(speedcode, 10, 1, songhandle);
  }

  fwrite(packeddata, packedsize, 1, songhandle);
  fclose(songhandle);

  PRCLEANUP:
  membuf_free(&src);
  membuf_free(&dest);

  if (pattwork) free(pattwork);
  if (songwork) free(songwork);
  if (instrwork) free(instrwork);
  printmainscreen();
  key = 0;
  rawkey = 0;
}

int packpattern(unsigned char *dest, unsigned char *src, int rows)
{
  unsigned char temp1[MAX_PATTROWS*4];
  unsigned char temp2[512];
  unsigned char instr = 0;
  int command = -1;
  int databyte = -1;
  int destsizeim = 0;
  int destsize = 0;
  int c, d;

  // First optimize instrument changes
  for (c = 0; c < rows; c++)
  {
    if ((c) && (src[c*4+1]) && (src[c*4+1] == instr))
    {
      temp1[c*4] = src[c*4];
      temp1[c*4+1] = 0;
      temp1[c*4+2] = src[c*4+2];
      temp1[c*4+3] = src[c*4+3];
    }
    else
    {
      temp1[c*4] = src[c*4];
      temp1[c*4+1] = src[c*4+1];
      temp1[c*4+2] = src[c*4+2];
      temp1[c*4+3] = src[c*4+3];
      if (src[c*4+1])
        instr = src[c*4+1];
    }

    switch(temp1[c*4+2])
    {
      // Remap speedtable commands
      case CMD_PORTAUP:
      case CMD_PORTADOWN:
      noportamento = 0;
      temp1[c*4+3] = tablemap[STBL][temp1[c*4+3]];
      break;

      case CMD_TONEPORTA:
      notoneporta = 0;
      temp1[c*4+3] = tablemap[STBL][temp1[c*4+3]];
      break;

      case CMD_VIBRATO:
      novib = 0;
      temp1[c*4+3] = tablemap[STBL][temp1[c*4+3]];
      break;

      case CMD_SETAD:
      nosetad = 0;
      break;

      case CMD_SETSR:
      nosetsr = 0;
      break;

      case CMD_SETWAVE:
      nosetwave = 0;
      break;

      // Remap table commands
      case CMD_SETWAVEPTR:
      nosetwaveptr = 0;
      temp1[c*4+3] = tablemap[WTBL][temp1[c*4+3]];
      break;

      case CMD_SETPULSEPTR:
      nosetpulseptr = 0;
      nopulse = 0;
      temp1[c*4+3] = tablemap[PTBL][temp1[c*4+3]];
      break;

      case CMD_SETFILTERPTR:
      nosetfiltptr = 0;
      gnofilter = 0;
      temp1[c*4+3] = tablemap[FTBL][temp1[c*4+3]];
      break;

      case CMD_SETFILTERCTRL:
      nosetfiltctrl = 0;
      gnofilter = 0;
      break;

      case CMD_SETFILTERCUTOFF:
      nosetfiltcutoff = 0;
      gnofilter = 0;
      break;

      case CMD_SETMASTERVOL:
      nosetmastervol = 0;
      // If no authorinfo being saved, erase timingmarks (not supported)
      if (!(playerversion & PLAYER_AUTHORINFO))
      {
        if (temp1[c*4+3] > 0x0f)
        {
          temp1[c*4+2] = 0;
          temp1[c*4+3] = 0;
        }
      }
      break;

      case CMD_FUNKTEMPO:
      nofunktempo = 0;
      temp1[c*4+3] = tablemap[STBL][temp1[c*4+3]];
      break;

      case CMD_SETTEMPO:
      if (temp1[c*4+3] >= 0x80) nochanneltempo = 0;
      else noglobaltempo = 0;
      // Decrease databyte of all tempo commands for the playroutine
      // Do not touch funktempo
      if ((temp1[c*4+3] & 0x7f) >= 3)
        temp1[c*4+3]--;
      break;

    }
  }

  if (noeffects)
  {
    command = 0;
    databyte = 0;
  }

  // Write in playroutine format
  for (c = 0; c < rows; c++)
  {
    // Instrument change with mapping
    if (temp1[c*4+1])
    {
      temp2[destsizeim++] = instrmap[INSTRCHG+temp1[c*4+1]];
    }
    // Rest+FX
    if (temp1[c*4] == REST)
    {
      if ((temp1[c*4+2] != command) || (temp1[c*4+3] != databyte))
      {
        command = temp1[c*4+2];
        databyte = temp1[c*4+3];
        temp2[destsizeim++] = FXONLY+command;
        if (command)
          temp2[destsizeim++] = databyte;
      }
      else
        temp2[destsizeim++] = REST;
    }
    else
    {
      // Normal note
      if ((temp1[c*4+2] != command) || (temp1[c*4+3] != databyte))
      {
        command = temp1[c*4+2];
        databyte = temp1[c*4+3];
        temp2[destsizeim++] = FX+command;
        if (command)
          temp2[destsizeim++] = databyte;
      }
      temp2[destsizeim++] = temp1[c*4];
    }
  }

  // Final step: optimize long singlebyte rests with "packed rest"
  for (c = 0; c < destsizeim;)
  {
    int packok = 1;

    // Never pack first row or sequencer goes crazy
    if (!c) packok = 0;
    
    // There must be no instrument or command changes on the row to be packed
    if (temp2[c] < FX)
    {
      dest[destsize++] = temp2[c++];
      packok = 0;
    }
    if ((temp2[c] >= FXONLY) && (temp2[c] < FIRSTNOTE))
    {
      int fxnum = temp2[c] - FXONLY;
      dest[destsize++] = temp2[c++];
      if (fxnum) dest[destsize++] = temp2[c++];
      packok = 0;
      goto NEXTROW;
    }
    if (temp2[c] < FXONLY)
     {
      int fxnum = temp2[c] - FX;
      dest[destsize++] = temp2[c++];
      if (fxnum) dest[destsize++] = temp2[c++];
      packok = 0;
    }

    if (temp2[c] != REST) packok = 0;

    if (!packok)
      dest[destsize++] = temp2[c++];
    else
    {
      for (d = c; d < destsizeim; )
      {
        if (temp2[d] == REST)
        {
          d++;
          if (d-c == 64) break;
        }
        else break;
      }
      d -= c;
      if (d > 1)
      {
        dest[destsize++] = -d;
        c += d;
      }
      else
        dest[destsize++] = temp2[c++];
    }
    NEXTROW: {}
  }
  // See if pattern too big
  if (destsize > 256) return -1;

  // If less than 256 bytes, insert endmark
  if (destsize < 256)
  dest[destsize++] = 0x00;

  return destsize;
}

int testoverlap(int area1start, int area1size, int area2start, int area2size)
{
  int area1last = area1start+area1size-1;
  int area2last = area2start+area2size-1;

  if (area1start == area2start) return 1;

  if (area1start < area2start)
  {
    if (area1last < area2start) return 0;
    else return 1;
  }
  else
  {
    if (area2last < area1start) return 0;
    else return 1;
  }
}

unsigned char swapnybbles(unsigned char n)
{
  unsigned char highnybble = n >> 4;
  unsigned char lownybble = n & 0xf;

  return (lownybble << 4) | highnybble;
}

int insertfile(char *name)
{
  int size;
  int handle = io_open(name);
  if (handle == -1) return 0;

  size = io_lseek(handle, 0, SEEK_END);
  io_lseek(handle, 0, SEEK_SET);
  while (size--)
  {
    membuf_append_char(&src, io_read8(handle));
  }
  io_close(handle);
  return 1;
}

void inserttext(const char *text)
{
  membuf_append(&src, text, strlen(text));
}

void insertdefine(const char *name, int value)
{
  char insertbuffer[80];

  sprintf(insertbuffer, "%-16s = %d\n", name, value);
  inserttext(insertbuffer);
}

void insertlabel(const char *name)
{
  char insertbuffer[80];

  sprintf(insertbuffer, "%s:\n", name);
  inserttext(insertbuffer);
}

void insertbytes(const unsigned char *bytes, int size)
{
  char insertbuffer[80];
  int row = 0;

  while (size--)
  {
    if (!row)
    {
      inserttext("                .BYTE (");
      sprintf(insertbuffer, "$%02x", *bytes);
      inserttext(insertbuffer);
      bytes++;
      row++;
    }
    else
    {
      sprintf(insertbuffer, ",$%02x", *bytes);
      inserttext(insertbuffer);
      bytes++;
      row++;
      if (row == MAX_BYTES_PER_ROW)
      {
        inserttext(")\n");
        row = 0;
      }
    }
  }
  if (row) inserttext(")\n");
}

void insertbyte(unsigned char byte)
{
  char insertbuffer[80];

  sprintf(insertbuffer, "                .BYTE ($%02x)\n", byte);
  inserttext(insertbuffer);
}

void insertaddrlo(const char *name)
{
  char insertbuffer[80];

  sprintf(insertbuffer, "                .BYTE (%s %% 256)\n", name);
  inserttext(insertbuffer);
}

void insertaddrhi(const char *name)
{
  char insertbuffer[80];

  sprintf(insertbuffer, "                .BYTE (%s / 256)\n", name);
  inserttext(insertbuffer);
}

void findtableduplicates(int num)
{
  int c,d,e;

  if (num == STBL)
  {
    for (c = 1; c <= MAX_TABLELEN; c++)
    {
      if (tableused[num][c])
      {
        for (d = c+1; d <= MAX_TABLELEN; d++)
        {
          if (tableused[num][d])
          {
            if ((ltable[num][d-1] == ltable[num][c-1]) && (rtable[num][d-1] == rtable[num][c-1]))
            {
              // Duplicate found, remove and map to the original
              tableused[num][d] = 0;
              for (e = d; e <= MAX_TABLELEN; e++)
                if (tableused[num][e]) tablemap[num][e]--;
              tablemap[num][d] = tablemap[num][c];
            }
          }
        }
      }
    }
  }
  else
  {
    for (c = 1; c <= MAX_TABLELEN; c++)
    {
      if (isusedandselfcontained(num, c))
      {
        for (d = c + gettablepartlen(num, c - 1); d <= MAX_TABLELEN; )
        {
          int len = gettablepartlen(num, d - 1);

          if (isusedandselfcontained(num, d))
          {
            for (e = 0; e < len; e++)
            {
              if (e < len-1)
              {
                // Is table data the same?
                if ((ltable[num][d+e-1] != ltable[num][c+e-1]) || (rtable[num][d+e-1] != rtable[num][c+e-1]))
                  break;
              }
              else
              {
                // Do both parts have a jump in the end?
                if (ltable[num][d+e-1] != ltable[num][c+e-1])
                  break;
                // Do both parts end?
                if (rtable[num][d+e-1] == 0)
                {
                  if (rtable[num][c+e-1] != 0)
                    break;
                }
                else
                {
                  // Do both parts loop in the same way?
                  if ((rtable[num][d+e-1] - d) != (rtable[num][c+e-1] - c))
                    break;
                }
              }
            }
            if (e == len)
            {
              // Duplicate found, remove and map to the original
              for (e = 0; e < len; e++)
                tableused[num][d+e] = 0;
              for (e = d; e < MAX_TABLELEN; e++)
                if (tableused[num][e]) tablemap[num][e] -= len;
              for (e = 0; e < len; e++)
                tablemap[num][d+e] = tablemap[num][c+e];
            }
          }
          d += len;
        }
      }
    }
  }
}

int isusedandselfcontained(int num, int start)
{
  int len = gettablepartlen(num, start - 1);
  int end = start + len - 1;
  int c;

  // Don't use jumps only
  if (len == 1) return 0;

  // Check that whole table is used
  for (c = start; c <= end; c++)
  {
    if (tableused[num][c] == 0) return 0;
  }
  // Check for jump to outside
  if (rtable[num][end-1] != 0)
  {
    if ((rtable[num][end-1] < start) || (rtable[num][end-1] > end)) return 0;
  }
  // Check for jump from outside
  for (c = 1; c < start; c++)
    if ((tableused[num][c]) && (ltable[num][c-1] == 0xff) && (rtable[num][c-1] >= start) && (rtable[num][c-1] <= end)) return 0;
  for (c = end+1; c <= MAX_TABLELEN; c++)
    if ((tableused[num][c]) && (ltable[num][c-1] == 0xff) && (rtable[num][c-1] >= start) && (rtable[num][c-1] <= end)) return 0;

  // OK!
  return 1;
}

void calcspeedtest(unsigned char pos)
{
  if (!pos) 
  {
    nozerospeed = 0;
    return;
  }

  if (ltable[STBL][pos-1] >= 0x80) nocalculatedspeed = 0;
  else nonormalspeed = 0;
}


