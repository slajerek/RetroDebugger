//
// GOATTRACKER sound routines
//

#define GSOUND_C

#ifdef __WIN32__
#include <windows.h>
#endif

#include "goattrk2.h"

// General / reSID output
int playspeed;
int usehardsid = 0;
int usecatweasel = 0;
int initted = 0;
int firsttimeinit = 1;
unsigned framerate = PALFRAMERATE;
Sint16 *buffer = NULL;
FILE *writehandle = NULL;

void gtsound_playrout(void);
void gtsound_mixer(Sint32 *dest, unsigned samples);
Uint32 gtsound_timer(Uint32 interval);

#ifdef __DONTOCMPILE_WIN32__

// Win32 HardSID output
typedef void (CALLBACK* lpWriteToHardSID)(Uint8 DeviceID, Uint8 SID_reg, Uint8 Data);
typedef Uint8 (CALLBACK* lpReadFromHardSID)(Uint8 DeviceID, Uint8 SID_reg);
typedef void (CALLBACK* lpInitHardSID_Mapper)(void);
typedef void (CALLBACK* lpMuteHardSID_Line)(int Mute);
typedef void (CALLBACK* lpHardSID_Delay)(Uint8 DeviceID, Uint16 Cycles);
typedef void (CALLBACK* lpHardSID_Write)(Uint8 DeviceID, Uint16 Cycles, Uint8 SID_reg, Uint8 Data);
typedef void (CALLBACK* lpHardSID_Flush)(Uint8 DeviceID);
typedef void (CALLBACK* lpHardSID_SoftFlush)(Uint8 DeviceID);
typedef boolean (CALLBACK* lpHardSID_Lock)(Uint8 DeviceID);
lpWriteToHardSID WriteToHardSID = NULL;
lpReadFromHardSID ReadFromHardSID = NULL;
lpInitHardSID_Mapper InitHardSID_Mapper = NULL;
lpMuteHardSID_Line MuteHardSID_Line = NULL;
lpHardSID_Delay HardSID_Delay = NULL;
lpHardSID_Write HardSID_Write = NULL;
lpHardSID_Flush HardSID_Flush = NULL;
lpHardSID_SoftFlush HardSID_SoftFlush = NULL;
lpHardSID_Lock HardSID_Lock = NULL;
HINSTANCE hardsiddll = 0;
int dll_initialized = FALSE;
// Cycle-exact HardSID support
int cycleexacthardsid = FALSE;
SDL_Thread* playerthread = NULL;
SDL_mutex* flushmutex = NULL;
volatile int runplayerthread = FALSE;
volatile int flushplayerthread = FALSE;
volatile int suspendplayroutine = FALSE;
int sound_thread(void *userdata);

void InitHardDLL(void);

// Win32 CatWeasel MK3 PCI output
#define SID_SID_PEEK_POKE   CTL_CODE(FILE_DEVICE_SOUND,0x0800UL + 1,METHOD_BUFFERED,FILE_ANY_ACCESS)
HANDLE catweaselhandle;

#else

// Unix HardSID & CatWeasel output
int hardsidfd = -1;
int catweaselfd = -1;

#endif

