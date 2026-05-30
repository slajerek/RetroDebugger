/** \file   archdep_stat.h
 * \brief   Simplified stat(2) call - header
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
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

#ifndef ARCHDEP_STAT_H
#define ARCHDEP_STAT_H

#include <stddef.h>

/* Visual Studio doesn't provide these macros. */
#ifdef _MSC_VER
#define S_ISDIR(_m)  (((_m) & S_IFMT) == _S_IFDIR)
#define S_ISFIFO(_m) (((_m) & S_IFMT) == _S_IFIFO)
#define S_ISCHR(_m)  (((_m) & S_IFMT) == _S_IFCHR)
#define S_ISBLK(_m)  (((_m) & S_IFMT) == _S_IFBLK)
#define S_ISREG(_m)  (((_m) & S_IFMT) == _S_IFREG)
#endif

int archdep_stat(const char *filename, size_t *len, unsigned int *isdir);

#endif
