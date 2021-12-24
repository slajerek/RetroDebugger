/*
 * hardsid.c - hardsid.c wrapper for the sdl ui.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * This file is part of VICE, modified from the sidplay2 sources.  It is
 * a one for all driver with real timing support via real time kernel
 * extensions or through the hardware buffering.  It supports the hardsid
 * isa/pci single/quattro and also the catweasel MK3/4.
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

#include "vice.h"

 /*
// note this is added to the source tree instead

#ifdef HAVE_HARDSID

#ifdef WINMIPS
#define UINT_PTR unsigned int
#endif

#ifdef UNIX_COMPILE
#include "../unix/hardsid.c"
#endif

//#if defined(WIN32_COMPILE) && !defined(__XBOX__)
//#include "../win32/hardsid.c"
//#endif

#ifdef AMIGA_SUPPORT
#include "../amigaos/hardsid.c"
#endif

#endif

*/