int gtsound_init(unsigned b, unsigned mr, unsigned writer, unsigned hardsid, unsigned m, unsigned ntsc, unsigned multiplier, unsigned catweasel, unsigned interpolate, unsigned customclockrate)
{
	
	LOGD("gtsound_init");
	
	
  int c;

//  #ifdef __WIN32__
//  if (!flushmutex)
//      flushmutex = SDL_CreateMutex();
//  #endif

  gtsound_uninit();

  if (multiplier)
  {
    if (ntsc)
    {
      framerate = NTSCFRAMERATE * multiplier;
      snd_bpmtempo = 150 * multiplier;
    }
    else
    {
      framerate = PALFRAMERATE * multiplier;
      snd_bpmtempo = 125 * multiplier;
    }
  }
  else
  {
    if (ntsc)
    {
      framerate = NTSCFRAMERATE / 2;
      snd_bpmtempo = 150 / 2;
    }
    else
    {
      framerate = PALFRAMERATE / 2;
      snd_bpmtempo = 125 / 2;
    }
  }

	/*
  if (hardsid)
  {
    #ifdef __WIN32__
    InitHardDLL();
    if (dll_initialized)
    {
      usehardsid = hardsid;
      if (cycleexacthardsid) {
        HardSID_Lock(usehardsid-1);
        HardSID_Flush(usehardsid-1);
        HardSID_Write(usehardsid-1, SIDWRITEDELAY, 0, 0x00);
        Sleep(300);
      }
      for (c = 0; c < NUMSIDREGS; c++)
      {
        sidreg[c] = 0;
        if (cycleexacthardsid) {
            HardSID_Write(usehardsid-1, SIDWRITEDELAY, c, 0x00);
        }
        else {
            WriteToHardSID(usehardsid-1, c, 0x00);
        }
      }
      if (cycleexacthardsid) {
        HardSID_SoftFlush(usehardsid-1);
      }
      else {
        MuteHardSID_Line(FALSE);
      }
    }
    else return 0;
    if (!cycleexacthardsid)
    {
      SDL_SetTimer(1000 / framerate, sound_timer);
    }
    else
    {
      runplayerthread = TRUE;
      playerthread = SDL_CreateThread(sound_thread, NULL);
      if (!playerthread) return 0;
    }
    #else
    char filename[80];
    sprintf(filename, "/dev/sid%d", hardsid-1);
    hardsidfd = open(filename, O_WRONLY, S_IREAD|S_IWRITE);
    if (hardsidfd >= 0)
    {
      usehardsid = hardsid;
      for (c = 0; c < NUMSIDREGS; c++)
      {
        Uint32 dataword = c << 8;
        write(hardsidfd, &dataword, 4);
      }
    }
    else return 0;
    SDL_SetTimer(1000 / framerate, sound_timer);
    #endif

    goto SOUNDOK;
  }

  if (catweasel)
  {
    #ifdef __WIN32__
    catweaselhandle = CreateFile("\\\\.\\SID6581_1", GENERIC_READ, FILE_SHARE_WRITE|FILE_SHARE_READ, 0L,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0L);
    if (catweaselhandle == INVALID_HANDLE_VALUE)
      return 0;
    #else
    catweaselfd = open("/dev/sid", O_WRONLY);
    if (catweaselfd < 0)
      catweaselfd = open("/dev/misc/sid", O_WRONLY);
    if (catweaselfd < 0)
      return 0;
    #ifndef __amigaos__
    if (ntsc)
      ioctl(catweaselfd, CWSID_IOCTL_NTSC);
    else
      ioctl(catweaselfd, CWSID_IOCTL_PAL);
    #endif
    #endif

    usecatweasel = 1;
    SDL_SetTimer(1000 / framerate, sound_timer);
    goto SOUNDOK;
  }

	*/
	
  if (!buffer) buffer = malloc(MIXBUFFERSIZE * sizeof(Sint16));
  if (!buffer) return 0;

  if (writer)
    writehandle = fopen("sidaudio.raw", "wb");

  playspeed = mr;
  if (playspeed < MINMIXRATE) playspeed = MINMIXRATE;
  if (playspeed > MAXMIXRATE) playspeed = MAXMIXRATE;
  if (b < MINBUF) b = MINBUF;
  if (b > MAXBUF) b = MAXBUF;

  if (firsttimeinit)
  {
    if (!snd_init(mr, SIXTEENBIT|MONO, b, 1, 0)) return 0;
    firsttimeinit = 0;
  }
  playspeed = snd_mixrate;
  sid_init(playspeed, m, ntsc, interpolate & 1, customclockrate, interpolate >> 1);

  snd_player = &gtsound_playrout;
  snd_setcustommixer(gtsound_mixer);

  SOUNDOK:
  initted = 1;
	
  return 1;
}

