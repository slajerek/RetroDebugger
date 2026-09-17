/*
 * USBSID-Pico is a RPi Pico (RP2040/RP2350) based board for interfacing one
 * or two MOS SID chips and/or hardware SID emulators over (WEB)USB with your
 * computer, phone or ASID supporting player.
 *
 * USBSID.h
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

#ifndef _USBSID_H_
#define _USBSID_H_

#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__unix__) || defined(__APPLE__)
  #define __US_LINUX_COMPILE
#elif defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
  #define __US_WINDOWS_COMPILE
#endif

#if defined(__US_WINDOWS_COMPILE)
  #ifndef WINAPI
    #if defined(_ARM_)
      #define WINAPI
    #else
      #define WINAPI __stdcall
    #endif
  #endif
#endif

#ifndef LIBUSB_CALL
  #if defined(_WIN32) || defined(__CYGWIN__)
    #define LIBUSB_CALL WINAPI
  #else
    #define LIBUSB_CALL
  #endif
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#ifdef __cplusplus
  #include <cstdint>
  #include <cstdio>
  #include <cstdlib>
  #include <cstring>
  #include <chrono>
  #include <thread>
  #include <atomic>
#else
  #include <stdbool.h>
  #include <stdint.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <pthread.h>
  #include <stdatomic.h>
#endif


/* Optional driver start and driver exit commands
 *
 * Some players do weird things on start, choose
 * any of these to suit your own needs
 *
 */

// #define US_RESET_ON_ENTRY     /* Send Reset SID command on LIBUSB Entry */
// #define US_CLEARBUS_ON_ENTRY  /* Send Clear Bus command on LIBUSB Entry */
// #define US_UNMUTE_ON_ENTRY    /* Send UnMute SID command on LIBUSB Exit */

// #define US_MUTE_ON_EXIT       /* Send Mute SID command on LIBUSB Exit */
// #define US_RESET_ON_EXIT      /* Send Reset SID command on LIBUSB Exit */


/* Uncomment for debug logging */
// #define USBSID_DEBUG
#ifdef USBSID_DEBUG
  #define USBDBG(...) fprintf(__VA_ARGS__)
#else
  #define USBDBG(...) ((void)0)
#endif
#define USBERR(...) fprintf(__VA_ARGS__)

using namespace std;

/* Pre-define libusb structs */
struct libusb_context;
struct libusb_transfer;

namespace USBSID_NS
{
  /* pre-declaration for static functions */
  class USBSID_Class;

  /* LIBUSB/USBSID related */
  enum {
    VENDOR_ID      = 0xCAFE,
    PRODUCT_ID     = 0x4011,
    ACM_CTRL_DTR   = 0x01,
    ACM_CTRL_RTS   = 0x02,
    EP_OUT_ADDR    = 0x02,
    EP_IN_ADDR     = 0x82,
    LEN_IN_BUFFER  = 1,
    LEN_OUT_BUFFER = 64,
    #ifdef DEBUG_USBSID_MEMORY
    LEN_TMP_BUFFER = 4
    #endif
  };

  enum {
    /* BYTE 0 - top 2 bits */
    WRITE        =   0,   /*        0b0 ~ 0x00 */
    READ         =   1,   /*        0b1 ~ 0x40 */
    CYCLED_WRITE =   2,   /*       0b10 ~ 0x80 */
    COMMAND      =   3,   /*       0b11 ~ 0xC0 */
    /* BYTE 0 - lower 6 bits for byte count */
    /* BYTE 0 - lower 6 bits for Commands */
    PAUSE        =  10,   /*     0b1010 ~ 0x0A */
    UNPAUSE      =  11,   /*     0b1011 ~ 0x0B */
    MUTE         =  12,   /*     0b1100 ~ 0x0C */
    UNMUTE       =  13,   /*     0b1101 ~ 0x0D */
    RESET_SID    =  14,   /*     0b1110 ~ 0x0E */
    DISABLE_SID  =  15,   /*     0b1111 ~ 0x0F */
    ENABLE_SID   =  16,   /*    0b10000 ~ 0x10 */
    CLEAR_BUS    =  17,   /*    0b10001 ~ 0x11 */
    CONFIG       =  18,   /*    0b10010 ~ 0x12 */
    RESET_MCU    =  19,   /*    0b10011 ~ 0x13 */
    BOOTLOADER   =  20,   /*    0b10100 ~ 0x14 */
  };

