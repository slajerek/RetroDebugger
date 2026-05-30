/*
 * types.h - Type definitions for VICE.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andr� Fachat <a.fachat@physik.tu-chemnitz.de>
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_TYPES_H
#define VICE_TYPES_H

#include "vice.h"

#include "vice_sdl.h"

/* VICE 3.10 reconciliation: 3.10 uses uint*_t and bool pervasively, and includes
   <stdint.h>/<stdbool.h> unconditionally in its types.h. Provide them here so every
   file that includes vicetypes.h (the renamed types.h) sees them. The BYTE/WORD/DWORD
   shims below remain for slajerek's ungated c64d_ code. */
#include <stdint.h>
#include <stdbool.h>

#ifndef BYTE
//#ifdef WIN32_COMPILE
typedef unsigned char BYTE;
typedef unsigned short WORD;

#ifdef WIN32
typedef unsigned long DWORD;
#else
typedef unsigned int DWORD;
#endif

typedef signed char SIGNED_CHAR;

#ifndef SWORD
typedef signed short SWORD;
#endif

//typedef signed long SDWORD;
typedef signed int SDWORD;
//#else
//#define BYTE Uint8
//#define SIGNED_CHAR Sint8
//#define WORD Uint16
//#define SWORD Sint16
//#define DWORD Uint32
//#define SDWORD Sint32
//#endif
#endif

#include <inttypes.h>	/* VICE 3.10: PRIx64/PRIu64 for printing 64-bit CLOCK -- needed on all platforms, not just LINUX (was #ifdef LINUX in slajerek's 3.1) */

/* VICE 3.10: MSVC <2019 lacks PRIu64/PRIx64, which 3.10 uses to print CLOCK. Provide fallbacks (no-op where <inttypes.h> already defines them). */
#ifndef PRIu64
#  ifdef _WIN32
#    define PRIu64 "llu"
#  else
#    define PRIu64 "lu"
#  endif
#endif
#ifndef PRIx64
#  ifdef _WIN32
#    define PRIx64 "llx"
#  else
#    define PRIx64 "lx"
#  endif
#endif

typedef uint64_t CLOCK;   /* VICE 3.10: CLOCK is 64-bit (was DWORD/32-bit); 3.10's
                             clock-overflow handling that replaced clkguard assumes this. */
/* Maximum value of a CLOCK.  */
#define CLOCK_MAX (~((CLOCK)0))

/* VICE 3.10: int<->ptr helper (RD already has int_to_void_ptr; 3.10 code uses
   vice_int_to_ptr). intptr_t cast works for both 32/64-bit. */
#define vice_int_to_ptr(x) ((void *)(intptr_t)(x))
#define vice_uint_to_ptr(x) ((void *)(uintptr_t)(x))	/* VICE 3.10 */

#ifdef _WIN64
#define vice_ptr_to_int(x) ((int)(long long)(x))
#define vice_ptr_to_uint(x) ((unsigned int)(unsigned long long)(x))
#define int_to_void_ptr(x) ((void *)(long long)(x))
#define uint_to_void_ptr(x) ((void *)(unsigned long long)(x))
#else
#define vice_ptr_to_int(x) ((int)(long)(x))
#define vice_ptr_to_uint(x) ((unsigned int)(unsigned long)(x))
#define int_to_void_ptr(x) ((void *)(long)(x))
#define uint_to_void_ptr(x) ((void *)(unsigned long)(x))
#endif

#endif
