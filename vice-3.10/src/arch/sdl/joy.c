/*
 * joy.c - SDL joystick support.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Bernhard Kuhn <kuhn@eikon.e-technik.tu-muenchen.de>
 *  Ulmer Lionel <ulmer@poly.polytechnique.fr>
 *  Andreas Boose <viceteam@t-online.de>
 *
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
#include "types.h"

#include "vice_sdl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "cmdline.h"
#include "joy.h"
#include "joyport.h"
#include "joystick.h"
#include "kbd.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "mouse.h"
#include "resources.h"
#include "sysfile.h"
#include "util.h"
#include "uihotkey.h"
#include "uimenu.h"
#include "vkbd.h"

#define DEFAULT_JOYSTICK_THRESHOLD 10000
#define DEFAULT_JOYSTICK_FUZZ      1000

#ifdef HAVE_SDL_NUMJOYSTICKS
static log_t sdljoy_log = LOG_DEFAULT;

/* Autorepeat in menu & vkbd */
static ui_menu_action_t autorepeat;
static int autorepeat_delay;

/* Joystick threshold (0..32767) */
static int joystick_threshold;

/* Joystick fuzz (0..32767) */
static int joystick_fuzz;

#endif /* HAVE_SDL_NUMJOYSTICKS */

/* ------------------------------------------------------------------------- */

/* Resources.  */

#ifdef HAVE_SDL_NUMJOYSTICKS
static int use_joysticks_for_menu = 0;

static int set_use_joysticks_for_menu(int val, void *param)
{
    use_joysticks_for_menu = val ? 1 : 0;

    return 0;
}

static int set_joystick_threshold(int val, void *param)
{
    if (val < 0 || val > 32767) {
        return -1;
    }

    joystick_threshold = val;
    return 0;
}

static int set_joystick_fuzz(int val, void *param)
{
    if (val < 0 || val > 32767) {
        return -1;
    }

    joystick_fuzz = val;
    return 0;
}

static const resource_int_t resources_int[] = {
    { "JoyThreshold", DEFAULT_JOYSTICK_THRESHOLD, RES_EVENT_NO, NULL,
      &joystick_threshold, set_joystick_threshold, NULL },
    { "JoyFuzz", DEFAULT_JOYSTICK_FUZZ, RES_EVENT_NO, NULL,
      &joystick_fuzz, set_joystick_fuzz, NULL },
    { "JoyMenuControl", 1, RES_EVENT_NO, NULL,
      &use_joysticks_for_menu, set_use_joysticks_for_menu, NULL },
    RESOURCE_INT_LIST_END
};
#endif /* HAVE_SDL_NUMJOYSTICKS */

/* Command-line options.  */

#ifdef HAVE_SDL_NUMJOYSTICKS
static const cmdline_option_t cmdline_options[] =
{
    { "-joymap", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyMapFile", NULL,
      "<name>", "Specify name of joystick map file" },
    { "-joythreshold", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyThreshold", NULL,
      "<0-32767>", "Set joystick threshold" },
    { "-joyfuzz", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "JoyFuzz", NULL,
      "<0-32767>", "Set joystick fuzz" },
    { "-joymenucontrol", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyMenuControl", (resource_value_t)1,
      NULL, "Enable controlling the menu with joysticks" },
    { "+joymenucontrol", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "JoyMenuControl", (resource_value_t)0,
      NULL, "Disable controlling the menu with joysticks" },
    CMDLINE_LIST_END
};
#endif

int joy_sdl_resources_init(void)
{
#ifdef HAVE_SDL_NUMJOYSTICKS
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }
#endif

    return 0;
}

int joy_sdl_cmdline_options_init(void)
{
#ifdef HAVE_SDL_NUMJOYSTICKS
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }
#endif
    return 0;
}

/* ------------------------------------------------------------------------- */

/*
 * Driver methods
 */

static bool sdl_joystick_open(joystick_device_t *joydev)
{
    /* NOP: SDL has its own mechanism for opening/closing devices and appears
     * to just keep all host devices it found open. We return `true` here to
     * avoid resource setters triggering an error.
     */
    return true;
}

static void sdl_joystick_poll(joystick_device_t *joydev)
{
    /* NOP: handled in SDL event loop */
}

static void sdl_joystick_close(joystick_device_t *joystick)
{
    /* NOP: The actual call to SDL_JoystickClose() happens on emulator shutdown,
     * not when closing via VICE's driver */
}


