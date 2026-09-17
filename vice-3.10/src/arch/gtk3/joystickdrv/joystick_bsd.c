/** \file   joystick_bsd.c
 * \brief   NetBSD/FreeBSD USB joystick support
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * \todo    Check if this code also works on OpenBSD.
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
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <usbhid.h>

#ifdef FREEBSD_COMPILE
/* for hid_* and HUG_* */
#include <dev/hid/hid.h>
/* for struct usb_device_info */
#include <dev/usb/usb_ioctl.h>
#endif

#ifdef NETBSD_COMPILE
#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>
#include <dev/hid/hid.h>

/* FreeBSD (9.3) doesn't have the D-Pad defines */
#ifndef HUG_D_PAD_UP
#define HUG_D_PAD_UP    0x0090
#endif
#ifndef HUG_D_PAD_DOWN
#define HUG_D_PAD_DOWN  0x0091
#endif
#ifndef HUG_D_PAD_RIGHT
#define HUG_D_PAD_RIGHT 0x0092
#endif
#ifndef HUG_D_PAD_LEFT
#define HUG_D_PAD_LEFT  0x0093
#endif

#endif  /* NETBSD_COMPILE */

#include "cmdline.h"
#include "joystick.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "types.h"
#include "util.h"

/* Constants used when calling scandir(3) and (re)constructing nodes in the
 * file system of HID devices
 */

/** \brief  Root directory of \c uhid* files */
#define ROOT_NODE       "/dev"

/** \brief  Length of the #ROOT_NODE */
#define ROOT_NODE_LEN   4

/** \brief  Prefix of HID files in the \c /dev virtual file system */
#define NODE_PREFIX     "uhid"

/** \brief  Length of the #NODE_PREIFX */
#define NODE_PREFIX_LEN 4


/** \brief  Driver-specific data
 */
typedef struct joy_priv_s {
    void          *buffer;          /**< buffer for reading HID data */
    report_desc_t  rep_desc;        /**< HID report descriptor */
    int            rep_size;        /**< size of \c rep_desc */
    int            rep_id;          /**< report ID */
    int            fd;              /**< file descriptor of HID device */
    int           *prev_axes;       /**< previous raw value of axes */
    int           *prev_buttons;    /**< previous raw value of buttons */
    int           *prev_hats;       /**< previous raw value of hats */
} joy_priv_t;


/* Forward declarations */
static bool        bsd_joy_open     (joystick_device_t *joydev);
static void        bsd_joy_poll     (joystick_device_t *joydev);
static void        bsd_joy_close    (joystick_device_t *joydev);
static void        bsd_joy_customize(joystick_device_t *joydev);
static joy_priv_t *joy_hid_open     (const char *node);
static void        joy_priv_free    (void *priv);


/** \brief  Log for BSD joystick driver */
static log_t bsd_joy_log;

/** \brief  BSD joystick driver declaration */
static joystick_driver_t driver = {
    .open      = bsd_joy_open,
    .poll      = bsd_joy_poll,
    .close     = bsd_joy_close,
    .customize = bsd_joy_customize,
    .priv_free = joy_priv_free
};


/** \brief  Allocate new private data object
 *
 * Allocate and initialize private data object instance.
 *
 * \return  new data object
 */
static joy_priv_t *joy_priv_new(void)
{
    joy_priv_t *priv = lib_malloc(sizeof *priv);

    priv->buffer       = NULL;
    priv->rep_desc     = NULL;
    priv->rep_size     = 0;
    priv->fd           = -1;
    priv->rep_id       = 0;
    priv->prev_axes    = NULL;
    priv->prev_buttons = NULL;
    priv->prev_hats    = NULL;
    return priv;
}

/** \brief  Free private data instance
 *
 * Free private data object and its resources.
 * Closes file descriptor, cleans up HID report descriptor and free HID buffer.
 *
 * \param[in]   priv    private data object
 */
static void joy_priv_free(void *priv)
{
    joy_priv_t *p = priv;

    if (p != NULL) {
       lib_free(p->buffer);
        if (p->rep_desc != NULL) {
            hid_dispose_report_desc(p->rep_desc);
        }
        lib_free(p->prev_axes);
        lib_free(p->prev_buttons);
        lib_free(p->prev_hats);
        if (p->fd >= 0) {
            close(p->fd);
        }
        lib_free(p);
    }
}

