//
// GOATTRACKER v2 table editor
//

#define GTABLE_C

#include "goattrk2.h"

unsigned char ltablecopybuffer[MAX_TABLELEN];
unsigned char rtablecopybuffer[MAX_TABLELEN];
int tablecopyrows = 0;

int etview[MAX_TABLES];
int etnum;
int etpos;
int etcolumn;
int etlock = 1;
int etmarknum = -1;
int etmarkstart;
int etmarkend;

void tablecommands(void)
{
  int c;

  switch(rawkey)
  {
    case KEY_Q:
    if ((shiftpressed) && (etnum == STBL))
    {
      int speed = (ltable[etnum][etpos] << 8) | rtable[etnum][etpos];
      speed *= 34716;
      speed /= 32768;
      if (speed > 65535) speed = 65535;

      ltable[etnum][etpos] = speed >> 8;
      rtable[etnum][etpos] = speed & 0xff;
    }
    break;

    case KEY_A:
    if ((shiftpressed) && (etnum == STBL))
    {
      int speed = (ltable[etnum][etpos] << 8) | rtable[etnum][etpos];
      speed *= 30929;
      speed /= 32768;

      ltable[etnum][etpos] = speed >> 8;
      rtable[etnum][etpos] = speed & 0xff;
    }
    break;
    
    case KEY_W:
    if ((shiftpressed) && (etnum == STBL))
    {
      int speed = (ltable[etnum][etpos] << 8) | rtable[etnum][etpos];
      speed *= 2;
      if (speed > 65535) speed = 65535;

      ltable[etnum][etpos] = speed >> 8;
      rtable[etnum][etpos] = speed & 0xff;
    }
    if ((shiftpressed) && ((etnum == PTBL) || (etnum == FTBL)) && (ltable[etnum][etpos] < 0x80))
    {
      int speed = (signed char)(rtable[etnum][etpos]);
      speed *= 2;

      if (speed > 127) speed = 127;
      if (speed < -128) speed = -128;
      rtable[etnum][etpos] = speed;
    }
    break;
    
    case KEY_S:
    if ((shiftpressed) && (etnum == STBL))
    {
      int speed = (ltable[etnum][etpos] << 8) | rtable[etnum][etpos];
      speed /= 2;

      ltable[etnum][etpos] = speed >> 8;
      rtable[etnum][etpos] = speed & 0xff;
    }
    if ((shiftpressed) && ((etnum == PTBL) || (etnum == FTBL)) && (ltable[etnum][etpos] < 0x80))
    {
      int speed = (signed char)(rtable[etnum][etpos]);
      speed /= 2;

      rtable[etnum][etpos] = speed;
    }
    break;

    case KEY_SPACE:
    if (!shiftpressed)
      playtestnote(FIRSTNOTE + epoctave * 12, einum, epchn);
    else
      releasenote(epchn);
    break;

    case KEY_RIGHT:
    etcolumn++;
    if (etcolumn > 3)
    {
      etpos -= etview[etnum];
      etcolumn = 0;
      etnum++;
      if (etnum >= MAX_TABLES) etnum = 0;
      etpos += etview[etnum];
    }
    if (shiftpressed) etmarknum = -1;
    break;

    case KEY_LEFT:
    etcolumn--;
    if (etcolumn < 0)
    {
      etpos -= etview[etnum];
      etcolumn = 3;
      etnum--;
      if (etnum < 0) etnum = MAX_TABLES - 1;
      etpos += etview[etnum];
    }
    if (shiftpressed) etmarknum = -1;
    break;

    case KEY_HOME:
    while (etpos != 0) tableup();
    break;
    
    case KEY_END:
    while (etpos != MAX_TABLELEN-1) tabledown();
    break;

    case KEY_PGUP:
    for (c = 0; c < PGUPDNREPEAT; c++) tableup();
    break;

    case KEY_PGDN:
    for (c = 0; c < PGUPDNREPEAT; c++) tabledown();
    break;

    case KEY_UP:
    tableup();
    break;

    case KEY_DOWN:
    tabledown();
    break;

    case KEY_X:
    case KEY_C:
    if (shiftpressed)
    {
      if (etmarknum != -1)
      {
        int d = 0;
        if (etmarkstart <= etmarkend)
        {
          for (c = etmarkstart; c <= etmarkend; c++)
          {
            ltablecopybuffer[d] = ltable[etmarknum][c];
            rtablecopybuffer[d] = rtable[etmarknum][c];
            if (rawkey == KEY_X)
            {
              ltable[etmarknum][c] = 0;
              rtable[etmarknum][c] = 0;
            }
            d++;
          }
        }
        else
        {
          for (c = etmarkend; c <= etmarkstart; c++)
          {
            ltablecopybuffer[d] = ltable[etmarknum][c];
            rtablecopybuffer[d] = rtable[etmarknum][c];
            if (rawkey == KEY_X)
            {
              ltable[etmarknum][c] = 0;
              rtable[etmarknum][c] = 0;
            }
            d++;
          }
        }
        tablecopyrows = d;
      }
      etmarknum = -1;
    }
    break;

    case KEY_V:
    if (shiftpressed)
    {
      if (tablecopyrows)
      {
        for (c = 0; c < tablecopyrows; c++)
        {
          ltable[etnum][etpos] = ltablecopybuffer[c];
          rtable[etnum][etpos] = rtablecopybuffer[c];
          etpos++;
          if (etpos >= MAX_TABLELEN) etpos = MAX_TABLELEN-1;
        }
      }
    }
    break;

    case KEY_O:
    if (shiftpressed) optimizetable(etnum);
    break;

    case KEY_U:
    if (shiftpressed)
    {
      etlock ^= 1;
      validatetableview();
    }
    break;

    case KEY_R:
    if (etnum == WTBL)
    {
      if (ltable[etnum][etpos] != 0xff)
      {
        // Convert absolute pitch to relative pitch or vice versa
        int basenote = epoctave * 12;
        int note = rtable[etnum][etpos];

        if (note >= 0x80)
        {
          note -= basenote;
          note &= 0x7f;
        }
        else
        {
          note += basenote;
          note |= 0x80;
        }

        rtable[etnum][etpos] = note;
      }
    }

    case KEY_L:
    if (etnum == PTBL)
    {
      int c;
      int currentpulse = -1;
      int targetpulse = ltable[etnum][etpos] << 4;
      int speed = rtable[etnum][etpos];
      int time;
      int steps;

      if (!speed) break;

      // Follow the chain of pulse commands backwards to the nearest set command so we know what current pulse is
      for (c = etpos-1; c >= 0; c--)
      {
        if (ltable[etnum][c] == 0xff) break;
        if (ltable[etnum][c] >= 0x80)
        {
          currentpulse = (ltable[etnum][c] << 8) | rtable[etnum][c];
          currentpulse &= 0xfff;
          break;
        }
      }
      if (currentpulse == -1) break;

      // Then follow the chain of modulation steps
      for (; c < etpos; c++)
      {
        if (ltable[etnum][c] < 0x80)
        {
          currentpulse += ltable[etnum][c] * (rtable[etnum][c] & 0xff);
          if (rtable[etnum][c] >= 0x80) currentpulse -= 256 * ltable[etnum][c];
          currentpulse &= 0xfff;
        }
      }

      time = abs(targetpulse - currentpulse) / speed;
      if (speed < 128)
        steps = (time + 126) / 127;
      else
        steps = time;

      if (!steps) break;
      if (etpos + steps > MAX_TABLELEN) break;
      if (targetpulse < currentpulse) speed = -speed;

      // Make room in the table
      for (c = steps; c > 1; c--) inserttable(etnum, etpos, 1);

      while (time)
      {
        if (abs(speed) < 128)
        {
          if (time < 127) ltable[etnum][etpos] = time;
            else ltable[etnum][etpos] = 127;
          rtable[etnum][etpos] = speed;
          time -= ltable[etnum][etpos];
          etpos++;
        }
        else
        {
          currentpulse += speed;
          ltable[etnum][etpos] = 0x80 | ((currentpulse >> 8) & 0xf);
          rtable[etnum][etpos] = currentpulse & 0xff;
          time--;
          etpos++;
        }
      }
    }
    if (etnum == FTBL)
    {
      int c;
      int currentfilter = -1;
      int targetfilter = ltable[etnum][etpos];
      int speed = rtable[etnum][etpos] & 0x7f;
      int time;
      int steps;

      if (!speed) break;

      // Follow the chain of filter commands backwards to the nearest set command so we know what current pulse is
      for (c = etpos-1; c >= 0; c--)
      {
        if (ltable[etnum][c] == 0xff) break;
        if (ltable[etnum][c] == 0x00)
        {
          currentfilter = rtable[etnum][c];
          break;
        }
      }
      if (currentfilter == -1) break;

      // Then follow the chain of modulation steps
      for (; c < etpos; c++)
      {
        if (ltable[etnum][c] < 0x80)
        {
          currentfilter += ltable[etnum][c] * rtable[etnum][c];
          currentfilter &= 0xff;
        }
      }

      time = abs(targetfilter - currentfilter) / speed;
      steps = (time + 126) / 127;
      if (!steps) break;
      if (etpos + steps > MAX_TABLELEN) break;
      if (targetfilter < currentfilter) speed = -speed;

      // Make room in the table
      for (c = steps; c > 1; c--) inserttable(etnum, etpos, 1);

      while (time)
      {
        if (time < 127) ltable[etnum][etpos] = time;
          else ltable[etnum][etpos] = 127;
        rtable[etnum][etpos] = speed;
        time -= ltable[etnum][etpos];
        etpos++;
      }
    }
    break;

    case KEY_N:
    if (shiftpressed)
    {
      switch (etnum)
      {
        // Negate pulse or filter speed
        case FTBL:
        if (!ltable[etnum][etpos]) break;
        case PTBL:
        if (ltable[etnum][etpos] < 0x80)
          rtable[etnum][etpos] = (rtable[etnum][etpos] ^ 0xff) + 1;
        break;

        // Negate relative note
        case WTBL:
        if ((ltable[etnum][etpos] != 0xff) && (rtable[etnum][etpos] < 0x80))
          rtable[etnum][etpos] = (0x80 - rtable[etnum][etpos]) & 0x7f;
        break;
      }
    }
    break;

    case KEY_DEL:
#ifdef __MACOSX__
    if (altpressed)
        inserttable(etnum, etpos, shiftpressed);
    else
#endif
        deletetable(etnum, etpos);
    break;

    case KEY_INS:
    inserttable(etnum, etpos, shiftpressed);
    break;

    case KEY_ENTER:
    if (etnum == WTBL)
    {
      int table = -1;
      int mstmode = MST_PORTAMENTO;

      switch (ltable[etnum][etpos])
      {
        case WAVECMD + CMD_PORTAUP:
        case WAVECMD + CMD_PORTADOWN:
        case WAVECMD + CMD_TONEPORTA:
        table = STBL;
        break;

        case WAVECMD + CMD_VIBRATO:
        table = STBL;
        mstmode = finevibrato;
        break;

        case WAVECMD + CMD_FUNKTEMPO:
        table = STBL;
        mstmode = MST_FUNKTEMPO;
        break;

        case WAVECMD + CMD_SETPULSEPTR:
        table = PTBL;
        break;

        case WAVECMD + CMD_SETFILTERPTR:
        table = FTBL;
        break;
      }
      switch (table)
      {
        default:
        editmode = EDIT_INSTRUMENT;
        eipos = etnum + 2;
        return;

        case STBL:
        if (rtable[etnum][etpos])
        {
          if (!shiftpressed)
          {
            gototable(STBL, rtable[etnum][etpos] - 1);
            return;
          }
          else
          {
            int oldeditpos = etpos;
            int oldeditcolumn = etcolumn;
            int pos = makespeedtable(rtable[etnum][etpos], mstmode, 1);
            gototable(WTBL, oldeditpos);
            etcolumn = oldeditcolumn;

            rtable[etnum][etpos] = pos + 1;
            return;
          }
        }
        else
        {
          int pos = findfreespeedtable();
          if (pos >= 0)
          {
            rtable[etnum][etpos] = pos + 1;
            gototable(STBL, pos);
            return;
          }
        }
        break;

        case PTBL:
        case FTBL:
        if (rtable[etnum][etpos])
        {
          gototable(table, rtable[etnum][etpos] - 1);
          return;
        }
        else
        {
          if (shiftpressed)
          {
            int pos = gettablelen(table);
            if (pos >= MAX_TABLELEN-1) pos = MAX_TABLELEN - 1;
            rtable[etnum][etpos] = pos + 1;
            gototable(table, pos);
            return;
          }
        }
      }
    }
    else
    {
      editmode = EDIT_INSTRUMENT;
      eipos = etnum + 2;
      return;
    }
    break;
    
    case KEY_APOST2:
    if (shiftpressed)
    {
      etpos -= etview[etnum];
      etnum--;
      if (etnum < 0) etnum = MAX_TABLES-1;
      etpos += etview[etnum];
    }
    else
    {
      etpos -= etview[etnum];
      etnum++;
      if (etnum >= MAX_TABLES) etnum = 0;
      etpos += etview[etnum];
    }
  }

  if (hexnybble >= 0)
  {
    switch(etcolumn)
    {
      case 0:
      ltable[etnum][etpos] &= 0x0f;
      ltable[etnum][etpos] |= hexnybble << 4;
      break;
      case 1:
      ltable[etnum][etpos] &= 0xf0;
      ltable[etnum][etpos] |= hexnybble;
      break;
      case 2:
      rtable[etnum][etpos] &= 0x0f;
      rtable[etnum][etpos] |= hexnybble << 4;
      break;
      case 3:
      rtable[etnum][etpos] &= 0xf0;
      rtable[etnum][etpos] |= hexnybble;
      break;
    }
    etcolumn++;
    if (etcolumn > 3)
    {
      etcolumn = 0;
      etpos++;
      if (etpos >= MAX_TABLELEN) etpos = MAX_TABLELEN-1;
    }
  }

  validatetableview();
}

