/*
 * sounddump.c - Implementation of the dump sound device.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

#include "sound.h"
#include "types.h"
#include "log.h"

/* #define DEBUG_DUMP */

#ifdef DEBUG_DUMP
#define DBG(x)  log_printf x
#else
#define DBG(x)
#endif

static FILE *dump_fd = NULL;

static int dump_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    /* No stereo capability. */
    *channels = 1;

    dump_fd = fopen(param ? param : "vicesnd.sid", "w");
    DBG(("dump_init param:%p fd:%p", param, dump_fd));
    return (dump_fd == NULL) ? -1 : 0;
}

static int dump_write(int16_t *pbuf, size_t nr)
{
    /*DBG(("dump_write"));*/
    return 0;
}

static int dump_dump(uint16_t addr, uint8_t byte, CLOCK clks)
{
    DBG(("dump_dump %d %d %d", (int)clks, addr, byte));
    return (fprintf(dump_fd, "%d %d %d\n", (int)clks, addr, byte) < 0);
}

#if 0
/* FIXME: it is unclear why this function does this. no other sound output
          driver implements "flush" for that matter
*/
static int dump_flush(char *state)
{
    DBG(("dump_flush state:'%s'"));
    if (fprintf(dump_fd, "%s", state) < 0) {
        return 1;
    }

    return fflush(dump_fd);
}
#endif

static void dump_close(void)
{
    DBG(("dump_close"));
    fclose(dump_fd);
    dump_fd = NULL;
}

static const sound_device_t dump_device =
{
    "dump",
    dump_init,
    dump_write,
    dump_dump,
    NULL, /* dump_flush, */
    NULL,
    dump_close,
    NULL,
    NULL,
    0,
    1,
    false
};

int sound_init_dump_device(void)
{
    DBG(("sound_init_dump_device"));
    return sound_register_device(&dump_device);
}