/** \brief  SDL-specific host device data
 */
typedef struct joy_priv_s {
    SDL_Joystick        *sdldev;    /**< SDL device reference */
    VICE_SDL_JoystickID  id;        /**< joystick ID used by VICE */
    int                  joynum;    /**< joystick index according to SDL */
} joy_priv_t;


static joy_priv_t *joy_priv_new(SDL_Joystick *sdldev, int joynum)
{
    joy_priv_t *priv = lib_malloc(sizeof *priv);

    priv->sdldev = sdldev;
    priv->joynum = joynum;
#ifdef USE_SDL2UI
    priv->id     = SDL_JoystickInstanceID(sdldev);
#else
    priv->id     = (VICE_SDL_JoystickID)joynum;
#endif
    return priv;
}

static void joy_priv_free(void *priv)
{
    lib_free(priv);
}


static joystick_driver_t sdl_joystick_driver = {
    .open  = sdl_joystick_open,
    .poll  = sdl_joystick_poll,
    .close = sdl_joystick_close,
    .priv_free = joy_priv_free
};

#ifdef HAVE_SDL_NUMJOYSTICKS

/**********************************************************
 * Generic high level joy routine                         *
 **********************************************************/

static bool sdljoy_get_devices(void);


void joystick_arch_init(void)
{
    sdljoy_log = log_open("SDLJoystick");

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
        log_error(sdljoy_log, "Subsystem init failed!");
    }

    joystick_driver_register(&sdl_joystick_driver);
    sdljoy_get_devices();
    /* sdljoy_rescan(); */
}


static joystick_device_t *scan_device(SDL_Joystick *sdldev, int index)
{
    joystick_device_t *joydev = joystick_device_new();
    joy_priv_t        *priv   = joy_priv_new(sdldev, index);
    const char        *name;
    char               buffer[64];
    int                i;

#ifdef USE_SDL2UI
    name = SDL_JoystickName(sdldev);
#else
    name = SDL_JoystickName(index);
#endif
    if (name != NULL) {
        joydev->name = lib_strdup(name);
    } else {
        log_warning(sdljoy_log,
                    "couldn't retrieve joystick name: %s.", SDL_GetError());
        joydev->name = lib_msprintf("Joystick_%d", index + 1);
    }
#ifdef USE_SDL2UI
    joydev->vendor  = SDL_JoystickGetVendor(sdldev);
    joydev->product = SDL_JoystickGetProduct(sdldev);
#endif
    joydev->priv    = priv;

    /* enumerate axes */
    for (i = 0; i < SDL_JoystickNumAxes(sdldev); i++) {
        joystick_axis_t *axis;

        snprintf(buffer, sizeof buffer, "Axis_%d", i + 1);
        axis = joystick_axis_new(buffer);
        axis->code = i;
#ifdef USE_SDL2UI
        axis->minimum = SDL_JOYSTICK_AXIS_MIN;
        axis->maximum = SDL_JOYSTICK_AXIS_MAX;
#endif
        joystick_device_add_axis(joydev, axis);
    }

    /* enumerate buttons */
    for (i = 0; i < SDL_JoystickNumButtons(sdldev); i++) {
        joystick_button_t *button;

        snprintf(buffer, sizeof buffer, "Button_%d", i + 1);
        button = joystick_button_new(buffer);
        button->code = i;
        joystick_device_add_button(joydev, button);
    }

    /* enumerate hats */
    for (i = 0; i < SDL_JoystickNumHats(sdldev); i++) {
        joystick_hat_t *hat;

        snprintf(buffer, sizeof buffer, "Hat_%d", i + 1);
        hat = joystick_hat_new(buffer);
        hat->code = i;
        joystick_device_add_hat(joydev, hat);
    }
    return joydev;
}

static bool sdljoy_get_devices(void)
{
    int num;
    int njoys = SDL_NumJoysticks();

    if (njoys < 0) {
        log_error(sdljoy_log, "Could not get number of joysticks: %s.", SDL_GetError());
        return false;
    } else if (njoys == 0) {
        log_message(sdljoy_log, "No joysticks found.");
        return true;
    }

    log_message(sdljoy_log, "Got %d potential devices.", njoys);
    for (num = 0; num < njoys; num++) {
#ifdef USE_SDL2UI
        log_message(sdljoy_log,
                    "SDL_JoystickNameForIndex(%d) = \"%s\"",
                    num, SDL_JoystickNameForIndex(num));
#endif
    }
    for (num = 0; num < njoys; num++) {
        SDL_Joystick *sdldev = SDL_JoystickOpen(num);

        if (sdldev == NULL) {
            log_warning(sdljoy_log, "failed to open device %d: %s", num, SDL_GetError());
        } else {
            joystick_device_t *joydev = scan_device(sdldev, num);

            if (joydev != NULL) {
                joystick_device_register(joydev);
            }
#if 0
            SDL_JoystickClose(sdldev);
#endif
        }

    }
    return true;
}