void deletetable(int num, int pos)
{
  int c, d;

  // Shift tablepointers in instruments
  for (c = 1; c < MAX_INSTR; c++)
  {
    if ((ginstr[c].ptr[num]-1) > pos) ginstr[c].ptr[num]--;
  }

  // Shift tablepointers in wavetable commands
  for (c = 0; c < MAX_TABLELEN; c++)
  {
    if ((ltable[WTBL][c] >= WAVECMD) && (ltable[WTBL][c] <= WAVELASTCMD))
    {
      int cmd = ltable[WTBL][c] & 0xf;

      if (num < STBL)
      {
        if (cmd == CMD_SETWAVEPTR+num)
        {
          if ((rtable[WTBL][c]-1) > pos) rtable[WTBL][c]--;
        }
      }
      else
      {
        if ((cmd == CMD_FUNKTEMPO) || ((cmd >= CMD_PORTAUP) && (cmd <= CMD_VIBRATO)))
        {
          if ((rtable[WTBL][c]-1) > pos) rtable[WTBL][c]--;
        }
      }
    }
  }

  // Shift tablepointers in patterns
  for (c = 0; c < MAX_PATT; c++)
  {
    for (d = 0; d <= MAX_PATTROWS; d++)
    {
      if (num < STBL)
      {
        if (pattern[c][d*4+2] == CMD_SETWAVEPTR+num)
        {
          if ((pattern[c][d*4+3]-1) > pos) pattern[c][d*4+3]--;
        }
      }
      else
      {
        if ((pattern[c][d*4+2] == CMD_FUNKTEMPO) ||
           ((pattern[c][d*4+2] >= CMD_PORTAUP) && (pattern[c][d*4+2] <= CMD_VIBRATO)))
        {
          if ((pattern[c][d*4+3]-1) > pos) pattern[c][d*4+3]--;
        }
      }
    }
  }

  // Shift jumppointers in the table itself
  for (c = 0; c < MAX_TABLELEN; c++)
  {
    if (num != STBL)
    {
      if (ltable[num][c] == 0xff)
        if ((rtable[num][c]-1) > pos) rtable[num][c]--;
    }
  }

  for (c = pos; c < MAX_TABLELEN; c++)
  {
    if (c+1 < MAX_TABLELEN)
    {
      ltable[num][c] = ltable[num][c+1];
      rtable[num][c] = rtable[num][c+1];
    }
    else
    {
      ltable[num][c] = 0;
      rtable[num][c] = 0;
    }
  }
}

