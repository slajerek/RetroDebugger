/*
 * USBSID-Pico is a RPi Pico (RP2040/RP2350) based board for interfacing one
 * or two MOS SID chips and/or hardware SID emulators over (WEB)USB with your
 * computer, phone or ASID supporting player.
 *
 * USBSID_Manager.cpp
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

#include <algorithm>
#include <cstring>
#include "USBSID_Manager.h"

using namespace USBSID_NS;

std::vector<USBSID_DeviceInfo> USBSID_Manager::Enumerate(void)
{
  return USBSID_EnumerateDevices();
}

bool USBSID_Manager::OpenAll(
  const std::vector<std::string> & serials,
  bool start_threaded, bool with_cycles
)
{
  /* Make sure there are not previously opened boards */
  CloseAll();

  std::vector<USBSID_DeviceInfo> found;
  if (serials.empty()) {
    found = Enumerate();
    if (found.empty()) return false;
  }
  const size_t want = serials.empty() ? found.size() : serials.size();

  for (size_t i = 0; i < want; i++) {
    std::unique_ptr<USBSID_Class> dev(new USBSID_Class());  /* C++11, VICE minimum */
    // auto dev = std::make_unique<USBSID_Class>(); /* C++14 */
    if (!serials.empty()) {
      dev->USBSID_SetTargetSerial(serials[i]);
    } else if (!found[i].serial.empty()) {
      dev->USBSID_SetTargetSerial(found[i].serial);
    } else {
      dev->USBSID_SetTargetIndex((int)i);
    }

    if (dev->USBSID_Init(start_threaded, with_cycles) < 0) {
      /* Couldn't open this particular board (permissions, unplugged between
       * Enumerate() and here, ... or whatever other reason)
       * Skip it and keep going with the rest. */
      continue;
    }

    BoardInfo info;
    info.serial = dev->USBSID_GetSerial();
    info.index = (int)boards_.size();
    info.numsids = dev->USBSID_GetNumSIDs();
    info.fmoplsid = dev->USBSID_GetFMOplSID();
    info.pcbversion = dev->USBSID_GetPCBVersion();

    uint8_t cfg[SOCKET_BUFFER_SIZE];
    info.socketconfig_valid = (dev->USBSID_GetSocketConfig(cfg) != nullptr);
    if (info.socketconfig_valid) {
      memcpy(info.socketconfig, cfg, SOCKET_BUFFER_SIZE);
    }

    boards_.push_back(info);
    devices_.push_back(std::move(dev));
  }

  BuildLogicalMap();
  return !devices_.empty();
}

/* BuildLogicalMap() follows each board's own configuration: a board's SIDs
 * are numbered by the SID id its firmware gave them (READ_SOCKETCFG bytes 8
 * and 9, one nibble per socket SID), and that id is also the register block
 * the firmware routes to it (id * 0x20). Default presets keep ids in socket
 * order, flipped and mixed presets do not: socket order alone falls short. */
void USBSID_Manager::BuildLogicalMap(void)
{
  logical_map_.clear();

  for (size_t b = 0; b < boards_.size(); b++) {
    BoardInfo &board = boards_[b];

    if (!board.socketconfig_valid) {
      /* No usable socket config reply for this board. Fall back to its flat
       * SID count with an unknown type rather than dropping it entirely. */
      for (int n = 0; n < board.numsids; n++) {
        logical_map_.push_back({(int)b, n, 0});
      }
      continue;
    }

    auto &dev = devices_[b];
    const uint8_t *cfg = board.socketconfig;
    const bool mirrored = (cfg[10] & 0x1) != 0;

    /* Every configured SID in socket order, with its firmware id */
    struct Found { int id; int type; };
    std::vector<Found> found;
    for (int socket = 1; socket <= 2; socket++) {
      int n = dev->USBSID_GetSocketNumSIDS(socket, board.socketconfig);
      if (n <= 0) continue;
      const uint8_t ids = cfg[(socket == 1) ? 8 : 9];
      found.push_back({ids & 0xF, dev->USBSID_GetSocketSIDType1(socket, board.socketconfig)});
      if (n == 2) {
        found.push_back({(ids >> 4) & 0xF,
                         dev->USBSID_GetSocketSIDType2(socket, board.socketconfig)});
      }
    }

    /* Ids are 0..3 and, unless mirrored, each one used once. Anything else
     * (a firmware that does not fill bytes 8/9 sends all zeroes) falls back
     * to socket order, which is what such a firmware routes by as well. */
    bool ids_valid = true;
    uint8_t seen = 0;
    for (const Found &f : found) {
      if (f.id > 3 || (!mirrored && (seen & (1u << f.id)) != 0)) { ids_valid = false; break; }
      seen |= (uint8_t)(1u << f.id);
    }
    if (!ids_valid) {
      for (size_t n = 0; n < found.size(); n++) {
        logical_map_.push_back({(int)b, (int)n, found[n].type});
      }
      continue;
    }

    /* In id order. A mirrored id (two sockets on one address) is one slot:
     * a write to it reaches both, the first socket's type describes it. */
    std::stable_sort(found.begin(), found.end(),
                     [](const Found &x, const Found &y) { return x.id < y.id; });
    int last_id = -1;
    for (const Found &f : found) {
      if (f.id == last_id) continue;
      logical_map_.push_back({(int)b, f.id, f.type});
      last_id = f.id;
    }
  }
}

