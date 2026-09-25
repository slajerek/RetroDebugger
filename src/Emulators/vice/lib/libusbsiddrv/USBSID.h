/*
 * USBSID-Pico is a RPi Pico (RP2040/RP2350) based board for interfacing one
 * or two MOS SID chips and/or hardware SID emulators over (WEB)USB with your
 * computer, phone or ASID supporting player.
 *
 * USBSID.h
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

#ifndef _USBSID_H_
#define _USBSID_H_

#ifdef __APPLE__
#undef HAVE_ALIGNED_ALLOC
#define USE_VENDOR_ITF
#define LIBUSB_TIMEOUT   1000
#else
#define LIBUSB_TIMEOUT   0
#endif

#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__unix__) || defined(__APPLE__)
  #define __US_LINUX_COMPILE
#elif defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
  #define __US_WINDOWS_COMPILE
#endif

/**
 * @brief Fix for CLANG64 and CLANGARM64 builds
 * @ref https://github.com/LouDnl/USBSID-Pico-driver/issues/11
 * @ref https://github.com/msys2/MINGW-packages/actions/runs/21104083635
 */
#if defined(__US_WINDOWS_COMPILE)
  /* Detect ARM64 across various compiler defines */
  #if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM) || defined(_ARM_)
    /* libusb on Windows typically expects WINAPI (stdcall) on x86,
       but Clang on ARM64 must treat this as empty. */
    #undef WINAPI
    #define WINAPI
    #undef LIBUSB_CALL
    #define LIBUSB_CALL
  #else
    #ifndef WINAPI
      #define WINAPI __stdcall
    #endif
    #ifndef LIBUSB_CALL
      #define LIBUSB_CALL WINAPI
    #endif
  #endif
#endif

/* Fallback for other platforms */
#ifndef LIBUSB_CALL
  #define LIBUSB_CALL
#endif

/* Macro wrapper around sizeof */
#ifndef count_of
#define count_of(a) (sizeof(a)/sizeof(uint8_t))
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <pthread.h> /* Required, see below */
/**
 * @brief More info on the pthread.h fix for CLANG64 and CLANGARM64 builds
 * @ref https://github.com/LouDnl/USBSID-Pico-driver/issues/11
 * @ref https://github.com/msys2/MINGW-packages/actions/runs/21104083635
 */



/* Uncomment for debug logging */
// #define USBSID_DEBUG
#ifdef USBSID_DEBUG
  #define USBDBG(...) fprintf(__VA_ARGS__)
#else
  #define USBDBG(...) ((void)0)
#endif
#define USBERR(...) fprintf(__VA_ARGS__)


/* Pre-define libusb structs */
struct libusb_context;
struct libusb_transfer;
struct libusb_device_handle;

namespace USBSID_NS
{
  class USBSID_Class;

  /* LIBUSB/USBSID related */

  enum {
    VENDOR_ID      = 0xCAFE,
    PRODUCT_ID     = 0x4011,
    ACM_CTRL_DTR   = 0x01,
    ACM_CTRL_RTS   = 0x02,
#ifndef USE_VENDOR_ITF /* By default the driver uses the CDC port */
    EP_OUT_ADDR    = 0x02,
    EP_IN_ADDR     = 0x82,
#else
    EP_OUT_ADDR    = 0x04,
    EP_IN_ADDR     = 0x84,
#endif
    LEN_IN_BUFFER  = 1,
    LEN_OUT_BUFFER = 64,
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

  /* Socket config array
   *  0 Initiator     = 0x37
   *  1 Verification  = 0x7f
   *  2 HiByte        = socketOne enabled
   *  2 LoByte        = socketOne dualsid
   *  3 LoByte        = socketOne chipType
   *  4 HiByte        = socketOne sid1Type
   *  4 LoByte        = socketOne sid2Type
   *  5 HiByte        = socketTwo enabled
   *  5 LoByte        = socketTwo dualsid
   *  6 LoByte        = socketTwo chipType
   *  7 HiByte        = socketTwo sid1Type
   *  7 LoByte        = socketTwo sid2Type
   *  8 HiByte        = socketOne SID1 id
   *  8 LoByte        = socketOne SID2 id
   *  9 HiByte        = socketOne SID1 id
   *  9 LoByte        = socketOne SID2 id
   * 10 LoByte 0b001  = socketTwo mirrors socketOne
   * 10 LoByte 0b010  = sockets are flipped One is Two and vice versa
   * 10 LoByte 0b100  = SID addresses are mixed (quad sid only)
   * 11 Terminator    = 0xff
   */
  #define SOCKET_BUFFER_SIZE 12


