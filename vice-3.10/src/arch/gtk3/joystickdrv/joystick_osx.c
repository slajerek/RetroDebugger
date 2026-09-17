/** \file   joystick_osx.c
 * \brief   Mac OS X joystick support using IOHIDManager
 *
 * \author  Christian Vogelgsang <chris@vogelgsang.org>
 * \author  David Hogan <david.q.hogan@gmail.com>
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

/*
 * NOTE: 2025-12-20 dqh: This implementation is a massive hack just to
 *                       get sane joystick support working in macOS. To
 *                       be reimplemented when the joystick subsystem
 *                       rebuild is finished.
 */

#define JOY_INTERNAL

#include "vice.h"

#include "joystick.h"
#include "log.h"
#include "lib.h"

#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDManager.h>

#define HID_CODE(page, usage) (((page) << 16) | (usage))

/*
 * This hat map was created from values observed on macOS 12 with PS4 and PS5 controller (bluetooth),
 * and based on various searches other controllers use this scheme. Xbox controllers are slightly
 * different which is handled via a hack later.
 */
#define MAX_HAT_MAP_INDEX 8
static const uint8_t hat_map[MAX_HAT_MAP_INDEX + 1] = {
    JOYSTICK_DIRECTION_UP,                              /* 0 */
    JOYSTICK_DIRECTION_UP | JOYSTICK_DIRECTION_RIGHT,   /* 1 */
    JOYSTICK_DIRECTION_RIGHT,                           /* 2 */
    JOYSTICK_DIRECTION_RIGHT | JOYSTICK_DIRECTION_DOWN, /* 3 */
    JOYSTICK_DIRECTION_DOWN,                            /* 4 */
    JOYSTICK_DIRECTION_DOWN | JOYSTICK_DIRECTION_LEFT,  /* 5 */
    JOYSTICK_DIRECTION_LEFT,                            /* 6 */
    JOYSTICK_DIRECTION_LEFT | JOYSTICK_DIRECTION_UP,    /* 7 */
    0,                                                  /* 8 */
};

/* ----- Statics ----- */

static IOHIDManagerRef mgr;

/* ----- Tools ----- */

static Boolean IOHIDDevice_GetLongProperty( IOHIDDeviceRef inIOHIDDeviceRef, CFStringRef inKey, long * outValue )
{
    Boolean result = FALSE;

    if ( inIOHIDDeviceRef ) {
        CFTypeRef tCFTypeRef = IOHIDDeviceGetProperty( inIOHIDDeviceRef, inKey );
        if ( tCFTypeRef ) {
            /* if this is a number */
            if ( CFNumberGetTypeID() == CFGetTypeID( tCFTypeRef ) ) {
                /* get it's value */
                result = CFNumberGetValue((CFNumberRef)tCFTypeRef, kCFNumberLongType, outValue);
            }
        }
    }
    return result;
}

/* HID Mgr Types */
typedef IOHIDDeviceRef  hid_device_ref_t;
typedef IOHIDElementRef hid_element_ref_t;

/* model a hid element */
struct joy_hid_element {
    int     usage_page;
    int     usage;

    int     min_pvalue; /* physical value */
    int     max_pvalue;
    int     min_lvalue; /* logical value */
    int     max_lvalue;

    hid_element_ref_t internal_element;
};
typedef struct joy_hid_element joy_hid_element_t;
typedef struct joy_hid_element *joy_hid_element_ptr_t;

/* model a hid device */
struct joy_hid_device {

    int                 num_elements;    /* number of elements in device */
    joy_hid_element_t   *elements;

    hid_device_ref_t    internal_device; /* pointer to native device */

    CFArrayRef          internal_elements;
};
typedef struct joy_hid_device joy_hid_device_t;
typedef struct joy_hid_device *joy_hid_device_ptr_t;

static int is_joystick(IOHIDDeviceRef ref)
{
    return
        IOHIDDeviceConformsTo( ref, kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick ) ||
        IOHIDDeviceConformsTo( ref, kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad );
}

static void joy_hidlib_close_device(joy_hid_device_t *device)
{
    /* close old device */
    if(device->internal_device != NULL) {
        IOHIDDeviceClose(device->internal_device, 0);
    }
}

