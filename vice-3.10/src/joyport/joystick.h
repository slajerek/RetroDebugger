/** \file   joystick.h
 * \brief   Common joystick emulation.
 *
 * \author  Andreas Boose <viceteam@t-online.de>
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/* This file is part of VICE, the Versatile Commodore Emulator.
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

#ifndef VICE_JOYSTICK_H
#define VICE_JOYSTICK_H

#include "types.h"
#include "joyport.h" /* for JOYPORT_MAX_PORTS */
#include <stdbool.h>
#include <stdint.h>
#include "lib.h"


#define JOYSTICK_DIRECTION_NONE  0
#define JOYSTICK_DIRECTION_UP    1
#define JOYSTICK_DIRECTION_DOWN  2
#define JOYSTICK_DIRECTION_LEFT  4
#define JOYSTICK_DIRECTION_RIGHT 8

#define JOYSTICK_AUTOFIRE_OFF   0
#define JOYSTICK_AUTOFIRE_ON    1

#define JOYSTICK_AUTOFIRE_MODE_PRESS       0    /* autofire only when fire is pressed */
#define JOYSTICK_AUTOFIRE_MODE_PERMANENT   1    /* autofire only when fire is NOT pressed */

#define JOYSTICK_AUTOFIRE_SPEED_DEFAULT    10   /* default autofire speed, button will be on this many times per second */
#define JOYSTICK_AUTOFIRE_SPEED_MIN        1
#define JOYSTICK_AUTOFIRE_SPEED_MAX        255

/** \brief  Use keypad as predefined keys for joystick emulation
 *
 * Should always be defined for proper VICE, can be undef'ed for ports
 */
#define COMMON_JOYKEYS

#ifdef COMMON_JOYKEYS

#define JOYSTICK_KEYSET_NUM          3
#define JOYSTICK_KEYSET_NUM_KEYS     16 /* 4 directions, 4 diagonals, 8 fire */

#define JOYSTICK_KEYSET_IDX_NUMBLOCK 0
#define JOYSTICK_KEYSET_IDX_A        1
#define JOYSTICK_KEYSET_IDX_B        2

extern int joykeys[JOYSTICK_KEYSET_NUM][JOYSTICK_KEYSET_NUM_KEYS];

/* several things depend on the order/exact values of the members in this enum,
 * DO NOT CHANGE!
 */
typedef enum {
    JOYSTICK_KEYSET_FIRE,
    JOYSTICK_KEYSET_SW,
    JOYSTICK_KEYSET_S,
    JOYSTICK_KEYSET_SE,
    JOYSTICK_KEYSET_W,
    JOYSTICK_KEYSET_E,
    JOYSTICK_KEYSET_NW,
    JOYSTICK_KEYSET_N,
    JOYSTICK_KEYSET_NE,
    JOYSTICK_KEYSET_FIRE2,
    JOYSTICK_KEYSET_FIRE3,
    JOYSTICK_KEYSET_FIRE4,
    JOYSTICK_KEYSET_FIRE5,
    JOYSTICK_KEYSET_FIRE6,
    JOYSTICK_KEYSET_FIRE7,
    JOYSTICK_KEYSET_FIRE8
} joystick_direction_t;
#endif

/* standard devices */
#define JOYDEV_NONE      0
#define JOYDEV_NUMPAD    1
#define JOYDEV_KEYSET1   2
#define JOYDEV_KEYSET2   3

#define JOYDEV_DEFAULT   JOYDEV_NUMPAD

#define JOYDEV_REALJOYSTICK_MIN (JOYDEV_KEYSET2 + 1)


/** \brief  Joystick device name length (including 0)
 */
#define JOYDEV_NAME_SIZE    0x80

/** \brief  Digital axis values
 */
typedef enum joystick_axis_value_e {
    JOY_AXIS_NEGATIVE = -1, /**< negative direction (usually up or left) */
    JOY_AXIS_MIDDLE   =  0, /**< middle, centered position */
    JOY_AXIS_POSITIVE =  1  /**< positive direction (usually down or right) */
} joystick_axis_value_t;

/** \brief  Hat direction joystick input index values
 */
typedef enum joystick_hat_direction_e {
    JOY_HAT_UP    = 0,
    JOY_HAT_DOWN  = 1,
    JOY_HAT_LEFT  = 2,
    JOY_HAT_RIGHT = 3
} joystick_hat_direction_t;

