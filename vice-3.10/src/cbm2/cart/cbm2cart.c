/*
 * cbm2cart.c -- CBM2 cartridge handling.
 *
 * Written by
 *  Groepaz <groepaz@gmx.net>
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

#include <string.h>

#include "cartridge.h"
#include "cmdline.h"
#include "cbm2cart.h"
#include "cbm2mem.h"
#include "cbm2rom.h"
#include "export.h"
#include "crt.h"
#include "lib.h"
#include "log.h"
#include "util.h"
#include "machine.h"
#include "monitor.h"
#include "resources.h"
#include "snapshot.h"
#include "sysfile.h"

#include "cbm2-generic.h"

#ifdef DEBUGCART
#define DBG(x) log_printf  x
#else
#define DBG(x)
#endif

/*
    cartridge port has:

    A0-A12          0000-1FFF   8k    (1000-1FFF "Disk ROM")
    CS Bank1        2000-3FFF   8k    (cartridge bank 1)
    CS Bank2        4000-5FFF   8k    (cartridge bank 2)
    CS Bank3        6000-7FFF   8k    (cartridge bank 3)
 */

/* Expansion port signals. */
export_t export = { 0, 0, 0, 0 };

/* global options for the cart system */
static int cbm2cartridge_reset; /* (resource) hardreset system after cart was attached/detached */

/* defaults */
static char *cartridge_file = NULL; /* (resource) file name */
static int cartridge_type = CARTRIDGE_NONE; /* (resource) is == CARTRIDGE_CRT (0) if CRT file */

/* actually in use */
static char *cartfile = NULL; /* file name */
static int cbm2cart_type = CARTRIDGE_NONE; /* is == CARTRIDGE_CRT (0) if CRT file */
static int crttype = CARTRIDGE_NONE; /* contains CRT ID if cbm2cart_type == 0 */

static int mem_cartridge_type = CARTRIDGE_NONE;  /* Type of the cartridge attached. */

/* ---------------------------------------------------------------------*/

/* FIXME: these probably shouldn't be here */
int cart08_ram = 0;
int cart1_ram = 0;
int cart2_ram = 0;
int cart4_ram = 0;
int cart6_ram = 0;
int cartC_ram = 0;

static export_resource_t export_res08 = {
    "RAM" , 0, CBM2_CART_BLK08, NULL, NULL, 0
};
static export_resource_t export_res1 = {
    "RAM" , 0, CBM2_CART_BLK1, NULL, NULL, 0
};
static export_resource_t export_res2 = {
    "RAM" , 0, CBM2_CART_BLK2, NULL, NULL, 0
};
static export_resource_t export_res4 = {
    "RAM" , 0, CBM2_CART_BLK4, NULL, NULL, 0
};
static export_resource_t export_res6 = {
    "RAM" , 0, CBM2_CART_BLK6, NULL, NULL, 0
};

/* ---------------------------------------------------------------------*/

static uint8_t romh_banks[1]; /* dummy */

uint8_t *ultimax_romh_phi1_ptr(uint16_t addr)
{
    return romh_banks;
}

uint8_t *ultimax_romh_phi2_ptr(uint16_t addr)
{
    return romh_banks;
}

/* ---------------------------------------------------------------------*/

static cartridge_info_t cartlist[] = {
    /* standard cartridges with CRT ID = 0 */
    { "Raw 4KiB C1",                       CARTRIDGE_CBM2_GENERIC_C1,      CARTRIDGE_GROUP_GENERIC },
    { "Raw 8KiB C2",                       CARTRIDGE_CBM2_GENERIC_C2,      CARTRIDGE_GROUP_GENERIC },
    { "Raw 8KiB C4",                       CARTRIDGE_CBM2_GENERIC_C4,      CARTRIDGE_GROUP_GENERIC },
    { "Raw 8KiB C6",                       CARTRIDGE_CBM2_GENERIC_C6,      CARTRIDGE_GROUP_GENERIC },

    /* all cartridges with a CRT ID > 0, alphabetically sorted */

    { NULL, 0, 0 }
};

cartridge_info_t *cartridge_get_info_list(void)
{
    return &cartlist[0];
}

/* ---------------------------------------------------------------------*/

