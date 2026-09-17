/* [C64D-REFERENCE-ONLY]
 * Vendored from VICE as REFERENCE, not as live code. Kept for features the
 * original emulator has and RetroDebugger does not yet (printer / RS232, drive
 * and cartridge menus, virtual keyboard). Its SDL calls are already commented
 * out; RetroDebugger draws its own ImGui UI.
 * DO NOT delete, and do not "port" it -- see src/Emulators/REFERENCE_FILES.md.
 */

/*
 * coproc.h - coproc.h wrapper for the sdl port.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_SDL_COPROC_H
#define VICE_SDL_COPROC_H

/* VICE 3.10: vice_pid_t for coproc support */
#if defined(WINDOWS_COMPILE) || defined(_WIN32)
#include <windows.h>
typedef HANDLE vice_pid_t;
#else
#include <sys/types.h>
typedef pid_t vice_pid_t;
#endif

int fork_coproc(int *fd_wr, int *fd_rd, char *cmd, vice_pid_t *childpid);
void kill_coproc(vice_pid_t pid);

#endif
