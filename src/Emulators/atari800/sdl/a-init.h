/* [C64D-REFERENCE-ONLY]
 * Vendored from atari800 as REFERENCE, not as live code. Kept for features the
 * original emulator has and RetroDebugger does not yet (printer / RS232, drive
 * and cartridge menus, virtual keyboard). Its SDL calls are already commented
 * out; RetroDebugger draws its own ImGui UI.
 * DO NOT delete, and do not "port" it -- see src/Emulators/REFERENCE_FILES.md.
 */
#ifndef SDL_INIT_H_
#define SDL_INIT_H_

int SDL_INIT_Initialise(void);
void SDL_INIT_Exit(void);

#endif /* SDL_INIT_H_ */
