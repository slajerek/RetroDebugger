/*
 * ps2mouse.c -- PS/2 mouse on userport emulation
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 * Based on code by
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

#include "vice.h"

#include "cmdline.h"
#include "resources.h"
#include "log.h"
#include "joyport.h"
#include "userport.h"
#include "userport_ps2mouse.h"
#include "alarm.h"
#include "maincpu.h"
#include "mousedrv.h"

static void mouse_button_left(int pressed);
static void mouse_button_right(int pressed);
static void mouse_button_middle(int pressed);
static void mouse_button_up(int pressed);
static void mouse_button_down(int pressed);

static log_t ps2mouse_log = LOG_DEFAULT;

#if 0
#define PS2MOUSE_DEBUG(args ...) log_message(ps2mouse_log, args)
#define PS2MOUSE_DEBUG_ENABLED
#endif

/* PS/2 mouse port bits */
#define PS2_CLK_BIT 0x40
#define PS2_DATA_BIT 0x80

/* PS/2 mouse timing */
#define PS2_BIT_DELAY_CLK 75

/* PS/2 mouse commands & replies (TODO: support more commands) */
#define PS2_REPLY_OK 0xfa
#define PS2_REPLY_ERROR 0xfc
#define PS2_CMD_SET_REMOTE_MODE 0xf0
#define PS2_CMD_READ_DATA 0xeb
#define PS2_CMD_GET_DEV_ID 0xf2
#define PS2_REPLY_DEV_ID 0x00

/* PS/2 mouse movement packet bits */
#define PS2_MDATA_YO 0x80
#define PS2_MDATA_XO 0x40
#define PS2_MDATA_YS 0x20
#define PS2_MDATA_XS 0x10
#define PS2_MDATA_A1 0x08
#define PS2_MDATA_MB 0x04
#define PS2_MDATA_RB 0x02
#define PS2_MDATA_LB 0x01

/* PS/2 mouse variables */
static uint8_t ps2mouse_value;
static uint8_t ps2mouse_in;
static uint8_t ps2mouse_out;
static uint8_t ps2mouse_prev;
static uint8_t ps2mouse_parity;
static int16_t ps2mouse_lastx;
static int16_t ps2mouse_lasty;
static uint8_t ps2mouse_buttons;

/* PS/2 transmission state */
enum {
    PS2_FROMTO_IDLE=0,
    PS2_FROM_START, /* mouse <- cpu */
    PS2_FROM_D0,
    PS2_FROM_D1,
    PS2_FROM_D2,
    PS2_FROM_D3,
    PS2_FROM_D4,
    PS2_FROM_D5,
    PS2_FROM_D6,
    PS2_FROM_D7,
    PS2_FROM_PARITY,
    PS2_FROM_STOP,
    PS2_FROM_ACK,
    PS2_CHECK_SEND, /* mouse -> cpu */
    PS2_TO_D0,
    PS2_TO_D1,
    PS2_TO_D2,
    PS2_TO_D3,
    PS2_TO_D4,
    PS2_TO_D5,
    PS2_TO_D6,
    PS2_TO_D7,
    PS2_TO_PARITY,
    PS2_TO_STOP
} ps2mouse_xmit_state = PS2_FROMTO_IDLE;

/* Output buffer */
#define PS2_QUEUE_SIZE 8
static uint8_t ps2mouse_queue[PS2_QUEUE_SIZE];
static uint8_t ps2mouse_queue_head;
static uint8_t ps2mouse_queue_tail;

/* ------------------------------------------------------------------------- */

static uint8_t userport_ps2mouse_read_pbx(uint8_t orig);
static void userport_ps2mouse_store_pbx(uint8_t value, int pulse);
static int userport_ps2mouse_enable(int val);
static int userport_ps2mouse_write_snapshot_module(snapshot_t *s);
static int userport_ps2mouse_read_snapshot_module(snapshot_t *s);

