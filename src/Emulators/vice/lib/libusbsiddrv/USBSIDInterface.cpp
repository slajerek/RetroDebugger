/*
 * USBSID-Pico is a RPi Pico (RP2040/RP2350) based board for interfacing one
 * or two MOS SID chips and/or hardware SID emulators over (WEB)USB with your
 * computer, phone or ASID supporting player.
 *
 * USBSIDInterface.cpp
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

#include <new>
#include "USBSIDInterface.h"
#include "USBSID.h"

using namespace USBSID_NS;

/* Catch C++ exceptions in C where a failed allocation aborts instead
 * if compiled out with -fno-exceptions */
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
  #define US_TRY        try
  #define US_CATCH_ALL  catch (...)
#else
  #define US_TRY        if (true)
  #define US_CATCH_ALL  else
#endif

static_assert(USBSID_SOCKETCONFIG_LEN == SOCKET_BUFFER_SIZE,
  "USBSID_SOCKETCONFIG_LEN must match SOCKET_BUFFER_SIZE");

/**
 * @brief: Copy src into a NULL terminated C buffer, truncating if needed.
 *
 * @param dst: destination buffer
 * @param len: size of dst, must be > 0
 * @param src: source string
 */
static void copy_cstr(char *dst, size_t len, const std::string &src)
{
  size_t n = (src.size() < len - 1) ? src.size() : len - 1;
  memcpy(dst, src.data(), n);
  dst[n] = '\0';
}