void inserttable(int num, int pos, int mode)
{
  int c, d;

  // Shift tablepointers in instruments
  for (c = 1; c < MAX_INSTR; c++)
  {
    if (!mode)
    {
      if ((ginstr[c].ptr[num]-1) >= pos) ginstr[c].ptr[num]++;
    }
    else
    {
      if ((ginstr[c].ptr[num]-1) > pos) ginstr[c].ptr[num]++;
    }
  }

  // Shift tablepointers in wavetable commands
  for (c = 0; c < MAX_TABLELEN; c++)
  {
    if ((ltable[WTBL][c] >= WAVECMD) && (ltable[WTBL][c] <= WAVELASTCMD))
    {
      int cmd = ltable[WTBL][c] & 0xf;

      if (num < STBL)
      {
        if (cmd == CMD_SETWAVEPTR+num)
        {
          if (!mode)
          {
            if ((rtable[WTBL][c]-1) >= pos) rtable[WTBL][c]++;
          }
          else
          {
            if ((rtable[WTBL][c]-1) > pos) rtable[WTBL][c]++;
          }
        }
      }
      else
      {
        if ((cmd == CMD_FUNKTEMPO) || ((cmd >= CMD_PORTAUP) && (cmd <= CMD_VIBRATO)))
        {
          if (!mode)
          {
            if ((rtable[WTBL][c]-1) >= pos) rtable[WTBL][c]++;
          }
          else
          {
            if ((rtable[WTBL][c]-1) > pos) rtable[WTBL][c]++;
          }
        }
      }
    }
  }


  // Shift tablepointers in patterns
  for (c = 0; c < MAX_PATT; c++)
  {
    for (d = 0; d <= MAX_PATTROWS; d++)
    {
      if (num < STBL)
      {
        if (pattern[c][d*4+2] == CMD_SETWAVEPTR+num)
        {
          if (!mode)
          {
            if ((pattern[c][d*4+3]-1) >= pos) pattern[c][d*4+3]++;
          }
          else
          {
            if ((pattern[c][d*4+3]-1) > pos) pattern[c][d*4+3]++;
          }
        }
      }
      else
      {
        if ((pattern[c][d*4+2] == CMD_FUNKTEMPO) ||
           ((pattern[c][d*4+2] >= CMD_PORTAUP) && (pattern[c][d*4+2] <= CMD_VIBRATO)))
        {
          if (!mode)
          {
            if ((pattern[c][d*4+3]-1) >= pos) pattern[c][d*4+3]++;
          }
          else
          {
            if ((pattern[c][d*4+3]-1) > pos) pattern[c][d*4+3]++;
          }
        }
      }
    }
  }

  // Shift jumppointers in the table itself
  if (num != STBL)
  {
    for (c = 0; c < MAX_TABLELEN; c++)
    {
      if (ltable[num][c] == 0xff)
      {
        if (!mode)
        {
          if ((rtable[num][c]-1) >= pos) rtable[num][c]++;
        }
        else
        {
          if ((rtable[num][c]-1) > pos) rtable[num][c]++;
        }
      }
    }
  }

  for (c = MAX_TABLELEN-1; c >= pos; c--)
  {
    if (c > pos)
    {
      ltable[num][c] = ltable[num][c-1];
      rtable[num][c] = rtable[num][c-1];
    }
    else
    {
      if ((num == WTBL) && (mode == 1))
      {
        ltable[num][c] = 0xe9;
        rtable[num][c] = 0;
      }
      else
      {
        ltable[num][c] = 0;
        rtable[num][c] = 0;
      }
    }
  }
}

