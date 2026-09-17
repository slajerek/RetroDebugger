/*
 * c64_diag_586220_harness.c - c64 diagnosis cartridge harness hub emulation.
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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "c64_diag_586220_harness.h"
#include "c64.h"
#include "cia.h"
#include "datasette.h"
#include "machine.h"
#include "tapeport.h"
#include "types.h"
#include "log.h"

/*#define DEBUG_DIAG_586220*/

#ifdef DEBUG_DIAG_586220
#define DBG(x)  log_printf  x
#else
#define DBG(x)
#endif

static uint8_t c64_diag_userport_pax = 0;
static uint8_t c64_diag_userport_pbx = 0;
static uint8_t c64_diag_userport_sp1 = 0;
static uint8_t c64_diag_userport_sp2 = 0;

static uint8_t c64_diag_tapeport = 0;
static uint8_t c64_diag_switches = 0;

static uint8_t c64_diag_joyport0 = 0;
static uint8_t c64_diag_joyport1 = 0;
static uint8_t c64_diag_joyvalue = 0;

static uint8_t c64_diag_keyboard_pax = 0;
static uint8_t c64_diag_keyboard_pbx = 0;

static uint8_t c64_diag_serial = 0;

void c64_diag_586220_init(void)
{
    c64_diag_userport_pax = 0;
    c64_diag_userport_pbx = 0;
    c64_diag_userport_sp1 = 0;
    c64_diag_userport_sp2 = 0;
    c64_diag_tapeport = 0;
    c64_diag_joyport0 = 0;
    c64_diag_joyport1 = 0;
    c64_diag_keyboard_pax = 0;
    c64_diag_keyboard_pbx = 0;
    c64_diag_serial = 0;
    c64_diag_switches = 0;
}


/* USERPORT connector

PIN | PIN | NOTES
-----------------
 4  |  6  | CNT1  <-> CNT2
 5  |  7  | SP1   <-> SP2
 9  |  M  | PA3   <-> PA2
 B  |  8  | FLAG2 <-  PC2

 C  |  H  | PB0   <-> PB4
 D  |  J  | PB1   <-> PB5
 E  |  K  | PB2   <-> PB6
 F  |  L  | PB3   <-> PB7

no data lines go to other ports

*/

/* called by userport_diag_586220_harness_store_paX() */
void c64_diag_586220_store_userport_pax(uint8_t val)
{
    c64_diag_userport_pax = val;
    DBG(("c64_diag_586220_store_userport_pax %02x", val));
}
/* called by userport_diag_586220_harness_store_pbx() */
void c64_diag_586220_store_userport_pbx(uint8_t val)
{
    c64_diag_userport_pbx = val;
}
/* called by userport_diag_586220_harness_store_spX */
void c64_diag_586220_store_userport_sp(uint8_t port, uint8_t val)
{
    DBG(("c64_diag_586220_store_userport_sp port:%d val:%02x", port, val));
    if (!port) {
        ciacore_set_sdr(machine_context.cia2, val);
        c64_diag_userport_sp2 = val;
    } else {
        ciacore_set_sdr(machine_context.cia1, val);
        c64_diag_userport_sp1 = val;
    }
}

/* called from userport_diag_586220_harness_read_paX */
uint8_t c64_diag_586220_read_userport_pax(void)
{
    uint8_t retval;

    retval = (c64_diag_userport_pax & 4) << 1;      /* bit2 -> bit3 */
    retval |= (c64_diag_userport_pax & 8) >> 1;     /* bit3 -> bit2 */

    retval ^= 0x0c;

    DBG(("c64_diag_586220_read_userport_pax %02x", retval));
    return retval;
}
/* called from userport_diag_586220_harness_read_pbx */
uint8_t c64_diag_586220_read_userport_pbx(void)
{
    uint8_t retval;

    retval = (c64_diag_userport_pbx >> 4) & 0x0f;   /* bit4-7 -> bit0-3 */
    retval |= (c64_diag_userport_pbx & 0x0f) << 4;  /* bit0-3 -> bit4-7 */

    return retval;
}
/* called from userport_diag_586220_harness_read_spX */
uint8_t c64_diag_586220_read_userport_sp(uint8_t port)
{
    DBG(("c64_diag_586220_read_userport_sp  port:%d val:%02x",
           port, port ? c64_diag_userport_sp2 : c64_diag_userport_sp1));
    if (!port) {
        return c64_diag_userport_sp1;
    }
    return c64_diag_userport_sp2;
}


/* TAPE connector

PIN | CABLE | NOTES
-------------------
C-3 |   7   | Motor | Joyport Switches control (MOTOR) can ground the line
D-4 |   6   | Read  | loops to 4 (READ <-> SENSE)
E-5 |   5   | Write | Joyport Switches control (WRITE) can ground the line
F-6 |   4   | Sense | loops to 6 (SENSE <-> READ)

*/

