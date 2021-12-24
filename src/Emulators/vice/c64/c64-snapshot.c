/*
 * c64-snapshot.c - C64 snapshot handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "vice.h"

#include <stdio.h>

#include "c64-memory-hacks.h"
#include "c64-snapshot.h"
#include "c64.h"
#include "c64gluelogic.h"
#include "c64memsnapshot.h"
#include "cia.h"
#include "drive-snapshot.h"
#include "drive.h"
#include "ioutil.h"
#include "joyport.h"
#include "joystick.h"
#include "keyboard.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "sid-snapshot.h"
#include "snapshot.h"
#include "sound.h"
#include "tapeport.h"
#include "types.h"
#include "userport.h"
#include "vice-event.h"
#include "vicii.h"

int c64d_snapshot_write_module(snapshot_t *s, int save_screen);
int c64d_snapshot_read_module(snapshot_t *s);

#define SNAP_MAJOR 1
#define SNAP_MINOR 1

int c64_snapshot_write(const char *name, int save_roms, int save_disks, int event_mode, int save_reu_data, int save_cart_roms, int save_screen)
{
    snapshot_t *s;

	LOGD("c64_snapshot_write: name=%s save_roms=%d save_disks=%d event_mode=%d save_reu_data=%d save_cart_roms=%d save_screen=%d", name, save_roms, save_disks, event_mode, save_reu_data, save_cart_roms, save_screen);

    s = snapshot_create(name, ((BYTE)(SNAP_MAJOR)), ((BYTE)(SNAP_MINOR)), machine_get_name(), 0);
    if (s == NULL) {
        return -1;
    }

    sound_snapshot_prepare();

    /* Execute drive CPUs to get in sync with the main CPU.  */
    drive_cpu_execute_all(maincpu_clk);

    if (maincpu_snapshot_write_module(s) < 0
        || c64_snapshot_write_module(s, save_roms, save_reu_data, save_cart_roms) < 0
        || ciacore_snapshot_write_module(machine_context.cia1, s) < 0
        || ciacore_snapshot_write_module(machine_context.cia2, s) < 0
        || sid_snapshot_write_module(s) < 0
        || drive_snapshot_write_module(s, save_disks, save_roms) < 0
        || vicii_snapshot_write_module(s) < 0
        || c64_glue_snapshot_write_module(s) < 0
        || event_snapshot_write_module(s, event_mode) < 0
        || memhacks_snapshot_write_modules(s) < 0
        || tapeport_snapshot_write_module(s, save_disks) < 0
        || keyboard_snapshot_write_module(s) < 0
        || joyport_snapshot_write_module(s, JOYPORT_1) < 0
        || joyport_snapshot_write_module(s, JOYPORT_2) < 0
        || userport_snapshot_write_module(s) < 0
		|| c64d_snapshot_write_module(s, save_screen) < 0)
	{
        snapshot_close(s);
        ioutil_remove(name);
        return -1;
    }

    snapshot_close(s);
    return 0;
}

int c64_snapshot_write_in_memory(int save_chips, int save_roms, int save_disks, int event_mode, int save_reu_data, int save_cart_roms, int save_screen,
								 int *snapshot_size, unsigned char **snapshot_data)
{
	snapshot_t *s;
	
	s = snapshot_create_in_memory( ((BYTE)(SNAP_MAJOR)), ((BYTE)(SNAP_MINOR)), machine_get_name(), 0);
	if (s == NULL) {
		return -1;
	}
	
	sound_snapshot_prepare();
	
	/* Execute drive CPUs to get in sync with the main CPU.  */
	drive_cpu_execute_all(maincpu_clk);
	
	if (save_chips == 0)
	{
		if (save_roms == 1 || save_reu_data == 1)
		{
			if (c64_snapshot_write_module(s, save_roms, save_reu_data, save_cart_roms) < 0)
			{
				lib_free(s->data);
				//snapshot_close(s);
				lib_free(s);
				*snapshot_data = NULL;
				*snapshot_size = -1;
				return -1;
			}
		}
		if (save_disks == 1)
		{
			if (drive_snapshot_write_module(s, save_disks, save_roms) < 0)
			{
				lib_free(s->data);
				//snapshot_close(s);
				lib_free(s);
				*snapshot_data = NULL;
				*snapshot_size = -1;
				return -1;
			}
		}
	}
	else
	{
		if (maincpu_snapshot_write_module(s) < 0
			|| c64_snapshot_write_module(s, save_roms, save_reu_data, save_cart_roms) < 0
			|| ciacore_snapshot_write_module(machine_context.cia1, s) < 0
			|| ciacore_snapshot_write_module(machine_context.cia2, s) < 0
			|| sid_snapshot_write_module(s) < 0
			|| drive_snapshot_write_module(s, save_disks, save_roms) < 0
			|| vicii_snapshot_write_module(s) < 0
			|| c64_glue_snapshot_write_module(s) < 0
			|| event_snapshot_write_module(s, event_mode) < 0
			|| memhacks_snapshot_write_modules(s) < 0
			|| tapeport_snapshot_write_module(s, save_disks) < 0
			|| keyboard_snapshot_write_module(s) < 0
			|| joyport_snapshot_write_module(s, JOYPORT_1) < 0
			|| joyport_snapshot_write_module(s, JOYPORT_2) < 0
			|| userport_snapshot_write_module(s) < 0
			|| c64d_snapshot_write_module(s, save_screen) < 0)
		{
			lib_free(s->data);
			//snapshot_close(s);
			lib_free(s);
			*snapshot_data = NULL;
			*snapshot_size = -1;
			return -1;
		}
	}

	
	*snapshot_size = s->pos;
	*snapshot_data = s->data;
	s->data = NULL;
	
//	snapshot_close(s);
	lib_free(s);

	return 0;
}