void gtsound_uninit(void)
{
	LOGD("gtsound_uninit");
	
  int c;

  if (!initted) return;
  initted = 0;

  // Apparently a delay is needed to make sure the sound timer thread is
  // not mixing stuff anymore, and we can safely delete related structures
  SDL_Delay(50);

//  if (usehardsid || usecatweasel)
//  {
//    #ifdef __WIN32__
//    if (!playerthread)
//    {
//      SDL_SetTimer(0, NULL);
//    }
//    else
//    {
//      runplayerthread = FALSE;
//      SDL_WaitThread(playerthread, NULL);
//      playerthread = NULL;
//    }
//    #else
//    SDL_SetTimer(0, NULL);
//    #endif
//  }
//  else
  {
      snd_setcustommixer(NULL);
    snd_player = NULL;
  }

  if (writehandle)
  {
    fclose(writehandle);
    writehandle = NULL;
  }
  
  if (buffer)
  {
    free(buffer);
    buffer = NULL;
  }

	/*
  if (usehardsid)
  {
    #ifdef __WIN32__
    for (c = 0; c < NUMSIDREGS; c++)
    {
        if (cycleexacthardsid) {
            HardSID_Write(usehardsid-1, SIDWRITEDELAY, c, 0x00);
        }
        else {
            WriteToHardSID(usehardsid-1, c, 0x00);
        }
    }
    MuteHardSID_Line(TRUE);
    #else
    if (hardsidfd >= 0)
    {
      for (c = 0; c < NUMSIDREGS; c++)
      {
        Uint32 dataword = c << 8;
        write(hardsidfd, &dataword, 4);
      }
      close(hardsidfd);
      hardsidfd = -1;
    }
    #endif
  }

  if (usecatweasel)
  {
    #ifdef __WIN32__
    DWORD w;
    unsigned char buf[NUMSIDREGS * 2];
    for (w = 0; w < NUMSIDREGS; w++)
    {
      buf[w*2] = 0x18 - w;
      buf[w*2+1] = 0;
    }
    DeviceIoControl(catweaselhandle, SID_SID_PEEK_POKE, buf, sizeof(buf), 0L, 0UL, &w, 0L);
    CloseHandle(catweaselhandle);
    catweaselhandle = INVALID_HANDLE_VALUE;
    #else
    if (catweaselfd >= 0)
    {
      unsigned char buf[NUMSIDREGS];
      memset(buf, 0, sizeof(buf));
      lseek(catweaselfd, 0, SEEK_SET);
      write(catweaselfd, buf, sizeof(buf));
      close(catweaselfd);
      catweaselfd = -1;
    }
    #endif
  }
	 */
}

void gtsound_suspend(void)
{
  #ifdef __DONTOCMPILE_WIN32__
  SDL_LockMutex(flushmutex);
  suspendplayroutine = TRUE;
  SDL_UnlockMutex(flushmutex);
  #endif
}

void gtsound_flush(void)
{
  #ifdef __DONTOCMPILE_WIN32__
  SDL_LockMutex(flushmutex);
  flushplayerthread = TRUE;
  SDL_UnlockMutex(flushmutex);
  #endif
}

Uint32 gtsound_timer(Uint32 interval)
{
  if (!initted) return interval;
  gtsound_playrout();
  return interval;
}

#ifdef __DONTOCMPILE_WIN32__
int gtsound_thread(void *userdata)
{
  unsigned long flush_cycles_interactive = hardsidbufinteractive * 1000; /* 0 = flush off for interactive mode*/
  unsigned long flush_cycles_playback = hardsidbufplayback * 1000; /* 0 = flush off for playback mode*/
  unsigned long cycles_after_flush = 0;
  boolean interactive;

  while (runplayerthread)
  {
    unsigned cycles = 1000000 / framerate; // HardSID should be clocked at 1MHz
    int c;

    if (flush_cycles_interactive > 0 || flush_cycles_playback > 0)
    {
      cycles_after_flush += cycles;
    }

    // Do flush if starting playback, stopping playback, starting an interactive note etc.
    if (flushplayerthread)
    {
      SDL_LockMutex(flushmutex);
      if (HardSID_Flush)
      {
        HardSID_Flush(usehardsid-1);
      }
      // Can clear player suspend now (if set)
      suspendplayroutine = FALSE;
      flushplayerthread = FALSE;
      SDL_UnlockMutex(flushmutex);

      SDL_Delay(0);
    }

    if (!suspendplayroutine) playroutine();

    interactive = !(boolean)recordmode /* jam mode */ || !(boolean)isplaying();

    for (c = 0; c < NUMSIDREGS; c++)
    {
      unsigned o = sid_getorder(c);

        // Extra delay before loading the waveform (and mt_chngate,x)
        if ((o == 4) || (o == 11) || (o == 18))
        {
        HardSID_Write(usehardsid-1, SIDWRITEDELAY+SIDWAVEDELAY, o, sidreg[o]);
          cycles -= SIDWRITEDELAY+SIDWAVEDELAY;
      }
        else
      {
            HardSID_Write(usehardsid-1, SIDWRITEDELAY, o, sidreg[o]);
            cycles -= SIDWRITEDELAY;
        }
    }

    // Now wait the rest of frame
    while (cycles)
    {
      unsigned runnow = cycles;
      if (runnow > 65535) runnow = 65535;
      HardSID_Delay(usehardsid-1, runnow);
      cycles -= runnow;
    }

      if ((flush_cycles_interactive>0 && interactive && cycles_after_flush>=flush_cycles_interactive) ||
          (flush_cycles_playback>0 && !interactive && cycles_after_flush>=flush_cycles_playback))
    {
      if (HardSID_SoftFlush)
        HardSID_SoftFlush(usehardsid-1);
          cycles_after_flush = 0;
    }
  }

  unsigned r;

  for (r = 0; r < NUMSIDREGS; r++)
  {
    HardSID_Write(usehardsid-1, SIDWRITEDELAY, r, 0);
  }
  if (HardSID_SoftFlush)
    HardSID_SoftFlush(usehardsid-1);

  return 0;
}
#endif

