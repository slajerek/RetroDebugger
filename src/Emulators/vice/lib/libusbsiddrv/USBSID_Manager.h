/*
 * USBSID-Pico is a RPi Pico (RP2040/RP2350) based board for interfacing one
 * or two MOS SID chips and/or hardware SID emulators over (WEB)USB with your
 * computer, phone or ASID supporting player.
 *
 * USBSID_Manager.h
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

#ifndef _USBSID_MANAGER_H_
#define _USBSID_MANAGER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "USBSID.h"


/**
 * @brief
 * Enumerates every attached USBSID-Pico, opens each board as an isolated
 * USBSID_Class instance and exposes them as one virtual USBSID
 * address space where each logical SID is accessible through it's index
 * number 0..TotalSIDs()-1 that USBSID_Manager::WriteRing()/WriteRingCycled()
 * route to the board that actually owns the SID.
 * Enumerated in USB connection order via provided order of serialnumbers.
 *
 * Connected board's own local register addressing (config) is not changed
 * in any way. This manageronly decides which board's USBSID_Class a write
 * goes to. The caller still computes the same board-local register it
 * would for a single connected board.
 *
 */
class USBSID_Manager {
  public:

    /* BoardInfo is generated once for each board when OpenAll() is called.
     * `socketconfig` stores each boards raw 12-byte GetSocketConfig() reply.
     * USBSID_GetSocketConfig() only returns real data on its first call
     * per device (see USBSID.h) and this cache is the only place after
     * OpenAll() to read it from. */
    struct BoardInfo {
      std::string serial;
      int index = -1;                   /* 0-based, USB connect/enumeration order */
      int numsids = 0;                  /* 1 ~ 4 */
      int fmoplsid = -1;                /* -1 if this board has none */
      int pcbversion = -1;              /* integer version of the pcb e.g. 10, 13 or 15 */
      bool socketconfig_valid = false;  /* validation check */
      uint8_t socketconfig[SOCKET_BUFFER_SIZE] = {0};
    };

    /* One `LogicalSlot` per (virtual) SID per board. This information is stored
     * to identify a SID in regard to thes present board and what the SIDType is.
     * (0 unknown, 1 N/A, 2 MOS8580, 3 MOS6581, 4 FMopl) is
     * `local_slot` is the board's own SID number, 0 based, as its config
     * assigns it (the SID id). `local_slot * 0x20` is the register block
     * the board routes to that SID. A board's slots are listed in that
     * order, following flipped/mixed socket presets. */
    struct LogicalSlot {
      int board_index;
      int local_slot;
      int sid_type;
    };

    USBSID_Manager() = default;
    ~USBSID_Manager() { CloseAll(); }

    USBSID_Manager(const USBSID_Manager &) = delete;
    USBSID_Manager & operator=(const USBSID_Manager &) = delete;

    /** @brief List every attached USBSID-Pico without opening any of them. */
    static std::vector<USBSID_NS::USBSID_DeviceInfo> Enumerate(void);

    /**
     * @brief Enumerate and open boards, retrieve info and generate `BoardInfo`.
     *
     * With `serials` empty, opens every attached board in USB connect
     * (enumeration) order. With `serials` non-empty, opens exactly the
     * boards belonging to supplied `serials` and in in that order.
     * board_index 0 is `serials[0]`, etc.
     * That order is also the order logical SID numbering follows
     * (see BuildLogicalMap()) and the order every broadcast (`*All()`) call
     * visits boards in. A serial not currently attached, or a board that
     * fails to open (permissions, unplugged mid-call), is skipped instead
     * of aborting opening the rest.
     *
     * @returns false if no board could be opened at all.
     */
    bool OpenAll(const std::vector<std::string> & serials = {},
                 bool start_threaded = true, bool with_cycles = true);
    void CloseAll(void);

    /** @brief Issue each open board's reset-equivalent command back-to-back,
     * for a coarse (not phase-locked) synchronized playback start. */
    void StartAll(void);

    int TotalSIDs(void) const { return (int)logical_map_.size(); }
    int BoardCount(void) const { return (int)boards_.size(); }
    const std::vector<BoardInfo> & Boards(void) const { return boards_; }
    const std::vector<LogicalSlot> & LogicalMap(void) const { return logical_map_; }

    /* reg/val are already board local addressing, exactly as a call to a single
     * board would be */
    void WriteRing(int logical_sid, uint8_t reg, uint8_t val);
    void WriteRingCycled(int logical_sid, uint8_t reg, uint8_t val, uint16_t cycles);
    unsigned char Read(int logical_sid, uint8_t reg);

    /* `count` x (reg, val, cycles hi, cycles lo) to the board owning
     * `logical_sid` in one lock, regs already board local */
    void WriteRingCycledN(int logical_sid, const uint8_t *items, int count);

    /* Free bytes in the ringbuffer of the board owning `logical_sid`, 0 for
     * an unknown SID. Each cycled write takes 4 bytes, there is no overflow
     * protection. A producer checks this before writing */
    int RingFreeBytes(int logical_sid);

    /* Flush only the board owning `logical_sid` */
    void FlushBoard(int logical_sid);

    /* Broadcast to every open board, in board open order */
    void FlushAll(void);
    void ResetRingBufferAll(void);
    void ResetAllRegistersAll(void);
    void UnMuteAll(void);
    void MuteAll(void);
    void SetMutedAll(bool muted);  /* Sets the firmware muted state, see USBSID_SetMuted() */
    void SetClockRateAll(long clockrate_cycles, bool suspend_sids);

  private:
    std::vector<std::unique_ptr<USBSID_NS::USBSID_Class>> devices_;
    std::vector<BoardInfo> boards_;
    std::vector<LogicalSlot> logical_map_;

    void BuildLogicalMap(void);
};

#endif /* _USBSID_MANAGER_H_ */
