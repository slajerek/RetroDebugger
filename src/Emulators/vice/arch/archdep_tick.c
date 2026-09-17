/* [C64D-REFERENCE-ONLY]
 * Vendored from VICE as REFERENCE, not as live code. Kept for features the
 * original emulator has and RetroDebugger does not yet (printer / RS232, drive
 * and cartridge menus, virtual keyboard). Its SDL calls are already commented
 * out; RetroDebugger draws its own ImGui UI.
 * DO NOT delete, and do not "port" it -- see src/Emulators/REFERENCE_FILES.md.
 */
/** \file   archdep_tick.c
 * \brief   Relating to the management of time.
 *
 * From VICE 3.10 arch/shared/archdep_tick.c
 * Adapted for the RetroDebugger embedded build.
 */

#include "vice.h"

#include "archdep_defs.h"

#if defined(WINDOWS_COMPILE) || defined(_WIN32)
#   include <windows.h>
#elif defined(HAVE_NANOSLEEP)
#   include <time.h>
#else
#   include <unistd.h>
#   include <errno.h>
#   include <sys/time.h>
#endif
#ifdef MACOS_COMPILE
#   include <mach/mach.h>
#   include <mach/mach_time.h>
#endif
#include <stdio.h>

#include "archdep_tick.h"

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifdef MACOS_COMPILE
static mach_timebase_info_data_t timebase_info;
#endif

#if defined(_WIN32)
static INT64 cached_qpc_freq = 0;
#endif

void tick_init(void)
{
#ifdef MACOS_COMPILE
    mach_timebase_info(&timebase_info);
#endif
#if defined(_WIN32)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        cached_qpc_freq = freq.QuadPart;
    }
#endif
}

tick_t tick_per_second(void)
{
    return TICK_PER_SECOND;
}

#ifdef MACOS_COMPILE
tick_t tick_now(void)
{
    return NANO_TO_TICK(mach_absolute_time() * timebase_info.numer / timebase_info.denom);
}

#elif defined(_WIN32)
tick_t tick_now(void)
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return NANO_TO_TICK((uint64_t)counter.QuadPart * NANO_PER_SECOND / cached_qpc_freq);
}

#else
tick_t tick_now(void)
{
    struct timespec now;

#if defined(LINUX_COMPILE)
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
#elif defined(FREEBSD_COMPILE)
    clock_gettime(CLOCK_MONOTONIC_PRECISE, &now);
#else
    clock_gettime(CLOCK_MONOTONIC, &now);
#endif

    return NANO_TO_TICK(((uint64_t)NANO_PER_SECOND * now.tv_sec) + now.tv_nsec);
}
#endif

#ifdef HAVE_NANOSLEEP
static inline void sleep_impl(tick_t sleep_ticks)
{
    struct timespec ts;
    uint64_t nanos = TICK_TO_NANO(sleep_ticks);

    if (nanos < NANO_PER_SECOND) {
        ts.tv_sec = 0;
        ts.tv_nsec = nanos;
    } else {
        ts.tv_sec = nanos / NANO_PER_SECOND;
        ts.tv_nsec = nanos % NANO_PER_SECOND;
    }

    nanosleep(&ts, NULL);
}

#elif defined(_WIN32)
static inline void sleep_impl(tick_t sleep_ticks)
{
    DWORD ms = (DWORD)(TICK_TO_MICRO(sleep_ticks) / 1000);
    if (ms == 0) ms = 1;
    Sleep(ms);
}
#else
static inline void sleep_impl(tick_t sleep_ticks)
{
    if (usleep(TICK_TO_MICRO(sleep_ticks)) == -EINVAL) {
        usleep(MICRO_PER_SECOND - 1);
    }
}
#endif

void tick_sleep(tick_t sleep_ticks)
{
    sleep_impl(sleep_ticks);
}

tick_t tick_now_after(tick_t previous_tick)
{
    tick_t after = tick_now();

    if (after == previous_tick - 1) {
        after = previous_tick;
    }

    return after;
}

tick_t tick_now_delta(tick_t previous_tick)
{
    return tick_now_after(previous_tick) - previous_tick;
}
