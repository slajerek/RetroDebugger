
/*
 * plus4-stubs.c - dummies for unneeded/unused functions
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#include <stddef.h>
#include <stdbool.h>

#include "c64/cart/clockport.h"
#include "cartridge.h"
#include "ds1307.h"
#include "rtc-58321a.h"
#include "mididrv.h"
#include "pet/petpia.h"
#ifdef HAVE_LIBCURL
#include "userport_wic64.h"
#endif

#ifdef WINDOWS_COMPILE
void mididrv_ui_reset_device_list(int device)
{
}

char *mididrv_ui_get_next_device_name(int device, int *id)
{
    return NULL;
}
#endif

/*******************************************************************************
    clockport
*******************************************************************************/

clockport_supported_devices_t clockport_supported_devices[] = { { 0, NULL } };

bool pia1_get_diagnostic_pin(void)
{
    return false;
}

/*******************************************************************************
    userport devices
*******************************************************************************/
#if 0
int ds1307_write_snapshot(rtc_ds1307_t *context, snapshot_t *s)
{
    return -1;
}
int ds1307_read_snapshot(rtc_ds1307_t *context, snapshot_t *s)
{
    return -1;
}
rtc_ds1307_t *ds1307_init(char *device)
{
    return NULL;
}
void ds1307_destroy(rtc_ds1307_t *context, int save)
{
}
void ds1307_set_clk_line(rtc_ds1307_t *context, uint8_t data)
{
}
void ds1307_set_data_line(rtc_ds1307_t *context, uint8_t data)
{
}
uint8_t ds1307_read_data_line(rtc_ds1307_t *context)
{
    return 0;
}

int rtc58321a_read_snapshot(rtc_58321a_t *context, snapshot_t *s)
{
    return -1;
}
int rtc58321a_write_snapshot(rtc_58321a_t *context, snapshot_t *s)
{
    return -1;
}
rtc_58321a_t *rtc58321a_init(char *device)
{
    return NULL;
}
void rtc58321a_destroy(rtc_58321a_t *context, int save)
{
}
uint8_t rtc58321a_read(rtc_58321a_t *context)
{
    return 0;
}
void rtc58321a_write_address(rtc_58321a_t *context, uint8_t address)
{
}
void rtc58321a_write_data(rtc_58321a_t *context, uint8_t data)
{
}
void rtc58321a_stop_clock(rtc_58321a_t *context)
{
}
void rtc58321a_start_clock(rtc_58321a_t *context)
{
}
#endif

#if 0
int rsuser_cmdline_options_init(void)
{
    return -1;
}
int rsuser_resources_init(void)
{
    return -1;
}
#endif
