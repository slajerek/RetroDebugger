/*
 * c64scmodel.c - C64 model detection and setting.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  groepaz <groepaz@gmx.net>
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

#include <string.h>

#include "c64-resources.h"
#include "c64iec.h"
#include "c64keyboard.h"
#include "c64model.h"
#include "cia.h"
#include "machine.h"
#include "resources.h"
#include "sid.h"
#include "tapeport.h"
#include "vicetypes.h"
#include "userport.h"
#include "vicii.h"

// c64d
#include "vicii-resources.h"
#include "log.h"

/******************************************************************************/

#define CIA_MODEL_DEFAULT_OLD CIA_MODEL_6526
#define CIA_MODEL_DEFAULT_NEW CIA_MODEL_6526A

static int c64model_get_temp(int vicii_model, int sid_model, int glue_logic,
                             int cia1_model, int cia2_model, int board, int iecreset,
                             const char *kernal, const char *chargen, int kernalrev);
static void c64model_set_temp(int model, int *vicii_model, int *sid_model,
                              int *glue_logic, int *cia1_model, int *cia2_model,
                              int *board, int *iecreset,
                              const char *kernal, const char *chargen, int *kernalrev);

/******************************************************************************/

// c64d - keep resid-fp
static int is_new_sid(int model)
{
	switch (model) {
		case SID_MODEL_6581:
		case SID_MODEL_6581R4:
		case SID_MODEL_6581R3_4885:
		case SID_MODEL_6581R3_0486S:
		case SID_MODEL_6581R3_3984:
		case SID_MODEL_6581R4AR_3789:
		case SID_MODEL_6581R3_4485:
		case SID_MODEL_6581R4_1986S:
		default:
			return 0;
			
		case SID_MODEL_8580:
		case SID_MODEL_8580D:
		case SID_MODEL_8580R5_3691:
		case SID_MODEL_8580R5_3691D:
		case SID_MODEL_8580R5_1489:
		case SID_MODEL_8580R5_1489D:
			return 1;
	}
}

static int is_new_cia(int model)
{
    switch (model) {
        case CIA_MODEL_6526:
        default:
            return 0;

        case CIA_MODEL_6526A:
            return 1;
    }
}

struct model_s {
    int vicii;      /* VIC-II model */
    int video;      /* machine video timing */
    int cia;        /* old or new */
    int glue;       /* discrete or ASIC */
    int sid;        /* old or new */
    int board;
    int iecreset;
    int datasette;
    int iec;
    int userport;
    int keyboard;
    char *kernalname;
    char *chargenname;
    int kernalrev;
};

static struct model_s c64models[] = {

    /* C64 PAL */
    { VICII_MODEL_6569, MACHINE_SYNC_PAL,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, BOARD_C64,
      IEC_SOFT_RESET, HAS_DATASETTE, HAS_IEC, HAS_USERPORT, HAS_KEYBOARD,
      "kernal", "chargen", C64_KERNAL_REV3 },

    /* C64C PAL */
    { VICII_MODEL_8565, MACHINE_SYNC_PAL,
      CIA_MODEL_DEFAULT_NEW, GLUE_CUSTOM_IC, NEW_SID, BOARD_C64,
      IEC_HARD_RESET, HAS_DATASETTE, HAS_IEC, HAS_USERPORT, HAS_KEYBOARD,
      "kernal", "chargen", C64_KERNAL_REV3 },

    /* C64 OLD PAL */
    { VICII_MODEL_6569R1, MACHINE_SYNC_PAL,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, BOARD_C64,
      IEC_SOFT_RESET, HAS_DATASETTE, HAS_IEC, HAS_USERPORT, HAS_KEYBOARD,
      "kernal", "chargen", C64_KERNAL_REV2 },

    /* C64 NTSC */
    { VICII_MODEL_6567, MACHINE_SYNC_NTSC,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, BOARD_C64,
      IEC_SOFT_RESET, HAS_DATASETTE, HAS_IEC, HAS_USERPORT, HAS_KEYBOARD,
      "kernal", "chargen", C64_KERNAL_REV3 },

    /* C64C NTSC */
    { VICII_MODEL_8562, MACHINE_SYNC_NTSC,
      CIA_MODEL_DEFAULT_NEW, GLUE_CUSTOM_IC, NEW_SID, BOARD_C64,
      IEC_HARD_RESET, HAS_DATASETTE, HAS_IEC, HAS_USERPORT, HAS_KEYBOARD,
      "kernal", "chargen", C64_KERNAL_REV3 },

    /* C64 OLD NTSC */
    { VICII_MODEL_6567R56A, MACHINE_SYNC_NTSCOLD,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, BOARD_C64,
      IEC_SOFT_RESET, HAS_DATASETTE, HAS_IEC, HAS_USERPORT, HAS_KEYBOARD,
      "kernal", "chargen", C64_KERNAL_REV1 },

