/*
 * USBSID-Pico is a RPi Pico (RP2040/RP2350) based board for interfacing one
 * or two MOS SID chips and/or hardware SID emulators over (WEB)USB with your
 * computer, phone or ASID supporting player.
 *
 * USBSIDInterface.h
 * This file is part of USBSID-Pico-driver (https://github.com/LouDnl/USBSID-Pico-driver)
 * File author: LouD
 *
 * USBSID-Pico-driver
 * Copyright (C) 2024-2026  LouD
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _USBSID_INTERFACE_H_
#define _USBSID_INTERFACE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

  /* Buffer sizes shared with the C++ driver */
  #define USBSID_SERIAL_LEN       64  /* Serial string incl. terminator */
  #define USBSID_PORTPATH_LEN      8  /* USB port path depth */
  #define USBSID_SOCKETCONFIG_LEN 12  /* Raw socket config reply */

  /* Onboard emulator upload file types */
  #define USBSID_UPLOAD_FILE_STDIN 0x00
  #define USBSID_UPLOAD_FILE_SID   0x01
  #define USBSID_UPLOAD_FILE_PRG   0x02

  /* Onboard emulator player commands for playercommand_USBSID() */
  #define USBSID_PLAYER_START 0xE1
  #define USBSID_PLAYER_STOP  0xE2
  #define USBSID_PLAYER_PAUSE 0xE3
  #define USBSID_PLAYER_NEXT  0xE4
  #define USBSID_PLAYER_PREV  0xE5
  #define USBSID_PLAYER_TWO   0xE6

  /* getfeatures_USBSID() bits */
  #define USBSID_FEATURE_RP2350   0x01
  #define USBSID_FEATURE_RGBVU    0x04
  #define USBSID_FEATURE_NET      0x10
  #define USBSID_FEATURE_NSD      0x20
  #define USBSID_FEATURE_SIDSTREAM 0x40  /* SID stream interface (USB interface 7) built in */
  #define USBSID_FEATURE_EMULATOR 0x80

  /* Firmware version integer from which readmulti_USBSID() uses a batch READ, no feature bit */
  #define USBSID_FW_VERSION_BATCH_READ 81

#ifdef USBSID_SID_STREAM
  /* SID stream, see streamstart_USBSID() */
  #define USBSID_STREAM_INFO_LEN       16
  #define USBSID_STREAM_FLAG_DEFERRED  0x20  /* Built late, mirror only */
  #define USBSID_STREAM_FLAG_OTHER     0x40  /* Bus owned by a non USB owner */

  /* Latest stream frame of one SID as returned by getstreamstate_USBSID() */
  typedef struct {
    uint32_t frames;     /* Frames received after the stream started, 0 = no data */
    uint16_t tick;       /* Firmware tick sequence */
    uint8_t flags;       /* USBSID_STREAM_FLAG_* bits */
    uint32_t cycles;     /* PHI1 cycle counter of the snapshot */
    uint32_t processed;  /* Main interface packets the firmware processed */
  } USBSIDstreamstate;

  /* Raw frame callback, runs in the stream event thread ~ no USB I/O on the
   * same board from inside it */
  typedef void (*USBSIDstreamcb)(const uint8_t *frame, int len, void *user);
