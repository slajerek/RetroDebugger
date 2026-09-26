/*
 * USBSID-Pico is a RPi Pico (RP2040/RP2350) based board for interfacing one
 * or two MOS SID chips and/or hardware SID emulators over (WEB)USB with your
 * computer, phone or ASID supporting player.
 *
 * USBSID.cpp
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

#include <libusb.h>
#include <time.h>
#include <algorithm>
#include "USBSID.h"


using namespace USBSID_NS;
using namespace std;


static inline uint8_t* us_alloc(size_t alignment, size_t size)
{
#if defined(__US_LINUX_COMPILE)
  #ifdef HAVE_ALIGNED_ALLOC
    return (uint8_t*)aligned_alloc(alignment, size);
  #else
    (void)alignment;
    return (uint8_t*)malloc(size);
  #endif
#elif defined(__US_WINDOWS_COMPILE)
  return (uint8_t*)_aligned_malloc(size, alignment);
#else
  (void)alignment;
  return (uint8_t*)malloc(size);
#endif
}

static inline void us_free(void* m)
{
#ifdef __US_WINDOWS_COMPILE
  return _aligned_free(m);
#else
  return free(m);
#endif
}

/**
 * @brief: Read the USB port path of a device.
 *
 * @param dev: libusb device
 * @return: port numbers from root hub to device, empty on failure
 */
static std::vector<uint8_t> us_port_path(libusb_device *dev)
{
  std::vector<uint8_t> path;
  uint8_t p[8];
  int n = libusb_get_port_numbers(dev, p, sizeof(p));
  if (n > 0) path.assign(p, p + n);
  return path;
}

/**
 * @brief: Order devices by bus, then port path.
 *
 * @return: true if a sorts before b
 */
static bool us_port_less(uint8_t bus_a, const std::vector<uint8_t> &path_a,
                         uint8_t bus_b, const std::vector<uint8_t> &path_b)
{
  if (bus_a != bus_b) return bus_a < bus_b;
  return path_a < path_b;
}


extern "C" {

/* USBSID */

USBSID_Class::USBSID_Class() :
  us_InstanceID(0)//,
{
  USBDBG(stdout, "[USBSID] Driver init start\n");
  us_InstanceID = ++instance_counter;
  us_CPUcycleDuration = ratio_t::den / (float)cycles_per_sec;
  us_InvCPUcycleDurationNanoSeconds = 1.0 / (ratio_t::den / (float)cycles_per_sec);
  m_StartTime = std::chrono::steady_clock::now();
  m_LastTime = m_StartTime;
  us_Initialised = true;
}

USBSID_Class::~USBSID_Class()
{
  USBDBG(stdout, "[USBSID] Driver de-init start\n");
  if (us_PortIsOpen) {
    if (USBSID_Close() == 0) {
      us_Initialised = false;
    }
  }
  USBSID_DeInitRingBuffer(); /* no-op after Close(), frees a ring left by a failed thread start */
  if (write_buffer) us_free(write_buffer);
  if (thread_buffer) us_free(thread_buffer);
  if (result) us_free(result);
  thread_buffer = NULL;
  write_buffer = NULL;
  result = NULL;
}

int USBSID_Class::USBSID_Init(bool start_threaded, bool with_cycles)
{
  if (!us_Initialised) return -1;
  if (!us_PortIsOpen) {
    USBDBG(stdout, "[USBSID] Setup start\n");
    /* Init USB */
    rc = LIBUSB_Setup(start_threaded, with_cycles);
    flush_buffer = 0;
    if (rc >= 0) {
      /* Start thread on init */
      if (threaded) {
        rc = USBSID_InitThread();
      }
      us_PortIsOpen = true;
      USBSID_Mute();
      USBSID_ClearBus();
      USBSID_UnMute();
      USBSID_GetClockRate();  /* Once on init */
      return rc;
    } else {
      USBDBG(stdout, "[USBSID] Not found\n");
      return -1;
    }
  } else {
    USBDBG(stdout, "[USBSID] Driver already started\n");
    return 0;
  }
}

int USBSID_Class::USBSID_Close(void)
{
  if (!us_PortIsOpen) return 0;
  int e = -1;
  if (rc >= 0) e = LIBUSB_Exit();
  if (rc != -1) USBERR(stderr, "Expected rc == -1, received: %d\n", rc);
  if (e != 0) USBERR(stderr, "Expected e == 0, received: %d\n", e);
  if (devh != NULL) USBERR(stderr, "Expected dev == NULL, received: %p\n", (void*)&devh);
  us_PortIsOpen = false;
  us_Initialised = false;
  USBDBG(stdout, "[USBSID] De-init finished\n");
  return 0;
}

void USBSID_Class::USBSID_Pause(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] Pause\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | PAUSE), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_Reset(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] Reset\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | RESET_SID), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  flush_buffer = 1;
  USBSID_SyncTime();
  return;
}

void USBSID_Class::USBSID_ResetAllRegisters(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] Reset All Registers\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | RESET_SID), 0x1, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_Mute(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] Mute\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | MUTE), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_UnMute(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] UnMute\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | UNMUTE), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_SetMuted(bool muted)
{ /* Argument 1 in the MUTE/UNMUTE command sets the firmware's muted state, which
   * masks every later volume register write until unmuted.
   * USBSID_Mute() sends
   * 0 and only zeroes the volume once, the next volume write is audible again. */
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] SetMuted %d\r\n", muted);
  unsigned char buff[3] = {(unsigned char)(COMMAND << 6 | (muted ? MUTE : UNMUTE)), 0x1, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_DisableSID(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] DisableSID\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | DISABLE_SID), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_EnableSID(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] EnableSID\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | ENABLE_SID), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_ClearBus(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] ClearBus\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | CLEAR_BUS), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_SetClockRate(long clockrate_cycles, bool suspend_sids)
{
  if (!us_PortIsOpen) return;
  for (uint8_t i = 0; i < (sizeof(clockSpeed) / sizeof(clockSpeed[0])); i++) {
    if (clockSpeed[i] == clockrate_cycles) {
      cycles_per_sec = clockSpeed[i];
      cycles_per_frame = refreshRate[i];
      cycles_per_raster = rasterRate[i];
      us_CPUcycleDuration = ratio_t::den / (float)cycles_per_sec;
      us_InvCPUcycleDurationNanoSeconds = 1.0 / (ratio_t::den / (float)cycles_per_sec);
      USBDBG(stdout, "[USBSID] Clockspeed set to: %ld\n", cycles_per_sec);
      USBDBG(stdout, "[USBSID] Cycles per raster %ld\n", cycles_per_raster);
      USBDBG(stdout, "[USBSID] Cycles per frame: %ld\n", cycles_per_frame);
      USBDBG(stdout, "[USBSID] CPU cycle duration in nanoseconds %f\n", us_CPUcycleDuration);
      USBDBG(stdout, "[USBSID] Inverted CPU cycle duration in nanoseconds %.09f\n",
        us_InvCPUcycleDurationNanoSeconds);
      if (clk_retrieved == 0 || us_clkrate != cycles_per_sec) {
        uint8_t configbuff[6] = {
          (COMMAND << 6 | CONFIG),
          0x50,
          i,
          (uint8_t)(suspend_sids == true ? 1 : 0),
          0, 0};
        USBSID_SingleWrite(configbuff, 6);
      }
      USBSID_SyncTime();
      return;
    }
  }
  return;
}

