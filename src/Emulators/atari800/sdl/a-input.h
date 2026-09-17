/* [C64D-REFERENCE-ONLY]
 * Vendored from atari800 as REFERENCE, not as live code. Kept for features the
 * original emulator has and RetroDebugger does not yet (printer / RS232, drive
 * and cartridge menus, virtual keyboard). Its SDL calls are already commented
 * out; RetroDebugger draws its own ImGui UI.
 * DO NOT delete, and do not "port" it -- see src/Emulators/REFERENCE_FILES.md.
 */
#ifndef SDL_INPUT_H_
#define SDL_INPUT_H_

#include <stdio.h>

int SDL_INPUT_ReadConfig(char *option, char *parameters);
void SDL_INPUT_WriteConfig(FILE *fp);

int SDL_INPUT_Initialise(int *argc, char *argv[]);
void SDL_INPUT_Exit(void);
/* Restarts input after e.g. exiting the console monitor. */
void SDL_INPUT_Restart(void);

void SDL_INPUT_Mouse(void);

#endif /* SDL_INPUT_H_ */
