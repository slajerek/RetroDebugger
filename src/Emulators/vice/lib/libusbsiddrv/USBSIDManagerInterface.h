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
   * Board enumeration: use enumerate_USBSID() from USBSIDInterface.h */
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
   * local_slot: the board's own SID number, 0 based, as its config assigns
   *             it; local_slot * 0x20 is that SID's register block
   * sid_type: 0 unknown, 1 N/A, 2 MOS8580, 3 MOS6581, 4 FMopl */
  typedef struct {
    int board_index;
    int local_slot;
    int sid_type;
  } USBSIDlogicalslot;

  /**
   * @brief: Allocate a manager instance.
   *
   * @return: handle, NULL on allocation failure
   */
  USBSIDmgr create_USBSIDMgr(void);

  /**
   * @brief: Close all boards and free the manager instance.
   */
  void close_USBSIDMgr(USBSIDmgr);

  /**
   * @brief: Open boards and build the logical SID map.
   *
   * @param serials: serials to open in this order, NULL with count 0 opens
   *                 every attached board in enumeration order
   * @param count: number of entries in serials
   * @param start_threaded: start each board's ringbuffer thread
   * @param with_cycles: use cycled ringbuffer writes
   * @return: true if at least one board opened
   */
  bool openall_USBSIDMgr(USBSIDmgr, const char *const *serials, size_t count,
                         bool start_threaded, bool with_cycles);
  void closeall_USBSIDMgr(USBSIDmgr);
  void startall_USBSIDMgr(USBSIDmgr);

  /* Info */
  int totalsids_USBSIDMgr(USBSIDmgr);
  int boardcount_USBSIDMgr(USBSIDmgr);

  /**
   * @brief: Copy info of an opened board.
   *
   * @param board: 0-based board index
   * @param out: destination
   * @return: false if board is out of range
   */
  bool getboardinfo_USBSIDMgr(USBSIDmgr, int board, USBSIDboardinfo *out);

  /**
   * @brief: Copy the board mapping of a logical SID.
   *
   * @param logical_sid: 0 .. totalsids_USBSIDMgr() - 1
   * @param out: destination
   * @return: false if logical_sid is out of range
   */
  bool getlogicalslot_USBSIDMgr(USBSIDmgr, int logical_sid, USBSIDlogicalslot *out);

  /* Routed to the board owning logical_sid, reg is board local */
  void writering_USBSIDMgr(USBSIDmgr, int logical_sid, uint8_t reg, uint8_t val);
  void writeringcycled_USBSIDMgr(USBSIDmgr, int logical_sid, uint8_t reg, uint8_t val, uint16_t cycles);
  unsigned char read_USBSIDMgr(USBSIDmgr, int logical_sid, uint8_t reg);
  /* count x (reg, val, cycles hi, cycles lo) in one lock, regs board local */
  void writeringcycledn_USBSIDMgr(USBSIDmgr, int logical_sid, const uint8_t *items, int count);
  /* free ringbuffer bytes of the board owning logical_sid, 0 for an unknown SID */
  int ringfreebytes_USBSIDMgr(USBSIDmgr, int logical_sid);
  void flushboard_USBSIDMgr(USBSIDmgr, int logical_sid);

  /* Broadcast to every open board */
  void flushall_USBSIDMgr(USBSIDmgr);
  void resetringbufferall_USBSIDMgr(USBSIDmgr);
  void resetallregistersall_USBSIDMgr(USBSIDmgr);
  void unmuteall_USBSIDMgr(USBSIDmgr);
  void muteall_USBSIDMgr(USBSIDmgr);
  void setmutedall_USBSIDMgr(USBSIDmgr, bool muted);  /* sets the firmware muted state */
  void setclockrateall_USBSIDMgr(USBSIDmgr, long clockrate_cycles, bool suspend_sids);

#ifdef __cplusplus
}
#endif

#endif /* _USBSID_MANAGER_INTERFACE_H_ */
