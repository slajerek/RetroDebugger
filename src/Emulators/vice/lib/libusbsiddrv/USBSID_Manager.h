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
 * Local register addressing (config) of a connected board is not changed
 * The manager only decides which board's USBSID_Class gets a write, the
 * caller computes the same board-local register as for a single board
 *
 */
class USBSID_Manager {
  public:

    /* BoardInfo is generated once for each board when OpenAll() is called
     * socketconfig stores the raw 12-byte GetSocketConfig() reply of the board
     * USBSID_GetSocketConfig() only returns real data on its first call
     * per device (see USBSID.h), after OpenAll() read it from this cache */
    struct BoardInfo {
      std::string serial;
      int index = -1;                   /* 0-based, USB connect/enumeration order */
      int numsids = 0;                  /* 1 ~ 4 */
      int fmoplsid = -1;                /* -1 if this board has none */
      int pcbversion = -1;              /* integer version of the pcb e.g. 10, 13 or 15 */
      bool socketconfig_valid = false;  /* validation check */
      uint8_t socketconfig[SOCKET_BUFFER_SIZE] = {0};
    };

    /* One LogicalSlot per (virtual) SID per board, identifies a SID on the board and its SIDType
     * (0 unknown, 1 N/A, 2 MOS8580, 3 MOS6581, 4 FMopl)
     * local_slot is the SID number on the board, 0 based, as assigned by its config (the SID id)
     * local_slot * 0x20 is the register block the board routes to that SID
     * Slots of a board are listed in that order, following flipped/mixed socket presets */
    struct LogicalSlot {
      int board_index;
      int local_slot;
      int sid_type;
    };

    USBSID_Manager() = default;
    ~USBSID_Manager() { CloseAll(); }

    USBSID_Manager(const USBSID_Manager &) = delete;
    USBSID_Manager & operator=(const USBSID_Manager &) = delete;

    /** @brief List all attached USBSID-Pico boards, none are opened */
    static std::vector<USBSID_NS::USBSID_DeviceInfo> Enumerate(void);

    /**
     * @brief Enumerate and open boards, retrieve info and generate BoardInfo, returns false if no board could be opened
     *
     * With serials empty all attached boards are opened in USB connect
     * (enumeration) order, with serials set only those boards are opened, in that order
     * board_index 0 is serials[0] etc
     * This order is used for logical SID numbering (see BuildLogicalMap())
     * and by all broadcast (*All()) calls
     * A serial not attached, or a board that fails to open (permissions,
     * unplugged mid-call), is skipped and the rest is opened
     */
    bool OpenAll(const std::vector<std::string> & serials = {},
                 bool start_threaded = true, bool with_cycles = true);
    void CloseAll(void);

    /** @brief Issue the reset-equivalent command to each open board back-to-back
     * for a coarse (not phase-locked) synchronized playback start */
    void StartAll(void);

    int TotalSIDs(void) const { return (int)logical_map_.size(); }
    int BoardCount(void) const { return (int)boards_.size(); }
    const std::vector<BoardInfo> & Boards(void) const { return boards_; }

    /** @brief Integer firmware version of a board e.g. 81 (0.8.1), 100 (1.0.0)
     * Read once and cached in USBSID_Class (OpenAll() pre-reads it), -1 for an invalid board or unknown */
    int FirmwareVersionInt(int board) const;
    const std::vector<LogicalSlot> & LogicalMap(void) const { return logical_map_; }

    /** @brief Borrow an open board for direct calls (command channel), nullptr when out of range. */
    USBSID_NS::USBSID_Class * Board(int index)
    {
      return (index >= 0 && index < (int)devices_.size()) ? devices_[index].get() : nullptr;
    }

    /* reg/val are board local addressing, as for a single board */
    void WriteRing(int logical_sid, uint8_t reg, uint8_t val);
    void WriteRingCycled(int logical_sid, uint8_t reg, uint8_t val, uint16_t cycles);
    unsigned char Read(int logical_sid, uint8_t reg);
    /* n (1-63) board local registers in one round trip, returns n or -1 */
    int ReadMulti(int logical_sid, const uint8_t *regs, uint8_t *out, int n);

#ifdef USBSID_SID_STREAM
    /* SID stream on all open boards, see USBSID_StreamStart()
     * rate 0 = firmware default, all configured SIDs of each board
     * Returns the number of boards streaming, boards lacking the stream
     * interface are skipped */
    int StreamStartAll(int rate_hz);
    void StreamStopAll(void);
    /* Last streamed value of a board local register of the board owning
     * logical_sid, no USB traffic, -1 when no stream data */
    int ReadCached(int logical_sid, uint8_t reg);
#endif /* USBSID_SID_STREAM */

    /* count x (reg, val, cycles hi, cycles lo) to the board owning
     * logical_sid in one lock, regs are board local */
    void WriteRingCycledN(int logical_sid, const uint8_t *items, int count);

    /* Free bytes in the ringbuffer of the board owning logical_sid, 0 for
     * an unknown SID. Each cycled write takes 4 bytes, there is no overflow
     * protection, check this before writing */
    int RingFreeBytes(int logical_sid);

    /* Flush only the board owning logical_sid */
    void FlushBoard(int logical_sid);

    /* Board unplugged or no longer accepting writes, see USBSID_IsDeviceLost().
     * Close and open again to recover */
    bool BoardLost(int board_index);
    bool AnyBoardLost(void);

    /* Broadcast to all open boards, in board open order */
    void FlushAll(void);
    void ResetRingBufferAll(void);
    void ResetAllRegistersAll(void);
    void UnMuteAll(void);
    void MuteAll(void);
    void SetMutedAll(bool muted);  /* Sets the firmware muted state, see USBSID_SetMuted() */
    void SetClockRateAll(long clockrate_cycles, bool suspend_sids);
    void SetClockRateAll(long clockrate_cycles, bool suspend_sids, bool force);  /* force sends also when the rate is unchanged */

  private:
    std::vector<std::unique_ptr<USBSID_NS::USBSID_Class>> devices_;
    std::vector<BoardInfo> boards_;
    std::vector<LogicalSlot> logical_map_;

    void BuildLogicalMap(void);
};

#endif /* _USBSID_MANAGER_H_ */
