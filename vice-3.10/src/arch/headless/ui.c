/** \file   ui.c
 * \brief   Headless UI stuff
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 * \author  Marcus Sutton <loggedoubt@gmail.com>
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#ifdef UNIX_COMPILE
#include <unistd.h>
#endif

#ifdef MACOS_COMPILE
#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreFoundation/CFString.h>
#endif

#include "debug_headless.h"
#include "archdep.h"

#include "autostart.h"
#include "cmdline.h"
#include "drive.h"
#include "interrupt.h"
#include "kbd.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "lightpen.h"
#include "resources.h"
#include "util.h"
#include "videoarch.h"
#include "vsync.h"
#include "vsyncapi.h"

#include "uiapi.h"
#include "uistatusbar.h"
#include "archdep.h"

/* for the fullscreen_capability() stub */
#include "fullscreen.h"

#include "ui.h"


/** \brief  Command line options shared between emu's, include VSID
 */
static const cmdline_option_t cmdline_options_common[] =
{
    CMDLINE_LIST_END
};


/** \brief  Flag indicating pause mode
 */
static int is_paused = 0;


/** \brief  Stub to satisfy the various $videochip-resources.c files
 *
 * \param[in]   cap_fullscreen  unused
 */
void fullscreen_capability(struct cap_fullscreen_s *cap_fullscreen)
{
    /* printf("%s\n", __func__); */

    return;
}


/** \brief  Initialize command line options (generic)
 *
 * \return  0 on success, -1 on failure
 */
int ui_cmdline_options_init(void)
{
    /* printf("%s\n", __func__); */

    return cmdline_register_options(cmdline_options_common);
}


/** \brief  Display a generic file chooser dialog
 *
 * \param[in]   format  format string for the dialog's title
 *
 * \return  a copy of the chosen file's name; free it with lib_free()
 *
 * \note    This is currently only called by event_playback_attach_image()
 *
 * \warning This function is unimplemented and will intentionally crash
 *          VICE if it is called.
 */
char *ui_get_file(const char *format, ...)
{
    /* printf("%s\n", __func__); */

    /*
     * Also not called when trying to play back events, at least, I've never
     * seen this called.
     */
    NOT_IMPLEMENTED();
    return NULL;
}


/** \brief  Initialize Gtk3/GLib
 *
 * \param[in]   argc    pointer to main()'s argc
 * \param[in]   argv    main()'s argv
 *
 */
void ui_init_with_args(int *argc, char **argv)
{
    log_verbose(LOG_DEFAULT, "Initialising headless ui with args");
}


/** \brief  Initialize Gtk3/GLib
 *
 * \return  0 on success, -1 on failure
 */
int ui_init(void)
{
    log_verbose(LOG_DEFAULT, "Initialising headless ui");

    return 0;
}


/** \brief  Finalize initialization after creating the main window(s)
 *
 * \note    This function exists for compatibility with other UIs,
 *          but could perhaps be used to activate fullscreen from the
 *          command-line or saved settings file (as it is in WinVICE.)
 *
 * \return  0 on success, -1 on failure
 */
int ui_init_finalize(void)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */

    return 0;
}


/** \brief  Display a dialog box in response to a CPU jam
 *
 * \param[in]   format  format string for the message to display
 *
 * \return  the action the user selected in response to the jam
 */
ui_jam_action_t ui_jam_dialog(const char *format, ...)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */

    va_list args;
    char *buffer;
#if 0
    int result;
#endif
    va_start(args, format);
    buffer = lib_mvsprintf(format, args);
    va_end(args);

    log_warning(LOG_DEFAULT, "%s", buffer);

    lib_free(buffer);

    /* the headless UI ignores this */
    return MACHINE_JAM_ACTION_QUIT; /* quit emulator */
}

static int save_resources_on_exit;

static int set_save_resources_on_exit(int val, void *param)
{
    save_resources_on_exit = val ? 1 : 0;

    return 0;
}

static resource_int_t resources_int[] = {
    { "SaveResourcesOnExit", 0, RES_EVENT_NO, NULL,
      &save_resources_on_exit, set_save_resources_on_exit, NULL },
    RESOURCE_INT_LIST_END
};

/** \brief  Initialize resources related to the UI in general
 *
 * \return  0 on success, -1 on failure
 */
int ui_resources_init(void)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }

    return 0;
}


/** \brief  Clean up memory used by VICE resources
 */
void ui_resources_shutdown(void)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */
}

/** \brief Clean up memory used by the UI system itself
 */
void ui_shutdown(void)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */
}


/** \brief  Dispatch events pending in the GLib main context loop
 */
void ui_dispatch_events(void)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */
}

/** \brief  Display the "Do you want to extend the disk image to
 *          40-track format?" dialog
 *
 * \return  nonzero to extend the image, 0 otherwise
 *
 * \warning This function is not implemented and it will intentionally
 *          crash VICE if called.
 */
int ui_extend_image_dialog(void)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */

    /* FIXME: this dialog needs to be implemented. */
    NOT_IMPLEMENTED();
    return 0;
}


/** \brief  Display error message through the UI
 *
 * \param[in]   format  format string for the error
 */
void ui_error(const char *format, ...)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */

    char *buffer;
    va_list ap;

    va_start(ap, format);
    buffer = lib_mvsprintf(format, ap);
    va_end(ap);

    log_error(LOG_DEFAULT, "VICE Error: %s", buffer);
    lib_free(buffer);
}


/** \brief  Display a message through the UI
 *
 * \param[in]   format  format string for message
 */
void ui_message(const char *format, ...)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */

    char *buffer;
    va_list ap;

    va_start(ap, format);
    buffer = lib_mvsprintf(format, ap);
    va_end(ap);

    log_message(LOG_DEFAULT, "VICE Message: %s", buffer);
    lib_free(buffer);
}


bool ui_pause_loop_iteration(void)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */
    /*
    ui_dispatch_next_event();
    archdep_usleep(10000);
    */
    return is_paused;
}


/** \brief  Keeps the ui events going while the emulation is paused
 *
 * \param[in]   addr    unused
 * \param[in]   data    unused
 */
static void pause_trap(uint16_t addr, void *data)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */
/*
    vsync_suspend_speed_eval();
    sound_suspend();

    while (ui_pause_loop_iteration());
*/
}


/** \brief  Get pause active state
 *
 * \return  boolean
 */
int ui_pause_active(void)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */

    return is_paused;
}


/** \brief  Pause emulation
 */
void ui_pause_enable(void)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */

    if (!ui_pause_active()) {
        is_paused = 1;
        interrupt_maincpu_trigger_trap(pause_trap, 0);
    }
}


/** \brief  Unpause emulation
 */
void ui_pause_disable(void)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */

    if (ui_pause_active()) {
        is_paused = 0;
    }
}


/** \brief  Send current light pen state to the emulator core for all windows
 */
void ui_update_lightpen(void)
{
    /* log_verbose(LOG_DEFAULT, "%s", __func__); */
}

void arch_ui_activate(void)
{
}

/* Temporary stubs for the UI action system, due to the joystick code being
 * built with the headless UI
 */

const char *ui_action_get_name(int action)
{
    return "";
}

int ui_action_get_id(const char *name)
{
    return 0;
}

void ui_action_trigger(int action)
{
}

/* FIXME: should there be a standard function with this name? */
video_canvas_t *ui_get_active_canvas(void)
{
    return NULL;
}
