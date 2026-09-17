/** \file   joystick_win32_directinput.c
 * \brief   Joystick support for Windows
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
#include <stdbool.h>
#include <stdint.h>
#include "joyport.h"
#include "joystick.h"
#include "lib.h"

#define DIRECTINPUT_VERSION 0x0800
/* required for any GUID stuff, spectacular linker errors on MSYS2 without it */
#define INITGUID
#include <dinput.h>

/*
 * New API
 */

/** \brief  Maximum number of axes in a DIJOYSTATE2 struct */
#define DIJS2_MAX_AXES       24

/** \brief  Maximum number of buttons in a DIJOYSTATE2 struct */
#define DIJS2_MAX_BUTTONS   128

/** \brief  Maximum number of hat (POVs) in a DIJOYSTATE2 struct */
#define DIJS2_MAX_HATS        4

/** \brief  Private data object
 *
 * Contains arch-specific data of a joystick device.
 */
typedef struct joy_priv_s {
    GUID                 guid;      /**< GUID */
    LPDIRECTINPUTDEVICE8 didev;     /**< DirectInput device instance */
    bool                 acquired;  /**< device has been acquired (DirectInput
                                         doesn't provide any method to determine
                                         if a device is acquired) */
    LONG                 prev_axes[DIJS2_MAX_AXES];         /**< prev state of axes */
    BYTE                 prev_buttons[DIJS2_MAX_BUTTONS];   /**< prev state of buttons */
    WORD                 prev_hats[DIJS2_MAX_HATS];         /**< prev state of POVs */
} joy_priv_t;


/** \brief  DirectInput8 handle
 *
 * Initialized during subsystem init.
 */
static LPDIRECTINPUT8 dinput_handle = NULL;

/** \brief  Window handle */
static HINSTANCE window_handle = NULL;

/** \brief  Log destination for the driver */
static log_t winjoy_log = LOG_DEFAULT;


/** \brief  Allocate and initialize private data object
 *
 * \return  new private data object
 */
static joy_priv_t *joy_priv_new(void)
{
    joy_priv_t *priv = lib_calloc(sizeof *priv, 1);
    priv->didev    = NULL;
    priv->acquired = false;
    /* set POV values to neutral (0xffff) */
    memset(priv->prev_hats, 0xff, sizeof priv->prev_hats);
    return priv;
}

/** \brief  Free private data object and its resources
 *
 * Unacquire() and Release() the DirectInput device inside \a priv and free
 * \a priv.
 *
 * \param[in]   priv    private data object
 */
static void joy_priv_free(void *priv)
{
    joy_priv_t *pr = priv;
    if (pr->didev != NULL) {
        /* it's safe to call Unacquire() on an unacquired device */
        IDirectInputDevice8_Unacquire(pr->didev);
        IDirectInputDevice8_Release(pr->didev);
    }
    lib_free(priv);
}


/*
 * Driver methods
 */

/** \brief  Open joystick device for polling
 *
 * \param[in]   joydev  joystick device
 *
 * \return  \c true on success, \c false on failure
 */
static bool win32_joy_open(joystick_device_t *joydev)
{
    joy_priv_t *priv = joydev->priv;
    HRESULT     result;

    result = IDirectInputDevice8_Acquire(priv->didev);
    if (SUCCEEDED(result)) {
        priv->acquired = true;
    } else {
        log_error(winjoy_log, "failed to acquire device \"%s\": error 0x%08lx",
                  joydev->name, (unsigned long)result);
        priv->acquired = false;
    }
    return priv->acquired;
}

/** \brief  Joystick drive poll() method
 *
 * \param[in]   joydev  joystick device to poll
 */
