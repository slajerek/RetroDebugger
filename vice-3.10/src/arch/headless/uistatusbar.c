/** \file   uistatusbar.c
 *  \brief  Headless UI statusbar stuff
 *
 *  The status bar widget is part of every machine window. This widget
 *  reacts to UI notifications from the emulation core and otherwise
 *  does not interact with the rest of the main menu.
 *
 *  Functions described as "Statusbar API functions" are called by the
 *  rest of the UI or the emulation system itself to report that the
 *  status displays must be updated to reflect possibly new
 *  information. It is not necessary for the data to be truly new; the
 *  statusbar implementation will treat repeated reports of the same
 *  state as no-ops when necessary for performance.
 *
 *  \author Marco van den Heuvel <blackystardust68@yahoo.com>
 *  \author Michael C. Martin <mcmartin@gmail.com>
 *  \author Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * Resources controlled by this widget. We probably need to expand this list.
 * (Do not add resources controlled by widgets #include'd by this widget, only
 *  add resources actually controlled from this widget)
 *
 * $VICERES SoundVolume all
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
#include <stdbool.h>

#include "uiapi.h"
#include "uistatusbar.h"


/** \brief Statusbar API function to register an elapsed time.
 *
 *  \param current The current time value in seconds
 *  \param total   The maximum time value in seconds
 *
 */
void ui_display_event_time(unsigned int current, unsigned int total)
{
    /* printf("%s\n", __func__); */
}


/** \brief Statusbar API function to display playback status.
 *
 *  \param playback_status  Unknown.
 *  \param version          seems to be the VICE version major.minor during
 *                          playback and `NULL` when playback is done.
 *
 *  \todo This function is not implemented and its API is not
 *        understood.
 *
 * \note    Since the statusbar message display widget has been removed, we
 *          have some space to implement a widget to display information
 *          regarding playback/recording.
 */
void ui_display_playback(int playback_status, char *version)
{
    /* printf("%s\n", __func__); */
}

/** \brief  Statusbar API function to display recording status.
 *
 *  \param  recording_status    seems to be bool indicating recording active
 *
 *  \todo   This function is not implemented and its API is not
 *          understood.
 *
 * \note    Since the statusbar message display widget has been removed, we
 *          have some space to implement a widget to display information
 *          regarding playback/recording.
 */
void ui_display_recording(int recording_status)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Statusbar API function to display a message in the status bar.
 *
 *  \param  text    The text to display.
 *  \param  fadeout Erase the text after five seconds unless it has already
 *                  been replaced.
 */
void ui_display_statustext(const char *text, bool fadeout)
{
    /* printf("%s\n", __func__); */
}

/** \brief  Statusbar API function to display current volume
 *
 * This function is a NOP since the volume can be checked and altered via the
 * Mixer Controls via the statusbar.
 *
 * \param[in]   vol     new volume level
 */
void ui_display_volume(int vol)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Statusbar API function to display current joyport inputs.
 *  \param  joyport An array of bytes of size at least
 *                  JOYPORT_MAX_PORTS+1, with data regarding each
 *                  active joyport.
 *  \warning The joyport array is, for all practical purposes,
 *           _1-indexed_. joyport[0] is unused.
 *  \sa ui_sb_state_s::current_joyports Describes the format of the
 *      data encoded in the joyport array. Note that current_joyports
 *      is 0-indexed as is typical for C arrays.
 */
void ui_display_joyport(uint16_t *joyport)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Statusbar API function to report changes in tape control status.
 *
 *  \param control The new tape control. See the DATASETTE_CONTROL_*
 *                 constants in datasette.h for legal values of this
 *                 parameter.
 */
void ui_display_tape_control_status(int port, int control)
{
    /* printf("%s\n", __func__); */
}

/** \brief  Statusbar API function to report changes in tape position.
 *
 *  \param  counter The new value of the position counter.
 *
 *  \note   Only the last three digits of the counter will be displayed.
 */