long USBSID_Class::USBSID_GetClockRate(void)
{
  if (!us_PortIsOpen) return 0;
  if (clk_retrieved == 1) {
    us_InvCPUcycleDurationNanoSeconds = 1.0 / (ratio_t::den / (float)cycles_per_sec);
    us_CPUcycleDuration = ratio_t::den / (float)cycles_per_sec;
    return cycles_per_sec;
  } else if (clk_retrieved == 0) {
    USBSID_SyncTime();
    uint8_t configbuff[6] = {(COMMAND << 6 | CONFIG), 0x57, 0, 0, 0, 0};
    USBSID_SingleWrite(configbuff, 6);
    us_clkrate = clockSpeed[USBSID_SingleReadConfig(result, 1)];
    cycles_per_sec = us_clkrate;
    clk_retrieved = 1;
    us_InvCPUcycleDurationNanoSeconds = 1.0 / (ratio_t::den / (float)cycles_per_sec);
    us_CPUcycleDuration = ratio_t::den / (float)cycles_per_sec;
    return cycles_per_sec;
  }
  return -1;
}

long USBSID_Class::USBSID_GetRefreshRate(void)
{
  return cycles_per_frame;
}

long USBSID_Class::USBSID_GetRasterRate(void)
{
  return cycles_per_raster;
}

uint8_t* USBSID_Class::USBSID_GetSocketConfig(uint8_t socket_config[])
{
  if (!us_PortIsOpen) return NULL;
  if (socketconfig == -1) {
    socketconfig = 1;
    uint8_t configbuff[6] = {(COMMAND << 6 | CONFIG), 0x37, 0, 0, 0, 0};
    USBSID_SingleWrite(configbuff, 6);
    uint8_t socket_buff[SOCKET_BUFFER_SIZE];
    int ret = USBSID_ReadConfig(socket_buff, SOCKET_BUFFER_SIZE);
    if (ret != SOCKET_BUFFER_SIZE) {
      socketconfig = -1;
      return NULL;
    } else {
      if (socket_buff[0] == 0x37
        && socket_buff[1] == 0x7F
        && socket_buff[SOCKET_BUFFER_SIZE - 1] == 0xFF) {
        memcpy(socket_config, socket_buff, SOCKET_BUFFER_SIZE);
        return socket_config;
      } else {
        socketconfig = -1;
        return NULL;
      }
    }
  } else {
    socketconfig = (socketconfig == 1 ? socketconfig : -1);
    return NULL;
  }
}

int USBSID_Class::USBSID_GetSocketNumSIDS(int socket, uint8_t socket_config[])
{
  if (!us_PortIsOpen) return 0;
  switch (socket) {
    case 1:
      if (((socket_config[2] & 0xF0) >> 4) == 1) {
        return ((socket_config[2] & 0xF) == 1 ? 2 : 1);
      } else {
        return 0;
      }
      break;
    case 2:
      if (((socket_config[5] & 0xF0) >> 4) == 1) {
        return ((socket_config[5] & 0xF) == 1 ? 2 : 1);
      } else {
        return 0;
      }
      break;
    default:
      return 0;
  }
};

int USBSID_Class::USBSID_GetSocketChipType(int socket, uint8_t socket_config[])
{
  if (!us_PortIsOpen) return 1; /* Unknown */
  switch (socket) {
    case 1:
      return (socket_config[3] & 0xF);
    case 2:
      return (socket_config[6] & 0xF);
      break;
    default:
      return 1; /* Unknown */
  }
  return 0;
};

/* 0 = unknown, 1 = N/A, 2 = MOS8085, 3 = MOS6581, 4 = FMopl */
int USBSID_Class::USBSID_GetSocketSIDType1(int socket, uint8_t socket_config[])
{
  if (!us_PortIsOpen) return 1; /* N/A */
  switch (socket) {
    case 1:
      // if (((socket_config[2] & 0xF0) >> 4) == 1) {
      if ((socket_config[2] & 0xF0) == 0x10) {
        return ((socket_config[4] & 0xF0) >> 4);
      } else {
        return 1; /* N/A */
      }
      break;
    case 2:
      // if (((socket_config[5] & 0xF0) >> 4) == 1) {
      if ((socket_config[5] & 0xF0) == 0x10) {
        return ((socket_config[7] & 0xF0) >> 4);
      } else {
        return 1; /* N/A */
      }
      break;
    default:
      return 1; /* N/A */
  }
};

/* 0 = unknown, 1 = N/A, 2 = MOS8085, 3 = MOS6581, 4 = FMopl */
int USBSID_Class::USBSID_GetSocketSIDType2(int socket, uint8_t socket_config[])
{
  if (!us_PortIsOpen) return 1; /* N/A */
  switch (socket) {
    case 1:
      // if ((((socket_config[2] & 0xF0) >> 4) == 1) && ((socket_config[2] & 0xF) == 1)) {
      if (socket_config[2] == 0x11) {
        return (socket_config[4] & 0xF);
      } else {
        return 1; /* N/A */
      }
      break;
    case 2:
      // if ((((socket_config[5] & 0xF0) >> 4) == 1) && ((socket_config[5] & 0xF) == 1)) {
      if (socket_config[5] == 0x11) {
        return (socket_config[7] & 0xF);
      } else {
        return 1; /* N/A */
      }
      break;
    default:
      return 1; /* N/A */
  }
};

int USBSID_Class::USBSID_GetNumSIDs(void)
{
  if (!us_PortIsOpen) return 0;
  if (numsids == 0) {
    uint8_t configbuff[6] = {(COMMAND << 6 | CONFIG), 0x39, 0, 0, 0, 0};
    USBSID_SingleWrite(configbuff, 6);
    numsids = USBSID_SingleReadConfig(result, 1);
    return numsids;
  } else {
    return numsids;
  }
  return 0;
}

int USBSID_Class::USBSID_GetFMOplSID(void)
{
  if (!us_PortIsOpen) return 0;
  if (fmoplsid == -1) {
    uint8_t configbuff[6] = {(COMMAND << 6 | CONFIG), 0x3A, 0, 0, 0, 0};
    USBSID_SingleWrite(configbuff, 6);
    fmoplsid = USBSID_SingleReadConfig(result, 1);
  }
  return (fmoplsid == 0 ? -1 : fmoplsid);
}

int USBSID_Class::USBSID_GetPCBVersion(void)
{
  if (!us_PortIsOpen) return 0;
  if (pcbversion == -1) {
    uint8_t configbuff[6] = {(COMMAND << 6 | CONFIG), 0x81, 0x1, 0, 0, 0};
    USBSID_SingleWrite(configbuff, 6);
    pcbversion = USBSID_SingleReadConfig(result, 1);
  }
  return pcbversion;
}

void USBSID_Class::USBSID_SetStereo(int state)
{
  if (!us_PortIsOpen) return;
  if (pcbversion == -1) {
    USBSID_GetPCBVersion();
  }
  if (pcbversion == 13) {
    uint8_t configbuff[6] = {(COMMAND << 6 | CONFIG), 0x89, (uint8_t)state, 0, 0, 0};
    USBSID_SingleWrite(configbuff, 6);
  }
  return;
}

void USBSID_Class::USBSID_ToggleStereo(void)
{
  if (!us_PortIsOpen) return;
  if (pcbversion == -1) {
    USBSID_GetPCBVersion();
  }
  if (pcbversion == 13) {
    uint8_t configbuff[6] = {(COMMAND << 6 | CONFIG), 0x88, 0x0, 0, 0, 0};
    USBSID_SingleWrite(configbuff, 6);
  }
  return;
}


/* SYNCHRONOUS */

void USBSID_Class::USBSID_SingleWrite(unsigned char *buff, size_t len)
{
  if (!us_PortIsOpen) return;
  int actual_length = 0;
  if (libusb_bulk_transfer(devh, EP_OUT_ADDR, buff, len, &actual_length, LIBUSB_TIMEOUT) < 0) {
    USBERR(stderr, "[USBSID] Error while sending synchronous write buffer of length %d\n",
      actual_length);
  }
  transfer_out_pending = false;
  return;
}