/* Actions to perform on joystick input */
typedef enum joystick_action_e {
    JOY_ACTION_NONE = 0,

    /* Joystick movement or button press */
    JOY_ACTION_JOYSTICK = 1,

    /* Keyboard key press */
    JOY_ACTION_KEYBOARD = 2,

    /* Map button */
    JOY_ACTION_MAP = 3,

    /* (De)Activate UI */
    JOY_ACTION_UI_ACTIVATE = 4,

    /* Call UI function */
    JOY_ACTION_UI_FUNCTION = 5,

    /* Joystick axis used for potentiometers */
    JOY_ACTION_POT_AXIS = 6,

    JOY_ACTION_MAX = JOY_ACTION_POT_AXIS
} joystick_action_t;

/** \brief  Joystick input types used by the vjm files
 */
typedef enum joystick_input_e {
    JOY_INPUT_AXIS   = 0,   /**< map host axis input */
    JOY_INPUT_BUTTON = 1,   /**< map host button input */
    JOY_INPUT_HAT    = 2,   /**< map host hat input */
    JOY_INPUT_BALL   = 3,   /**< map host ball input */

    JOY_INPUT_MAX    = JOY_INPUT_BALL
} joystick_input_t;


/* Input mapping for each direction/button/etc */
typedef struct joystick_mapping_s {
    /* Action to perform */
    joystick_action_t action;

    union {
        uint16_t joy_pin;

        /* key[0] = row, key[1] = column, key[2] = flags */
        int key[3];
        int ui_action;
    } value;
} joystick_mapping_t;

/** \brief  Calibration for a host input
 */
typedef struct joystick_calibration_s {
    bool invert;            /**< invert value */
    struct {
        int32_t negative;   /**< axis threshold: V <= T: active input */
        int32_t positive;   /**< axis threshold: V >= T: active input */
    } threshold;
} joystick_calibration_t;

/** \brief  Joystick button object
 *
 * Information on a host device button input.
 */
typedef struct joystick_button_s {
    uint32_t                  code;         /**< event code */
    char                     *name;         /**< button name */
    int32_t                   prev;         /**< previous polled value */
    int32_t                   index;        /**< index in buttons array */
    joystick_mapping_t        mapping;      /**< button mapping */
    joystick_calibration_t    calibration;  /**< button calibration */
    struct joystick_device_s *device;       /**< parent joystick device */
} joystick_button_t;


/** \brief  Joystick axis object
 *
 * Information on a host device axis input.
 */
typedef struct joystick_axis_s {
    uint32_t  code;     /**< axis event code */
    char     *name;     /**< axis name */
    int32_t   index;    /**< index in hats array */
    int32_t   prev;     /**< previous polled value */
    /* capabilities (TODO: more data like fuzz, flat) */
    int32_t   minimum;  /**< minimum axis value */
    int32_t   maximum;  /**< maximum axis value */
    bool      digital;  /**< axis is digital (reports -1, 0, 1) */
    struct {
       joystick_mapping_t negative; /**< negative direction */
       joystick_mapping_t positive; /**< positive direction */
       unsigned int       pot;      /**< pot number (1 or 2, 0 means unmapped) */
    } mapping;          /**< mapping for negative and positive directions, and
                             pot. TODO: support pot values other than on/off so
                             emulated paddles and mice can be mapped to axes. */
    joystick_calibration_t    calibration;  /**< axis calibration */
    struct joystick_device_s *device;       /**< parent joystick device */

} joystick_axis_t;


/** \brief  Joystick hat object
 *
 * Information on a host device hat input.
 */
typedef struct joystick_hat_s {
    uint32_t  code;     /**< hat event code */
    char     *name;     /**< hat name */
    int32_t   index;    /**< index in hats array (for the old API) */
    int32_t   prev;     /**< previous polled value */
    struct {
        joystick_mapping_t up;      /**< mapping for 'up' direction */
        joystick_mapping_t down;    /**< mapping for 'down' direction */
        joystick_mapping_t left;    /**< mapping for 'left' direction */
        joystick_mapping_t right;   /**< mapping for 'right' direction */
    } mapping;          /**< mappings per direction */
    joystick_calibration_t    calibration;  /* XXX: no idea if this makes sense
                                                    for hats */
    struct joystick_device_s *device;       /**< parent joystick device */
} joystick_hat_t;


/** \brief  Joystick device object
 *
 * Contains all information on a host joystick device.
 */
