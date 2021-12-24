//
// GOATTRACKER v2 online help
//

#define GHELP_C

#include "goattrk2.h"

#define HELP_HEADER 15
#define HELP_NORMAL 7

int printrows(int column, int row, int color, char *strings[] ) {
  int n = 0;
  while(strings[n]) {
    printtext(column, row++, color, strings[n++]);
  }
  return row;
}

void onlinehelp(int standalone,int context)
{
  char *genkeys[] = {
    "F1  Play from beginning",
    "F2  Play from current position",
    "F3  Play current pattern",
    "F4  Stop playing",
    "F5  Go to pattern editor",
    "F6  Go to orderlist editor",
    "F7  Go to instrument/table editor",
    "F8  Go to songname editor",
    "F9  Pack, relocate & save PRG,SID etc.",
    "F10 Load song/instrument",
    "F11 Save song/instrument",
    "F12 This screen",
    "SHIFT+F1-F3 Follow play begin/pos/patt.",
    "SHIFT+F4 Mute current channel",
    "SHIFT+F5-F6 Change speed multiplier",
    "SHIFT+F7 Change hardrestart ADSR",
    "SHIFT+F8 Switch between 6581/8580 SID",
    "SHIFT+F10 Merge-load song",
    "SHIFT+, . Move song startpos & restart",
    "TAB Cycle between editing modes",
    "INS Insert row (Press on endmark to",
    "DEL Delete row change patt. length)",
#ifdef __MACOSX__    
    "ALT+DEL emulates INS on Mac OS X",
#endif    
    "SHIFT+ESC Clear/optimize all musicdata",
    "ESC Exit program",
    NULL
  };

  char *patternkeys[] = {
    "Enter notes like on piano (PT or DMC)",
    "0-9 & A-F to enter commands",
    "SPC Switch between jam/editmode",
    "BACKSPC Insert rest",
    "RET Keyoff (/w SHIFT = Keyon)",
    "- + Select instrument",
    "/ * Select octave",
    "< > Select pattern",
    "BACKQUOTE Select channel",
    "SHIFT+SPC Play from cursor pos",
    "SHIFT+CRSR Mark pattern",
    "SHIFT+Q,W Transpose half/octave up",
    "SHIFT+A,S Transpose half/octave down",
    "SHIFT+E,R Copy,paste effects",
    "SHIFT+H Make hifi vib/portaspeed",
    "SHIFT+I Invert selection/pattern",
    "SHIFT+J,K Join/split pattern",
    "SHIFT+L Mark/unmark whole pattern",
    "SHIFT+M,N Choose highlighting step",
    "SHIFT+O,P Shrink/expand pattern",
    "SHIFT+X,C,V Cut,copy,paste pattern",
    "SHIFT+Z Cycle autoadvance-mode",
    "SHIFT+1,2,3 Mute channel",
    NULL
  };

  char *songkeys[] = {
    "0-9 & A-F to enter pattern numbers",
    "SPC Set start position for F2 key",
    "BACKSPC Set end position for F2 key",
    "RET Go to pattern (/w SHIFT=all chns.)",
    "< > Select subtune",
    "- + Insert transpose down/up command",
    "SHIFT+CRSR LEFT/RIGHT Mark orderlist",
    "SHIFT+L Mark/unmark whole orderlist",
    "SHIFT+R Insert repeat command",
    "SHIFT+X,C,V Cut,copy,paste orderlist",
    "SHIFT+1,2,3 Swap orderlist with chn.",
    NULL
  };

  char *instkeys[] = {
    "0-9 & A-F to enter parameters",
    "SPC Play test note",
    "SHIFT+SPC Silence test note",
    "RET Go to table",
    "- + Select instrument",
    "/ * Select octave",
    "BACKQUOTE Select table",
    "SHIFT+CRSR Mark table",
    "SHIFT+Q,W Trans. speed half/octave up",
    "SHIFT+A,S Trans. speed half/octave down",
    "SHIFT+L Convert pulse/filter limit",
    "SHIFT+N Edit name/negate value or note",
    "SHIFT+O Optimize table (remove unused)",
    "SHIFT+R Convert absolute/relative note",
    "SHIFT+S ""Smart"" instrument paste",
    "SHIFT+U Unlock/lock table view",
    "SHIFT+X,C,V Cut,copy,paste instr./table",
    "SHIFT+DEL Delete instrument+tabledata",
    "SHIFT+RET Convert vibrato parameter",
    NULL
  };

  char *pattcmds[] = {
    "                                                                               ",
    "Command 0XY: Do nothing. Databyte will always be 00.                           ",
    "                                                                               ",
    "Command 1XY: Portamento up. XY is index to a 16-bit speed in speedtable.       ",
    "                                                                               ",
    "Command 2XY: Portamento down. XY is index to a 16-bit speed in speedtable.     ",
    "                                                                               ",
    "Command 3XY: Toneportamento. Raise or lower pitch until target note has been   ",
    "             reached. XY is index to a 16-bit speed or 00 for ""tie note"".      ",
    "                                                                               ",
    "Command 4XY: Vibrato. XY is index to speedtable. Left side value determines how",
    "             long until the direction changes (speed) and right side value is  ",
    "             the amount of pitch change each tick (depth).                     ",
    "                                                                               ",
    "Command 5XY: Set attack/decay register to value XY.                            ",
    "                                                                               ",
    "Command 6XY: Set sustain/release register to value XY.                         ",
    "                                                                               ",
    "Command 7XY: Set waveform register to value XY. If a wavetable is actively     ",
    "             changing the channel's waveform at the same time, will be         ",
    "             ineffective.                                                      ",
    "                                                                               ",
    "Command 8XY: Set wavetable pointer. 00 stops wavetable execution.              ",
    "                                                                               ",
    "Command 9XY: Set pulsetable pointer. 00 stops pulsetable execution.            ",
    "                                                                               ",
    "Command AXY: Set filtertable pointer. 00 stops filtertable execution.          ",
    "                                                                               ",
    "Command BXY: Set filter control. X is resonance and Y is channel bitmask.      ",
    "             00 turns filter off and also stops filtertable execution.         ",
    "                                                                               ",
    "Command CXY: Set filter cutoff to XY. Can be ineffective if the filtertable is ",
    "             active and also changing the cutoff.                              ",
    "                                                                               ",
    "Command DXY: Set mastervolume to Y, if X is 0. If X is not 0, value XY is      ",
    "             copied to the timing mark location, which is playeraddress+$3F.   ",
    "                                                                               ",
    "Command EXY: Funktempo. XY is an index to speedtable. Will alternate left side ",
    "             and right side tempo values on each pattern step.                 ",
    "                                                                               ",
    "Command FXY: Set tempo. Values 03-7F set tempo on all channels, values 83-FF   ",
    "             only on current channel (subtract 80 to get actual tempo). Tempos ",
    "             00 and 01 recall the funktempos set by EXY command.               ",
    NULL
  };

  char *instparm[] = {
    "                                                                               ",
    "Attack/Decay          0 is fastest attack or decay, F is slowest               ",
    "                                                                               ",
    "Sustain/Release       Sustain level 0 is silent and F is the loudest. Release  ",
    "                      behaves like Attack & Decay (F slowest).                 ",
    "                                                                               ",
    "Wavetable Pos         Wavetable startposition. Value 00 stops the wavetable    ",
    "                      execution and is not very useful.                        ",
    "                                                                               ",
    "Pulsetable Pos        Pulsetable startposition. Value 00 will leave pulse      ",
    "                      execution untouched.                                     ",
    "                                                                               ",
    "Filtertable Pos       Filtertable startposition. Value 00 will leave filter    ",
    "                      execution untouched. In most cases it makes sense to have",
    "                      a filter-controlling instrument only on one channel at a ",
    "                      time.                                                    ",
    "                                                                               ",
    "Vibrato Param         Instrument vibrato parameters. An index to the speed-    ",
    "                      table, see command 4XY.                                  ",
    "                                                                               ",
    "Vibrato Delay         How many ticks until instrument vibrato starts. Value 00 ",
    "                      turns instrument vibrato off.                            ",
    "                                                                               ",
    "HR/Gate Timer         How many ticks before note start note fetch, gateoff and ",
    "                      hard restart happen. Can be at most tempo-1. So on tempo ",
    "                      4 highest acceptable value is 3. Bitvalue 80 disables    ",
    "                      hard restart and bitvalue 40 disables gateoff.           ",
    "                                                                               ",
    "1stFrame Wave         Waveform used on init frame of the note, usually 09 (gate",
    "                      + testbit). Values 00, FE and FF have special meaning:   ",
    "                      leave waveform unchanged and additionally set gate off   ",
    "                      (FE), gate on (FF), or gate unchanged (00).              ",
    NULL
  };

  char *tables[] = {
    "                                                                               ",
    "Wavetable left side:  00    Leave waveform unchanged                           ",
    "                      01-0F Delay this step by 1-15 frames                     ",
    "                      10-DF Waveform values                                    ",
    "                      E0-EF Inaudible waveform values 00-0F                    ",
    "                      F0-FE Execute command 0XY-EXY. Right side is parameter   ",
    "                      FF    Jump. Right side tells jump position (00 = stop)   ",
    "                                                                               ",
    "Wavetable right side: 00-5F Relative notes                                     ",
    "                      60-7F Negative relative notes (lower pitch)              ",
    "                      80    Keep frequency unchanged                           ",
    "                      81-DF Absolute notes C#0 - B-7                           ",
    "                                                                               ",
    "Pulsetable left side: 01-7F Pulse modulation step. Left side indicates time and",
    "                            right side the speed (8bit signed value).          ",
    "                      8X-FX Set pulse width. X is the high 4 bits, right side  ",
    "                            tells the 8 low bits.                              ",
    "                      FF    Jump. Right side tells jump position (00 = stop)   ",
    "                                                                               ",
    "Filt.table left side: 00    Set cutoff, indicated by right side                ",
    "                      01-7F Filter modulation step. Left side indicates time   ",
    "                            and right side the speed (signed 8bit value)       ",
    "                      80-F0 Set filter parameters. Left side high nybble tells ",
    "                            the passband (90 = lowpass, A0 = bandpass etc.) and",
    "                            right side tells resonance/channel bitmask, as in  ",
    "                            command BXY.                                       ",
    "                      FF    Jump. Right side tells jump position (00 = stop)   ",
    "                                                                               ",
    "Speedtbl. vibrato:    XX YY Left side tells how long until vibrato direction   ",
    "                            changes (speed), right side is the value added to  ",
    "                            pitch each tick (depth).                           ",
    "                                                                               ",
    "Speedtbl. portamento: XX YY A 16-bit value added to pitch each tick. Left side ",
    "                            is the MSB and the right side the LSB.             ",
    "                                                                               ",
    "Speedtbl. funktempo:  XX YY Two 8-bit tempo values that are alternated on each ",
    "                            pattern row, starting from the left side.          ",
    "                                                                               ",
    "For both vibrato and portamento, if XX has the high bit ($80) set, note        ",
    "independent vibrato depth / portamento speed calculation is enabled, and YY    ",
    "specifies the divisor (higher value -> lower result and more rastertime taken).",
    NULL
  };

  int hview = -1;

  int lastrow=0;

  // Close menu once online help exits
  menu = 0;
  
  for (;;)
  {
    int left = hview + 2;
    int right = hview + 2;

    clearscreen();
    if(!context) {
      printtext(0, left++, HELP_HEADER, "GENERAL KEYS");
      left = printrows(0,left,HELP_NORMAL, genkeys);
      left++;

      printtext(40,right++, HELP_HEADER, "PATTERN EDIT MODE");
      right = printrows(40,right,HELP_NORMAL, patternkeys);
      right++;
    
      printtext(0, left++, HELP_HEADER, "SONG EDIT MODE");
      left = printrows(0,left,HELP_NORMAL, songkeys);
      left++;

      printtext(0, left++, HELP_HEADER, "SONGNAME EDIT MODE");
      printtext(0, left++, HELP_NORMAL, "Use cursor UP/DOWN to change rows");
      left++;

      printtext(40,right++, HELP_HEADER, "INSTRUMENT/TABLE EDIT MODE");
      right = printrows(40,right,HELP_NORMAL, instkeys);
      right++;
    
      left = (left<right ? right : left);
    
      printtext(0, left++, HELP_HEADER, "PATTERN COMMANDS");
      left = printrows(0,left,HELP_NORMAL, pattcmds);
      left++;

      printtext(0, left++, HELP_HEADER, "INSTRUMENT PARAMETERS");
      left = printrows(0,left,HELP_NORMAL, instparm);
      left++;

      printtext(0, left++, HELP_HEADER,"TABLES");
      left = printrows(0,left,HELP_NORMAL, tables);
      left++;
    } else {
      switch(editmode) {
      case EDIT_PATTERN:      
        printtext(0,left++, HELP_HEADER, "PATTERN EDIT MODE");
        left = printrows(0,left,HELP_NORMAL, patternkeys);
        left++;
        printtext(0, left++, HELP_HEADER, "PATTERN COMMANDS");
        left = printrows(0,left,HELP_NORMAL, pattcmds);
        left++;
        break;
      case EDIT_ORDERLIST:
        printtext(0, left++, HELP_HEADER, "SONG EDIT MODE");
        left = printrows(0,left,HELP_NORMAL, songkeys);
        left++;
        break;
      case EDIT_INSTRUMENT:
        printtext(0,left++, HELP_HEADER, "INSTRUMENT/TABLE EDIT MODE");
        left = printrows(0,left,HELP_NORMAL, instkeys);
        left++;
        printtext(0, left++, HELP_HEADER, "INSTRUMENT PARAMETERS");
        left = printrows(0,left,HELP_NORMAL, instparm);
        left++;
        break;
      case EDIT_NAMES:
        printtext(0, left++, HELP_HEADER, "SONGNAME EDIT MODE");
        printtext(0, left++, HELP_NORMAL, "Use cursor UP/DOWN to change rows");
        left++;
        break;
      case EDIT_TABLES:
        printtext(0,left++, HELP_HEADER, "INSTRUMENT/TABLE EDIT MODE");
        left = printrows(0,left++,HELP_NORMAL, instkeys);
        left++;
        break;
      default:
        break;
      }

    }

    if(!lastrow) lastrow=left;

    printblank(0, 0, MAX_COLUMNS);
    sprintf(textbuffer, "%s Online Help", programname);
    printtext(0, 0, HELP_HEADER, textbuffer);
    if(standalone) {
      printtext(55, 0, HELP_HEADER, "Arrows/PgUp/PgDn/Home/End scroll, ESC exits");
    } else {
      printtext(34, 0, HELP_HEADER, "Arrows/PgUp/PgDn/Home/End scroll, F12 toggles context, others exit");
    }
    printbg(0, 0, 1, MAX_COLUMNS);

    fliptoscreen();
    waitkeymousenoupdate();


    if (win_quitted)
    {
      exitprogram = 1;
      break;
    }

    switch(rawkey)
    {
      case KEY_LEFT:
      case KEY_UP:
        hview++;
        break;

      case KEY_RIGHT:
      case KEY_DOWN:
        hview--;
        break;

      case KEY_PGUP:
        hview+=PGUPDNREPEAT;
        break;

      case KEY_PGDN:
        hview-=PGUPDNREPEAT;
        break;

      case KEY_HOME:
      hview = -1;
      break;

      case KEY_END:
      hview = -(lastrow-MAX_ROWS+1);
      break;

      case KEY_F12:
        context = !context;
        hview = -1;
        lastrow = 0;
        continue;
        break;

    case KEY_ESC:
      goto EXITHELP;
      break;

      default:
      if (rawkey && !standalone) goto EXITHELP;

      break;
    }

    if ((mouseb) && (mousey == 1)) hview++;
    if ((mouseb) && (mousey == MAX_ROWS-1)) hview--;

    if (hview > -1) hview = -1;
    if (hview < -(lastrow-MAX_ROWS+1)) hview = -(lastrow-MAX_ROWS+1);
    if ((mouseb) && (!prevmouseb) && (!mousey)) break;
  }
  EXITHELP: ;
  if(!standalone) {
    printmainscreen();
    key = 0;
    rawkey = 0;
  }
}