unsigned char USBSID_Class::USBSID_SingleRead(uint8_t reg)
{
  if (!us_PortIsOpen) return 0;
  int actual_length;
  unsigned char buff[3] = {(READ << 6), reg, 0};
  if (libusb_bulk_transfer(devh, EP_OUT_ADDR, buff, 3, &actual_length, LIBUSB_TIMEOUT) < 0) {
    USBERR(stderr, "[USBSID] Error while sending write command for reading\n");
  }
  rc = libusb_bulk_transfer(devh, EP_IN_ADDR, result, 1, &actual_length, LIBUSB_TIMEOUT);
  transfer_in_pending = false;
  if (rc == LIBUSB_ERROR_TIMEOUT) {
    USBERR(stderr, "[USBSID] Timeout error while reading (%d)\n", actual_length);
    return 0;
  } else if (rc < 0) {
    USBERR(stderr, "[USBSID] Error while waiting for char while reading: %d, %s: %s\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    return 0;
  }
  return result[0];
}

unsigned char USBSID_Class::USBSID_SingleReadConfig(unsigned char *buff, size_t len)
{
  if (!us_PortIsOpen) return 0;
  int actual_length;
  rc = libusb_bulk_transfer(devh, EP_IN_ADDR, buff, len, &actual_length, LIBUSB_TIMEOUT);
  transfer_in_pending = false;
  if (rc == LIBUSB_ERROR_TIMEOUT) {
    USBERR(stderr, "[USBSID] Timeout error while reading (%d)\n", actual_length);
    return 0;
  } else if (rc < 0) {
    USBERR(stderr, "[USBSID] Error while waiting for char while reading: %d, %s: %s\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    return 0;
  }
  return *buff;
}

int USBSID_Class::USBSID_ReadConfig(unsigned char *buff, size_t len)
{
  if (!us_PortIsOpen) return 0;
  int actual_length;
  rc = libusb_bulk_transfer(devh, EP_IN_ADDR, buff, len, &actual_length, LIBUSB_TIMEOUT);
  transfer_in_pending = false;
  if (rc == LIBUSB_ERROR_TIMEOUT) {
    USBERR(stderr, "[USBSID] Timeout error while reading (%d)\n", actual_length);
    return 0;
  } else if (rc < 0) {
    USBERR(stderr, "[USBSID] Error while waiting for char while reading: %d, %s: %s\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    return 0;
  }
  return actual_length;
}


/* ASYNCHRONOUS */

void USBSID_Class::USBSID_Write(unsigned char *buff, size_t len)
{
  if (!us_PortIsOpen) return;
  if (threaded) {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n",
      __func__, threaded);
    return;
  }
  write_completed = 0;
  memcpy(out_buffer, buff, len);
  libusb_submit_transfer(transfer_out);
  libusb_handle_events_completed(ctx, NULL);
  return;
}

void USBSID_Class::USBSID_Write(uint8_t reg, uint8_t val)
{
  if (!us_PortIsOpen) return;
  if (threaded) {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n",
      __func__, threaded);
    return;
  }
  write_completed = 0;
  write_buffer[0] = 0x0;
  write_buffer[1] = (reg & 0xFF);
  write_buffer[2] = val;
  USBSID_Write(write_buffer, 3);
  return;
}

void USBSID_Class::USBSID_Write(unsigned char *buff, size_t len, uint16_t cycles)
{
  if (!us_PortIsOpen) return;
  if (threaded) {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n",
      __func__, threaded);
    return;
  }
  USBSID_WaitForCycle(cycles);
  write_completed = 0;
  memcpy(out_buffer, buff, len);
  libusb_submit_transfer(transfer_out);
  libusb_handle_events_completed(ctx, NULL);
  return;
}

void USBSID_Class::USBSID_Write(uint8_t reg, uint8_t val, uint16_t cycles)
{
  if (!us_PortIsOpen) return;
  if (threaded) {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n",
      __func__, threaded);
    return;
  }
  USBSID_WaitForCycle(cycles);
  write_completed = 0;
  write_buffer[0] = 0x0;
  write_buffer[1] = (reg & 0xFF);
  write_buffer[2] = val;
  USBSID_Write(write_buffer, 3);
  return;
}

void USBSID_Class::USBSID_WriteCycled(uint8_t reg, uint8_t val, uint16_t cycles)
{
  if (!us_PortIsOpen) return;
  if (threaded) {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n",
      __func__, threaded);
    return;
  }
  write_completed = 0;
  write_buffer[0] = (CYCLED_WRITE << 6);
  write_buffer[1] = (reg & 0xFF);
  write_buffer[2] = val;
  write_buffer[3] = (uint8_t)(cycles >> 8);
  write_buffer[4] = (uint8_t)(cycles & 0xFF);
  USBSID_Write(write_buffer, 5);
  return;
}

