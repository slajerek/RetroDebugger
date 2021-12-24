#ifndef _INCLUDE_SIDPLAY_SIDINT_H
#define _INCLUDE_SIDPLAY_SIDINT_H 1
#ifndef _GENERATED_STDINT_H
#define _GENERATED_STDINT_H "psid64 1.2"
/* generated using a gnu compiler version Apple LLVM version 8.1.0 (clang-802.0.42) Target: x86_64-apple-darwin16.7.0 Thread model: posix InstalledDir: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin */

#ifdef WIN32

 typedef signed char       int8_t;
 typedef signed short      int16_t;
 typedef signed int        int32_t;
 typedef unsigned char     uint8_t;
 typedef unsigned short    uint16_t;
 typedef unsigned int      uint32_t;

#include "stdint.h"
#define int_least8_t int8_t
#define uint_least8_t uint8_t
#define int_least16_t int16_t
#define uint_least16_t uint16_t
#define int_least32_t int32_t
#define uint_least32_t uint32_t
#define int_least64_t int64_t
#define uint_least64_t uint64_t

#else

#include <stdint.h>

#endif


/* system headers have good uint64_t */
#ifndef _HAVE_UINT64_T
#define _HAVE_UINT64_T
#endif

  /* once */
#endif
#endif