#endif /* USBSID_SID_STREAM */

  /* Attached board info as returned by enumerate_USBSID() */
  typedef struct {
    char serial[USBSID_SERIAL_LEN];            /* Empty if unreadable */
    uint8_t bus;
    uint8_t port_path_len;
    uint8_t port_path[USBSID_PORTPATH_LEN];
  } USBSIDdevinfo;

  /**
   * @brief List attached boards without opening them, sorted by bus/port, returns number of boards found (may exceed max) or -1 on failure
   *
   * @param out array to fill, may be NULL when max is 0
   * @param max capacity of out
   */
  int enumerate_USBSID(USBSIDdevinfo *out, int max);

  /* USBSID Interface for use in STD C applications */
  typedef void * USBSIDitf;
  USBSIDitf create_USBSID(void);  /* NULL on allocation failure */
  int init_USBSID(USBSIDitf, bool start_threaded, bool with_cycles);
  void close_USBSID(USBSIDitf);
  void pause_USBSID(USBSIDitf);
  void reset_USBSID(USBSIDitf);
  void resetallregisters_USBSID(USBSIDitf);
  void clearbus_USBSID(USBSIDitf);
  void mute_USBSID(USBSIDitf);
  void unmute_USBSID(USBSIDitf);
  void setmuted_USBSID(USBSIDitf, bool muted);  /* sets the firmware muted state, volume writes stay masked while muted */
  void setclockrate_USBSID(USBSIDitf, long clockrate_cycles, bool suspend_sids);
  void setclockrateforced_USBSID(USBSIDitf, long clockrate_cycles, bool suspend_sids, bool force);  /* force sends also when the rate is unchanged */
  long getclockrate_USBSID(USBSIDitf);
  long getrefreshrate_USBSID(USBSIDitf);
  long getrasterrate_USBSID(USBSIDitf);
  int getnumsids_USBSID(USBSIDitf);
  int getfmoplsid_USBSID(USBSIDitf);
  int getpcbversion_USBSID(USBSIDitf);
  void setstereo_USBSID(USBSIDitf, int state);
  void togglestereo_USBSID(USBSIDitf);
  void enablesid_USBSID(USBSIDitf);
  void disablesid_USBSID(USBSIDitf);

  /* Board targeting, call before init_USBSID() */

  /**
   * @brief Select board to open by USB serial number
   *
   * @param serial NUL terminated serial, NULL or "" clears the selection
   */
  void settargetserial_USBSID(USBSIDitf, const char *serial);

  /**
   * @brief Select board to open by VID/PID match index, used without serial
   *
   * @param idx 0-based index in libusb device list order
   */
  void settargetindex_USBSID(USBSIDitf, int idx);

  /**
   * @brief Copy serial of the opened board into buff, NUL terminated, returns full serial length or -1 on invalid arguments
   *
   * @param buff destination, USBSID_SERIAL_LEN bytes is enough
   * @param len size of buff
   */
  int getserial_USBSID(USBSIDitf, char *buff, size_t len);

  /* Socket config */

  /**
   * @brief Read raw socket config, only succeeds once per opened board, returns true if cfg holds a valid reply
   *
   * @param cfg USBSID_SOCKETCONFIG_LEN byte destination
   */
  bool getsocketconfig_USBSID(USBSIDitf, uint8_t *cfg);
  int getsocketnumsids_USBSID(USBSIDitf, int socket, uint8_t *cfg);
  int getsocketchiptype_USBSID(USBSIDitf, int socket, uint8_t *cfg);
  int getsocketsidtype1_USBSID(USBSIDitf, int socket, uint8_t *cfg);
  int getsocketsidtype2_USBSID(USBSIDitf, int socket, uint8_t *cfg);

  /* Helpers */
  bool initialised_USBSID(USBSIDitf);
  bool available_USBSID(USBSIDitf);
  bool portisopen_USBSID(USBSIDitf);
  bool devicelost_USBSID(USBSIDitf);  /* unplugged or no longer accepting writes, close and open again */
  // int found_USBSID(USBSIDitf);

  /* Synchronous direct */
  void writesingle_USBSID(USBSIDitf, unsigned char *buff, size_t len);
  unsigned char readsingle_USBSID(USBSIDitf, uint8_t reg);
  int readmulti_USBSID(USBSIDitf, const uint8_t *regs, uint8_t *out, int n);  /* n (1-63) registers, one round trip, n or -1 */

#ifdef USBSID_SID_STREAM
  /* SID stream (firmware with USBSID_FEATURE_SIDSTREAM), threaded and unthreaded */
  /* rate 50/60/250/500/1000 Hz or 0 (50), sid_mask 0 = all. Returns 0 or -1 */
  int streamstart_USBSID(USBSIDitf, int rate_hz, uint8_t sid_mask);
  int streamstop_USBSID(USBSIDitf);                                   /* 0, -1 for NULL */
  bool streamrunning_USBSID(USBSIDitf);
  int streamquery_USBSID(USBSIDitf, uint8_t *info);                  /* USBSID_STREAM_INFO_LEN bytes, 0 or -1 */
  int readcached_USBSID(USBSIDitf, uint8_t reg);                      /* 0-255, -1 when no data for that SID */
  int getsidmemory_USBSID(USBSIDitf, uint8_t *out);                   /* 128 bytes, mask of SIDs with data or -1 */
  bool getstreamstate_USBSID(USBSIDitf, int sid, USBSIDstreamstate *state);
  void setstreamcallback_USBSID(USBSIDitf, USBSIDstreamcb cb, void *user);