extern "C"
{
  int enumerate_USBSID(USBSIDdevinfo *out, int max){
    if( out == NULL && max > 0 ) return -1;
    US_TRY {
      std::vector<USBSID_DeviceInfo> found = USBSID_EnumerateDevices();
      for( int i = 0; i < max && (size_t)i < found.size(); i++ ){
        const USBSID_DeviceInfo &d = found[i];
        copy_cstr(out[i].serial, USBSID_SERIAL_LEN, d.serial);
        out[i].bus = d.bus;
        size_t n = d.port_path.size() < USBSID_PORTPATH_LEN
          ? d.port_path.size() : USBSID_PORTPATH_LEN;
        out[i].port_path_len = (uint8_t)n;
        memset(out[i].port_path, 0, USBSID_PORTPATH_LEN);
        if( n > 0 ) memcpy(out[i].port_path, d.port_path.data(), n);
      }
      return (int)found.size();
    } US_CATCH_ALL {
      return -1;
    }
  };
  USBSIDitf create_USBSID(void){
    return (USBSID_Class*) new (std::nothrow) USBSID_Class();
  };
  int init_USBSID(USBSIDitf p, bool start_threaded, bool with_cycles){
    if( p == NULL ) return -1;
    US_TRY {
      return ((USBSID_Class*) p)->USBSID_Init(start_threaded, with_cycles);
    } US_CATCH_ALL {
      return -1;
    }
  };
  void close_USBSID(USBSIDitf p){
    if( p == NULL ) return;
    delete (USBSID_Class*) p;
  };
  void pause_USBSID(USBSIDitf p){
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_Pause();
  };
  void reset_USBSID(USBSIDitf p){
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_Reset();
  };
  void resetallregisters_USBSID(USBSIDitf p){
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_ResetAllRegisters();
  };
  void clearbus_USBSID(USBSIDitf p){
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_ClearBus();
  };
  void mute_USBSID(USBSIDitf p){
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_Mute();
  };
  void unmute_USBSID(USBSIDitf p){
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_UnMute();
  };
  void setmuted_USBSID(USBSIDitf p, bool muted){
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_SetMuted(muted);
  };
  void setclockrate_USBSID(USBSIDitf p, long clockrate_cycles, bool suspend_sids){
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_SetClockRate(clockrate_cycles, suspend_sids);
  };
  long getclockrate_USBSID(USBSIDitf p){
    if( p == NULL ) return 0;
    return ((USBSID_Class*)p)->USBSID_GetClockRate();
  };
  long getrefreshrate_USBSID(USBSIDitf p){
    /* Callable without connection */
    if( p == NULL ) return 0;
    return ((USBSID_Class*)p)->USBSID_GetRefreshRate();
  };
  long getrasterrate_USBSID(USBSIDitf p){
    /* Callable without connection */
    if( p == NULL ) return 0;
    return ((USBSID_Class*)p)->USBSID_GetRasterRate();
  };
  int getnumsids_USBSID(USBSIDitf p){
    if( p == NULL ) return -1;
    return ((USBSID_Class*)p)->USBSID_GetNumSIDs();
  }
  int getfmoplsid_USBSID(USBSIDitf p) {
    if( p == NULL ) return -1;
    return ((USBSID_Class*)p)->USBSID_GetFMOplSID();
  };
  int getpcbversion_USBSID(USBSIDitf p) {
    if( p == NULL ) return -1;
    return ((USBSID_Class*)p)->USBSID_GetPCBVersion();
  };
  void setstereo_USBSID(USBSIDitf p, int state) {
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_SetStereo(state);
  };
  void togglestereo_USBSID(USBSIDitf p) {
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_ToggleStereo();
  };
  void enablesid_USBSID(USBSIDitf p) {
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_EnableSID();
  };
  void disablesid_USBSID(USBSIDitf p) {
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_DisableSID();
  };
  void settargetserial_USBSID(USBSIDitf p, const char *serial) {
    if( p == NULL ) return;
    US_TRY {
      ((USBSID_Class*)p)->USBSID_SetTargetSerial(serial ? serial : "");
    } US_CATCH_ALL {
      return;
    }
  };
  void settargetindex_USBSID(USBSIDitf p, int idx) {
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_SetTargetIndex(idx);
  };
  int getserial_USBSID(USBSIDitf p, char *buff, size_t len) {
    if( p == NULL || buff == NULL || len == 0 ) return -1;
    const std::string &s = ((USBSID_Class*)p)->USBSID_GetSerial();
    copy_cstr(buff, len, s);
    return (int)s.size();
  };
  bool getsocketconfig_USBSID(USBSIDitf p, uint8_t *cfg) {
    if( p == NULL || cfg == NULL ) return false;
    return ((USBSID_Class*)p)->USBSID_GetSocketConfig(cfg) != NULL;
  };
  int getsocketnumsids_USBSID(USBSIDitf p, int socket, uint8_t *cfg) {
    if( p == NULL || cfg == NULL ) return 0;
    return ((USBSID_Class*)p)->USBSID_GetSocketNumSIDS(socket, cfg);
  };
  int getsocketchiptype_USBSID(USBSIDitf p, int socket, uint8_t *cfg) {
    if( p == NULL || cfg == NULL ) return 0;
    return ((USBSID_Class*)p)->USBSID_GetSocketChipType(socket, cfg);
  };
  int getsocketsidtype1_USBSID(USBSIDitf p, int socket, uint8_t *cfg) {
    if( p == NULL || cfg == NULL ) return 0;
    return ((USBSID_Class*)p)->USBSID_GetSocketSIDType1(socket, cfg);
  };
  int getsocketsidtype2_USBSID(USBSIDitf p, int socket, uint8_t *cfg) {
    if( p == NULL || cfg == NULL ) return 0;
    return ((USBSID_Class*)p)->USBSID_GetSocketSIDType2(socket, cfg);
  };
  bool initialised_USBSID(USBSIDitf p) {
    /* Callable without connection */
    if( p == NULL ) return false;
    return ((USBSID_Class*)p)->USBSID_isInitialised();
  }
  bool available_USBSID(USBSIDitf p) {
    /* Callable without connection */
    if( p == NULL ) return false;
    return ((USBSID_Class*)p)->USBSID_isAvailable();
  }
  bool portisopen_USBSID(USBSIDitf p) {
    /* Callable without connection */
    if( p == NULL ) return false;
    return ((USBSID_Class*)p)->USBSID_isOpen();
  }
  // int found_USBSID(USBSIDitf p) {
  //   if( p == NULL ) return -1;
  //   return ((USBSID_Class*)p)->us_Found;
  // }
  void writesingle_USBSID(USBSIDitf p, unsigned char *buff, size_t len){
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_SingleWrite(buff, len);
  };
  unsigned char readsingle_USBSID(USBSIDitf p, uint8_t reg){
    if( p == NULL ) return 0;
    return ((USBSID_Class*)p)->USBSID_SingleRead(reg);
  };
  void writebuffer_USBSID(USBSIDitf p, unsigned char *buff, size_t len){
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_Write(buff, len);
  };
  void write_USBSID(USBSIDitf p, uint8_t reg, uint8_t val){
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_Write(reg, val);
  };
  void writecycled_USBSID(USBSIDitf p, uint8_t reg, uint8_t val, uint16_t cycles){
    if( p == NULL ) return;
    return ((USBSID_Class*)p)->USBSID_WriteCycled(reg, val, cycles);
  };
  unsigned char read_USBSID(USBSIDitf p, uint8_t reg){
    if( p == NULL ) return 0;
    return ((USBSID_Class*)p)->USBSID_Read(reg);
  };
  void writering_USBSID(USBSIDitf p, uint8_t reg, uint8_t val){
    if( p == NULL ) return;
    return ((USBSID_Class*) p)->USBSID_WriteRing(reg, val);
  };
  void writeringcycled_USBSID(USBSIDitf p, uint8_t reg, uint8_t val, uint16_t cycles){
    if( p == NULL ) return;
    return ((USBSID_Class*) p)->USBSID_WriteRingCycled(reg, val, cycles);
  };
  void writeringcycledn_USBSID(USBSIDitf p, const uint8_t *items, int count){
    if( p == NULL ) return;
    return ((USBSID_Class*) p)->USBSID_WriteRingCycledN(items, count);
  };
  void enablethread_USBSID(USBSIDitf p){
    if( p == NULL ) return;
    return ((USBSID_Class*) p)->USBSID_EnableThread();
  };
  void disablethread_USBSID(USBSIDitf p){
    if( p == NULL ) return;
    return ((USBSID_Class*) p)->USBSID_DisableThread();
  };
  void setflush_USBSID(USBSIDitf p){
    if( p == NULL ) return;
    return ((USBSID_Class*) p)->USBSID_SetFlush();
  };
  void flush_USBSID(USBSIDitf p){
    if( p == NULL ) return;
    return ((USBSID_Class*) p)->USBSID_Flush();
  };
  void resetringbuffer_USBSID(USBSIDitf p){
    if( p == NULL ) return;
    return ((USBSID_Class*) p)->USBSID_ResetRingBuffer();
  }
  void restartringbuffer_USBSID(USBSIDitf p){
    if( p == NULL ) return;
    return ((USBSID_Class*) p)->USBSID_RestartRingBuffer();
  }
  int ringfree_USBSID(USBSIDitf p){
    if( p == NULL ) return 0;
    return ((USBSID_Class*) p)->USBSID_RingFree();
  };
  void setbuffsize_USBSID(USBSIDitf p, int size){
    if( p == NULL ) return;
    return ((USBSID_Class*) p)->USBSID_SetBufferSize(size);
  }
  void setdiffsize_USBSID(USBSIDitf p, int size){
    if( p == NULL ) return;
    return ((USBSID_Class*) p)->USBSID_SetDiffSize(size);
  }
  void restartthread_USBSID(USBSIDitf p, bool with_cycles){
    if( p == NULL ) return;
    return ((USBSID_Class*) p)->USBSID_RestartThread(with_cycles);
  }
  int_fast64_t waitforcycle_USBSID(USBSIDitf p, uint_fast64_t cycles){
    if( p == NULL ) return 0;
    /* Split into chunks, driver takes at most 16 bit cycles per call */
    uint_fast64_t waited = 0;
    while( cycles > 0 ){
      uint_fast16_t chunk = (uint_fast16_t)(cycles > 0xFFFF ? 0xFFFF : cycles);
      waited += ((USBSID_Class*) p)->USBSID_WaitForCycle(chunk);
      cycles -= chunk;
    }
    return (int_fast64_t)waited;
  };
}