  /* Thread related */
  static int run_thread;

  /* Fake C64 Memory */
  #ifdef DEBUG_USBSID_MEMORY
  static uint8_t sid_memory[0x20];
  static uint8_t sid_memory_changed[0x20];
  static uint16_t sid_memory_cycles[0x20];
  #endif

  /* LIBUSB related */
  static struct libusb_device_handle *devh = NULL;
  static struct libusb_transfer *transfer_out = NULL;  /* OUT-going transfers (OUT from host PC to USB-device) */
  static struct libusb_transfer *transfer_in = NULL;  /* IN-coming transfers (IN to host PC from USB-device) */
  static libusb_context *ctx = NULL;
  static bool in_buffer_dma = false;
  static bool out_buffer_dma = false;

  static bool threaded = false;
  static bool withcycles = false;
  static int rc, read_completed, write_completed;

  /* USB buffer related */
  static uint8_t * __restrict__ in_buffer;     /* incoming libusb will reside in this buffer */
  static uint8_t * __restrict__ out_buffer;    /* outgoing libusb will reside in this buffer */
  static uint8_t * __restrict__ thread_buffer; /* data to be transfered to the out_buffer will reside in this buffer */
  static uint8_t * __restrict__ write_buffer;  /* non async data will be written from this buffer */
  #ifdef DEBUG_USBSID_MEMORY
  static uint8_t * __restrict__ temp_buffer;   /* temp buffer for debug printing */
  #endif
  static uint8_t * __restrict__ result;        /* variable where read data is copied into */
  static int len_out_buffer;      /* changable variable for out buffer size */
  static int buffer_pos = 1;      /* current position of the out buffer */
  static int flush_buffer = 0;    /* flush buffer yes or no */

  /* Ringbuffer related */
  typedef struct {
    int ring_read;
    int ring_write;
    int is_allocated;
    uint8_t * __restrict__ ringbuffer;
  } ring_buffer_t;
  static ring_buffer_t us_ringbuffer;
  const int min_diff_size = 16;
  const int min_ring_size = 256;
  static int diff_size = 64;
  static int ring_size = 8192;

  /* Clock cycles per second
   * Clock speed: 0.985 MHz (PAL) or 1.023 MHz (NTSC)
   *
   * For some reason both 1022727 and 1022730 are
   * mentioned as NTSC clock cycles per second
   * Going for the rates specified by Vice it should
   * be 1022730, except in the link about raster time
   * on c64 wiki it's 1022727.
   * I chose to implement both, let's see how this
   * works out
   *
   * https://sourceforge.net/p/vice-emu/code/HEAD/tree/trunk/vice/src/c64/c64.h
   */