  /* Ringbuffer related */

  typedef struct {
    int ring_read;
    int ring_write;
    int is_allocated;
    uint8_t * __restrict__ ringbuffer;
  } ring_buffer_t;

  static const int min_diff_size = 16;
  static const int min_ring_size = 256;
  static const int default_diff_size = 64;
  static const int default_ring_size = 8192;


  /* Clockspeed related */

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


  /* Timing related */
  typedef std::nano                               ratio_t;      /* 1000000000 */
  typedef std::chrono::steady_clock::time_point   timestamp_t;  /* Point in time */
  typedef std::chrono::nanoseconds                duration_t;   /* Duration in nanoseconds */


  /* USBSID instance related */

  /* Shared instance id counter for diagnostics only. Does not guard any connections
   * or connection behaviours */
  static std::atomic_int instance_counter{-1};

  /* Enumartion struct to hold information about detected physical boards */
  struct USBSID_DeviceInfo {
    std::string serial;
    uint8_t bus = 0;
    std::vector<uint8_t> port_path;  /* libusb_get_port_numbers(), stable tie-breaker */
  };


  /* Le class */

  class USBSID_Class {
    private:

      /* Driver related */
      int us_InstanceID;                             /* got tattoo? */
      bool us_Initialised = false;                   /* done yet? */
      bool us_Available = false;                     /* are you there? */
      bool us_PortIsOpen = false;                    /* open wide! */

      /* Which physical board to open.
       * For single-board like behaviour, open the first VID/PID match */
      std::string want_serial;                       /* The one we want */
      std::string opened_serial;                     /* The one we get */
      int want_index = 0;                            /* The index we want */

      /* Timing related */
      double us_CPUcycleDuration;                    /* CPU cycle duration in nanoseconds */
      double us_InvCPUcycleDurationNanoSeconds;      /* Inverted CPU cycle duration in nanoseconds */
      timestamp_t m_StartTime;                       /* That moment when... */
      timestamp_t m_LastTime;                        /* I know what you did last summer! */

      /* Ringbuffer related */
      int diff_size = default_diff_size;             /* must init with something to work */
      int ring_size = default_ring_size;             /* must init with something to work */

      /* LIBUSB related */
      libusb_context *ctx = NULL;
      struct libusb_device_handle *devh = NULL;
      struct libusb_transfer *transfer_out = NULL;   /* OUT-going transfers (OUT from host PC to USB-device) */
      struct libusb_transfer *transfer_in = NULL;    /* IN-coming transfers (IN to host PC from USB-device) */
      bool transfer_out_pending = false;             /* for better transfer out sync */
      bool transfer_in_pending = false;              /* for better transfer in sync */
      bool in_buffer_dma = false;                    /* is the in buffer DMA or not */
      bool out_buffer_dma = false;                   /* is the out buffer DMA or not */

      bool threaded = false;                         /* are we threading the needle? */
      bool withcycles = false;                       /* with or without bicycles */
      int rc = -1;                                   /* to rc or not to rc, that is the question */
      int read_completed = 0;                        /* still not done? */
      int write_completed = 0;                       /* still not done? */

      /* USB buffer related */
      uint8_t * __restrict__ in_buffer = NULL;       /* incoming libusb will reside in this buffer */
      uint8_t * __restrict__ out_buffer = NULL;      /* outgoing libusb will reside in this buffer */
      uint8_t * __restrict__ thread_buffer = NULL;   /* data to be transfered to the out_buffer will reside in this buffer */
      uint8_t * __restrict__ write_buffer = NULL;    /* non async data will be written from this buffer */
      uint8_t * __restrict__ result = NULL;          /* variable where read data is copied into */
      int len_out_buffer = 0;                        /* changable variable for out buffer size */
      int buffer_pos = 1;                            /* current position of the out buffer */
      int flush_buffer = 0;                          /* flush buffer yes or no */

      /* Ringbuffer */
      ring_buffer_t us_ringbuffer = {0, 0, 0, NULL}; /* Init with default null values */

