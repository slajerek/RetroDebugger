//
// GOATTRACKER v2 screen display routines
//

#define GDISPLAY_C

#include "goattrk2.h"

char *notename[] =
 {"C-0", "C#0", "D-0", "D#0", "E-0", "F-0", "F#0", "G-0", "G#0", "A-0", "A#0", "B-0",
  "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
  "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
  "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",
  "C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",
  "C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5",
  "C-6", "C#6", "D-6", "D#6", "E-6", "F-6", "F#6", "G-6", "G#6", "A-6", "A#6", "B-6",
  "C-7", "C#7", "D-7", "D#7", "E-7", "F-7", "F#7", "G-7", "G#7", "...", "---", "+++"};
  
char timechar[] = {':', ' '};

int timemin = 0;
int timesec = 0;
int timeframe = 0;

void printmainscreen(void)
{
  clearscreen();
  printstatus();
  fliptoscreen();
}

void displayupdate(void)
{
  if (cursorflashdelay >= 6)
  {
    cursorflashdelay %= 6;
    cursorflash++;
    cursorflash &= 3;
  }
  printstatus();
  fliptoscreen();
}

void printstatus(void)
{
  int c, d, color, color2;
  int cc = cursorcolortable[cursorflash];
#ifndef __MACOSX__
  menu = 0;
#endif

  if ((mouseb > MOUSEB_LEFT) && (mousey <= 1) && (!eamode))
  {
#ifndef __MACOSX__
    menu = 1;
#else
    if (!prevmouseb)
      menu ^= 1;
#endif
  }

  printblankc(0, 0, 15+16, MAX_COLUMNS);

  if (!menu)
  {
    if (!strlen(loadedsongfilename))
      sprintf(textbuffer, "%s", programname);
    else
      sprintf(textbuffer, "%s - %s", programname, loadedsongfilename);
    textbuffer[49] = 0;
    printtext(0, 0, 15+16, textbuffer);

    if (usefinevib)
      printtext(40+10, 0, 15+16, "FV");

    if (optimizepulse)
      printtext(43+10, 0, 15+16, "PO");

    if (optimizerealtime)
      printtext(46+10, 0, 15+16, "RO");

    if (ntsc)
      printtext(49+10, 0, 15+16, "NTSC");
    else
      printtext(49+10, 0, 15+16, " PAL");

    if (!sidmodel)
      printtext(54+10, 0, 15+16, "6581");
    else
      printtext(54+10, 0, 15+16, "8580");

    sprintf(textbuffer, "HR:%04X", adparam);
    printtext(59+10, 0, 15+16, textbuffer);
    if (eamode) printbg(62+10+eacolumn, 0, cc, 1);

    if (multiplier)
    {
      sprintf(textbuffer, "%2dX", multiplier);
      printtext(67+10, 0, 15+16, textbuffer);
    }
    else printtext(67+10, 0, 15+16, "25Hz");

    printtext(72+20, 0, 15+16, "F12=HELP");
  }
  else
  {
    printtext(0, 0, 15+16, " PLAY | PLAYPOS | PLAYPATT | STOP | LOAD | SAVE | PACK/RL | HELP | CLEAR | QUIT |");
  }

  if ((followplay) && (isplaying()))
  {
    for (c = 0; c < MAX_CHN; c++)
    {
      int newpos = chn[c].pattptr / 4;
      if (chn[c].advance) epnum[c] = chn[c].pattnum;

      if (newpos > pattlen[epnum[c]]) newpos = pattlen[epnum[c]];

      if (c == epchn)
      {
        eppos = newpos;
        epview = newpos-VISIBLEPATTROWS/2;
      }

      newpos = chn[c].songptr;
      newpos--;
      if (newpos < 0) newpos = 0;
      if (newpos > songlen[esnum][c]) newpos = songlen[esnum][c];

      if ((c == eschn) && (chn[c].advance))
      {
        eseditpos = newpos;
        if (newpos - esview < 0)
        {
          esview = newpos;
        }
        if (newpos - esview >= VISIBLEORDERLIST)
        {
          esview = newpos - VISIBLEORDERLIST + 1;
        }
      }
    }
  }

  for (c = 0; c < MAX_CHN; c++)
  {
    sprintf(textbuffer, "CHN %d PATT.%02X", c+1, epnum[c]);
    printtext(2+c*15, 2, CTITLE, textbuffer);

    for (d = 0; d < VISIBLEPATTROWS; d++)
    {
      int p = epview+d;
      color = CNORMAL;
      if ((epnum[c] == chn[c].pattnum) && (isplaying()))
      {
        int chnrow = chn[c].pattptr / 4;
        if (chnrow > pattlen[chn[c].pattnum]) chnrow = pattlen[chn[c].pattnum];
        if (chnrow == p) color = CPLAYING;
      }

      if (chn[c].mute) color = CMUTE;
      if (p == eppos) color = CEDIT;
      if ((p < 0) || (p > pattlen[epnum[c]]))
      {
        sprintf(textbuffer, "             ");
      }
      else
      {
        if (!(patterndispmode & 1))
        {
          if (p < 100)
            sprintf(textbuffer, " %02d", p);
          else
            sprintf(textbuffer, "%03d", p);
        }
        else
          sprintf(textbuffer, " %02X", p);

        if (pattern[epnum[c]][p*4] == ENDPATT)
        {
          sprintf(&textbuffer[3], " PATT. END");
          if (color == CNORMAL) color = CCOMMAND;
        }
        else
        {
          sprintf(&textbuffer[3], " %s %02X%01X%02X",
            notename[pattern[epnum[c]][p*4]-FIRSTNOTE],
            pattern[epnum[c]][p*4+1],
            pattern[epnum[c]][p*4+2],
            pattern[epnum[c]][p*4+3]);
          if (patterndispmode & 2)
          {
            if (!pattern[epnum[c]][p*4+1])
              memset(&textbuffer[8], '.', 2);
            if (!pattern[epnum[c]][p*4+2])
              memset(&textbuffer[10], '.', 3);
          }
        }
      }
      textbuffer[3] = 0;
      if (p%stepsize)
      {
        printtext(2+c*15, 3+d, CNORMAL, textbuffer);
      }
      else
      {
        printtext(2+c*15, 3+d, CCOMMAND, textbuffer);
      }
      if (color == CNORMAL)
        color2 = CCOMMAND;
      else
        color2 = color;
      printtext(6+c*15, 3+d, color2, &textbuffer[4]);
      printtext(10+c*15, 3+d, color, &textbuffer[8]);
      printtext(12+c*15, 3+d, color2, &textbuffer[10]);
      printtext(13+c*15, 3+d, color, &textbuffer[11]);
      if (c == epmarkchn)
      {
        if (epmarkstart <= epmarkend)
        {
          if ((p >= epmarkstart) && (p <= epmarkend))
            printbg(2+c*15+4, 3+d, 1, 9);
        }
        else
        {
          if ((p <= epmarkstart) && (p >= epmarkend))
            printbg(2+c*15+4, 3+d, 1, 9);
        }
      }
      if ((color == CEDIT) && (editmode == EDIT_PATTERN) && (epchn == c))
      {
        switch(epcolumn)
        {
          case 0:
          if (!eamode) printbg(2+c*15+4, 3+d, cc, 3);
          break;

          default:
          if (!eamode) printbg(2+c*15+7+epcolumn, 3+d, cc, 1);
          break;
        }
      }
    }
  }

  sprintf(textbuffer, "CHN ORDERLIST (SUBTUNE %02X, POS %02X)", esnum, eseditpos);
  printtext(40+10, 2, CTITLE, textbuffer);
  for (c = 0; c < MAX_CHN; c++)
  {
    sprintf(textbuffer, " %d ", c+1);
    printtext(40+10, 3+c, 15, textbuffer);
    for (d = 0; d < VISIBLEORDERLIST; d++)
    {
      int p = esview+d;
      color = CNORMAL;
      if (isplaying())
      {
        int chnpos = chn[c].songptr;
        chnpos--;
        if (chnpos < 0) chnpos = 0;
        if ((p == chnpos) && (chn[c].advance)) color = CPLAYING;
      }
      if (p == espos[c]) color = CEDIT;
      if ((esend[c]) && (p == esend[c])) color = CEDIT;

      if ((p < 0) || (p > (songlen[esnum][c]+1)) || (p > MAX_SONGLEN+1))
      {
        sprintf(textbuffer, "   ");
      }
      else
      {
        if (songorder[esnum][c][p] < LOOPSONG)
        {
          if ((songorder[esnum][c][p] < REPEAT) || (p >= songlen[esnum][c]))
          {
            sprintf(textbuffer, "%02X ", songorder[esnum][c][p]);
            if ((p >= songlen[esnum][c]) && (color == CNORMAL)) color = CCOMMAND;
          }
          else
          {
            if (songorder[esnum][c][p] >= TRANSUP)
            {
              sprintf(textbuffer, "+%01X ", songorder[esnum][c][p]&0xf);
              if (color == CNORMAL) color = CCOMMAND;
            }
            else
            {
              if (songorder[esnum][c][p] >= TRANSDOWN)
              {
                sprintf(textbuffer, "-%01X ", 16-(songorder[esnum][c][p] & 0x0f));
                if (color == CNORMAL) color = CCOMMAND;
              }
              else
              {
                sprintf(textbuffer, "R%01X ", (songorder[esnum][c][p]+1) & 0x0f);
                if (color == CNORMAL) color = CCOMMAND;
              }
            }
          }
        }
        if (songorder[esnum][c][p] == LOOPSONG)
        {
          sprintf(textbuffer, "RST");
          if (color == CNORMAL) color = CCOMMAND;
        }
      }
      printtext(44+10+d*3, 3+c, color, textbuffer);
      if (c == esmarkchn)
      {
        if (esmarkstart <= esmarkend)
        {
          if ((p >= esmarkstart) && (p <= esmarkend))
          {
            if (p != esmarkend)
              printbg(44+10+d*3, 3+c, 1, 3);
            else
              printbg(44+10+d*3, 3+c, 1, 2);
          }
        }
        else
        {
          if ((p <= esmarkstart) && (p >= esmarkend))
          {
            if (p != esmarkstart)
              printbg(44+10+d*3, 3+c, 1, 3);
            else
              printbg(44+10+d*3, 3+c, 1, 2);
          }
        }
      }
      if ((p == eseditpos) && (editmode == EDIT_ORDERLIST) && (eschn == c))
      {
        if (!eamode) printbg(44+10+d*3+escolumn, 3+c, cc, 1);
      }
    }
  }

  sprintf(textbuffer, "INSTRUMENT NUM. %02X  %-16s", einum, ginstr[einum].name);
  printtext(40+10, 7, CTITLE, textbuffer);

  sprintf(textbuffer, "Attack/Decay    %02X", ginstr[einum].ad);
  if (eipos == 0) color = CEDIT; else color = CNORMAL;
  printtext(40+10, 8, color, textbuffer);

  sprintf(textbuffer, "Sustain/Release %02X", ginstr[einum].sr);
  if (eipos == 1) color = CEDIT; else color = CNORMAL;
  printtext(40+10, 9, color, textbuffer);

  sprintf(textbuffer, "Wavetable Pos   %02X", ginstr[einum].ptr[WTBL]);
  if (eipos == 2) color = CEDIT; else color = CNORMAL;
  printtext(40+10, 10, color, textbuffer);

  sprintf(textbuffer, "Pulsetable Pos  %02X", ginstr[einum].ptr[PTBL]);
  if (eipos == 3) color = CEDIT; else color = CNORMAL;
  printtext(40+10, 11, color, textbuffer);

  sprintf(textbuffer, "Filtertable Pos %02X", ginstr[einum].ptr[FTBL]);
  if (eipos == 4) color = CEDIT; else color = CNORMAL;
  printtext(40+10, 12, color, textbuffer);

  sprintf(textbuffer, "Vibrato Param   %02X", ginstr[einum].ptr[STBL]);
  if (eipos == 5) color = CEDIT; else color = CNORMAL;
  printtext(60+10, 8, color, textbuffer);

  sprintf(textbuffer, "Vibrato Delay   %02X", ginstr[einum].vibdelay);
  if (eipos == 6) color = CEDIT; else color = CNORMAL;
  printtext(60+10, 9, color, textbuffer);

  sprintf(textbuffer, "HR/Gate Timer   %02X", ginstr[einum].gatetimer);
  if (eipos == 7) color = CEDIT; else color = CNORMAL;
  printtext(60+10, 10, color, textbuffer);

  sprintf(textbuffer, "1stFrame Wave   %02X", ginstr[einum].firstwave);
  if (eipos == 8) color = CEDIT; else color = CNORMAL;
  printtext(60+10, 11, color, textbuffer);

  if (editmode == EDIT_INSTRUMENT)
  {
    if (eipos < 9)
    {
      if (!eamode) printbg(56+10+eicolumn+20*(eipos/5), 8+(eipos%5), cc, 1);
    }
    else
    {
      if (!eamode) printbg(60+10+strlen(ginstr[einum].name), 7, cc, 1);
    }
  }

  sprintf(textbuffer, "WAVE TBL  PULSETBL  FILT.TBL  SPEEDTBL");
  printtext(40+10, 14, CTITLE, textbuffer);

  for (c = 0; c < MAX_TABLES; c++)
  {
    for (d = 0; d < VISIBLETABLEROWS; d++)
    {
      int p = etview[c]+d;

      color = CNORMAL;
      switch (c)
      {
        case WTBL:
        if (ltable[c][p] >= WAVECMD) color = CCOMMAND;
        break;

        case PTBL:
        if (ltable[c][p] >= 0x80) color = CCOMMAND;
        break;

        case FTBL:
        if ((ltable[c][p] >= 0x80) || ((!ltable[c][p]) && (rtable[c][p]))) color = CCOMMAND;
        break;
      }
      if ((p == etpos) && (etnum == c)) color = CEDIT;
      sprintf(textbuffer, "%02X:%02X %02X", p+1, ltable[c][p], rtable[c][p]);
      printtext(40+10+10*c, 15+d, color, textbuffer);

      if (etmarknum == c)
      {
        if (etmarkstart <= etmarkend)
        {
          if ((p >= etmarkstart) && (p <= etmarkend))
            printbg(40+10+10*c+3, 15+d, 1, 5);
        }
        else
        {
          if ((p <= etmarkstart) && (p >= etmarkend))
            printbg(40+10+10*c+3, 15+d, 1, 5);
        }
      }
    }
  }

  if (editmode == EDIT_TABLES)
  {
    if (!eamode) printbg(43+10+etnum*10+(etcolumn & 1)+(etcolumn/2)*3, 15+etpos-etview[etnum], cc, 1);
  }

  printtext(40+10, 31, CTITLE, "NAME   ");
  sprintf(textbuffer, "%-32s", songname);
  printtext(47+10, 31, CEDIT, textbuffer);

  printtext(40+10, 32, CTITLE, "AUTHOR ");
  sprintf(textbuffer, "%-32s", authorname);
  printtext(47+10, 32, CEDIT, textbuffer);

  printtext(40+10, 33, CTITLE, "COPYR. ");
  sprintf(textbuffer, "%-32s", copyrightname);
  printtext(47+10, 33, CEDIT, textbuffer);

  if ((editmode == EDIT_NAMES) && (!eamode))
  {
    switch(enpos)
    {
      case 0:
      printbg(47+10+strlen(songname), 31, cc, 1);
      break;
      case 1:
      printbg(47+10+strlen(authorname), 32, cc, 1);
      break;
      case 2:
      printbg(47+10+strlen(copyrightname), 33, cc, 1);
      break;
    }
  }
  sprintf(textbuffer, "OCTAVE %d", epoctave);
  printtext(0, 35, CTITLE, textbuffer);

  switch(autoadvance)
  {
    case 0:
    color = 10;
    break;

    case 1:
    color = 14;
    break;

    case 2:
    color = 12;
    break;
  }

  if (recordmode) printtext(0, 36, color, "EDITMODE");
  else printtext(0, 36, color, "JAM MODE");

  if (isplaying()) printtext(10, 35, CTITLE, "PLAYING");
  else printtext(10, 35, CTITLE, "STOPPED");
  if (multiplier)
  {
    if (!ntsc)
      sprintf(textbuffer, " %02d%c%02d ", timemin, timechar[timeframe/(25*multiplier) & 1], timesec);
    else
      sprintf(textbuffer, " %02d%c%02d ", timemin, timechar[timeframe/(30*multiplier) & 1], timesec);
  }
  else
  {
    if (!ntsc)
      sprintf(textbuffer, " %02d%c%02d ", timemin, timechar[(timeframe/13) & 1], timesec);
    else
      sprintf(textbuffer, " %02d%c%02d ", timemin, timechar[(timeframe/15) & 1], timesec);
  }

  printtext(10, 36, CEDIT, textbuffer);

  printtext(80, 35, CTITLE, " CHN1   CHN2   CHN3 ");
  for (c = 0; c < MAX_CHN; c++)
  {
    int chnpos = chn[c].songptr;
    int chnrow = chn[c].pattptr/4;
    chnpos--;
    if (chnpos < 0) chnpos = 0;
    if (chnrow > pattlen[chn[c].pattnum]) chnrow = pattlen[chn[c].pattnum];
    if (chnrow >= 100) chnrow -= 100;

    sprintf(textbuffer, "%03d/%02d",
      chnpos,chnrow);
    printtext(80+7*c, 36, CEDIT, textbuffer);
  }

  if (etlock) printtext(78, 36, CTITLE, " ");
    else printtext(78, 36, CTITLE, "U");
}


void resettime(void)
{
  timemin = 0;
  timesec = 0;
  timeframe = 0;
}

void incrementtime(void)
{
  {
    timeframe++;
    if (!ntsc)
    {
      if (((multiplier) && (timeframe >= PALFRAMERATE*multiplier))
          || ((!multiplier) && (timeframe >= PALFRAMERATE/2)))
      {
        timeframe = 0;
        timesec++;
      }
    }
    else
    {
      if (((multiplier) && (timeframe >= NTSCFRAMERATE*multiplier))
          || ((!multiplier) && (timeframe >= NTSCFRAMERATE/2)))
      {
        timeframe = 0;
        timesec++;
      }
    }
    if (timesec == 60)
    {
      timesec = 0;
      timemin++;
      timemin %= 60;
    }
  }
}

