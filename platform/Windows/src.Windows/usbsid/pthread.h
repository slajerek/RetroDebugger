/*
 * pthread.h - Minimal POSIX threads layer for the USBSID-Pico driver
 * on MSVC/ClangCL builds.
 *
 * Covers the subset used by src/Emulators/vice/lib/libusbsiddrv:
 * mutex, condition variable, create/join/exit/self and clock_gettime.
 * Only on the include path of the USBSID driver sources in c64d.vcxproj.
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
 */

#ifndef US_WIN_PTHREAD_H
#define US_WIN_PTHREAD_H
#pragma once

#if !defined(_WIN32)
#error "usbsid/pthread.h is for Windows builds only"
#endif

#include <winsock2.h> /* struct timeval for libusb event timeouts */
#include <windows.h>
#include <process.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef HANDLE pthread_t;
typedef int pthread_attr_t;
typedef SRWLOCK pthread_mutex_t;
typedef int pthread_mutexattr_t;
typedef CONDITION_VARIABLE pthread_cond_t;
typedef int pthread_condattr_t;

#define PTHREAD_MUTEX_INITIALIZER SRWLOCK_INIT
#define PTHREAD_COND_INITIALIZER  CONDITION_VARIABLE_INIT

#ifndef CLOCK_REALTIME
typedef int clockid_t;
#define CLOCK_REALTIME 0

/**
 * @brief: Read the wall clock time
 * @param clk: clock id, only CLOCK_REALTIME is supported
 * @param ts: receives the current time
 * @return: 0 on success, -1 on failure
 */
static inline int clock_gettime(clockid_t clk, struct timespec *ts)
{
    (void)clk;
    return (timespec_get(ts, TIME_UTC) == TIME_UTC) ? 0 : -1;
}
#endif

/**
 * @brief: Initialize a mutex
 * @param m: mutex to initialize
 * @param attr: ignored
 * @return: 0
 */
static inline int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *attr)
{
    (void)attr;
    InitializeSRWLock(m);
    return 0;
}

/**
 * @brief: Destroy a mutex, no-op for SRW locks
 * @param m: mutex to destroy
 * @return: 0
 */
static inline int pthread_mutex_destroy(pthread_mutex_t *m)
{
    (void)m;
    return 0;
}

/**
 * @brief: Lock a mutex
 * @param m: mutex to lock
 * @return: 0
 */
static inline int pthread_mutex_lock(pthread_mutex_t *m)
{
    AcquireSRWLockExclusive(m);
    return 0;
}

/**
 * @brief: Unlock a mutex
 * @param m: mutex to unlock
 * @return: 0
 */
static inline int pthread_mutex_unlock(pthread_mutex_t *m)
{
    ReleaseSRWLockExclusive(m);
    return 0;
}

/**
 * @brief: Initialize a condition variable
 * @param c: condition variable to initialize
 * @param attr: ignored
 * @return: 0
 */
static inline int pthread_cond_init(pthread_cond_t *c, const pthread_condattr_t *attr)
{
    (void)attr;
    InitializeConditionVariable(c);
    return 0;
}

/**
 * @brief: Destroy a condition variable, no-op on Windows
 * @param c: condition variable to destroy
 * @return: 0
 */
static inline int pthread_cond_destroy(pthread_cond_t *c)
{
    (void)c;
    return 0;
}

/**
 * @brief: Wake one waiter
 * @param c: condition variable to signal
 * @return: 0
 */
static inline int pthread_cond_signal(pthread_cond_t *c)
{
    WakeConditionVariable(c);
    return 0;
}

/**
 * @brief: Wake all waiters
 * @param c: condition variable to broadcast
 * @return: 0
 */
static inline int pthread_cond_broadcast(pthread_cond_t *c)
{
    WakeAllConditionVariable(c);
    return 0;
}

/**
 * @brief: Wait on a condition variable
 * @param c: condition variable to wait on
 * @param m: locked mutex, released while waiting
 * @return: 0
 */
static inline int pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{
    SleepConditionVariableSRW(c, m, INFINITE, 0);
    return 0;
}

/**
 * @brief: Wait on a condition variable until an absolute CLOCK_REALTIME time
 * @param c: condition variable to wait on
 * @param m: locked mutex, released while waiting
 * @param abstime: absolute deadline
 * @return: 0 when woken, ETIMEDOUT on timeout
 */
static inline int pthread_cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m,
                                         const struct timespec *abstime)
{
    struct timespec now;
    long long ms;

    timespec_get(&now, TIME_UTC);
    ms = ((long long)abstime->tv_sec - (long long)now.tv_sec) * 1000LL
       + ((long long)abstime->tv_nsec - (long long)now.tv_nsec) / 1000000LL;
    if (ms < 0) {
        ms = 0;
    }
    if (!SleepConditionVariableSRW(c, m, (DWORD)ms, 0)) {
        return (GetLastError() == ERROR_TIMEOUT) ? ETIMEDOUT : EINVAL;
    }
    return 0;
}

struct us_pthread_start {
    void *(*fn)(void *);
    void *arg;
};

/**
 * @brief: Thread entry, frees the start block and runs the pthread routine
 * @param p: heap allocated struct us_pthread_start
 * @return: 0
 */
static inline unsigned __stdcall us_pthread_trampoline(void *p)
{
    struct us_pthread_start start = *(struct us_pthread_start *)p;

    free(p);
    start.fn(start.arg);
    return 0;
}

/**
 * @brief: Start a joinable thread
 * @param t: receives the thread handle
 * @param attr: ignored
 * @param fn: thread routine
 * @param arg: argument for the thread routine
 * @return: 0 on success, errno value on failure
 */
static inline int pthread_create(pthread_t *t, const pthread_attr_t *attr,
                                 void *(*fn)(void *), void *arg)
{
    struct us_pthread_start *start;
    uintptr_t h;

    (void)attr;
    start = (struct us_pthread_start *)malloc(sizeof(*start));
    if (start == NULL) {
        return ENOMEM;
    }
    start->fn = fn;
    start->arg = arg;
    h = _beginthreadex(NULL, 0, us_pthread_trampoline, start, 0, NULL);
    if (h == 0) {
        free(start);
        return EAGAIN;
    }
    *t = (pthread_t)h;
    return 0;
}

/**
 * @brief: Wait for a thread to finish and release its handle
 * @param t: thread to join
 * @param retval: ignored, set to NULL when given
 * @return: 0
 */
static inline int pthread_join(pthread_t t, void **retval)
{
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
    if (retval != NULL) {
        *retval = NULL;
    }
    return 0;
}

/**
 * @brief: End the calling thread
 * @param retval: ignored
 */
static inline void pthread_exit(void *retval)
{
    (void)retval;
    _endthreadex(0);
}

/**
 * @brief: Get a pseudo handle for the calling thread
 * @return: current thread pseudo handle
 */
static inline pthread_t pthread_self(void)
{
    return GetCurrentThread();
}

#ifdef __cplusplus
}
#endif

#endif /* US_WIN_PTHREAD_H */