static int cart_attach_cmdline(const char *param, void *extra_param)
{
    int type = vice_ptr_to_int(extra_param);

    /* NULL param is used for +cart */
    if (!param) {
        cartridge_detach_image(-1);
        return 0;
    }
    return cartridge_attach_image(type, param);
}

static const cmdline_option_t cmdline_options[] =
{
    /* hardreset on cartridge change */
    { "-cartreset", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "CartridgeReset", (void *)1,
      NULL, "Reset machine if a cartridge is attached or detached" },
    { "+cartreset", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "CartridgeReset", (void *)0,
      NULL, "Do not reset machine if a cartridge is attached or detached" },
#if 0
    /* smart attach */
    { "-cart", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void*)CARTRIDGE_CBM2_DETECT, NULL, NULL,
      "<Name>", "Smart-attach cartridge image" },
#endif
    /* smart-insert CRT */
    { "-cartcrt", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cart_attach_cmdline, (void *)CARTRIDGE_CRT, NULL, NULL,
      "<Name>", "Attach CRT cartridge image" },
    /* no cartridge */
    { "+cart", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      cart_attach_cmdline, NULL, NULL, NULL,
      NULL, "Disable default cartridge" },
    CMDLINE_LIST_END
};

int cartridge_cmdline_options_init(void)
{
    if (generic_cmdline_options_init()) {
        return 0;
    }
    mon_cart_cmd.cartridge_attach_image = cartridge_attach_image;
    mon_cart_cmd.cartridge_detach_image = cartridge_detach_image;
#if 0
    mon_cart_cmd.cartridge_trigger_freeze = cartridge_trigger_freeze;
    mon_cart_cmd.cartridge_trigger_freeze_nmi_only = cartridge_trigger_freeze_nmi_only;
#endif
    mon_cart_cmd.export_dump = export_dump;
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

/* FIXME: these probably shouldn't be here */

static int set_cart08_ram(int val, void *param)
{
    cart08_ram = val ? 1 : 0;

    export_remove(&export_res08);
    if (cart08_ram) {
        if (export_add(&export_res08) < 0) {
            return -1;
        }
    }

    generic_cartrom_to_mem_hack();
    mem_initialize_memory_bank(15);
    return 0;
}

static int set_cart1_ram(int val, void *param)
{
    cart1_ram = val ? 1 : 0;

    export_remove(&export_res1);
    if (cart1_ram) {
        if (export_add(&export_res1) < 0) {
            return -1;
        }
    }

    generic_cartrom_to_mem_hack();
    mem_initialize_memory_bank(15);
    return 0;
}

static int set_cart2_ram(int val, void *param)
{
    cart2_ram = val ? 1 : 0;

    export_remove(&export_res2);
    if (cart2_ram) {
        if (export_add(&export_res2) < 0) {
            return -1;
        }
    }

    generic_cartrom_to_mem_hack();
    mem_initialize_memory_bank(15);
    return 0;
}

static int set_cart4_ram(int val, void *param)
{
    cart4_ram = val ? 1 : 0;

    export_remove(&export_res4);
    if (cart4_ram) {
        if (export_add(&export_res4) < 0) {
            return -1;
        }
    }

    generic_cartrom_to_mem_hack();
    mem_initialize_memory_bank(15);
    return 0;
}

static int set_cart6_ram(int val, void *param)
{
    cart6_ram = val ? 1 : 0;

    export_remove(&export_res6);
    if (cart6_ram) {
        if (export_add(&export_res6) < 0) {
            return -1;
        }
    }

    generic_cartrom_to_mem_hack();
    mem_initialize_memory_bank(15);
    return 0;
}

static int set_cartC_ram(int val, void *param)
{
    cartC_ram = val ? 1 : 0;
#if 0
    /* FIXME: block C is not available at the expansion port? */
    export_remove(&export_resC);
    if (cartC_ram) {
        if (export_add(&export_resC) < 0) {
            return -1;
        }
    }
#endif
    generic_cartrom_to_mem_hack();
    mem_initialize_memory_bank(15);
    return 0;
}

/*
    we have 3 resources for the main cart that may be changed in arbitrary order:

    - cartridge type
    - cartridge file name
    - cartridge change reset behaviour

    the following functions try to deal with this in a hopefully sane way... however,
    do _NOT_ change the used resources from the (G)UI directly. (used the set_default
    function instead)
*/

static int try_cartridge_attach(int type, const char *filename)
{
    if (filename) {
        if (util_file_exists(filename)) {
            DBG(("try_cartridge_attach: '%s'", filename));
#if 1
            if (crt_getid(filename) >= 0) {
                DBG(("try_cartridge_attach: attach .crt file '%s'", filename));
                cartridge_type = CARTRIDGE_CRT; /* resource value modified */
                return cartridge_attach_image(CARTRIDGE_CRT, filename);
            } else
#endif
            if ((type != CARTRIDGE_NONE) && (type != CARTRIDGE_CRT)) {
                DBG(("try_cartridge_attach: attach binary file '%s'", filename));
                cartridge_type = type; /* resource value modified */
                return cartridge_attach_image(type, filename);
            }
        } else {
            DBG(("try_cartridge_attach: cartridge_file does not exist: '%s'", filename));
        }
    }

    return 0;
}

static int set_cartridge_type(int val, void *param)
{
    switch (val) {
        case CARTRIDGE_NONE:    /* fall through */
        case CARTRIDGE_CRT:

        case CARTRIDGE_CBM2_GENERIC_C1:
        case CARTRIDGE_CBM2_GENERIC_C2:
        case CARTRIDGE_CBM2_GENERIC_C4:
        case CARTRIDGE_CBM2_GENERIC_C6:
            /* add extra cartridges here */
            break;
        default:
            return -1;
    }

    DBG(("set_cartridge_type: %d", val));
    if (cartridge_type != val) {
        DBG(("cartridge_type changed: %d", val));
        cartridge_type = val;
        return try_cartridge_attach(cartridge_type, cartridge_file);
    }

    return 0;
}

/*
*/
static int set_cartridge_file(const char *name, void *param)
{
    DBG(("set_cartridge_file: '%s'", name));
    if (cartridge_file == NULL) {
        util_string_set(&cartridge_file, ""); /* resource value modified */
    }

    if (!strcmp(cartridge_file, name)) {
        return 0;
    }

    if (name == NULL || !strlen(name)) {
        cartridge_detach_image(-1);
        return 0;
    }

    DBG(("cartridge_file changed: '%s'", name));

    if (util_file_exists(name)) {
        util_string_set(&cartridge_file, name); /* resource value modified */
        return try_cartridge_attach(cartridge_type, cartridge_file);
    } else {
        DBG(("cartridge_file does not exist: '%s'", name));
        cartridge_type = CARTRIDGE_NONE; /* resource value modified */
        util_string_set(&cartridge_file, ""); /* resource value modified */
    }

    return 0;
}

static int set_cartridge_reset(int value, void *param)
{
    int val = value ? 1 : 0;

/*    DBG(("cbm2cartridge_reset: %d", val)); */
    if (cbm2cartridge_reset != val) {
        DBG(("cbm2cartridge_reset changed: %d", val));
        cbm2cartridge_reset = val; /* resource value modified */
    }
    return 0;
}

static const resource_int_t resources_int[] = {
    { "CartridgeReset", 1, RES_EVENT_NO, NULL,
      &cbm2cartridge_reset, set_cartridge_reset, NULL },
    { "CartridgeType", CARTRIDGE_NONE,
      RES_EVENT_STRICT, (resource_value_t)CARTRIDGE_NONE,
      &cartridge_type, set_cartridge_type, NULL },
    { "Ram08", 0, RES_EVENT_NO, NULL,
      &cart08_ram, set_cart08_ram, NULL },
    { "Ram1", 0, RES_EVENT_NO, NULL,
      &cart1_ram, set_cart1_ram, NULL },
    { "Ram2", 0, RES_EVENT_NO, NULL,
      &cart2_ram, set_cart2_ram, NULL },
    { "Ram4", 0, RES_EVENT_NO, NULL,
      &cart4_ram, set_cart4_ram, NULL },
    { "Ram6", 0, RES_EVENT_NO, NULL,
      &cart6_ram, set_cart6_ram, NULL },
    { "RamC", 0, RES_EVENT_NO, NULL,
      &cartC_ram, set_cartC_ram, NULL },
    RESOURCE_INT_LIST_END
};

static const resource_string_t resources_string[] = {
    { "CartridgeFile", "", RES_EVENT_NO, NULL,
      &cartridge_file, set_cartridge_file, NULL },
    RESOURCE_STRING_LIST_END
};

int cartridge_resources_init(void)
{
    /* first the general int resource, so we get the "Cartridge Reset" one */
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    return generic_resources_init();
}

void cartridge_resources_shutdown(void)
{
    generic_resources_shutdown();
    lib_free(cartridge_file);
    lib_free(cartfile);
}

/* ---------------------------------------------------------------------*/

void cartridge_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    *base = NULL;
    *start = 0;
    *limit = 0;
}

