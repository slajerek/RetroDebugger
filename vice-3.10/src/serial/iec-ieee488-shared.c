/*
 * iec-ieee488-shared.c - Common resources/options for IEC and IEEE-488
 *
 * Written by
 *  Olaf 'Rhialto' Seibert <rhialto@falu.nl>
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

/*
 * This file defines resources and command line options that are shared
 * between IEC and IEEE-488 buses.
 *
 * On machines where both are present at the same time (such as C64 or
 * VIC-20 with IEEE-488 cartridge), these are used for both at the same
 * time.
 *
 * When there is only one bus, we use the same unified name for the
 * resourcs and options.
 *
 * To avoid that this file refers to (and makes the linker include) a
 * bus type that is not used on a particular emulator, the callbacks are
 * not referenced directly, but are provided as function pointers.
 * A fancy name for this is "dependency injection".
 *
 * The possible concrete callbacks can be:
 * - set_iec_device_enable() in serial/serial-iec-device.c
 * - set_ieee_device_enable() in parallel/parallel.c
 */

#include "vice.h"

#include "cmdline.h"
#include "iec-ieee488-shared.h"
#include "iecbus.h"

#ifdef DEBUG_IECEE
#define DBG(x) log_printf  x
#else
#define DBG(x)
#endif

#define MAX_CALLBACKS   2

static resource_set_func_int_t *callbacks[MAX_CALLBACKS];
int iec_ieee_device_enabled[IECBUS_NUM];

static int indirect_callback(int enable, void *param)
{
    int i;
    int ret = 0;
    unsigned int unit;

    unit = vice_ptr_to_uint(param);
    DBG(("indirect_callback: enable: %d, unit: %u", enable, unit));

    if ((unit < 4 || unit >= IECBUS_NUM)) {
        return -1;
    }

    iec_ieee_device_enabled[unit] = enable ? 1 : 0;

    for (i = 0; i < MAX_CALLBACKS; i++) {
        if (callbacks[i] != NULL) {
            ret |= (*callbacks[i])(enable, param);
        }
    }

    return ret;
}

static const resource_int_t resources_int[] = {
    { "BusDevice4", 0, RES_EVENT_SAME, NULL,
      &iec_ieee_device_enabled[4], indirect_callback, (void *)4 },
    { "BusDevice5", 0, RES_EVENT_SAME, NULL,
      &iec_ieee_device_enabled[5], indirect_callback, (void *)5 },
    { "BusDevice6", 0, RES_EVENT_SAME, NULL,
      &iec_ieee_device_enabled[6], indirect_callback, (void *)6 },
    { "BusDevice7", 0, RES_EVENT_SAME, NULL,
      &iec_ieee_device_enabled[7], indirect_callback, (void *)7 },
    { "BusDevice8", 0, RES_EVENT_SAME, NULL,
      &iec_ieee_device_enabled[8], indirect_callback, (void *)8 },
    { "BusDevice9", 0, RES_EVENT_SAME, NULL,
      &iec_ieee_device_enabled[9], indirect_callback, (void *)9 },
    { "BusDevice10", 0, RES_EVENT_SAME, NULL,
      &iec_ieee_device_enabled[10], indirect_callback, (void *)10 },
    { "BusDevice11", 0, RES_EVENT_SAME, NULL,
      &iec_ieee_device_enabled[11], indirect_callback, (void *)11 },
    RESOURCE_INT_LIST_END
};

int iec_ieee_device_resources_init(resource_set_func_int_t *callback)
{
    int i;
    DBG(("iec_ieee_device_resources_init"));

    for (i = 0; i < MAX_CALLBACKS; i++) {
        if (callbacks[i] == NULL) {
            callbacks[i] = callback;
            break;
        }
        if (callbacks[i] == callback) {
            break;
        }
    }

    if (!resources_exists(resources_int[0].name)) {
        return resources_register_int(resources_int);
    }

    return 0;
}

static const cmdline_option_t cmdline_options[] =
{
    { "-busdevice4", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice4", (resource_value_t)1,
      NULL, "Enable IEC/IEEE-488 device emulation for device #4" },
    { "+busdevice4", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice4", (resource_value_t)0,
      NULL, "Disable IEC/IEEE-488 device emulation for device #4" },
    { "-busdevice5", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice5", (resource_value_t)1,
      NULL, "Enable IEC/IEEE-488 device emulation for device #5" },
    { "+busdevice5", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice5", (resource_value_t)0,
      NULL, "Disable IEC/IEEE-488 device emulation for device #5" },
    { "-busdevice6", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice6", (resource_value_t)1,
      NULL, "Enable IEC/IEEE-488 device emulation for device #6" },
    { "+busdevice6", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice6", (resource_value_t)0,
      NULL, "Disable IEC/IEEE-488 device emulation for device #6" },
    { "-busdevice7", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice7", (resource_value_t)1,
      NULL, "Enable IEC/IEEE-488 device emulation for device #7" },
    { "+busdevice7", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice7", (resource_value_t)0,
      NULL, "Disable IEC/IEEE-488 device emulation for device #7" },
    { "-busdevice8", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice8", (resource_value_t)1,
      NULL, "Enable IEC/IEEE-488 device emulation for device #8" },
    { "+busdevice8", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice8", (resource_value_t)0,
      NULL, "Disable IEC/IEEE-488 device emulation for device #8" },
    { "-busdevice9", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice9", (resource_value_t)1,
      NULL, "Enable IEC/IEEE-488 device emulation for device #9" },
    { "+busdevice9", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice9", (resource_value_t)0,
      NULL, "Disable IEC/IEEE-488 device emulation for device #9" },
    { "-busdevice10", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice10", (resource_value_t)1,
      NULL, "Enable IEC/IEEE-488 device emulation for device #10" },
    { "+busdevice10", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice10", (resource_value_t)0,
      NULL, "Disable IEC/IEEE-488 device emulation for device #10" },
    { "-busdevice11", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice11", (resource_value_t)1,
      NULL, "Enable IEC/IEEE-488 device emulation for device #11" },
    { "+busdevice11", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BusDevice11", (resource_value_t)0,
      NULL, "Disable IEC/IEEE-488 device emulation for device #11" },
    CMDLINE_LIST_END
};

int iec_ieee_device_cmdline_options_init(void)
{
    DBG(("iec_ieee_device_cmdline_options_init"));

    if (!cmdline_option_exists(cmdline_options[0].name)) {
        return cmdline_register_options(cmdline_options);
    }
    return 0;
}