static void win32_joy_poll(joystick_device_t *joydev)
{
    joy_priv_t  *priv;
    DIJOYSTATE2  jstate;
    /* map axis instance (dwType & 0xff) IDs to their position in the joystick
     * state DIJOYSTATE2 struct: */
    LONG        *axis_map[] = {
        &jstate.lX,   &jstate.lY,   &jstate.lZ,
        &jstate.lRx,  &jstate.lRy,  &jstate.lRz,
        &jstate.lVX,  &jstate.lVY,  &jstate.lVZ,
        &jstate.lVRx, &jstate.lVRy, &jstate.lVRz,
        &jstate.lAX,  &jstate.lAY,  &jstate.lAZ,
        &jstate.lARx, &jstate.lARy, &jstate.lARz,
        &jstate.lFX,  &jstate.lFY,  &jstate.lFZ,
        &jstate.lFRx, &jstate.lFRy, &jstate.lFRz
    };
    /* mapping of directions to eight sections of 45 degrees */
    static const int32_t hat_map[36000/4500] = {
        JOYSTICK_DIRECTION_UP,
        JOYSTICK_DIRECTION_UP|JOYSTICK_DIRECTION_RIGHT,
        JOYSTICK_DIRECTION_RIGHT,
        JOYSTICK_DIRECTION_DOWN|JOYSTICK_DIRECTION_RIGHT,
        JOYSTICK_DIRECTION_DOWN,
        JOYSTICK_DIRECTION_DOWN|JOYSTICK_DIRECTION_LEFT,
        JOYSTICK_DIRECTION_LEFT,
        JOYSTICK_DIRECTION_UP|JOYSTICK_DIRECTION_LEFT
    };
    int     i;
    HRESULT result;

    /* poll device */
    priv = joydev->priv;
    if (!priv->acquired) {
        /* not open, done */
        return;
    }
    result = IDirectInputDevice8_Poll(priv->didev);
    if (result != DI_OK && result != DI_NOEFFECT) {
        return;
    }

    /* get device state */
    result = IDirectInputDevice8_GetDeviceState(priv->didev, sizeof(jstate), &jstate);
    if (result != DI_OK) {
        return;
    }

    /* handle buttons */
    for (i = 0; i < joydev->num_buttons; i++) {
        /* buttons are simply listed in sequence, starting at 0 */
        joystick_button_t *button = joydev->buttons[i];
        int32_t            value  = jstate.rgbButtons[i] & 0x80;
        BYTE               prev   = priv->prev_buttons[i];

        if (prev == value) {
            continue;
        }
        priv->prev_buttons[i] = (BYTE)value;

        joy_button_event(button, value);
    }

    /* handle axes */
    for (i = 0; i < joydev->num_axes; i++) {
        joystick_axis_t *axis = joydev->axes[i];
        uint32_t         code = axis->code;
        LONG             prev = priv->prev_axes[i];

        if (code < sizeof axis_map / sizeof axis_map[0]) {
            LONG value   = *(axis_map[code]);

            if (value != prev) {
                priv->prev_axes[i] = value;
                joy_axis_event(axis, (int32_t)value);
            }
        }
    }

    /* handle POVs */
    for (i = 0; i < joydev->num_hats; i++) {
        /* POVs are simply reported sequentially, so hat index is POV value index */
        joystick_hat_t *hat       = joydev->hats[i];
        WORD            value     = LOWORD(jstate.rgdwPOV[i]);
        WORD            prev      = priv->prev_hats[i];
        int32_t         direction = JOYSTICK_DIRECTION_NONE;  /* neutral */
        if (prev == value) {
            continue;
        }
        priv->prev_hats[i] = value;

        /* POVs map to 360 degrees, in units of 1/100th of a degree, neutral
         * is reported as 0xffff.
         * Translate position on a circle to joystick directions, clockwise
         * from North through to Northwest */
        if (value == 0xffff) {
            /* report neutral position */
            direction = JOYSTICK_DIRECTION_NONE;
        } else {
            /* rotate hat 22.5 deg so 0-22.5 deg becomes North, 22.5-45 becomes
             * Northeast, and so on, divide by 45 deg so we get an index into
             * direction map */
            WORD hidx = ((value + 2250) % 36000) / 4500;
            direction = hat_map[hidx];
        }
        joy_hat_event(hat, direction);
    }
}

/** \brief  Close joystick device
 *
 * \param[in]   joydev  joystick device
 */
static void win32_joy_close(joystick_device_t *joydev)
{
    joy_priv_t *priv = joydev->priv;

    IDirectInputDevice8_Unacquire(priv->didev);
    priv->acquired = false;
}


/** \brief  Driver definition */
static joystick_driver_t win32_joy_driver = {
    .open      = win32_joy_open,
    .poll      = win32_joy_poll,
    .close     = win32_joy_close,
    .priv_free = joy_priv_free
};


