/*
 * vic20cartmem.c -- VIC20 Cartridge memory handling.
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
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

/* #define DEBUGCART */

#include "vice.h"

#include <stdio.h>
#include "cartridge.h"

#include "machine.h"
#include "mem.h"
#include "resources.h"
#include "log.h"
#ifdef HAVE_RAWNET
#define CARTRIDGE_INCLUDE_PRIVATE_API
#define CARTRIDGE_INCLUDE_PUBLIC_API
#include "ethernetcart.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API
#undef CARTRIDGE_INCLUDE_PUBLIC_API
#endif
#include "types.h"
#include "vic20mem.h"
#include "vic20cart.h"
#include "vic20cartmem.h"
#include "vic20-generic.h"
#include "vic20-ieee488.h"
#include "vic20-midi.h"
#include "vic-fp.h"

#include "behrbonz.h"
#include "c64acia.h"
#include "digimax.h"
#include "ds12c887rtc.h"
#include "finalexpansion.h"
#include "georam.h"
#include "ioramcart.h"
#include "megacart.h"
#include "minimon.h"
#include "mikroassembler.h"
#include "rabbit.h"
#include "reu.h"
#include "sfx_soundexpander.h"
#include "sfx_soundsampler.h"
#include "sidcart.h"
#include "superexpander.h"
#include "ultimem.h"
#include "writenow.h"

#ifdef DEBUGCART
#define DBG(x)  log_printf x
#else
#define DBG(x)
#endif

/* ------------------------------------------------------------------------- */

int mem_cartridge_type = CARTRIDGE_NONE;    /* cartridge in the "main slot" */
int mem_cart_blocks = 0;

/* ------------------------------------------------------------------------- */

uint8_t cartridge_read_ram123(uint16_t addr)
{
    switch (mem_cartridge_type) {
        /* main slot */
        case CARTRIDGE_VIC20_GENERIC:
            vic20_cpu_last_data = generic_ram123_read(addr);
            break;
        case CARTRIDGE_VIC20_UM:
            vic20_cpu_last_data = vic_um_ram123_read(addr);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            vic20_cpu_last_data = finalexpansion_ram123_read(addr);
            break;
        case CARTRIDGE_VIC20_FP:
            vic20_cpu_last_data = vic_fp_ram123_read(addr);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            vic20_cpu_last_data = megacart_ram123_read(addr);
            break;
        case CARTRIDGE_VIC20_MIKRO_ASSEMBLER:
            vic20_cpu_last_data = mikroassembler_ram123_read(addr);
            break;
        case CARTRIDGE_VIC20_SUPEREXPANDER:
            vic20_cpu_last_data = superexpander_ram123_read(addr);
            break;
        default:
            vic20_cpu_last_data = vic20_v_bus_last_data;
            break;
    }
    /* open bus */
    vic20_mem_v_bus_read(addr);
    return vic20_cpu_last_data;
}

uint8_t cartridge_peek_ram123(uint16_t addr)
{
    switch (mem_cartridge_type) {
        /* main slot */
        case CARTRIDGE_VIC20_GENERIC:
            return generic_ram123_read(addr);
        case CARTRIDGE_VIC20_UM:
            return vic_um_ram123_read(addr);
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            return finalexpansion_ram123_read(addr);
        case CARTRIDGE_VIC20_FP:
            return vic_fp_ram123_read(addr);
        case CARTRIDGE_VIC20_MEGACART:
            return megacart_ram123_read(addr);
        case CARTRIDGE_VIC20_MIKRO_ASSEMBLER:
            return mikroassembler_ram123_read(addr);
        case CARTRIDGE_VIC20_SUPEREXPANDER:
            return superexpander_ram123_read(addr);
        default:
            break;
    }
    /* open bus */
    return 0;
}

void cartridge_store_ram123(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    switch (mem_cartridge_type) {
        /* main slot */
        case CARTRIDGE_VIC20_GENERIC:
            generic_ram123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_ram123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_ram123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_ram123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_ram123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_MIKRO_ASSEMBLER:
            mikroassembler_ram123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_SUPEREXPANDER:
            superexpander_ram123_store(addr, value);
            break;
    }
    vic20_mem_v_bus_store(addr);
}

