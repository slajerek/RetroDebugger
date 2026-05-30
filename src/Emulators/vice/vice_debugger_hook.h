#ifndef VICE_DEBUGGER_HOOK_H
#define VICE_DEBUGGER_HOOK_H

/* Centralized routing for slajerek's c64d_* debugger hooks. With RETRODEBUGGER
 * defined (the normal RetroDebugger build) each macro expands to the real hook
 * call -> behavior is identical to before. Undefined, the hooks compile out
 * (all are side-effect-only; OFF = no-op), allowing a hook-free vanilla build.
 * The c64d_* functions are declared by ViceWrapper.h / per-subsystem headers;
 * this file only routes CALL SITES through macros. */

#ifdef RETRODEBUGGER

  /* CPU clock accounting */
  #define VICE_HOOK_CPU_CLK_INC()                 c64d_maincpu_clk++
  #define VICE_HOOK_CPU_CLK_DEC()                 c64d_maincpu_clk--
  #define VICE_HOOK_CPU_CLK_ADD(n)                c64d_maincpu_clk += (n)

  /* Drive cell access taps */
  #define VICE_HOOK_DRIVE_CELL_READ(addr)         c64d_mark_drive1541_cell_read(addr)
  #define VICE_HOOK_DRIVE_CELL_WRITE(addr, val)   c64d_mark_drive1541_cell_write((addr), (val))

  /* Drive state */
  #define VICE_HOOK_DRIVE_TRACK_DIRTY(track)      c64d_mark_drive1541_contents_track_dirty(track)
  #define VICE_HOOK_DRIVE_IS_DEBUG()              c64d_is_debug_on_drive1541()

  /* VIC observer */
  #define VICE_HOOK_VIC_START_FRAME()             c64d_c64_vicii_start_frame()
  #define VICE_HOOK_VIC_START_RASTER(line)        c64d_c64_vicii_start_raster_line(line)
  #define VICE_HOOK_VIC_CYCLE()                   c64d_c64_vicii_cycle()
  #define VICE_HOOK_VIC_IRQ_FLAG_CLEAR()          vicii.c64d_irq_flag = 0
  #define VICE_HOOK_VIC_IRQ_FLAG_SET()            vicii.c64d_irq_flag = 1
  #define VICE_HOOK_VIC_REFRESH_SCREEN()          c64d_refresh_screen()

  /* CIA observer */
  #define VICE_HOOK_CIA_IRQ_FLAG_SET(ctx)         (ctx)->c64d_irq_flag = 1
  #define VICE_HOOK_CIA_IRQ_FLAG_CLEAR(ctx)       (ctx)->c64d_irq_flag = 0
  #define VICE_HOOK_CIA1_REG_WRITTEN(addr)        c64d_cia1_register_written = (addr) & 0xf
  #define VICE_HOOK_CIA1_WRITE_VALUE(val)         c64d_cia1_write_value = (val)
  #define VICE_HOOK_CIA1_REG_READ(addr)           c64d_cia1_register_read = (addr) & 0xf
  #define VICE_HOOK_CIA1_READ_VALUE(val)          c64d_cia1_read_value = (val)
  #define VICE_HOOK_CIA2_REG_WRITTEN(addr)        c64d_cia2_register_written = (addr) & 0xf
  #define VICE_HOOK_CIA2_WRITE_VALUE(val)         c64d_cia2_write_value = (val)
  #define VICE_HOOK_CIA2_REG_READ(addr)           c64d_cia2_register_read = (addr) & 0xf
  #define VICE_HOOK_CIA2_READ_VALUE(val)          c64d_cia2_read_value = (val)

  /* VIA observer */
  #define VICE_HOOK_VIA_IRQ_FLAG_CLEAR(ctx)       (ctx)->c64d_irq_flagged = 0
  #define VICE_HOOK_VIA_IRQ_FLAG_SET(ctx)         (ctx)->c64d_irq_flagged = 1

  /* SID observer */
  #define VICE_HOOK_SID_IS_RECEIVING_CHANNELS(n)  c64d_is_receive_channels_data[(n)]
  #define VICE_HOOK_SID_CHANNELS_DATA(no, v1, v2, v3, mix) \
                                                  c64d_sid_channels_data((no),(v1),(v2),(v3),(mix))
  #define VICE_HOOK_SID_SET_WAVE_ATTENUATION(val) c64d_wave_attenuation = (val)
  #define VICE_HOOK_SID_SET_WAVE_SHIFT(val)       c64d_wave_shift = (val)

  /* Lifecycle / UI */
  #define VICE_HOOK_LIFECYCLE_DEBUG_MODE(mode)    c64d_set_debug_mode(mode)
  #define VICE_HOOK_LIFECYCLE_JAM_FLAG()          c64d_is_cpu_in_jam_state = 1
  #define VICE_HOOK_LIFECYCLE_MESSAGE(msg)        c64d_show_message(msg)
  #define VICE_HOOK_LIFECYCLE_SPEED(spd, fps)     c64d_display_speed((spd), (fps))
  #define VICE_HOOK_LIFECYCLE_UIMON_LINE(p)       c64d_uimon_print_line(p)
  #define VICE_HOOK_LIFECYCLE_UIMON_PRINT(p)      c64d_uimon_print(p)

  /* Input / display */
  #define VICE_HOOK_INPUT_DRIVE_LED(n, p1, p2)    c64d_display_drive_led((n), (p1), (p2))

  /* VIC draw: ON = only draw sprites when the debugger skip-flag is clear */
  #define VICE_HOOK_VIC_DRAW_SPRITES()            (c64d_skip_drawing_sprites == 0)