void gtsound_playrout(void)
{
  int c;

  playroutine();
	
  if (usehardsid)
  {
    #ifdef __DONTOCMPILE_WIN32__
    for (c = 0; c < NUMSIDREGS; c++)
    {
      unsigned o = sid_getorder(c);
        if (cycleexacthardsid) {
            HardSID_Write(usehardsid-1, SIDWRITEDELAY, o, sidreg[o]);
        }
        else {
            WriteToHardSID(usehardsid-1, o, sidreg[o]);
        }
    }
    #else
    for (c = 0; c < NUMSIDREGS; c++)
    {
      unsigned o = sid_getorder(c);
      Uint32 dataword = (o << 8) | sidreg[o];
      write(hardsidfd, &dataword, 4);
    }
    #endif
  }
  else if (usecatweasel)
  {
    #ifdef __DONTOCMPILE_WIN32__
    DWORD w;
    unsigned char buf[NUMSIDREGS * 2];

    for(w = 0; w < NUMSIDREGS; w++)
    {
      unsigned o = sid_getorder(w);

      buf[w*2] = o;
      buf[w*2+1] = sidreg[o];
    }
    DeviceIoControl(catweaselhandle, SID_SID_PEEK_POKE, buf, sizeof(buf), 0L, 0UL, &w, 0L);
    #else
    for (c = 0; c < NUMSIDREGS; c++)
    {
      unsigned o = sid_getorder(c);

      lseek(catweaselfd, o, SEEK_SET);
      write(catweaselfd, &sidreg[o], 1);
    }
    #endif
  }
}

void gtsound_mixer(Sint32 *dest, unsigned samples)
{
  int c;

  if (!initted) return;
  if (samples > MIXBUFFERSIZE) return;
  if (!buffer) return;

  sid_fillbuffer(buffer, samples);
  if (writehandle)
    fwrite(buffer, samples * sizeof(Uint16), 1, writehandle);

  for (c = 0; c < samples; c++)
    dest[c] = buffer[c];
}

#ifdef __DONTOCMPILE_WIN32__
void InitHardDLL()
{
  if (!(hardsiddll=LoadLibrary("HARDSID.DLL"))) return;

  WriteToHardSID = (lpWriteToHardSID) GetProcAddress(hardsiddll, "WriteToHardSID");
  ReadFromHardSID = (lpReadFromHardSID) GetProcAddress(hardsiddll, "ReadFromHardSID");
  InitHardSID_Mapper = (lpInitHardSID_Mapper) GetProcAddress(hardsiddll, "InitHardSID_Mapper");
  MuteHardSID_Line = (lpMuteHardSID_Line) GetProcAddress(hardsiddll, "MuteHardSID_Line");

  if (!WriteToHardSID) return;

  // Try to get cycle-exact interface
  HardSID_Delay = (lpHardSID_Delay) GetProcAddress(hardsiddll, "HardSID_Delay");
  HardSID_Write = (lpHardSID_Write) GetProcAddress(hardsiddll, "HardSID_Write");
  HardSID_Flush = (lpHardSID_Flush) GetProcAddress(hardsiddll, "HardSID_Flush");
  HardSID_SoftFlush = (lpHardSID_SoftFlush) GetProcAddress(hardsiddll, "HardSID_SoftFlush");
  HardSID_Lock = (lpHardSID_Lock) GetProcAddress(hardsiddll, "HardSID_Lock");
  if ((HardSID_Delay) && (HardSID_Write) && (HardSID_Flush) && (HardSID_SoftFlush) && (HardSID_Lock))
    cycleexacthardsid = TRUE;

  InitHardSID_Mapper();
  dll_initialized = TRUE;
}
#endif