  /* Clock cycles per second */
  enum clock_speeds
  {
    DEFAULT = 1000000,  /* 1 MHz     = 1 us */
    PAL     = 985248,   /* 0.985 MHz = 1.014973 us */
    NTSC    = 1022727,  /* 1.023 MHz = 0.977778 us */
    DREAN   = 1023440,  /* 1.023 MHz = 0.977097 us */
    NTSC2   = 1022730,  /* 1.023 MHz = 0.977778 us */
  };
  /* Refreshrates (cycles) in microseconds */
  enum refresh_rates
  {
    HZ_DEFAULT = 20000,  /* 50Hz ~ 20000 == 20 us */
    HZ_EU      = 19950,  /* 50Hz ~ 20000 == 20 us    / 50.125Hz ~ 19.950124688279 exact */
    HZ_US      = 16715,  /* 60Hz ~ 16667 == 16.67 us / 59.826Hz ~ 16.715140574332 exact */
  };
  /* Rasterrates (cycles) in microseconds
   * Source: https://www.c64-wiki.com/wiki/raster_time
   *
   * PAL: 1 horizontal raster line takes 63 cycles
   * or 504 pixels including side borders
   * whole screen consists of 312 horizontal lines
   * for a frame including upper and lower borders
   * 63 * 312 CPU cycles is 19656 for a complete
   * frame update @ 985248 Hertz
   * 985248 / 19656 = approx 50.12 Hz frame rate
   *
   * NTSC: 1 horizontal raster line takes 65 cycles
   * whole screen consists of 263 rasters per frame
   * 65 * 263 CPU cycles is 17096 for a complete
   * frame update @ 985248 Hertz
   * 1022727 / 17096 = approx 59.83 Hz frame rate
   *
   */
  enum raster_rates
  {
    R_DEFAULT = 20000,  /* 20us  ~ fallback */
    R_EU      = 19656,  /* PAL:  63 cycles * 312 lines = 19656 cycles per frame update @  985248 Hz = 50.12 Hz frame rate */
    R_US      = 17096,  /* NTSC: 65 cycles * 263 lines = 17096 cycles per frame update @ 1022727 Hz = 59.83 Hz Hz frame rate */
  };
  static const enum clock_speeds clockSpeed[]   = { DEFAULT, PAL, NTSC, DREAN, NTSC2 };
  static const enum refresh_rates refreshRate[] = { HZ_DEFAULT, HZ_EU, HZ_US, HZ_US, HZ_US };
  static const enum raster_rates rasterRate[]   = { R_DEFAULT, R_EU, R_US, R_US, R_US };
  static long cycles_per_sec    = DEFAULT;     /* default @ 1000000 */
  static long cycles_per_frame  = HZ_DEFAULT;  /* default @ 20000 */
  static long cycles_per_raster = R_DEFAULT;   /* default @ 20000 */
  static int clk_retrieved = 0;
  static long us_clkrate = 0;
  static int numsids = 0;
  static int fmoplsid = -1;
  static int pcbversion = -1;
  static int socketconfig = -1;

  /* Object related */
  static bool us_Initialised = false;
  static bool us_Available = false;
  static bool us_PortIsOpen = false;
  static int instance = -1;

  /* Timing related */
  typedef std::nano                                      ratio_t;      /* 1000000000 */
  typedef std::chrono::high_resolution_clock::time_point timestamp_t;  /* Point in time */
  typedef std::chrono::nanoseconds                       duration_t;   /* Duration in nanoseconds */
  static double us_CPUcycleDuration               = ratio_t::den / (float)cycles_per_sec;          /* CPU cycle duration in nanoseconds */
  static double us_InvCPUcycleDurationNanoSeconds = 1.0 / (ratio_t::den / (float)cycles_per_sec);  /* Inverted CPU cycle duration in nanoseconds */
  static timestamp_t m_StartTime   = std::chrono::high_resolution_clock::now();
  static timestamp_t m_LastTime    = m_StartTime;

  #ifdef __cplusplus
  static std::atomic_int us_thread(0);
  #else
  static _Atomic int us_thread = 0;
  #endif
  static pthread_mutex_t us_mutex;
  class USBSID_Class {
    private:

      int us_InstanceID;

