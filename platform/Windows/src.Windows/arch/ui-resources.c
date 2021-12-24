/*
 * uiresources.c - Windows resources.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Tibor Biczo <crown@mail.matav.hu>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

// THIS IS CUT-DOWN VERSION FOR Set Single CPU (C64 Debugger) with most of functions removed

#include "vice.h"

#include <stdio.h>
#include <windows.h>


#include "ui.h"

#include "cmdline.h"
#include "lib.h"
//#include "res.h"
#include "machine.h"
#include "resources.h"
#include "translate.h"
#include "types.h"
#include "uiapi.h"
//#include "uilib.h"
#include "util.h"
#include "videoarch.h"

int uilib_cpu_is_smp(void)
{
    DWORD_PTR process_affinity;
    DWORD_PTR system_affinity;

    if (GetProcessAffinityMask(GetCurrentProcess(), &process_affinity, &system_affinity)) {
        /* Check if multi CPU system or not */
        if ((system_affinity & (system_affinity - 1))) {
            return 1;
        }
    }
    return 0;
}



int set_single_cpu(int val, void *param)
{
    DWORD_PTR process_affinity;
    DWORD_PTR system_affinity;

    if (uilib_cpu_is_smp()) {

        GetProcessAffinityMask(GetCurrentProcess(), &process_affinity, &system_affinity);
        if (val == 1) {
            /* Set it to first CPU */
            if (SetThreadAffinityMask(GetCurrentThread(), system_affinity ^ (system_affinity & (system_affinity - 1)))) 
			{
               // ui_resources.single_cpu = 1;
            } 
			else 
			{
                return -1;
            }
        } else {
            /* Set it to all CPUs */
            if (SetThreadAffinityMask(GetCurrentThread(), system_affinity)) 
			{
                //ui_resources.single_cpu = 0;
            } 
			else 
			{
                return -1;
            }
        }
    }
    return 0;
}