/* ---------------------------------------------------------------------*/

/* called by cbm2.c:machine_specific_reset (calls XYZ_reset) */
void cartridge_reset(void)
{
    /* cart_unset_alarms(); */
    /* cart_reset_memptr(); */
#if 0
    switch (mem_cartridge_type) {
        /* add extra cartridges */
    }
#endif
}

/* called by cbm2.c:machine_specific_powerup (calls XYZ_reset) */
void cartridge_powerup(void)
{
#if 0
    switch (mem_cartridge_type) {
        /* add extra cartridges */
    }
#endif
}

void cart_power_off(void)
{
    if (cbm2cartridge_reset) {
        /* "Turn off machine before removing cartridge" */
        machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);
    }
}

/*
    Attach cartridge from snapshot

    Sets static variables related to the "Main Slot".
*/
static void cart_attach_from_snapshot(int type)
{
    cbm2cart_type = type;
}

static void cbm2cart_detach_cartridges(void)
{
    DBG(("cbm2cart_detach_cartridges"));
#if 1
    resources_set_string("Cart1Name", "");
    resources_set_string("Cart2Name", "");
    resources_set_string("Cart4Name", "");
    resources_set_string("Cart6Name", "");
#endif
#if 1
    generic_detach(CARTRIDGE_CBM2_GENERIC_C1 |
                   CARTRIDGE_CBM2_GENERIC_C2 |
                   CARTRIDGE_CBM2_GENERIC_C4 |
                   CARTRIDGE_CBM2_GENERIC_C6);
    /* add extra cartridges here */
#endif
    mem_cartridge_type = CARTRIDGE_NONE;

    cart_power_off();
}