/** \brief  Helper for logging errors with DirectInput
 *
 * Log error message in the form "<tt>{msg}</tt>: error 0x<tt>{result}</tt>".
 *
 * \param[in]   msg     message
 * \param[in]   result  error code
 */
static void log_err_helper(const char *msg, HRESULT result)
{
    log_error(winjoy_log, "%s: error 0x%08lx", msg, (unsigned long)result);
}

/** \brief  Size of buffer required to store a GUID as string */
#define GUIDSTR_BUFSIZE 37

/** \brief  Generate string from GUID
 *
 * Generate string of \a guid and place in \a buffer.
 * The \a buffer is expected to be at least #GUIDSTR_BUFSIZE bytes, which allows
 * for the string and its nul terminator.
 * Format of the string is "01234567-89AB-CDEF-0123-456789ABCDEF".
 *
 * \param[in]   guid    GUID reference
 * \param[out]  buffer  destination of GUID string
 *
 * \note    There is a \c StringFromGUID2() function in \c combaseapi.h, but
 *          that function expects an OLE string, some kind of wide string thing,
 *          so we just do it ourselves here.
 */
static void guid_to_str(const GUID *guid, char *buffer)
{
    snprintf(buffer, GUIDSTR_BUFSIZE,
             "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
             guid->Data1,
             guid->Data2,
             guid->Data3,
             guid->Data4[0], guid->Data4[1],
             guid->Data4[2], guid->Data4[3], guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    buffer[GUIDSTR_BUFSIZE - 1] = '\0';
}

/** \brief  Callback for IDirectInputDevice8::EnumObjects
 *
 * Callback function enumerating axes, buttons and hats (POVs).
 *
 * \param[in]   lpddoi  DirectInput device instance
 * \param[in]   pvref   extra callback param (VICE joystick device instance)
 *
 * \return  \c DIENUM_CONTINUE
 */
static BOOL enum_objects_callback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvref)
{
    joystick_device_t *joydev = pvref;
    joy_priv_t        *priv   = joydev->priv;
    DWORD              type   = lpddoi->dwType;

    if (type & DIDFT_ABSAXIS) {
        /* Got (absolute) axis */
        joystick_axis_t *axis;
        DIPROPRANGE      range;
        HRESULT          result;

        axis = joystick_axis_new(lpddoi->tszName);
        axis->code = DIDFT_GETINSTANCE(lpddoi->dwType);

        /* obtain raw range of axis */
        range.diph.dwSize       = sizeof(DIPROPRANGE);
        range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        range.diph.dwObj        = lpddoi->dwType;
        range.diph.dwHow        = DIPH_BYID;
        result = IDirectInputDevice8_GetProperty(priv->didev,
                                                 DIPROP_RANGE,
                                                 &range.diph);
        if (SUCCEEDED(result)) {
            axis->minimum = range.lMin;
            axis->maximum = range.lMax;
        }
        joystick_device_add_axis(joydev, axis);

    } else if (type & DIDFT_BUTTON) {
        /* Got regular (non-toggle) button */
        joystick_button_t *button = joystick_button_new(lpddoi->tszName);

        button->code = DIDFT_GETINSTANCE(lpddoi->dwType);
        joystick_device_add_button(joydev, button);

    } else if (type & DIDFT_POV) {
        /* Got hat (POV) */
        joystick_hat_t *hat = joystick_hat_new(lpddoi->tszName);

        hat->code = DIDFT_GETINSTANCE(lpddoi->dwType);
        joystick_device_add_hat(joydev, hat);
    }
    return DIENUM_CONTINUE;
}

/** \brief  Callback for IDirectInput8_EnumDevices()
 *
 *
 * \param[in]   lpddi   DirectInput device instance
 * \param[in]   pvref   extra data for callback
 *
 * \return  \c DIENUM_CONTINUE on success, \c DIENUM_STOP on failure
 */