uint8_t cartridge_read_blk1(uint16_t addr)
{
    switch (mem_cartridge_type) {
        /* main slot */
        case CARTRIDGE_VIC20_BEHRBONZ:
            vic20_cpu_last_data = behrbonz_blk13_read(addr);
            break;
        case CARTRIDGE_VIC20_GENERIC:
            vic20_cpu_last_data = generic_blk1_read(addr);
            break;
        case CARTRIDGE_VIC20_UM:
            vic20_cpu_last_data = vic_um_blk1_read(addr);
            break;
        case CARTRIDGE_VIC20_FP:
            vic20_cpu_last_data = vic_fp_blk1_read(addr);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            vic20_cpu_last_data = megacart_blk123_read(addr);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            vic20_cpu_last_data = finalexpansion_blk1_read(addr);
            break;
    }
    return vic20_cpu_last_data;
}

uint8_t cartridge_peek_blk1(uint16_t addr)
{
    switch (mem_cartridge_type) {
        /* main slot */
        case CARTRIDGE_VIC20_BEHRBONZ:
            return behrbonz_blk13_read(addr);
        case CARTRIDGE_VIC20_GENERIC:
            return generic_blk1_read(addr);
        case CARTRIDGE_VIC20_UM:
            return vic_um_blk1_read(addr);
        case CARTRIDGE_VIC20_FP:
            return vic_fp_blk1_read(addr);
        case CARTRIDGE_VIC20_MEGACART:
            return megacart_blk123_read(addr);
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            return finalexpansion_blk1_read(addr);
    }
    return 0;
}

void cartridge_store_blk1(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    switch (mem_cartridge_type) {
        /* main slot */
        case CARTRIDGE_VIC20_GENERIC:
            generic_blk1_store(addr, value);
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_blk1_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_blk1_store(addr, value);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_blk123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_blk1_store(addr, value);
            break;
    }
}

uint8_t cartridge_read_blk2(uint16_t addr)
{
    switch (mem_cartridge_type) {
        /* main slot */
        case CARTRIDGE_VIC20_BEHRBONZ:
            vic20_cpu_last_data = behrbonz_blk25_read(addr);
            break;
        case CARTRIDGE_VIC20_GENERIC:
            vic20_cpu_last_data = generic_blk2_read(addr);
            break;
        case CARTRIDGE_VIC20_UM:
            vic20_cpu_last_data = vic_um_blk23_read(addr);
            break;
        case CARTRIDGE_VIC20_FP:
            vic20_cpu_last_data = vic_fp_blk23_read(addr);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            vic20_cpu_last_data = megacart_blk123_read(addr);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            vic20_cpu_last_data = finalexpansion_blk2_read(addr);
            break;
    }
    return vic20_cpu_last_data;
}

uint8_t cartridge_peek_blk2(uint16_t addr)
{
    switch (mem_cartridge_type) {
        /* main slot */
        case CARTRIDGE_VIC20_BEHRBONZ:
            return behrbonz_blk25_read(addr);
        case CARTRIDGE_VIC20_GENERIC:
            return generic_blk2_read(addr);
        case CARTRIDGE_VIC20_UM:
            return vic_um_blk23_read(addr);
        case CARTRIDGE_VIC20_FP:
            return vic_fp_blk23_read(addr);
        case CARTRIDGE_VIC20_MEGACART:
            return megacart_blk123_read(addr);
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            return finalexpansion_blk2_read(addr);
    }
    return 0;
}

void cartridge_store_blk2(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    switch (mem_cartridge_type) {
        /* main slot */
        case CARTRIDGE_VIC20_GENERIC:
            generic_blk2_store(addr, value);
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_blk23_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_blk23_store(addr, value);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_blk123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_blk2_store(addr, value);
            break;
    }
}

uint8_t cartridge_read_blk3(uint16_t addr)
{
    switch (mem_cartridge_type) {
        /* main slot */
        case CARTRIDGE_VIC20_BEHRBONZ:
            vic20_cpu_last_data = behrbonz_blk13_read(addr);
            break;
        case CARTRIDGE_VIC20_GENERIC:
            vic20_cpu_last_data = generic_blk3_read(addr);
            break;
        case CARTRIDGE_VIC20_UM:
            vic20_cpu_last_data = vic_um_blk23_read(addr);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            vic20_cpu_last_data = finalexpansion_blk3_read(addr);
            break;
        case CARTRIDGE_VIC20_FP:
            vic20_cpu_last_data = vic_fp_blk23_read(addr);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            vic20_cpu_last_data = megacart_blk123_read(addr);
            break;
        case CARTRIDGE_VIC20_MIKRO_ASSEMBLER:
            vic20_cpu_last_data = mikroassembler_blk3_read(addr);
            break;
    }
    return vic20_cpu_last_data;
}

