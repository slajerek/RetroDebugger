/*
 * c64-snapshot.h - C64 snapshot handling.
 *
 * Written by
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

#ifndef VICE_C64_SNAPSHOT_H
#define VICE_C64_SNAPSHOT_H

extern int c64_snapshot_write(const char *name, int save_roms, int save_disks, int event_mode, int save_reu_data, int save_cart_roms, int save_screen);
extern int c64_snapshot_read(const char *name, int event_mode, int read_roms, int read_disks, int read_reu_data, int read_cart_roms);

extern int c64_snapshot_read_from_memory(int save_chips, int read_roms, int read_disks, int event_mode, int read_reu_data, int read_cart_roms,
										 unsigned char *snapshot_data, int snapshot_size);
extern int c64_snapshot_write_in_memory(int save_chips, int save_roms, int save_disks, int event_mode,
										int save_reu_data, int save_cart_roms, int save_screen,
										int *snapshot_size, unsigned char **snapshot_data);

#endif