/** \brief  Open joystick device for polling
 *
 * \param[in]   joydev  joystick device
 */
static bool bsd_joy_open (joystick_device_t *joydev)
{
    if (joydev != NULL) {
        joy_priv_t *priv = joydev->priv;

        if (priv == NULL) {
            log_error(bsd_joy_log,
                      "%s(): failed to open device %s",
                      __func__, joydev->node);
            return false;   /* error opening device, already reported */
        }
        priv->fd = open(joydev->node, O_RDONLY|O_NONBLOCK);
        if (priv->fd < 0) {
            log_error(bsd_joy_log,
                      "%s(): failed to open device %s: %d: %s",
                      __func__, joydev->node, errno, strerror(errno));
            return false;
        }
        return true;
    }
    return false;
}

/** \brief  Poll joystick device
 *
 * \param[in]   joydev  joystick device
 */
static void bsd_joy_poll(joystick_device_t *joydev)
{
    joy_priv_t *priv = joydev->priv;

    if (priv != NULL && priv->fd >= 0) {
        ssize_t rsize;

        while ((rsize = read(priv->fd, priv->buffer, (size_t)priv->rep_size)) == priv->rep_size) {
            struct hid_data *data;
            struct hid_item  item;

            data = hid_start_parse(priv->rep_desc, 1 << hid_input, priv->rep_id);
            if (data == NULL) {
                return;
            }

            while (hid_get_item(data, &item) > 0) {
                joystick_axis_t   *axis;
                joystick_button_t *button;
                joystick_hat_t    *hat;
                int                value = hid_get_data(priv->buffer, &item);
                int                usage = HID_USAGE(item.usage);
                unsigned int       page  = HID_PAGE(item.usage);
                int                prev;

                switch (page) {
                    case HUP_GENERIC_DESKTOP:
                        switch (usage) {
                            case HUG_X:     /* fall through */
                            case HUG_Y:     /* fall through */
                            case HUG_Z:     /* fall through */
                            case HUG_RX:    /* fall through */
                            case HUG_RY:    /* fall through */
                            case HUG_RZ:    /* fall through */
                            case HUG_SLIDER:
                                /* axis */
                                axis = joystick_axis_from_code(joydev, (uint32_t)usage);
                                if (axis != NULL) {
                                    /* XXX: On my Logitech F710 the Y axis is inverted by
                                     *      FreeBSD, NetBSD just reports insane values.
                                     *      So for FreeBSD we'd need calibration to be
                                     *      implemented for the F710 to work.
                                     */
                                    prev = priv->prev_axes[axis->index];
                                    if (value != prev) {
                                        priv->prev_axes[axis->index] = value;
                                        joy_axis_event(axis, (int32_t)value);
                                    }
                                }
                                break;

                            case HUG_HAT_SWITCH:
                                /* hat */
                                hat = joystick_hat_from_code(joydev, (uint32_t)usage);
                                if (hat != NULL) {
                                    prev = priv->prev_hats[hat->index];
                                    if (prev != value) {
                                        priv->prev_hats[hat->index] = value;
                                        joy_hat_event(hat, (int32_t)value);
                                    }
                                }
                                break;

                            case HUG_D_PAD_UP:      /* fall through */
                            case HUG_D_PAD_DOWN:    /* fall through */
                            case HUG_D_PAD_LEFT:    /* fall through */
                            case HUG_D_PAD_RIGHT:
                                /* D-Pad is mapped as buttons */
                                button = joystick_button_from_code(joydev, (uint32_t)usage);
                                if (button != NULL) {
                                    prev = priv->prev_buttons[button->index];
                                    if (prev != value) {
                                        priv->prev_buttons[button->index] = value;
                                        joy_button_event(button, (int32_t)value);
                                    }
                                }
                                break;
                            default:
                                break;
                        }
                        break;
                    case HUP_BUTTON:
                        /* button event */
                        button = joystick_button_from_code(joydev, (uint32_t)usage);
                        if (button != NULL) {
                            prev = priv->prev_buttons[button->index];
                            if (prev != value) {
                                priv->prev_buttons[button->index] = value;
                                joy_button_event(button, (int32_t)value);
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
            hid_end_parse(data);
        }

        if (rsize != -1 && errno != EAGAIN) {
            log_warning(bsd_joy_log,
                        "weird report size: %zd: %s",
                        rsize, strerror(errno));
        }
    }
}

/** \brief  Close joystick device
 *
 * \param[in]   joydev  joystick device
 */
static void bsd_joy_close(joystick_device_t *joydev)
{
    if (joydev != NULL && joydev->priv != NULL) {
        joy_priv_t *priv = joydev->priv;

        if (priv->fd >= 0) {
            close(priv->fd);
            priv->fd = -1;
        }
    }
}


/** \brief  scandir select callback
 *
 * Check if name matches "uhid?*".
 *
 * \param[in]   de  directory entry
 *
 * \return  non-0 when matching "uhid?*"
 */
static int sd_select(const struct dirent *de)
{
    const char *name = de->d_name;

    return ((strlen(name) >= NODE_PREFIX_LEN + 1u) &&
            (strncmp(NODE_PREFIX, name, NODE_PREFIX_LEN) == 0));
}

/** \brief  Get full path of UHID device node
 *
 * \param[in]   node    node in /dev/
 *
 * \return  full path to \a node
 * \note    free with \c lib_free() after use
 */
static char *full_node_path(const char *node)
{
    size_t  nlen = strlen(node);
    size_t  plen = ROOT_NODE_LEN + 1u + nlen + 1u;
    char   *path = lib_malloc(plen);

    memcpy(path, ROOT_NODE, ROOT_NODE_LEN);
    path[ROOT_NODE_LEN] = '/';
    memcpy(path + ROOT_NODE_LEN + 1, node, nlen + 1u);
    return path;
}

/** \brief  Open device for HID usage
 *
 * Open \c node and get associated HID data from it for scanning/polling.
 * This function opens the device and allocates a buffer for HID reports, gets
 * the report ID and size and allocates a \c joy_priv_t instance with all that
 * data.
 *
 * \param[in]   node    path in \c /dev/ to the device
 *
 * \return  new initialized \c joy_priv_t instance or \c NULL on error
 */
static joy_priv_t *joy_hid_open(const char *node)
{
    joy_priv_t    *priv;
    report_desc_t  rep_desc;
    int            rep_id;
    int            rep_size;
    int            fd;

    fd = open(node, O_RDONLY|O_NONBLOCK);
    if (fd < 0) {
        /* don't log, (Net)BSD allocates a lot of nodes in /dev that aren't
         * actually valid */
        return NULL;
    }

    /* get report ID if possible, else asume 0 */
#ifdef USB_GET_REPORT_ID
    if (ioctl(fd, USB_GET_REPORT_ID, &rep_id) < 0) {
        log_warning(bsd_joy_log, "USB_GET_REPORT_ID failed.");
        close(fd);
        return NULL;
    }
#else
    rep_id = 0;
#endif

    /* get report description */
    rep_desc = hid_get_report_desc(fd);
    if (rep_desc == NULL) {
        log_error(bsd_joy_log,
                  "failed to get HID report for %s: %s",
                  node, strerror(errno));
        close(fd);
        return NULL;
    }

    /* get report size */
    rep_size = hid_report_size(rep_desc, hid_input, rep_id);
    if (rep_size <= 0) {
        log_error(bsd_joy_log, "invalid report size of %d", rep_size);
        hid_dispose_report_desc(rep_desc);
        close(fd);
        return NULL;
    }

    /* success: allocate private data object and store what we need for polling
     * and further querying */
    priv = joy_priv_new();
    priv->buffer       = lib_malloc((size_t)rep_size);
    priv->rep_desc     = rep_desc;
    priv->rep_size     = rep_size;
    priv->rep_id       = rep_id;
    priv->fd           = fd;

    return priv;
}

/** \brief  Add axis to joystick device
 *
 * \param[in]   joydev  joystick device
 * \param[in]   item    HID item with axis information
 *
 */
static void add_joy_axis(joystick_device_t     *joydev,
                         const struct hid_item *item)
{
    joystick_axis_t *axis;

    axis = joystick_axis_new(hid_usage_in_page(item->usage));
    axis->code    = (uint32_t)HID_USAGE(item->usage);
    axis->minimum = item->logical_minimum;
    axis->maximum = item->logical_maximum;
#if 0
    log_message(bsd_joy_log, "axis %u: %s (%d-%d)",
                axis->code, axis->name, axis->minimum, axis->maximum);
#endif
    joystick_device_add_axis(joydev, axis);
}

/** \brief  Add button to joystick device
 *
 * \param[in]   joydev  joystick device
 * \param[in]   item    HID item with button information
 */
static void add_joy_button(joystick_device_t     *joydev,
                           const struct hid_item *item)
{
    joystick_button_t *button;

    button = joystick_button_new(hid_usage_in_page(item->usage));
    button->code = (uint32_t)HID_USAGE(item->usage);

    log_message(bsd_joy_log, "button %u: %s", button->code, button->name);
    joystick_device_add_button(joydev, button);
}

/** \brief  Add hat to joystick device
 *
 * \param[in]   joydev  joystick device
 * \param[in]   item    HID item with hat information
 */
static void add_joy_hat(joystick_device_t     *joydev,
                           const struct hid_item *item)
{
    joystick_hat_t *hat;

    hat = joystick_hat_new(hid_usage_in_page(item->usage));
    hat->code = (uint32_t)HID_USAGE(item->usage);

    log_message(bsd_joy_log, "hat %u: %s", hat->code, hat->name);
    joystick_device_add_hat(joydev, hat);
}

/** \brief  Scan device for inputs
 *
 * Scan \a joydev for axes, buttons and hats and register them with \a joydev.
 *
 * \param[in]   joydev  joystick device
 *
 * \return  \c true on succes
 */
static bool scan_inputs(joystick_device_t *joydev)
{
    joy_priv_t      *priv;
    struct hid_data *hdata;
    struct hid_item  hitem;

    priv  = joydev->priv;
    hdata = hid_start_parse(priv->rep_desc, 1 << hid_input, priv->rep_id);
    if (hdata == NULL) {
        log_error(bsd_joy_log, "hid_start_parse() failed: %s,", strerror(errno));
        return false;
    }

    while (hid_get_item(hdata, &hitem) > 0) {
        unsigned int page  = HID_PAGE (hitem.usage);
        int          usage = HID_USAGE(hitem.usage);

        switch (page) {
            case HUP_GENERIC_DESKTOP:
                switch (usage) {
                    case HUG_X:     /* fall through */
                    case HUG_Y:     /* fall through */
                    case HUG_Z:     /* fall through */
                    case HUG_RX:    /* fall through */
                    case HUG_RY:    /* fall through */
                    case HUG_RZ:    /* fall through */
                    case HUG_SLIDER:
                        /* got an axis */
                        add_joy_axis(joydev, &hitem);
                        break;
                    case HUG_HAT_SWITCH:
                        /* hat, seems to be D-Pad on Logitech F710 */
                        add_joy_hat(joydev, &hitem);
                        break;
                    case HUG_D_PAD_UP:      /* fall through */
                    case HUG_D_PAD_DOWN:    /* fall through */
                    case HUG_D_PAD_LEFT:    /* fall through */
                    case HUG_D_PAD_RIGHT:
                        /* treat D-Pad as buttons */
                        add_joy_button(joydev, &hitem);
                        break;
                    default:
                        break;
                }
                break;
            case HUP_BUTTON:
                /* usage appears to be the button number */
                add_joy_button(joydev, &hitem);
                break;
            default:
                break;
        }
    }

    hid_end_parse(hdata);
    return true;
}


/** \brief  Scan joystick device for capabilities
 *
 * \param[in]   node    device node to scan
 *
 * \return  new joystick device instance or <tt>NULL</tt> on error
 */
static joystick_device_t *scan_device(const char *node)
{
    joystick_device_t      *joydev;
    joy_priv_t             *priv;
    struct usb_device_info  devinfo;
    char                   *name;

    /* try to open device and get HID report information */
    priv = joy_hid_open(node);
    if (priv == NULL) {
        return NULL;
    }

    /* get device info for vendor and product */
    if (ioctl(priv->fd, USB_GET_DEVICEINFO, &devinfo) < 0) {
        log_error(bsd_joy_log, "failed to get USB device info: %s", strerror(errno));
        joy_priv_free(priv);
        return NULL;
    }
    if (*devinfo.udi_vendor == '\0' && *devinfo.udi_product == '\0') {
        /* fall back to device node as name */
        name = lib_strdup(node);
    } else {
        name = util_concat(devinfo.udi_vendor, " ", devinfo.udi_product, NULL);
    }

    /* now we can allocate the joystick device instance and its data */
    joydev          = joystick_device_new();
    joydev->node    = lib_strdup(node);
    joydev->name    = name;
    joydev->vendor  = devinfo.udi_vendorNo;
    joydev->product = devinfo.udi_productNo;

    joydev->priv = priv;
    return joydev;
}


/** \brief  Initialize BSD joystick driver and add available devices
 */
void joystick_arch_init(void)
{
    struct dirent **namelist = NULL;
    int             nl_count;
    int             n;

    bsd_joy_log = log_open("BSD Joystick");
    log_message(bsd_joy_log, "Registering driver.");
    joystick_driver_register(&driver);


    /* Initialize HID library so we can retrieve strings for page and usage;
     * without this button names will be "0x00001" etc, not very informative.
     * (Parses /usr/share/misc/ubs_hid_usages on FreeBSD)
     */
    hid_init(NULL);

    log_message(bsd_joy_log, "Scanning available devices:");
    nl_count = scandir(ROOT_NODE, &namelist, sd_select, NULL);
    if (nl_count < 0) {
        log_warning(bsd_joy_log,
                    "scandir(\"%s/%s?*\") failed, giving up.",
                    ROOT_NODE, NODE_PREFIX);
        return;
    } else if (nl_count == 0) {
        log_message(bsd_joy_log, "no devices found.");
        return;
    }

    /* scan uhid device nodes and register valid joystick devices */
    for (n = 0; n < nl_count; n++) {
        joystick_device_t *joydev;
        char              *node;

        node   = full_node_path(namelist[n]->d_name);
        joydev = scan_device(node);
        if (joydev != NULL) {
            joy_priv_t *priv;

            log_message(bsd_joy_log, "%s: %s", joydev->node, joydev->name);

            /* scan axes, buttons and hats */
            if (scan_inputs(joydev)) {
                /* OK: try to register */
                if (!joystick_device_register(joydev)) {
                    /* failure */
                    log_warning(bsd_joy_log,
                                "failed to register device %s (\"%s\")",
                                joydev->node, joydev->name);
                    joystick_device_free(joydev);
                } else {
                    priv = joydev->priv;
                    /* allocate arrays for previous input states */
                    priv->prev_axes    = lib_calloc((size_t)joydev->num_axes,
                                                    sizeof *priv->prev_axes);
                    priv->prev_buttons = lib_calloc((size_t)joydev->num_buttons,
                                                    sizeof *priv->prev_buttons);
                    priv->prev_hats    = lib_calloc((size_t)joydev->num_hats,
                                                    sizeof *priv->prev_hats);
                }
            } else {
                /* failure while scanning: log and free invalid device */
                log_warning(bsd_joy_log,
                            "failed to scan inputs for device %s (\"%s\")",
                            joydev->node, joydev->name);
                joystick_device_free(joydev);
            }

            /* close device */
            priv = joydev->priv;
            close(priv->fd);
        }
        lib_free(node);
        free(namelist[n]);
    }

    free(namelist);
}


/** \brief  Driver-specific cleanup
 *
 * Currently doesn't do anything, just here to satisfy the API
 */
void joystick_arch_shutdown(void)
{
    /* NOP */
}


/** \brief  Custom mapper/calibrator
 *
 * \param[in]   joydev  joystick device
 */
static void bsd_joy_customize(joystick_device_t *joydev)
{
#ifdef FREEBSD_COMPILE
    if (joydev->vendor == 0x046d && joydev->product == 0xc21f) {
        joystick_axis_t *left_thumb_y;

        /* Logitech F710 (XInput mode) */

        /* For some reason the Y axis of the left thumbstick is inverted on at
         * least FreeBSD 14.2. On NetBSD 10.1 the device doesn't appear to
         * function at all */
        left_thumb_y = joydev->axes[1];
        left_thumb_y->calibration.invert = true;
    }
#endif
}
