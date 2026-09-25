/*
 * USBSID-Pico is a RPi Pico (RP2040/RP2350) based board for interfacing one
 * or two MOS SID chips and/or hardware SID emulators over (WEB)USB with your
 * computer, phone or ASID supporting player.
 *
 * USBSIDManagerInterface.cpp
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

#include <cstring>
#include <new>
#include <string>
#include <vector>
#include "USBSIDManagerInterface.h"
#include "USBSID_Manager.h"

/* Catch C++ exceptions in C where a failed allocation aborts instead
 * if compiled out with -fno-exceptions */
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
  #define US_TRY        try
  #define US_CATCH_ALL  catch (...)
#else
  #define US_TRY        if (true)
  #define US_CATCH_ALL  else
#endif

static_assert(sizeof(((USBSID_Manager::BoardInfo *)0)->socketconfig) == USBSID_SOCKETCONFIG_LEN,
  "USBSID_SOCKETCONFIG_LEN must match BoardInfo::socketconfig");

extern "C"
{
  USBSIDmgr create_USBSIDMgr(void){
    return (USBSID_Manager*) new (std::nothrow) USBSID_Manager();
  };
  void close_USBSIDMgr(USBSIDmgr m){
    if( m == NULL ) return;
    delete (USBSID_Manager*) m;
  };
  bool openall_USBSIDMgr(USBSIDmgr m, const char *const *serials, size_t count,
                         bool start_threaded, bool with_cycles){
    if( m == NULL ) return false;
    if( serials == NULL && count > 0 ) return false;
    US_TRY {
      std::vector<std::string> list;
      for( size_t i = 0; i < count; i++ ){
        list.emplace_back(serials[i] ? serials[i] : "");
      }
      return ((USBSID_Manager*) m)->OpenAll(list, start_threaded, with_cycles);
    } US_CATCH_ALL {
      return false;
    }
  };
  void closeall_USBSIDMgr(USBSIDmgr m){
    if( m == NULL ) return;
    return ((USBSID_Manager*) m)->CloseAll();
  };
  void startall_USBSIDMgr(USBSIDmgr m){
    if( m == NULL ) return;
    return ((USBSID_Manager*) m)->StartAll();
  };
  int totalsids_USBSIDMgr(USBSIDmgr m){
    if( m == NULL ) return 0;
    return ((USBSID_Manager*) m)->TotalSIDs();
  };
  int boardcount_USBSIDMgr(USBSIDmgr m){
    if( m == NULL ) return 0;
    return ((USBSID_Manager*) m)->BoardCount();
  };
  bool getboardinfo_USBSIDMgr(USBSIDmgr m, int board, USBSIDboardinfo *out){
    if( m == NULL || out == NULL ) return false;
    const std::vector<USBSID_Manager::BoardInfo> &boards = ((USBSID_Manager*) m)->Boards();
    if( board < 0 || (size_t)board >= boards.size() ) return false;
    const USBSID_Manager::BoardInfo &b = boards[board];
    size_t n = (b.serial.size() < USBSID_SERIAL_LEN - 1)
      ? b.serial.size() : USBSID_SERIAL_LEN - 1;
    memcpy(out->serial, b.serial.data(), n);
    out->serial[n] = '\0';
    out->index = b.index;
    out->numsids = b.numsids;
    out->fmoplsid = b.fmoplsid;
    out->pcbversion = b.pcbversion;
    out->socketconfig_valid = b.socketconfig_valid;
    memcpy(out->socketconfig, b.socketconfig, USBSID_SOCKETCONFIG_LEN);
    return true;
  };
  bool getlogicalslot_USBSIDMgr(USBSIDmgr m, int logical_sid, USBSIDlogicalslot *out){
    if( m == NULL || out == NULL ) return false;
    const std::vector<USBSID_Manager::LogicalSlot> &map = ((USBSID_Manager*) m)->LogicalMap();
    if( logical_sid < 0 || (size_t)logical_sid >= map.size() ) return false;
    out->board_index = map[logical_sid].board_index;
    out->local_slot = map[logical_sid].local_slot;
    out->sid_type = map[logical_sid].sid_type;
    return true;
  };
  void writering_USBSIDMgr(USBSIDmgr m, int logical_sid, uint8_t reg, uint8_t val){
    if( m == NULL ) return;
    return ((USBSID_Manager*) m)->WriteRing(logical_sid, reg, val);
  };
  void writeringcycled_USBSIDMgr(USBSIDmgr m, int logical_sid, uint8_t reg, uint8_t val, uint16_t cycles){
    if( m == NULL ) return;
    return ((USBSID_Manager*) m)->WriteRingCycled(logical_sid, reg, val, cycles);
  };
  unsigned char read_USBSIDMgr(USBSIDmgr m, int logical_sid, uint8_t reg){
    if( m == NULL ) return 0;
    return ((USBSID_Manager*) m)->Read(logical_sid, reg);
  };
  void writeringcycledn_USBSIDMgr(USBSIDmgr m, int logical_sid, const uint8_t *items, int count){
    if( m == NULL ) return;
    return ((USBSID_Manager*) m)->WriteRingCycledN(logical_sid, items, count);
  };
  int ringfreebytes_USBSIDMgr(USBSIDmgr m, int logical_sid){
    if( m == NULL ) return 0;
    return ((USBSID_Manager*) m)->RingFreeBytes(logical_sid);
  };
  void flushboard_USBSIDMgr(USBSIDmgr m, int logical_sid){
    if( m == NULL ) return;
    return ((USBSID_Manager*) m)->FlushBoard(logical_sid);
  };
  void flushall_USBSIDMgr(USBSIDmgr m){
    if( m == NULL ) return;
    return ((USBSID_Manager*) m)->FlushAll();
  };
  void resetringbufferall_USBSIDMgr(USBSIDmgr m){
    if( m == NULL ) return;
    return ((USBSID_Manager*) m)->ResetRingBufferAll();
  };
  void resetallregistersall_USBSIDMgr(USBSIDmgr m){
    if( m == NULL ) return;
    return ((USBSID_Manager*) m)->ResetAllRegistersAll();
  };
  void unmuteall_USBSIDMgr(USBSIDmgr m){
    if( m == NULL ) return;
    return ((USBSID_Manager*) m)->UnMuteAll();
  };
  void muteall_USBSIDMgr(USBSIDmgr m){
    if( m == NULL ) return;
    return ((USBSID_Manager*) m)->MuteAll();
  };
  void setmutedall_USBSIDMgr(USBSIDmgr m, bool muted){
    if( m == NULL ) return;
    return ((USBSID_Manager*) m)->SetMutedAll(muted);
  };
  void setclockrateall_USBSIDMgr(USBSIDmgr m, long clockrate_cycles, bool suspend_sids){
    if( m == NULL ) return;
    return ((USBSID_Manager*) m)->SetClockRateAll(clockrate_cycles, suspend_sids);
  };
}