unsigned char USBSID_Class::USBSID_Read(uint8_t reg)
{
  if (!us_PortIsOpen) return 0;
  if (threaded == 0) {  /* Reading not supported with threaded writes */
    read_completed = write_completed = 0;
    *result = 0;
    uint8_t rw_buff[2];
    rw_buff[0] = (READ << 6);
    rw_buff[1] = reg;

    /* prepare in buffer first */
    transfer_in_pending = true;
    rc = libusb_submit_transfer(transfer_in);
    if (rc < 0) {
      transfer_in_pending = false;
      USBERR(stderr, "[USBSID] submit IN failed %d - %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
      return 0xFF;
    }
    /* now out buffer */
    memcpy(out_buffer, rw_buff, 2);
    transfer_out_pending = true;
    rc = libusb_submit_transfer(transfer_out);
    if (rc < 0) {
      transfer_out_pending = false;
      libusb_cancel_transfer(transfer_in);
      USBERR(stderr, "[USBSID] submit OUT failed %d - %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
      return 0xFF;
    }
    while (!read_completed && transfer_in_pending) {
      struct timeval tv = {0, 1000};
      libusb_handle_events_timeout_completed(ctx, &tv, NULL);
    }

    return *result;
  } else {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n",
      __func__, threaded);
  }
  return 0xFF;
}

unsigned char USBSID_Class::USBSID_Read(unsigned char *writebuff)
{
  if (!us_PortIsOpen) return 0;
  if (threaded == 0) {  /* Reading not supported with threaded writes */
    read_completed = write_completed = 0;
    *result = 0;

    /* prepare in buffer first */
    transfer_in_pending = true;
    rc = libusb_submit_transfer(transfer_in);
    if (rc < 0) {
      transfer_in_pending = false;
      USBERR(stderr, "[USBSID] submit IN failed %d - %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
      return 0xFF;
    }
    /* now out buffer */
    writebuff[0] = (READ << 6);
    memcpy(out_buffer, writebuff, 3);
    transfer_out_pending = true;
    rc = libusb_submit_transfer(transfer_out);
    if (rc < 0) {
      transfer_out_pending = false;
      libusb_cancel_transfer(transfer_in);
      USBERR(stderr, "[USBSID] submit OUT failed %d - %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
      return 0xFF;
    }
    while (!read_completed && transfer_in_pending) {
      struct timeval tv = {0, 1000};
      libusb_handle_events_timeout_completed(ctx, &tv, NULL);
    }
    return *result;
  } else {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n",
      __func__, threaded);
  }
  return 0xFF;
}

unsigned char USBSID_Class::USBSID_Read(unsigned char *writebuff, uint16_t cycles)
{
  if (!us_PortIsOpen) return 0;
  if (threaded == 0) {  /* Reading not supported with threaded writes */
    USBSID_WaitForCycle(cycles);
    read_completed = 0;
    *result = 0;

    /* prepare in buffer first */
    transfer_in_pending = true;
    rc = libusb_submit_transfer(transfer_in);
    if (rc < 0) {
      transfer_in_pending = false;
      USBERR(stderr, "[USBSID] submit IN failed %d - %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
      return 0xFF;
    }
    /* now out buffer */
    writebuff[0] = (READ << 6);
    memcpy(out_buffer, writebuff, 3);
    transfer_out_pending = true;
    rc = libusb_submit_transfer(transfer_out);
    if (rc < 0) {
      transfer_out_pending = false;
      libusb_cancel_transfer(transfer_in);
      USBERR(stderr, "[USBSID] submit OUT failed %d - %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
      return 0xFF;
    }
    while (!read_completed && transfer_in_pending) {
      struct timeval tv = {0, 1000};
      libusb_handle_events_timeout_completed(ctx, &tv, NULL);
    }
    return *result;
  } else {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n",
      __func__, threaded);
  }
  return 0xFF;
}


/* THREADING */

void* USBSID_Class::USBSID_Thread(void)
{ /* Only starts when threaded == true */
  USBDBG(stdout, "[USBSID] Thread starting\r\n");
  #ifdef _GNU_SOURCE
  pthread_setname_np(pthread_self(), "USBSID Thread");
  #endif
  pthread_detach(pthread_self());
  USBDBG(stdout, "[USBSID] Thread detached\r\n");
  if (withcycles) {
    USBDBG(stdout, "[USBSID] Thread with cycles\r\n");
  }
  pthread_mutex_lock(&us_mutex);
  while(run_thread == 1) {
    if (flush_buffer == 1) {
      USBSID_FlushBuffer();
    }
    while ((run_thread == 1)
           && (us_ringbuffer.ring_read != us_ringbuffer.ring_write)
           && (USBSID_RingDiff() > diff_size)) {
      if (withcycles) {
        USBSID_RingPopCycled();
      } else {
        USBSID_RingPop();
      }
    }
    /* CPU + latency fix built around RingPut.
     * Any call to RingPut is now done inside us_mutex and signals us_cond
     * just before unlocking the mutex which wakes up this thread when new
     * data is put into the ring instead of constantly polling the ringbuffer
     * and causing cpu->thread hike with multiple boards connected.
     * The timeout is just a safety net incase of a missed wakeup and not
     * the real wake mechanism, so its exact value is not latency-critical.
     * USBSID_SendThreadBuffer() releases us_mutex during USB I/O, a
     * producer never waits on a transfer. */
    if (run_thread == 1
        && !((us_ringbuffer.ring_read != us_ringbuffer.ring_write)
             && (USBSID_RingDiff() > diff_size))) {
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_nsec += 2000000L; /* 2ms safety-net timeout */
      if (ts.tv_nsec >= 1000000000L) {
        ts.tv_nsec -= 1000000000L;
        ts.tv_sec += 1;
      }
      pthread_cond_timedwait(&us_cond, &us_mutex, &ts);
    }
  }
  USBDBG(stdout, "[USBSID] Thread finished\r\n");
  pthread_mutex_unlock(&us_mutex);
  us_thread--;
  pthread_exit(NULL);
  return NULL;
}

int USBSID_Class::USBSID_InitThread(void)
{
  USBDBG(stdout, "[USBSID] Init Thread start\r\n");
  /* Init ringbuffer */
  flush_buffer = 0;
  run_thread = buffer_pos = 1;
  threaded = withcycles = true;
  pthread_mutex_lock(&us_mutex);
  USBSID_InitRingBuffer(ring_size, diff_size);
  us_thread++;
  pthread_mutex_unlock(&us_mutex);
  int error;
  error = pthread_create(&this->us_ptid, NULL, &this->_USBSID_Thread, this);
  if (error != 0) {
    USBERR(stderr, "[USBSID] Thread can't be created :[%s]\n", strerror(error));
  }
  return rc;
}

void USBSID_Class::USBSID_StopThread(void)
{
  USBDBG(stdout, "[USBSID] Stop thread\r\n");
  if (USBSID_IsRunning() == 1) {
    USBDBG(stdout, "[USBSID] Set thread exit = 1\r\n");
    pthread_mutex_lock(&us_mutex);
    run_thread = flush_buffer = 0;
    pthread_cond_signal(&us_cond);
    pthread_mutex_unlock(&us_mutex);
    pthread_join(us_ptid, NULL);
    USBDBG(stdout, "[USBSID] Thread attached\r\n");
    threaded = withcycles = false;
    USBSID_DeInitRingBuffer(); /* after the join, the thread reads the ring until it exits */
    while (us_thread > 0) {};
    pthread_mutex_destroy(&us_mutex);
  }
  return;
}

int USBSID_Class::USBSID_IsRunning(void)
{
  return run_thread;
}

void USBSID_Class::USBSID_RestartThread(bool with_cycles)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] Restart thread (%d)\r\n", USBSID_IsRunning());
  /* First check if not already running */
  USBSID_StopThread();
  /* Stop any active transfers */
  transfer_in_pending = false;
  transfer_out_pending = false;
  LIBUSB_StopTransfers();
  /* Free all buffers */
  LIBUSB_FreeOutBuffer();
  LIBUSB_FreeInBuffer();
  /* Re-init variabels */
  threaded = true;
  withcycles = with_cycles;
  len_out_buffer = LEN_OUT_BUFFER;
  /* Init all buffers */
  LIBUSB_InitOutBuffer();
  LIBUSB_InitInBuffer();
  /* Init thread */
  USBSID_InitThread();
  return;
}

void USBSID_Class::USBSID_EnableThread(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] Enable thread (%d)\r\n", USBSID_IsRunning());
  if (USBSID_IsRunning() != 1) {
    USBSID_InitThread();
  }
  return;
}

void USBSID_Class::USBSID_DisableThread(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] Disable thread (%d)\r\n", USBSID_IsRunning());
  USBSID_StopThread();
  return;
}


/* RINGBUFFER */

void USBSID_Class::USBSID_ResetRingBuffer(void)
{
  us_ringbuffer.ring_read = us_ringbuffer.ring_write = 0;
  buffer_pos = 1;
  flush_buffer = 0;
  return;
}

void USBSID_Class::USBSID_SetBufferSize(int size)
{
  USBDBG(stdout, "[USBSID] SetBufferSize request: %d\n", size);
  if (size >= min_ring_size) {
    ring_size = size;
  } else {
    ring_size = min_ring_size;
  }
  return;
}

void USBSID_Class::USBSID_SetDiffSize(int size)
{
  USBDBG(stdout, "[USBSID] SetDiffSize request: %d\n", size);
  if (size >= min_diff_size) {
    diff_size = size;
  } else {
    diff_size = min_diff_size;
  }
  return;
}

void USBSID_Class::USBSID_InitRingBuffer(int buffer_size, int differ_size)
{ /* Init with variable settings */
  USBSID_SetBufferSize(buffer_size);
  USBSID_SetDiffSize(differ_size);
  USBSID_ResetRingBuffer();
  if (us_ringbuffer.is_allocated == 1) us_free(us_ringbuffer.ringbuffer); /* no leak on re-init */
  us_ringbuffer.ringbuffer = us_alloc(2 * ring_size, (sizeof(uint8_t)) * ring_size);
  us_ringbuffer.is_allocated = (us_ringbuffer.ringbuffer != NULL) ? 1 : 0;
  USBDBG(stdout, "[USBSID] Init RingBuffer with size: %d and diffsize: %d\n",
     buffer_size, differ_size);
  return;
}

