/*
 * midi-win32-drv.c - MIDI emulation for win32.
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
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

#undef DEBUG

#include "vice.h"

#ifdef WINDOWS_COMPILE

#ifdef HAVE_MIDI

#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>

#include "archdep.h"
#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "mididrv.h"
#include "util.h"

#include "resources.h"

#if 0
#ifndef DWORD_PTR
#define DWORD_PTR unsigned long
#endif
#endif

/* ------------------------------------------------------------------------- */

static log_t mididrv_log = LOG_DEFAULT;

static HMIDIIN handle_in = 0;
static HMIDIOUT handle_out = 0;

static void CALLBACK midi_callback(HMIDIIN handle, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

/* ------------------------------------------------------------------------- */

#define OUT_BUF_LEN 3

static int out_index = 0;
static BYTE out_buf[OUT_BUF_LEN];

/* ------------------------------------------------------------------------- */

#define IN_BUF_LEN 1024

static volatile unsigned int in_wi = 0;
static volatile unsigned int in_ri = 0;
static BYTE in_buf[IN_BUF_LEN];

static char *midi_in_dev;
static char *midi_out_dev;
static int midi_in_dev_int = 0;
static int midi_out_dev_int = 0;

static int set_midi_in_dev(const char *val, void *param)
{
    char realval[0x10];
    if (val) {
        midi_in_dev_int = atoi(val);
    }
    sprintf(realval, "%d", midi_in_dev_int);
    util_string_set(&midi_in_dev, realval);
    return 0;
}

static int set_midi_out_dev(const char *val, void *param)
{
    char realval[0x10];
    if (val) {
        midi_out_dev_int = atoi(val);
    }
    sprintf(realval, "%d", midi_out_dev_int);
    util_string_set(&midi_out_dev, realval);
    return 0;
}

static const resource_string_t resources_string[] = {
    { "MIDIInDev", ARCHDEP_MIDI_IN_DEV, RES_EVENT_NO, NULL,
      &midi_in_dev, set_midi_in_dev, NULL },
    { "MIDIOutDev", ARCHDEP_MIDI_OUT_DEV, RES_EVENT_NO, NULL,
      &midi_out_dev, set_midi_out_dev, NULL },
    RESOURCE_STRING_LIST_END
};

int mididrv_resources_init(void)
{
    return resources_register_string(resources_string);
}

void mididrv_resources_shutdown(void)
{
    lib_free(midi_in_dev);
    lib_free(midi_out_dev);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-midiin", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MIDIInDev", NULL,
      "<number>", "Specify MIDI-In device" },
    { "-midiout", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MIDIOutDev", NULL,
      "<number>", "Specify MIDI-Out device" },
    CMDLINE_LIST_END
};

int mididrv_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

static void reset_fifo(void)
{
    in_wi = 0;
    in_ri = 0;
}

static int write_fifo(BYTE data)
{
    if (((in_wi - in_ri) % IN_BUF_LEN) == (IN_BUF_LEN - 1)) {
        return 1;
    }

    in_buf[in_wi] = data;
    in_wi = (in_wi + 1) % IN_BUF_LEN;
    return 0;
}

