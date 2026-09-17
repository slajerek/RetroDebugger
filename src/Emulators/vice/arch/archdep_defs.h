/* [C64D-REFERENCE-ONLY]
 * Vendored from VICE as REFERENCE, not as live code. Kept for features the
 * original emulator has and RetroDebugger does not yet (printer / RS232, drive
 * and cartridge menus, virtual keyboard). Its SDL calls are already commented
 * out; RetroDebugger draws its own ImGui UI.
 * DO NOT delete, and do not "port" it -- see src/Emulators/REFERENCE_FILES.md.
 */
/** \file   archdep_defs.h
 * \brief   Defines, enums and types used by the archdep functions
 *
 * From VICE 3.10 arch/shared/archdep_defs.h, adapted for embedded build.
 */

#ifndef VICE_ARCHDEP_DEFS_H
#define VICE_ARCHDEP_DEFS_H

#include "vice.h"
#include <inttypes.h>

/** \brief  Extension used for autostart disks */
#define ARCHDEP_AUTOSTART_DISK_EXTENSION    "d64"

#if defined(WINDOWS_COMPILE)
# define ARCHDEP_FINDPATH_SEPARATOR_STRING  ";"
#else
# define ARCHDEP_FINDPATH_SEPARATOR_STRING  ":"
#endif

/* Path max */
#ifdef WINDOWS_COMPILE
# include <stdlib.h>
# ifndef _MAX_PATH
#  define _MAX_PATH 260
# endif
# define ARCHDEP_PATH_MAX   _MAX_PATH
#elif defined(UNIX_COMPILE) || defined(HAIKU_COMPILE)
# include <limits.h>
# define ARCHDEP_PATH_MAX   PATH_MAX
#else
# define ARCHDEP_PATH_MAX   4096
#endif

/** \brief  XDG Base Directory Specification dirs */
#define ARCHDEP_XDG_CACHE_HOME  ".cache"
#define ARCHDEP_XDG_CONFIG_HOME ".config"

/* Determine if we compile against SDL */
#if defined(USE_SDLUI) || defined(USE_SDL2UI)
# define ARCHDEP_USE_SDL
#endif

#if defined(WINDOWS_COMPILE) || defined(BEOS_COMPILE)
# ifdef ARCHDEP_USE_SDL
#  define ARCHDEP_VICERC_NAME   "sdl-vice.ini"
#  define ARCHDEP_VICE_RTC_NAME "sdl-vice.rtc"
# else
#  define ARCHDEP_VICERC_NAME   "vice.ini"
#  define ARCHDEP_VICE_RTC_NAME "vice.rtc"
# endif
#else
# ifdef ARCHDEP_USE_SDL
#  define ARCHDEP_VICERC_NAME   "sdl-vicerc"
#  define ARCHDEP_VICE_RTC_NAME "sdl-vice.rtc"
# else
#  define ARCHDEP_VICERC_NAME   "vicerc"
#  define ARCHDEP_VICE_RTC_NAME "vice.rtc"
# endif
#endif

/** \brief  Autostart diskimage prefix/suffix */
#define ARCHDEP_AUTOSTART_DISKIMAGE_PREFIX  "autostart-"
#define ARCHDEP_AUTOSTART_DISKIMAGE_SUFFIX  ".d64"

/* Printf specifiers for size_t / ssize_t */
#ifdef _WIN32
# define PRI_SIZE_T     "Iu"
# define PRI_SSIZE_T    "Id"
#else
# define PRI_SIZE_T     "zu"
# define PRI_SSIZE_T    "zd"
#endif

#endif