      /* Clock cycles per second, per refresh rate, per raster rate */
      long cycles_per_sec    = DEFAULT;              /* default @ 1000000 */
      long cycles_per_frame  = HZ_DEFAULT;           /* default @ 20000 */
      long cycles_per_raster = R_DEFAULT;            /* default @ 20000 */
      int clk_retrieved = 0;
      long us_clkrate = 0;
      int numsids = 0;
      int fmoplsid = -1;
      int pcbversion = -1;
      int socketconfig = -1;

      /* Threading the needle */
      int run_thread = 0;
      std::atomic_int us_thread{0};
      pthread_mutex_t us_mutex = PTHREAD_MUTEX_INITIALIZER;
      pthread_cond_t us_cond = PTHREAD_COND_INITIALIZER;
      pthread_t us_ptid;

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
#ifndef USE_VENDOR_ITF /* CDC only, see LIBUSB_ConfigureDevice() */
      unsigned char encoding[7] = { 0x40, 0x54, 0x89, 0x00, 0x00, 0x00, 0x08 };  // 9000000 ~ 0x895440
#endif

      /* Threading */
      void* USBSID_Thread(void);
      int USBSID_InitThread(void);
      void USBSID_StopThread(void);
      int USBSID_IsRunning(void);

      /* Ringbuffer */
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
      void USBSID_RingPop(void);        /* Threaded writer */

    public:

      USBSID_Class();   /* Constructor */
      ~USBSID_Class();  /* Deconstructor */

      /* Multiboard device targeting, must be called before USBSID_Init() */
      void USBSID_SetTargetSerial(const std::string &serial) { want_serial = serial; }
      void USBSID_SetTargetIndex(int idx) { want_index = idx; }
      const std::string& USBSID_GetSerial(void) const { return opened_serial; }

      /* USBSID */
      int USBSID_Init(bool start_threaded, bool with_cycles);                  /* Well it inits? */
      int USBSID_Close(void);                                                  /* And this does not */
      int USBSID_GetInstanceID(void){ return us_InstanceID; };                 /* Does it count? */
      bool USBSID_isInitialised(void){ return us_Initialised; };               /* Probability 50% */
      bool USBSID_isAvailable(void){ return us_Available; };                   /* Only if you're nice */
      bool USBSID_isOpen(void){ return us_PortIsOpen; };                       /* Adults only */

      /* USBSID & SID control */
      void USBSID_Pause(void);                                                 /* Pause playing by releasing chipselect pins */
      void USBSID_Reset(void);                                                 /* Reset all SID chips */
      void USBSID_ResetAllRegisters(void);                                     /* Reset register for all SID chips */
      void USBSID_Mute(void);                                                  /* Mute all SID chips */
      void USBSID_UnMute(void);                                                /* UnMute all SID chips */
      void USBSID_SetMuted(bool muted);                                        /* Mute or unmute and set the firmware's muted state, volume writes stay masked while muted */
      void USBSID_DisableSID(void);                                            /* Release reset pin and unmute SID */
      void USBSID_EnableSID(void);                                             /* Assert reset pin and release chipselect pins */
      void USBSID_ClearBus(void);                                              /* Clear the SID bus from any data */
      void USBSID_SetClockRate(long clockrate_cycles,                          /* Set CPU clockrate in Hertz */
                               bool suspend_sids);                             /* Assert SID RES signal while changing clockrate (Advised!)*/
      long USBSID_GetClockRate(void);                                          /* Get CPU clockrate in Hertz  */
      long USBSID_GetRefreshRate(void);                                        /* Get cycles per refresh rate */
      long USBSID_GetRasterRate(void);                                         /* Get cycles per raster rate */
      uint8_t* USBSID_GetSocketConfig(uint8_t socket_config[]);                /* Get socket config for parsing */
      int USBSID_GetSocketNumSIDS(int socket, uint8_t socket_config[]);        /* Get the socket number of sids configured */
      int USBSID_GetSocketChipType(int socket, uint8_t socket_config[]);       /* Get the socket chip type configured */
      int USBSID_GetSocketSIDType1(int socket, uint8_t socket_config[]);       /* Get the socket SID 1 type configured */
      int USBSID_GetSocketSIDType2(int socket, uint8_t socket_config[]);       /* Get the socket SID 2 type configured (only works for clone chip types ofcourse) */
      int USBSID_GetNumSIDs(void);                                             /* Get the total number of sids configured */
      int USBSID_GetFMOplSID(void);                                            /* Get the sid number (if configured) to address FMOpl */
      int USBSID_GetPCBVersion(void);                                          /* Get the PCB version */
      void USBSID_SetStereo(int state);                                        /* Set device to mono or stereo ~ v1.3 PCB only */
      void USBSID_ToggleStereo(void);                                          /* Toggle between mono and stereo ~ v1.3 PCB only */