static void joy_hidlib_process_element(IOHIDElementRef internal_element,
                                       joystick_device_t *joydev,
                                       joy_hid_element_t **elements_ptr,
                                       int *element_count,
                                       int *capacity)
{
    IOHIDElementType type = IOHIDElementGetType(internal_element);

    /* Recursively process collection elements */
    if (type == kIOHIDElementTypeCollection) {
        CFArrayRef children = IOHIDElementGetChildren(internal_element);
        if (children) {
            CFIndex child_count = CFArrayGetCount(children);
            for (CFIndex i = 0; i < child_count; i++) {
                IOHIDElementRef child = (IOHIDElementRef)CFArrayGetValueAtIndex(children, i);
                joy_hidlib_process_element(child, joydev, elements_ptr, element_count, capacity);
            }
        }
        return;
    }

    /* Only process input elements */
    if (type != kIOHIDElementTypeInput_Misc &&
        type != kIOHIDElementTypeInput_Button &&
        type != kIOHIDElementTypeInput_Axis) {
        return;
    }

    /* Expand array if needed */
    if (*element_count >= *capacity) {
        *capacity *= 2;
        *elements_ptr = lib_realloc(*elements_ptr, sizeof(joy_hid_element_t) * (*capacity));
    }

    joy_hid_element_t *e = &(*elements_ptr)[*element_count];
    (*element_count)++;

    uint32_t usage_page = IOHIDElementGetUsagePage(internal_element);
    uint32_t usage = IOHIDElementGetUsage(internal_element);
    CFIndex pmin = IOHIDElementGetPhysicalMin(internal_element);
    CFIndex pmax = IOHIDElementGetPhysicalMax(internal_element);
    CFIndex lmin = IOHIDElementGetLogicalMin(internal_element);
    CFIndex lmax = IOHIDElementGetLogicalMax(internal_element);

    e->usage_page = (int)usage_page;
    e->usage = (int)usage;
    e->min_pvalue = (int)pmin;
    e->max_pvalue = (int)pmax;
    e->min_lvalue = (int)lmin;
    e->max_lvalue = (int)lmax;
    e->internal_element = internal_element;

#if 0 /* Disabled until we have mapping UI*/
    code = HID_CODE(usage_page, usage);

    /* Process axes */
    if (usage_page == kHIDPage_GenericDesktop) {
        switch (usage) {
            case kHIDUsage_GD_X:
            case kHIDUsage_GD_Y:
            // case kHIDUsage_GD_Z:
            // case kHIDUsage_GD_Rx:
            // case kHIDUsage_GD_Ry:
            // case kHIDUsage_GD_Rz:
            // case kHIDUsage_GD_Slider:
                if (e->min_lvalue != e->max_lvalue) {
                    log_message(LOG_DEFAULT, "joy-hid: axis: usage_page=0x%x usage=0x%x pmin=%ld pmax=%ld lmin=%ld lmax=%ld",
                        usage_page, usage, pmin, pmax, lmin, lmax);

                    joystick_axis_t *axis = joystick_axis_new(NULL);
                    axis->code = HID_CODE(usage_page, usage);
                    axis->minimum = e->min_lvalue;
                    axis->maximum = e->max_lvalue;
                    joystick_device_add_axis(joydev, axis);
                }
                break;
            case kHIDUsage_GD_Hatswitch:
                log_message(LOG_DEFAULT, "joy-hid: hat: usage_page=0x%x usage=0x%x pmin=%ld pmax=%ld lmin=%ld lmax=%ld",
                    usage_page, usage, pmin, pmax, lmin, lmax);

                joystick_hat_t *hat = joystick_hat_new(NULL);
                hat->code = HID_CODE(usage_page, usage);
                joystick_device_add_hat(joydev, hat);
                break;
        }
    }
    else if (usage_page == kHIDPage_Button) {
        log_message(LOG_DEFAULT, "joy-hid: button: usage_page=0x%x usage=0x%x pmin=%ld pmax=%ld lmin=%ld lmax=%ld",
                    usage_page, usage, pmin, pmax, lmin, lmax);

        joystick_button_t *button = joystick_button_new(NULL);
        button->code = HID_CODE(usage_page, usage);
        joystick_device_add_button(joydev, button);
    }
#endif
}

