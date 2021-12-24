/*
    $Id$

    psid64 - PlaySID player for a real C64 environment
    Copyright (C) 2001-2003  Roland Hermans <rolandh@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef RELOC65_H
#define RELOC65_H

//////////////////////////////////////////////////////////////////////////////
//                             I N C L U D E S
//////////////////////////////////////////////////////////////////////////////

#include <map>
#include <string>


//////////////////////////////////////////////////////////////////////////////
//                  F O R W A R D   D E C L A R A T O R S
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//                     D A T A   D E C L A R A T O R S
//////////////////////////////////////////////////////////////////////////////

typedef std::map<std::string,int> globals_t;


//////////////////////////////////////////////////////////////////////////////
//                  F U N C T I O N   D E C L A R A T O R S
//////////////////////////////////////////////////////////////////////////////

extern int              reloc65 (char **buf, int *fsize, int addr, globals_t *globals);

#endif // RELOC65_H