int gettablelen(int num)
{
  int c;

  for (c = MAX_TABLELEN-1; c >= 0; c--)
  {
    if (ltable[num][c] | rtable[num][c]) break;
  }
  return c+1;
}

int gettablepartlen(int num, int pos)
{
  int c;

  if (pos < 0) return 0;
  if (num == STBL) return 1;

  for (c = pos; c < MAX_TABLELEN; c++)
  {
    if (ltable[num][c] == 0xff)
    {
      c++;
      break;
    }
  }
  return c-pos;
}

void optimizetable(int num)
{
  int c,d;

  memset(tableused, 0, sizeof tableused);

  for (c = 0; c < MAX_PATT; c++)
  {
    for (d = 0; d < pattlen[c]; d++)
    {
      if ((pattern[c][d*4+2] >= CMD_SETWAVEPTR) && (pattern[c][d*4+2] <= CMD_SETFILTERPTR))
        exectable(pattern[c][d*4+2] - CMD_SETWAVEPTR, pattern[c][d*4+3]);
      if ((pattern[c][d*4+2] >= CMD_PORTAUP) && (pattern[c][d*4+2] <= CMD_VIBRATO))
        exectable(STBL, pattern[c][d*4+3]);
      if (pattern[c][d*4+2] == CMD_FUNKTEMPO)
        exectable(STBL, pattern[c][d*4+3]);
    }
  }

  for (c = 0; c < MAX_INSTR; c++)
  {
    for (d = 0; d < MAX_TABLES; d++)
    {
      exectable(d, ginstr[c].ptr[d]);
    }
  }

  for (c = 0; c < MAX_TABLELEN; c++)
  {
    if (tableused[WTBL][c+1])
    {
      if ((ltable[WTBL][c] >= WAVECMD) && (ltable[WTBL][c] <= WAVELASTCMD))
      {
        d = -1;

        switch(ltable[WTBL][c] - WAVECMD)
        {
          case CMD_PORTAUP:
          case CMD_PORTADOWN:
          case CMD_TONEPORTA:
          case CMD_VIBRATO:
          d = STBL;
          break;

          case CMD_SETPULSEPTR:
          d = PTBL;
           break;

           case CMD_SETFILTERPTR:
           d = FTBL;
          break;
        }

        if (d != -1) exectable(d, rtable[WTBL][c]);
      }
    }
  }

  for (c = MAX_TABLELEN-1; c >= 0; c--)
  {
    if ((ltable[num][c]) || (rtable[num][c])) break;
  }
  for (; c >= 0; c--)
  {
    if (!tableused[num][c+1]) deletetable(num, c);
  }
}