static userport_device_t ps2mouse_device = {
    "PS/2 Mouse",                                   /* device name */
    JOYSTICK_ADAPTER_ID_NONE,                       /* this is NOT a joystick adapter */
    USERPORT_DEVICE_TYPE_MOUSE_ADAPTER,             /* device is a mouse adapter */
    userport_ps2mouse_enable,                       /* enable function */
    userport_ps2mouse_read_pbx,                     /* read pb0-pb7 function */
    userport_ps2mouse_store_pbx,                    /* store pb0-pb7 function */
    NULL,                                           /* NO read pa2 pin function */
    NULL,                                           /* NO store pa2 pin function */
    NULL,                                           /* NO read pa3 pin function */
    NULL,                                           /* NO store pa3 pin function */
    0,                                              /* pc pin is NOT needed */
    NULL,                                           /* NO store sp1 pin function */
    NULL,                                           /* NO read sp1 pin function */
    NULL,                                           /* NO store sp2 pin function */
    NULL,                                           /* NO read sp2 pin function */
    NULL,                                           /* NO reset function */
    NULL,                                           /* NO powerup function */
    userport_ps2mouse_write_snapshot_module,        /* snapshot write function */
    userport_ps2mouse_read_snapshot_module          /* snapshot read function */
};

static int ps2mouse_enabled = 0;

static int userport_ps2mouse_enable(int val)
{
    ps2mouse_enabled = (unsigned int)(val ? 1 : 0);

    return 0;
}

int userport_ps2mouse_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_MOUSE_PS2, &ps2mouse_device);
}

/* ---------------------------------------------------------------------*/

static int userport_ps2mouse_write_snapshot_module(snapshot_t *s)
{
    /* FIXME */
    return 0;
}

static int userport_ps2mouse_read_snapshot_module(snapshot_t *s)
{
    /* FIXME */
    return 0;
}

/* ------------------------------------------------------------------------- */


static int ps2mouse_queue_put(uint8_t value)
{
    uint8_t new_head = (ps2mouse_queue_head + 1) & (PS2_QUEUE_SIZE - 1);
    if (new_head == ps2mouse_queue_tail) {
#ifdef PS2MOUSE_DEBUG_ENABLED
        PS2MOUSE_DEBUG("queue full!");
#endif
        return 0;
    }
    ps2mouse_queue[ps2mouse_queue_head] = value;
    ps2mouse_queue_head = new_head;
    return 1;
}

static int ps2mouse_queue_empty(void)
{
    return (ps2mouse_queue_head == ps2mouse_queue_tail);
}

static uint8_t ps2mouse_queue_get(void)
{
    uint8_t retval = ps2mouse_queue[ps2mouse_queue_tail];
    ++ps2mouse_queue_tail;
    ps2mouse_queue_tail &= (PS2_QUEUE_SIZE - 1);
    return retval;
}

/* ------------------------------------------------------------------------- */


static int ps2mouse_handle_command(uint8_t value)
{
    int16_t diff_x, diff_y, new_x, new_y;
    uint8_t new_buttons;
#ifdef PS2MOUSE_DEBUG_ENABLED
    PS2MOUSE_DEBUG("cmd: got %02x", value);
#endif
    ps2mouse_xmit_state = PS2_CHECK_SEND;

    if (ps2mouse_parity) {
#ifdef PS2MOUSE_DEBUG_ENABLED
        PS2MOUSE_DEBUG("parity error");
#endif
        return ps2mouse_queue_put(PS2_REPLY_ERROR);
    }

    switch (value) {
        case PS2_CMD_GET_DEV_ID:
            return (ps2mouse_queue_put(PS2_REPLY_OK)
                    && ps2mouse_queue_put(PS2_REPLY_DEV_ID));
            break;

        case PS2_CMD_SET_REMOTE_MODE:
#ifdef HAVE_MOUSE
            mouse_poll(); /* HACK: should probably happen elsewhere */
            mouse_get_raw_int16(&ps2mouse_lastx, &ps2mouse_lasty);
#endif
            return (ps2mouse_queue_put(PS2_REPLY_OK));
            break;

        case PS2_CMD_READ_DATA:
            new_buttons = ps2mouse_buttons;
#ifdef HAVE_MOUSE
            mouse_poll(); /* HACK: should probably happen elsewhere */
            mouse_get_raw_int16(&new_x, &new_y);

            diff_x = (new_x - ps2mouse_lastx);
            if (diff_x < 0) {
                new_buttons |= PS2_MDATA_XS;
            }
            if (diff_x < -256 || diff_x > 255) {
                new_buttons |= PS2_MDATA_XO;
            }
            ps2mouse_lastx = new_x;

            diff_y = (int16_t)(new_y - ps2mouse_lasty);
            if (diff_y < 0) {
                new_buttons |= PS2_MDATA_YS;
            }
            if (diff_y < -256 || diff_y > 255) {
                new_buttons |= PS2_MDATA_YO;
            }
            ps2mouse_lasty = new_y;
#else
            diff_x = 0;
            diff_y = 0;
#endif
#ifdef PS2MOUSE_DEBUG_ENABLED
            PS2MOUSE_DEBUG("x/y/b: %02x, %02x, %02x", diff_x, diff_y, new_buttons);
#endif
            return (ps2mouse_queue_put(PS2_REPLY_OK)
                    && ps2mouse_queue_put(new_buttons)
                    && ps2mouse_queue_put((uint8_t)diff_x)
                    && ps2mouse_queue_put((uint8_t)diff_y));
            break;

        default:
#ifdef PS2MOUSE_DEBUG_ENABLED
            PS2MOUSE_DEBUG("unsupported command %02x", value);
#endif
            return (ps2mouse_queue_put(PS2_REPLY_ERROR) & 0);
            break;
    }
    return 0;
}