void USBSID_Class::USBSID_InitRingBuffer(void)
{ /* Init with default settings or with values set prior to calling this function */
  USBSID_SetBufferSize(ring_size);
  USBSID_SetDiffSize(diff_size);
  USBSID_ResetRingBuffer();
  if (us_ringbuffer.is_allocated == 1) us_free(us_ringbuffer.ringbuffer); /* no leak on re-init */
  us_ringbuffer.ringbuffer = us_alloc(2 * ring_size, (sizeof(uint8_t)) * ring_size);
  us_ringbuffer.is_allocated = (us_ringbuffer.ringbuffer != NULL) ? 1 : 0;
  USBDBG(stdout, "[USBSID] Init RingBuffer with default size: %d and default diffsize: %d\n",
     ring_size, diff_size);
  return;
}

void USBSID_Class::USBSID_DeInitRingBuffer(void)
{
  USBSID_ResetRingBuffer();
  USBSID_SetBufferSize(default_ring_size);
  USBSID_SetDiffSize(default_diff_size);
  if (us_ringbuffer.is_allocated == 1) us_free(us_ringbuffer.ringbuffer);
  us_ringbuffer.ringbuffer = NULL; /* a second call must not free again */
  us_ringbuffer.is_allocated = 0;
  return;
}

void USBSID_Class::USBSID_RestartRingBuffer(void)
{ /* This function can be deprecated in favour of using deinit and init */
  if (!us_PortIsOpen) return;
  /* Store diff and ring size for fun and profit lol */
  int temp_diff_size = diff_size;
  int temp_ring_size = ring_size;
  USBSID_DeInitRingBuffer();
  /* Reassigned previous sizes */
  diff_size = temp_diff_size;
  ring_size = temp_ring_size;
  USBSID_InitRingBuffer();
  return;
}

int USBSID_Class::USBSID_RingFree(void)
{ /* The ringbuffer has no overflow protection, a producer that writes faster
   * than the thread sends overwrites unsent data. This lets the caller apply
   * backpressure. Read without the lock, so an estimate. One slot always stays
   * unused to tell a full ring from an empty one. */
  if (us_ringbuffer.is_allocated != 1) return 0;
  const int r = us_ringbuffer.ring_read;
  const int w = us_ringbuffer.ring_write;
  return ring_size - 1 - ((w - r + ring_size) % ring_size);
}

bool USBSID_Class::USBSID_IsHigher()
{
  return (us_ringbuffer.ring_read < us_ringbuffer.ring_write);
}

int USBSID_Class::USBSID_RingDiff()
{
  int d = (USBSID_IsHigher()
    ? (us_ringbuffer.ring_read - us_ringbuffer.ring_write)
    : (us_ringbuffer.ring_write - us_ringbuffer.ring_read));
  return ((d < 0) ? (d * -1) : d);
}

void USBSID_Class::USBSID_RingPut(uint8_t item)
{
  us_ringbuffer.ringbuffer[us_ringbuffer.ring_write] = item;
  us_ringbuffer.ring_write = (us_ringbuffer.ring_write + 1) % ring_size;
  return;
}

uint8_t USBSID_Class::USBSID_RingGet()
{
  uint8_t item = us_ringbuffer.ringbuffer[us_ringbuffer.ring_read];
  us_ringbuffer.ring_read = (us_ringbuffer.ring_read + 1) % ring_size;
  return item;
}

void USBSID_Class::USBSID_Flush(void)
{ /* Signal only, USBSID_FlushBuffer() runs on the driver thread */
  if (!us_PortIsOpen) return;
  USBSID_SetFlush();
  return;
}

void USBSID_Class::USBSID_SetFlush(void)
{
  USBSID_SyncTime();
  pthread_mutex_lock(&us_mutex);
  flush_buffer = 1;
  pthread_cond_signal(&us_cond);
  pthread_mutex_unlock(&us_mutex);
  return;
}


/* RINGBUFFER READS & WRITES */

void USBSID_Class::USBSID_FlushBuffer(void)
{
  if (!threaded || flush_buffer != 1) {
    flush_buffer = 0;
    return;
  }

  /* The flush is a deadline, so whatever is still waiting in the ring belongs
   * in this packet.
   *
   * this drains whatever is waiting in the ring on a flush deadline instead
   * of leaving it to the thread's own diff_size-gated drain loop, which
   * otherwise leaves writes sitting in the ring far longer than their caller
   * intended. */
  const int rec = (withcycles == 1) ? 4 : 2;
  const int cap = (withcycles == 1) ? 61 : 63;
  int waiting = (us_ringbuffer.ring_write - us_ringbuffer.ring_read
                 + ring_size) % ring_size;
  while (waiting >= rec && (buffer_pos + rec) <= cap) {
    for (int i = 0; i < rec; i++) {
      thread_buffer[buffer_pos++] = USBSID_RingGet();
    }
    waiting -= rec;
  }

  if (((withcycles == 1)
    ? (buffer_pos >= 5)
    : (buffer_pos >= 3))) {
    thread_buffer[0] = (withcycles == 1)
      ? (uint8_t)(CYCLED_WRITE << 6 | (buffer_pos - 1))
      : (uint8_t)(WRITE << 6 | (buffer_pos - 1));
    flush_buffer = 0;
    USBSID_SendThreadBuffer();
  } else {
    flush_buffer = 0;
  }
  return;
}

void USBSID_Class::USBSID_WriteRing(uint8_t reg, uint8_t val)
{
  if (!us_PortIsOpen) return;
  if (threaded && !withcycles) {
    pthread_mutex_lock(&us_mutex);
    USBSID_RingPut(reg);
    USBSID_RingPut(val);
    pthread_cond_signal(&us_cond);
    pthread_mutex_unlock(&us_mutex);
  } else {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded = %d and withcycles = %d\n",
      __func__, threaded, withcycles);
  }
  return;
}

void USBSID_Class::USBSID_WriteRingCycled(uint8_t reg, uint8_t val, uint16_t cycles)
{
  if (!us_PortIsOpen) return;
  if (threaded && withcycles) {
    pthread_mutex_lock(&us_mutex);
    USBSID_RingPut(reg);
    USBSID_RingPut(val);
    USBSID_RingPut((uint8_t)(cycles >> 8) & 0xFF);
    USBSID_RingPut((uint8_t)(cycles & 0xFF));
    pthread_cond_signal(&us_cond);
    pthread_mutex_unlock(&us_mutex);
  } else {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded = %d and withcycles = %d\n",
      __func__, threaded, withcycles);
  }
  return;
}

void USBSID_Class::USBSID_WriteRingCycledN(const uint8_t *items, int count)
{ /* Batch variant of USBSID_WriteRingCycled(). `items` holds `count` writes of
   * 4 bytes each: reg, val, cycles hi, cycles lo. The driver thread holds
   * us_mutex while it sends, so one lock per batch keeps a producer from
   * queueing behind the thread on every single write. */
  if (!us_PortIsOpen || items == NULL || count <= 0) return;
  if (threaded && withcycles) {
    pthread_mutex_lock(&us_mutex);
    for (int i = 0; i < count * 4; i++) USBSID_RingPut(items[i]);
    pthread_cond_signal(&us_cond);
    pthread_mutex_unlock(&us_mutex);
  } else {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded = %d and withcycles = %d\n",
      __func__, threaded, withcycles);
  }
  return;
}