int makespeedtable(unsigned data, int mode, int makenew)
{
  int c;
  unsigned char l = 0, r = 0;

  if (!data) return -1;

  switch (mode)
  {
    case MST_NOFINEVIB:
    l = (data & 0xf0) >> 4;
    r = (data & 0x0f) << 4;
    break;

    case MST_FINEVIB:
    l = (data & 0x70) >> 4;
    r = ((data & 0x0f) << 4) | ((data & 0x80) >> 4);
    break;

    case MST_FUNKTEMPO:
    l = (data & 0xf0) >> 4;
    r = data & 0x0f;
    break;

    case MST_PORTAMENTO:
    l = (data << 2) >> 8;
    r = (data << 2) & 0xff;
    break;
    
    case MST_RAW:
    r = data & 0xff;
    l = data >> 8;
    break;
  }

  if (makenew == 0)
  {
    for (c = 0; c < MAX_TABLELEN; c++)
    {
      if ((ltable[STBL][c] == l) && (rtable[STBL][c] == r))
        return c;
    }
  }

  for (c = 0; c < MAX_TABLELEN; c++)
  {
    if ((!ltable[STBL][c]) && (!rtable[STBL][c]))
    {
      ltable[STBL][c] = l;
      rtable[STBL][c] = r;

      settableview(STBL, c);
      return c;
    }
  }
  return -1;
}

