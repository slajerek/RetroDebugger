/** \file   joystick_linux_evdev.c
 * \brief   Linux joystick driver using evdev
 *
 * Experimental Linux evdev joystick driver.
 *
 * The Linux /dev/js* API has been superceeded since many years by evdev. In
 * addition libevdev allows use to easily obtain information on event sources,
 * such as range, deadzone and fuzz and device information such as vendor,
 * product and version.
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libevdev/libevdev.h>
#include <limits.h>
#include <linux/input.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "debug_gtk3.h"
#include "joystick.h"
#include "lib.h"
#include "log.h"


/** \brief  Array length helper */
#define ARRAY_LEN(arr) (sizeof arr / sizeof arr[0])

/** \brief  Minimum valid axis event code */
#define AXIS_CODE_MIN   ABS_X

/** \brief  Maximum valid axis event code */
#define AXIS_CODE_MAX   (ABS_RESERVED - 1u)

/** \brief  Minimum valid button event code */
#define BUTTON_CODE_MIN BTN_JOYSTICK

/** \brief  Maximum valid button event code */
#define BUTTON_CODE_MAX BTN_THUMBR

/** \brief  Maximum number of axis event codes */
#define NUM_AXES_MAX    (AXIS_CODE_MAX - AXIS_CODE_MIN + 1u)

/** \brief  Maximum number of button event codes */
#define NUM_BUTTONS_MAX (BUTTON_CODE_MAX - BUTTON_CODE_MIN + 1u)


/** \brief  Driver-specific joystick data
 *
 * Contains data required by the evdev driver that isn't stored in the generic
 * joystick code.
 *
 * Unlike the other joystick drivers in VICE, Linux' event device API doesn't
 * use 0-based indexes for axes and buttons, but event types and event codes
 * in a certain range. So we need to keep an array of valid event codes for
 * buttons and axes, as well as axis minimum/maximum values since these aren't
 * always reported in the INT16_MIN to INT16_MAX range.
 */
typedef struct joy_priv_s {
    struct libevdev *evdev;                     /**< evdev instance */
    int              fd;                        /**< file descriptor */
} joy_priv_t;


/** \brief  Log used for the Linux evdev driver */
static log_t joy_evdev_log;


/** \brief  Allocate and initialize driver-specific joystick data
 *
 * \return  new data instance, free with \c lib_free()
 */
static joy_priv_t *joy_priv_new(void)
{
    joy_priv_t *priv;

    priv = lib_malloc(sizeof *priv);
    priv->fd    = -1;
    priv->evdev = NULL;
    return priv;
}

/** \brief  Free driver-specific joystick data
 *
 * Close file descriptor, free evdev instance and other data.
 *
 * \param[in]   priv    driver-specific joystick data
 */
static void joy_priv_free(void *priv)
{
    if (priv != NULL) {
        joy_priv_t *p = priv;

        libevdev_free(p->evdev);
        close(p->fd);
        lib_free(p);
    }
}

/** \brief  Dispatcher for joystick events
 *
 * \param[in]   joydev  joystick device instance
 * \param[in]   event   event data
 */
static void dispatch_event(joystick_device_t  *joydev, struct input_event *event)
{
    if (event->type == EV_KEY) {
        joystick_button_t *button;

        printf("button %02x (%s): %d\n",
               event->code, libevdev_event_code_get_name(EV_KEY, event->code),
               event->value);

        button = joystick_button_from_code(joydev, event->code);
        if (button != NULL) {
            joy_button_event(button, event->value);
        }

    } else if (event->type == EV_ABS) {
        joystick_axis_t *axis;
#if 0
        printf("axis %02x (%s): %d\n",
               event->code, libevdev_event_code_get_name(EV_ABS, event->code),
               event->value);
#endif
        axis = joystick_axis_from_code(joydev, event->code);
        if (axis != NULL) {
            joy_axis_event(axis, event->value);
        }
    }
}

/** \brief  Open callback for the joystick system
 *
 * Open joystick for polling.
 *
 * \param[in]   joydev  joystick device
 *
 * \return  \c true on success (also when \a joydev was already opened)
 */