struct alarm_s *c64dtv_ps2mouse_alarm;

static void c64dtv_ps2mouse_alarm_handler(CLOCK offset, void *data)
{
    int another_alarm = 1;

    alarm_unset(c64dtv_ps2mouse_alarm);

    ps2mouse_out ^= PS2_CLK_BIT;
    ps2mouse_out &= ~PS2_DATA_BIT;

    switch (ps2mouse_xmit_state) {
        case PS2_FROM_START:
            ps2mouse_value = 0;
            ps2mouse_parity = 0;
            ps2mouse_out |= (ps2mouse_in & PS2_DATA_BIT);
#ifdef PS2MOUSE_DEBUG_ENABLED
            PS2MOUSE_DEBUG("start: clk/data = %i/%i", (ps2mouse_out >> 6) & 1, (ps2mouse_out >> 7) & 1);
#endif
            if ((ps2mouse_out & PS2_CLK_BIT) == 0) {
                ++ps2mouse_xmit_state;
            }
            break;

        case PS2_FROM_D0:
        case PS2_FROM_D1:
        case PS2_FROM_D2:
        case PS2_FROM_D3:
        case PS2_FROM_D4:
        case PS2_FROM_D5:
        case PS2_FROM_D6:
        case PS2_FROM_D7:
            if (ps2mouse_out & PS2_CLK_BIT) {
                ps2mouse_value >>= 1;
                if (ps2mouse_in & PS2_DATA_BIT) {
                    ps2mouse_value |= 0x80;
                    ps2mouse_parity ^= PS2_DATA_BIT;
                }
            }
            ps2mouse_out |= (ps2mouse_in & PS2_DATA_BIT);
#ifdef PS2MOUSE_DEBUG_ENABLED
            PS2MOUSE_DEBUG("d%i: clk/data = %i/%i", ps2mouse_xmit_state - PS2_FROM_D0, (ps2mouse_out >> 6) & 1, (ps2mouse_out >> 7) & 1);
#endif
            if ((ps2mouse_out & PS2_CLK_BIT) == 0) {
                ++ps2mouse_xmit_state;
            }
            break;

        case PS2_FROM_PARITY:
            if (ps2mouse_out & PS2_CLK_BIT) {
                if (ps2mouse_in & PS2_DATA_BIT) {
                    ps2mouse_parity ^= PS2_DATA_BIT;
                }
            }
            ps2mouse_out |= PS2_DATA_BIT;
#ifdef PS2MOUSE_DEBUG_ENABLED
            PS2MOUSE_DEBUG("parity: clk/data = %i/%i", (ps2mouse_out >> 6) & 1, (ps2mouse_out >> 7) & 1);
#endif
            if ((ps2mouse_out & PS2_CLK_BIT) == 0) {
                ++ps2mouse_xmit_state;
            }
            break;

        case PS2_FROM_STOP:
            if (ps2mouse_out & PS2_CLK_BIT) {
                if (ps2mouse_in & PS2_DATA_BIT) {
                    ps2mouse_parity ^= PS2_DATA_BIT;
                }
                ps2mouse_out |= PS2_DATA_BIT;
            } else {
                ps2mouse_out |= ps2mouse_parity;
            }
#ifdef PS2MOUSE_DEBUG_ENABLED
            PS2MOUSE_DEBUG("stop: clk/data = %i/%i", (ps2mouse_out >> 6) & 1, (ps2mouse_out >> 7) & 1);
#endif
            if ((ps2mouse_out & PS2_CLK_BIT) == 0) {
                ++ps2mouse_xmit_state;
            }
            break;

        case PS2_FROM_ACK:
            ps2mouse_out |= PS2_DATA_BIT; /* ps2mouse_parity; */
#ifdef PS2MOUSE_DEBUG_ENABLED
            PS2MOUSE_DEBUG("got %02x, parity: %02x, clk/data = %i/%i", ps2mouse_value, ps2mouse_parity, (ps2mouse_out >> 6) & 1, (ps2mouse_out >> 7) & 1);
#endif
            another_alarm = ps2mouse_handle_command(ps2mouse_value);
            break;

        case PS2_CHECK_SEND:
            if (ps2mouse_queue_empty()) {
                ps2mouse_out |= (PS2_DATA_BIT | PS2_CLK_BIT);
                another_alarm = 0;
                ps2mouse_xmit_state = PS2_FROMTO_IDLE;
#ifdef PS2MOUSE_DEBUG_ENABLED
                PS2MOUSE_DEBUG("all sent. clk/data = %i/%i", (ps2mouse_out >> 6) & 1, (ps2mouse_out >> 7) & 1);
#endif
                break;
            }
            if ((ps2mouse_in & PS2_CLK_BIT) == 0) {
                ps2mouse_out &= ~PS2_CLK_BIT;
#ifdef PS2MOUSE_DEBUG_ENABLED
                PS2MOUSE_DEBUG("hold! clk/data = %i/%i", (ps2mouse_out >> 6) & 1, (ps2mouse_out >> 7) & 1);
#endif
                break;
            }

            if ((ps2mouse_out & PS2_CLK_BIT) == 0) {
                ps2mouse_parity = PS2_DATA_BIT;
                ps2mouse_value = ps2mouse_queue_get();
#ifdef PS2MOUSE_DEBUG_ENABLED
                PS2MOUSE_DEBUG("sending %02x, clk/data = %i/%i", ps2mouse_value, (ps2mouse_out >> 6) & 1, (ps2mouse_out >> 7) & 1);
#endif
                ps2mouse_xmit_state = PS2_TO_D0;
            }
            break;

        case PS2_TO_D0:
        case PS2_TO_D1:
        case PS2_TO_D2:
        case PS2_TO_D3:
        case PS2_TO_D4:
        case PS2_TO_D5:
        case PS2_TO_D6:
        case PS2_TO_D7:
            if (ps2mouse_out & PS2_CLK_BIT) {
                ps2mouse_out |= (ps2mouse_value & 1) ? PS2_DATA_BIT : 0;
                ps2mouse_prev = ps2mouse_out;
                ps2mouse_parity ^= (ps2mouse_value & 1) ? PS2_DATA_BIT : 0;
                ps2mouse_value >>= 1;
            } else {
                ps2mouse_out |= (ps2mouse_prev & PS2_DATA_BIT);
            }
#ifdef PS2MOUSE_DEBUG_ENABLED
            PS2MOUSE_DEBUG("to_d%i: clk/data = %i/%i", ps2mouse_xmit_state - PS2_TO_D0, (ps2mouse_out >> 6) & 1, (ps2mouse_out >> 7) & 1);
#endif
            if ((ps2mouse_out & PS2_CLK_BIT) == 0) {
                ++ps2mouse_xmit_state;
            }
            break;

        case PS2_TO_PARITY:
            if (ps2mouse_out & PS2_CLK_BIT) {
                ps2mouse_out |= ps2mouse_parity;
                ps2mouse_prev = ps2mouse_out;
            } else {
                ps2mouse_out |= (ps2mouse_prev & PS2_DATA_BIT);
            }
#ifdef PS2MOUSE_DEBUG_ENABLED
            PS2MOUSE_DEBUG("to_parity: clk/data = %i/%i", (ps2mouse_out >> 6) & 1, (ps2mouse_out >> 7) & 1);
#endif
            if ((ps2mouse_out & PS2_CLK_BIT) == 0) {
                ++ps2mouse_xmit_state;
            }
            break;

        case PS2_TO_STOP:
            ps2mouse_out |= PS2_DATA_BIT;
#ifdef PS2MOUSE_DEBUG_ENABLED
            PS2MOUSE_DEBUG("stop: clk/data = %i/%i", (ps2mouse_out >> 6) & 1, (ps2mouse_out >> 7) & 1);
#endif
            if ((ps2mouse_out & PS2_CLK_BIT) == 0) {
                ps2mouse_xmit_state = PS2_CHECK_SEND;
            }
            break;

        default:
            ps2mouse_out |= (PS2_DATA_BIT | PS2_CLK_BIT);
            another_alarm = 0;
            ps2mouse_xmit_state = PS2_FROMTO_IDLE;
#ifdef PS2MOUSE_DEBUG_ENABLED
            PS2MOUSE_DEBUG("bug! state %i. clk/data = %i/%i", ps2mouse_xmit_state, (ps2mouse_out >> 6) & 1, (ps2mouse_out >> 7) & 1);
#endif
            break;
    }

    if (another_alarm) {
        alarm_set(c64dtv_ps2mouse_alarm, maincpu_clk + PS2_BIT_DELAY_CLK);
    }
}