#else /* !RETRODEBUGGER — all side-effect hooks become no-ops */

  /* CPU clock accounting */
  #define VICE_HOOK_CPU_CLK_INC()                 ((void)0)
  #define VICE_HOOK_CPU_CLK_DEC()                 ((void)0)
  #define VICE_HOOK_CPU_CLK_ADD(n)                ((void)0)

  /* Drive cell access taps */
  #define VICE_HOOK_DRIVE_CELL_READ(addr)         ((void)0)
  #define VICE_HOOK_DRIVE_CELL_WRITE(addr, val)   ((void)0)

  /* Drive state */
  #define VICE_HOOK_DRIVE_TRACK_DIRTY(track)      ((void)0)
  #define VICE_HOOK_DRIVE_IS_DEBUG()              0

  /* VIC observer */
  #define VICE_HOOK_VIC_START_FRAME()             ((void)0)
  #define VICE_HOOK_VIC_START_RASTER(line)        ((void)0)
  #define VICE_HOOK_VIC_CYCLE()                   ((void)0)
  #define VICE_HOOK_VIC_IRQ_FLAG_CLEAR()          ((void)0)
  #define VICE_HOOK_VIC_IRQ_FLAG_SET()            ((void)0)
  #define VICE_HOOK_VIC_REFRESH_SCREEN()          ((void)0)

  /* CIA observer */
  #define VICE_HOOK_CIA_IRQ_FLAG_SET(ctx)         ((void)0)
  #define VICE_HOOK_CIA_IRQ_FLAG_CLEAR(ctx)       ((void)0)
  #define VICE_HOOK_CIA1_REG_WRITTEN(addr)        ((void)0)
  #define VICE_HOOK_CIA1_WRITE_VALUE(val)         ((void)0)
  #define VICE_HOOK_CIA1_REG_READ(addr)           ((void)0)
  #define VICE_HOOK_CIA1_READ_VALUE(val)          ((void)0)
  #define VICE_HOOK_CIA2_REG_WRITTEN(addr)        ((void)0)
  #define VICE_HOOK_CIA2_WRITE_VALUE(val)         ((void)0)
  #define VICE_HOOK_CIA2_REG_READ(addr)           ((void)0)
  #define VICE_HOOK_CIA2_READ_VALUE(val)          ((void)0)

  /* VIA observer */
  #define VICE_HOOK_VIA_IRQ_FLAG_CLEAR(ctx)       ((void)0)
  #define VICE_HOOK_VIA_IRQ_FLAG_SET(ctx)         ((void)0)

  /* SID observer */
  #define VICE_HOOK_SID_IS_RECEIVING_CHANNELS(n)  0
  #define VICE_HOOK_SID_CHANNELS_DATA(no, v1, v2, v3, mix) ((void)0)
  #define VICE_HOOK_SID_SET_WAVE_ATTENUATION(val) ((void)0)
  #define VICE_HOOK_SID_SET_WAVE_SHIFT(val)       ((void)0)

  /* Lifecycle / UI */
  #define VICE_HOOK_LIFECYCLE_DEBUG_MODE(mode)    ((void)0)
  #define VICE_HOOK_LIFECYCLE_JAM_FLAG()          ((void)0)
  #define VICE_HOOK_LIFECYCLE_MESSAGE(msg)        ((void)0)
  #define VICE_HOOK_LIFECYCLE_SPEED(spd, fps)     ((void)0)
  #define VICE_HOOK_LIFECYCLE_UIMON_LINE(p)       ((void)0)
  #define VICE_HOOK_LIFECYCLE_UIMON_PRINT(p)      ((void)0)

  /* Input / display */
  #define VICE_HOOK_INPUT_DRIVE_LED(n, p1, p2)    ((void)0)

  /* VIC draw: OFF = always draw sprites (vanilla behavior) */
  #define VICE_HOOK_VIC_DRAW_SPRITES()            1

#endif /* RETRODEBUGGER */

#endif /* VICE_DEBUGGER_HOOK_H */