/* called from tape_diag_586220_harness... */
void c64_diag_586220_store_tapeport(uint8_t pin, uint8_t val)
{
    c64_diag_tapeport &= ~(1 << pin);
    c64_diag_tapeport |= (val << pin);

    switch (pin) {
        /* motor <-> write */
        case C64_DIAG_TAPEPORT_MOTOR:
            DBG(("c64_diag_586220_store_tapeport motor:%d",val));
            machine_set_tape_write_in(TAPEPORT_PORT_1, val);
            break;
        case C64_DIAG_TAPEPORT_WRITE:
            DBG(("c64_diag_586220_store_tapeport write:%d",val));
            machine_set_tape_motor_in(TAPEPORT_PORT_1, val);
            break;
        /* read <-> sense */
        case C64_DIAG_TAPEPORT_READ:
            DBG(("c64_diag_586220_store_tapeport read:%d\n",val));
            machine_set_tape_sense(TAPEPORT_PORT_1, val);
            break;
        case C64_DIAG_TAPEPORT_SENSE:
            DBG(("c64_diag_586220_store_tapeport sense:%d\n",val));
            machine_set_tape_read_in(TAPEPORT_PORT_1, val);
            break;
    }

    if (c64_diag_tapeport & 5) {
        c64_diag_switches = 1;
    } else {
        c64_diag_switches = 0;
    }
}

#if 0
/* TODO: unused */
uint8_t c64_diag_586220_read_tapeport(uint8_t pin)
{
    uint8_t retval;

    retval = c64_diag_tapeport & 0xf5;
    retval |= (c64_diag_tapeport & 8) >> 2;
    retval |= (c64_diag_tapeport & 2) << 2;
    retval &= (1 << pin);

    return retval;
}
#endif

/* JOYSTICK connector

when enabled, via the analog switch connected to the tape port, bits0-4 of the
two joystick ports are connected 1:1

*/

/* called by harness_store_dig */
void c64_diag_586220_store_joyport_dig(uint8_t port, uint8_t val)
{
    /*DBG(("c64_diag_586220_store_joyport_dig port:%d val:%02x", port, val));*/
    if (!port) {
        c64_diag_joyport0 = val;
    } else {
        c64_diag_joyport1 = val;
    }
    c64_diag_joyvalue = val;
}

/* called by harness_read_dig */
uint8_t c64_diag_586220_read_joyport_dig(uint8_t port)
{
    /*DBG(("c64_diag_586220_read_joyport_dig port:%d",port));*/
    if (c64_diag_switches) {
#if 0
        if (!port) {
            return c64_diag_joyport1;
        }
        return c64_diag_joyport0;
#endif
        return c64_diag_joyvalue;
    }
    return 0xff;
}

/* called by harness_read_potX */
uint8_t c64_diag_586220_read_joyport_pot(void)
{
    /* resistor value for all resistors is 120Ohm */
    /* diag expects 0x58...0x78 */
    return 0x60;
}


/* KEYBOARD connector */

/* TODO: unused */
void c64_diag_586220_store_keyboard(uint8_t port, uint8_t val)
{
    if (!port) {
        c64_diag_keyboard_pax = val;
    } else {
        c64_diag_keyboard_pbx = val;
    }
}

/* TODO: unused */
uint8_t c64_diag_586220_read_keyboard(uint8_t port)
{
    if (!port) {
        return c64_diag_keyboard_pbx;
    }
    return c64_diag_keyboard_pax;
}


/* IEC connector

PIN | CABLE           | NOTES
--------------------------------------------------------------------------------
1-5 | SRQIN - DATA    |   SRQIN->CIA1 FLAG   DATA->CIA2 PA5/PA7
3-4 | ATN   - CLOCK   |

*/

/* TODO: unused */
void c64_diag_586220_store_serial(uint8_t val)
{
    /*  Bit 7  Serial Bus Data Input
        Bit 6  Serial Bus Clock Pulse Input
        Bit 5  Serial Bus Data Output
        Bit 4  Serial Bus Clock Pulse Output
        Bit 3  Serial Bus ATN Signal Output */
    DBG(("c64_diag_586220_store_serial %02x", val));
    c64_diag_serial = val;
}

/* TODO: unused */
uint8_t c64_diag_586220_read_serial(void)
{
    uint8_t retval;

    retval = ((c64_diag_serial >> 3) & 1) << 6;        /* ATN Output -> Clock Input */
    retval |= ((c64_diag_serial >> 5) & 1) << 7;       /* Data Output -> SRQIN */
    /*  Bit 7  Serial Bus Data Input
        Bit 6  Serial Bus Clock Pulse Input
        Bit 5  Serial Bus Data Output
        Bit 4  Serial Bus Clock Pulse Output
        Bit 3  Serial Bus ATN Signal Output */
    DBG(("c64_diag_586220_read_serial %02x (%02x)", c64_diag_serial, retval));
    return retval;
}

