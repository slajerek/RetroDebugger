/*
 * plus4exp-resources.c
 *
 * Written by
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

#include <stdio.h>

#include "drive.h"
#include "drivemem.h"
#include "init.h"
#include "lib.h"
#include "plus4exp-resources.h"
#include "resources.h"
#include "uiapi.h"
#include "userport.h"


static void set_drive_ram(unsigned int dnr)
{
    diskunit_context_t *unit = diskunit_context[dnr];

    if ((unit->type != DRIVE_TYPE_1570) &&
        (unit->type != DRIVE_TYPE_1571) &&
        (unit->type != DRIVE_TYPE_1571CR)) {
        return;
    }

    drivemem_init(unit);

    return;
}

static int set_drive_parallel_cable(int val, void *param)
{
    diskunit_context_t *unit = diskunit_context[vice_ptr_to_uint(param)];
    int userport_device = -1;

    switch (val) {
        case DRIVE_PC_NONE:
        case DRIVE_PC_STANDARD:
            break;
        default:
            return -1;
    }

    unit->parallel_cable = val;
    set_drive_ram(vice_ptr_to_uint(param));

    /* some magic to automatically insert or remove the parallel cable into/from the user port */
    resources_get_int("UserportDevice", &userport_device);

    if ((val == DRIVE_PC_NONE) && (userport_device == USERPORT_DEVICE_DRIVE_PAR_CABLE)) {
        int hasparcable = 0;
        int dnr;
        /* check if any drive has a parallel cable enabled */
        for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
            int cable;
            resources_get_int_sprintf("Drive%iParallelCable", &cable, dnr + 8);
            if (cable != DRIVE_PC_NONE) {
                hasparcable = 1;
            }
        }
        /* if no drive uses parallel cable, disable it in the userport settings */
        if (hasparcable == 0) {
            resources_set_int("UserportDevice", USERPORT_DEVICE_NONE);
        }
    } else if (val != DRIVE_PC_NONE) {
        if (userport_device == USERPORT_DEVICE_NONE) {
            resources_set_int("UserportDevice", USERPORT_DEVICE_DRIVE_PAR_CABLE);
        } else if (userport_device != USERPORT_DEVICE_DRIVE_PAR_CABLE) {
            if (init_main_is_done()) {
                ui_message("Warning: the user port is already being used for another device.\n"
                            "To be able to use the parallel cable, you must also set up the user port accordingly.");
            }
        }
    }

    return 0;
}

static resource_int_t res_drive[] = {
    { NULL, DRIVE_PC_NONE, RES_EVENT_SAME, NULL,
      NULL, set_drive_parallel_cable, NULL },
    RESOURCE_INT_LIST_END
};

int plus4exp_resources_init(void)
{
    int dnr;

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        res_drive[0].name = lib_msprintf("Drive%iParallelCable", dnr + 8);
        res_drive[0].value_ptr = &(diskunit_context[dnr]->parallel_cable);
        res_drive[0].param = vice_uint_to_ptr(dnr);

        if (resources_register_int(res_drive) < 0) {
            return -1;
        }

        lib_free(res_drive[0].name);
    }

    return 0;
}

void plus4exp_resources_shutdown(void)
{
}
