/* [C64D-REFERENCE-ONLY]
 * Vendored from VICE as REFERENCE, not as live code. Kept for features the
 * original emulator has and RetroDebugger does not yet (printer / RS232, drive
 * and cartridge menus, virtual keyboard). Its SDL calls are already commented
 * out; RetroDebugger draws its own ImGui UI.
 * DO NOT delete, and do not "port" it -- see src/Emulators/REFERENCE_FILES.md.
 */
/** \file   archdep_tick.h
 * \brief   Relating to the management of time.
 *
 * From VICE 3.10 arch/shared/archdep_tick.h.
 * Provides tick_t type and timing functions.
 */

#ifndef VICE_TICK_H
#define VICE_TICK_H

#include <stdint.h>

#define MILLI_PER_SECOND    (1000)
#define MICRO_PER_SECOND    (1000 * 1000)
#define NANO_PER_SECOND     (1000 * 1000 * 1000)

#define TICK_PER_SECOND     (MICRO_PER_SECOND)

typedef uint32_t tick_t;

#define TICK_TO_MILLI(tick) ((uint32_t) (uint64_t)((double)(tick) * ((double)MILLI_PER_SECOND / TICK_PER_SECOND)))
#define TICK_TO_MICRO(tick) ((uint32_t) (uint64_t)((double)(tick) * ((double)MICRO_PER_SECOND / TICK_PER_SECOND)))
#define TICK_TO_NANO(tick)  ((uint64_t) (uint64_t)((double)(tick) * ((double)NANO_PER_SECOND  / TICK_PER_SECOND)))
#define NANO_TO_TICK(nano)  ((tick_t)   (uint64_t)((double)(nano) / ((double)NANO_PER_SECOND  / TICK_PER_SECOND)))

void tick_init(void);
tick_t tick_per_second(void);
tick_t tick_now(void);
tick_t tick_now_after(tick_t previous_tick);
tick_t tick_now_delta(tick_t previous_tick);
void tick_sleep(tick_t delay);

#endif