static bool linux_joystick_evdev_open(joystick_device_t *joydev)
{
    struct libevdev *evdev;
    joy_priv_t      *priv;
    int              fd;
    int              rc;

    if (joydev == NULL || joydev->node == NULL) {
        return false;
    }

    priv = joydev->priv;
    if (priv->fd >= 0) {
        /* already opened */
        return true;
    }

    fd = open(joydev->node, O_RDONLY|O_NONBLOCK);
    if (fd < 0) {
        return false;
    }
    priv->fd = fd;

    /* get evdev instance from file descriptor */
    rc = libevdev_new_from_fd(fd, &evdev);
    if (rc < 0) {
        log_error(LOG_DEFAULT, "failed to initialize libevdev: %s", strerror(rc));
        close(fd);
        return false;
    }
    priv->evdev = evdev;

    return true;
}

/** \brief  Poll callback for the joystick system
 *
 * \param[in]   joydev  joystick device
 */
static void linux_joystick_evdev_poll(joystick_device_t *joydev)
{
    struct libevdev *evdev;
    joy_priv_t      *priv;
    int              rc;
    unsigned int     flags = LIBEVDEV_READ_FLAG_NORMAL;

    priv = joydev->priv;
    if (priv == NULL || priv->fd < 0 || priv->evdev == NULL) {
        return;
    }
    evdev = priv->evdev;

    while (libevdev_has_event_pending(evdev)) {
        struct input_event event;

        rc = libevdev_next_event(evdev, flags, &event);
        if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            while (rc == LIBEVDEV_READ_STATUS_SYNC) {
                rc = libevdev_next_event(evdev, flags, &event);
            }
        } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            if (event.type == EV_ABS || event.type == EV_KEY) {
                dispatch_event(joydev, &event);
            }
        }
    }
}

/** \brief  Close callback for the joystick system
 *
 * Release resources associated with the joystick.
 *
 * \param[in]   joydev  joystick data device
 */
static void linux_joystick_evdev_close(joystick_device_t *joydev)
{
    if (joydev != NULL && joydev->priv != NULL) {
        joy_priv_t *priv = joydev->priv;

        if (priv->fd >= 0) {
            close(priv->fd);
        }
        if (priv->evdev != NULL) {
            libevdev_free(priv->evdev);
        }
        priv->fd    = -1;
        priv->evdev = NULL;
    }
}

/** \brief  Custom mapping/calibration callback
 *
 * Just for debugging/testing right now.
 *
 * \param[in]   joydev  joystick device
 */
static void linux_joystick_customize(joystick_device_t *joydev)
{
#if 0
    printf("%s() called for device %04x:%04x\n",
           __func__, (unsigned int)joydev->vendor, (unsigned int)joydev->product);

    if (joydev->vendor == 0x46d && joydev->product == 0xc21f) {
        printf("%s(): inverting Y axis for F710\n", __func__);
        joydev->axes[1]->calibration.invert = true;
    }
#endif
}

/** \brief  Filter callback for scandir(3)
 *
 * \param[in]   de  dirent instance
 *
 * \return  \c 1 if the directory entry matches 'event*', \c 0 otherwise
 */
static int sd_filter(const struct dirent *de)
{
    /* on Debian 12.10 joystick devices end with "-event-joystick", no idea if
     * this is also true on other Linux distros, needs checking! */
    return strstr(de->d_name, "-event-joystick") != NULL;
}

/** \brief  Scan device for available buttons
 *
 * \param[in]   joydev  joystick device instance
 * \param[in]   evdev   libevdev instance
 */
static void scan_buttons(joystick_device_t *joydev, struct libevdev *evdev)
{
    if (libevdev_has_event_type(evdev, EV_KEY)) {
        uint32_t code;

        for (code = BUTTON_CODE_MIN; code <= BUTTON_CODE_MAX; code++) {
            if (libevdev_has_event_code(evdev, EV_KEY, code)) {
                joystick_button_t *button;

                button = joystick_button_new(libevdev_event_code_get_name(EV_KEY, code));
                button->code = code;
                /* joydev takes ownership of button */
                joystick_device_add_button(joydev, button);
            }
        }
    }
}

