#ifndef _CGT2SidRegisters_H_
#define _CGT2SidRegisters_H_

#include "SYS_Defs.h"

// SID register access for the GT2 SID state view.
//
// Reading is trivial -- gsid.cpp keeps the live register file in
// sidreg[NUMSIDREGS] and ships it to reSID once per audio buffer.
//
// Writing is NOT. GT2's playroutine() runs every frame whether the song is
// playing or stopped (gtsound_playrout() calls it unconditionally; only
// incrementtime() is gated on songinit != PLAY_STOPPED), and it rebuilds most
// of sidreg[] from the player's own state. So poking sidreg[] directly would
// be overwritten within one frame -- pausing does not help.
//
// Instead a write is reverse-mapped onto the "ghost" variables playroutine()
// derives that register from, so the player itself carries the new value
// forward:
//
//   $00/$01 +7c -> chn[c].freq            (gplay.c:1183-1184)
//   $02/$03 +7c -> chn[c].pulse           (gplay.c:1185-1186)
//   $04    +7c -> chn[c].wave & .gate     (gplay.c:1187)
//   $05/$06 +7c -> sidreg[] directly -- the player only touches ADSR on note
//                  init / hard restart / command, never per frame
//   $15        -> nothing; playroutine() forces 0x00 (gplay.c:491), GT2 does
//                  not use filter cutoff low bits
//   $16        -> filtercutoff            (gplay.c:492)
//   $17        -> filterctrl              (gplay.c:493)
//   $18        -> filtertype | masterfader (gplay.c:494)
//
// Nothing here modifies gplay.c or the GT2 text-mode UI.

#define GT2_NUM_SID_REGISTERS 0x19

// Live value as the player last emitted it.
u8 GT2_GetSidRegister(int registerNum);

// Applies `value` to the ghost state behind `registerNum` so it survives the
// next playroutine() pass. Out-of-range registers are ignored.
void GT2_SetSidRegister(int registerNum, u8 value);

// True when writing this register actually sticks for more than one frame.
// $15 never does (the player hard-codes it to 0), and $16..$18 do not while a
// filter table program is running -- filterptr != 0 means playroutine()
// recomputes cutoff/ctrl/type every frame from the table.
bool GT2_IsSidRegisterWriteEffective(int registerNum);

#endif