static void joy_hidlib_device_specific_init_ps3(joystick_device_t *joydev)
{
    IOHIDDeviceRef dev = ((joy_hid_device_t *)joydev->priv)->internal_device;

    uint8_t report[49];
    CFIndex len;
    IOReturn r;

    /* Reading feature reports F2 and F5 "wakes up" the PS3 controller. */

    /* F2 feature report - 17 bytes of data */
    memset(report, 0, sizeof(report));
    len = 17;
    r = IOHIDDeviceGetReport(dev, kIOHIDReportTypeFeature, 0xF2, report, &len);
    if (r != kIOReturnSuccess) {
        log_message(LOG_LEVEL_ERROR, "joy-hid: PS3 controller: could not get FEATURE report F2 (r=%x)", (unsigned)r);
        /* Continue anyway, some clones might not support this */
    }

    /* F5 feature report - 8 bytes of data */
    memset(report, 0, sizeof(report));
    len = 8;
    r = IOHIDDeviceGetReport(dev, kIOHIDReportTypeFeature, 0xF5, report, &len);
    if (r != kIOReturnSuccess) {
        log_message(LOG_LEVEL_ERROR, "joy-hid: PS3 controller: could not get FEATURE report F5 (r=%x)", (unsigned)r);
        /* Continue anyway */
    }

    /*
     * Send output report to activate controller and attempt to turn off LEDs.
     * Setting LED bitmap to 0 should turn all LEDs off.
     *
     * The LED configuration bytes are required for the controller to function.
     */
    uint8_t output_report[48] = {
        0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00,
        0xff, 0x27, 0x10, 0x00, 0x32,  /* 10-14: LED 1 config */
        0xff, 0x27, 0x10, 0x00, 0x32,  /* 15-19: LED 2 config */
        0xff, 0x27, 0x10, 0x00, 0x32,  /* 20-24: LED 3 config */
        0xff, 0x27, 0x10, 0x00, 0x32,  /* 25-29: LED 4 config */
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00
    };

    len = sizeof(output_report);
    r = IOHIDDeviceSetReport(dev, kIOHIDReportTypeOutput, 0x01, output_report, len);
    if (r != kIOReturnSuccess) {
        log_message(LOG_LEVEL_ERROR, "joy-hid: PS3 controller: could not send output report (r=%x)", (unsigned)r);
    }
}

static void joy_hidlib_device_specific_init(joystick_device_t *joydev)
{
    /* PS3 controllers need an activation sequence. IDs taken from SDL2. */
    if (
            (joydev->vendor == 0x054c && joydev->product == 0x0268)
        ||  (joydev->vendor == 0x0925 && joydev->product == 0x0005)
        ||  (joydev->vendor == 0x8888 && joydev->product == 0x0308)
    ) {
        joy_hidlib_device_specific_init_ps3(joydev);
        return;
    }
}

static void joy_hidlib_enumerate_elements(joystick_device_t *joydev)
{
    joy_hid_device_t *device = (joy_hid_device_t *)joydev->priv;

    IOHIDDeviceRef dev = device->internal_device;
    if (dev == NULL) {
        return;
    }

    /* Get all elements (including collections) */
    CFArrayRef internal_elements = IOHIDDeviceCopyMatchingElements(dev, NULL, 0);
    if (!internal_elements) {
        return;
    }

    /* Start with initial capacity */
    int capacity = 32;
    int element_count = 0;
    joy_hid_element_t *elements = lib_malloc(sizeof(joy_hid_element_t) * capacity);

    /* Process all top-level elements recursively */
    CFIndex cnt = CFArrayGetCount(internal_elements);
    for (CFIndex i = 0; i < cnt; i++) {
        IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(internal_elements, i);
        joy_hidlib_process_element(element, joydev, &elements, &element_count, &capacity);
    }

    device->num_elements = element_count;
    device->internal_elements = internal_elements;
    device->elements = elements;

    /* We don't have mapping ui, so pretend each device is a simple joystick with a hat. */

    /* Add two axis, x and y, with codes 0 and 1 */
    joystick_axis_t *axis_x = joystick_axis_new(NULL);
    axis_x->code = 0;
    axis_x->minimum = 0;
    axis_x->maximum = 65535;
    axis_x->mapping.negative.action = JOY_ACTION_JOYSTICK;
    axis_x->mapping.negative.value.joy_pin = JOYSTICK_DIRECTION_LEFT;
    axis_x->mapping.positive.action = JOY_ACTION_JOYSTICK;
    axis_x->mapping.positive.value.joy_pin = JOYSTICK_DIRECTION_RIGHT;
    joystick_device_add_axis(joydev, axis_x);

    joystick_axis_t *axis_y = joystick_axis_new(NULL);
    axis_y->code = 1;
    axis_y->minimum = 0;
    axis_y->maximum = 65535;
    axis_y->mapping.negative.action = JOY_ACTION_JOYSTICK;
    axis_y->mapping.negative.value.joy_pin = JOYSTICK_DIRECTION_UP;
    axis_y->mapping.positive.action = JOY_ACTION_JOYSTICK;
    axis_y->mapping.positive.value.joy_pin = JOYSTICK_DIRECTION_DOWN;
    joystick_device_add_axis(joydev, axis_y);

    /* Add a single hat with code 0 for now */
    joystick_hat_t *hat = joystick_hat_new(NULL);
    hat->code = 0;
    joystick_device_add_hat(joydev, hat);

    /* Add a single button with code 0 for now */
    joystick_button_t *button = joystick_button_new(NULL);
    button->code = 0;
    joystick_device_add_button(joydev, button);
}