static BOOL enum_devices_callback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvref)
{
    LPDIRECTINPUTDEVICE8 didev;
    DIDEVCAPS            caps;
    joystick_device_t   *joydev;
    joy_priv_t          *priv;
    HRESULT              result;
#if 0
    int                  raxes;     /* reported number of axes */
    int                  rbuttons;  /* reported number of buttons */
    int                  rpovs;     /* reported number of POVs */
#endif

    /* create device instance */
    result = IDirectInput8_CreateDevice(dinput_handle,
                                        &lpddi->guidInstance,
                                        &didev,
                                        NULL);
    if (result != DI_OK) {
        log_err_helper("IDirectInput8::CreateDevice() failed", result);
        return DIENUM_STOP;
    }

    /* we want the extended DIJOYSTATE2 data format */
    IDirectInputDevice8_SetDataFormat(didev, &c_dfDIJoystick2);
    IDirectInputDevice8_SetCooperativeLevel(didev,
                                            (HWND)window_handle,
                                            DISCL_NONEXCLUSIVE|DISCL_BACKGROUND);

    /* get capabilities of device */
    caps.dwSize = sizeof(DIDEVCAPS);
    result = IDirectInputDevice8_GetCapabilities(didev, &caps);
    if (result != DI_OK) {
        log_err_helper("IDirectInput8::GetCapabilities() failed", result);
        return DIENUM_STOP;
    }

    /* create VICE device instance */
    joydev = joystick_device_new();
    joydev->name    = lib_strdup(lpddi->tszProductName);
    joydev->vendor  = (uint16_t)(lpddi->guidProduct.Data1 & 0xffff);
    joydev->product = (uint16_t)((lpddi->guidProduct.Data1 > 16u) & 0xffff);
    /* Don't set num axes etc here, the joystick_device_add_foo() functions
     * update the number of inputs when called. */
#if 0
    raxes    = caps.dwAxes;
    rbuttons = caps.dwButtons;
    rpovs    = caps.dwPOVs;
#endif

    /* generate GUID as string for the node member */
    joydev->node = lib_calloc(GUIDSTR_BUFSIZE, 1);
    guid_to_str(&lpddi->guidInstance, joydev->node);
#if 0
    log_message(winjoy_log,
                "got device \"%s\" [%04x:%04x] GUID: %s, axes: %d, buttons: %d, POVs: %d",
                joydev->name, (unsigned int)joydev->vendor, (unsigned int)joydev->product,
                joydev->node, raxes, rbuttons, rpovs);
#endif
    /* store arch-specific data */
    priv = joy_priv_new();
    priv->guid  = lpddi->guidInstance;
    priv->didev = didev;
    joydev->priv = priv;

    /* enumerate axes, buttons and hats */
    IDirectInputDevice8_EnumObjects(didev,
                                    enum_objects_callback,
                                    (LPVOID)joydev,
                                    DIDFT_ABSAXIS|DIDFT_BUTTON|DIDFT_POV);

    /* register device and continue */
    joystick_device_register(joydev);
    return DIENUM_CONTINUE;
}


/** \brief  Initialize driver and enumerate and register devices
 */
void joystick_arch_init(void)
{
    HRESULT  result;

    /* initialize DirectInput for use */
    winjoy_log   = log_open("Joystick");
    log_message(winjoy_log, "Initializing DirectInput8:");
    window_handle = GetModuleHandle(NULL);
    result        = DirectInput8Create(window_handle,
                                       DIRECTINPUT_VERSION,
                                       &IID_IDirectInput8,
                                       (void *)&dinput_handle,
                                       NULL);
    if (result != DI_OK) {
        log_error(winjoy_log,
                  "failed to initialize DirectInput8: 0x%08lx, giving up.",
                  (unsigned long)result);
        log_close(winjoy_log);
        return;
    }
    log_message(winjoy_log, "OK.");

    /* register driver */
    joystick_driver_register(&win32_joy_driver);

    /* enumerate devices */
    log_message(winjoy_log, "Obtaining devices list:");
    result = IDirectInput8_EnumDevices(dinput_handle,
                                       DIDEVTYPE_JOYSTICK,
                                       enum_devices_callback,
                                       NULL,
                                       DIEDFL_ATTACHEDONLY);
    if (result != DI_OK) {
        log_err_helper("IDirectInput8::EnumDevices() failed", result);
    }
}


void joystick_arch_shutdown(void)
{
    if (dinput_handle != NULL) {
        IDirectInput8_Release(dinput_handle);
    }
}
