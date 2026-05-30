/*
 * powerpad.c
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

#include "joyport.h"
#include "joystick.h"
#include "resources.h"
#include "snapshot.h"
#include "mouse.h"
#include "powerpad.h"

#include "log.h"

/*  PowerPad by Chalkboard Inc.

    User Guide: https://archive.org/details/ChalkBoardPowerPadUsersGuide
    Programmers Manual: https://archive.org/details/power-pad-programming-kit-chalk-board-inc

    Nevermind the weirdness with handling more than one port - it doesn't work
    right now, but it might do so in the future :)

 */

/* #define DEBUG_POWERPAD */

#ifdef DEBUG_POWERPAD
#define DBG(_x_) log_printf  _x_
#else
#define DBG(_x_)
#endif

/* Control port <--> Chalkboard connections:

   cport |  Chalkboard  | I/O
   ------------------------
     1   |   DATA     |  I      ->PB0   JOYPORT_UP
     2   |   CLEAR    |  O      <-PB1   JOYPORT_DOWN
     3   |   CLOCK    |  O      <-PB2   JOYPORT_LEFT
     4   |   SENSE    |  I      ->PB3   JOYPORT_RIGHT

 */

#define POWERPAD_DATA     JOYPORT_UP
#define POWERPAD_CLEAR    JOYPORT_DOWN
#define POWERPAD_CLOCK    JOYPORT_LEFT
#define POWERPAD_SENSE    JOYPORT_RIGHT

#define POWERPAD_DATA_BIT     JOYPORT_UP_BIT
#define POWERPAD_CLEAR_BIT    JOYPORT_DOWN_BIT
#define POWERPAD_CLOCK_BIT    JOYPORT_LEFT_BIT
#define POWERPAD_SENSE_BIT    JOYPORT_RIGHT_BIT

static int powerpad_enabled[JOYPORT_MAX_PORTS] = {0};

static uint8_t counter[JOYPORT_MAX_PORTS] = {0};

static uint8_t clock_line[JOYPORT_MAX_PORTS] = {0};
static uint8_t clear_line[JOYPORT_MAX_PORTS] = {0};

#define MAX_SENDBITS    (16+2)
static uint8_t mouse_digital_val[JOYPORT_MAX_PORTS] = {0};

static uint8_t pos_x[JOYPORT_MAX_PORTS];
static uint8_t pos_y[JOYPORT_MAX_PORTS];

/* ------------------------------------------------------------------------- */

static joyport_t joyport_powerpad_device;

static int joyport_powerpad_set_enabled(int port, int enabled)
{
    int new_state = enabled ? 1 : 0;

    if (new_state == powerpad_enabled[port]) {
        return 0;
    }

    counter[port] = 0;

    /* set current state */
    powerpad_enabled[port] = new_state;

    if (new_state) {
        mouse_type = MOUSE_TYPE_POWERPAD;
    }

    return 0;
}

static void mouse_get_new_movement(void)
{
    int16_t new_x16, new_y16;

    mouse_get_raw_int16(&new_x16, &new_y16);
    pos_x[0] = (uint8_t)(new_x16 >> 1) ^ 0xff;
    pos_y[0] = (uint8_t)(new_y16 >> 1) ^ 0xff;

    /* DBG(("mouse_get_new_movement %dx%d %02xx%02x", new_x16, new_y16, pos_x[0], pos_y[0])); */
}

static uint8_t joyport_digital_value(int port)
{
    uint8_t retval = 0xff;
    if (_mouse_enabled) {
        /* we need to poll here, else the mouse can not move if the C64 code does not read POTX */
        mouse_poll();
        retval = (uint8_t)((~mouse_digital_val[port]));
        /* joyport_display_joyport(port, mouse_type_to_id(MOUSE_TYPE_POWERPAD), (uint16_t)(~retval)); */
    }
    return retval;
}


void powerpad_button_left(int pressed)
{
    DBG(("powerpad_button_left(%d)", pressed));
    if (pressed) {
        mouse_digital_val[0] |= POWERPAD_SENSE;
    } else {
        mouse_digital_val[0] &= (uint8_t)~POWERPAD_SENSE;
    }
}

