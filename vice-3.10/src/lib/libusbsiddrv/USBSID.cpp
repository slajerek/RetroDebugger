/*
 * USBSID-Pico is a RPi Pico (RP2040/RP2350) based board for interfacing one
 * or two MOS SID chips and/or hardware SID emulators over (WEB)USB with your
 * computer, phone or ASID supporting player.
 *
 * USBSID.cpp
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

#include <libusb.h>
#include "USBSID.h"


using namespace USBSID_NS;
using namespace std;

#ifndef count_of
#define count_of(a) (sizeof(a)/sizeof(uint8_t))
#endif

static inline uint8_t* us_alloc(size_t alignment, size_t size)
{
#if defined(__US_LINUX_COMPILE)
  return (uint8_t*)aligned_alloc(alignment, size);
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

extern "C" {

/* USBSID */

USBSID_Class::USBSID_Class() :
  us_InstanceID(0),
  us_Found(0)
{
  USBDBG(stdout, "[USBSID] Driver init start\n");

  if (us_PortIsOpen && (instance == 0)) USBSID_GetNumSIDs();  /* Retrieve numsids on 2nd class init when port is open */
  if (instance >= numsids) instance = (numsids - 1);  /* Don't count above maximum sids the board has */
  us_InstanceID = ++instance;  /* Current object id */
  us_Initialised = true;
}

USBSID_Class::~USBSID_Class()
{
  USBDBG(stdout, "[USBSID] Driver de-init start\n");
  if (us_PortIsOpen)
    if (USBSID_Close() == 0) us_Initialised = false;
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
  if (!us_Initialised) return 0;
  int e = -1;
  if (rc >= 0) e = LIBUSB_Exit();
  if (rc != -1) USBERR(stderr, "Expected rc == -1, received: %d\n", rc);
  if (e != 0) USBERR(stderr, "Expected e == 0, received: %d\n", e);
  if (devh != NULL) USBERR(stderr, "Expected dev == NULL, received: %p", (void*)&devh);
  if (us_PortIsOpen) us_PortIsOpen = false;
  USBDBG(stdout, "[USBSID] De-init finished\n");
  return 0;
}

