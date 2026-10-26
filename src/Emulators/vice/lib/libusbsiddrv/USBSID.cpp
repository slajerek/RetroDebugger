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
 * @brief Return the USB port path from root hub to device, empty on failure
 *
 * @param dev libusb device
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
 * @brief Order devices by bus, then port path, return true if a sorts before b
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
  /* Destroy in this function, a thread restart after StopThread() locks it again */
  pthread_mutex_destroy(&us_mutex);
  pthread_cond_destroy(&us_cond);
#ifdef USBSID_SID_STREAM
  pthread_mutex_destroy(&stream_cb_mutex);
#endif /* USBSID_SID_STREAM */
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
      if (!passive) {
        USBSID_Mute();
        USBSID_ClearBus();
        USBSID_UnMute();
        USBSID_GetClockRate();  /* Once on init */
      }
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
  /* Always exit, a transfer error leaves rc < 0 with the device still open */
  int e = LIBUSB_Exit();
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
  USBDBG(stdout, "[USBSID] Pause\n");
  unsigned char buff[3] = {(COMMAND << 6 | PAUSE), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_Reset(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] Reset\n");
  unsigned char buff[3] = {(COMMAND << 6 | RESET_SID), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  flush_buffer = 1;
  USBSID_SyncTime();
  return;
}

void USBSID_Class::USBSID_ResetAllRegisters(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] Reset All Registers\n");
  unsigned char buff[3] = {(COMMAND << 6 | RESET_SID), 0x1, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_Mute(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] Mute\n");
  unsigned char buff[3] = {(COMMAND << 6 | MUTE), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_UnMute(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] UnMute\n");
  unsigned char buff[3] = {(COMMAND << 6 | UNMUTE), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_SetMuted(bool muted)
{ /* Argument 1 in the MUTE/UNMUTE command sets the firmware's muted state, which
   * masks all following volume register writes until unmuted
   * USBSID_Mute() sends 0 and zeroes the volume once, a following volume write is audible again */
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] SetMuted %d\n", muted);
  unsigned char buff[3] = {(unsigned char)(COMMAND << 6 | (muted ? MUTE : UNMUTE)), 0x1, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_DisableSID(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] DisableSID\n");
  unsigned char buff[3] = {(COMMAND << 6 | DISABLE_SID), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_EnableSID(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] EnableSID\n");
  unsigned char buff[3] = {(COMMAND << 6 | ENABLE_SID), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_ClearBus(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] ClearBus\n");
  unsigned char buff[3] = {(COMMAND << 6 | CLEAR_BUS), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_SetClockRate(long clockrate_cycles, bool suspend_sids)
{ /* Only sends the command if the requested rate differs from the known board rate */
  USBSID_SetClockRate(clockrate_cycles, suspend_sids, false);
  return;
}

void USBSID_Class::USBSID_SetClockRate(long clockrate_cycles, bool suspend_sids, bool force)
{ /* force sends the command, also when the board rate is known
   * Used when switching rates during use (e.g. a tracker changing PAL/NTSC per tune) */
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
      if (force || clk_retrieved == 0 || us_clkrate != cycles_per_sec) {
        uint8_t configbuff[6] = {
          (COMMAND << 6 | CONFIG),
          0x50,
          i,
          (uint8_t)(suspend_sids == true ? 1 : 0),
          0, 0};
        USBSID_SingleWrite(configbuff, 6);
        us_clkrate = cycles_per_sec;  /* Keep the known board rate current */
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
  const int err = libusb_bulk_transfer(devh, EP_OUT_ADDR, buff, (int)len, &actual_length, LIBUSB_TIMEOUT);
  if (err < 0) {
    USBERR(stderr, "[USBSID] Error while sending synchronous write buffer of length %d\n",
      actual_length);
    LIBUSB_CheckLost(err);
  }
  return;
}

unsigned char USBSID_Class::USBSID_SingleRead(uint8_t reg)
{
  if (!us_PortIsOpen) return 0;
  int actual_length;
  unsigned char buff[3] = {(READ << 6), reg, 0};
  const int err = libusb_bulk_transfer(devh, EP_OUT_ADDR, buff, 3, &actual_length, LIBUSB_TIMEOUT);
  if (err < 0) {
    USBERR(stderr, "[USBSID] Error while sending write command for reading\n");
    LIBUSB_CheckLost(err);
  }
  rc = LIBUSB_ReadIn(result, 1, &actual_length);
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

/**
 * @brief Read n registers in one round trip (batch READ), returns n or -1 on failure
 *
 * Registers are read back to back in the given order
 * Order against queued ringbuffer writes of the driver thread is undefined
 * Older firmware (below US_FW_VERSION_BATCH_READ) gets n x USBSID_SingleRead()
 * A short reply falls back to USBSID_SingleRead() for the remainder
 *
 * @param regs n register addresses, SID local addressing as for SingleRead
 * @param out Receives n values
 * @param n Register count, 1-63
 */
int USBSID_Class::USBSID_ReadMulti(const uint8_t *regs, uint8_t *out, int n)
{
  if (!us_PortIsOpen || regs == NULL || out == NULL) return -1;
  if (n < 1 || n > (LEN_OUT_BUFFER - 1)) return -1;
  const int fv = USBSID_GetFirmwareVersionInt();
  if (fv < US_FW_VERSION_BATCH_READ) {  /* Firmware lacks batch read or version unknown (-1) */
    for (int i = 0; i < n; i++) out[i] = USBSID_SingleRead(regs[i]);
    return n;
  }
  unsigned char buff[LEN_OUT_BUFFER];
  buff[0] = (unsigned char)((READ << 6) | n);
  memcpy(&buff[1], regs, (size_t)n);
  int actual_length = 0;
  rc = libusb_bulk_transfer(devh, EP_OUT_ADDR, buff, n + 1, &actual_length, LIBUSB_TIMEOUT);
  if (rc < 0) {
    USBERR(stderr, "[USBSID] Error while sending batch read command: %d, %s: %s\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    return -1;
  }
  rc = LIBUSB_ReadIn(out, (size_t)n, &actual_length);
  if (rc < 0 || actual_length < 1) {
    USBERR(stderr, "[USBSID] Error while reading batch read reply: %d, %s: %s\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    return -1;
  }
  for (int i = actual_length; i < n; i++) {  /* Short reply */
    out[i] = USBSID_SingleRead(regs[i]);
  }
  return n;
}

unsigned char USBSID_Class::USBSID_SingleReadConfig(unsigned char *buff, size_t len)
{
  if (!us_PortIsOpen) return 0;
  int actual_length;
  rc = LIBUSB_ReadIn(buff, len, &actual_length);
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
  rc = LIBUSB_ReadIn(buff, len, &actual_length);
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


/* COMMAND CHANNEL */

/**
 * @brief Send raw command bytes with one synchronous bulk transfer, returns bytes sent or -1
 *
 * @param buff Bytes to send
 * @param len Number of bytes
 */
int USBSID_Class::USBSID_SendCommand(const unsigned char *buff, size_t len)
{
  if (!us_PortIsOpen || buff == NULL) return -1;
  int actual_length = 0;
  int ret = libusb_bulk_transfer(devh, EP_OUT_ADDR, const_cast<unsigned char *>(buff),
    (int)len, &actual_length, LIBUSB_TIMEOUT);
  if (ret < 0) {
    USBERR(stderr, "[USBSID] Error sending command: %d, %s: %s\n",
      ret, libusb_error_name(ret), libusb_strerror((enum libusb_error)ret));
    LIBUSB_CheckLost(ret);
    return -1;
  }
  return actual_length;
}

/**
 * @brief Read one reply with a synchronous bulk transfer, returns bytes read or -1 on failure or timeout
 *
 * @param buff Destination, receives at most len bytes
 * @param len Maximum number of bytes
 */
int USBSID_Class::USBSID_ReadResponse(unsigned char *buff, size_t len)
{
  if (!us_PortIsOpen || buff == NULL) return -1;
  int actual_length = 0;
  int ret = LIBUSB_ReadIn(buff, len, &actual_length);
  if (ret < 0) {
    USBERR(stderr, "[USBSID] Error reading reply: %d, %s: %s\n",
      ret, libusb_error_name(ret), libusb_strerror((enum libusb_error)ret));
    return -1;
  }
  return actual_length;
}

/**
 * @brief Send a 6 byte config command, returns bytes sent or -1
 *
 * @param sub Config sub command, byte 1
 * @param a Byte 2
 * @param b Byte 3
 * @param c Byte 4
 * @param d Byte 5
 */
int USBSID_Class::USBSID_SendConfig(uint8_t sub, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
  unsigned char buff[6] = {(COMMAND << 6 | CONFIG), sub, a, b, c, d};
  return USBSID_SendCommand(buff, sizeof(buff));
}

/**
 * @brief Read the firmware feature bitmask (US_FEATURE_*) once per open, returns -1 on failure
 */
int USBSID_Class::USBSID_GetFeatures(void)
{
  if (!us_PortIsOpen) return -1;
  if (features >= 0) return features;
  if (USBSID_SendConfig(US_FEATURES, 0, 0, 0, 0) < 0) return -1;
  unsigned char reply[1] = {0};
  if (USBSID_ReadResponse(reply, 1) != 1) return -1;
  features = reply[0];
  return features;
}

/**
 * @brief Read the firmware version integer (e.g. 81 for 0.8.1) once per open, returns -1 on failure
 *
 * Sends USBSID_VERSION with byte 2 set, new firmware replies one byte, old firmware the 64 byte string
 */
int USBSID_Class::USBSID_GetFirmwareVersionInt(void)
{
  if (!us_PortIsOpen) return -1;
  if (fwversion >= 0) return fwversion;
  if (USBSID_SendConfig(USBSID_VERSION, 1, 0, 0, 0) < 0) return -1;
  unsigned char reply[64] = {0};  /* Old firmware answers with the 64 byte version string reply */
  const int got = USBSID_ReadResponse(reply, sizeof(reply));
  if (got < 1) return -1;
  /* Reply byte 0 == 0x80 with byte 1 != 0 (or more than one byte) is the old string reply
   * (byte 1 = string length) and means firmware older than US_FW_VERSION_BATCH_READ
   * The string "0.7.1" is parsed to 71 like the firmware CMake does (dots dropped,
   * leading zeros stripped), 0 when it holds no digits */
  if (got > 1 || (reply[0] == USBSID_VERSION && reply[1] != 0)) {  /* reply[] is zeroed, got == 1 reads reply[1] == 0 */
    int v = 0;
    const int slen = (got > 2) ? (reply[1] < got - 2 ? reply[1] : got - 2) : 0;
    for (int i = 0; i < slen; i++) {
      const unsigned char c = reply[2 + i];
      if (c >= '0' && c <= '9') v = v * 10 + (c - '0');
      else if (c != '.') break;  /* Stop at a suffix such as "-rc1" */
    }
    fwversion = (v < US_FW_VERSION_BATCH_READ) ? v : 0;  /* Never claim a new version from a string reply */
    return fwversion;
  }
  fwversion = reply[0];
  return fwversion;
}

/**
 * @brief Upload a tune to the onboard emulator, returns data bytes sent or -1 on failure
 *
 * Sends UPLOAD_SID_START, the data in 62 byte UPLOAD_SID_DATA packets,
 * UPLOAD_SID_END and UPLOAD_SID_SIZE, 64 bytes per packet
 *
 * @param data file contents
 * @param len file size in bytes
 * @param filetype UPLOAD_FILE_SID, UPLOAD_FILE_PRG or UPLOAD_FILE_STDIN
 */
int USBSID_Class::USBSID_UploadTune(const uint8_t *data, size_t len, uint8_t filetype)
{
  if (!us_PortIsOpen || (data == NULL && len > 0)) return -1;
  unsigned char buff[UPLOAD_PACKET_SIZE] = {0};
  buff[0] = (COMMAND << 6 | CONFIG);
  buff[1] = UPLOAD_SID_START;
  buff[2] = filetype;
  if (USBSID_SendCommand(buff, UPLOAD_PACKET_SIZE) < 0) return -1;

  size_t sent = 0;
  while (sent < len) {
    size_t n = len - sent;
    if (n > UPLOAD_PAYLOAD_SIZE) n = UPLOAD_PAYLOAD_SIZE;
    memset(buff, 0, UPLOAD_PACKET_SIZE);
    buff[0] = (COMMAND << 6 | CONFIG);
    buff[1] = UPLOAD_SID_DATA;
    memcpy(&buff[2], data + sent, n);
    if (USBSID_SendCommand(buff, UPLOAD_PACKET_SIZE) < 0) return -1;
    sent += n;
  }

  memset(buff, 0, UPLOAD_PACKET_SIZE);
  buff[0] = (COMMAND << 6 | CONFIG);
  buff[1] = UPLOAD_SID_END;
  if (USBSID_SendCommand(buff, UPLOAD_PACKET_SIZE) < 0) return -1;

  memset(buff, 0, UPLOAD_PACKET_SIZE);
  buff[0] = (COMMAND << 6 | CONFIG);
  buff[1] = UPLOAD_SID_SIZE;
  buff[2] = (uint8_t)((len >> 8) & 0xFF);
  buff[3] = (uint8_t)(len & 0xFF);
  if (USBSID_SendCommand(buff, UPLOAD_PACKET_SIZE) < 0) return -1;
  return (int)sent;
}

/**
 * @brief Set the max play time of the uploaded tune, returns bytes sent or -1 on failure
 *
 * @param ms play time in milliseconds
 */
int USBSID_Class::USBSID_PlayerSetPlaytime(uint32_t ms)
{
  return USBSID_SendConfig(UPLOAD_SID_PLAYTIME,
    (uint8_t)((ms >> 24) & 0xFF), (uint8_t)((ms >> 16) & 0xFF),
    (uint8_t)((ms >> 8) & 0xFF), (uint8_t)(ms & 0xFF));
}

/**
 * @brief Load the uploaded tune and start it, returns bytes sent or -1 on failure
 *
 * @param subtune 0 based subtune
 */
int USBSID_Class::USBSID_PlayerLoad(uint8_t subtune)
{
  return USBSID_SendConfig(SID_PLAYER_TUNE, 0, subtune, 0, 0);
}

/**
 * @brief Send a parameterless player command, returns bytes sent or -1 on failure
 *
 * @param cmd SID_PLAYER_START, STOP, PAUSE, NEXT, PREV or TWO
 */
int USBSID_Class::USBSID_PlayerCommand(uint8_t cmd)
{
  return USBSID_SendConfig(cmd, 0, 0, 0, 0);
}

/**
 * @brief Mute or unmute a chip or voice of the onboard player, returns bytes sent or -1 on failure
 *
 * @param chip 1-4, 0 for all chips (voice must be 0)
 * @param voice 1-3, 0 for the whole chip
 * @param mute true to mute
 */
int USBSID_Class::USBSID_PlayerMute(uint8_t chip, uint8_t voice, bool mute)
{
  return USBSID_SendConfig(SID_PLAYER_MUTE, chip, voice, (uint8_t)(mute ? 1 : 0), 0);
}

/**
 * @brief Read the play time of the current onboard tune, returns milliseconds or -1 on failure
 */
long USBSID_Class::USBSID_PlayerTime(void)
{
  if (USBSID_SendConfig(SID_PLAYER_TIME, 0, 0, 0, 0) < 0) return -1;
  unsigned char reply[4] = {0};
  if (USBSID_ReadResponse(reply, 4) != 4) return -1;
  return (long)(((uint32_t)reply[0] << 24) | ((uint32_t)reply[1] << 16) |
                ((uint32_t)reply[2] << 8) | (uint32_t)reply[3]);
}

/**
 * @brief Read the onboard player mute state, returns true if state holds a valid reply
 *
 * @param state 5 byte destination, chip mask then voice masks of chips 1-4
 */
bool USBSID_Class::USBSID_PlayerMuted(uint8_t state[5])
{
  if (state == NULL) return false;
  if (USBSID_SendConfig(SID_PLAYER_MUTED, 0, 0, 0, 0) < 0) return false;
  return USBSID_ReadResponse(state, 5) == 5;
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
  while (transfer_out_pending) {  /* Previous packet in flight, e.g. after USBSID_Read() */
    struct timeval tv = {0, 1000};
    libusb_handle_events_timeout_completed(ctx, &tv, NULL);
  }
  memcpy(out_buffer, buff, len);
  transfer_out_pending = true;
  rc = libusb_submit_transfer(transfer_out);
  if (rc < 0) {
    transfer_out_pending = false;
    USBERR(stderr, "[USBSID] submit OUT failed %d - %s: %s\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    LIBUSB_CheckLost(rc);
    return;
  }
  /* Wait for this transfer, the stream event thread may handle its completion */
  while (transfer_out_pending) {
    struct timeval tv = {0, 1000};
    libusb_handle_events_timeout_completed(ctx, &tv, NULL);
  }
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
  while (transfer_out_pending) {  /* Previous packet in flight, e.g. after USBSID_Read() */
    struct timeval tv = {0, 1000};
    libusb_handle_events_timeout_completed(ctx, &tv, NULL);
  }
  memcpy(out_buffer, buff, len);
  transfer_out_pending = true;
  rc = libusb_submit_transfer(transfer_out);
  if (rc < 0) {
    transfer_out_pending = false;
    USBERR(stderr, "[USBSID] submit OUT failed %d - %s: %s\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    LIBUSB_CheckLost(rc);
    return;
  }
  /* Wait for this transfer, the stream event thread may handle its completion */
  while (transfer_out_pending) {
    struct timeval tv = {0, 1000};
    libusb_handle_events_timeout_completed(ctx, &tv, NULL);
  }
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
  if (!us_PortIsOpen || transfer_in == NULL || transfer_out == NULL) return 0;
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
    /* out buffer */
    memcpy(out_buffer, rw_buff, 2);
    transfer_out_pending = true;
    rc = libusb_submit_transfer(transfer_out);
    if (rc < 0) {
      transfer_out_pending = false;
      LIBUSB_CancelIn();
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
  if (!us_PortIsOpen || transfer_in == NULL || transfer_out == NULL) return 0;
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
    /* out buffer */
    writebuff[0] = (READ << 6);
    memcpy(out_buffer, writebuff, 3);
    transfer_out_pending = true;
    rc = libusb_submit_transfer(transfer_out);
    if (rc < 0) {
      transfer_out_pending = false;
      LIBUSB_CancelIn();
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
  if (!us_PortIsOpen || transfer_in == NULL || transfer_out == NULL) return 0;
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
    /* out buffer */
    writebuff[0] = (READ << 6);
    memcpy(out_buffer, writebuff, 3);
    transfer_out_pending = true;
    rc = libusb_submit_transfer(transfer_out);
    if (rc < 0) {
      transfer_out_pending = false;
      LIBUSB_CancelIn();
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


/* SID STREAM */
#ifdef USBSID_SID_STREAM

/**
 * @brief Start the SID memory stream on the stream interface, returns 0 or -1 on failure
 *
 * Claims STREAM_ITF, submits STREAM_TRANSFERS persistent IN transfers, starts
 * the stream event thread and sends STREAM_REQ_START. A running stream is
 * restarted with the new settings and an empty cache
 *
 * Reads US_FEATURES first, firmware lacking the SID stream
 * (US_FEATURE_SIDSTREAM) returns -1
 *
 * @param rate_hz 50, 60, 250, 500 or 1000, 0 for the firmware default (50)
 * @param sid_mask bit n = SID n+1, 0 for all configured SIDs
 */
int USBSID_Class::USBSID_StreamStart(int rate_hz, uint8_t sid_mask)
{
  if (!us_PortIsOpen || devh == NULL || device_lost) return -1;
  if (rate_hz < 0 || rate_hz > 0xFFFF) return -1;
  USBSID_StreamStop();
  features = -1;  /* Read fresh */
  const int f = USBSID_GetFeatures();
  if (f < 0 || !(f & US_FEATURE_SIDSTREAM)) return -1;
  int r = libusb_claim_interface(devh, STREAM_ITF);
  if (r < 0) {
    USBERR(stderr, "[USBSID] Stream interface %d not available: %d, %s: %s\n",
      STREAM_ITF, r, libusb_error_name(r), libusb_strerror((enum libusb_error)r));
    return -1;
  }
  stream_claimed = true;
  uint8_t info[STREAM_INFO_LEN];
  if (USBSID_StreamQuery(info) < 0) {
    USBSID_StreamStop();
    return -1;
  }
  /* START increments the firmware session id, frames of older ids are dropped */
  stream_session = (info[10] + 1) & 0xFF;
  stream_errors = 0;
  for (int i = 0; i < 128; i++) stream_regs[i].store(0, std::memory_order_relaxed);
  for (int i = 0; i < 4; i++) {
    stream_frames[i] = 0;
    stream_cycles[i] = 0;
    stream_processed[i] = 0;
    stream_tick[i] = 0;
    stream_flags[i] = 0;
  }

  stream_active = true;
  for (int i = 0; i < STREAM_TRANSFERS; i++) {
    uint8_t *buf = (uint8_t *)malloc(LEN_OUT_BUFFER);
    stream_xfer[i] = libusb_alloc_transfer(0);
    if (buf == NULL || stream_xfer[i] == NULL) {
      if (buf != NULL) free(buf);
      USBSID_StreamStop();
      return -1;
    }
    libusb_fill_bulk_transfer(stream_xfer[i], devh, STREAM_EP_IN, buf, LEN_OUT_BUFFER,
      usb_stream_in, this, 0);
    stream_xfer[i]->flags = LIBUSB_TRANSFER_FREE_BUFFER;  /* libusb_free_transfer() frees buf */
    stream_pending[i] = true;
    r = libusb_submit_transfer(stream_xfer[i]);
    if (r < 0) {
      stream_pending[i] = false;
      USBERR(stderr, "[USBSID] Error submitting stream transfer: %d, %s: %s\n",
        r, libusb_error_name(r), libusb_strerror((enum libusb_error)r));
      USBSID_StreamStop();
      return -1;
    }
  }

  stream_events = true;
  r = pthread_create(&stream_ptid, NULL, &USBSID_Class::_USBSID_StreamThread, this);
  if (r != 0) {
    stream_events = false;
    USBERR(stderr, "[USBSID] Stream thread can't be created :[%s]\n", strerror(r));
    USBSID_StreamStop();
    return -1;
  }
  stream_thread = true;

  uint8_t start[4] = {
    (uint8_t)(rate_hz & 0xFF), (uint8_t)((rate_hz >> 8) & 0xFF),
    (uint8_t)(sid_mask & 0x0F), 0  /* byte 3 reserved */
  };
  /* Cast to int, C++20 deprecates | between two different enum types */
  r = libusb_control_transfer(devh,
    (int)LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
    STREAM_REQ_START, 0, STREAM_ITF, start, sizeof(start), 1000);
  if (r < 0) {  /* Unsupported rate stalls */
    USBERR(stderr, "[USBSID] Stream start (rate %d) refused: %d, %s: %s\n",
      rate_hz, r, libusb_error_name(r), libusb_strerror((enum libusb_error)r));
    LIBUSB_CheckLost(r);
    USBSID_StreamStop();
    return -1;
  }
  return 0;
}

/**
 * @brief Stop the stream, cancel its transfers, release the interface, returns 0
 *
 * Safe to call when no stream runs
 */
int USBSID_Class::USBSID_StreamStop(void)
{
  if (!stream_claimed && !stream_thread) return 0;
  if (stream_active && devh != NULL && !device_lost) {
    int r = libusb_control_transfer(devh,
      (int)LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
      STREAM_REQ_STOP, 0, STREAM_ITF, NULL, 0, 1000);
    LIBUSB_CheckLost(r);
  }
  stream_active = false;
  stream_session = -1;
  for (int i = 0; i < STREAM_TRANSFERS; i++) {
    if (stream_xfer[i] != NULL && stream_pending[i]) {
      libusb_cancel_transfer(stream_xfer[i]);
    }
  }

  /* Wait for the cancel callbacks, bounded for an unresponsive device */
  const timestamp_t limit = std::chrono::steady_clock::now() + std::chrono::seconds(1);
  for (;;) {
    bool pending = false;
    for (int i = 0; i < STREAM_TRANSFERS; i++) pending = pending || stream_pending[i];
    if (!pending || std::chrono::steady_clock::now() >= limit) break;
    if (stream_thread) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } else {
      struct timeval tv = {0, 1000};
      libusb_handle_events_timeout_completed(ctx, &tv, NULL);
    }
  }
  if (stream_thread) {
    stream_events = false;
    pthread_join(stream_ptid, NULL);
    stream_thread = false;
  }

  /* A transfer still in flight stays allocated, its callback frees it */
  for (int i = 0; i < STREAM_TRANSFERS; i++) {
    if (stream_xfer[i] == NULL) continue;
    if (stream_pending[i]) {
      stream_xfer[i]->user_data = NULL;
    } else {
      libusb_free_transfer(stream_xfer[i]);
    }
    stream_xfer[i] = NULL;
    stream_pending[i] = false;
  }
  if (stream_claimed && devh != NULL) {
    libusb_release_interface(devh, STREAM_ITF);
  }
  stream_claimed = false;
  return 0;
}

/**
 * @brief Check whether the stream delivers frames, returns true when started and at least one IN transfer is alive
 */
bool USBSID_Class::USBSID_StreamRunning(void)
{
  if (!stream_active) return false;
  for (int i = 0; i < STREAM_TRANSFERS; i++) {
    if (stream_pending[i]) return true;
  }
  return false;
}

/**
 * @brief Read the firmware stream state (STREAM_REQ_INFO), returns 0 or -1 on failure or firmware lacking the stream
 *
 * [0] format version, [1] frame size, [2-3] max rate LE, [4] SID count,
 * [5] bit 0 running, [6-7] rate LE, [8] active SID mask, [9] reserved,
 * [10] session id, [12-15] frames dropped LE
 *
 * @param info receives STREAM_INFO_LEN bytes
 */
int USBSID_Class::USBSID_StreamQuery(uint8_t info[STREAM_INFO_LEN])
{
  if (!us_PortIsOpen || devh == NULL || info == NULL) return -1;
  memset(info, 0, STREAM_INFO_LEN);
  int r = libusb_control_transfer(devh,
    (int)LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
    STREAM_REQ_INFO, 0, STREAM_ITF, info, STREAM_INFO_LEN, 1000);
  if (r < 0) {
    LIBUSB_CheckLost(r);
    return -1;
  }
  return (r >= 11) ? 0 : -1;
}

/**
 * @brief Last streamed value of a register, no USB traffic, returns 0-255, -1 when no frame of that SID arrived yet
 *
 * $00-$18 hold the last written value (clone chips also the last read value)
 * $19-$1C hold the last read value, age is in USBSID_GetStreamState()
 *
 * @param reg register, bits 5-6 select the SID as for USBSID_Write()
 */
int USBSID_Class::USBSID_ReadCached(uint8_t reg)
{
  if (stream_frames[(reg >> 5) & 0x03].load(std::memory_order_acquire) == 0) return -1;
  return stream_regs[reg & 0x7F].load(std::memory_order_relaxed);
}

/**
 * @brief Copy the streamed registers of all 4 SIDs, returns bit n set for each SID n with data, -1 for a NULL out
 *
 * Registers of one SID can mix two consecutive frames
 *
 * @param out receives 128 bytes, SID n at n * 32, 0 for SIDs with no data
 */
int USBSID_Class::USBSID_GetSidMemory(uint8_t out[128])
{
  if (out == NULL) return -1;
  int mask = 0;
  for (int sid = 0; sid < 4; sid++) {
    if (stream_frames[sid].load(std::memory_order_acquire) != 0) mask |= (1 << sid);
  }
  for (int i = 0; i < 128; i++) out[i] = stream_regs[i].load(std::memory_order_relaxed);
  return mask;
}

/**
 * @brief Info of the latest frame of one SID, returns false for an invalid sid or a NULL state
 *
 * @param sid SID 0-3 of this board
 * @param state receives the frame info, frames 0 when no data
 */
bool USBSID_Class::USBSID_GetStreamState(int sid, USBSID_StreamState *state)
{
  if (sid < 0 || sid > 3 || state == NULL) return false;
  state->frames = stream_frames[sid].load(std::memory_order_acquire);
  state->tick = stream_tick[sid];
  state->flags = stream_flags[sid];
  state->cycles = stream_cycles[sid];
  state->processed = stream_processed[sid];
  return true;
}

/**
 * @brief Set the raw frame callback
 *
 * @param cb called for each accepted frame in the stream event thread, NULL to remove
 * @param user passed to cb
 */
void USBSID_Class::USBSID_SetStreamCallback(USBSID_StreamCallback cb, void *user)
{
  pthread_mutex_lock(&stream_cb_mutex);
  stream_cb = cb;
  stream_cb_user = user;
  pthread_mutex_unlock(&stream_cb_mutex);
}

/**
 * @brief Store one received frame in the cache, call the frame callback
 *
 * @param f frame
 * @param len received length
 */
void USBSID_Class::USBSID_StreamFrame(const uint8_t *f, int len)
{
  if (len != STREAM_FRAME_SIZE || f[0] != STREAM_FRAME_SID) return;
  if ((int)f[48] != stream_session) return;  /* Older session id or not started */
  const int sid = f[1] & 0x03;
  for (int r = 0; r < 32; r++) {
    stream_regs[sid * 32 + r].store(f[12 + r], std::memory_order_relaxed);
  }
  stream_tick[sid] = (uint16_t)(f[2] | (f[3] << 8));
  stream_flags[sid] = (uint8_t)(f[1] & 0xE0);
  stream_cycles[sid] = (uint32_t)f[4] | ((uint32_t)f[5] << 8) | ((uint32_t)f[6] << 16) | ((uint32_t)f[7] << 24);
  stream_processed[sid] = (uint32_t)f[8] | ((uint32_t)f[9] << 8) | ((uint32_t)f[10] << 16) | ((uint32_t)f[11] << 24);
  stream_frames[sid].fetch_add(1, std::memory_order_release);
  pthread_mutex_lock(&stream_cb_mutex);
  if (stream_cb != NULL) stream_cb(f, len, stream_cb_user);
  pthread_mutex_unlock(&stream_cb_mutex);
}

/**
 * @brief Stream event thread, handles libusb events while the stream runs
 *
 * Completes the async transfers of the driver while it runs
 */
void* USBSID_Class::USBSID_StreamThread(void)
{
  #ifdef _GNU_SOURCE
  pthread_setname_np(pthread_self(), "USBSID Stream");
  #endif
  while (stream_events) {
    struct timeval tv = {0, 20000};
    libusb_handle_events_timeout_completed(ctx, &tv, NULL);
  }
  return NULL;
}
#endif /* USBSID_SID_STREAM */


/* THREADING */

void* USBSID_Class::USBSID_Thread(void)
{ /* Only starts when threaded == true */
  USBDBG(stdout, "[USBSID] Thread starting\n");
  #ifdef _GNU_SOURCE
  pthread_setname_np(pthread_self(), "USBSID Thread");
  #endif
  if (withcycles) {
    USBDBG(stdout, "[USBSID] Thread with cycles\n");
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
    /* RingPut runs inside us_mutex and signals us_cond before unlocking
     * This wakes the thread when new data is put into the ring
     * The timeout is a safety net for a missed wakeup
     * USBSID_SendThreadBuffer() releases us_mutex during USB I/O */
    if (run_thread == 1
        && flush_buffer != 1  /* Rest of a flush still to send */
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
  USBDBG(stdout, "[USBSID] Thread finished\n");
  pthread_mutex_unlock(&us_mutex);
  us_thread--;
  pthread_exit(NULL);
  return NULL;
}

int USBSID_Class::USBSID_InitThread(void)
{
  USBDBG(stdout, "[USBSID] Init Thread start\n");
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
  USBDBG(stdout, "[USBSID] Stop thread\n");
  if (USBSID_IsRunning() == 1) {
    USBDBG(stdout, "[USBSID] Set thread exit = 1\n");
    pthread_mutex_lock(&us_mutex);
    run_thread = flush_buffer = 0;
    pthread_cond_signal(&us_cond);
    pthread_mutex_unlock(&us_mutex);
    pthread_join(us_ptid, NULL);  /* joinable, the thread does not detach itself */
    USBDBG(stdout, "[USBSID] Thread joined\n");
    threaded = withcycles = false;
    USBSID_DeInitRingBuffer(); /* after the join, the thread reads the ring until it exits */
    while (us_thread > 0) {};
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
  USBDBG(stdout, "[USBSID] Restart thread (%d)\n", USBSID_IsRunning());
  /* First check if not already running */
  USBSID_StopThread();
  /* Stop any active transfers */
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
  USBDBG(stdout, "[USBSID] Enable thread (%d)\n", USBSID_IsRunning());
  if (USBSID_IsRunning() != 1) {
    USBSID_InitThread();
  }
  return;
}

void USBSID_Class::USBSID_DisableThread(void)
{
  if (!us_PortIsOpen) return;
  USBDBG(stdout, "[USBSID] Disable thread (%d)\n", USBSID_IsRunning());
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
{ /* The ringbuffer has no overflow protection, writing faster than the thread
   * sends overwrites unsent data, use this for backpressure
   * Read without the lock, an estimate only. One slot stays unused */
  if (us_ringbuffer.is_allocated != 1) return 0;
  const int r = us_ringbuffer.ring_read;
  const int w = us_ringbuffer.ring_write;
  return ring_size - 1 - ((w - r + ring_size) % ring_size);
}

int USBSID_Class::USBSID_RingDiff()
{ /* Bytes waiting in the ring, counted across the wrap-around */
  return (us_ringbuffer.ring_write - us_ringbuffer.ring_read + ring_size) % ring_size;
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

  /* A flush is a deadline, all that waits in the ring goes in this packet
   * Runs on the driver thread, USBSID_Flush() only raises flush_buffer
   * More waiting than one packet holds keeps flush_buffer raised and the
   * following thread loop sends the rest */
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
    /* Rest goes out on the following thread loop,
       a flush raised during the send stays raised */
    if (waiting >= rec) flush_buffer = 1;
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
{ /* Batch variant of USBSID_WriteRingCycled(), items holds count writes of
   * 4 bytes (reg, val, cycles hi, cycles lo)
   * One lock per batch, the driver thread holds us_mutex while it sends */
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

  if (flush_buffer == 1) {
    /* Fill the packet from the ring before sending */
    USBSID_FlushBuffer();
  } else if (buffer_pos == 61  /* >= 61 || >= 4 */
      || buffer_pos == len_out_buffer) {
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
  if (flush_buffer == 1) {
    /* Fill the packet from the ring before sending */
    USBSID_FlushBuffer();
  } else if (buffer_pos == 63  /* >= 61 || >= 4 */
    || buffer_pos == len_out_buffer) {
    thread_buffer[0] = (uint8_t)((WRITE << 6) | (buffer_pos - 1));
    USBSID_SendThreadBuffer();
  }
  return;
}

void USBSID_Class::USBSID_SendThreadBuffer(void)
{ /* Driver thread only, us_mutex held on entry and on return
   * us_mutex is released during USB I/O, producers only wait on ring access
   * thread_buffer and out_buffer are only touched by the driver thread */
  const int len = buffer_pos;
  buffer_pos = 1;
  pthread_mutex_unlock(&us_mutex);
  while (transfer_out_pending && run_thread == 1) {  /* Previous packet in flight */
    struct timeval tv = {0, 500};  // 0.5 ms
    libusb_handle_events_timeout_completed(ctx, &tv, NULL);
  }
  if (!transfer_out_pending && transfer_out != NULL) {  /* NULL after LIBUSB_StopTransfers() */
    memset(out_buffer, 0, len_out_buffer);
    memcpy(out_buffer, thread_buffer, len);
    transfer_out_pending = true;
    const int err = libusb_submit_transfer(transfer_out);
    if (err < 0) {
      transfer_out_pending = false;
      out_failures++;
      LIBUSB_CheckLost(err);
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
{ /* Driver thread only, us_mutex held on entry and on return
   * Blocks on libusb events with us_mutex released */
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
  USBDBG(stdout, "[USBSID] Open device\n");
  /* Count the devices in the device list */
  struct libusb_device **devs;
  ssize_t cnt = libusb_get_device_list(ctx, &devs);
  if (cnt < 0) {
    rc = (int)cnt;
    USBERR(stderr, "[USBSID] Error listing USB devices: %d %s: %s\n",
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

  /* Find the target wanted by serial and assign the index requested */
  for (ssize_t i = 0; i < cnt; i++) {
    struct libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(devs[i], &desc) < 0) continue;
    if (desc.idVendor != VENDOR_ID || desc.idProduct != PRODUCT_ID) continue;

    if (!want_serial.empty()) { /* If requested by serial number */
      /* Reading a serial requires an open handle
       * Probe each match and close again on a miss */
      struct libusb_device_handle *probe = NULL;
      if (libusb_open(devs[i], &probe) == 0) {
        /* Read the serialnumber */
        std::string serial;
        if (desc.iSerialNumber) {
          unsigned char sbuf[64] = {0};
          int slen = libusb_get_string_descriptor_ascii(probe, desc.iSerialNumber, sbuf, sizeof(sbuf) - 1);
          if (slen > 0) serial.assign((char*)sbuf, slen);
        }
        /* Is it the one wanted? */
        if (serial == want_serial) {
          devh = probe;
          opened_serial = serial;
          target = devs[i];
          break; /* Break out of the loop if this is the one wanted */
        }
        /* Close the device if not the one wanted */
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
    USBERR(stderr, "[USBSID] Error, no matching USBSID-Pico found (serial='%s' index=%d): %d\n",
      want_serial.empty() ? "<any>" : want_serial.c_str(), want_index, rc);
    libusb_free_device_list(devs, 1);
    return rc;
  }

  /* Validation */
  if (!devh) {
    rc = libusb_open(target, &devh);
    if (rc < 0 || !devh) {
      USBERR(stderr, "[USBSID] Error opening USB device: %d %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
      libusb_free_device_list(devs, 1);
      return rc;
    }
  }

  /* No errors? Then clear the device list */
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

  /* On macOS the IOKit CDC driver reclaims interfaces unless auto-detach is enabled
   * Auto-detach makes libusb detach/reattach the kernel driver
   * around libusb_claim_interface / libusb_release_interface */
  rc = libusb_set_auto_detach_kernel_driver(devh, 1);
  if (rc == LIBUSB_ERROR_NOT_SUPPORTED) {
    rc = 0;
  } else if (rc < 0) {
    USBERR(stderr, "[USBSID] Error setting auto detach kernel driver: %d %s: %s\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
  }
  return rc;
}

void USBSID_Class::LIBUSB_CloseDevice(void)
{
  USBDBG(stdout, "[USBSID] Close device\n");
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
    /* Device already gone after reboot (e.g. bootloader) is expected */
    if (rc < 0 && rc != LIBUSB_ERROR_NOT_FOUND && rc != LIBUSB_ERROR_NO_DEVICE) {
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
  USBDBG(stdout, "[USBSID] Detach kernel driver\n");
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
      USBERR(stderr, "[USBSID] Error claiming interface: %d, %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
      rc = -1;
      break;
    }
  }
  return rc;
}

int USBSID_Class::LIBUSB_ConfigureDevice(void)
{
  USBDBG(stdout, "[USBSID] Configure device\n");
#ifdef USE_VENDOR_ITF
    /* again macOS needs this */
    rc = libusb_set_interface_alt_setting(devh, 4, 0);
    if (rc < 0) {  /* not fatal, report only */
      USBERR(stderr, "[USBSID] Error setting alt setting 0 on interface 4: %d, %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    }
    rc = libusb_control_transfer(devh, 0x21, 0x22, 0x01, 4, NULL, 0, 1000);
    if (rc < 0) {  /* should return 0 or higher */
      USBERR(stderr, "[USBSID] Error configuring line state on interface 4 during control transfer: %d, %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
      rc = -1;
      return rc;
    }
#else
  /* Start configuring the device:
   * set line state */
  rc = libusb_control_transfer(devh, 0x21, 0x22, ACM_CTRL_DTR | ACM_CTRL_RTS, 0, NULL, 0, 0);
  if (rc < 0) {  /* should return 0 or higher */
    USBERR(stderr, "[USBSID] Error configuring line state during control transfer: %d, %s: %s\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    rc = -1;
    return rc;
  }

  /* set line encoding, required but not used for CDC */
  rc = libusb_control_transfer(devh, 0x21, 0x20, 0, 0, encoding, sizeof(encoding), 1000);
  if (rc < 0 || rc != 7) {  /* should return 7 for the encoding size */
    USBERR(stderr, "[USBSID] Error configuring line encoding during control transfer: %d, %s: %s\n",
      rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    rc = -1;
    return rc;
  }
#endif
  return rc;
}

/**
 * @brief Bulk IN read of up to len bytes into buff, returns the libusb_bulk_transfer() result
 *
 * The request is rounded up to a multiple of LEN_IN_BUFFER plus
 * LEN_IN_XFER - LEN_IN_BUFFER bytes of room, on CDC (Linux, Windows)
 * both are 1 and the request stays len
 *
 * Vendor interface on macOS (LEN_IN_BUFFER 64, LEN_IN_XFER 128):
 * the request must be at least the 64 byte max packet size, a shorter
 * request ends in LIBUSB_ERROR_OVERFLOW and halts the endpoint, all reads
 * fail with LIBUSB_ERROR_PIPE until the halt is cleared
 * An exact 64 byte reply is followed by a zero length packet, the extra
 * room lets that packet end this transfer, else the following read gets it
 * and all later replies arrive one read late
 *
 * @param buff destination, receives at most len bytes
 * @param len number of bytes wanted
 * @param actual_length set to the number of bytes copied into buff
 */
int USBSID_Class::LIBUSB_ReadIn(unsigned char *buff, size_t len, int *actual_length)
{
  size_t req = ((len + LEN_IN_BUFFER - 1) / LEN_IN_BUFFER) * LEN_IN_BUFFER;
  if (req == 0) req = LEN_IN_BUFFER;
  req += LEN_IN_XFER - LEN_IN_BUFFER;  /* room for the zero length packet */
  unsigned char packet[LEN_IN_XFER];
  std::vector<unsigned char> large;
  unsigned char *in = packet;
  if (req > LEN_IN_XFER) {
    large.resize(req);
    in = large.data();
  }
  int got = 0;
  int ret = libusb_bulk_transfer(devh, EP_IN_ADDR, in, (int)req, &got, LIBUSB_TIMEOUT);
  if (ret == LIBUSB_ERROR_PIPE || ret == LIBUSB_ERROR_OVERFLOW) {
    libusb_clear_halt(devh, EP_IN_ADDR);  /* keep the following read working */
  }
  LIBUSB_CheckLost(ret);
  if (got > (int)len) got = (int)len;
  if (got > 0 && buff != in) memcpy(buff, in, (size_t)got);
  *actual_length = got;
  return ret;
}

/**
 * @brief Discard stale IN data left by an earlier host process
 *
 * Open and reconnect only, runs synchronously before the async transfers
 * and any command, short timeout (LIBUSB_TIMEOUT is 0 (infinite) on CDC)
 * Drains EP_IN_ADDR, the CDC IN endpoint or the vendor IN endpoint on macOS
 */
void USBSID_Class::LIBUSB_DrainIn(void)
{
  unsigned char junk[LEN_IN_XFER > 128 ? LEN_IN_XFER : 128];
  for (int i = 0; i < 8; i++) {
    int got = 0;
    int ret = libusb_bulk_transfer(devh, EP_IN_ADDR, junk, (int)sizeof(junk), &got, 15);
    if (ret == LIBUSB_ERROR_TIMEOUT) break;
    if (ret == LIBUSB_ERROR_PIPE || ret == LIBUSB_ERROR_OVERFLOW) {
      libusb_clear_halt(devh, EP_IN_ADDR);
      continue;
    }
    if (ret != 0) break;  /* no device or other hard error */
    USBDBG(stdout, "[USBSID] Drained %d stale IN bytes\n", got);
  }
}

/**
 * @brief Cancel the pending async IN transfer and wait for its callback
 *
 * Bounded wait, on an unresponsive device the transfer stays pending
 * and a later submit returns LIBUSB_ERROR_BUSY
 */
void USBSID_Class::LIBUSB_CancelIn(void)
{
  if (transfer_in == NULL || !transfer_in_pending) return;
  if (libusb_cancel_transfer(transfer_in) != 0) {  /* Not in flight anymore */
    transfer_in_pending = false;
    return;
  }
  const timestamp_t limit = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
  while (transfer_in_pending && std::chrono::steady_clock::now() < limit) {
    struct timeval tv = {0, 1000};
    libusb_handle_events_timeout_completed(ctx, &tv, NULL);
  }
}

/**
 * @brief Mark the device lost on a missing device or repeated out failures
 *
 * @param error libusb error code or transfer status, negative for an error code
 */
void USBSID_Class::LIBUSB_CheckLost(int error)
{
  if (error == LIBUSB_ERROR_NO_DEVICE || out_failures >= MAX_OUT_FAILURES) {
    if (!device_lost) {
      USBERR(stderr, "[USBSID] Device lost, close and open the device again\n");
    }
    device_lost = true;
  }
}

void USBSID_Class::LIBUSB_InitOutBuffer(void)
{
  USBDBG(stdout, "[USBSID] Init out buffers\n");
  out_buffer = libusb_dev_mem_alloc(devh, len_out_buffer);
  if (out_buffer == NULL) {
    USBDBG(stdout, "[USBSID] libusb_dev_mem_alloc failed on out_buffer, allocating with malloc\n");
    out_buffer = us_alloc(2 * len_out_buffer, (sizeof(uint8_t)) * len_out_buffer);
  } else {
    out_buffer_dma = true;
  }
  USBDBG(stdout, "[USBSID] Alloc out_buffer complete\n");
  transfer_out = libusb_alloc_transfer(0);
  USBDBG(stdout, "[USBSID] Alloc transfer_out complete\n");
  libusb_fill_bulk_transfer(transfer_out, devh, EP_OUT_ADDR, out_buffer, len_out_buffer, usb_out, this, LIBUSB_TIMEOUT);
  USBDBG(stdout, "[USBSID] libusb_fill_bulk_transfer transfer_out complete\n");

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
  USBDBG(stdout, "[USBSID] Free out buffers\n");
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
  USBDBG(stdout, "[USBSID] Init in buffers\n");
  in_buffer = libusb_dev_mem_alloc(devh, LEN_IN_XFER);
  if (in_buffer == NULL) {
    USBDBG(stdout, "[USBSID] libusb_dev_mem_alloc failed on in_buffer, allocating with malloc\n");
    in_buffer = us_alloc(2 * LEN_IN_XFER, (sizeof(uint8_t)) * LEN_IN_XFER);
  } else {
    in_buffer_dma = true;
  }
  USBDBG(stdout, "[USBSID] Alloc in_buffer complete\n");
  transfer_in = libusb_alloc_transfer(0);
  USBDBG(stdout, "[USBSID] Alloc transfer_in complete\n");
  libusb_fill_bulk_transfer(transfer_in, devh, EP_IN_ADDR, in_buffer, LEN_IN_XFER, usb_in, this, LIBUSB_TIMEOUT);
  USBDBG(stdout, "[USBSID] libusb_fill_bulk_transfer transfer_in complete\n");

  if (result == NULL) {
    result = us_alloc(2 * LEN_IN_BUFFER, (sizeof(uint8_t)) * (LEN_IN_BUFFER));
  }
  return;
}

void USBSID_Class::LIBUSB_FreeInBuffer(void)
{
  USBDBG(stdout, "[USBSID] Free in buffers\n");
  if (in_buffer_dma) {
    rc = libusb_dev_mem_free(devh, in_buffer, LEN_IN_XFER);
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
  USBDBG(stdout, "[USBSID] Stopping transfers\n");

  /* Keep the pending flags, only the callbacks clear them
   * A transfer libusb cancels (timeout) returns NOT_FOUND with its callback still due */
  if (transfer_out) {
    rc = libusb_cancel_transfer(transfer_out);
    if (rc == 0) {
      transfer_out_pending = true;
    } else if (rc != LIBUSB_ERROR_NOT_FOUND) {
      USBERR(stderr, "[USBSID] Error cancel OUT %d - %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    }
  }

  if (transfer_in) {
    rc = libusb_cancel_transfer(transfer_in);
    if (rc == 0) {
      transfer_in_pending = true;
    } else if (rc != LIBUSB_ERROR_NOT_FOUND) {
      USBERR(stderr, "[USBSID] Error cancel IN %d - %s: %s\n",
        rc, libusb_error_name(rc), libusb_strerror((enum libusb_error)rc));
    }
  }

  /* Wait for the cancel callbacks, bounded for an unresponsive device */
  const timestamp_t limit = std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while ((transfer_out_pending || transfer_in_pending)
         && std::chrono::steady_clock::now() < limit) {
    struct timeval tv = {0, 1000};
    libusb_handle_events_timeout_completed(ctx, &tv, NULL);
  }

  /* A transfer still in flight stays allocated, libusb_close() walks the
   * in-flight list and locks each transfer. Detach it from this instance */
  if (transfer_out) {
    if (transfer_out_pending) {
      transfer_out->user_data = NULL;
    } else {
      libusb_free_transfer(transfer_out);
    }
    transfer_out = NULL;
  }

  if (transfer_in) {
    if (transfer_in_pending) {
      transfer_in->user_data = NULL;
    } else {
      libusb_free_transfer(transfer_in);
    }
    transfer_in = NULL;
  }
  transfer_out_pending = transfer_in_pending = false;
}

int USBSID_Class::LIBUSB_Setup(bool start_threaded, bool with_cycles)
{
  rc = read_completed = write_completed = -1;
  features = -1;  /* Firmware may differ from the last open */
  fwversion = -1;
  device_lost = false;
  out_failures = 0;
  threaded = start_threaded;
  withcycles = with_cycles;
  len_out_buffer = LEN_OUT_BUFFER;

  /* Initialize libusb */
  rc = libusb_init(&ctx);
  /* For LIBUSB version 1.0.27+ this is possible: */
  // rc = libusb_init_context(&ctx, /*options=NULL, /*num_options=*/0);
  if (rc != 0) {
    USBERR(stderr, "[USBSID] Error initializing libusb: %d %s: %s\n",
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

  LIBUSB_DrainIn();  /* before any async transfer or command */

  LIBUSB_InitOutBuffer();
  LIBUSB_InitInBuffer();

  if (rc < 0) {
    USBERR(stderr, "[USBSID] Error, could not open device: %d, %s: %s\n",
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
  /* Runs when devh is set, rc is not checked, a failed LIBUSB_Setup() or a transfer
   * error leaves rc < 0 with the device open and its interface claimed, on MacOS
   * that locks the device for retries and for all other processes */
  if (devh) {
#ifdef USBSID_SID_STREAM
    USBSID_StreamStop();
#endif /* USBSID_SID_STREAM */
    USBSID_StopThread();
    #ifdef US_MUTE_ON_EXIT
    USBSID_Mute();
    #endif
    #ifdef US_RESET_ON_EXIT
    USBSID_Reset();
    #endif
    LIBUSB_StopTransfers();  /* uses the pending flags to cancel and wait */
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
  USBDBG(stdout, "[USBSID] Closed USB device\n");
  return 0;
}

void LIBUSB_CALL USBSID_Class::usb_out(struct libusb_transfer *transfer)
{
  USBSID_Class *self = (USBSID_Class *)transfer->user_data;

  if (self == NULL) {  /* Detached by LIBUSB_StopTransfers(), owned by nobody */
    libusb_free_transfer(transfer);
    return;
  }

  /* Keep the transfer on error or cancel, a later write reuses it */
  if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
    self->rc = transfer->status;
    if (transfer->status != LIBUSB_TRANSFER_CANCELLED) {
      USBERR(stderr, "[USBSID] Warning: transfer out interrupted with status %d, %s: %s\n",
        transfer->status, libusb_error_name(transfer->status), libusb_strerror((enum libusb_error)transfer->status));
      self->out_failures++;
      self->LIBUSB_CheckLost(transfer->status == LIBUSB_TRANSFER_NO_DEVICE ? LIBUSB_ERROR_NO_DEVICE : LIBUSB_ERROR_IO);
    }
    self->transfer_out_pending = false;
    return;
  }

  self->out_failures = 0;

  if (transfer->actual_length != self->len_out_buffer) {
    USBERR(stderr, "[USBSID] Sent data length %d is different from the defined buffer length: %d or actual length %d\n",
      transfer->length, self->len_out_buffer, transfer->actual_length);
  }
  self->transfer_out_pending = false;
}

void LIBUSB_CALL USBSID_Class::usb_in(struct libusb_transfer *transfer)
{
  USBSID_Class *self = (USBSID_Class *)transfer->user_data;

  if (self == NULL) {  /* Detached by LIBUSB_StopTransfers(), owned by nobody */
    libusb_free_transfer(transfer);
    return;
  }

  /* Keep the transfer on error or cancel, a later read reuses it */
  if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
    self->rc = transfer->status;
    if (transfer->status != LIBUSB_TRANSFER_CANCELLED) {
      USBERR(stderr, "[USBSID] Warning: transfer in interrupted with status '%s'\n",
        libusb_error_name(transfer->status));
      if (transfer->status == LIBUSB_TRANSFER_NO_DEVICE) {
        self->LIBUSB_CheckLost(LIBUSB_ERROR_NO_DEVICE);
      }
    }
    self->transfer_in_pending = false;
    return;
  }

  if (transfer->actual_length > 0) {
    memcpy(self->result, self->in_buffer, 1);
  }

  self->read_completed = 1;
  self->transfer_in_pending = false;
}

#ifdef USBSID_SID_STREAM
void LIBUSB_CALL USBSID_Class::usb_stream_in(struct libusb_transfer *transfer)
{
  USBSID_Class *self = (USBSID_Class *)transfer->user_data;

  if (self == NULL) {  /* Detached by USBSID_StreamStop(), owned by nobody */
    libusb_free_transfer(transfer);
    return;
  }

  int idx = -1;
  for (int i = 0; i < STREAM_TRANSFERS; i++) {
    if (self->stream_xfer[i] == transfer) idx = i;
  }

  switch (transfer->status) {
    case LIBUSB_TRANSFER_COMPLETED:
      self->USBSID_StreamFrame(transfer->buffer, transfer->actual_length);
      break;
    case LIBUSB_TRANSFER_CANCELLED:
    case LIBUSB_TRANSFER_TIMED_OUT:
      break;
    case LIBUSB_TRANSFER_NO_DEVICE:
      self->stream_errors++;
      self->LIBUSB_CheckLost(LIBUSB_ERROR_NO_DEVICE);
      break;
    default:
      self->stream_errors++;
      break;
  }

  /* Resubmit while the stream runs, an errored transfer stays down */
  if (self->stream_active && !self->device_lost
      && (transfer->status == LIBUSB_TRANSFER_COMPLETED || transfer->status == LIBUSB_TRANSFER_TIMED_OUT)) {
    if (libusb_submit_transfer(transfer) == 0) return;
    self->stream_errors++;
  }
  if (idx >= 0) self->stream_pending[idx] = false;
}
#endif /* USBSID_SID_STREAM */

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