uint8_t cartridge_peek_blk3(uint16_t addr)
{
    switch (mem_cartridge_type) {
        /* main slot */
        case CARTRIDGE_VIC20_BEHRBONZ:
            return behrbonz_blk13_read(addr);
        case CARTRIDGE_VIC20_GENERIC:
            return generic_blk3_read(addr);
        case CARTRIDGE_VIC20_UM:
            return vic_um_blk23_read(addr);
        case CARTRIDGE_VIC20_FP:
            return vic_fp_blk23_read(addr);
        case CARTRIDGE_VIC20_MEGACART:
            return megacart_blk123_read(addr);
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            return finalexpansion_blk3_read(addr);
    }
    return 0;
}

void cartridge_store_blk3(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    switch (mem_cartridge_type) {
        /* main slot */
        case CARTRIDGE_VIC20_GENERIC:
            generic_blk3_store(addr, value);
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_blk23_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_blk23_store(addr, value);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_blk123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_blk3_store(addr, value);
            break;
    }
}

/* A000-BFFF */
uint8_t cartridge_read_blk5(uint16_t addr)
{
    int res = CART_READ_THROUGH;
    uint8_t value;
    /* DBG(("cartridge_read_blk5 (%d) 0x%04x", mem_cartridge_type, addr)); */

    /* "Slot 0" */

    if (minimon_cart_enabled()) {

        if ((res = minimon_blk5_read(addr, &value)) == CART_READ_VALID) {
            return value;
        }
        /* open bus value, in case no cartridge is attached to pass through */
        vic20_cpu_last_data = (addr >> 8);
        /* DBG(("cartridge_read_blk5 %02x %04x", vic20_cpu_last_data, addr)); */
    }

    /* main slot */
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_BEHRBONZ:
            vic20_cpu_last_data = behrbonz_blk25_read(addr);
            break;
        case CARTRIDGE_VIC20_GENERIC:
            vic20_cpu_last_data = generic_blk5_read(addr);
            break;
        case CARTRIDGE_VIC20_UM:
            vic20_cpu_last_data = vic_um_blk5_read(addr);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            vic20_cpu_last_data = finalexpansion_blk5_read(addr);
            break;
        case CARTRIDGE_VIC20_FP:
            vic20_cpu_last_data = vic_fp_blk5_read(addr);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            vic20_cpu_last_data = megacart_blk5_read(addr);
            break;
        case CARTRIDGE_VIC20_MIKRO_ASSEMBLER:
            vic20_cpu_last_data = mikroassembler_blk5_read(addr);
            break;
        case CARTRIDGE_VIC20_SUPEREXPANDER:
            vic20_cpu_last_data = superexpander_blk5_read(addr);
            break;
        case CARTRIDGE_VIC20_WRITE_NOW:
            vic20_cpu_last_data = writenow_blk5_read(addr);
            break;
    }
    return vic20_cpu_last_data;
}

/* A000-BFFF */
uint8_t cartridge_peek_blk5(uint16_t addr)
{
    int res = CART_READ_THROUGH;
    uint8_t value;

    /* "Slot 0" */

    if (minimon_cart_enabled()) {
        if ((res = minimon_blk5_read(addr, &value)) == CART_READ_VALID) {
            return value;
        }
    }

    /* main slot */
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_BEHRBONZ:
            return behrbonz_blk25_read(addr);
        case CARTRIDGE_VIC20_GENERIC:
            return generic_blk5_read(addr);
        case CARTRIDGE_VIC20_UM:
            return vic_um_blk5_read(addr);
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            return finalexpansion_blk5_read(addr);
        case CARTRIDGE_VIC20_FP:
            return vic_fp_blk5_read(addr);
        case CARTRIDGE_VIC20_MEGACART:
            return megacart_blk5_read(addr);
        case CARTRIDGE_VIC20_SUPEREXPANDER:
            return superexpander_blk5_read(addr);
        case CARTRIDGE_VIC20_WRITE_NOW:
            return writenow_blk5_read(addr);
    }
    return 0;
}

/* A000-BFFF */
void cartridge_store_blk5(uint16_t addr, uint8_t value)
{
    /* "Slot 0" */

    /* main slot */
    vic20_cpu_last_data = value;
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_GENERIC:
            generic_blk5_store(addr, value);
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_blk5_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_blk5_store(addr, value);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_blk5_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_blk5_store(addr, value);
            break;
    }
}

/* ------------------------------------------------------------------------- */