void USBSID_Class::USBSID_Pause(void)
{
  if (!us_Initialised) return;
  USBDBG(stdout, "[USBSID] Pause\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | PAUSE), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_Reset(void)
{
  if (!us_Initialised) return;
  USBDBG(stdout, "[USBSID] Reset\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | RESET_SID), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_ResetAllRegisters(void)
{
  if (!us_Initialised) return;
  USBDBG(stdout, "[USBSID] Reset All Registers\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | RESET_SID), 0x1, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_Mute(void)
{
  if (!us_Initialised) return;
  USBDBG(stdout, "[USBSID] Mute\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | MUTE), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_UnMute(void)
{
  if (!us_Initialised) return;
  USBDBG(stdout, "[USBSID] UnMute\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | UNMUTE), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_DisableSID(void)
{
  if (!us_Initialised) return;
  USBDBG(stdout, "[USBSID] DisableSID\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | DISABLE_SID), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_EnableSID(void)
{
  if (!us_Initialised) return;
  USBDBG(stdout, "[USBSID] EnableSID\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | ENABLE_SID), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_ClearBus(void)
{
  if (!us_Initialised) return;
  USBDBG(stdout, "[USBSID] ClearBus\r\n");
  unsigned char buff[3] = {(COMMAND << 6 | CLEAR_BUS), 0x0, 0x0};
  USBSID_SingleWrite(buff, 3);
  return;
}

void USBSID_Class::USBSID_SetClockRate(long clockrate_cycles, bool suspend_sids)
{
  if (!us_Initialised) return;
  for (uint8_t i = 0; i < 4; i++) {
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
      USBDBG(stdout, "[USBSID] Inverted CPU cycle duration in nanoseconds %.09f\n", us_InvCPUcycleDurationNanoSeconds);
      if (clk_retrieved == 0 || us_clkrate != cycles_per_sec) {
        uint8_t configbuff[6] = {(COMMAND << 6 | CONFIG), 0x50, i, (uint8_t)(suspend_sids == true ? 1 : 0), 0, 0};
        USBSID_SingleWrite(configbuff, 6);
      }
      return;
    }
  }
  return;
}

long USBSID_Class::USBSID_GetClockRate(void)
{
  if (!us_Initialised) return 0;
  if (clk_retrieved == 1) {
    us_InvCPUcycleDurationNanoSeconds = 1.0 / (ratio_t::den / (float)cycles_per_sec);
    us_CPUcycleDuration = ratio_t::den / (float)cycles_per_sec;
    return cycles_per_sec;
  } else if (clk_retrieved == 0) {
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

/* Socket config array
 * 0 = Initiator
 * 1 = Verification
 * 2 HiByte = socketOne enabled
 * 2 LoByte = socketOne dualsid
 * 3 HiByte = socketOne chipType
 * 3 LoByte = socketOne cloneType
 * 4 HiByte = socketOne sid1Type
 * 4 LoByte = socketOne sid2Type
 * 5 HiByte = socketTwo enabled
 * 5 LoByte = socketTwo dualsid
 * 6 HiByte = socketTwo chipType
 * 6 LoByte = socketTwo cloneType
 * 7 HiByte = socketTwo sid1Type
 * 7 LoByte = socketTwo sid2Type
 * 8 = socketTwo mirror socketOne
 * 9 = Terminator
 */
uint8_t* USBSID_Class::USBSID_GetSocketConfig(uint8_t socket_config[])
{
  if (!us_Initialised) return NULL;
  if (socketconfig == -1) {
    socketconfig = 1;
    uint8_t configbuff[6] = {(COMMAND << 6 | CONFIG), 0x37, 0, 0, 0, 0};
    USBSID_SingleWrite(configbuff, 6);
    uint8_t socket_buff[10];
    USBSID_SingleReadConfig(socket_buff, 10);
    if (socket_buff[0] == 0x37
      && socket_buff[1] == 0x7F
      && socket_buff[9] == 0xFF) {
      memcpy(socket_config, socket_buff, 10);
      return socket_config;
    } else {
      socketconfig = -1;
      return NULL;
    }
  } else {
    socketconfig = (socketconfig == 1 ? socketconfig : -1);
    return NULL;
  }
}

int USBSID_Class::USBSID_GetSocketNumSIDS(int socket, uint8_t socket_config[])
{
  if (!us_Initialised) return 0;
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
{ /* TODO: FINISH */
  if (!us_Initialised) return 0;
  return 0;
};

/* 0 = unknown, 1 = N/A, 2 = MOS8085, 3 = MOS6581, 4 = FMopl */
int USBSID_Class::USBSID_GetSocketSIDType1(int socket, uint8_t socket_config[])
{
  if (!us_Initialised) return 0;
  switch (socket) {
    case 1:
      if (((socket_config[2] & 0xF0) >> 4) == 1) {
        return ((socket_config[4] & 0xF0) >> 4);
      } else {
        return 1;
      }
      break;
    case 2:
      if (((socket_config[5] & 0xF0) >> 4) == 1) {
        return ((socket_config[7] & 0xF0) >> 4);
      } else {
        return 1;
      }
      break;
    default:
      return 1;
  }
};

/* 0 = unknown, 1 = N/A, 2 = MOS8085, 3 = MOS6581, 4 = FMopl */
int USBSID_Class::USBSID_GetSocketSIDType2(int socket, uint8_t socket_config[])
{
  if (!us_Initialised) return 0;
  switch (socket) {
    case 1:
      if ((((socket_config[2] & 0xF0) >> 4) == 1) && ((socket_config[2] & 0xF) == 1)) {
        return ((socket_config[4] & 0xF) >> 4);
      } else {
        return 1;
      }
      break;
    case 2:
      if ((((socket_config[5] & 0xF0) >> 4) == 1) && ((socket_config[5] & 0xF) == 1)) {
        return (socket_config[7] & 0xF);
      } else {
        return 1;
      }
      break;
    default:
      return 1;
  }
};

int USBSID_Class::USBSID_GetNumSIDs(void)
{
  if (!us_Initialised) return 0;
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
  if (!us_Initialised) return 0;
  if (fmoplsid == -1) {
    uint8_t configbuff[6] = {(COMMAND << 6 | CONFIG), 0x3A, 0, 0, 0, 0};
    USBSID_SingleWrite(configbuff, 6);
    fmoplsid = USBSID_SingleReadConfig(result, 1);
  }
  return (fmoplsid == 0 ? -1 : fmoplsid);
}

int USBSID_Class::USBSID_GetPCBVersion(void)
{
  if (!us_Initialised) return 0;
  if (pcbversion == -1) {
    uint8_t configbuff[6] = {(COMMAND << 6 | CONFIG), 0x81, 0x1, 0, 0, 0};
    USBSID_SingleWrite(configbuff, 6);
    pcbversion = USBSID_SingleReadConfig(result, 1);
  }
  return pcbversion;
}

void USBSID_Class::USBSID_SetStereo(int state)
{
  if (!us_Initialised) return;
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
  if (!us_Initialised) return;
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

void USBSID_Class::USBSID_SingleWrite(unsigned char *buff, int len)
{
  if (!us_Initialised) return;
  int actual_length = 0;
  if (libusb_bulk_transfer(devh, EP_OUT_ADDR, buff, len, &actual_length, 0) < 0) {
    USBERR(stderr, "[USBSID] Error while sending synchronous write buffer of length %d\n", actual_length);
  }
  return;
}

unsigned char USBSID_Class::USBSID_SingleRead(uint8_t reg)
{
  if (!us_Initialised) return 0;
  int actual_length;
  unsigned char buff[3] = {(READ << 6), reg, 0};
  if (libusb_bulk_transfer(devh, EP_OUT_ADDR, buff, 3, &actual_length, 0) < 0) {
    USBERR(stderr, "[USBSID] Error while sending write command for reading\n");
  }
  rc = libusb_bulk_transfer(devh, EP_IN_ADDR, result, 1, &actual_length, 0);
  if (rc == LIBUSB_ERROR_TIMEOUT) {
    USBERR(stderr, "[USBSID] Timeout error while reading (%d)\n", actual_length);
    return 0;
  } else if (rc < 0) {
    USBERR(stderr, "[USBSID] Error while waiting for char while reading\n");
    return 0;
  }
  return result[0];
}

unsigned char USBSID_Class::USBSID_SingleReadConfig(unsigned char *buff, int len)
{
  if (!us_Initialised) return 0;
  int actual_length;
  rc = libusb_bulk_transfer(devh, EP_IN_ADDR, buff, len, &actual_length, 0);
  if (rc == LIBUSB_ERROR_TIMEOUT) {
    USBERR(stderr, "[USBSID] Timeout error while reading (%d)\n", actual_length);
    return 0;
  } else if (rc < 0) {
    USBERR(stderr, "[USBSID] Error while waiting for char while reading\n");
    return 0;
  }
  return *buff;
}

/* ASYNCHRONOUS */

void USBSID_Class::USBSID_Write(unsigned char *buff, size_t len)
{
  if (!us_Initialised) return;
  if (threaded) {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n", __func__, threaded);
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
  if (!us_Initialised) return;
  if (threaded) {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n", __func__, threaded);
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
  if (!us_Initialised) return;
  if (threaded) {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n", __func__, threaded);
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
  if (!us_Initialised) return;
  if (threaded) {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n", __func__, threaded);
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
  if (!us_Initialised) return;
  if (threaded) {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n", __func__, threaded);
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
  if (!us_Initialised) return 0;
  if (threaded == 0) {  /* Reading not supported with threaded writes */
    read_completed = write_completed = 0;
    uint8_t rw_buff[2];
    rw_buff[0] = (READ << 6);
    rw_buff[1] = reg;
    memcpy(out_buffer, rw_buff, 2);
    libusb_submit_transfer(transfer_out);
    libusb_handle_events_completed(ctx, NULL);
    libusb_submit_transfer(transfer_in);
    libusb_handle_events_completed(ctx, &read_completed);
    return *result;
  } else {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n", __func__, threaded);
  }
  return 0xFF;
}

unsigned char USBSID_Class::USBSID_Read(unsigned char *writebuff)
{
  if (!us_Initialised) return 0;
  if (threaded == 0) {  /* Reading not supported with threaded writes */
    read_completed = write_completed = 0;
    writebuff[0] = (READ << 6);
    memcpy(out_buffer, writebuff, 3);
    libusb_submit_transfer(transfer_out);
    libusb_handle_events_completed(ctx, NULL);
    libusb_submit_transfer(transfer_in);
    libusb_handle_events_completed(ctx, &read_completed);
    return *result;
  } else {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n", __func__, threaded);
  }
  return 0xFF;
}

unsigned char USBSID_Class::USBSID_Read(unsigned char *writebuff, uint16_t cycles)
{
  if (!us_Initialised) return 0;
  if (threaded == 0) {  /* Reading not supported with threaded writes */
    USBSID_WaitForCycle(cycles);
    read_completed = 0;
    writebuff[0] = (READ << 6);
    memcpy(out_buffer, writebuff, 3);
    libusb_submit_transfer(transfer_out);
    libusb_handle_events_completed(ctx, NULL);
    libusb_submit_transfer(transfer_in);
    libusb_handle_events_completed(ctx, &read_completed);
    return *result;
  } else {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded (%d) is enabled\n", __func__, threaded);
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
    while ((us_ringbuffer.ring_read != us_ringbuffer.ring_write)
           && (USBSID_RingDiff() > diff_size)) {
      if (withcycles) {
        USBSID_RingPopCycled();
      } else {
        USBSID_RingPop();
      }
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
  USBSID_InitRingBuffer(ring_size, diff_size);
  pthread_mutex_lock(&us_mutex);
  us_thread++;
  pthread_mutex_unlock(&us_mutex);
  int error;
  error = pthread_create(&this->us_ptid, NULL, &this->_USBSID_Thread, NULL);
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
    run_thread = flush_buffer = 0;
    USBSID_DeInitRingBuffer();
    pthread_join(us_ptid, NULL);
    USBDBG(stdout, "[USBSID] Thread attached\r\n");
    threaded = withcycles = false;
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
  if (!us_Initialised) return;
  USBDBG(stdout, "[USBSID] Restart thread (%d)\r\n", USBSID_IsRunning());
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
  if (!us_Initialised) return;
  USBDBG(stdout, "[USBSID] Enable thread (%d)\r\n", USBSID_IsRunning());
  if (USBSID_IsRunning() != 1) {
    USBSID_InitThread();
  }
  return;
}

void USBSID_Class::USBSID_DisableThread(void)
{
  if (!us_Initialised) return;
  USBDBG(stdout, "[USBSID] Disable thread (%d)\r\n", USBSID_IsRunning());
  USBSID_StopThread();
  return;
}


/* RINGBUFFER */

void USBSID_Class::USBSID_ResetRingBuffer(void)
{
  us_ringbuffer.ring_read = us_ringbuffer.ring_write = 0;
  return;
}

void USBSID_Class::USBSID_SetBufferSize(int size)
{
  if (size >= min_ring_size)
    ring_size = size;
  else ring_size = min_ring_size;
  return;
}

void USBSID_Class::USBSID_SetDiffSize(int size)
{
  if (size >= min_diff_size)
    diff_size = size;
  else diff_size = min_diff_size;
  return;
}

void USBSID_Class::USBSID_InitRingBuffer(int buffer_size, int differ_size)
{ /* Init with variable settings */
  USBSID_SetBufferSize(buffer_size);
  USBSID_SetDiffSize(differ_size);
  USBSID_ResetRingBuffer();
  us_ringbuffer.ringbuffer = us_alloc(2 * ring_size, (sizeof(uint8_t)) * ring_size);
  USBDBG(stdout, "[USBSID] Init RingBuffer with size: %d and diffsize: %d\n",
     buffer_size, differ_size);
  return;
}

void USBSID_Class::USBSID_InitRingBuffer(void)
{ /* Init with default settings or with values set prior to calling this function */
  USBSID_SetBufferSize(ring_size);
  USBSID_SetDiffSize(diff_size);
  USBSID_ResetRingBuffer();
  us_ringbuffer.ringbuffer = us_alloc(2 * ring_size, (sizeof(uint8_t)) * ring_size);
  us_ringbuffer.is_allocated = 1;
  USBDBG(stdout, "[USBSID] Init RingBuffer with default size: %d and default diffsize: %d\n",
     ring_size, diff_size);
  return;
}

void USBSID_Class::USBSID_DeInitRingBuffer(void)
{
  USBSID_ResetRingBuffer();
  USBSID_SetBufferSize(min_ring_size);
  USBSID_SetDiffSize(min_diff_size);
  if (us_ringbuffer.is_allocated == 1) us_free(us_ringbuffer.ringbuffer);
  return;
}

void USBSID_Class::USBSID_RestartRingBuffer(void)
{
  if (!us_Initialised) return;
  USBSID_DeInitRingBuffer();
  USBSID_InitRingBuffer();
  return;
}

bool USBSID_Class::USBSID_IsHigher()
{
  return (us_ringbuffer.ring_read < us_ringbuffer.ring_write);
}

int USBSID_Class::USBSID_RingDiff()
{
  int d = (USBSID_IsHigher() ? (us_ringbuffer.ring_read - us_ringbuffer.ring_write) : (us_ringbuffer.ring_write - us_ringbuffer.ring_read));
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
{
  if (!us_Initialised) return;
  USBSID_SetFlush();
  USBSID_FlushBuffer();
  return;
}

void USBSID_Class::USBSID_SetFlush(void)
{
  flush_buffer = 1;
  return;
}


/* RINGBUFFER READS & WRITES */

void USBSID_Class::USBSID_FlushBuffer(void)
{
  if (threaded && flush_buffer == 1
    && ((withcycles == 1)
    ? (buffer_pos >= 5)
    : (buffer_pos >= 3))) {
    thread_buffer[0] = (withcycles == 1)
      ? (uint8_t)(CYCLED_WRITE << 6 | (buffer_pos - 1))
      : (uint8_t)(WRITE << 6 | (buffer_pos - 1));
    memcpy(out_buffer, thread_buffer, buffer_pos);
    buffer_pos = 1;
    flush_buffer = 0;
    libusb_submit_transfer(transfer_out);
    libusb_handle_events_completed(ctx, NULL);
    memset(thread_buffer, 0, 64);
    memset(out_buffer, 0, len_out_buffer);
  } else {
    flush_buffer = 0;
  }
  return;
}

void USBSID_Class::USBSID_WriteRing(uint8_t reg, uint8_t val)
{
  if (!us_Initialised) return;
  if (threaded && !withcycles) {
    USBSID_RingPut(reg);
    USBSID_RingPut(val);
  } else {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded = %d and withcycles = %d\n", __func__, threaded, withcycles);
  }
  return;
}

void USBSID_Class::USBSID_WriteRingCycled(uint8_t reg, uint8_t val, uint16_t cycles)
{
  if (!us_Initialised) return;
  if (threaded && withcycles) {
    USBSID_RingPut(reg);
    USBSID_RingPut(val);
    USBSID_RingPut((uint8_t)(cycles >> 8) & 0xFF);
    USBSID_RingPut((uint8_t)(cycles & 0xFF));
  } else {
    USBERR(stderr, "[USBSID] Function '%s' cannot be used when threaded = %d and withcycles = %d\n", __func__, threaded, withcycles);
  }
  return;
}

void USBSID_Class::USBSID_RingPopCycled(void)
{
  thread_buffer[buffer_pos++] = USBSID_RingGet();  /* register */
  thread_buffer[buffer_pos++] = USBSID_RingGet();  /* value */
  thread_buffer[buffer_pos++] = USBSID_RingGet();  /* n cycles high */
  thread_buffer[buffer_pos++] = USBSID_RingGet();  /* n cycles low */

  if (buffer_pos == 61  /* >= 61 || >= 4 */
      || buffer_pos == len_out_buffer
      || flush_buffer == 1) {
    flush_buffer = 0;
    thread_buffer[0] = (uint8_t)((CYCLED_WRITE << 6) | (buffer_pos - 1));
    memcpy(out_buffer, thread_buffer, buffer_pos);
    buffer_pos = 1;
    libusb_submit_transfer(transfer_out);
    libusb_handle_events_completed(ctx, NULL);
    memset(thread_buffer, 0, 64);
    memset(out_buffer, 0, len_out_buffer);
  }
  return;
}

void USBSID_Class::USBSID_RingPop(void)
{
  write_completed = 0;

  /* Ex: 0xD418 */
  thread_buffer[buffer_pos++] = USBSID_RingGet();  /* register */
  thread_buffer[buffer_pos++] = USBSID_RingGet();  /* value */
  if (buffer_pos == 63  /* >= 61 || >= 4 */
    || buffer_pos == len_out_buffer
    || flush_buffer == 1) {
    flush_buffer = 0;
    thread_buffer[0] = (uint8_t)((WRITE << 6) | (buffer_pos - 1));
    memcpy(out_buffer, thread_buffer, buffer_pos);
    buffer_pos = 1;
    libusb_submit_transfer(transfer_out);
    libusb_handle_events_completed(ctx, NULL);
    memset(thread_buffer, 0, 64);
    memset(out_buffer, 0, len_out_buffer);
  }
  return;
}


/* BUS */

uint8_t USBSID_Class::USBSID_Address(uint16_t addr)
{ /* Unused at the moment */
  enum {
    SIDUMASK = 0xFF00,
    SIDLMASK = 0xFF,
    SID1ADDR = 0xD400,
    SID1MASK = 0x1F,
    SID2ADDR = 0xD420,
    SID2MASK = 0x3F,
    SID3ADDR = 0xD440,
    SID3MASK = 0x5F,
    SID4ADDR = 0xD460,
    SID4MASK = 0x7F,
  };
  /* Set address for SID no# */
  /* D500, DE00 or DF00 is the second sid in SIDTYPE1, 3 & 4 */
  /* D500, DE00 or DF00 is the third sid in all other SIDTYPE */
  static uint8_t a;
  switch (addr) {
    case 0xD400 ... 0xD499:
      a = (uint8_t)(addr & SIDLMASK); /* $D400 -> $D479 1, 2, 3 & 4 */
      break;
    case 0xD500 ... 0xD599:
    case 0xDE00 ... 0xDF99:
      a = ((SID3ADDR | (addr & SID2MASK)) & SIDLMASK);
      break;
    default:
      a = (uint8_t)(addr & SIDLMASK);
      break;
  }
  return a;
}


/* TIMING AND CYCLES */

uint_fast64_t USBSID_Class::USBSID_CycleFromTimestamp(timestamp_t timestamp)
{
  if (!us_Initialised) return 0;
  USBSID_GetClockRate();  /* Make sure we use the right clockrate */
  us_InvCPUcycleDurationNanoSeconds = 1.0 / (ratio_t::den / (float)cycles_per_sec);
  auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp - m_StartTime);
  return (int_fast64_t)((double)nsec.count() * us_InvCPUcycleDurationNanoSeconds);
}

uint_fast64_t USBSID_Class::USBSID_WaitForCycle(uint_fast16_t cycles)
{ /* Returns the waited microseconds since last target time ~ not the actual cycles */
  if (!us_Initialised) return 0;
  USBSID_GetClockRate();  /* Make sure we use the right clockrate */
  timestamp_t now = std::chrono::high_resolution_clock::now();
  double dur = (double)cycles * us_CPUcycleDuration;  /* duration in nanoseconds */
  duration_t duration = (duration_t)(int_fast64_t)dur; /* equals dur but as chrono nanoseconds */
  auto target_time = m_LastTime + duration;  /* ns to wait since m_LastTime (now + duration for actual wait time) */
  // auto target_time = now + duration;  /* ns to wait since m_LastTime (now + duration for actual wait time) */
  auto target_delta = target_time - now;
  auto wait_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(target_delta);
  // auto wait_usec = std::chrono::duration_cast<std::chrono::microseconds>(target_delta);
  if (wait_nsec.count() > 0) {
      std::this_thread::sleep_for(wait_nsec);
  }
  m_LastTime = target_time;
  /* int_fast64_t waited_cycles = wait_usec.count(); */
  int_fast64_t waited_cycles = (int_fast64_t)((double)wait_nsec.count() * us_InvCPUcycleDurationNanoSeconds);
  // USBDBG(stdout, "[C] %ld [WC] %ld [us] %ld [ns] %ld [ts] %lu\n",
  //   cycles, waited_cycles, wait_usec.count(), wait_nsec.count(), USBSID_CycleFromTimestamp(now));
  // return (wait_nsec.count() > 0) ? waited_cycles : 0;
  return waited_cycles;
}


/* LIBUSB */

int USBSID_Class::LIBUSB_OpenDevice(void)
{
  USBDBG(stdout, "[USBSID] Open device\r\n");
  /* Look for a specific device and open it. */
  devh = libusb_open_device_with_vid_pid(ctx, VENDOR_ID, PRODUCT_ID);
  if (!devh) {
    rc = -1;
    USBERR(stderr, "[USBSID] Error opening USB device with VID & PID: %d %s: %s\r\n", rc, libusb_error_name(rc), libusb_strerror(rc));
  }
  return rc;
}

void USBSID_Class::LIBUSB_CloseDevice(void)
{
  USBDBG(stdout, "[USBSID] Close device\r\n");
  if (devh) {
    for (int if_num = 0; if_num < 2; if_num++) {
      if (libusb_kernel_driver_active(devh, if_num)) {
        rc = libusb_detach_kernel_driver(devh, if_num);
        USBERR(stderr, "[USBSID] Error, in libusb_detach_kernel_driver: %d, %s: %s\n", rc, libusb_error_name(rc), libusb_strerror(rc));
      }
      libusb_release_interface(devh, if_num);
    }
    libusb_close(devh);
  }
  return;
}

int USBSID_Class::LIBUSB_Available(uint16_t vendor_id, uint16_t product_id)
{
  struct libusb_device **devs;
  struct libusb_device *dev;
  size_t i = 0;
  int r;
  us_Available = false;
  us_Found = 0;

  if (libusb_get_device_list(ctx, &devs) < 0)
    return 0;

  while ((dev = devs[i++]) != NULL) {
    struct libusb_device_descriptor desc;
    r = libusb_get_device_descriptor(dev, &desc);
    if (r < 0)
      goto out;
    if (desc.idVendor == vendor_id && desc.idProduct == product_id) {
      us_Available = true;
      us_Found++;
      continue;
    }
  }
out:
  libusb_free_device_list(devs, 1);
  return (us_Available ? us_Found : 0);
}

int USBSID_Class::LIBUSB_DetachKernelDriver(void)
{
  USBDBG(stdout, "[USBSID] Detach kernel driver\r\n");
  /* As we are dealing with a CDC-ACM device, it's highly probable that
   * Linux already attached the cdc-acm driver to this device.
   * We need to detach the drivers from all the USB interfaces. The CDC-ACM
   * Class defines two interfaces: the Control interface and the
   * Data interface.
   */
  for (int if_num = 0; if_num < 2; if_num++) {
    if (libusb_kernel_driver_active(devh, if_num)) {
      libusb_detach_kernel_driver(devh, if_num);
    }
    rc = libusb_claim_interface(devh, if_num);
    if (rc < 0) {
      USBERR(stderr, "[USBSID] Error claiming interface: %d, %s: %s\r\n", rc, libusb_error_name(rc), libusb_strerror(rc));
      rc = -1;
      break;
    }
  }
  return rc;
}

int USBSID_Class::LIBUSB_ConfigureDevice(void)
{
  USBDBG(stdout, "[USBSID] Configure device\r\n");
  /* Start configuring the device:
   * set line state */
  rc = libusb_control_transfer(devh, 0x21, 0x22, ACM_CTRL_DTR | ACM_CTRL_RTS, 0, NULL, 0, 0);
  if (rc < 0) {  /* should return 0 or higher */
    USBERR(stderr, "[USBSID] Error configuring line state during control transfer: %d, %s: %s\r\n", rc, libusb_error_name(rc), libusb_strerror(rc));
    rc = -1;
    return rc;
  }

  /* set line encoding here */  // NOTE: NOT USED FOR CDC
  rc = libusb_control_transfer(devh, 0x21, 0x20, 0, 0, encoding, sizeof(encoding), 0);
  if (rc < 0 || rc != 7) {  /* should return 7 for the encoding size */
    USBERR(stderr, "[USBSID] Error configuring line encoding during control transfer: %d, %s: %s\r\n", rc, libusb_error_name(rc), libusb_strerror(rc));
    rc = -1;
    return rc;
  }
  return rc;
}

void USBSID_Class::LIBUSB_InitOutBuffer(void)
{
  USBDBG(stdout, "[USBSID] Init out buffers\r\n");
  out_buffer = libusb_dev_mem_alloc(devh, len_out_buffer);
  if (out_buffer == NULL) {
    USBDBG(stdout, "[USBSID] libusb_dev_mem_alloc failed on out_buffer, allocating with malloc\r\n");
    // out_buffer = (uint8_t*)aligned_alloc(2 * len_out_buffer, (sizeof(uint8_t)) * len_out_buffer);
    out_buffer = us_alloc(2 * len_out_buffer, (sizeof(uint8_t)) * len_out_buffer);
  } else {
    out_buffer_dma = true;
  }
  USBDBG(stdout, "[USBSID] Alloc out_buffer complete\r\n");
  transfer_out = libusb_alloc_transfer(0);
  USBDBG(stdout, "[USBSID] Alloc transfer_out complete\r\n");
  libusb_fill_bulk_transfer(transfer_out, devh, EP_OUT_ADDR, out_buffer, len_out_buffer, usb_out, NULL, 0);
  USBDBG(stdout, "[USBSID] libusb_fill_bulk_transfer transfer_out complete\r\n");

  if (thread_buffer == NULL) {
    // thread_buffer = (uint8_t*)aligned_alloc(2 * len_out_buffer, (sizeof(uint8_t)) * (len_out_buffer));
    thread_buffer = us_alloc(2 * len_out_buffer, (sizeof(uint8_t)) * (len_out_buffer));
  }
  if (write_buffer == NULL) {
    // write_buffer = (uint8_t*)aligned_alloc(2 * len_out_buffer, (sizeof(uint8_t)) * (len_out_buffer));
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
      USBERR(stderr, "[USBSID] Error, failed to free out_buffer DMA memory: %d, %s: %s\n", rc, libusb_error_name(rc), libusb_strerror(rc));
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
  libusb_fill_bulk_transfer(transfer_in, devh, EP_IN_ADDR, in_buffer, LEN_IN_BUFFER, usb_in, &read_completed, 0);
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
      USBERR(stderr, "[USBSID] Error, failed to free in_buffer DMA memory: %d, %s: %s\n", rc, libusb_error_name(rc), libusb_strerror(rc));
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
  if (transfer_out != NULL) {
    rc = libusb_cancel_transfer(transfer_out);
    libusb_free_transfer(transfer_out);
    if (rc < 0 && rc != -5) {
      USBERR(stderr, "[USBSID] Error, failed to cancel transfer %d - %s: %s\n", rc, libusb_error_name(rc), libusb_strerror(rc));
    }
  }

  if (transfer_in != NULL) {
    rc = libusb_cancel_transfer(transfer_in);
    libusb_free_transfer(transfer_in);
    if (rc < 0 && rc != -5) {
      USBERR(stderr, "[USBSID] Error, failed to cancel transfer %d - %s: %s\n", rc, libusb_error_name(rc), libusb_strerror(rc));
    }
  }
  return;
}

int USBSID_Class::LIBUSB_Setup(bool start_threaded, bool with_cycles)
{
  rc = read_completed = write_completed = -1;
  threaded = start_threaded;
  withcycles = with_cycles;
  len_out_buffer = LEN_OUT_BUFFER;
  write_buffer = us_alloc(2 * len_out_buffer, (sizeof(uint8_t)) * (len_out_buffer));
  thread_buffer = us_alloc(2 * len_out_buffer, (sizeof(uint8_t)) * (len_out_buffer));
  result = us_alloc(2 * LEN_IN_BUFFER, (sizeof(uint8_t)) * (LEN_IN_BUFFER));

  /* Initialize libusb */
  rc = libusb_init(&ctx);
  // rc = libusb_init_context(&ctx, /*options=NULL, /*num_options=*/0);  // NOTE: REQUIRES LIBUSB 1.0.27!!
  if (rc != 0) {
    USBERR(stderr, "[USBSID] Error initializing libusb: %d %s: %s\r\n", rc, libusb_error_name(rc), libusb_strerror(rc));
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
  #ifdef US_UNMUTE_ON_ENTRY
  USBSID_UnMute();
  #endif
  #ifdef US_RESET_ON_ENTRY
  USBSID_Reset();
  #endif
  #ifdef US_CLEARBUS_ON_ENTRY
  USBSID_ClearBus();
  #endif
  LIBUSB_InitOutBuffer();
  LIBUSB_InitInBuffer();

  if (rc < 0) {
    USBERR(stderr, "[USBSID] Error, could not open device: %d, %s: %s\r\n", rc, libusb_error_name(rc), libusb_strerror(rc));
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
    LIBUSB_StopTransfers();
    LIBUSB_FreeInBuffer();
    LIBUSB_FreeOutBuffer();
    LIBUSB_CloseDevice();
  }
  if (ctx) {
    libusb_exit(ctx);
  }

  rc = -1;
  devh = NULL;
  USBDBG(stdout, "[USBSID] Closed USB device\r\n");
  return 0;
}

void LIBUSB_CALL USBSID_Class::usb_out(struct libusb_transfer *transfer)
{
  if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
    rc = transfer->status;
    if (rc != LIBUSB_TRANSFER_CANCELLED) {
      USBERR(stderr, "[USBSID] Warning: transfer out interrupted with status %d, %s: %s\r", rc, libusb_error_name(rc), libusb_strerror(rc));
    }
    libusb_free_transfer(transfer);
    return;
  }

  if (transfer->actual_length != len_out_buffer) {
    USBERR(stderr, "[USBSID] Sent data length %d is different from the defined buffer length: %d or actual length %d\r", transfer->length, len_out_buffer, transfer->actual_length);
  }

  // BUG: Resubmit is shit for normal tunes but good for cycle exact digitunes, sigh...
  // if (threaded) libusb_submit_transfer(transfer_out);  /* Resubmit queue when finished */
  return;
}

void LIBUSB_CALL USBSID_Class::usb_in(struct libusb_transfer *transfer)
{
  read_completed = (*(int *)transfer->user_data);

  if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
    rc = transfer_in->status;
    if (rc != LIBUSB_TRANSFER_CANCELLED) {
      USBERR(stderr, "[USBSID] Warning: transfer in interrupted with status '%s'\r", libusb_error_name(rc));
    }
    libusb_free_transfer(transfer);
    return;
  }

  memcpy(result, in_buffer, 1);
  read_completed = 1;
  return;
}


} /* extern "C" */