typedef struct joystick_device_s {
    /** \brief  Device name ("<vendor-name> <product-name>") */
    char               *name;

    /** \brief  Arch-specific device identifier
     *
     * Path or UUID use to identify the device for the driver's \c open()
     * method to (re)open the device for use.
     * For example on Linux this will be a "file" in the <tt>/dev/input/</tt>
     * directory.
     */
    char               *node;

    /** \brief  HID vendor ID */
    uint16_t            vendor;

    /** \brief  HID product ID */
    uint16_t            product;

    /** \brief  List of axes */
    joystick_axis_t   **axes;

    /** \brief  List of buttons */
    joystick_button_t **buttons;

    /** \brief  List of hats */
    joystick_hat_t    **hats;

    /** \brief  Number of axes */
    int                 num_axes;

    /** \brief  Number of buttons */
    int                 num_buttons;

    /** \brief  Number of hats */
    int                 num_hats;

    /* bookkeeping for resizing arrays when adding elements */
    size_t              max_axes;       /**< size of \c axes array */
    size_t              max_buttons;    /**< size of \c buttons array */
    size_t              max_hats;       /**< size of \c hats array */

    /** \brief  Do not sort axes, buttons and hats
     *
     * If, for some reason, a driver doesn't want the joystick system to sort
     * the axes, buttons and hats of a device when registering that device, this
     * flag can be set to \c true to disable sorting of those objects.
     */
    bool                disable_sort;

    /** \brief  Emulated machine's joystick port associated with host device */
    int                 joyport;

    /** \brief  Private arch-specific data
     *
     * An arch-specific driver can store data here that cannot be portably
     * contained in the core joystick data. This pointer will be passed to the
     * driver's \c priv_free() method (if that method reference is non-NULL) on
     * calling \c joystick_device_free().
     *
     * For example: the Linux driver stores a \c joy_priv_t object here that
     * contains a file descriptor and a \c struct libevdev instance.
     */
    void               *priv;
} joystick_device_t;


/** \brief  Host joystick driver object
 *
 * Methods to be called by the VICE core joystick code to open, poll and close
 * devices, and to clean up on detaching/shutdown.
 *
 * The arch-specific code is required to call \c joystick_driver_register() to
 * register itself on emulator startup. (TODO: maybe have the core code request
 * this somehow?).
 */
typedef struct joystick_driver_s {
    /** \brief  Open host device for use */
    bool (*open)     (joystick_device_t *);

    /** \brief  Poll host device */
    void (*poll)     (joystick_device_t *);

    /** \brief  Close host device */
    void (*close)    (joystick_device_t *);

    /** \brief  Optional method to free arch-specific device data */
    void (*priv_free)(void *);

    /** \brief  Function to call after registering a device
     *
     * This function is called after #joystick_device_register has processed
     * its argument. It can be used to customize mappings or calibration if so
     * required.
     */
    void (*customize)(joystick_device_t *);

} joystick_driver_t;


extern bool joystick_init_done;


int joystick_init(void);
int joystick_resources_init(void);
int joystick_cmdline_options_init(void);

/* SDL-specific functions. */
int joy_sdl_init(void);
int joy_sdl_resources_init(void);
int joy_sdl_cmdline_options_init(void);
void joystick_set_axis_value(unsigned int index, unsigned int axis, uint8_t value);


int joystick_check_set(signed long key, int keysetnum, unsigned int joyport);
int joystick_check_clr(signed long key, int keysetnum, unsigned int joyport);
void joystick_joypad_clear(void);

uint8_t joystick_get_axis_value(unsigned int port, unsigned int pot);

void joystick_set_value_absolute(unsigned int joyport, uint16_t value);
void joystick_set_value_or(unsigned int joyport, uint16_t value);
void joystick_set_value_and(unsigned int joyport, uint16_t value);
void joystick_clear(unsigned int joyport);
void joystick_clear_all(void);

void joystick_event_playback(CLOCK offset, void *data);
void joystick_event_delayed_playback(void *data);
void joystick_register_delay(unsigned int delay);

int joystick_joyport_register(void);

/* Replaced with joystick_arch_init()` */
#if 0
void linux_joystick_init(void);
void linux_joystick_evdev_init(void);
void bsd_joystick_init(void);
void joy_hidlib_init(void);
void joy_hidlib_exit(void);
void win32_joystick_init(void);
#endif


uint16_t get_joystick_value(int index);

typedef void (*joystick_machine_func_t)(void);

void joystick_register_machine(joystick_machine_func_t func);

