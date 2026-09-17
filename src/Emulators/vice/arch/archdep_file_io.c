/* [C64D-REFERENCE-ONLY]
 * Vendored from VICE as REFERENCE, not as live code. Kept for features the
 * original emulator has and RetroDebugger does not yet (printer / RS232, drive
 * and cartridge menus, virtual keyboard). Its SDL calls are already commented
 * out; RetroDebugger draws its own ImGui UI.
 * DO NOT delete, and do not "port" it -- see src/Emulators/REFERENCE_FILES.md.
 */
/*
 * archdep_file_io.c - File I/O helpers from VICE 3.10 arch layer
 *
 * Provides archdep_file_size() and archdep_fseeko() for the embedded build.
 */

#include "vice.h"

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#define fseeko _fseeki64
#define ftello _ftelli64
#else
#include <sys/types.h>
#endif

off_t archdep_file_size(FILE *stream)
{
    off_t pos;
    off_t end;

    pos = ftello(stream);
    if (pos < 0) {
        return -1;
    }
    if (fseeko(stream, 0, SEEK_END) != 0) {
        return -1;
    }
    end = ftello(stream);
    if (fseeko(stream, pos, SEEK_SET) != 0) {
        return -1;
    }
    return end;
}

int archdep_fseeko(FILE *stream, off_t offset, int whence)
{
    return fseeko(stream, offset, whence);
}

off_t archdep_ftello(FILE *stream)
{
    return ftello(stream);
}

/* Compare two file paths for real-path equality.
   Simple string comparison for our embedded build. */
int archdep_real_path_equal(const char *path1, const char *path2)
{
    if (path1 == NULL || path2 == NULL) {
        return 0;
    }
    return strcmp(path1, path2) == 0;
}
