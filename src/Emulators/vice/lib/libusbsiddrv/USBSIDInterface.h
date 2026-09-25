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

  /* Attached board info as returned by enumerate_USBSID() */
  typedef struct {
    char serial[USBSID_SERIAL_LEN];            /* Empty if unreadable */
    uint8_t bus;
    uint8_t port_path_len;
    uint8_t port_path[USBSID_PORTPATH_LEN];
  } USBSIDdevinfo;

  /**
   * @brief: List attached boards without opening them, sorted by bus/port.
   *
   * @param out: array to fill, may be NULL when max is 0
   * @param max: capacity of out
   * @return: number of boards found (may exceed max), -1 on failure
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
   * @brief: Select board to open by USB serial number.
   *
   * @param serial: NUL terminated serial, NULL or "" clears the selection
   */
  void settargetserial_USBSID(USBSIDitf, const char *serial);

  /**
   * @brief: Select board to open by VID/PID match index, used without serial.
   *
   * @param idx: 0-based index in libusb device list order
   */
  void settargetindex_USBSID(USBSIDitf, int idx);

  /**
   * @brief: Copy serial of the opened board into buff, NUL terminated.
   *
   * @param buff: destination, USBSID_SERIAL_LEN bytes is always enough
   * @param len: size of buff
   * @return: full serial length, -1 on invalid arguments
   */
  int getserial_USBSID(USBSIDitf, char *buff, size_t len);

  /* Socket config */

  /**
   * @brief: Read raw socket config, only succeeds once per opened board.
   *
   * @param cfg: USBSID_SOCKETCONFIG_LEN byte destination
   * @return: true if cfg holds a valid reply
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
  // int found_USBSID(USBSIDitf);

  /* Synchronous direct */
  void writesingle_USBSID(USBSIDitf, unsigned char *buff, size_t len);
  unsigned char readsingle_USBSID(USBSIDitf, uint8_t reg);

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