void ui_display_tape_counter(int port, int counter)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Statusbar API function to report changes in the tape motor.
 *
 *  \param  motor   Nonzero if the tape motor is now on.
 */
void ui_display_tape_motor_status(int port, int motor)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Statusbar API function to report changes in tape status.
 *  \param  tape_status The new tape status.
 *  \note   This function does nothing and its API is not
 *          understood. Furthermore, no other extant UIs appear to react
 *          to this call.
 */
void ui_set_tape_status(int port, int tape_status)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Statusbar API function to report mounting or unmounting of a tape
 *          image.
 *
 *  \param  image   The filename of the tape image (if mounted), or the
 *                  empty string or NULL (if unmounting).
 */
void ui_display_tape_current_image(int port, const char *image)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Statusbar API function to report changes in drive LED intensity.
 *
 *  \param  drive_number    The unit to update (0-3 for drives 8-11)
 *  \param  pwm1            The intensity of the first LED (0=off,
 *                          1000=maximum intensity)
 *  \param  led_pwm2        The intensity of the second LED (0=off,
 *                          1000=maximum intensity)
 *  \todo   The statusbar API does not yet support dual-unit disk
 *          drives.
 */
void ui_display_drive_led(unsigned int drive_number,
                          unsigned int drive_base,
                          unsigned int pwm1,
                          unsigned int led_pwm2)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Statusbar API function to report changes in drive head location.
 *
 *  \param  drive_number        The unit to update (0-3 for drives 8-11)
 *  \param  drive_base          Currently unused.
 *  \param  half_track_number   Twice the value of the head
 *                              location. 18.0 is 36, while 18.5 would be 37.
 *
 *  \todo   The statusbar API does not yet support dual-unit disk
 *          drives. The drive_base argument will likely come into play
 *          once it does.
 */
void ui_display_drive_track(unsigned int drive_number,
                            unsigned int drive_base,
                            unsigned int half_track_number,
                            unsigned int drive_side)
{
    /* printf("%s\n", __func__); */
}


/** \brief Update information about each drive.
 *
 *  \param state           A bitmask int, where bits 0-3 indicate
 *                         whether or not drives 8-11 respectively are
 *                         being emulated carefully enough to provide
 *                         LED information.
 *  \param drive_led_color An array of size at least NUM_DISK_UNITS that
 *                         provides information about the LEDs on this
 *                         drive. An element of this array will only
 *                         be checked if the corresponding bit in
 *                         state is 1.
 *  \note Before calling this function, the drive configuration
 *        resources (Drive8Type, Drive9Type, etc) should all be set to
 *        the values you wish to display.
 *  \warning If a drive's LEDs are active when its LED values change,
 *           the UI will not reflect the LED type change until the
 *           next time the led's values are updated. This should not
 *           happen under normal circumstances.
 *  \sa compute_drives_enabled_mask() for how this function determines
 *      which drives are truly active
 *  \sa ui_sb_state_s::drive_led_types for the data in each element of
 *      drive_led_color
 */
void ui_enable_drive_status(ui_drive_enable_t state, int *drive_led_color)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Statusbar API function to report mounting or unmounting of
 *          a disk image.
 *
 *  \param  unit_number     0-3 to represent disk units at device 8-11.
 *  \param  drive_number    0-1 to represent the drives in a unit
 *  \param  image           The filename of the disk image (if mounted),
 *                          or the empty string or NULL (if unmounting).
 *  \todo This API is insufficient to describe drives with two disk units.
 */
void ui_display_drive_current_image(unsigned int unit_number, unsigned int drive_number, const char *image)
{
    /* printf("%s\n", __func__); */
}


/** \brief  Show reset on statusbar
 *
 * A device was reset, so we show it on the statusbar
 *
 * \param[in]   device  device number
 * \param[in]   mode    reset mode (only for the machine itself)
 */
void ui_display_reset(int device, int mode)
{
    /* NOP */
}