static void joy_hidlib_free_elements(joy_hid_device_t *device)
{
    if(device == NULL) {
        return;
    }
    if(device->elements) {
        lib_free(device->elements);
        device->elements = NULL;
    }
    if(device->internal_elements) {
        CFRelease(device->internal_elements);
        device->internal_elements = NULL;
    }
}

static int joy_hidlib_get_value(joy_hid_device_t *device,
                          joy_hid_element_t *element,
                          int *value, int phys)
{
    IOHIDValueRef value_ref;
    IOReturn result = IOHIDDeviceGetValue( device->internal_device,
                                           element->internal_element,
                                           &value_ref );
    if(result == kIOReturnSuccess) {
        if(phys) {
            *value = (int)IOHIDValueGetScaledValue( value_ref, kIOHIDValueScaleTypePhysical );
        } else {
            *value = (int)IOHIDValueGetIntegerValue( value_ref );
        }
        return 0;
    } else {
        return -1;
    }
}


static bool macos_joystick_open(joystick_device_t *joydev)
{
    return true;
}

static void macos_joystick_poll(joystick_device_t *joydev)
{
    joy_hid_device_t *device = (joy_hid_device_t *)joydev->priv;
    int i;
    int value;
    int buttons_pressed = 0;

    for (i = 0; i < device->num_elements; i++) {
        joy_hid_element_t e = device->elements[i];

        if(e.usage_page == kHIDPage_GenericDesktop) {
            switch(e.usage) {
            case kHIDUsage_GD_X:
                if (joy_hidlib_get_value(device, &e, &value, 0) >= 0) {
                    joystick_axis_t *axis_x = joystick_axis_from_code(joydev, 0);
                    if (axis_x != NULL) {
                        /* Normalize to axis range */
                        float normalized_value = (float)(value - e.min_lvalue) / (e.max_lvalue - e.min_lvalue);
                        int axis_value = (int)(normalized_value * (axis_x->maximum - axis_x->minimum) + axis_x->minimum);
                        joy_axis_event(axis_x, axis_value);
                    }
                }
                break;

            case kHIDUsage_GD_Y:
                if (joy_hidlib_get_value(device, &e, &value, 0) >= 0) {
                    joystick_axis_t *axis_y = joystick_axis_from_code(joydev, 1);
                    if (axis_y != NULL) {
                        /* Normalize to axis range */
                        float normalized_value = (float)(value - e.min_lvalue) / (e.max_lvalue - e.min_lvalue);
                        int axis_value = (int)(normalized_value * (axis_y->maximum - axis_y->minimum) + axis_y->minimum);
                        joy_axis_event(axis_y, axis_value);
                    }
                }
                break;

            case kHIDUsage_GD_Hatswitch:
                if (joy_hidlib_get_value(device, &e, &value, 0) >= 0) {
                    if (joydev->vendor == 0x45e) {
                        /*
                            * Microsoft device hack ... idea from godot source:
                            * https://github.com/godotengine/godot/blob/master/platform/osx/joypad_osx.cpp
                            *
                            * Basically the order is the same, but xbox starts with center rather than ends
                            * with it. Which makes more sense tbh.
                            *
                            * Anyway we'll just assume all Microsoft hats are like this rather than
                            * checking for product_id in (0x0b05, 0x02e0, 0x02fd, 0x0b13) like they do.
                            * If there are older exceptions, they should be handled, and we'll assume that
                            * newer devices won't change from this scheme. Tested with whatever controller
                            * came with Xbox Series X.
                            */
                        if (value) {
                            value--;
                        } else {
                            value = 8;
                        }
                    }

                    if (value >= 0 && value <= MAX_HAT_MAP_INDEX) {
                        joystick_hat_t *hat = joystick_hat_from_code(joydev, 0);
                        if (hat != NULL) {
                            joy_hat_event(hat, hat_map[value]);
                        }
                    }
                }
                break;
            }
        } else if (e.usage_page == kHIDPage_Button) {
            if (joy_hidlib_get_value(device, &e, &value, 0) >= 0) {
                if (value > 0)
                    buttons_pressed += 1;
            }
        }
    }

    /*
     * Until we have a joystick mapping UI we use this to turn all buttons
     * into a single button by counting pressed buttons.
     *
     * Yes this sucks but for controllers with many buttons this is better
     * than picking some arbitrary button and having that be the only one.
     */

    joystick_button_t *button = joystick_button_from_code(joydev, 0);
    if (button != NULL) {
        joy_button_event(button, buttons_pressed > 0 ? 1 : 0);
    }
}

