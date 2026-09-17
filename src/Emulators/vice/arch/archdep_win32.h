/* [C64D-REFERENCE-ONLY]
 * Vendored from VICE as REFERENCE, not as live code. Kept for features the
 * original emulator has and RetroDebugger does not yet (printer / RS232, drive
 * and cartridge menus, virtual keyboard). Its SDL calls are already commented
 * out; RetroDebugger draws its own ImGui UI.
 * DO NOT delete, and do not "port" it -- see src/Emulators/REFERENCE_FILES.md.
 */
/*
 * archdep_win32.h - Windows-specific arch definitions for embedded VICE.
 *
 * Modeled after archdep_unix.h for the RetroDebugger Windows build.
 */

#ifndef VICE_ARCHDEP_WIN32_H
#define VICE_ARCHDEP_WIN32_H

#include "archapi.h"

/* Filesystem dependant operators.  */
#define FSDEVICE_DEFAULT_DIR "."
#define ARCHDEP_FSDEVICE_DEFAULT_DIR "."

#ifndef ARCHDEP_PATH_MAX
# define ARCHDEP_PATH_MAX 260
#endif

#define FSDEV_DIR_SEP_STR    "\\"
#define FSDEV_DIR_SEP_CHR    '\\'
#define FSDEV_EXT_SEP_STR    "."
#define FSDEV_EXT_SEP_CHR    '.'

#define ARCHDEP_DIR_SEP_STR  FSDEV_DIR_SEP_STR
#define ARCHDEP_DIR_SEP_CHR  FSDEV_DIR_SEP_CHR
#define ARCHDEP_EXT_SEP_STR  FSDEV_EXT_SEP_STR
#define ARCHDEP_EXT_SEP_CHR  FSDEV_EXT_SEP_CHR

/* Path separator.  */
#define ARCHDEP_FINDPATH_SEPARATOR_CHAR   ';'
#define ARCHDEP_FINDPATH_SEPARATOR_STRING ";"

/* Modes for fopen().  */
#define MODE_READ              "rb"
#define MODE_READ_TEXT         "r"
#define MODE_READ_WRITE        "r+b"
#define MODE_WRITE             "wb"
#define MODE_WRITE_TEXT        "w"
#define MODE_APPEND            "ab"
#define MODE_APPEND_READ_WRITE "a+b"

/* Printer default devices.  */
#define ARCHDEP_PRINTER_DEFAULT_DEV1 "print.dump"
#define ARCHDEP_PRINTER_DEFAULT_DEV2 "NUL:"
#define ARCHDEP_PRINTER_DEFAULT_DEV3 "NUL:"

/* Default RS232 devices.  */
#define ARCHDEP_RS232_DEV1 "COM1:"
#define ARCHDEP_RS232_DEV2 "COM2:"
#define ARCHDEP_RS232_DEV3 "rs232.dump"
#define ARCHDEP_RS232_DEV4 "NUL:"

/* Default location of raw disk images.  */
#define ARCHDEP_RAWDRIVE_DEFAULT "A:"

/* Access types (Windows io.h equivalents) */
#include <io.h>
#define ARCHDEP_R_OK 4
#define ARCHDEP_W_OK 2
#define ARCHDEP_X_OK 0  /* no execute check on Windows */
#define ARCHDEP_F_OK 0

/* Standard line delimiter.  */
#define ARCHDEP_LINE_DELIMITER "\r\n"

/* Ethernet default device */
#define ARCHDEP_ETHERNET_DEFAULT_DEVICE ""

/* Default sound fragment size */
#define ARCHDEP_SOUND_FRAGMENT_SIZE 1

const char *archdep_home_path(void);

/* set this path to customize the preference storage */
extern const char *archdep_pref_path;

#define LIBDIR "."
#define DOCDIR "."
#define VICEUSERDIR "vice"

#define archdep_signals_init(x)
#define archdep_signals_pipe_set()
#define archdep_signals_pipe_unset()

#endif