int c64_snapshot_read(const char *name, int event_mode, int read_roms, int read_disks, int read_reu_data, int read_cart_roms)
{
    snapshot_t *s;
    BYTE minor, major;

    s = snapshot_open(name, &major, &minor, machine_get_name());
    if (s == NULL) {
        return -1;
    }

    if (major != SNAP_MAJOR || minor != SNAP_MINOR) {
        log_error(LOG_DEFAULT, "Snapshot version (%d.%d) not valid: expecting %d.%d.", major, minor, SNAP_MAJOR, SNAP_MINOR);
        snapshot_set_error(SNAPSHOT_MODULE_INCOMPATIBLE);
        goto fail;
    }

    vicii_snapshot_prepare();

    joyport_clear_devices();

    if (maincpu_snapshot_read_module(s) < 0
        || c64_snapshot_read_module(s, read_reu_data, read_cart_roms) < 0
        || ciacore_snapshot_read_module(machine_context.cia1, s) < 0
        || ciacore_snapshot_read_module(machine_context.cia2, s) < 0
        || sid_snapshot_read_module(s) < 0
        || drive_snapshot_read_module(s, read_roms, read_disks) < 0
        || vicii_snapshot_read_module(s) < 0
        || c64_glue_snapshot_read_module(s) < 0
        || event_snapshot_read_module(s, event_mode) < 0
        || memhacks_snapshot_read_modules(s) < 0
        || tapeport_snapshot_read_module(s) < 0
        || keyboard_snapshot_read_module(s) < 0
        || joyport_snapshot_read_module(s, JOYPORT_1) < 0
        || joyport_snapshot_read_module(s, JOYPORT_2) < 0
        || userport_snapshot_read_module(s) < 0
		|| c64d_snapshot_read_module(s) < 0) {
        goto fail;
    }

    snapshot_close(s);

    sound_snapshot_finish();

    return 0;

fail:
    if (s != NULL) {
        snapshot_close(s);
    }

    machine_trigger_reset(MACHINE_RESET_MODE_SOFT);

    return -1;
}

int c64_snapshot_read_from_memory(int read_chips, int read_roms, int read_disks, int event_mode, int read_reu_data, int read_cart_roms,
								  unsigned char *snapshot_data, int snapshot_size)
{
	snapshot_t *s;
	BYTE minor, major;
	
	s = snapshot_open_from_memory(&major, &minor, machine_get_name(), snapshot_data, snapshot_size);
	if (s == NULL) {
		return -1;
	}
	
	if (major != SNAP_MAJOR || minor != SNAP_MINOR) {
		log_error(LOG_DEFAULT, "Snapshot version (%d.%d) not valid: expecting %d.%d.", major, minor, SNAP_MAJOR, SNAP_MINOR);
		snapshot_set_error(SNAPSHOT_MODULE_INCOMPATIBLE);
		goto fail;
	}
	
	vicii_snapshot_prepare();
	
	joyport_clear_devices();
	
	if (maincpu_snapshot_read_module(s) < 0
		|| c64_snapshot_read_module(s, read_reu_data, read_cart_roms) < 0
		|| ciacore_snapshot_read_module(machine_context.cia1, s) < 0
		|| ciacore_snapshot_read_module(machine_context.cia2, s) < 0
		|| sid_snapshot_read_module(s) < 0
		|| drive_snapshot_read_module(s, read_roms, read_disks) < 0
		|| vicii_snapshot_read_module(s) < 0
		|| c64_glue_snapshot_read_module(s) < 0
		|| event_snapshot_read_module(s, event_mode) < 0
		|| memhacks_snapshot_read_modules(s) < 0
		|| tapeport_snapshot_read_module(s) < 0
		|| keyboard_snapshot_read_module(s) < 0
		|| joyport_snapshot_read_module(s, JOYPORT_1) < 0
		|| joyport_snapshot_read_module(s, JOYPORT_2) < 0
		|| userport_snapshot_read_module(s) < 0
		|| c64d_snapshot_read_module(s) < 0) {
		goto fail;
	}
	
	snapshot_close(s);
	
	sound_snapshot_finish();
	
	return 0;
	
fail:
	if (s != NULL) {
		snapshot_close(s);
	}
	
	machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
	
	return -1;
}