void userport_ps2mouse_store_pbx(uint8_t value, int pulse)
{
    ps2mouse_in = value;
    if (((ps2mouse_prev & PS2_CLK_BIT) == 0) && (value & PS2_CLK_BIT)
        && (ps2mouse_xmit_state == PS2_FROMTO_IDLE) && ((value & PS2_DATA_BIT) == 0)) {
        ps2mouse_xmit_state = PS2_FROM_START;
        ps2mouse_out = value;
        alarm_set(c64dtv_ps2mouse_alarm, maincpu_clk + PS2_BIT_DELAY_CLK);
    }
    ps2mouse_prev = value;
    return;
}

uint8_t userport_ps2mouse_read_pbx(uint8_t orig)
{
    return ps2mouse_out;
}

/* ------------------------------------------------------------------------- */

static void userport_ps2mouse_alarm_init(void)
{
    c64dtv_ps2mouse_alarm = alarm_new(maincpu_alarm_context, "PS2MOUSEAlarm",
                                      c64dtv_ps2mouse_alarm_handler, NULL);
}

/* called from src/c64dtv/c64dtvmem.c:979 */
void ps2mouse_reset(void)
{
    ps2mouse_in = 0xff;
    ps2mouse_out = 0xff;
    ps2mouse_prev = 0xff;
    ps2mouse_xmit_state = PS2_FROMTO_IDLE;
    ps2mouse_parity = 0;
    ps2mouse_lastx = 0;
    ps2mouse_lasty = 0;
    ps2mouse_buttons = PS2_MDATA_A1;
    ps2mouse_queue_head = 0;
    ps2mouse_queue_tail = 0;
    return;
}

