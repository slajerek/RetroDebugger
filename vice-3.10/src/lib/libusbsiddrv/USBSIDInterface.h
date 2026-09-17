/*
 * USBSID-Pico is a RPi Pico (RP2040/RP2350) based board for interfacing one
 * or two MOS SID chips and/or hardware SID emulators over (WEB)USB with your
 * computer, phone or ASID supporting player.
 *
 * USBSIDInterface.h
 * This file is part of USBSID-Pico (https://github.com/LouDnl/USBSID-Pico-driver)
 * File author: LouD
 *
 * Copyright (c) 2024-2025 LouD
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
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

  /* USBSID Interface for use in STD C applications */
  typedef void * USBSIDitf;
  USBSIDitf create_USBSID(void);
  int init_USBSID(USBSIDitf, bool start_threaded, bool with_cycles);
  void close_USBSID(USBSIDitf);
  void pause_USBSID(USBSIDitf);
  void reset_USBSID(USBSIDitf);
  void resetallregisters_USBSID(USBSIDitf);
  void clearbus_USBSID(USBSIDitf);
  void mute_USBSID(USBSIDitf);
  void unmute_USBSID(USBSIDitf);
  void setclockrate_USBSID(USBSIDitf, long clockrate_cycles, bool suspend_sids);
  long getclockrate_USBSID(USBSIDitf);
  long getrefreshrate_USBSID(USBSIDitf);
  long getrasterrate_USBSID(USBSIDitf);
  int getnumsids_USBSID(USBSIDitf);
  int getfmoplsid_USBSID(USBSIDitf);
  int getpcbversion_USBSID(USBSIDitf);
  void setstereo_USBSID(USBSIDitf, int state);
  void togglestereo_USBSID(USBSIDitf);

  /* Helpers */
  bool initialised_USBSID(USBSIDitf);
  bool available_USBSID(USBSIDitf);
  bool portisopen_USBSID(USBSIDitf);
  int found_USBSID(USBSIDitf);

  /* Synchronous direct */
  void writesingle_USBSID(USBSIDitf, unsigned char *buff, int len);
  unsigned char readsingle_USBSID(USBSIDitf, uint8_t reg);

  /* Asynchronous direct */
  void writebuffer_USBSID(USBSIDitf, unsigned char *buff, int len);
  void write_USBSID(USBSIDitf, uint8_t reg, uint8_t val);
  void writecycled_USBSID(USBSIDitf, uint8_t reg, uint8_t val, uint16_t cycles);
  unsigned char read_USBSID(USBSIDitf p,  uint8_t reg);

  /* Asynchronous thread */
  void writering_USBSID(USBSIDitf, uint8_t reg, uint8_t val);
  void writeringcycled_USBSID(USBSIDitf, uint8_t reg, uint8_t val, uint16_t cycles);

  /* Thread buffer */
  void enablethread_USBSID(USBSIDitf);
  void disablethread_USBSID(USBSIDitf);
  void setflush_USBSID(USBSIDitf);
  void flush_USBSID(USBSIDitf);
  void restartringbuffer_USBSID(USBSIDitf);
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
