/*
 * USBSID-Pico is a RPi Pico (RP2040/RP2350) based board for interfacing one
 * or two MOS SID chips and/or hardware SID emulators over (WEB)USB with your
 * computer, phone or ASID supporting player.
 *
 * USBSIDManagerInterface.h
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

#ifndef _USBSID_MANAGER_INTERFACE_H_
#define _USBSID_MANAGER_INTERFACE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "USBSIDInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

  /* USBSID Manager Interface for use in STD C applications
   * Board enumeration with enumerate_USBSID() from USBSIDInterface.h */
  typedef void * USBSIDmgr;

  /* Per opened board info, filled by openall_USBSIDMgr() */
  typedef struct {
    char serial[USBSID_SERIAL_LEN];
    int index;                                   /* 0-based board open order */
    int numsids;                                 /* 1 ~ 4 */
    int fmoplsid;                                /* -1 if none */
    int pcbversion;                              /* e.g. 10, 13 or 15 */
    bool socketconfig_valid;
    uint8_t socketconfig[USBSID_SOCKETCONFIG_LEN];
  } USBSIDboardinfo;

  /* Logical SID to board mapping
   * local_slot is the SID number on the board, 0 based, as assigned by its
   * config. local_slot * 0x20 is the register block of that SID
   * sid_type: 0 unknown, 1 N/A, 2 MOS8580, 3 MOS6581, 4 FMopl */
  typedef struct {
    int board_index;
    int local_slot;
    int sid_type;
  } USBSIDlogicalslot;

  /**
   * @brief Allocate a manager instance, returns handle, NULL on allocation failure
   */
  USBSIDmgr create_USBSIDMgr(void);

  /**
   * @brief Close all boards and free the manager instance
   */
  void close_USBSIDMgr(USBSIDmgr);

  /**
   * @brief Open boards and build the logical SID map, returns true if at least one board opened
   *
   * @param serials serials to open in this order, NULL with count 0 opens
   *                all attached boards in enumeration order
   * @param count number of entries in serials
   * @param start_threaded start each board's ringbuffer thread
   * @param with_cycles use cycled ringbuffer writes
   */
  bool openall_USBSIDMgr(USBSIDmgr, const char *const *serials, size_t count,
                         bool start_threaded, bool with_cycles);
  void closeall_USBSIDMgr(USBSIDmgr);
  void startall_USBSIDMgr(USBSIDmgr);

  /* Info */
  int totalsids_USBSIDMgr(USBSIDmgr);
  int boardcount_USBSIDMgr(USBSIDmgr);

  /**
   * @brief Copy info of an opened board, returns false if board is out of range
   *
   * @param board 0-based board index
   * @param out destination
   */
  bool getboardinfo_USBSIDMgr(USBSIDmgr, int board, USBSIDboardinfo *out);

  /**
   * @brief Firmware version integer of a board, read once by openall_USBSIDMgr(), returns e.g. 81 (0.8.1), 100 (1.0.0), -1 if unknown or board is out of range
   *
   * @param board 0-based board index
   */
  int getfirmwareversionint_USBSIDMgr(USBSIDmgr, int board);

  /**
   * @brief Copy the board mapping of a logical SID, returns false if logical_sid is out of range
   *
   * @param logical_sid 0 .. totalsids_USBSIDMgr() - 1
   * @param out destination
   */
  bool getlogicalslot_USBSIDMgr(USBSIDmgr, int logical_sid, USBSIDlogicalslot *out);

  /**
   * @brief Borrow an opened board as a USBSIDitf for USBSIDInterface.h calls, returns board handle, NULL if board is out of range
   *
   * Owned by the manager, do not pass it to close_USBSID(), invalid after
   * closeall_USBSIDMgr() or close_USBSIDMgr()
   *
   * @param board 0-based board index
   */
  USBSIDitf board_USBSIDMgr(USBSIDmgr, int board);

  /* Routed to the board owning logical_sid, reg is board local */
  void writering_USBSIDMgr(USBSIDmgr, int logical_sid, uint8_t reg, uint8_t val);
  void writeringcycled_USBSIDMgr(USBSIDmgr, int logical_sid, uint8_t reg, uint8_t val, uint16_t cycles);
  unsigned char read_USBSIDMgr(USBSIDmgr, int logical_sid, uint8_t reg);
  /* n (1-63) board local registers in one round trip, n or -1 */
  int readmulti_USBSIDMgr(USBSIDmgr, int logical_sid, const uint8_t *regs, uint8_t *out, int n);
#ifdef USBSID_SID_STREAM
  /* SID stream on all open boards, rate 0 = 50 Hz, number of boards streaming */
  int streamstartall_USBSIDMgr(USBSIDmgr, int rate_hz);
  void streamstopall_USBSIDMgr(USBSIDmgr);
  /* last streamed board local register, no USB traffic, -1 when no data */
  int readcached_USBSIDMgr(USBSIDmgr, int logical_sid, uint8_t reg);
#endif /* USBSID_SID_STREAM */
  /* count x (reg, val, cycles hi, cycles lo) in one lock, regs board local */
  void writeringcycledn_USBSIDMgr(USBSIDmgr, int logical_sid, const uint8_t *items, int count);
  /* free ringbuffer bytes of the board owning logical_sid, 0 for an unknown SID */
  int ringfreebytes_USBSIDMgr(USBSIDmgr, int logical_sid);
  void flushboard_USBSIDMgr(USBSIDmgr, int logical_sid);
  /* board unplugged or no longer accepting writes, close and open again */
  bool boardlost_USBSIDMgr(USBSIDmgr, int board_index);
  bool anyboardlost_USBSIDMgr(USBSIDmgr);

  /* Broadcast to all open boards */
  void flushall_USBSIDMgr(USBSIDmgr);
  void resetringbufferall_USBSIDMgr(USBSIDmgr);
  void resetallregistersall_USBSIDMgr(USBSIDmgr);
  void unmuteall_USBSIDMgr(USBSIDmgr);
  void muteall_USBSIDMgr(USBSIDmgr);
  void setmutedall_USBSIDMgr(USBSIDmgr, bool muted);  /* sets the firmware muted state */
  void setclockrateall_USBSIDMgr(USBSIDmgr, long clockrate_cycles, bool suspend_sids);
  void setclockrateallforced_USBSIDMgr(USBSIDmgr, long clockrate_cycles, bool suspend_sids, bool force);  /* force sends also when the rate is unchanged */

#ifdef __cplusplus
}
#endif

#endif /* _USBSID_MANAGER_INTERFACE_H_ */