void cartridge_detach_image(int type)
{
    DBG(("cartridge_detach_image type %04x", (unsigned)type));
    if (type < 0) {
        cbm2cart_detach_cartridges();
    } else {
        if (CARTRIDGE_CBM2_IS_GENERIC(type)) {
            generic_detach(type);
        } else {
#if 0
            /* add extra cartridges here */
            switch (type) {
            }
#endif
        }
        cart_power_off();
    }
}

/*
    set currently active cart as default
*/
void cartridge_set_default(void)
{
    int type = CARTRIDGE_NONE;

    if (cartfile != NULL) {
        if (util_file_exists(cartfile)) {
#if 1
            if (crt_getid(cartfile) >= 0) {
                type = CARTRIDGE_CRT;
            } else
#endif
            {
                type = cbm2cart_type;
            }
        } else {
            DBG(("cartridge_set_default: file does not exist: '%s'", cartfile ? cartfile : "NULL"));
        }
    } else {
        DBG(("cartridge_set_default: no filename\n"));
    }
    DBG(("cartridge_set_default: id %d '%s'", type, cartfile ? cartfile : "NULL"));

    if (type == CARTRIDGE_NONE) {
        util_string_set(&cartridge_file, ""); /* resource value modified */
    } else {
        util_string_set(&cartridge_file, cartfile); /* resource value modified */
    }
    cartridge_type = type; /* resource value modified */
}

/** \brief  Wipe "default cartidge"
 */
void cartridge_unset_default(void)
{
    util_string_set(&cartridge_file, "");
    cartridge_type = CARTRIDGE_NONE;
}

