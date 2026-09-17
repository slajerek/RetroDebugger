/* [C64D-REFERENCE-LIVE-EXCEPTION]
 * Vendored from VICE 3.10 (arch/shared/socketdrv/socketimpl.h + the unix and
 * win32 impl headers, collapsed into one file). LIVE code: root/socket.c
 * includes it whenever HAVE_NETWORK is defined (IDE64 USB server, netplay).
 * Platform dispatch is on _WIN32 because our vice-config.h defines
 * UNIX_COMPILE unconditionally on every platform.
 * See src/Emulators/REFERENCE_FILES.md.
 */
/*
 * socketimpl.h - Socket-specific stuff.
 *
 * Written by
 *  Spiro Trikaliotis <spiro.trikaliotis@gmx.de>
 *
 * based on code from network.c written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
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

#ifndef VICE_SOCKETIMPL_H
#define VICE_SOCKETIMPL_H

#include "vice.h"

#ifdef HAVE_NETWORK

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

typedef unsigned long in_addr_t;

/* Used by vice_network_get_errorcode() (socket.c). Upstream defines it in
   arch/shared/archdep_win32.h; our arch/archdep_win32.h is a REFERENCE-ONLY
   copy that lacks it, so it lives here with the rest of the live socket
   platform layer. */
#ifndef ARCHDEP_SOCKET_ERROR
#define ARCHDEP_SOCKET_ERROR WSAGetLastError()
#endif

#else /* unix / macOS */

#if !defined(HAVE_GETDTABLESIZE) && defined(HAVE_GETRLIMIT)
#include <sys/resource.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>

typedef int SOCKET;
typedef struct timeval TIMEVAL;

#define closesocket close

#ifndef INVALID_SOCKET
# define INVALID_SOCKET -1
#endif

#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long)-1)
#endif

#ifndef HAVE_IN_ADDR_T
typedef unsigned long in_addr_t;
#endif

/* Used by vice_network_get_errorcode() (socket.c); upstream defines it in
   arch/shared/archdep_unix.h, which our REFERENCE-ONLY copy lacks. */
#include <errno.h>
#ifndef ARCHDEP_SOCKET_ERROR
#define ARCHDEP_SOCKET_ERROR errno
#endif

#endif /* _WIN32 */

#endif /* #ifdef HAVE_NETWORK */

#endif /* #ifndef VICE_SOCKETIMPL_H */