static int read_fifo(BYTE *data)
{
    if (((in_wi - in_ri) % IN_BUF_LEN) != 0) {
        *data = in_buf[in_ri];
        in_ri = (in_ri + 1) % IN_BUF_LEN;
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static int message_len(BYTE msg)
{
    int len = 0;

    switch (msg & 0xf0) {
        case 0x80: /* Note Off */
        case 0x90: /* Note On */
        case 0xa0: /* Polyphonic Aftertouch */
        case 0xb0: /* Control Change */
        case 0xe0: /* Pitch Wheel */
            len = 3;
            break;
        case 0xc0: /* Program Change */
        case 0xd0: /* Channel Aftertouch */
            len = 2;
            break;
        case 0xf0: /* Special */
            switch (msg) {
                case 0xf0: /* Sysex Start (shouldn't happen here) */
                case 0xf7: /* Sysex End (shouldn't happen here) */
                    len = -1;
                    break;
                case 0xf2: /* Song Pointer */
                    len = 3;
                    break;
                case 0xf1: /* Quarter Frame */
                case 0xf3: /* Song Select */
                case 0xf9: /* Measure End */
                    len = 2;
                    break;
                case 0xf6: /* Tuning Request */
                case 0xf8: /* Timing Clock */
                case 0xfa: /* Start */
                case 0xfb: /* Continue */
                case 0xfc: /* Stop */
                case 0xfe: /* Active Sensing */
                case 0xff: /* Reset */
                    len = 1;
                    break;
                default:
                    break;
            }
            break;
        default: /* running status */
            len = 2;
            break;
    }
    return len;
}

static int num_in_devices, idx_in_device;
static int num_out_devices, idx_out_device;

void mididrv_ui_reset_device_list(int device)
{
    if (device == 0) {
        num_in_devices = midiInGetNumDevs();
        idx_in_device = 0;
    } else {
        num_out_devices = midiOutGetNumDevs();
        idx_out_device = 0;
    }
}

char *mididrv_ui_get_next_device_name(int device, int *id)
{
    MMRESULT ret;
    static MIDIINCAPS mic;
    static MIDIOUTCAPS moc;

    if (device == 0) {
        if (idx_in_device < num_in_devices) {
            ret = midiInGetDevCaps(idx_in_device, &mic, sizeof(MIDIINCAPS));
            if (ret == MMSYSERR_NOERROR) {
                *id = idx_in_device;
                idx_in_device++;
                return mic.szPname;
            } else {
                return "unknown";
            }
        } else {
            return NULL;
        }
    } else {
        if (idx_out_device < num_out_devices) {
            ret = midiOutGetDevCaps(idx_out_device, &moc, sizeof(MIDIOUTCAPS));
            if (ret == MMSYSERR_NOERROR) {
                *id = idx_out_device;
                idx_out_device++;
                return moc.szPname;
            } else {
                return "unknown";
            }
        } else {
            return NULL;
        }
    }
}

static void dump_sources(void)
{
    int i, n;
    MMRESULT ret;
    MIDIINCAPS mic;

    n = midiInGetNumDevs();
    log_message(mididrv_log,"found %d sources", n);
    for (i = 0 ; i < n; ++i) {
        ret = midiInGetDevCaps(i, &mic, sizeof(MIDIINCAPS));
        log_message(mididrv_log, "source #%d: %s",
            i, (ret == MMSYSERR_NOERROR) ? mic.szPname : "Error getting name");
    }
}

static void dump_destinations(void)
{
    int i, n;
    MMRESULT ret;
    MIDIOUTCAPS moc;

    n = midiOutGetNumDevs();
    log_message(mididrv_log, "found %d destinations", n);
    for (i = 0; i < n; ++i) {
        ret = midiOutGetDevCaps(i, &moc, sizeof(MIDIOUTCAPS));
        log_message(mididrv_log, "destination #%d: %s",
            i, (ret == MMSYSERR_NOERROR) ? moc.szPname : "Error getting name");
    }
}

/* ------------------------------------------------------------------------- */

void mididrv_init(void)
{
    if (mididrv_log == LOG_DEFAULT) {
        mididrv_log = log_open("MIDIdrv");
    }
    log_message(mididrv_log, "init driver");
    dump_sources();
    dump_destinations();
}

/* opens a MIDI-In device, returns handle */
int mididrv_in_open(void)
{
    MMRESULT ret;

    log_message(mididrv_log, "Opening MIDI-In device #%d", midi_in_dev_int);
    if (handle_in) {
        mididrv_in_close();
    }

    dump_sources();

    if (midi_in_dev_int != -1) {
        ret = midiInOpen(&handle_in, midi_in_dev_int, (DWORD_PTR)midi_callback, 0, CALLBACK_FUNCTION);
        if (ret != MMSYSERR_NOERROR) {
            log_error(mididrv_log, "Cannot open MIDI-In device #%d!", midi_in_dev_int);
            handle_in = 0;
            return -1;
        }
    } else {
        handle_in = 0;
        return -1;
    }

    /* reset FIFO */
    reset_fifo();

    /* can theoretically return MMSYSERR_INVALHANDLE */
    ret = midiInStart(handle_in);

    return 0;
}

/* opens a MIDI-Out device, returns handle */
int mididrv_out_open(void)
{
    MMRESULT ret;

    log_message(mididrv_log, "Opening MIDI-Out device #%d", midi_out_dev_int);
    if (handle_out) {
        mididrv_out_close();
    }

    dump_destinations();

    if (midi_out_dev_int != -1) {
        ret = midiOutOpen(&handle_out, midi_out_dev_int, 0, 0, CALLBACK_NULL);
        if (ret != MMSYSERR_NOERROR) {
            log_error(mididrv_log, "Cannot open MIDI-Out device #%d!", midi_out_dev_int);
            handle_out = 0;
            return -1;
        }
    } else {
        handle_out = 0;
        return -1;
    }

    /* reset buffer */
    out_index = 0;

    return 0;
}

/* closes the MIDI-In device*/
void mididrv_in_close(void)
{
    MMRESULT ret;
#ifdef DEBUG
    log_message(mididrv_log, "in_close");
#endif
    if (!handle_in) {
        log_error(mididrv_log, "Attempt to close MIDI-In device that wasn't open!");
        return;
    }
    /* can theoretically return MMSYSERR_INVALHANDLE */
    ret = midiInReset(handle_in);

    ret = midiInClose(handle_in);
    if (ret != MMSYSERR_NOERROR) {
        log_error(mididrv_log, "Couldn't close MIDI-In device.");
    }
    handle_in = 0;
}

/* closes the MIDI-Out device*/
void mididrv_out_close(void)
{
    MMRESULT ret;
#ifdef DEBUG
    log_message(mididrv_log, "out_close");
#endif
    if (!handle_out) {
        log_error(mididrv_log, "Attempt to close MIDI-Out device that wasn't open!");
        return;
    }

    /* can theoretically return MMSYSERR_INVALHANDLE */
    ret = midiOutReset(handle_out);

    ret = midiOutClose(handle_out);
    if (ret != MMSYSERR_NOERROR) {
        log_error(mididrv_log, "Couldn't close MIDI-Out device.");
    }
    handle_out = 0;
}

/* sends a byte to MIDI-Out */
void mididrv_out(uint8_t b)
{
    MMRESULT ret;
    int thres;

#ifdef DEBUG
    log_message(mididrv_log, "out %02x", b);
#endif

    out_buf[out_index] = b;
    out_index++;
    if (out_index > OUT_BUF_LEN) {
        out_index = 0;
        log_error(mididrv_log, "MIDI-Out overrun.");
    }

    thres = message_len(out_buf[0]);

    /* flush when enough bytes have been queued */
    if (out_index >= thres) {
        DWORD data;

        out_index = 0;
        data = out_buf[0] | (out_buf[1] << 8) | (out_buf[2] << 16);
#ifdef DEBUG
        log_message(mididrv_log, "flushing out %06lx", data);
#endif
        ret = midiOutShortMsg(handle_out, data);
        if (ret != MMSYSERR_NOERROR) {
            log_error(mididrv_log, "Failed to output data on MIDI-Out device.");
        }
    }

    return;
}

static void CALLBACK midi_callback(HMIDIIN handle, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    int len;
    int i;
    switch (uMsg) {
        case MIM_DATA:
#ifdef DEBUG
            log_message(mididrv_log, "MIDI callback got %08lx", dwParam1);
#endif
            len = message_len((BYTE)(dwParam1 & 0xff));
            for (i = 0; i < len; i++) {
                write_fifo((BYTE)(dwParam1 & 0xff));
                dwParam1 >>= 8;
            }
            break;
        case MIM_LONGDATA:
            break;
        case MIM_OPEN:
        case MIM_CLOSE:
        case MIM_ERROR:
        case MIM_LONGERROR:
        case MIM_MOREDATA:
            break;
        default:
            break;
    }
}


/* gets a byte from MIDI-In, returns !=0 if byte received, byte in *b. */
int mididrv_in(uint8_t *b)
{
    if (!handle_in) {
        log_error(mididrv_log, "Attempt to read from closed MIDI-In port!");
        return -1;
    }

    if (read_fifo(b)) {
#ifdef DEBUG
        log_message(mididrv_log, "in got %02x", *b);
#endif
        return 1;
    }
    return 0;
}

#endif
#endif