/** \brief  Get SDL joystick ID for joystick device by index
 *
 * \param[in]   ordinal index in the register devices list
 *
 * \return  SDL joystick ID or -1 when \a ordinal is out of bounds
 * \todo    Perhaps rename?
 */
VICE_SDL_JoystickID joy_ordinal_to_id(int ordinal)
{
    joystick_device_t *joydev = joystick_device_by_index(ordinal);

    if (joydev != NULL) {
        joy_priv_t *priv = joydev->priv;
        return priv->id;
    }
    return -1;
}



int sdljoy_rescan(void)
{
#if 0
    int i, axis, button, hat, ball;
    SDL_Joystick *joy;
    char *name;
    int num_joysticks, num_valid_joysticks;

    joystick_close();
    num_joysticks = SDL_NumJoysticks();

    if (num_joysticks == 0) {
        log_message(sdljoy_log, "No joysticks found");
        return 0;
    }
    joy_ordinal_to_id = lib_realloc(joy_ordinal_to_id, (num_joysticks + 1) * sizeof(VICE_SDL_JoystickID));

    log_message(sdljoy_log, "%i joysticks found", num_joysticks);

    for (i = 0, num_valid_joysticks = 0; i < num_joysticks; ++i) {
        joy = SDL_JoystickOpen(i);
        if (joy) {
#ifdef USE_SDL2UI
            name = lib_strdup(SDL_JoystickName(joy));
#else
            name = lib_strdup(SDL_JoystickName(i));
#endif
            axis = SDL_JoystickNumAxes(joy);
            button = SDL_JoystickNumButtons(joy);
            hat = SDL_JoystickNumHats(joy);
            ball = SDL_JoystickNumBalls(joy);

            log_message(sdljoy_log, "Device %i \"%s\" (%i axes, %i buttons, %i hats, %i balls)", i, name, axis, button, hat, ball);
            register_joystick_driver(&sdl_joystick_driver,
                name,
                joy,
                axis,
                button,
                hat);
#ifdef USE_SDL2UI
            joy_ordinal_to_id[num_valid_joysticks++] = SDL_JoystickGetDeviceInstanceID(i);
#else
            joy_ordinal_to_id[num_valid_joysticks++] = (VICE_SDL_JoystickID)i;
#endif
            lib_free(name);
        } else {
            log_warning(sdljoy_log, "Couldn't open joystick %i", i);
        }
    }
    joy_ordinal_to_id[num_valid_joysticks] = -1;

    SDL_JoystickEventState(SDL_ENABLE);
#endif
    return 0;
}


/* ------------------------------------------------------------------------- */

static inline joystick_axis_value_t sdljoy_axis_direction(Sint16 value, joystick_axis_value_t prev)
{
    int thres = joystick_threshold;

    if (prev == 0) {
        thres += joystick_fuzz;
    } else {
        thres -= joystick_fuzz;
    }

    if (value < -thres) {
        return JOY_AXIS_NEGATIVE;
    } else if (value > thres) {
        return JOY_AXIS_POSITIVE;
    } else if ((value < thres) && (value > -thres)) {
        return JOY_AXIS_MIDDLE;
    }

    return prev;
}

static inline uint8_t sdljoy_hat_direction(Uint8 value, uint8_t prev)
{
    uint8_t b;

    b = (value ^ prev) & value;
    b &= SDL_HAT_UP | SDL_HAT_DOWN | SDL_HAT_LEFT | SDL_HAT_RIGHT;

    switch (b) {
        case SDL_HAT_UP:
            return JOYSTICK_DIRECTION_UP;
        case SDL_HAT_DOWN:
            return JOYSTICK_DIRECTION_DOWN;
        case SDL_HAT_LEFT:
            return JOYSTICK_DIRECTION_LEFT;
        case SDL_HAT_RIGHT:
            return JOYSTICK_DIRECTION_RIGHT;
        default:
            /* ignore diagonals and releases */
            break;
    }

    return 0;
}