/* ------------------------------------------------------------------------- */

static const mouse_func_t mouse_funcs =
{
    mouse_button_left,
    mouse_button_right,
    mouse_button_middle,
    mouse_button_up,
    mouse_button_down
};

/* called from src/c64dtv/c64dtv.c:507 */
int mouse_ps2_resources_init(void)
{
    return mousedrv_resources_init(&mouse_funcs);
}

/* called from src/c64dtv/c64dtv.c:637 */
int mouse_ps2_cmdline_options_init(void)
{
    return mousedrv_cmdline_options_init();
}

/* called from src/c64dtv/c64dtv.c:776 */
void mouse_ps2_init(void)
{
    if (ps2mouse_log == LOG_DEFAULT) {
        ps2mouse_log = log_open("ps2mouse");
    }

    userport_ps2mouse_alarm_init();

    ps2mouse_reset();
    mousedrv_init();
}

/* called from src/c64dtv/c64dtv.c:823 */
void mouse_ps2_shutdown(void)
{
}

static void mouse_button_left(int pressed)
{
    if (pressed) {
        ps2mouse_buttons |= PS2_MDATA_LB;
    } else {
        ps2mouse_buttons &= ~PS2_MDATA_LB;
    }
}

static void mouse_button_middle(int pressed)
{
    if (pressed) {
        ps2mouse_buttons |= PS2_MDATA_MB;
    } else {
        ps2mouse_buttons &= ~PS2_MDATA_MB;
    }
}

static void mouse_button_right(int pressed)
{
    if (pressed) {
        ps2mouse_buttons |= PS2_MDATA_RB;
    } else {
        ps2mouse_buttons &= ~PS2_MDATA_RB;
    }
}

static void mouse_button_up(int pressed)
{
}

static void mouse_button_down(int pressed)
{
}
