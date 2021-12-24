/*! \file unix/socketimpl.h \n
 *  \author Spiro Trikaliotis\n
 *  \brief  Abstraction from network sockets.
 *
 * socketimpl.h - Abstraction from network sockets. Unix implementation.
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

#ifdef HAVE_NETWORK
 
#ifdef MINIX_SUPPORT
# include <limits.h>
# define PF_INET AF_INET

# ifndef MINIX_HAS_RECV_SEND
extern ssize_t recv(int socket, void *buffer, size_t length, int flags);
extern ssize_t send(int socket, const void *buffer, size_t length, int flags);
# endif

#endif /* #ifdef MINIX_SUPPORT */

#if !defined(HAVE_GETDTABLESIZE) && defined(HAVE_GETRLIMIT)
#include <sys/resource.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#ifndef VMS
#include <sys/select.h>
#endif
 
#include <unistd.h>

#ifdef __minix
# define recv(socket, buffer, length, flags) recvfrom(socket, buffer, length, flags, NULL, NULL)
extern ssize_t send(int socket, const void *buffer, size_t length, int flags);
#endif

typedef unsigned int SOCKET;
typedef struct timeval TIMEVAL;

#define closesocket close

#ifndef INVALID_SOCKET
# define INVALID_SOCKET (SOCKET)(~0)
#endif

#define SOCKET_IS_INVALID(_x) ((_x) < 0)

#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long)-1)
#endif

#ifndef HAVE_IN_ADDR_T
typedef unsigned long in_addr_t;
#endif

#endif /* #ifdef HAVE_NETWORK */

#endif /* #ifndef VICE_SOCKETIMPL_H */