static void macos_joystick_close(joystick_device_t *joydev)
{
}

static void macos_joystick_priv_free(void *priv)
{
    joy_hid_device_t *device = (joy_hid_device_t *)priv;
    joy_hidlib_free_elements(device);
    joy_hidlib_close_device(device);
    lib_free(device);
}

static joystick_driver_t osx_joystick_driver = {
    .open  = macos_joystick_open,
    .poll = macos_joystick_poll,
    .close = macos_joystick_close,
    .priv_free = macos_joystick_priv_free
};

static CFDictionaryRef CreateHIDDeviceMatchDictionary(const int page, const int usage)
{
    CFDictionaryRef retval = NULL;
    CFNumberRef pageNumRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &page);
    CFNumberRef usageNumRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
    const void *keys[2] = { (void *) CFSTR(kIOHIDDeviceUsagePageKey), (void *) CFSTR(kIOHIDDeviceUsageKey) };
    const void *vals[2] = { (void *) pageNumRef, (void *) usageNumRef };

    if (pageNumRef && usageNumRef) {
        retval = CFDictionaryCreate(kCFAllocatorDefault, keys, vals, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    }

    if (pageNumRef) {
        CFRelease(pageNumRef);
    }
    if (usageNumRef) {
        CFRelease(usageNumRef);
    }

    return retval;
}

static void hid_stable_id(IOHIDDeviceRef dev, char *out, size_t outlen)
{
    CFTypeRef t  = IOHIDDeviceGetProperty(dev, CFSTR(kIOHIDTransportKey));
    CFTypeRef v  = IOHIDDeviceGetProperty(dev, CFSTR(kIOHIDVendorIDKey));
    CFTypeRef p  = IOHIDDeviceGetProperty(dev, CFSTR(kIOHIDProductIDKey));
    CFTypeRef s  = IOHIDDeviceGetProperty(dev, CFSTR(kIOHIDSerialNumberKey));
    CFTypeRef l  = IOHIDDeviceGetProperty(dev, CFSTR(kIOHIDLocationIDKey));
    CFTypeRef pu = IOHIDDeviceGetProperty(dev, CFSTR(kIOHIDPhysicalDeviceUniqueIDKey));

    char tr[32] = "unknown", sn[128] = "", uid[256] = "";
    uint32_t vid = 0, pid = 0, loc = 0;

    if (t && CFGetTypeID(t) == CFStringGetTypeID())
        CFStringGetCString((CFStringRef)t, tr, sizeof tr, kCFStringEncodingUTF8);

    if (v && CFGetTypeID(v) == CFNumberGetTypeID())
        CFNumberGetValue((CFNumberRef)v, kCFNumberSInt32Type, &vid);
    if (p && CFGetTypeID(p) == CFNumberGetTypeID())
        CFNumberGetValue((CFNumberRef)p, kCFNumberSInt32Type, &pid);

    if (s && CFGetTypeID(s) == CFStringGetTypeID())
        CFStringGetCString((CFStringRef)s, sn, sizeof sn, kCFStringEncodingUTF8);

    if (l && CFGetTypeID(l) == CFNumberGetTypeID())
        CFNumberGetValue((CFNumberRef)l, kCFNumberSInt32Type, &loc);

    if (pu && CFGetTypeID(pu) == CFStringGetTypeID())
        CFStringGetCString((CFStringRef)pu, uid, sizeof uid, kCFStringEncodingUTF8);

    if (sn[0])
        snprintf(out, outlen, "%s:%04x:%04x@sn=%s", tr, vid, pid, sn);
    else if (!strcasecmp(tr, "USB") && loc)
        snprintf(out, outlen, "usb:%04x:%04x@loc=0x%08x", vid, pid, loc);
    else if (uid[0])
        snprintf(out, outlen, "%s:%04x:%04x@uid=%s", tr, vid, pid, uid);
    else
        snprintf(out, outlen, "%s:%04x:%04x", tr, vid, pid);
}