/* FIXME: this function must make sure it produces no false positives */
int cartridge_detect(const char *filename)
{
    int type = CARTRIDGE_NONE;
    FILE *fd;

    fd = fopen(filename, "rb");
    if (fd == NULL) {
        return CARTRIDGE_NONE;
    }
#if 0
    /* add detection here */
#endif
    fclose (fd);

    DBG(("detected cartridge type: %04x", (unsigned int)type));

    return type;
}

static int cart_bin_attach(int type, const char *filename, uint8_t *rawcart)
{
    if (CARTRIDGE_CBM2_IS_GENERIC(type)) {
        return generic_bin_attach(type, filename, rawcart);
    }
#if 0
    /* add extra cartridges here */
    switch (type) {
    }
#endif
    log_error(LOG_DEFAULT,
              "cartridge_bin_attach: unsupported type (%04x)", (unsigned int)type);
    return -1;
}

/*
    called by cartridge_attach_image after cart_crt/bin_attach
    XYZ_config_setup should copy the raw cart image into the
    individual implementations array.
*/
static void cart_attach(int type, uint8_t *rawcart)
{
    /* cart_detach_conflicting(type); */
    if (CARTRIDGE_CBM2_IS_GENERIC(type)) {
        generic_config_setup(rawcart);
    } else {
#if 0
        /* add extra cartridges here */
        switch (type) {
        }
#endif
    }
}

/*
    returns -1 on error, else a positive CRT ID

    FIXME: to simplify this function a little bit, all subfunctions should
           also return the respective CRT ID on success
*/
static int crt_attach(const char *filename, uint8_t *rawcart)
{
    crt_header_t header;
    int rc, new_crttype;
    FILE *fd;

    DBG(("crt_attach: %s", filename));

    fd = crt_open(filename, &header);

    if (fd == NULL) {
        return -1;
    }

    new_crttype = header.type;
    if (new_crttype & 0x8000) {
        /* handle our negative test IDs */
        new_crttype -= 0x10000;
    }

    DBG(("crt_attach ID: %d", new_crttype));

/*  cart should always be detached. there is no reason for doing fancy checks
    here, and it will cause problems incase a cart MUST be detached before
    attaching another, or even itself. (eg for initialization reasons)

    most obvious reason: attaching a different ROM (software) for the same
    cartridge (hardware) */

    cartridge_detach_image(new_crttype);

    switch (new_crttype) {
        case CARTRIDGE_CRT:
            rc = generic_crt_attach(fd, rawcart);
            if (rc != CARTRIDGE_NONE) {
                new_crttype = rc;
            }
            break;
        /* add extra cartridges here */
        default:
            archdep_startup_log_error("unknown CRT ID: %d", new_crttype);
            rc = -1;
            break;
    }

    fclose(fd);

    if (rc == -1) {
        DBG(("crt_attach error (%d)", rc));
        return -1;
    }
    DBG(("crt_attach return ID: %d", new_crttype));
    return new_crttype;
}

/*
    attach cartridge image

    type == -1  NONE
    type ==  0  CRT format

    returns -1 on error, 0 on success
*/