void deleteinstrtable(int i)
{
  int c,d;
  int eraseok = 1;

  for (c = 0; c < MAX_TABLES; c++)
  {
    if (ginstr[i].ptr[c])
    {
      int pos = ginstr[i].ptr[c]-1;
      int len = gettablepartlen(c, pos);

      // Check that this table area isn't used by another instrument
      for (d = 1; d < MAX_INSTR; d++)
      {
        if ((d != i) && (ginstr[d].ptr[c]))
        {
          int cmppos = ginstr[d].ptr[c]-1;
          if ((cmppos >= pos) && (cmppos < pos+len)) eraseok = 0;
        }
      }
      if (eraseok)
        while (len--) deletetable(c, pos);
    }
  }
}

void gototable(int num, int pos)
{
  editmode = EDIT_TABLES;
  settableview(num, pos);
}

void settableview(int num, int pos)
{
  etnum = num;
  etcolumn = 0;
  etpos = pos;

  validatetableview();
}

void settableviewfirst(int num, int pos)
{
  etview[num] = pos;
  settableview(num, pos);
}
void validatetableview(void)
{
  if (etpos - etview[etnum] < 0)
    etview[etnum] = etpos;
  if (etpos - etview[etnum] >= VISIBLETABLEROWS)
    etview[etnum] = etpos - VISIBLETABLEROWS + 1;

  // Table view lock?
  if (etlock)
  {
    int c;

    for (c = 0; c < MAX_TABLES; c++) etview[c] = etview[etnum];
  }
}