void joystick_arch_init(void)
{
    if ( !mgr ) {
        /* create the manager */
        mgr = IOHIDManagerCreate( kCFAllocatorDefault, 0L );
    }
    if( !mgr ) {
        return;
    }

    joystick_driver_register(&osx_joystick_driver);

    {
        const void *vals[] = {
            (void *) CreateHIDDeviceMatchDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick),
            (void *) CreateHIDDeviceMatchDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad),
            (void *) CreateHIDDeviceMatchDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_MultiAxisController),
        };
        CFArrayRef array = CFArrayCreate(kCFAllocatorDefault, vals, 3, &kCFTypeArrayCallBacks);

        IOHIDManagerSetDeviceMatchingMultiple(mgr, array);

        CFRelease(array);
        CFRelease(vals[0]);
        CFRelease(vals[1]);
        CFRelease(vals[2]);
    }

    /* open it */
    IOReturn tIOReturn = IOHIDManagerOpen( mgr, 0L);
    if ( kIOReturnSuccess != tIOReturn ) {
        return;
    }

    /* create set of devices */
    CFSetRef device_set = IOHIDManagerCopyDevices( mgr );
    if ( !device_set ) {
        return;
    }

    int i;
    CFIndex num_devices = CFSetGetCount( device_set );
    IOHIDDeviceRef *all_devices = lib_malloc(sizeof(IOHIDDeviceRef) * num_devices);
    CFSetGetValues(device_set, (const void **)all_devices);

    for ( i = 0; i < num_devices ; i++ ) {
        IOHIDDeviceRef dev = all_devices[i];
        if(is_joystick(dev)) {
            char buffer[256];
            joystick_device_t *joydev = joystick_device_new();

            CFStringRef product_key;
            product_key = IOHIDDeviceGetProperty( dev, CFSTR( kIOHIDProductKey ) );
            if(product_key && CFStringGetCString(product_key, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
                joydev->name = lib_strdup(buffer);
            } else {
                joydev->name = lib_strdup("N/A");
            }

            hid_stable_id(dev, buffer, sizeof(buffer));
            joydev->node        = lib_strdup(buffer);

            long vendor_id = 0;
            IOHIDDevice_GetLongProperty( dev, CFSTR( kIOHIDVendorIDKey ), &vendor_id );
            joydev->vendor = (uint16_t)vendor_id;

            long product_id = 0;
            IOHIDDevice_GetLongProperty( dev, CFSTR( kIOHIDProductIDKey ), &product_id );
            joydev->product = (uint16_t)product_id;

            joy_hid_device_t *joydev_priv   = lib_malloc(sizeof(joy_hid_device_t));
            joydev->priv                    = joydev_priv;
            joydev_priv->internal_device    = dev;

            if (kIOReturnSuccess != IOHIDDeviceOpen(dev, 0)) {
                log_message(LOG_LEVEL_ERROR, "joy-hid: could not open device %s (vendor: %04x, product: %04x)", buffer, joydev->vendor, joydev->product);
                joystick_device_free(joydev);
                continue;
            }

            joy_hidlib_device_specific_init(joydev);
            joy_hidlib_enumerate_elements(joydev);

            joystick_device_register(joydev);
        }
    }

    lib_free(all_devices);
    CFRelease( device_set );
}

void joystick_arch_shutdown(void)
{
    if(mgr) {
        IOHIDManagerClose( mgr, 0 );
        mgr = NULL;
    }
}