bool sdljoy_get_joy_for_event(VICE_SDL_JoystickID   device_id,
                              joystick_device_t   **joy_device,
                              int                  *joy_index)
{
    int count = joystick_device_count();

    if (count > 0) {
#ifdef USE_SDL2UI
        int i;

        for (i = 0; i < count; i++) {
            joystick_device_t *joydev = joystick_device_by_index(i);
            joy_priv_t        *priv   = joydev->priv;

            if (priv->id == device_id) {
                if (joy_device != NULL) {
                    *joy_device = joydev;
                }
                if (joy_index != NULL) {
                    *joy_index = i;
                }
                return true;
            }
        }
#else
        if (joy_device != NULL) {
            *joy_device = joystick_device_by_index((int)device_id);
        }
        if (joy_index != NULL) {
            *joy_index = (int)device_id;
        }
#endif
    }
    return false;
}


joystick_device_t *sdljoy_get_joydev_for_event(VICE_SDL_JoystickID event_device_id)
{
    int count;

    count = joystick_device_count();
    if (count > 0) {
#ifdef USE_SDL2UI
        int i;

        for (i = 0; i < count; i++) {
            joystick_device_t *joydev = joystick_device_by_index(i);

            if (joydev != NULL) {
                joy_priv_t *priv = joydev->priv;

                if (priv->id == event_device_id) {
                    return joydev;
                }
            }
        }
#else
        return joystick_device_by_index((int)event_device_id);
#endif
    }
    return NULL;
}


int sdljoy_get_joynum_for_event(VICE_SDL_JoystickID event_device_id)
{
#ifdef USE_SDL2UI
    int i;
    int count;

    count = joystick_device_count();
    for (i = 0; i < count; i++) {
        joystick_device_t *joydev = joystick_device_by_index(i);

        if (joydev != NULL) {
            joy_priv_t *priv = joydev->priv;

            if (priv->id == event_device_id) {
                return i;
            }
        }
    }
    return -1;
#else
    /* SDL 1.2 doesn't have instance IDs, just indexes */
    return (int)event_device_id;
#endif
}




static joystick_mapping_t *sdljoy_get_mapping(SDL_Event e)
{
    joystick_mapping_t *retval = NULL;
    uint8_t cur;
    int joynum;

    switch (e.type) {
        case SDL_JOYAXISMOTION:
            cur = sdljoy_axis_direction(e.jaxis.value, 0);
            if (cur != JOY_AXIS_MIDDLE) {
                joynum = sdljoy_get_joynum_for_event((VICE_SDL_JoystickID)e.jaxis.which);
                if (joynum != -1) {
                    retval = joy_get_axis_mapping_not_setting_value(joynum, e.jaxis.axis, cur);
                }
            }
            break;
        case SDL_JOYHATMOTION:
            cur = sdljoy_hat_direction(e.jhat.value, 0);
            if (cur > 0) {
                joynum = sdljoy_get_joynum_for_event((VICE_SDL_JoystickID)e.jhat.which);
                if (joynum != -1) {
                    retval = joy_get_hat_mapping_not_setting_value(joynum, e.jhat.hat, cur);
                }
            }
            break;
        case SDL_JOYBUTTONDOWN:
            joynum = sdljoy_get_joynum_for_event((VICE_SDL_JoystickID)e.jbutton.which);
            if (joynum != -1) {
                retval = joy_get_button_mapping_not_setting_value(joynum, e.jbutton.button, e.jbutton.state);
            }
            break;
        default:
            break;
    }
    return retval;
}


void sdljoy_clear_presses(void)
{
    int i;

    for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
        joystick_set_value_and(i, 0);
    }
}

/* ------------------------------------------------------------------------- */

ui_menu_action_t sdljoy_autorepeat(void)
{
    if (autorepeat_delay) {
        if (autorepeat != MENU_ACTION_NONE) {
            --autorepeat_delay;
        }
        return MENU_ACTION_NONE;
    } else {
        autorepeat_delay = 4;
    }
    return autorepeat;
}

void sdljoy_autorepeat_init(void) {
    autorepeat_delay = 30;
    autorepeat = MENU_ACTION_NONE;
}