      /* Synchronous direct */
      void USBSID_SingleWrite(unsigned char *buff, size_t len);                /* Single write buffer of size_t ~ example: config writing */
      unsigned char USBSID_SingleRead(uint8_t reg);                            /* Single read register, return result */
      unsigned char USBSID_SingleReadConfig(unsigned char *buff, size_t len);  /* Single to buffer of specified length ~ example: config reading */
      int USBSID_ReadConfig(unsigned char *buff, size_t len);                  /* Single to buffer of specified length ~ returns size of data */

      /* Asynchronous direct */
      void USBSID_Write(unsigned char *buff, size_t len);                      /* Write buffer of size_t len */
      void USBSID_Write(uint8_t reg, uint8_t val);                             /* Write register and value */
      void USBSID_Write(unsigned char *buff, size_t len, uint16_t cycles);     /* Wait n cycles, write buffer of size_t len */
      void USBSID_Write(uint8_t reg, uint8_t val, uint16_t cycles);            /* Wait n cycles, write register and value */
      void USBSID_WriteCycled(uint8_t reg, uint8_t val, uint16_t cycles);      /* Write register and value, USBSID uses cycles for delay */
      unsigned char USBSID_Read(uint8_t reg);                                  /* Write register, return result */
      unsigned char USBSID_Read(unsigned char *writebuff);                     /* Write buffer, return result */
      unsigned char USBSID_Read(unsigned char *writebuff, uint16_t cycles);    /* Wait for n cycles and write buffer, return result */

      /* Asynchronous thread */
      void USBSID_WriteRing(uint8_t reg, uint8_t val);                         /* Write register and value to ringbuffer, USBSID adds 10 delay cycles to each write */
      void USBSID_WriteRingCycled(uint8_t reg, uint8_t val, uint16_t cycles);  /* Write register, value, and cycles to ringbuffer */
      void USBSID_WriteRingCycledN(const uint8_t *items, int count);           /* Write count x (reg, val, cycles hi, cycles lo) to ringbuffer, one lock and one wakeup */

      /* Threading */
      void USBSID_EnableThread(void);                                          /* Enable the thread on the fly */
      void USBSID_DisableThread(void);                                         /* Disable the running thread and switch to non threaded and cycled on the fly */

      /* Ringbuffer */
      void USBSID_SetFlush(void);                                              /* Set flush buffer flag to 1 */
      void USBSID_Flush(void);                                                 /* Set flush buffer flag to 1 and flushes the buffer */
      void USBSID_SetBufferSize(int size);                                     /* Set the buffer size for storing writes */
      void USBSID_SetDiffSize(int size);                                       /* Set the minimum size difference between head & tail */
      void USBSID_ResetRingBuffer(void);                                       /* Resets the ringbuffer to default state */
      void USBSID_RestartRingBuffer(void);                                     /* Restart the ringbuffer */
      int USBSID_RingFree(void);                                               /* Free bytes in the ringbuffer, unlocked estimate */

      /* Thread utils */
      void USBSID_RestartThread(bool with_cycles);                             /* Restart the thread that handles the ringbuffer */
      static void *_USBSID_Thread(void *context)                               /* Internal wrapper to start the thread */
      { /* Required for supplying private function to pthread_create */
        return ((USBSID_Class *)context)->USBSID_Thread();
      }

      /* Timing and cycles */
      uint_fast64_t USBSID_WaitForCycle(uint_fast16_t cycles);                 /* Sleep for n cycles */
      void USBSID_SyncTime(void);                                              /* Sync time for cycle delay function */
  };

  /* Enumerate every VID/PID-matching USBSID-Pico currently attached, without
   * opening any of them for I/O. Opens each briefly (required by libusb to
   * read its string descriptors) only to read back its serial number, then
   * closes it again.
   * Results are sorted by bus/port-path to identified the first physically
   * connected board when raw device-list order isn't guaranteed to be
   * that across platforms */
  std::vector<USBSID_DeviceInfo> USBSID_EnumerateDevices(void);

} /* USBSID_NS (USBSIDDriver) */


#ifdef USBSID_OPTOFF
#pragma GCC diagnostic pop
#pragma GCC pop_options
#endif

#endif /* _USBSID_H_ */