#endif /* USBSID_SID_STREAM */

  /* Command channel, synchronous, use with the thread disabled */
  /**
   * @brief Open without mute, bus clear or clock query, call before init_USBSID()
   *
   * @param on true for a passive open
   */
  void setpassive_USBSID(USBSIDitf, bool on);
  /**
   * @brief Send raw command bytes, returns bytes sent or -1 on failure
   */
  int sendcommand_USBSID(USBSIDitf, const unsigned char *buff, size_t len);
  /**
   * @brief Read one reply of at most len bytes, returns bytes read or -1 on failure
   */
  int readresponse_USBSID(USBSIDitf, unsigned char *buff, size_t len);
  /**
   * @brief Send a 6 byte config command (CONFIG, sub, a, b, c, d), returns bytes sent or -1 on failure
   */
  int sendconfig_USBSID(USBSIDitf, uint8_t sub, uint8_t a, uint8_t b, uint8_t c, uint8_t d);
  /**
   * @brief Read the firmware feature bitmask (USBSID_FEATURE_*), cached, returns bitmask or -1 on failure
   */
  int getfeatures_USBSID(USBSIDitf);
  /**
   * @brief Read the firmware version integer (81 = 0.8.1, 100 = 1.0.0), cached, returns version integer or -1 on failure
   */
  int getfirmwareversionint_USBSID(USBSIDitf);
  /* Onboard emulator, firmware with ONBOARD_EMULATOR=1 */
  /**
   * @brief Upload a tune file to the onboard emulator, returns data bytes sent or -1 on failure
   *
   * @param filetype USBSID_UPLOAD_FILE_*
   */
  int uploadtune_USBSID(USBSIDitf, const uint8_t *data, size_t len, uint8_t filetype);
  int playersetplaytime_USBSID(USBSIDitf, uint32_t ms);  /* max play time, -1 on failure */
  int playerload_USBSID(USBSIDitf, uint8_t subtune);  /* 0 based subtune, loads and starts */
  int playercommand_USBSID(USBSIDitf, uint8_t cmd);  /* USBSID_PLAYER_*, -1 on failure */
  int playermute_USBSID(USBSIDitf, uint8_t chip, uint8_t voice, bool mute);  /* chip 0 = all, voice 0 = chip */
  long playertime_USBSID(USBSIDitf);  /* play time in ms, -1 on failure */
  bool playermuted_USBSID(USBSIDitf, uint8_t *state);  /* 5 bytes ~ chip mask, voice masks chips 1-4 */
  /* Asynchronous direct */
  void writebuffer_USBSID(USBSIDitf p, unsigned char *buff, size_t len);
  void write_USBSID(USBSIDitf, uint8_t reg, uint8_t val);
  void writecycled_USBSID(USBSIDitf, uint8_t reg, uint8_t val, uint16_t cycles);
  unsigned char read_USBSID(USBSIDitf p,  uint8_t reg);

  /* Asynchronous thread */
  void writering_USBSID(USBSIDitf, uint8_t reg, uint8_t val);
  void writeringcycled_USBSID(USBSIDitf, uint8_t reg, uint8_t val, uint16_t cycles);
  void writeringcycledn_USBSID(USBSIDitf, const uint8_t *items, int count);  /* count x (reg, val, cycles hi, cycles lo) */

  /* Thread buffer */
  void enablethread_USBSID(USBSIDitf);
  void disablethread_USBSID(USBSIDitf);
  void setflush_USBSID(USBSIDitf);
  void flush_USBSID(USBSIDitf);
  void resetringbuffer_USBSID(USBSIDitf);
  void restartringbuffer_USBSID(USBSIDitf);
  int ringfree_USBSID(USBSIDitf);  /* free ringbuffer bytes, 4 per cycled write */
  void setbuffsize_USBSID(USBSIDitf, int size);
  void setdiffsize_USBSID(USBSIDitf, int size);

  /* Thread utils */
  void restartthread_USBSID(USBSIDitf, bool with_cycles);

  /* Timing and cycles */
  int_fast64_t waitforcycle_USBSID(USBSIDitf, uint_fast64_t cycles);

#ifdef __cplusplus
}
#endif

#endif /* _USBSID_INTERFACE_H_ */
