/*
 * wordcraft-dongle.c - tape port dongle that helps WordCraft work.
 *
 * Written by
 *  Olaf 'Rhialto' Seibert <rhialto@falu.nl>
 * based on public information from http://bitbarn.co.uk/trusley/early_days.htm
 * http://bitbarn.co.uk/dryfire/dcpe.php#pets
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

#include "cmdline.h"
#include "log.h"
#include "maincpu.h"
#include "machine.h"
#include "mem.h"
#include "resources.h"
#include "tapeport.h"

#include "wordcraft-dongle.h"


static int wordcraft_dongle_enabled[TAPEPORT_MAX_PORTS] = { 0 };

static log_t wclog = LOG_DEFAULT;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void wordcraft_dongle_powerup(int port);
static int wordcraft_dongle_enable(int port, int val);
static void wordcraft_dongle_set_motor_line(int port, int value);
static void wordcraft_dongle_toggle_write_line(int port, int value);
static void wordcraft_dongle_set_sense_line(int port, int value);
static void wordcraft_dongle_set_read_line(int port, int value);

static tapeport_device_t wordcraft_dongle_device = {
    "Wordcraft dongle",                 /* device name */
    TAPEPORT_DEVICE_TYPE_DONGLE,        /* device is a 'dongle' type device */
    VICE_MACHINE_PET,                   /* device works on PETs only */
    TAPEPORT_PORT_ALL_MASK,             /* device works on all ports */
    wordcraft_dongle_enable,            /* device enable function */
    wordcraft_dongle_powerup,           /* device specific hard reset function */
    NULL,                               /* NO device shutdown function */
    wordcraft_dongle_set_motor_line,    /* set motor line function */
    wordcraft_dongle_toggle_write_line, /* set write line function */
    wordcraft_dongle_set_sense_line,    /* set sense line function */
    wordcraft_dongle_set_read_line,     /* set read line function */
    NULL,                               /* NO device snapshot write function */
    NULL                                /* NO device snapshot read function */
};

/* ------------------------------------------------------------------------- */

struct dongle_data {
    bool load_line;
    bool clock_in_line;
    bool data_in_line;
    bool clock_enable_line;
    bool value_detected;
    int shift_count;
    unsigned int dongle_value;
} dongle_data[TAPEPORT_MAX_PORTS];

static unsigned int reverse16(unsigned int bits)
{
    unsigned int v = bits;

    /* swap odd and even bits */
    v = ((v >> 1) & 0x5555) | ((v & 0x5555) << 1);
    /* swap consecutive pairs */
    v = ((v >> 2) & 0x3333) | ((v & 0x3333) << 2);
    /* swap nibbles ... */
    v = ((v >> 4) & 0x0F0F) | ((v & 0x0F0F) << 4);
    /* swap bytes */
    v = ((v >> 8) & 0x00FF) | ((v & 0x00FF) << 8);

    return v;
}

static int wordcraft_dongle_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (wordcraft_dongle_enabled[port] == val) {
        return 0;
    }

    wordcraft_dongle_enabled[port] = val;
    if (!val) {
        dongle_data[port].value_detected = false;
    }

    dongle_data[port].dongle_value = reverse16(0xD210); /* $084B or 2123. Most common value */
    /* dongle_data[port].dongle_value = reverse16(0x4800); / * 7 bits used */
    /* dongle_data[port].dongle_value = reverse16(0x2C00); / * 7 bits used */
    /* dongle_data[port].dongle_value = reverse16(0x0200); / * 7 bits used */
    /*
     * Working values:
     * Wordcraft
     *           E10         reverse(0x0200) (7 bits used)
     *                 /F10  reverse(0x9000)  some copies crash later
     *           80me  /F32  reverse(0xD210)
     *           80e   /G21  reverse(0x4800) s/n D25210  (7 bits used)
     *           80me  /G25  reverse(0x2C00)  (7 bits used)
     *           80    /H12  reverse(0xD210) (15 bits used)  doesn't set sense line?
     *           96xxxx/H12  reverse(0xD210) (15 bits used)
     *           80    /H17  reverse(0xD210)
     *           96xxxx/H17  reverse(0xD210) (15 bits used)
     * CBM 80F3 80ME distributor code D /F32: reverse(D210) (7 bits used)
     *
     * M=matrix, otherwise letter quality printers?
     * E=english?
     * FR=french  AZ=azerty
     * GB=english QW=qwerty
     * NL=dutch
     * 80 = 80 cols, 32 KB
     * 96 = 80 cols, 96 KB
     *
     * Loading documents: g,filename,drivenr,chapternr
     * for NL version:    l,filename,drivenr,chapternr
     *                    u,$  directory listing
     */
    return 0;
}

int wordcraft_dongle_resources_init(int amount)
{
    wclog = log_open("WordCraft");

    return tapeport_device_register(TAPEPORT_DEVICE_WORDCRAFT_DONGLE,
                                    &wordcraft_dongle_device);
}