void USBSID_Class::USBSID_RingPopCycled(void)
{
  if (transfer_out_pending) {
    USBSID_WaitTransferOut();
    return;
  }
  thread_buffer[buffer_pos++] = USBSID_RingGet();  /* register */
  thread_buffer[buffer_pos++] = USBSID_RingGet();  /* value */
  thread_buffer[buffer_pos++] = USBSID_RingGet();  /* n cycles high */
  thread_buffer[buffer_pos++] = USBSID_RingGet();  /* n cycles low */

  if (buffer_pos == 61  /* >= 61 || >= 4 */
      || buffer_pos == len_out_buffer
      || flush_buffer == 1) {
    flush_buffer = 0;
    thread_buffer[0] = (uint8_t)((CYCLED_WRITE << 6) | (buffer_pos - 1));
    USBSID_SendThreadBuffer();
  }
  return;
}

void USBSID_Class::USBSID_RingPop(void)
{
  if (transfer_out_pending) {
    USBSID_WaitTransferOut();
    return;
  }
  write_completed = 0;

  /* Ex: 0xD418 */
  thread_buffer[buffer_pos++] = USBSID_RingGet();  /* register */
  thread_buffer[buffer_pos++] = USBSID_RingGet();  /* value */
  if (buffer_pos == 63  /* >= 61 || >= 4 */
    || buffer_pos == len_out_buffer
    || flush_buffer == 1) {
    flush_buffer = 0;
    thread_buffer[0] = (uint8_t)((WRITE << 6) | (buffer_pos - 1));
    USBSID_SendThreadBuffer();
  }
  return;
}

void USBSID_Class::USBSID_SendThreadBuffer(void)
{ /* Driver thread only, us_mutex held on entry and on return. Release
   * us_mutex during USB I/O, producers only wait on ring access.
   * thread_buffer and out_buffer are only touched by the driver thread. */
  const int len = buffer_pos;
  buffer_pos = 1;
  pthread_mutex_unlock(&us_mutex);
  while (transfer_out_pending && run_thread == 1) {  /* Previous packet in flight */
    struct timeval tv = {0, 500};  // 0.5 ms
    libusb_handle_events_timeout_completed(ctx, &tv, NULL);
  }
  if (!transfer_out_pending && transfer_out != NULL) {  /* NULL after a failed transfer */
    memset(out_buffer, 0, len_out_buffer);
    memcpy(out_buffer, thread_buffer, len);
    transfer_out_pending = true;
    if (libusb_submit_transfer(transfer_out) < 0) {
      transfer_out_pending = false;
    } else {
      libusb_handle_events_completed(ctx, NULL);
#ifdef USE_VENDOR_ITF
      struct timeval tv = {0, 500};  // 0.5 ms
      libusb_handle_events_timeout_completed(ctx, &tv, NULL);
#endif
    }
  }
  memset(thread_buffer, 0, 64);
  pthread_mutex_lock(&us_mutex);
  return;
}

void USBSID_Class::USBSID_WaitTransferOut(void)
{ /* Driver thread only, us_mutex held on entry and on return. Block on
   * libusb events with us_mutex released instead of spinning. */
  pthread_mutex_unlock(&us_mutex);
  struct timeval tv = {0, 500};  // 0.5 ms
  libusb_handle_events_timeout_completed(ctx, &tv, NULL);
  pthread_mutex_lock(&us_mutex);
  return;
}


/* TIMING AND CYCLES */

void USBSID_Class::USBSID_SyncTime(void)
{
  if (!us_PortIsOpen) return;
  m_LastTime = std::chrono::steady_clock::now();
}

uint_fast64_t USBSID_Class::USBSID_WaitForCycle(uint_fast16_t cycles)
{
  auto start = std::chrono::steady_clock::now();
  auto duration_ns = duration_t((long)(cycles * us_CPUcycleDuration));
  auto target_time = (start + duration_ns);

  do {
  } while (std::chrono::steady_clock::now() < target_time);

  auto end = std::chrono::steady_clock::now();
  auto actual_ns = static_cast<long long>(std::chrono::duration_cast<duration_t>(end - start).count());

  return (uint_fast64_t)actual_ns;
}


/* LIBUSB */