/* the mapping of real devices to emulated joystick ports */
extern int joystick_port_map[JOYPORT_MAX_PORTS];

void joystick_set_snes_mapping(int port);

void joy_axis_event  (joystick_axis_t   *axis,   int32_t value);
void joy_button_event(joystick_button_t *button, int32_t value);
void joy_hat_event   (joystick_hat_t    *hat,    int32_t value);

void joystick(void);
void joystick_close(void);
void joystick_resources_shutdown(void);
void joystick_ui_reset_device_list(void);
const char *joystick_ui_get_next_device_name(int *id);
int joy_arch_mapping_dump(const char *filename);
int joy_arch_mapping_load(const char *filename);

int32_t joy_axis_prev(uint8_t joynum, uint8_t axis);

char *get_joy_pot_mapping_string(int joystick_device_num, int pot);
char *get_joy_pin_mapping_string(int joystick_device, int pin);
char *get_joy_extra_mapping_string(int which);
joystick_mapping_t *joy_get_axis_mapping(uint8_t joynum, uint8_t axis, joystick_axis_value_t value, joystick_axis_value_t *prev);
joystick_mapping_t *joy_get_axis_mapping_not_setting_value(uint8_t joynum, uint8_t axis, joystick_axis_value_t value);
joystick_mapping_t *joy_get_button_mapping(uint8_t joynum, uint8_t button, uint8_t value, uint8_t *prev);
joystick_mapping_t *joy_get_button_mapping_not_setting_value(uint8_t joynum, uint8_t button, uint8_t value);
joystick_axis_value_t joy_hat_prev(uint8_t joynum, uint8_t hat);
joystick_mapping_t *joy_get_hat_mapping(uint8_t joynum, uint8_t hat, uint8_t value, uint8_t *prev);
joystick_mapping_t *joy_get_hat_mapping_not_setting_value(uint8_t joynum, uint8_t hat, uint8_t value);
void joy_set_pot_mapping(int joystick_device_num, int axis, int pot);
void joy_delete_pin_mapping(int joystick_device, int pin);
void joy_delete_pot_mapping(int joystick_device, int pot);
#if (defined USE_SDLUI ||defined USE_SDL2UI)
void joy_delete_extra_mapping(int type);
#endif


/*
 * Arch-specific functions
 */

void joystick_arch_init    (void);
void joystick_arch_shutdown(void);



void               joystick_driver_register  (const joystick_driver_t *driver);

joystick_device_t *joystick_device_new       (void);
void               joystick_device_free      (joystick_device_t *joydev);
bool               joystick_device_register  (joystick_device_t *joydev);
bool               joystick_device_open      (joystick_device_t *joydev);
void               joystick_device_close     (joystick_device_t *joydev);

joystick_device_t *joystick_device_by_index  (int index);
int                joystick_device_count     (void);

void               joystick_device_set_name  (joystick_device_t *joydev,
                                              const char        *name);
void               joystick_device_set_node  (joystick_device_t *joydev,
                                              const char        *node);

void               joystick_device_add_axis  (joystick_device_t *joydev,
                                              joystick_axis_t   *axis);
void               joystick_device_add_button(joystick_device_t *joydev,
                                              joystick_button_t *button);
void               joystick_device_add_hat   (joystick_device_t *joydev,
                                              joystick_hat_t    *hat);
void               joystick_device_clear_mappings(joystick_device_t *joydev);

void               joystick_mapping_init     (joystick_mapping_t *mapping);
void               joystick_calibration_init (joystick_calibration_t *calibration);

joystick_axis_t   *joystick_axis_new         (const char *name);
joystick_axis_t   *joystick_axis_from_code   (joystick_device_t *joydev,
                                              uint32_t code);
void               joystick_axis_free        (joystick_axis_t *axis);
void               joystick_axis_clear_mappings(joystick_axis_t *axis);

joystick_button_t *joystick_button_new       (const char *name);
joystick_button_t *joystick_button_from_code (joystick_device_t *joydev,
                                              uint32_t code);
void               joystick_button_free      (joystick_button_t *button);
void               joystick_button_clear_mappings(joystick_button_t *button);

joystick_hat_t    *joystick_hat_new          (const char *name);
joystick_hat_t    *joystick_hat_from_code    (joystick_device_t *joydev,
                                              uint32_t code);
void               joystick_hat_free         (joystick_hat_t *hat);
void               joystick_hat_clear_mappings(joystick_hat_t *hat);

#endif