/* ---------------------------------------------------------------------*/

static void wordcraft_dongle_detect_values(int port)
{
    /* Try to detect the expected dongle value(s) by inspecting the code */
    if (!dongle_data[port].value_detected) {
        /* Look at PC */
        uint16_t pc = reg_pc; /* unfortunately maincpu_get_pc(); returns 0 */
        unsigned int i;
        uint8_t lo = 0, hi = 0;
        bool lo_found = false;
        uint8_t *mem = mem_ram;
        log_message(wclog, "Detecting WordCraft dongle code from PC = $%04x", pc);

        if (pc >= 0x8000) {
            log_error(wclog, "PC too high for WordCraft dongle code. Ignoring.");
            return;
        }

        /* Search for the sequence LDY $7D STY $xxxx TYA  EOR #hi */
        for (i = 0; i < 128; i++) {
            if (mem[pc + i + 0x00] == 0xA4 &&           /* LDY $7D */
                mem[pc + i + 0x01] == 0x7D &&
                mem[pc + i + 0x02] == 0x8C &&           /* STY $xxxx */
                mem[pc + i + 0x05] == 0x98 &&           /* TYA */
                mem[pc + i + 0x06] == 0x49) {           /* EOR #hi */
                hi = mem[pc + i + 0x07];

                if (mem[pc + i + 0x08] == 0xD0 &&       /* BNE */
                    mem[pc + i + 0x0C] == 0x49) {       /* EOR #lo */
                    lo = mem[pc + i + 0x0D];
                    lo_found = true;
                }

                if (lo_found) {
                    log_message(wclog, "Found $%02x $%02x at $%04x +$%02x and +$%02x", hi, lo, pc, i+0x07, i+0x0D);
                } else {
                    log_message(wclog, "Found $%02x at $%04x +$%02x", hi, pc, i+0x07);
                }

                break;
            }
        }

        if (i >= 128) {
            log_error(wclog, "Did NOT find expected dongle code/value near %04x", pc);

            /* Try for version /D3 */
            i = 0x7E;   /* 0x2735 - 0x26b7 */

            if (mem[pc + i + 0] == 0x98 &&      /* TYA */
                mem[pc + i + 1] == 0x49) {      /* EOR $hi */
                hi = mem[pc + i + 2];
                log_message(wclog, "Found %02x 2nd try at %04x", hi, pc + i + 2);
            }
        }

        if (lo_found ? hi|lo : hi) {
            dongle_data[port].value_detected = true;
            dongle_data[port].dongle_value = reverse16((hi << 8) | lo);
        } else {
            /* The "WMAST?0ME/F32" program accepts any value except 0 */
            log_message(wclog, "Value zero found; not using that.");
        }
    }
}

/* ---------------------------------------------------------------------*/

static void wordcraft_dongle_powerup(int port)
{
    dongle_data[port].shift_count = 0;
}

static void wordcraft_dongle_set_motor_line(int port, int value)
{
    if (value != dongle_data[port].clock_enable_line) {
        log_verbose(wclog, "wordcraft_dongle_set_motor_line: %d %d", port, value);
    }
    dongle_data[port].clock_enable_line = value;        /* but apparently not used */
}

/*
 * The tape input on the PET is strictly edge-triggered. To compensate for that,
 * after every "active transition", the dongle checking code toggles the active
 * direction. This way it can detect both 1 and 0 inputs.
 * Because of this, we do not need to worry if we send duplicate "transitions"
 * in the same direction into the PIA code. Those are implemented inaccurately.
 */
static void wordcraft_dongle_toggle_write_line(int port, int value)
{
    log_debug(wclog, "wordcraft_dongle_toggle_write_line: %d %d", port, value);

    if (!dongle_data[port].clock_in_line && value) {
        /*
         * Shift a bit on the RISING edge of the clock. The dongle checking
         * code does both transitions right after each other, before reading
         * input.  So it doesn't really matter which edge is used.  LSB first.
         */
        bool bit = (dongle_data[port].dongle_value >> dongle_data[port].shift_count) & 1;
        log_verbose(wclog, "wordcraft_dongle_toggle_write_line: send bit #%d %d",
                dongle_data[port].shift_count, bit);
        dongle_data[port].shift_count++;

        tapeport_set_read_in(bit, port);
        dongle_data[port].data_in_line = bit;
    }
    dongle_data[port].clock_in_line = value;
}

static void wordcraft_dongle_set_sense_line(int port, int value)
{
    if (!dongle_data[port].load_line && value) {
        log_verbose(wclog, "wordcraft_dongle_sense_line: %d %d rise: restart shift sequence", port, value);
        /* Restart the shifting sequence */
        dongle_data[port].shift_count = 0;

        wordcraft_dongle_detect_values(port);
    }
    dongle_data[port].load_line = value;
}

static void wordcraft_dongle_set_read_line(int port, int value)
{
    log_debug(wclog, "wordcraft_dongle_set_read_line: %d %d", port, value);
}