      /* LIBUSB */
      int LIBUSB_Setup(bool start_threaded, bool with_cycles);
      int LIBUSB_Exit(void);
      int LIBUSB_Available(uint16_t vendor_id, uint16_t product_id);
      void LIBUSB_StopTransfers(void);
      int LIBUSB_OpenDevice(void);
      void LIBUSB_CloseDevice(void);
      int LIBUSB_DetachKernelDriver(void);
      int LIBUSB_ConfigureDevice(void);
      void LIBUSB_InitOutBuffer(void);
      void LIBUSB_FreeOutBuffer(void);
      void LIBUSB_InitInBuffer(void);
      void LIBUSB_FreeInBuffer(void);
      static void LIBUSB_CALL usb_out(struct libusb_transfer *transfer);
      static void LIBUSB_CALL usb_in(struct libusb_transfer *transfer);

      /* Line encoding ~ baud rate is ignored by TinyUSB */
      unsigned char encoding[7] = { 0x40, 0x54, 0x89, 0x00, 0x00, 0x00, 0x08 };  // 9000000 ~ 0x895440

      /* Threading */
      void* USBSID_Thread(void);
      int USBSID_InitThread(void);
      void USBSID_StopThread(void);
      int USBSID_IsRunning(void);
      pthread_t us_ptid;

      /* Ringbuffer */
      void USBSID_ResetRingBuffer(void);
      void USBSID_InitRingBuffer(int buffer_size, int differ_size);
      void USBSID_InitRingBuffer(void);
      void USBSID_DeInitRingBuffer(void);
      bool USBSID_IsHigher(void);
      int USBSID_RingDiff(void);
      void USBSID_RingPut(uint8_t item);
      uint8_t USBSID_RingGet(void);
      void USBSID_FlushBuffer(void);

      /* Ringbuffer reads & writes*/
      void USBSID_RingPopCycled(void);  /* Threaded writer with cycles */
      void USBSID_RingPop(void);  /* Threaded writer */

    public:

      USBSID_Class();   /* Constructor */
      ~USBSID_Class();  /* Deconstructor */

      int us_Found;

      /* USBSID */
      int USBSID_Init(bool start_threaded, bool with_cycles);
      int USBSID_Close(void);
      bool USBSID_isInitialised(void){ return us_Initialised; };
      bool USBSID_isAvailable(void){ return us_Available; };
      bool USBSID_isOpen(void){ return us_PortIsOpen; };

      /* USBSID & SID control */
      void USBSID_Pause(void);                                            /* Pause playing by releasing chipselect pins */
      void USBSID_Reset(void);                                            /* Reset all SID chips */
      void USBSID_ResetAllRegisters(void);                                /* Reset register for all SID chips */
      void USBSID_Mute(void);                                             /* Mute all SID chips */
      void USBSID_UnMute(void);                                           /* UnMute all SID chips */
      void USBSID_DisableSID(void);                                       /* Release reset pin and unmute SID */
      void USBSID_EnableSID(void);                                        /* Assert reset pin and release chipselect pins */
      void USBSID_ClearBus(void);                                         /* Clear the SID bus from any data */
      void USBSID_SetClockRate(long clockrate_cycles,                     /* Set CPU clockrate in Hertz */
                               bool suspend_sids);                        /* Assert SID RES signal while changing clockrate (Advised!)*/
      long USBSID_GetClockRate(void);                                     /* Get CPU clockrate in Hertz  */
      long USBSID_GetRefreshRate(void);                                   /* Get cycles per refresh rate */
      long USBSID_GetRasterRate(void);                                    /* Get cycles per raster rate */
      uint8_t* USBSID_GetSocketConfig(uint8_t socket_config[]);           /* Get socket config for parsing */
      int USBSID_GetSocketNumSIDS(int socket, uint8_t socket_config[]);   /* Get the socket number of sids configured */
      int USBSID_GetSocketChipType(int socket, uint8_t socket_config[]);  /* Get the socket chip type configured */
      int USBSID_GetSocketSIDType1(int socket, uint8_t socket_config[]);  /* Get the socket SID 1 type configured */
      int USBSID_GetSocketSIDType2(int socket, uint8_t socket_config[]);  /* Get the socket SID 2 type configured (only works for clone chip types ofcourse) */
      int USBSID_GetNumSIDs(void);                                        /* Get the total number of sids configured */
      int USBSID_GetFMOplSID(void);                                       /* Get the sid number (if configured) to address FMOpl */
      int USBSID_GetPCBVersion(void);                                     /* Get the PCB version */
      void USBSID_SetStereo(int state);                                   /* Set device to mono or stereo ~ v1.3 PCB only */
      void USBSID_ToggleStereo(void);                                     /* Toggle between mono and stereo ~ v1.3 PCB only */