void tableup(void)
{
  if (shiftpressed)
  {
    if ((etmarknum != etnum) || (etpos != etmarkend))
    {
      etmarknum = etnum;
      etmarkstart = etmarkend = etpos;
    }
  }
  etpos--;
  if (etpos < 0) etpos = 0;
  if (shiftpressed) etmarkend = etpos;
}

void tabledown(void)
{
  if (shiftpressed)
  {
    if ((etmarknum != etnum) || (etpos != etmarkend))
    {
      etmarknum = etnum;
      etmarkstart = etmarkend = etpos;
    }
  }
  etpos++;
  if (etpos >= MAX_TABLELEN) etpos = MAX_TABLELEN-1;
  if (shiftpressed) etmarkend = etpos;
}

void exectable(int num, int ptr)
{
  // Jump error check
  if ((num != STBL) && (ptr) && (ptr <= MAX_TABLELEN))
  {
    if (ltable[num][ptr-1] == 0xff)
    {
      tableerror = TYPE_JUMP;
      return;
    }
  }

  for (;;)
  {
    // Exit when table stopped
    if (!ptr) break;
    // Overflow check
    if ((num != STBL) && (ptr > MAX_TABLELEN))
    {
      tableerror = TYPE_OVERFLOW;
      break;
    }
    // If were already here, exit
    if (tableused[num][ptr]) break;
    // Mark current position used
    tableused[num][ptr] = 1;
    // Go to next ptr.
    if (num != STBL)
    {
      if (ltable[num][ptr-1] == 0xff)
      {
        ptr = rtable[num][ptr-1];
      }
      else ptr++;
    }
    else break;
  }
}

int findfreespeedtable(void)
{
  int c;
  for (c = 0; c < MAX_TABLELEN; c++)
  {
    if ((!ltable[STBL][c]) && (!rtable[STBL][c]))
    {
      return c;
    }
  }
  return -1;
}