/** \brief  Scan device for available axes
 *
 * \param[in]   joydev  joystick device instance
 * \param[in]   evdev   libevdev instance
 */
static void scan_axes(joystick_device_t *joydev, struct libevdev *evdev)
{
    if (libevdev_has_event_type(evdev, EV_ABS)) {
        uint32_t code;

        for (code = AXIS_CODE_MIN; code <= AXIS_CODE_MAX; code++) {
            if (libevdev_has_event_code(evdev, EV_ABS, code)) {
                const struct input_absinfo *info;
                joystick_axis_t            *axis;

                info = libevdev_get_abs_info(evdev, code);
                axis = joystick_axis_new(libevdev_event_code_get_name(EV_ABS, code));
                axis->code = code;
                if (info != NULL) {
                    axis->minimum = info->minimum;
                    axis->maximum = info->maximum;
                }
                /* joydev takes ownership of axis */
                joystick_device_add_axis(joydev, axis);
            }
        }
    }
}

static joystick_device_t *scan_device(const char *node)
{
    joystick_device_t *joydev;
    struct libevdev   *evdev;
    joy_priv_t        *priv;
    char               path[1024];
    int                fd;
    int                rc;

    snprintf(path, sizeof path, "/dev/input/by-id/%s", node);
    fd = open(path, O_RDONLY|O_NONBLOCK);
    if (fd < 0) {
        return NULL;
    }

    /* get evdev instance from file descriptor */
    rc = libevdev_new_from_fd(fd, &evdev);
    if (rc < 0) {
        log_error(LOG_DEFAULT, "failed to initialize libevdev: %s", strerror(rc));
        close(fd);
        return NULL;
    }

    /* create new joystick device instance and add data */
    joydev = joystick_device_new();
    joydev->name        = lib_strdup(libevdev_get_name(evdev));
    joydev->node        = lib_strdup(path);
    joydev->vendor      = (uint16_t)libevdev_get_id_vendor(evdev);
    joydev->product     = (uint16_t)libevdev_get_id_product(evdev);

    priv = joy_priv_new();
    joydev->priv = priv;

    /* scan for valid inputs */
    scan_buttons(joydev, evdev);
    scan_axes(joydev, evdev);

    /* clean up */
    libevdev_free(evdev);
    close(fd);

    return joydev;
}


/** \brief  Object used to register driver for devices
 *
 * The address of this object is used in the joystick code, it isn't copied,
 * so we cannot move this into `linux_joystick_init()` to use for registration.
 */
static joystick_driver_t driver = {
    .open      = linux_joystick_evdev_open,
    .poll      = linux_joystick_evdev_poll,
    .close     = linux_joystick_evdev_close,
    .priv_free = joy_priv_free,
    .customize = linux_joystick_customize
};


/** \brief  Initialize Linux evdev joystick driver
 *
 * Scan device nodes in \c /dev/input/ for supported joysticks/gamepads.
 */
void joystick_arch_init(void)
{
    struct dirent **namelist = NULL;
    int             sd_result;
    int             i;

    joy_evdev_log = log_open("evdev Joystick");

    log_message(joy_evdev_log, "Initializing Linux evdev joystick driver.");
    joystick_driver_register(&driver);

    sd_result = scandir("/dev/input/by-id", &namelist, sd_filter, alphasort);
    if (sd_result < 0) {
        log_error(LOG_DEFAULT, "scandir() failed on /dev/input: %s", strerror(errno));
        return;
    }

    for (i = 0; i < sd_result; i++) {
        joystick_device_t *joydev;

        //log_message(joy_evdev_log, "Possible device '%s'", namelist[i]->d_name);
        joydev = scan_device(namelist[i]->d_name);
        if (joydev != NULL) {
            joystick_device_register(joydev);
#if 0
            /* open joystick: REMOVE once we have opening/closing via resource
             * and manually implemented properly */
            linux_joystick_evdev_open(joydev);
#endif
      }
    }
    free(namelist);
}


/** \brief  Linux evdev-specific shutdown
 *
 * Runs after `joystick_close()` closed and freed all devices.
 */
void joystick_arch_shutdown(void)
{
    /* printf("%s(): called\n", __func__); */
}