uint8_t sdljoy_check_axis_movement(SDL_Event e)
{
    uint8_t cur, prev;
    Uint8 joynum;
    Uint8 axis;
    Sint16 value;

    joynum = sdljoy_get_joynum_for_event((VICE_SDL_JoystickID)e.jaxis.which);
    axis = e.jaxis.axis;
    value = e.jaxis.value;

    prev = joy_axis_prev(joynum, axis);

    cur = sdljoy_axis_direction(value, prev);

    if (cur == prev) {
        return 0;
    }

    joy_get_axis_mapping(joynum, axis, cur, NULL);
    return cur;
}

uint8_t sdljoy_check_hat_movement(SDL_Event e)
{
    uint8_t cur, prev;
    Uint8 joynum;
    Uint8 hat;
    Uint8 value;

    joynum = sdljoy_get_joynum_for_event((VICE_SDL_JoystickID)e.jhat.which);
    hat = e.jhat.hat;
    value = e.jhat.value;

    prev = joy_hat_prev(joynum, hat);

    cur = sdljoy_hat_direction(value, prev);
    if (cur == prev) {
        return 0;
    }

    joy_get_hat_mapping(joynum, hat, cur, NULL);
    return cur;
}

void sdljoy_axis_event(Uint8 joynum, Uint8 axisnum, Sint16 value)
{
    joystick_device_t *joydev = joystick_device_by_index(joynum);
    joystick_axis_t   *axis   = joydev->axes[axisnum];
#if 0
    prev = joy_axis_prev(joynum, axisnum);

    cur = sdljoy_axis_direction(value, prev);
#endif
    joy_axis_event(axis, value);
}

static ui_menu_action_t sdljoy_perform_event_for_menu_action(joystick_mapping_t* event, Sint16 value)
{
    ui_menu_action_t retval = MENU_ACTION_NONE;

    if (event->action == JOY_ACTION_JOYSTICK) {
        if (use_joysticks_for_menu) {
            switch (event->value.joy_pin) {
                case 0x01:
                    retval = autorepeat = MENU_ACTION_UP;
                    break;
                case 0x02:
                    retval = autorepeat = MENU_ACTION_DOWN;
                    break;
                case 0x04:
                    retval = autorepeat = MENU_ACTION_LEFT;
                    break;
                case 0x08:
                    retval = autorepeat = MENU_ACTION_RIGHT;
                    break;
                case 0x10:
                    retval = MENU_ACTION_SELECT;
                    break;
                default:
                    break;
            }
        }
    } else if (event->action == JOY_ACTION_UI_ACTIVATE) {
        retval = MENU_ACTION_CANCEL;
    } else if (event->action == JOY_ACTION_MAP) {
        retval = MENU_ACTION_MAP;
    }
    if (!value) {
        autorepeat = MENU_ACTION_NONE;
        autorepeat_delay = 30;
        retval += MENU_ACTION_NONE_RELEASE;
    }
    return retval;
}

ui_menu_action_t sdljoy_axis_event_for_menu_action(Uint8 joynum, Uint8 axis, Sint16 value)
{
    ui_menu_action_t retval = MENU_ACTION_NONE;
    joystick_axis_value_t cur, prev;
    joystick_mapping_t *prev_mapping, *cur_mapping;

    prev = joy_axis_prev(joynum, axis);
    cur = sdljoy_axis_direction(value, prev);
    if (cur != prev) {
        prev_mapping = joy_get_axis_mapping(joynum, axis, cur, NULL);
        cur_mapping = joy_get_axis_mapping_not_setting_value(joynum, axis, cur);
        if (cur_mapping) {
            retval = sdljoy_perform_event_for_menu_action(cur_mapping, 1);
        } else if (prev_mapping) {
            retval = sdljoy_perform_event_for_menu_action(prev_mapping, 0);
        }
    }
    return retval;
}

ui_menu_action_t sdljoy_button_event_for_menu_action(Uint8 joynum, Uint8 button, Uint8 value) {
    joystick_mapping_t *prev_mapping = joy_get_button_mapping(joynum, button, value, NULL);
    joystick_mapping_t *cur_mapping = joy_get_button_mapping_not_setting_value(joynum, button, value);
    ui_menu_action_t retval = MENU_ACTION_NONE;

    if (cur_mapping) {
        retval = sdljoy_perform_event_for_menu_action(cur_mapping, 1);
    } else if (prev_mapping) {
        retval = sdljoy_perform_event_for_menu_action(prev_mapping, 0);
    }
    return retval;
}