static uint8_t powerpad_read(int port)
{
    uint8_t retval = 0;
    uint16_t joyval = joyport_digital_value(port);

    switch (counter[port]) {
        case 0:     /* always 0 */
            retval |= 0;
            break;
        case 1:     /* always 0 */
            mouse_get_new_movement();
            retval |= 0;
            break;

        case 2:
            retval |= (pos_y[port] >> 1) & 1;
            break;
        case 3:
            retval |= (pos_y[port] >> 2) & 1;
            break;
        case 4:
            retval |= (pos_y[port] >> 3) & 1;
            break;
        case 5:
            retval |= (pos_y[port] >> 4) & 1;
            break;
        case 6:
            retval |= (pos_y[port] >> 5) & 1;
            break;
        case 7:
            retval |= (pos_y[port] >> 6) & 1;
            break;
        case 8:
            retval |= (pos_y[port] >> 7) & 1;
            break;

        case 9:
            retval |= (pos_x[port] >> 1) & 1;
            break;
        case 10:
            retval |= (pos_x[port] >> 2) & 1;
            break;
        case 11:
            retval |= (pos_x[port] >> 3) & 1;
            break;
        case 12:
            retval |= (pos_x[port] >> 4) & 1;
            break;
        case 13:
            retval |= (pos_x[port] >> 5) & 1;
            break;
        case 14:
            retval |= (pos_x[port] >> 6) & 1;
            break;
        case 15:
            retval |= (pos_x[port] >> 7) & 1;
            break;

        default:
            break;
    }

    /* DBG(("powerpad_read(%d) count:%d joyval:%02x (%dx%d) retval: %02x", port, counter[port], joyval, pos_x[port], pos_y[port], retval)); */
    joyport_display_joyport(port, mouse_type_to_id(MOUSE_TYPE_POWERPAD), ~((uint16_t)(~retval) & joyval));

    return ~(retval) & joyval;
}

static void powerpad_store(int port, uint8_t val)
{
    uint8_t new_clock = JOYPORT_BIT_BOOL(val, POWERPAD_CLOCK_BIT);    /* bit 2, joy left */
    uint8_t new_clear = JOYPORT_BIT_BOOL(val, POWERPAD_CLEAR_BIT);    /* bit 1, joy down */

    if ((new_clear == 0) && (clock_line[port] == 0) && (new_clock == 1)) {
        counter[port]++;
        /* DBG(("powerpad_store(port %d) clock count:%d", port, counter[port])); */
    }

    if ((new_clock == 0) && (clear_line[port] == 0) && (new_clear == 1)) {
        counter[port] = 0;
        /* DBG(("powerpad_store(port %d) clear", port)); */
    }

    clear_line[port] = new_clear;
    clock_line[port] = new_clock;
    /* DBG(("powerpad_store(port %d) val:%02x clk:%d clear:%d", port, val, new_clock, new_clear)); */
}

static void powerpad_powerup(int port)
{
    counter[port] = 0;
}

/* ------------------------------------------------------------------------- */

static int powerpad_write_snapshot(struct snapshot_s *s, int p);
static int powerpad_read_snapshot(struct snapshot_s *s, int p);

static joyport_t joyport_powerpad_device = {
    "PowerPad",                        /* name of the device */
    JOYPORT_RES_ID_MOUSE,              /* device uses the mouse for input, only 1 mouse type device can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,           /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,              /* device does NOT use the potentiometer lines */
    JOYPORT_5VDC_REQUIRED,             /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,          /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_DRAWING_PAD,        /* device is a Drawing Pad adapter */
    0x06,                              /* bits 1,2 are output bits */
    joyport_powerpad_set_enabled,      /* device enable/disable function */
    powerpad_read,                     /* digital line read function */
    powerpad_store,                    /* digital line store function */
    NULL,                              /* NO pot-x read function */
    NULL,                              /* NO pot-y read function */
    powerpad_powerup,                  /* powerup function */
    powerpad_write_snapshot,           /* device write snapshot function */
    powerpad_read_snapshot,            /* device read snapshot function */
    NULL,                              /* NO device hook function */
    0                                  /* NO device hook function mask */
};

/* ------------------------------------------------------------------------- */

int powerpad_register(void)
{
    return joyport_device_register(JOYPORT_ID_POWERPAD, &joyport_powerpad_device);
}

/* ------------------------------------------------------------------------- */

/* POWERPAD snapshot module format:

   type  |   name  | description
   ----------------------------------
   BYTE  | COUNTER | counter value
   BYTE  | CLEAR   | clear line state
   BYTE  | CLOCK   | clock line state
 */

static const char snap_module_name[] = "POWERPAD";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int powerpad_write_snapshot(struct snapshot_s *s, int p)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, counter[p]) < 0
        || SMW_B(m, clear_line[p]) < 0
        || SMW_B(m, clock_line[p]) < 0) {
            snapshot_module_close(m);
            return -1;
    }
    return snapshot_module_close(m);
}

static int powerpad_read_snapshot(struct snapshot_s *s, int p)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || SMR_B(m, &counter[p]) < 0
        || SMR_B(m, &clear_line[p]) < 0
        || SMR_B(m, &clock_line[p]) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
