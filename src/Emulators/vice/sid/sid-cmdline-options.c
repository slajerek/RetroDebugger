/*
 * sid-cmdline-options.c - SID command line options.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "hardsid.h"
#include "lib.h"
#include "machine.h"
#include "resources.h"
#include "sid.h"
#include "sid-cmdline-options.h"
#include "sid-resources.h"
#include "usbsid.h"
#include "util.h"

static char *sid_address_range = NULL;

struct engine_s {
    const char *name;
    int engine;
};

static const struct engine_s engine_match[] = {
    { "0", SID_FASTSID_6581 },
    { "fast", SID_FASTSID_6581 },
    { "fastold", SID_FASTSID_6581 },
    { "fast6581", SID_FASTSID_6581 },
    { "1", SID_FASTSID_8580 },
    { "fastnew", SID_FASTSID_8580 },
    { "fast8580", SID_FASTSID_8580 },
#ifdef HAVE_RESID
    { "256", SID_RESID_6581 },
    { "resid", SID_RESID_6581 },
    { "residold", SID_RESID_6581 },
    { "resid6581", SID_RESID_6581 },
    { "257", SID_RESID_8580 },
    { "residnew", SID_RESID_8580 },
    { "resid8580", SID_RESID_8580 },
    { "258", SID_RESID_8580D },
    { "residdigital", SID_RESID_8580D },
    { "residd", SID_RESID_8580D },
    { "residnewd", SID_RESID_8580D },
    { "resid8580d", SID_RESID_8580D },
    { "260", SID_RESID_DTVSID },
    { "dtv", SID_RESID_DTVSID },
    { "c64dtv", SID_RESID_DTVSID },
    { "dtvsid", SID_RESID_DTVSID },
#endif
#ifdef HAVE_CATWEASELMKIII
    { "512", SID_CATWEASELMKIII },
    { "catweaselmkiii", SID_CATWEASELMKIII },
    { "catweasel3", SID_CATWEASELMKIII },
    { "catweasel", SID_CATWEASELMKIII },
    { "cwmkiii", SID_CATWEASELMKIII },
    { "cw3", SID_CATWEASELMKIII },
    { "cw", SID_CATWEASELMKIII },
#endif
#ifdef HAVE_HARDSID
    { "768", SID_HARDSID },
    { "hardsid", SID_HARDSID },
    { "hard", SID_HARDSID },
    { "hs", SID_HARDSID },
#endif
#ifdef HAVE_PARSID
    { "1024", SID_PARSID },
    { "parsid", SID_PARSID },
    { "par", SID_PARSID },
    { "lpt", SID_PARSID },
#endif
#ifdef HAVE_SSI2001
    { "1280", SID_SSI2001 },
    { "ssi2001", SID_SSI2001 },
    { "ssi", SID_SSI2001 },
#endif
#ifdef HAVE_USBSID
    { "1792", SID_USBSID },
    { "usbsid", SID_USBSID },
    { "usbs", SID_USBSID },
    { "us", SID_USBSID },
#endif
    { NULL, -1 }
};

int sid_common_set_engine_model(const char *param, void *extra_param)
{
    int engine;
    int model;
    int temp = -1;
    int i = 0;

    if (!param) {
        return -1;
    }

    do {
        if (strcmp(engine_match[i].name, param) == 0) {
            temp = engine_match[i].engine;
        }
        i++;
    } while ((temp == -1) && (engine_match[i].name != NULL));

    if (temp == -1) {
        return -1;
    }

    engine = (temp >> 8) & 0xff;
    model = temp & 0xff;

    return sid_set_engine_model(engine, model);
}

static const cmdline_option_t sidengine_cmdline_options[] = {
    { "-sidenginemodel", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      sid_common_set_engine_model, NULL, NULL, NULL,
      "<engine and model>", "Specify SID engine and model (0: FastSID 6581, 1: FastSID 8580, 256: ReSID 6581, 257: ReSID 8580, 258: ReSID 8580 + digiboost)" },
    CMDLINE_LIST_END
};

#ifdef HAVE_RESID
static const cmdline_option_t siddtvengine_cmdline_options[] = {
    { "-sidenginemodel", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      sid_common_set_engine_model, NULL, NULL, NULL,
      "<engine and model>", "Specify SID engine and model (0: FastSID 6581, 1: FastSID 8580, 256: ReSID 6581, 257: ReSID 8580, 258: ReSID 8580 + digiboost, 260: DTVSID)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t resid_cmdline_options[] = {
    { "-residsamp", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResidSampling", NULL,
      "<method>", "reSID sampling method (0: fast, 1: interpolating, 2: resampling, 3: fast resampling)" },
    { "-residpass", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResidPassband", NULL,
      "<percent>", "reSID resampling passband in percentage of total bandwidth (0 - 90)" },
    { "-residgain", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResidGain", NULL,
      "<percent>", "reSID gain in percent (90 - 100)" },
    { "-residfilterbias", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResidFilterBias", NULL,
      "<number>", "reSID filter bias setting, which can be used to adjust DAC bias in millivolts." },
    { "-resid8580pass", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResid8580Passband", NULL,
      "<percent>", "reSID 8580 resampling passband in percentage of total bandwidth (0 - 90)" },
    { "-resid8580gain", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResid8580Gain", NULL,
      "<percent>", "reSID 8580 gain in percent (90 - 100)" },
    { "-resid8580filterbias", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResid8580FilterBias", NULL,
      "<number>", "reSID 8580 filter bias setting, which can be used to adjust DAC bias in millivolts.", },
    CMDLINE_LIST_END
};
#endif

#ifdef HAVE_HARDSID
static const cmdline_option_t hardsid_cmdline_options[] = {
    { "-hardsidmain", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidHardSIDMain", NULL,
      "<device>", "Set the HardSID device for the main SID output" },
    { "-hardsidright", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidHardSIDRight", NULL,
      "<device>", "Set the HardSID device for the right SID output" },
    CMDLINE_LIST_END
};
#endif

#ifdef HAVE_USBSID
static const cmdline_option_t usbsid_cmdline_options[] = {
    { "-usreadmode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidUSBSIDReadMode", NULL,
      "<1 or 0>", "Enable USBSID-Pico read mode (disables cycled writing & digiplay)" },
    { "-usaudiomode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidUSBSIDAudioMode", NULL,
      "<1 or 0>", "Set USBSID-Pico PCB (v1.3) audio mode to Stereo (1) or Mono (0) (default)" },
    { "-usdiffsize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidUSBSIDDiffSize", NULL,
      "<number divisable by 8>", "Set USBSID-Pico write buffer head -> tail diff size (default: 64)" },
    { "-usbuffsize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidUSBSIDBufferSize", NULL,
      "<number divisable by 8>", "Set USBSID-Pico write buffer size (default: 8192)" },
    CMDLINE_LIST_END
};
#endif

static cmdline_option_t stereo_cmdline_options[] = {
    { "-sidstereo", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidStereo", NULL,
      "<amount>", "amount of extra SID chips. (0..2)" },
    { "-sidstereoaddress", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidStereoAddressStart", NULL,
      "<Base address>", "Specify base address for 2nd SID" },
    { "-sidtripleaddress", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidTripleAddressStart", NULL,
      "<Base address>", "Specify base address for 3rd SID" },
    CMDLINE_LIST_END
};

static const cmdline_option_t common_cmdline_options[] = {
    { "-sidfilters", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SidFilters", (void *)1,
      NULL, "Emulate SID filters" },
    { "+sidfilters", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SidFilters", (void *)0,
      NULL, "Do not emulate SID filters" },
    CMDLINE_LIST_END
};

static char *generate_sid_address_range(void)
{
    char *temp1, *temp2, *temp3;

    temp3 = lib_strdup(". (");

    temp1 = util_gen_hex_address_list(0xd420, 0xd500, 0x20);
    temp2 = util_concat(temp3, temp1, "/", NULL);
    lib_free(temp3);
    lib_free(temp1);
    temp3 = temp2;

    if (machine_class == VICE_MACHINE_C128) {
        temp1 = util_gen_hex_address_list(0xd700, 0xd800, 0x20);
    } else {
        temp1 = util_gen_hex_address_list(0xd500, 0xd800, 0x20);
    }

    temp2 = util_concat(temp3, temp1, "/", NULL);
    lib_free(temp3);
    lib_free(temp1);
    temp3 = temp2;

    temp1 = util_gen_hex_address_list(0xde00, 0xe000, 0x20);
    temp2 = util_concat(temp3, temp1, ")", NULL);
    lib_free(temp3);
    lib_free(temp1);

    return temp2;
}

int sid_cmdline_options_init(void)
{
#ifdef HAVE_RESID
    if (machine_class == VICE_MACHINE_C64DTV) {
        if (cmdline_register_options(siddtvengine_cmdline_options) < 0) {
            return -1;
        }
    } else {
        if (cmdline_register_options(sidengine_cmdline_options) < 0) {
            return -1;
        }
    }
#else
    if (cmdline_register_options(sidengine_cmdline_options) < 0) {
        return -1;
    }
#endif

#ifdef HAVE_RESID
    if (cmdline_register_options(resid_cmdline_options) < 0) {
        return -1;
    }
#endif

#ifdef HAVE_USBSID
    if (cmdline_register_options(usbsid_cmdline_options) < 0) {
        return -1;
    }
#endif

#ifdef HAVE_HARDSID
    if (hardsid_available()) {
        if (cmdline_register_options(hardsid_cmdline_options) < 0) {
            return -1;
        }
    }
#endif

    if ((machine_class != VICE_MACHINE_C64DTV) &&
        (machine_class != VICE_MACHINE_VIC20) &&
        (machine_class != VICE_MACHINE_PLUS4) &&
        (machine_class != VICE_MACHINE_PET) &&
        (machine_class != VICE_MACHINE_CBM5x0) &&
        (machine_class != VICE_MACHINE_CBM6x0)) {

        sid_address_range = generate_sid_address_range();
        stereo_cmdline_options[1].description = sid_address_range;
        stereo_cmdline_options[2].description = sid_address_range;

        if (cmdline_register_options(stereo_cmdline_options) < 0) {
            return -1;
        }
    }
    return cmdline_register_options(common_cmdline_options);
}

void sid_cmdline_options_shutdown(void)
{
    if (sid_address_range) {
        lib_free(sid_address_range);
    }
}
