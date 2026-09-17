/*
 * joyport_diag_586220_harness.c - Joyport part of the DIAG 586220 harness
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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
#include <stdlib.h>
#include <string.h>

#include "c64_diag_586220_harness.h"
#include "joyport_diag_586220_harness.h"
#include "joyport.h"
#include "joystick.h"
#include "machine.h"
#include "resources.h"
#include "snapshot.h"
#include "log.h"

/*#define DEBUG_DIAG_586220*/

#ifdef DEBUG_DIAG_586220
#define DBG(x)  log_printf  x
#else
#define DBG(x)
#endif

#ifdef JOYPORT_EXPERIMENTAL_DEVICES

static uint8_t harness_read_dig(int joyport)
{
    uint8_t retval = c64_diag_586220_read_joyport_dig(joyport);
    /*DBG(("harness_read_dig port:%d retval:%02x", joyport, retval));*/
    return retval;
}

static void harness_store_dig(int joyport, uint8_t val)
{
    /*DBG(("harness_store_dig port:%d val:%02x", joyport, val));*/
    c64_diag_586220_store_joyport_dig(joyport, val);
}

static uint8_t harness_read_potx(int joyport)
{
    return c64_diag_586220_read_joyport_pot();
}

static uint8_t harness_read_poty(int joyport)
{
    return c64_diag_586220_read_joyport_pot();
}


static int joyport_harness_enabled[JOYPORT_MAX_PORTS] = {0};

static int joyport_harness_set_enabled(int port, int enabled)
{
    int new_state = enabled ? 1 : 0;

    joyport_harness_enabled[port] = new_state;

    return 0;
}

static joyport_t joyport_harness_device = {
    "586220 DIAG Harness",       /* name of the device */
    JOYPORT_RES_ID_NONE,         /* device can be used in multiple ports at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,     /* device is NOT a lightpen */
    JOYPORT_POT_REQUIRED,        /* device uses the potentiometer lines */
    JOYPORT_5VDC_NOT_NEEDED,     /* device does NOT need +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,    /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_DIAG_HARNESS, /* device is a diag harness */
    0,                           /* output bits are programmable */
    joyport_harness_set_enabled, /* device enable/disable function */
    harness_read_dig,            /* digital line read function */
    harness_store_dig,           /* digital line store function */
    harness_read_potx,           /* pot-x read function */
    harness_read_poty,           /* pot-y read function */
    NULL,                        /* NO powerup function */
    NULL,                        /* NO device write snapshot function */
    NULL,                        /* NO device read snapshot function */
    NULL,                        /* NO device hook function */
    0                            /* NO device hook function mask */
};

int joyport_diag_586220_harness_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_DIAG_586220_HARNESS, &joyport_harness_device);
}

#endif