int cartridge_attach_image(int type, const char *filename)
{
    unsigned char *rawcartdata;  /* raw cartridge data while loading/attaching */
    char *abs_filename;
    int cartid = type; /* FIXME: this will get the crtid */
    int carttype = CARTRIDGE_NONE;
    /* FIXME: we should convert the intermediate type to generic type 0 */

    if (filename == NULL) {
        return -1;
    }

    /* Attaching no cartridge always works. */
    if (type == CARTRIDGE_NONE || *filename == '\0') {
        return 0;
    }

    if (archdep_path_is_relative(filename)) {
        archdep_expand_path(&abs_filename, filename);
    } else {
        abs_filename = lib_strdup(filename);
    }

    DBG(("CART: detach slot main ID: %d", mem_cartridge_type));
    cartridge_detach_image(mem_cartridge_type);

    if (type == CARTRIDGE_CBM2_DETECT) {
        type = cartridge_detect(filename);
    }

    if (type == CARTRIDGE_CRT) {
        carttype = crt_getid(abs_filename);
        if (carttype == -1) {
            log_message(LOG_DEFAULT, "CART: '%s' is not a valid CRT file.", abs_filename);
            lib_free(abs_filename);
            return -1;
        }
    } else {
        carttype = type;
    }
    DBG(("CART: cartridge_attach_image type: %d ID: %d", type, cartid));

    /* allocate temporary array */
    rawcartdata = lib_malloc(CBM2CART_IMAGE_LIMIT);

    if (type == CARTRIDGE_CRT) {
        DBG(("CART: attach CRT ID: %d '%s'", carttype, filename));
        cartid = crt_attach(abs_filename, rawcartdata);
        if (cartid == CARTRIDGE_NONE) {
            goto exiterror;
        }
        if (type < 0) {
            DBG(("CART: attach generic CRT ID: %d", type));
        }
    } else {
       DBG(("CART: attach BIN ID: %d '%s'", carttype, filename));
        cartid = carttype;
        if (cart_bin_attach(cartid, abs_filename, rawcartdata) < 0) {
            goto exiterror;
        }
    }

    mem_cartridge_type = cartid;

    cart_attach(cartid, rawcartdata);
    cart_power_off();

    DBG(("CART: set ID: %d type: %d", carttype, type));
    cbm2cart_type = type;
    if (type == CARTRIDGE_CRT) {
        crttype = carttype;
    }
    util_string_set(&cartfile, abs_filename);

    DBG(("CART: cartridge_attach_image type: %d ID: %d done.", type, cartid));
    lib_free(rawcartdata);
    log_message(LOG_DEFAULT, "CART: attached '%s' as ID %d.", filename, cartid);
    return 0;

exiterror:
    DBG(("CART: error\n"));
    lib_free(rawcartdata);
    log_message(LOG_DEFAULT, "CART: could not attach '%s'.", filename);
    return -1;
}

/* FIXME: add additional image to standard cartridge */
int cartridge_attach_add_image(int type, const char *filename)
{
    return -1;
}

void cartridge_trigger_freeze(void)
{
}

int cartridge_save_image(int type, const char *filename)
{
    return -1;
}

int cartridge_save_secondary_image(int type, const char *filename)
{
    return -1;
}

int cartridge_flush_image(int type)
{
    return -1;
}

int cartridge_flush_secondary_image(int type)
{
    return -1;
}

int cartridge_can_save_image(int crtid)
{
    return 0;
}

int cartridge_can_flush_image(int crtid)
{
    return 0;
}

int cartridge_can_save_secondary_image(int crtid)
{
    return 0;
}

int cartridge_can_flush_secondary_image(int crtid)
{
    return 0;
}

int cartridge_enable(int crtid)
{
    return -1;
}

int cartridge_disable(int crtid)
{
    return -1;
}

int cartridge_type_enabled(int crtid)
{
    return 0;
}

/* FIXME: slot arg is ignored right now.
   this should return a valid cartridge ID for a given slot, or CARTRIDGE_NONE
   (it does NOT return CARTRIDGE_CRT)
*/
int cartridge_get_id(int slot)
{
    DBG(("cartridge_get_id(slot:%d): type:%d", slot, cbm2cart_type));
    return cbm2cart_type;
}

/* FIXME: slot arg is ignored right now.
   this should return a pointer to a filename, or NULL
*/
char *cartridge_get_filename_by_slot(int slot)
{
    DBG(("cartridge_get_filename_by_slot(slot:%d)", slot));
    return cartfile;
}

/* FIXME: slot arg is ignored right now.
   this should return a pointer to a filename, or NULL
*/
char *cartridge_get_secondary_filename_by_slot(int slot)
{
    return NULL;
}

/* ------------------------------------------------------------------------- */

/*
    Snapshot reading and writing
*/

/* FIXME: due to the snapshots being generally broken as a whole, none of this
          could be tested */

#define CBM2CART_DUMP_MAX_CARTS  1

#define CBM2CART_DUMP_VER_MAJOR   0
#define CBM2CART_DUMP_VER_MINOR   1
#define SNAP_MODULE_NAME  "CBM2CART"

