# VICE Joystick API

> To get a nicely formatted HTML version of this document, including syntax
> highlighting, use:
>
> `pandoc -s -t html -f gfm joystick.md > joystick.html`


## Preface

This document describes the updated joystick API, which currently is a **work in
progress**. All information herein is subject to change while the joystick code
is being worked on. The inner workings of the actual emulation of the I/O system
will not be described, just the translation of host device input to emulated
joystick device, so no actual CIA/VIA emulation.


## Overview of the joystick system in VICE

The joystick system in VICE is split into two parts: **common code** and
**driver code**. The driver code is specific to an OS/UI, while the common code,
as the name implies, is used for every OS/UI.

### Common code

The common code (in `src/joyport/`) is responsible for interpreting data from
the drivers and passing that to the emulation, as well as handling mapping and
calibration of host inputs to emulated inputs. It is also responsible for
providing the UI with information on host and emulated devices, and at a later
point, passing host input to the UI for mapping and calibration dialogs.

### Driver code

The driver code is responsible for reading data from a host device and passing
that back to the common code, as well as providing the common code with a list
of available host devices and their properties.


## Changes in the separation of driver and common code

I've tried to keep the code required to implement a driver as small as possible,
moving a number of responsibilities from the drivers to the common code.

* The old code would let the driver interpret raw axis and button values and
  send that back to the emulation (during the `poll()` callback). The drivers
  now simply pass the raw values to the common code, and the common code
  interprets those values with the help of the information on the host devices
  provided by the driver (while also doing calibration).

* A driver no longer needs to concern itself with ordering inputs, the common
  code handles that.

* Every input now has a unique *code*, which can be an event code (like with
  Linux' evdev), a simple index of the input (as in SDL) or a HID usage code
  (as on FreeBSD/NetBSD). The API provides drivers with methods of looking up
  axis, button and hat objects through their respective code.

* Event handlers in the common code now refer to inputs by instance, not index.
  So for an axis event a driver would call `joy_axis_event()` with a host device
  instance, axis instance and raw axis value.


**TODO**: Proper (simple) description of `joystick_device_t` and its members
          `joystick_axis_t`, `joystick_button_t` and `joystick_hat_t`.

**TODO**: Explain ownership of objects (container assumes ownership of its
          elements and is responsible for freeing them after use, etc).


## Implementing a driver

Implementing a driver should be fairly straightforward. A driver registers
itself with the joystick system and adds host devices it has detected.

During joystick system initialization an arch-specific initialization function
is called (and expected to be implemented by the driver), where the driver
registers itself and adds host devices:

```C
void joystick_arch_init(void)
```

The function to register the driver is:

```C
void joystick_driver_register(const joystick_driver_t *driver)
```

Where `joystick_driver_t` is defined as:
```C
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
```

> Currently (re)opening a device hasn't been implemented yet, so the `open()`
> method can be ignored, for now.

### Driver methods

The `poll()` method is called by the emulation at the end of *every emulated
scanline*, and is expected to process any pending event data and pass that
along to `joy_axis_event()`, `joy_button_event()` or `joy_hat_event()`.

The `close()` method should close the host device (e.g. close file descriptor)
and put the device in a proper state for opening again. It should **not** free
its private data in the `priv` member of the `joystick_device_t`, that is done
in the `priv_free()` method, called by the joystick system on shutdown.
It should also **not** free the joystick device instance, that again is done by
the joystick system.

The `priv_free()` method (if used) is, as mentioned above, called on emulator
shutdown (or once we implement plug-n-pray, on device unplugging), and can be
used to free any arch-specific resources that cannot be contained in the
`joystick_device_t` instance or its members.
> For example: the DirectInput driver for Windows stores a `GUID` and an
> `LPDIRECTINPUTDEVICE8` instance in `priv`.

The `customize()` method can be used to customize the default mapping and
calibration applied by the joystick system when `joystick_device_register()` is
called.


### Example of driver implementation

The basic structure of a driver is the following:

```C

/* Some arch-specific data of a device (obvious pseudo code) */
typedef struct foo_priv_s {
    FOO_DEVICE *foodev;
} foo_priv_t;


/* Declaration of driver methods */
static joystick_driver_t foo_driver = {
    .poll      = foo_poll,
    .close     = foo_close
    .priv_free = foo_priv_free
};


/*
 * Called after the joystick system has initialized during emulator boot
 */
void joystick_arch_init(void)
{
    /* Arch-specific initialization, if required */
    FOO_JOYSTICK_SYSTEM_INIT();

    /* Register driver */
    joystick_driver_register(&foo_driver);

    /* Iterate devices and register them with the joystick system */
    for (int i = 0; i < NUM_HOST_DEVICES; i++) {

        joystick_device_t *joydev = joystick_device_new();

        FOO_DEVICE *foodev = OPEN_FOO_DEVICE(i);

        joystick_device_set_name(joydev, foodev->name);
        joystick_device_set_node(joydev, foodev->...); /* filesystem node of
                                                          device, GUID string,
                                                          whatever */
        joydev->vendor  = foodev->vendor_id;    /* USB HID vendor ID */
        joydev->product = foodev->product_id;   /* USB HID product ID */
 
        /* Iterate axes, buttons and perhaps hats of a device and add them */
        for (int a = 0; a < NUM_AXES(foodev); a++) {

            joystick_axis_t *axis = joystick_axis_new(foodev->AXES[a].name);
            axis->code = foodev->AXES[a].code;   /* some unique event code, can
                                                     be HID usage, or just index
                                                     of axis */
            /* set limits if available */
            axis->minimum = foodev->AXES[a].min; /* default is INT16_MIN */
            axis->maximum = foodev->AXES[a].max; /* default is INT16_MAX */

            /* store arch-specific data in `priv` member */
            foo_priv_t *priv = lib_malloc(sizeof *priv);
            priv->foodev = foodev;
            joydev->priv = priv;

            /* add axis to device: device takes ownership */
            joystick_device_add_axis(joydev, axis);
        }

        /*
         * ... Do the same for buttons and hats, if available ...
         */

        /* Now register the device with the joystick system: the joystick
         * system takes ownership of the device and its members
         */
        joystick_device_register(joydev);
    }
}


/*
 * Clean up any arch-specific resources here on emulator shutdown
 */
void joystick_arch_shutdown(void)
{
    FOO_JOYSTICK_SYSTEM_CLOSE();
}


static void foo_poll(joystick_device_t *joydev)
{
    foo_priv_t *priv = joydev->priv;

    while (HAS_EVENT_PENDING(priv->foodev) {
        FOO_EVENT event = GET_EVENT(priv->foodev);

        switch (event.type) {
            case FOO_AXIS:
                joystick_axis_t *axis = joystick_axis_from_code(joydev, event.code);
                joy_axis_event(axis, event.value);
                break;
            case FOO_BUTTON:
                joystick_button_t *button = joystick_button_from_code(joydev, event.code);
                joy_button_event(button, event.value);
                break;
        }
    }
}


static void foo_close(joystick_device_t *joydev)
{
    foo_priv_t *priv = joydev->priv;

    if (priv->foodev != NULL) {
        FOO_DEVICE_CLOSE(priv->foodev);
        priv->foodev = NULL;
    }
}


static void foo_priv_free(void *priv)
{
    foo_priv_t *p = priv;

    FOO_DEVICE_FREE(p->foodev);
    lib_free(p);
}
```