    /* C64 PAL-N */
    { VICII_MODEL_6572, MACHINE_SYNC_PALN,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, BOARD_C64,
      IEC_SOFT_RESET, HAS_DATASETTE, HAS_IEC, HAS_USERPORT, HAS_KEYBOARD,
      "kernal", "chargen", C64_KERNAL_REV3 },

    /* SX64 PAL, FIXME: guessed */
    { VICII_MODEL_6569, MACHINE_SYNC_PAL,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, BOARD_C64,
      IEC_SOFT_RESET, NO_DATASETTE, HAS_IEC, HAS_USERPORT, HAS_KEYBOARD,
      "sxkernal", "chargen", C64_KERNAL_SX64 },

    /* SX64 NTSC, FIXME: guessed */
    { VICII_MODEL_6567, MACHINE_SYNC_NTSC,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, BOARD_C64,
      IEC_SOFT_RESET, NO_DATASETTE, HAS_IEC, HAS_USERPORT, HAS_KEYBOARD,
      "sxkernal", "chargen", C64_KERNAL_SX64 },

    /* C64 Japanese, FIXME: guessed */
    { VICII_MODEL_6567, MACHINE_SYNC_NTSC,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, BOARD_C64,
      IEC_SOFT_RESET, HAS_DATASETTE, HAS_IEC, HAS_USERPORT, HAS_KEYBOARD,
      "jpkernal", "jpchrgen", C64_KERNAL_JAP },

    /* C64 GS, FIXME: guessed */
    { VICII_MODEL_8565, MACHINE_SYNC_PAL,
      CIA_MODEL_DEFAULT_NEW, GLUE_CUSTOM_IC, NEW_SID, BOARD_C64,
      IEC_HARD_RESET, NO_DATASETTE, NO_IEC, NO_USERPORT, NO_KEYBOARD,
      "gskernal", "chargen", C64_KERNAL_GS64 },

    /* PET64 PAL, FIXME: guessed */
    { VICII_MODEL_6569, MACHINE_SYNC_PAL,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, BOARD_C64,
      IEC_SOFT_RESET, HAS_DATASETTE, HAS_IEC, HAS_USERPORT, HAS_KEYBOARD,
      "edkernal", "chargen", C64_KERNAL_4064 },

    /* PET64 NTSC, FIXME: guessed */
    { VICII_MODEL_6567, MACHINE_SYNC_NTSC,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, BOARD_C64,
      IEC_SOFT_RESET, HAS_DATASETTE, HAS_IEC, HAS_USERPORT, HAS_KEYBOARD,
      "edkernal", "chargen", C64_KERNAL_4064 },

    /* ultimax, FIXME: guessed */
    /* FIXME: the MAX uses a VICII 6566, currenly unemulated, NTSC-M only */
    { VICII_MODEL_6567, MACHINE_SYNC_NTSC,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, BOARD_MAX,
      IEC_SOFT_RESET, HAS_DATASETTE, NO_IEC, NO_USERPORT, HAS_KEYBOARD,
      "kernal", "chargen", C64_KERNAL_MAX },
};

/* ------------------------------------------------------------------------- */

static int c64model_get_temp(int vicii_model, int sid_model, int glue_logic,
                      int cia1_model, int cia2_model, int board,
                      int iecreset, const char *kernal, const char *chargen, int kernalrev)
{
    int new_sid;
    int new_cia;
    int i;

    if (cia1_model != cia2_model) {
        return C64MODEL_UNKNOWN;
    }

    new_sid = is_new_sid(sid_model);
    new_cia = is_new_cia(cia1_model);

    for (i = 0; i < C64MODEL_NUM; ++i) {
        if ((c64models[i].vicii == vicii_model)
            && (is_new_cia(c64models[i].cia) == new_cia)
            && (c64models[i].glue == glue_logic)
            && (c64models[i].sid == new_sid)
            && (c64models[i].board == board)
            && (c64models[i].iecreset == iecreset)
            && (c64models[i].kernalrev == kernalrev)
            && (!strcmp(c64models[i].kernalname, kernal))
            && (!strcmp(c64models[i].chargenname, chargen))) {
            return i;
        }
    }

    return C64MODEL_UNKNOWN;
}

/* get model from details */
int c64model_get_model(c64model_details_t *details)
{
    return c64model_get_temp(details->vicii_model, details->sid_model, details->glue_logic,
                             details->cia1_model, details->cia2_model,
                             details->board, details->iecreset, details->kernal, details->chargen, details->kernalrev);
}