int USBSID_Class::LIBUSB_OpenDevice(void)
{
  USBDBG(stdout, "[USBSID] Open device\r\n");
  /* Count the devices in the device list */
  struct libusb_device **devs;
  ssize_t cnt = libusb_get_device_list(ctx, &devs);
  if (cnt < 0) {
    rc = (int)cnt;
    USBERR(stderr, "[USBSID] Error listing USB devices: %d %s: %s\r\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    return rc;
  }

  /* Create the target device data */
  struct libusb_device *target = NULL;

  /* Index candidates, sorted like USBSID_EnumerateDevices() */
  struct Candidate {
    struct libusb_device *dev;
    uint8_t bus;
    std::vector<uint8_t> path;
  };
  std::vector<Candidate> candidates;

  /* Find the target we want by serial and assign the index requested */
  for (ssize_t i = 0; i < cnt; i++) {
    struct libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(devs[i], &desc) < 0) continue;
    if (desc.idVendor != VENDOR_ID || desc.idProduct != PRODUCT_ID) continue;

    if (!want_serial.empty()) { /* If requested by serial number */
      /* Reading a serial requires an open handle,
       * probe each match and close again on a miss. */
      struct libusb_device_handle *probe = NULL;
      if (libusb_open(devs[i], &probe) == 0) {
        /* Read the serialnumber */
        std::string serial;
        if (desc.iSerialNumber) {
          unsigned char sbuf[64] = {0};
          int slen = libusb_get_string_descriptor_ascii(probe, desc.iSerialNumber, sbuf, sizeof(sbuf) - 1);
          if (slen > 0) serial.assign((char*)sbuf, slen);
        }
        /* Is it the one we wanted? */
        if (serial == want_serial) {
          devh = probe;
          opened_serial = serial;
          target = devs[i];
          break; /* Break out of the loop if this is the one we wanted */
        }
        /* Close the device if not the one we wanted */
        libusb_close(probe);
      }
    } else { /* If no serial given collect for index selection */
      candidates.push_back({devs[i], libusb_get_bus_number(devs[i]), us_port_path(devs[i])});
    }
  }

  if (want_serial.empty()) {
    std::sort(candidates.begin(), candidates.end(), [](const Candidate &a, const Candidate &b) {
      return us_port_less(a.bus, a.path, b.bus, b.path);
    });
    if (want_index >= 0 && (size_t)want_index < candidates.size()) {
      target = candidates[want_index].dev;
    }
  }

  /* Validation */
  if (!target) {
    rc = -1;
    USBERR(stderr, "[USBSID] Error, no matching USBSID-Pico found (serial='%s' index=%d): %d\r\n",
      want_serial.empty() ? "<any>" : want_serial.c_str(), want_index, rc);
    libusb_free_device_list(devs, 1);
    return rc;
  }

  /* Validation */
  if (!devh) {
    rc = libusb_open(target, &devh);
    if (rc < 0 || !devh) {
      USBERR(stderr, "[USBSID] Error opening USB device: %d %s: %s\r\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
      libusb_free_device_list(devs, 1);
      return rc;
    }
  }

  /* No errors? Then we can clear the device list */
  libusb_free_device_list(devs, 1);

  /* Assign the serial number if still empty */
  if (opened_serial.empty()) {
    struct libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(libusb_get_device(devh), &desc) == 0 && desc.iSerialNumber) {
      unsigned char sbuf[64] = {0};
      int slen = libusb_get_string_descriptor_ascii(devh, desc.iSerialNumber, sbuf, sizeof(sbuf) - 1);
      if (slen > 0) opened_serial.assign((char*)sbuf, slen);
    }
  }

  /* On macOS the IOKit CDC driver will reclaim interfaces unless we enable
   * auto-detach, which makes libusb detach/reattach the kernel driver
   * automatically around libusb_claim_interface / libusb_release_interface. */
  rc = libusb_set_auto_detach_kernel_driver(devh, 1);
  if (rc == LIBUSB_ERROR_NOT_SUPPORTED) {
    rc = 0;
  } else if (rc < 0) {
    USBERR(stderr, "[USBSID] Error setting auto detach kernel driver: %d %s: %s\r\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
  }
  return rc;
}

void USBSID_Class::LIBUSB_CloseDevice(void)
{
  USBDBG(stdout, "[USBSID] Close device\r\n");
  if (devh) {
#ifdef USE_VENDOR_ITF
    /* e.g. macOS needs Vendor interface 4, others may stay with 0, 1 */
    int start_if = 4;
    /* no need to detach again... */
    rc = libusb_release_interface(devh, start_if);
#else
    libusb_release_interface(devh, 0);
    libusb_release_interface(devh, 1);

    rc = libusb_attach_kernel_driver(devh, 0);
    if (rc < 0 && rc != LIBUSB_ERROR_NOT_FOUND) {
      fprintf(stderr, "Attach error on interface 0: %s\n", libusb_error_name(rc));
    }
#endif
    libusb_close(devh);
    devh = NULL;
  }
  return;
}

int USBSID_Class::LIBUSB_Available(uint16_t vendor_id, uint16_t product_id)
{
  struct libusb_device **devs;
  struct libusb_device *dev;
  size_t i = 0;
  int r;
  int found = 0;
  us_Available = false;

  if (libusb_get_device_list(ctx, &devs) < 0)
    return 0;

  while ((dev = devs[i++]) != NULL) {
    struct libusb_device_descriptor desc;
    r = libusb_get_device_descriptor(dev, &desc);
    if (r < 0)
      goto out;
    if (desc.idVendor == vendor_id && desc.idProduct == product_id) {
      us_Available = true;
      found++;
      continue;
    }
  }
out:
  libusb_free_device_list(devs, 1);
  return (us_Available ? found : 0);
}

int USBSID_Class::LIBUSB_DetachKernelDriver(void)
{
  USBDBG(stdout, "[USBSID] Detach kernel driver\r\n");
  /* USBSID-Pico acts as a CDC-ACM device and on Linux it's
   * highly probable that the OS already attached the cdc-acm
   * driver the moment it got plugged in.
   * On device close the drivers need to be detached from all
   * the USB interfaces to return to previous functionalities.
   * The CDC-ACM Class defines two interfaces:
   * the Control interface and the Data interface.
   */
    int start_if, end_if;
    /* macOS needs Vendor interface 4, others can stay with 0, 1 */
#ifdef USE_VENDOR_ITF
    start_if = 4;
    end_if = 5;
#else
    start_if = 0;
    end_if = 2;
#endif
  for (int if_num = start_if; if_num < end_if; if_num++) {
    if (libusb_kernel_driver_active(devh, if_num) == 1) {
      libusb_detach_kernel_driver(devh, if_num);
    }
    rc = libusb_claim_interface(devh, if_num);
    if (rc < 0) {
      USBERR(stderr, "[USBSID] Error claiming interface: %d, %s: %s\r\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
      rc = -1;
      break;
    }
  }
  return rc;
}

int USBSID_Class::LIBUSB_ConfigureDevice(void)
{
  USBDBG(stdout, "[USBSID] Configure device\r\n");
#ifdef USE_VENDOR_ITF
    /* again macOS needs this */
    rc = libusb_set_interface_alt_setting(devh, 4, 0);
    rc = libusb_control_transfer(devh, 0x21, 0x22, 0x01, 4, NULL, 0, 1000);
#else
  /* Start configuring the device:
   * set line state */
  rc = libusb_control_transfer(devh, 0x21, 0x22, ACM_CTRL_DTR | ACM_CTRL_RTS, 0, NULL, 0, 0);
  if (rc < 0) {  /* should return 0 or higher */
    USBERR(stderr, "[USBSID] Error configuring line state during control transfer: %d, %s: %s\r\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    rc = -1;
    return rc;
  }

  /* set line encoding here, required but not used for CDC */
  rc = libusb_control_transfer(devh, 0x21, 0x20, 0, 0, encoding, sizeof(encoding), 1000);
  if (rc < 0 || rc != 7) {  /* should return 7 for the encoding size */
    USBERR(stderr, "[USBSID] Error configuring line encoding during control transfer: %d, %s: %s\r\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    rc = -1;
    return rc;
  }
#endif
  return rc;
}

void USBSID_Class::LIBUSB_InitOutBuffer(void)
{
  USBDBG(stdout, "[USBSID] Init out buffers\r\n");
  out_buffer = libusb_dev_mem_alloc(devh, len_out_buffer);
  if (out_buffer == NULL) {
    USBDBG(stdout, "[USBSID] libusb_dev_mem_alloc failed on out_buffer, allocating with malloc\r\n");
    out_buffer = us_alloc(2 * len_out_buffer, (sizeof(uint8_t)) * len_out_buffer);
  } else {
    out_buffer_dma = true;
  }
  USBDBG(stdout, "[USBSID] Alloc out_buffer complete\r\n");
  transfer_out = libusb_alloc_transfer(0);
  USBDBG(stdout, "[USBSID] Alloc transfer_out complete\r\n");
  libusb_fill_bulk_transfer(transfer_out, devh, EP_OUT_ADDR, out_buffer, len_out_buffer, usb_out, this, LIBUSB_TIMEOUT);
  USBDBG(stdout, "[USBSID] libusb_fill_bulk_transfer transfer_out complete\r\n");

  if (thread_buffer == NULL) {
    thread_buffer = us_alloc(2 * len_out_buffer, (sizeof(uint8_t)) * (len_out_buffer));
  }
  if (write_buffer == NULL) {
    write_buffer = us_alloc(2 * len_out_buffer, (sizeof(uint8_t)) * (len_out_buffer));
  }
  return;
}

void USBSID_Class::LIBUSB_FreeOutBuffer(void)
{
  USBDBG(stdout, "[USBSID] Free out buffers\r\n");
  if (out_buffer_dma) {
    rc = libusb_dev_mem_free(devh, out_buffer, len_out_buffer);
    if (rc < 0) {
      USBERR(stderr, "[USBSID] Error, failed to free out_buffer DMA memory: %d, %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    }
  } else {
    if (out_buffer) us_free(out_buffer);
    out_buffer = NULL;
  }
  if (thread_buffer) {
    us_free(thread_buffer);
    thread_buffer = NULL;
  }
  if (write_buffer) {
    us_free(write_buffer);
    write_buffer = NULL;
  }
  return;
}

void USBSID_Class::LIBUSB_InitInBuffer(void)
{
  USBDBG(stdout, "[USBSID] Init in buffers\r\n");
  in_buffer = libusb_dev_mem_alloc(devh, LEN_IN_BUFFER);
  if (in_buffer == NULL) {
    USBDBG(stdout, "[USBSID] libusb_dev_mem_alloc failed on in_buffer, allocating with malloc\r\n");
    in_buffer = us_alloc(2 * LEN_IN_BUFFER, (sizeof(uint8_t)) * LEN_IN_BUFFER);
  } else {
    in_buffer_dma = true;
  }
  USBDBG(stdout, "[USBSID] Alloc in_buffer complete\r\n");
  transfer_in = libusb_alloc_transfer(0);
  USBDBG(stdout, "[USBSID] Alloc transfer_in complete\r\n");
  libusb_fill_bulk_transfer(transfer_in, devh, EP_IN_ADDR, in_buffer, LEN_IN_BUFFER, usb_in, this, LIBUSB_TIMEOUT);
  USBDBG(stdout, "[USBSID] libusb_fill_bulk_transfer transfer_in complete\r\n");

  if (result == NULL) {
    result = us_alloc(2 * LEN_IN_BUFFER, (sizeof(uint8_t)) * (LEN_IN_BUFFER));
  }
  return;
}

void USBSID_Class::LIBUSB_FreeInBuffer(void)
{
  USBDBG(stdout, "[USBSID] Free in buffers\r\n");
  if (in_buffer_dma) {
    rc = libusb_dev_mem_free(devh, in_buffer, LEN_IN_BUFFER);
    if (rc < 0) {
      USBERR(stderr, "[USBSID] Error, failed to free in_buffer DMA memory: %d, %s: %s\n", rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    }
  } else {
    if (in_buffer) us_free(in_buffer);
    in_buffer = NULL;
  }
  if (result) {
    us_free(result);
    result = NULL;
  }
  return;
}

void USBSID_Class::LIBUSB_StopTransfers(void)
{
  USBDBG(stdout, "[USBSID] Stopping transfers\r\n");

  if (transfer_out && transfer_out_pending) {
    rc = libusb_cancel_transfer(transfer_out);
    if (rc < 0 && rc != LIBUSB_ERROR_NOT_FOUND) {
      USBERR(stderr, "[USBSID] Error cancel OUT %d - %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    }
  }

  if (transfer_in && transfer_in_pending) {
    rc = libusb_cancel_transfer(transfer_in);
    if (rc < 0 && rc != LIBUSB_ERROR_NOT_FOUND) {
      USBERR(stderr, "[USBSID] Error cancel IN %d - %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    }
  }

  while (transfer_out_pending || transfer_in_pending) {
    struct timeval tv = {0, 1000};
    libusb_handle_events_timeout_completed(ctx, &tv, NULL);
  }

  if (transfer_out) {
    libusb_free_transfer(transfer_out);
    transfer_out = NULL;
  }

  if (transfer_in) {
    libusb_free_transfer(transfer_in);
    transfer_in = NULL;
  }
}

int USBSID_Class::LIBUSB_Setup(bool start_threaded, bool with_cycles)
{
  rc = read_completed = write_completed = -1;
  threaded = start_threaded;
  withcycles = with_cycles;
  len_out_buffer = LEN_OUT_BUFFER;

  /* Initialize libusb */
  rc = libusb_init(&ctx);
  /* For LIBUSB version 1.0.27+ we could use this: */
  // rc = libusb_init_context(&ctx, /*options=NULL, /*num_options=*/0);
  if (rc != 0) {
    USBERR(stderr, "[USBSID] Error initializing libusb: %d %s: %s\r\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    goto out;
  }

  /* Set debugging output to min/max (4) level */
  libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, 0);

  /* Check for an available USBSID-Pico */
  if (LIBUSB_Available(VENDOR_ID, PRODUCT_ID) <= 0) {
    USBDBG(stderr, "[USBSID] USBSID-Pico not connected\n");
    goto out;
  }

  if (LIBUSB_OpenDevice() < 0) {
    goto out;
  }

  if (LIBUSB_DetachKernelDriver() < 0) {
    goto out;
  }

  if (LIBUSB_ConfigureDevice() < 0) {
    goto out;
  }

  LIBUSB_InitOutBuffer();
  LIBUSB_InitInBuffer();

  if (rc < 0) {
    USBERR(stderr, "[USBSID] Error, could not open device: %d, %s: %s\r\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    goto out;
  }

  if (rc > 0 && rc == 7) {  /* 7 for the return size of the encoding */
    rc = 0;
  }

  return rc;
out:
  LIBUSB_Exit();
  return rc;
}

int USBSID_Class::LIBUSB_Exit(void)
{
  if (rc >= 0) {
    USBSID_StopThread();
    #ifdef US_MUTE_ON_EXIT
    USBSID_Mute();
    #endif
    #ifdef US_RESET_ON_EXIT
    USBSID_Reset();
    #endif
    transfer_in_pending = false;
    transfer_out_pending = false;
    LIBUSB_StopTransfers();
    LIBUSB_FreeInBuffer();
    LIBUSB_FreeOutBuffer();
    LIBUSB_CloseDevice();
  }
  if (ctx) {
    libusb_exit(ctx);
  }
  ctx = NULL;
  rc = -1;
  devh = NULL;
  USBDBG(stdout, "[USBSID] Closed USB device\r\n");
  return 0;
}

void LIBUSB_CALL USBSID_Class::usb_out(struct libusb_transfer *transfer)
{
  USBSID_Class *self = (USBSID_Class *)transfer->user_data;

  if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
    if (self) self->rc = transfer->status;
    if (transfer->status != LIBUSB_TRANSFER_CANCELLED) {
      USBERR(stderr, "[USBSID] Warning: transfer out interrupted with status %d, %s: %s\r",
        transfer->status, libusb_error_name(transfer->status), libusb_strerror((enum libusb_error)transfer->status));
    }
    libusb_free_transfer(transfer);
    if (self) {
      self->transfer_out = NULL;
      self->transfer_out_pending = false;
    }
    return;
  }

  if (self && transfer->actual_length != self->len_out_buffer) {
    USBERR(stderr, "[USBSID] Sent data length %d is different from the defined buffer length: %d or actual length %d\r",
      transfer->length, self->len_out_buffer, transfer->actual_length);
  }
  if (self) self->transfer_out_pending = false;
}

void LIBUSB_CALL USBSID_Class::usb_in(struct libusb_transfer *transfer)
{
  USBSID_Class *self = (USBSID_Class *)transfer->user_data;

  if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
    if (self) self->rc = transfer->status;
    if (transfer->status != LIBUSB_TRANSFER_CANCELLED) {
      USBERR(stderr, "[USBSID] Warning: transfer in interrupted with status '%s'\r",
        libusb_error_name(transfer->status));
    }
    libusb_free_transfer(transfer);
    if (self) self->transfer_in = NULL;
    return;
  }

  if (self && transfer->actual_length > 0) {
    memcpy(self->result, self->in_buffer, 1);
  }

  if (self) {
    self->read_completed = 1;
    self->transfer_in_pending = false;
  }
}

} /* extern "C" */


std::vector<USBSID_DeviceInfo> USBSID_NS::USBSID_EnumerateDevices(void)
{
  std::vector<USBSID_DeviceInfo> found;

  libusb_context *enum_ctx = NULL;
  if (libusb_init(&enum_ctx) != 0) return found;
  libusb_set_option(enum_ctx, LIBUSB_OPTION_LOG_LEVEL, 0);

  struct libusb_device **devs;
  ssize_t cnt = libusb_get_device_list(enum_ctx, &devs);
  if (cnt < 0) {
    libusb_exit(enum_ctx);
    return found;
  }

  for (ssize_t i = 0; i < cnt; i++) {
    struct libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(devs[i], &desc) < 0) continue;
    if (desc.idVendor != VENDOR_ID || desc.idProduct != PRODUCT_ID) continue;

    USBSID_DeviceInfo info;
    info.bus = libusb_get_bus_number(devs[i]);
    info.port_path = us_port_path(devs[i]);

    if (desc.iSerialNumber) {
      struct libusb_device_handle *h = NULL;
      if (libusb_open(devs[i], &h) == 0) {
        unsigned char sbuf[64] = {0};
        int slen = libusb_get_string_descriptor_ascii(h, desc.iSerialNumber, sbuf, sizeof(sbuf) - 1);
        if (slen > 0) info.serial.assign((char*)sbuf, slen);
        libusb_close(h);
      }
    }

    found.push_back(info);
  }

  libusb_free_device_list(devs, 1);
  libusb_exit(enum_ctx);

  std::sort(found.begin(), found.end(), [](const USBSID_DeviceInfo &a, const USBSID_DeviceInfo &b) {
    return us_port_less(a.bus, a.port_path, b.bus, b.port_path);
  });

  return found;
}