int cartridge_snapshot_write_modules(struct snapshot_s *s)
{
    snapshot_module_t *m;

    uint8_t i;
    uint8_t number_of_carts = 0;
    int cart_ids[CBM2CART_DUMP_MAX_CARTS];

    memset(cart_ids, 0, sizeof(cart_ids));

    if (mem_cartridge_type != CARTRIDGE_NONE) {
        cart_ids[number_of_carts++] = mem_cartridge_type;
    }

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CBM2CART_DUMP_VER_MAJOR, CBM2CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, number_of_carts) < 0) {
        goto fail;
    }

    /* Not much to do if no carts present */
    if (number_of_carts == 0) {
        return snapshot_module_close(m);
    }

    /* Save "global" cartridge things */
    if (0
        || SMW_DW(m, (uint32_t)mem_cartridge_type) < 0
        /* || SMW_DW(m, (uint32_t)cart_freeze_alarm_time) < 0 */
        /* || SMW_DW(m, (uint32_t)cart_nmi_alarm_time) < 0 */
        ) {
        goto fail;
    }

    /* Save cart IDs */
    for (i = 0; i < number_of_carts; i++) {
        if (SMW_DW(m, (uint32_t)cart_ids[i]) < 0) {
            goto fail;
        }
    }

    /* Main module done */
    snapshot_module_close(m);
    m = NULL;

    /* Save individual cart data */
    for (i = 0; i < number_of_carts; i++) {
        switch (cart_ids[i]) {

            case CARTRIDGE_CBM2_GENERIC:
                if (generic_snapshot_write_module(s) < 0) {
                    return -1;
                }
                break;

                /* add extra cartridges here */

            default:
                /* If the cart cannot be saved, we obviously can't load it either.
                   Returning an error at this point is better than failing at later. */
                DBG(("CART snapshot save: cart %i handler missing", cart_ids[i]));
                return -1;
        }
    }

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}

int cartridge_snapshot_read_modules(struct snapshot_s *s)
{
    snapshot_module_t *m;
    uint8_t vmajor, vminor;

    uint8_t i;
    uint8_t number_of_carts;
    int cart_ids[CBM2CART_DUMP_MAX_CARTS];
    int local_cartridge_reset;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CBM2CART_DUMP_VER_MAJOR) || (vminor != CBM2CART_DUMP_VER_MINOR)) {
        goto fail;
    }

    /* disable cartridge reset while detaching old cart */
    resources_get_int("CartridgeReset", &local_cartridge_reset);
    resources_set_int("CartridgeReset", 0);
    cartridge_detach_image(-1);
    resources_set_int("CartridgeReset", local_cartridge_reset);

    if (SMR_B(m, &number_of_carts) < 0) {
        goto fail;
    }

    /* Not much to do if no carts in snapshot */
    if (number_of_carts == 0) {
        return snapshot_module_close(m);
    }

    if (number_of_carts > CBM2CART_DUMP_MAX_CARTS) {
        DBG(("CART snapshot read: carts %i > max %i", number_of_carts, CBM2CART_DUMP_MAX_CARTS));
        goto fail;
    }

    /* Read "global" cartridge things */
    if (0
        || SMR_DW_INT(m, &mem_cartridge_type) < 0
        /* || SMR_DW(m, &cart_freeze_alarm_time) < 0 */
        /* || SMR_DW(m, &cart_nmi_alarm_time) < 0 */
        ) {
        goto fail;
    }

    /* cart ID */
    for (i = 0; i < number_of_carts; i++) {
        if (SMR_DW_INT(m, &cart_ids[i]) < 0) {
            goto fail;
        }
    }

    /* Main module done */
    snapshot_module_close(m);
    m = NULL;

    /* Read individual cart data */
    for (i = 0; i < number_of_carts; i++) {
        switch (cart_ids[i]) {

            case CARTRIDGE_CBM2_GENERIC:
                if (generic_snapshot_read_module(s) < 0) {
                    goto fail2;
                }
                break;

                /* add extra cartridges here */

            default:
                DBG(("CART snapshot read: cart %i handler missing", cart_ids[i]));
                goto fail2;
        }
    }

    cart_attach_from_snapshot(cart_ids[i]);

    /* set up config */
    /* machine_update_memory_ptrs(); */

    /* restore alarms */
    /* cart_undump_alarms(); */

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
fail2:
    mem_cartridge_type = CARTRIDGE_NONE; /* Failed to load cartridge! */
    return -1;
}