int c64model_get(void)
{
	int modelType;
    int vicii_model, sid_model, glue_logic, cia1_model, cia2_model, board, iecreset, kernalrev;
    char c[0x10], k[0x10];
    const char *chargen = c, *kernal = k;

    if ((resources_get_int("VICIIModel", &vicii_model) < 0)
        || (resources_get_int("SidModel", &sid_model) < 0)
        || (resources_get_int("GlueLogic", &glue_logic) < 0)
        || (resources_get_int("CIA1Model", &cia1_model) < 0)
        || (resources_get_int("CIA2Model", &cia2_model) < 0)
        || (resources_get_int("BoardType", &board) < 0)
        || (resources_get_int("IECReset", &iecreset) < 0)
        || (resources_get_int("KernalRev", &kernalrev) < 0)
        || (resources_get_string("KernalName", &kernal) < 0)
        || (resources_get_string("ChargenName", &chargen) < 0)) {
        return -1;
    }

	LOGD("vicii_resources.model=%d vicii_model=%d", (BYTE)vicii_resources.model, vicii_model);
	

    modelType = c64model_get_temp(vicii_model, sid_model, glue_logic,
                             cia1_model, cia2_model, board, iecreset,
                             kernal, chargen, kernalrev);
	
	LOGD("modelType=%d", modelType);
	
	return modelType;
}

static void c64model_set_temp(int model, int *vicii_model, int *sid_model,
                       int *glue_logic, int *cia1_model, int *cia2_model,
                       int *board, int *iecreset,
                       const char *kernal, const char *chargen, int *kernalrev)
{
    int old_model;
    int old_engine;
    int old_sid_model;
    int new_sid_model;
    int old_type;
    int new_type;

    old_model = c64model_get_temp(*vicii_model, *sid_model, *glue_logic,
                                  *cia1_model, *cia2_model, *board,
                                  *iecreset, kernal, chargen, *kernalrev);

    if ((model == old_model) || (model == C64MODEL_UNKNOWN)) {
        return;
    }

    *vicii_model = c64models[model].vicii;
    *cia1_model = c64models[model].cia;
    *cia2_model = c64models[model].cia;
    *glue_logic = c64models[model].glue;
    *board = c64models[model].board;
    *iecreset = c64models[model].iecreset;
    *kernalrev = c64models[model].kernalrev;

    /* Only change the SID model if the model changes from 6581 to 8580.
       This allows to switch between "pal"/"oldpal" without changing
       the specific SID model. The current engine is preserved. */
    old_engine = (*sid_model >> 8);
    old_sid_model = (*sid_model & 0xff);

    new_sid_model = c64models[model].sid;

    old_type = is_new_sid(old_sid_model);
    new_type = is_new_sid(new_sid_model);

    if (old_type != new_type) {
        *sid_model = (old_engine << 8) | new_sid_model;
    }
}

/* get details for model */
void c64model_set_details(c64model_details_t *details, int model)
{
    c64model_set_temp(model, &details->vicii_model, &details->sid_model,
                       &details->glue_logic, &details->cia1_model, &details->cia2_model,
                       &details->board, &details->iecreset,
                       details->kernal, details->chargen, &details->kernalrev);
}

void c64model_set(int model)
{
    int old_model;
    int old_engine;
    int old_sid_model;
    int old_type;
    int new_sid_model;
    int new_type;

    old_model = c64model_get();

	// C64D: this is making troubles in custom models...
//    if ((model == old_model) || (model == C64MODEL_UNKNOWN)) {
//        return;
//    }

    resources_set_int("VICIIModel", c64models[model].vicii);
    resources_set_int("CIA1Model", c64models[model].cia);
    resources_set_int("CIA2Model", c64models[model].cia);
    resources_set_int("GlueLogic", c64models[model].glue);
    resources_set_int("BoardType", c64models[model].board);
    resources_set_int("IECReset", c64models[model].iecreset);

    resources_set_string("KernalName", c64models[model].kernalname);
    resources_set_string("ChargenName", c64models[model].chargenname);

    resources_set_int("KernalRev", c64models[model].kernalrev);

    /* Only change the SID model if the model changes from 6581 to 8580
       or the specific SID type changes if residfp is used. This allows
       to switch between "pal"/"oldpal" without changing the specific
       SID model. The current engine is preserved. */
    resources_get_int("SidEngine", &old_engine);
    resources_get_int("SidModel", &old_sid_model);
    new_sid_model = c64models[model].sid;

    old_type = is_new_sid(old_sid_model);
    new_type = is_new_sid(new_sid_model);

    if (old_type != new_type) {
        sid_set_engine_model(old_engine, new_sid_model);
    }
    userport_enable(c64models[model].userport);
    c64keyboard_enable(c64models[model].keyboard);
    c64iec_enable(c64models[model].iec);
    tapeport_enable(c64models[model].datasette);
}