      /* Synchronous direct */
      void USBSID_SingleWrite(unsigned char *buff, int len);                /* Single write buffer of size_t ~ example: config writing */
      unsigned char USBSID_SingleRead(uint8_t reg);                            /* Single read register, return result */
      unsigned char USBSID_SingleReadConfig(unsigned char *buff, int len);  /* Single to buffer of specified length ~ example: config reading */

      /* Asynchronous direct */
      void USBSID_Write(unsigned char *buff, size_t len);                    /* Write buffer of size_t len */
      void USBSID_Write(uint8_t reg, uint8_t val);                           /* Write register and value */
      void USBSID_Write(unsigned char *buff, size_t len, uint16_t cycles);   /* Wait n cycles, write buffer of size_t len */
      void USBSID_Write(uint8_t reg, uint8_t val, uint16_t cycles);          /* Wait n cycles, write register and value */
      void USBSID_WriteCycled(uint8_t reg, uint8_t val, uint16_t cycles);    /* Write register and value, USBSID uses cycles for delay */
      unsigned char USBSID_Read(uint8_t reg);                                /* Write register, return result */
      unsigned char USBSID_Read(unsigned char *writebuff);                   /* Write buffer, return result */
      unsigned char USBSID_Read(unsigned char *writebuff, uint16_t cycles);  /* Wait for n cycles and write buffer, return result */

      /* Asynchronous thread */
      void USBSID_WriteRing(uint8_t reg, uint8_t val);                         /* Write register and value to ringbuffer, USBSID adds 10 delay cycles to each write */
      void USBSID_WriteRingCycled(uint8_t reg, uint8_t val, uint16_t cycles);  /* Write register, value, and cycles to ringbuffer */

      /* Threading */
      void USBSID_EnableThread(void);   /* Enable the thread on the fly */
      void USBSID_DisableThread(void);  /* Disable the running thread and switch to non threaded and cycled on the fly */

      /* Ringbuffer */
      void USBSID_SetFlush(void);          /* Set flush buffer flag to 1 */
      void USBSID_Flush(void);             /* Set flush buffer flag to 1 and flushes the buffer */
      void USBSID_SetBufferSize(int size); /* Set the buffer size for storing writes */
      void USBSID_SetDiffSize(int size);   /* Set the minimum size difference between head & tail */
      void USBSID_RestartRingBuffer(void); /* Restart the ringbuffer*/

      /* Thread utils */
      void USBSID_RestartThread(bool with_cycles);
      static void *_USBSID_Thread(void *context)
      { /* Required for supplying private function to pthread_create */
        return ((USBSID_Class *)context)->USBSID_Thread();
      }

      /* Timing and cycles */
      uint_fast64_t USBSID_WaitForCycle(uint_fast16_t cycles);         /* Sleep for n cycles */
      uint_fast64_t USBSID_CycleFromTimestamp(timestamp_t timestamp);  /* Returns cycles since m_StartTime */

      /* Utils */
      /* TODO: Deprecate this function, emulator/player should handle this */
      uint8_t USBSID_Address(uint16_t addr);  /* Calculates correct SID address to write to if player does not */
  };

} /* USBSIDDriver */


#ifdef USBSID_OPTOFF
#pragma GCC diagnostic pop
#pragma GCC pop_options
#endif

#endif /* _USBSID_H_ */