ui_menu_action_t sdljoy_hat_event_for_menu_action(Uint8 joynum, Uint8 hat, Uint8 value) {
    joystick_mapping_t *prev_mapping = joy_get_hat_mapping(joynum, hat, value, NULL);
    joystick_mapping_t *cur_mapping = joy_get_hat_mapping_not_setting_value(joynum, hat, value);
    ui_menu_action_t retval = MENU_ACTION_NONE;

    if (cur_mapping) {
        retval = sdljoy_perform_event_for_menu_action(cur_mapping, 1);
    } else if (prev_mapping) {
        retval = sdljoy_perform_event_for_menu_action(prev_mapping, 0);
    }

    return retval;
}

/* ------------------------------------------------------------------------- */

/* unused at the moment (2014-07-19, compyx) */
#if 0
static ui_menu_entry_t *sdljoy_get_hotkey(SDL_Event e)
{
    ui_menu_entry_t *retval = NULL;
    sdljoystick_mapping_t *joyevent = sdljoy_get_mapping(e);

    if ((joyevent != NULL) && (joyevent->action == JOY_ACTION_UI_FUNCTION)) {
        retval = joyevent->value.ui_function;
    }

    return retval;
}
#endif

void sdljoy_set_joystick(SDL_Event e, int bits)
{
    joystick_mapping_t *joyevent = sdljoy_get_mapping(e);

    if (joyevent != NULL) {
        joyevent->action = JOY_ACTION_JOYSTICK;
        joyevent->value.joy_pin = (uint16_t)bits;
    }
}

void sdljoy_set_joystick_axis(SDL_Event e, int pot)
{
    int joynum;

    if (e.type != SDL_JOYAXISMOTION) {
        return;
    }
    joynum = sdljoy_get_joynum_for_event((VICE_SDL_JoystickID)e.jaxis.which);
    if (joynum == -1) {
        return;
    }
    joy_set_pot_mapping(joynum, e.jaxis.axis, pot);
}

void sdljoy_set_hotkey(SDL_Event e, ui_menu_entry_t *value)
{
    joystick_mapping_t *joyevent = sdljoy_get_mapping(e);

    if (joyevent != NULL) {
        joyevent->action = JOY_ACTION_UI_FUNCTION;
        if (value->action > ACTION_NONE) {
            joyevent->value.ui_action = value->action;
        }
    }
}

void sdljoy_set_keypress(SDL_Event e, int row, int col)
{
    joystick_mapping_t *joyevent = sdljoy_get_mapping(e);

    if (joyevent != NULL) {
        joyevent->action = JOY_ACTION_KEYBOARD;
        joyevent->value.key[0] = row;
        joyevent->value.key[1] = col;
    }
}

void sdljoy_set_extra(SDL_Event e, int type)
{
    joystick_mapping_t *joyevent = sdljoy_get_mapping(e);

    if (joyevent != NULL) {
        joyevent->action = type ? JOY_ACTION_MAP : JOY_ACTION_UI_ACTIVATE;
    }
}

void sdljoy_unset(SDL_Event e)
{
    joystick_mapping_t *joyevent = sdljoy_get_mapping(e);

    if (joyevent != NULL) {
        joyevent->action = JOY_ACTION_NONE;
    }
}

/* ------------------------------------------------------------------------- */

static int _sdljoy_swap_ports = 0;

void sdljoy_swap_ports(void)
{
    int i, k;

    resources_get_int("JoyDevice1", &i);
    resources_get_int("JoyDevice2", &k);
    resources_set_int("JoyDevice1", k);
    resources_set_int("JoyDevice2", i);
    _sdljoy_swap_ports ^= 1;
}

int sdljoy_get_swap_ports(void)
{
    return _sdljoy_swap_ports;
}

void joystick_arch_shutdown(void)
{
}


/* ------------------------------------------------------------------------- */

#else
/* !HAVE_SDL_NUMJOYSTICKS */

void sdljoy_swap_ports(void)
{
    int i, k;

    resources_get_int("JoyDevice1", &i);
    resources_get_int("JoyDevice2", &k);
    resources_set_int("JoyDevice1", k);
    resources_set_int("JoyDevice2", i);
}

void joystick(void)
{
    /* Provided only for archdep joy.h. TODO: Migrate joystick polling in here if any needed? */
}

void joystick_close(void)
{
}

int joy_arch_init(void)
{
    return 0;
}
#endif