void cartridge_init(void)
{
    /* main slot */
    behrbonz_init();
    finalexpansion_init();
    megacart_init();
    superexpander_init();
    vic_fp_init();

    /* io slot */
#ifdef HAVE_RAWNET
    ethernetcart_init();
#endif
    aciacart_init();
    georam_init();
}

void cartridge_reset(void)
{
    /* io slot */
#ifdef HAVE_RAWNET
    if (ethernetcart_cart_enabled()) {
        ethernetcart_reset();
    }
#endif
    if (aciacart_cart_enabled()) {
        aciacart_reset();
    }
    if (digimax_cart_enabled()) {
        digimax_reset();
    }
    if (ds12c887rtc_cart_enabled()) {
        ds12c887rtc_reset();
    }
    if (sfx_soundexpander_cart_enabled()) {
        sfx_soundexpander_reset();
    }
    if (sfx_soundsampler_cart_enabled()) {
        sfx_soundsampler_reset();
    }
    if (georam_cart_enabled()) {
        georam_reset();
    }

    /* main slot */
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_BEHRBONZ:
            behrbonz_reset();
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_reset();
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_reset();
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_reset();
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_reset();
            break;
    }

    /* slot 0 */
    if (minimon_cart_enabled()) {
        minimon_reset();
    }
}

void cartridge_powerup(void)
{
    /* "IO Slot" */
    if (georam_cart_enabled()) {
        georam_powerup();
    }
#if 0
    /* FIXME */
    if (reu_cart_enabled()) {
        reu_powerup();
    }
#endif
    /* main slot */
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_UM:
            vic_um_powerup();
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_powerup();
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_powerup();
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_powerup();
            break;
    }

    /* slot 0 */
    if (minimon_cart_enabled()) {
        minimon_powerup();
    }
}

void cartridge_attach(int type, uint8_t *rawcart)
{
    int cartridge_reset;

    mem_cartridge_type = type;

    DBG(("cartridge_attach type: %d", type));
#if 0
    switch (type) {
        case CARTRIDGE_VIC20_GENERIC:
            generic_config_setup(rawcart);
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_config_setup(rawcart);
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_config_setup(rawcart);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_config_setup(rawcart);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_config_setup(rawcart);
            break;
        default:
            mem_cartridge_type = CARTRIDGE_NONE;
    }
#endif

    resources_get_int("CartridgeReset", &cartridge_reset);

    if (cartridge_reset != 0) {
        /* "Turn off machine before inserting cartridge" */
        machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);
    }
}

static void cart_detach_all(void)
{
    /* io slot */
    vic20_ieee488_detach();
#ifdef HAVE_MIDI
    vic20_midi_detach();
#endif
    sidcart_detach();
    ioramcart_io2_detach();
    ioramcart_io3_detach();

    /* c64 through mascuerade carts */
    aciacart_detach();
    digimax_detach();
    ds12c887rtc_detach();
    georam_detach();
    sfx_soundexpander_detach();
    sfx_soundsampler_detach();
#ifdef HAVE_RAWNET
    ethernetcart_detach();
#endif
    /* main slot */
    behrbonz_detach();
    generic_detach();
    finalexpansion_detach();
    rabbit_detach();
    megacart_detach();
    vic_um_detach();
    vic_fp_detach();
    writenow_detach();

    /* slot 0 */
    minimon_detach();
}

void cartridge_detach(int type)
{
    int cartridge_reset;

    switch (type) {
        case -1:
            cart_detach_all();
            break;
        case CARTRIDGE_VIC20_BEHRBONZ:
            behrbonz_detach();
            break;
        case CARTRIDGE_VIC20_GENERIC:
            generic_detach();
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_detach();
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_detach();
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_detach();
            break;
        case CARTRIDGE_VIC20_MINIMON:
            minimon_detach();
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_detach();
            break;
        case CARTRIDGE_VIC20_RABBIT:
            rabbit_detach();
            break;
        case CARTRIDGE_VIC20_WRITE_NOW:
            writenow_detach();
            break;
    }
    mem_cartridge_type = CARTRIDGE_NONE;
    /* this is probably redundant as it is also performed by the
       local detach functions. */
    mem_cart_blocks = 0;
    mem_initialize_memory();

    resources_get_int("CartridgeReset", &cartridge_reset);

    if (cartridge_reset != 0) {
        /* "Turn off machine before removing cartridge" */
        machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);
    }
}

/* ------------------------------------------------------------------------- */

void cartridge_sound_chip_init(void)
{
    /* io slot */
    digimax_sound_chip_init();
    sfx_soundexpander_sound_chip_init();
    sfx_soundsampler_sound_chip_init();
}
