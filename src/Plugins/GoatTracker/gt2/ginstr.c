//
// GOATTRACKER v2 instrument editor
//

#define GINSTR_C

#include "goattrk2.h"

INSTR instrcopybuffer;
int cutinstr = -1;

int einum;
int eipos;
int eicolumn;

void instrumentcommands(void)
{
  switch(rawkey)
  {
    case 0x8:
    case KEY_DEL:
    if ((einum) && (shiftpressed) && (eipos < 9))
    {
      deleteinstrtable(einum);
      clearinstr(einum);
    }
    break;

    case KEY_X:
    if ((einum) && (shiftpressed) && (eipos < 9))
    {
      cutinstr = einum;
      memcpy(&instrcopybuffer, &ginstr[einum], sizeof(INSTR));
      clearinstr(einum);
    }
    break;

    case KEY_C:
    if ((einum) && (shiftpressed) && (eipos < 9))
    {
      cutinstr = -1;
      memcpy(&instrcopybuffer, &ginstr[einum], sizeof(INSTR));
    }
    break;

    case KEY_S:
    if ((einum) && (shiftpressed) && (eipos < 9))
    {
      memcpy(&ginstr[einum], &instrcopybuffer, sizeof(INSTR));
      if (cutinstr != -1)
      {
        int c, d;
        for (c = 0; c < MAX_PATT; c++)
        {
          for (d = 0; d < pattlen[c]; d++)
            if (pattern[c][d*4+1] == cutinstr) pattern[c][d*4+1] = einum;
        }
      }
    }
    break;

    case KEY_V:
    if ((einum) && (shiftpressed) && (eipos < 9))
    {
      memcpy(&ginstr[einum], &instrcopybuffer, sizeof(INSTR));
    }
    break;

    case KEY_RIGHT:
    if (eipos < 9)
    {
      eicolumn++;
      if (eicolumn > 1)
      {
        eicolumn = 0;
        eipos += 5;
        if (eipos >= 9) eipos -= 10;
        if (eipos < 0) eipos = 8;
      }
    }
    break;

    case KEY_LEFT:
    if (eipos < 9)
    {
      eicolumn--;
      if (eicolumn < 0)
      {
        eicolumn = 1;
        eipos -= 5;
        if (eipos < 0) eipos += 10;
        if (eipos >= 9) eipos = 8;
      }
    }
    break;

    case KEY_DOWN:
    if (eipos < 9)
    {
      eipos++;
      if (eipos > 8) eipos = 0;
    }
    break;

    case KEY_UP:
    if (eipos < 9)
    {
      eipos--;
      if (eipos < 0) eipos = 8;
    }
    break;

    case KEY_N:
    if ((eipos != 9) && (shiftpressed))
    {
      eipos = 9;
      return;
    }
    break;

    case KEY_U:
    if (shiftpressed)
    {
      etlock ^= 1;
      validatetableview();
    }
    break;

    case KEY_SPACE:
    if (eipos != 9)
    {
      if (!shiftpressed)
        playtestnote(FIRSTNOTE + epoctave * 12, einum, epchn);
      else
        releasenote(epchn);
    }
    break;

    case KEY_ENTER:
    if (!einum) break;
    switch(eipos)
    {
      case 2:
      case 3:
      case 4:
      case 5:
      {
        int pos;

        if (ginstr[einum].ptr[eipos-2])
        {
          if ((eipos == 5) && (shiftpressed))
          {
            ginstr[einum].ptr[STBL] = makespeedtable(ginstr[einum].ptr[STBL], finevibrato, 1) + 1;
            break;
          }
          pos = ginstr[einum].ptr[eipos-2] - 1;
        }
        else
        {
          pos = gettablelen(eipos-2);
          if (pos >= MAX_TABLELEN-1) pos = MAX_TABLELEN - 1;
          if (shiftpressed) ginstr[einum].ptr[eipos-2] = pos + 1;
        }
        gototable(eipos-2, pos);
      }
      return;

      case 9:
      eipos = 0;
      break;
    }
    break;
  }
  if ((eipos == 9) && (einum)) editstring(ginstr[einum].name, MAX_INSTRNAMELEN);
  if ((hexnybble >= 0) && (eipos < 9) && (einum))
  {
    unsigned char *ptr = &ginstr[einum].ad;
    ptr += eipos;

    switch(eicolumn)
    {
      case 0:
      *ptr &= 0x0f;
      *ptr |= hexnybble << 4;
      eicolumn++;
      break;

      case 1:
      *ptr &= 0xf0;
      *ptr |= hexnybble;
      eicolumn++;
      if (eicolumn > 1)
      {
        eicolumn = 0;
        eipos++;
        if (eipos >= 9) eipos = 0;
      }
      break;
    }
  }
  // Validate instrument parameters
  if (einum)
  {
    if (!(ginstr[einum].gatetimer & 0x3f)) ginstr[einum].gatetimer |= 1;
  }
}


void clearinstr(int num)
{
  memset(&ginstr[num], 0, sizeof(INSTR));
  if (num)
  {
    if (multiplier)
      ginstr[num].gatetimer = 2 * multiplier;
    else
      ginstr[num].gatetimer = 1;

    ginstr[num].firstwave = 0x9;
  }
}

void gotoinstr(int i)
{
  if (i < 0) return;
  if (i >= MAX_INSTR) return;

  einum = i;
  showinstrtable();

  editmode = EDIT_INSTRUMENT;
}

void nextinstr(void)
{
  einum++;
  if (einum >= MAX_INSTR) einum = MAX_INSTR - 1;
  showinstrtable();
}

void previnstr(void)
{
  einum--;
  if (einum < 0) einum = 0;
  showinstrtable();
}

void showinstrtable(void)
{
  if (!etlock)
  {
    int c;

    for (c = MAX_TABLES-1; c >= 0; c--)
    {
      if (ginstr[einum].ptr[c])
        settableviewfirst(c, ginstr[einum].ptr[c] - 1);
    }
  }
}