void USBSID_Manager::CloseAll(void)
{
  devices_.clear();  /* ~USBSID_Class() closes each board's connection */
  boards_.clear();
  logical_map_.clear();
}

void USBSID_Manager::StartAll(void)
{
  for (auto &dev : devices_) {
    dev->USBSID_Reset();
  }
}


/* WRAPPERS */

void USBSID_Manager::WriteRing(int logical_sid, uint8_t reg, uint8_t val)
{
  if (logical_sid < 0 || (size_t)logical_sid >= logical_map_.size()) return;
  devices_[logical_map_[logical_sid].board_index]->USBSID_WriteRing(reg, val);
}

void USBSID_Manager::WriteRingCycled(int logical_sid, uint8_t reg, uint8_t val, uint16_t cycles)
{
  if (logical_sid < 0 || (size_t)logical_sid >= logical_map_.size()) return;
  devices_[logical_map_[logical_sid].board_index]->USBSID_WriteRingCycled(reg, val, cycles);
}

unsigned char USBSID_Manager::Read(int logical_sid, uint8_t reg)
{
  if (logical_sid < 0 || (size_t)logical_sid >= logical_map_.size()) return 0;
  return devices_[logical_map_[logical_sid].board_index]->USBSID_Read(reg);
}

void USBSID_Manager::WriteRingCycledN(int logical_sid, const uint8_t *items, int count)
{
  if (logical_sid < 0 || (size_t)logical_sid >= logical_map_.size()) return;
  devices_[logical_map_[logical_sid].board_index]->USBSID_WriteRingCycledN(items, count);
}

int USBSID_Manager::RingFreeBytes(int logical_sid)
{
  if (logical_sid < 0 || (size_t)logical_sid >= logical_map_.size()) return 0;
  return devices_[logical_map_[logical_sid].board_index]->USBSID_RingFree();
}

void USBSID_Manager::FlushBoard(int logical_sid)
{
  if (logical_sid < 0 || (size_t)logical_sid >= logical_map_.size()) return;
  devices_[logical_map_[logical_sid].board_index]->USBSID_SetFlush();
}

void USBSID_Manager::FlushAll(void)
{
  for (auto &dev : devices_) dev->USBSID_SetFlush();
}

void USBSID_Manager::ResetRingBufferAll(void)
{
  for (auto &dev : devices_) dev->USBSID_ResetRingBuffer();
}

void USBSID_Manager::ResetAllRegistersAll(void)
{
  for (auto &dev : devices_) dev->USBSID_ResetAllRegisters();
}

void USBSID_Manager::UnMuteAll(void)
{
  for (auto &dev : devices_) dev->USBSID_UnMute();
}

void USBSID_Manager::MuteAll(void)
{
  for (auto &dev : devices_) dev->USBSID_Mute();
}

void USBSID_Manager::SetMutedAll(bool muted)
{
  for (auto &dev : devices_) dev->USBSID_SetMuted(muted);
}

void USBSID_Manager::SetClockRateAll(long clockrate_cycles, bool suspend_sids)
{
  for (auto &dev : devices_) dev->USBSID_SetClockRate(clockrate_cycles, suspend_sids);
}
