/** \file   settings_model.c
 * \brief   Model settings dialog
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 * \author  Groepaz <groepaz@gmx.de>
 */

/*
 * $VICERES IECReset            x64 x64sc xscpu64
 * $VICERES GlueLogic           x64 x64sc xscpu64
 * $VICERES Go64Mode            x128
 * $VICERES DtvRevision         x64dtv
 * $VICERES VICIINewLuminances  x64dtv
 * $VICERES HummerADC           x64dtv
 *
 * following are set internally:
 *
 * $VICERES BoardType           x64 x64sc xscpu64
 * $VICERES DTVFlashRevision    x64dtv
 *
 *  (for more, see used widgets)
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "archdep.h"
#include "c128machinetypewidget.h"
#include "c64dtvmodel.h"
#include "c64model.h"
#include "cbm2hardwiredswitcheswidget.h"
#include "cbm2memorysizewidget.h"
#include "cbm2rammappingwidget.h"
#include "ciamodelwidget.h"
#include "crtcontrolwidget.h"
#include "debug_gtk3.h"
#include "kernalrevisionwidget.h"
#include "machine.h"
#include "machinemodelwidget.h"
#include "machinepowerfrequencywidget.h"
#include "petiosizewidget.h"
#include "petkeyboardtypewidget.h"
#include "petmiscwidget.h"
#include "petmodel.h"
#include "petram9widget.h"
#include "petramawidget.h"
#include "petramsizewidget.h"
#include "petvideosizewidget.h"
#include "plus4aciawidget.h"
#include "plus4memhacks.h"
#include "plus4memoryexpansionwidget.h"
#include "plus4memorysizewidget.h"
#include "resourcecheckbutton.h"
#include "resources.h"
#include "sidmodelwidget.h"
#include "superpetwidget.h"
#include "uistatusbar.h"
#include "v364speechwidget.h"
#include "vdcmodelwidget.h"
#include "vic20memoryexpansionwidget.h"
#include "vice_gtk3.h"
#include "videomodelwidget.h"

#include "settings_model.h"


/*
 * Forward declarations
 */
static void plus4_debug_dump_resources(void);
static void machine_model_handler_pet(int model);


/** \brief  List of C64DTV revisions
 *
 */
static const vice_gtk3_radiogroup_entry_t c64dtv_revisions[] = {
    { "DTV2", DTVREV_2 },
    { "DTV3", DTVREV_3 },
    { NULL,   -1 }
};


/*
 * Function pointers
 */

/** \brief  Function to call to get the model for the current machine
 */
static int (*get_model_func)(void);

/** \brief  Function to call to get the memhack name */
static const char *(*get_memhack_func)(int hack);


/** \brief  Machine model widget
 *
 * This widget controls which machine model is used.
 *
 * Changing the machine model means a lot of other resources being changed, and
 * changing a resource changes the model (or invalidates it).
 */
static GtkWidget *machine_widget = NULL;

/** \brief  CIA model widget
 *
 * Used by C64, C64SC, SCPU64, C64DTV, C128, CBM5x0, CBM-II, VSID
 */
static GtkWidget *cia_widget = NULL;

/** \brief  Video model widget
 */
static GtkWidget *video_widget = NULL;

/** \brief  Machine power frequency widget */
static GtkWidget *power_frequency_widget;

/** \brief  RAM widget */
static GtkWidget *ram_widget = NULL;

/** \brief  Memhacks widget
 *
 * Hardware hacks related to installed memory, x64/x64sc only.
 */
static GtkWidget *memhack_widget = NULL;

/** \brief  ACIA widget
 *
 * Plus4 only.
 */
static GtkWidget *acia_widget = NULL;

/** \brief  V364 speech widget
 *
 * Used only in xplus4.
 */
static GtkWidget *speech_widget = NULL;

/** \brief  VDC display widget
 *
 * Used in x128.
 */
static GtkWidget *vdc_widget = NULL;

/** \brief  SID widget */
static GtkWidget *sid_widget = NULL;

/** \brief  KERNAL widget */
static GtkWidget *kernal_widget = NULL;

/** \brief  PET video widget */
static GtkWidget *pet_video_size_widget = NULL;

/** \brief  PET keyboard widget */
static GtkWidget *pet_keyboard_widget = NULL;

/** \brief  PET miscellaneous settings widget */
static GtkWidget *pet_misc_widget = NULL;

/** \brief  PET I/O size widget */
static GtkWidget *pet_io_widget = NULL;

/** \brief  PET RAM9 ($9000-$9fff) widget */
static GtkWidget *pet_ram9_widget = NULL;

/** \brief  PET RAMA ($a000-$afff) widget */
static GtkWidget *pet_rama_widget = NULL;

/** \brief  C64 DTV revision widget */
static GtkWidget *c64dtv_rev_widget = NULL;

/** \brief  C64 DTV Hummer ADC widget */
static GtkWidget *c64dtv_hummer_adc_widget = NULL;

/** \brief  Reset with IEC checkbox */
static GtkWidget *reset_with_iec_widget = NULL;

/** \brief  C64 "discrete glue logic" radio button */
static GtkWidget *c64_discrete_radio = NULL;

/** \brief  C64 "custom glue logic" radio button */
static GtkWidget *c64_custom_radio = NULL;

/** \brief  Current value of MachineVideoStandard resource
 *
 * Is set in the constructor and checked and updated in update_crt_controls()
 */
static int mvs_current = 0;


/** \brief  Update CRT controls
 *
 * Checks old and new values for the "MachineVideoStandard" resource and if the
 * values switched from PAL to NTSC or vice versa the status bar CRT widgets
 * are recreated.
 */
static void update_crt_controls(void)
{
    int  mvs_new = 0;
    bool pal_cur;
    bool pal_new;

    resources_get_int("MachineVideoStandard", &mvs_new);
    debug_gtk3("called: machine sync was %d, is now %d\n", mvs_current, mvs_new);

    pal_cur = (mvs_current == MACHINE_SYNC_PAL || mvs_current == MACHINE_SYNC_PALN);
    pal_new = (mvs_new == MACHINE_SYNC_PAL || mvs_new == MACHINE_SYNC_PALN);

    if (pal_cur != pal_new) {
        debug_gtk3("recreating CRT controls");
        ui_statusbar_recreate_crt_controls();
    }
    mvs_current = mvs_new;
}


/** \brief  Function called video model changes
 *
 * \param[in]   model   new videochip model
 */
static void video_model_callback(int model)
{
    if (get_model_func != NULL) {
        machine_model_widget_update(machine_widget, false);
        if (machine_class == VICE_MACHINE_PLUS4) {
            plus4_debug_dump_resources();
        }
        update_crt_controls();
    }
}

/* {{{ C128 glue logic
 *
 * x128-specific callbacks
 */


/** \brief  Function called on VDC revision changes
 *
 * \param[in]   revision    new VDC revision (unused)
 */
static void vdc_revision_callback(int revision)
{
    if (get_model_func != NULL) {
        machine_model_widget_update(machine_widget, false);
    }
}


/** \brief  Function called on VDC RAM changes
 *
 * \param[in]   state   new VDC RAM state (unused)
 */
static void vdc_ram_callback(int state)
{
    if (get_model_func != NULL) {
        machine_model_widget_update(machine_widget, false);
    }
}

/* }}} */

/** \brief  Function called on SID model changes
 *
 * \param[in]   model   new SID model
 */
static void sid_model_callback(int model)
{
    if (get_model_func != NULL) {
        machine_model_widget_update(machine_widget, false);
    }
}

/** \brief  Custom callback for the Kernal Revision widget
 *
 * Triggers an update of the 'machine model' widget when a different kernal rev
 * has been selected. Only valid for x64/x64sc as far as I know.
 *
 * \param[in]   rev     new KERNAL revision
 */
static void kernal_revision_callback(int rev)
{
    machine_model_widget_update(machine_widget, false);
}

/** \brief  Function called on IEC checkbox toggles
 *
 * \param[in]   widget  IEC widget (unused)
 * \param[in]   data    extra event data (unused)
 */
static void iec_callback(GtkWidget *widget, gpointer data)
{
    machine_model_widget_update(machine_widget, false);
}

/** \brief  Callback for CIA model changes
 *
 * \param[in]   cia_num     CIA number (1 or 2)
 * \param[in]   cia_model   CIA model ID
 */
static void cia_model_callback(int cia_num, int cia_model)
{
    if (get_model_func != NULL) {
        machine_model_widget_update(machine_widget, false);
    }
}

/** \brief  Callback for machine power frequency changes
 *
 * \param[in]   self        radio group with the frequency radio buttons (ignored)
 * \param[in]   frequency   new frequency of the "MachinePowerFrequency" resource (ignored)
 */
static void power_frequency_callback(GtkWidget *self, int frequency)
{
    debug_gtk3("called with frequency %d", frequency);
    if (get_model_func != NULL) {
        machine_model_widget_update(machine_widget, false);
    }
}

/* {{{ PET glue logic */

/** \brief  Callback for PET RAM size changes
 *
 * \param[in]   size    RAM size in KiB (unused)
 */
static void pet_ram_size_callback(int size)
{
    if (get_model_func != NULL) {
        /* resources may have changed; sync widgets */
        machine_model_widget_update(machine_widget, true);
    }
}


/** \brief  Function called on PET I/O size changes
 *
 * \param[in]   size    new I/O size (unused)
 */
static void pet_video_size_callback(int size)
{
    if (get_model_func != NULL) {
        /* resources may have changed; sync widgets */
        machine_model_widget_update(machine_widget, true);
    }
}


/** \brief  Function called on PET keyboard type changes
 *
 * \param[in]   type    new keyboard type (unused)
 */
static void pet_keyboard_type_callback(int type)
{
    if (get_model_func != NULL) {
        /* resources may have changed; sync widgets */
        machine_model_widget_update(machine_widget, true);
    }
}


/** \brief  Function called on PET CRTC changes
 *
 * \param[in]   state   new CRTC state (unused)
 */
static void pet_crtc_callback(int state)
{
    if (get_model_func != NULL) {
        /* resources may have changed; sync widgets */
        machine_model_widget_update(machine_widget, true);
    }
}


/** \brief  Function called on PET blank-on-eoi changes
 *
 * \param[in]   state   new blank-on-eoi state (unused)
 */
static void pet_blank_callback(int state)
{
    if (get_model_func != NULL) {
        /* resources may have changed; sync widgets */
        machine_model_widget_update(machine_widget, true);
    }
}


/** \brief  Function called on PET screen-mirrors-like-2001 changes
 *
 * \param[in]   state   new blank-on-eoi state (unused)
 */
static void pet_screen2001_callback(int state)
{
    if (get_model_func != NULL) {
        /* resources may have changed; sync widgets */
        machine_model_widget_update(machine_widget, true);
    }
}


/** \brief  Callback for SuperPET I/O changes
 *
 * \param[in]   state    enabled
 */
static void superpet_enable_callback(int state)
{
    if (get_model_func != NULL) {
        /* resources may have changed; sync widgets */
        machine_model_widget_update(machine_widget, true);
    }
}


/** \brief  Function called on PET I/O changes
 *
 * \param[in]   state   new I/O state (unused)
 */
static void pet_io_callback(int state)
{
    if (get_model_func != NULL) {
        /* resources may have changed; sync widgets */
        machine_model_widget_update(machine_widget, true);
    }
}


/** \brief  Function called on PET RAM9 changes
 *
 * \param[in]   state   new RAM9 state (unused)
 */
static void pet_ram9_callback(int state)
{
    if (get_model_func != NULL) {
        /* resources may have changed; sync widgets */
        machine_model_widget_update(machine_widget, true);
    }
}


/** \brief  Function called on PET RAMA changes
 *
 * \param[in]   state   new RAMA state (unused)
 */
static void pet_rama_callback(int state)
{
    if (get_model_func != NULL) {
        /* resources may have changed; sync widgets */
        machine_model_widget_update(machine_widget, true);
    }
}
/* }}} */

/* {{{ Plus4 glue logic and helpers */

/** \brief  Debug hook: dump Plus4-related resources on stdoud
 */
static void plus4_debug_dump_resources(void)
{
#ifdef HAVE_DEBUG_GTK3UI
    int model = -1;
    int video = 0;
    int ram = 0;
    int hack = -1;
    const char *rom = NULL;

    const char *vidmodes[] = { "UNKNOWN", "PAL", "NTSC" };

    /* get model */
    if (get_model_func != NULL) {
        model = get_model_func();
    }

    /* get TED PAL/NTSC mode */
    if (resources_get_int("MachineVideoStandard", &video) < 0) {
        video = 0;
    }

    /* get RAM size */
    resources_get_int("RamSize", &ram);
    /* get memory exp hack */
    resources_get_int("MemoryHack", &hack);

    g_print("Plus4 resources dump:\n");
    g_print("    get_model_func()    : %d\n", model);
    g_print("    MachineVideoStandard: %d (%s)\n", video, vidmodes[video]);
    g_print("    RAM size            : %dKiB\n", ram);
    g_print("    MemoryHack          : %d (%s)\n",
            hack,
            get_memhack_func != NULL ?
            get_memhack_func(hack) : "get_memhack_func not set");

    /* dump active ROMs */
    resources_get_string("KernalName", &rom);
    g_print("    KernalName          : %s\n", rom);
    resources_get_string("BasicName", &rom);
    g_print("    BasicName           : %s\n", rom);
    resources_get_string("FunctionLoWName", &rom);
    g_print("    FunctionLoWName     : %s\n", rom);
    resources_get_string("FunctionHighName", &rom);
    g_print("    FunctionHighName:   : %s\n", rom);
    resources_get_string("c1loName", &rom);
    g_print("    c1loName            : %s\n", rom);
    resources_get_string("c1hiName", &rom);
    g_print("    c1hiName            : %s\n", rom);
    resources_get_string("c2loName", &rom);
    g_print("    c2loName            : %s\n", rom);
    resources_get_string("c2hiName", &rom);
    g_print("    c2hiName            : %s\n", rom);

#endif
}


/** \brief  Extra calback for the Plus4 memory size widget
 *
 * Triggered when the widget changes value.
 *
 * \param[in,out]   widget      plus4 ram size widget
 * \param[in]       value       new size in KiB
 */
static void plus4_mem_size_callback(GtkWidget *widget, int value)
{
    int size = 0;

    resources_get_int("RamSize", &size);
#if 0
    debug_gtk3("Got new value: %dKiB, RamSize = %d", value, size);
    debug_gtk3("Calling plus4_memory_expansion_widget_sync(): ");
#endif
    plus4_memory_expansion_widget_sync();
    machine_model_widget_update(machine_widget, false);
    plus4_debug_dump_resources();
}


/** \brief  Extra calback for the Plus4 memory expansion hack widget
 *
 * Triggered when the widget changes value.
 *
 * \param[in,out]   widget      plus4 memory expansion hack widget
 * \param[in]       value       new size in KiB
 */
static void plus4_mem_hack_callback(GtkWidget *widget, int value)
{
    int size = 0;

    resources_get_int("RamSize", &size);

    plus4_memory_size_widget_sync();
    gtk_widget_set_sensitive(ram_widget, value == MEMORY_HACK_NONE);
    plus4_memory_size_widget_sync();

    machine_model_widget_update(machine_widget, false);
    plus4_debug_dump_resources();
}


/** \brief  Callback to update the Plus4 model widget on ACIA widget change
 *
 * \param[in]   widget  ACIA widget
 * \param[in]   value   new value
 */
static void plus4_acia_widget_callback(GtkWidget *widget, int value)
{
    machine_model_widget_update(machine_widget, false);
}


/** \brief  Callback to update the Plus4 model widget on v364 widget change
 *
 * \param[in]   widget  v364 widget
 * \param[in]   value   new value
 */
static void v364_speech_widget_callback(GtkWidget *widget, int value)
{
    machine_model_widget_update(machine_widget, false);
}


/* }}} */

/*
 * C64(sc) model change handling
 */
static void c64_misc_widget_sync(void);

/** \brief  Callback triggered on changing machine model
 *
 * \param[in]   model   machine model
 */
static void machine_model_handler_c64(int model)
{
    GtkWidget *sid_group;

    /* synchronize video chip widget */
    video_model_widget_update(video_widget);

    /* synchronize SID chip widget */
    sid_group = gtk_grid_get_child_at(GTK_GRID(sid_widget), 0, 1);
    if (sid_group != NULL) {
        vice_gtk3_resource_radiogroup_sync(sid_group);
    }

    /* synchronize CIA widget */
    cia_model_widget_sync(cia_widget);

    /* synchronize kernal-revision widget */
    if (machine_class != VICE_MACHINE_SCPU64) {
        kernal_revision_widget_sync(kernal_widget);
    }

    /* synchronize power frequency widget */
    if (power_frequency_widget != NULL) {
        machine_power_frequency_widget_sync(power_frequency_widget);
    }

    /* synchronize misc widget */
    c64_misc_widget_sync();
}

/** \brief  Callback triggered on changing machine model
 *
 * \param[in]   model   machine model
 */
static void machine_model_handler_c128(int model)
{
    GtkWidget *sid_group;
#ifdef HAVE_DEBUG_GTK3UI
    int res_board_type = -1;
    int res_vdc_revision = -1;
    int res_vdc_64kb = -1;
    int res_machine_type = -1;
    int res_video_standard = -1;
    int res_cia1 = -1;
    int res_cia2 = -1;
    int res_sid = -1;

    debug_gtk3("Got model change for C128: %d.", model);

    resources_get_int("BoardType",      &res_board_type);
    resources_get_int("VDCRevision",    &res_vdc_revision);
    resources_get_int("VDC64KB",        &res_vdc_64kb);
    resources_get_int("MachineType",    &res_machine_type);
    resources_get_int("MachineVideoStandard",    &res_video_standard);
    resources_get_int("CIA1Model",      &res_cia1);
    resources_get_int("CIA2Model",      &res_cia2);
    resources_get_int("SIDModel",       &res_sid);

    printf("=== %s ===\n", __func__);
    printf("    BoardType             : %d\n", res_board_type);
    printf("    VDCRevision           : %d\n", res_vdc_revision);
    printf("    VDC64KB               : %d\n", res_vdc_64kb);
    printf("    MachineType           : %d\n", res_machine_type);
    printf("    MachineVideoStandard: : %d\n", res_video_standard);
    printf("    CIA1                  : %d\n", res_cia1);
    printf("    CIA2                  : %d\n", res_cia2);
    printf("    SIDModel              : %d\n", res_sid);
#endif

    /* sync video chip (VICIIe) widget */
    video_model_widget_update(video_widget);

    /* sync VDC widget */
    vdc_model_widget_update(vdc_widget);

    /* sync SID chip widget */
    sid_group = gtk_grid_get_child_at(GTK_GRID(sid_widget), 0, 1);
    if (sid_group != NULL) {
        vice_gtk3_resource_radiogroup_sync(sid_group);
    }

    /* synchronize CIA widget */
    cia_model_widget_sync(cia_widget);

    /* synchronize machine power frequency widget */
    machine_power_frequency_widget_sync(power_frequency_widget);
}

/*
 * C64DTV widget glue logic
 */

/** \brief  Callback for the DTV revision
 *
 * Calls model widget update
 *
 * \param[in]   widget      radio button triggering the callback (unused)
 * \param[in]   revision    new DTV revision (unused)
 */
static void dtv_revision_callback(GtkWidget *widget, int revision)
{
    if (get_model_func != NULL) {
        machine_model_widget_update(machine_widget, false);
    }
}

/** \brief  Callback for the DTV VIC-II model (sync factor)
 *
 * Calls model widget update
 *
 * \param[in]   model   new VIC-II model (unused)
 */
static void dtv_video_callback(int model)
{
    machine_model_widget_update(machine_widget, false);
}

/** \brief  Sync "Hummer ADC" widget with the associated resource
 *
 */
static void c64dtv_hummer_adc_sync(void)
{
    int hummer_adc = 0;

    resources_get_int("HummerAdc", &hummer_adc);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c64dtv_hummer_adc_widget),
                                                   (gboolean)hummer_adc);
}

/** \brief  Update DTV widget on 'Hummer ADC' toggle
 *
 * \param[in]   widget  check button
 * \param[in]   value   new value (ignored)
 */
static void c64dtv_hummer_adc_callback(GtkWidget *widget, gboolean value)
{
    machine_model_widget_update(machine_widget, false);
}

/** \brief  Callback for DTV machine model changes
 *
 * Updates the DTV revision and VIC-II model widgets.
 *
 * \param[in]   model   DTV model
 */
static void machine_model_handler_c64dtv(int model)
{
    GtkWidget *group;
    int        rev;

    switch (model) {
        case DTVMODEL_V2_PAL:       /* fall through */
        case DTVMODEL_V2_NTSC:
            rev = DTVREV_2;
            break;
        case DTVMODEL_V3_PAL:       /* fall through */
        case DTVMODEL_V3_NTSC:      /* fall through */
        case DTVMODEL_HUMMER_PAL:
        case DTVMODEL_HUMMER_NTSC:
            rev = DTVREV_3;
            break;
        default:
            rev = -1;
            break;
    }

    /* update revision widget */
    group = gtk_grid_get_child_at(GTK_GRID(c64dtv_rev_widget), 0, 1);
    if (group != NULL && GTK_IS_GRID(group)) {
        vice_gtk3_resource_radiogroup_set(group, rev);
    }

    /* update VIC-II model widget */
    video_model_widget_update(video_widget);
    /* update Hummer ADC widget */
    c64dtv_hummer_adc_sync();
}

/*
 * VIC-20 glue logic
 */


/** \brief  Callback for the VIC-2- VIC model (sync factor)
 *
 * Calls model widget update
 *
 * \param[in]   model   new VIC model
 */
static void vic20_video_callback(int model)
{
    machine_model_widget_update(machine_widget, false);
}

/** \brief  Callback for VIC-20 machine model changes
 *
 * \param[in]   model   VIC-20 model
 */
static void machine_model_handler_vic20(int model)
{
    /* update VIC model widget */
    video_model_widget_update(video_widget);

    /* FIXME: */
    /* vic20_memory_expansion_widget_sync(); */
}

/*
 * Plus4 glue logic
 *
 * This logic won't appear to work very well due to the Plus4 model depending
 * on other resources that aren't presented in the model setting widgets, but
 * in separate widgets/dialogs such as KernalName, ACIA1Enable, Speech, etc.
 *
 * For example having the model 'Plus4 PAL' enabled and selecting NTSC won't
 * select the model 'Plus 4 NTSC', but 'Unknown' due to the NSTC Plus4 using
 * a different Kernal.
 */

/** \brief  Callback for the Plus4 TED model (sync factor)
 *
 * Calls model widget update
 *
 * \param[in]   model   new TED model
 */
static void plus4_video_callback(int model)
{
    machine_model_widget_update(machine_widget, false);
    plus4_debug_dump_resources();
}

/** \brief  Handler for the model change for Plus4
 *
 * \param[in]   model   new model (unused, it seems)
 */
static void machine_model_handler_plus4(int model)
{
    video_model_widget_update(video_widget);
    plus4_memory_size_widget_sync();
    plus4_acia_widget_sync();
    v364_speech_widget_sync();
    plus4_debug_dump_resources();
}


/*
 * CBM-II glue logic
 */

/** \brief  Callback for the CBM-II 5x0 VIC-II model (sync factor)
 *
 * Calls model widget update.
 *
 * \param[in]   model   new VIC-II model
 */
static void cbm5x0_video_callback(int model)
{
    machine_model_widget_update(machine_widget, false);
}

/** \brief  Callback for the CBM-II 6x0/7x0 CRTC model (sync factor)
 *
 * Calls model widget update.
 *
 * \param[in]   model   new VIC-II model
 */
static void cbm2_video_callback(int model)
{
    machine_model_widget_update(machine_widget, false);
}

/** \brief  Callback for the CBM-II 6x0/7x0 ModelLine switches
 *
 * Calls model widget update.
 *
 * \param[in]   widget      switches widget (unused)
 * \param[in]   model_line  new mode line value (unused)
 */
static void cbm2_switches_callback(GtkWidget *widget, int model_line)
{
    machine_model_widget_update(machine_widget, false);
}

/** \brief  Callback for CBM-II 6x0/7x0 memory size changes
 *
 * Calls model widget update.
 *
 * \param[in]   widget  memory size widget (unused)
 * \param[in]   size    new memory size (unused)
 */
static void cbm2_memory_size_callback(GtkWidget *widget, int size)
{
    machine_model_widget_update(machine_widget, false);
}

/** \brief  Callback for CBM 5x0 model changes
 *
 * \param[in]   model   new model
 */
static void machine_model_handler_cbm5x0(int model)
{
    video_model_widget_update(video_widget);
    cbm2_memory_size_widget_update(ram_widget);
    /* synchronize machine power frequency widget */
    machine_power_frequency_widget_sync(power_frequency_widget);
}

/** \brief  Callback for CBM 6x0/7x0 model changes
 *
 * \param[in]   model   new model
 */
static void machine_model_handler_cbm6x0(int model)
{
    video_model_widget_update(video_widget);
    cbm2_memory_size_widget_update(ram_widget);
    /* synchronize machine power frequency widget */
    machine_power_frequency_widget_sync(power_frequency_widget);
}

/** \brief  Set sensitivity of PET Ram9 and RamA widgets
 *
 * Only the 8296 memory mapping has the Ram9 and RamA resources.
 * Use RamSize as a proxy to determine this.
 */
static void pet_set_ram9a_sensitivity(void)
{
    int ramSize;
    gboolean model_is_8296;

    resources_get_int("RamSize", &ramSize);
    model_is_8296 = (ramSize == 128);
    gtk_widget_set_sensitive(pet_ram9_widget, model_is_8296);
    gtk_widget_set_sensitive(pet_rama_widget, model_is_8296);
}

/** \brief  Callback for PET model changes.
 *          Also called if the model doesn't apparently change;
 *          one resource change may have triggered another so all widgets must
 *          be synced.
 *
 * \param[in]   model   new model
 */
static void machine_model_handler_pet(int model)
{
    pet_ram_size_widget_sync(ram_widget);
    pet_video_size_widget_sync(pet_video_size_widget);
    pet_keyboard_type_widget_sync(pet_keyboard_widget);
    pet_misc_widget_sync(pet_misc_widget);
    pet_io_size_widget_sync(pet_io_widget);
    pet_ram9_widget_sync(pet_ram9_widget);
    pet_rama_widget_sync(pet_rama_widget);
    pet_superpet_enable_widget_sync();
    pet_set_ram9a_sensitivity();
}

/** \brief  Generic callback for machine model changes
 *
 * \param[in]   model
 */
static void machine_model_callback(int model)
{
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:
            machine_model_handler_c64(model);
            break;
        case VICE_MACHINE_C64DTV:
            machine_model_handler_c64dtv(model);
            break;
        case VICE_MACHINE_VIC20:
            machine_model_handler_vic20(model);
            break;
        case VICE_MACHINE_PLUS4:
            machine_model_handler_plus4(model);
            break;
        case VICE_MACHINE_CBM5x0:
            machine_model_handler_cbm5x0(model);
            break;
        case VICE_MACHINE_CBM6x0:
            machine_model_handler_cbm6x0(model);
            break;
        case VICE_MACHINE_PET:
            machine_model_handler_pet(model);
            break;
        case VICE_MACHINE_C128:
            machine_model_handler_c128(model);
            break;
        default:
            debug_gtk3("unsupported machine_class %d.", machine_class);
            return;
    }
    update_crt_controls();
}

/** \brief  Handler for the 'toggled' event of the C64 "Glue Logic" radio buttons
 *
 * \param[in]   widget      radio button triggering the event
 * \param[in]   user_data   glue value (int)
 */
static void on_c64_glue_toggled(GtkWidget *widget, gpointer user_data)
{
    int glue = GPOINTER_TO_INT(user_data);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        resources_set_int("GlueLogic", glue);
        machine_model_widget_update(machine_widget, false);
    }
}

/** \brief  Sync "Reset-to-IEC" widget with the associated resource
 *
 */
static void c64_reset_with_iec_sync(void)
{
    int iecreset = 0;
    resources_get_int("IECReset", &iecreset);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(reset_with_iec_widget), iecreset);
}

/** \brief  Create widget to toggle "Reset-to-IEC"
 *
 * \return  GtkGrid
 */
static GtkWidget *create_reset_with_iec_widget(void)
{
    reset_with_iec_widget = vice_gtk3_resource_check_button_new("IECReset",
            "Reset goes to IEC");
    g_signal_connect(GTK_WIDGET(reset_with_iec_widget), "toggled",
            G_CALLBACK(iec_callback), NULL);

    return reset_with_iec_widget;
}

/** \brief  Create widget to toggle "Go64Mode"
 *
 * \return  GtkGrid
 */
static GtkWidget *create_go64_widget(void)
{
    return vice_gtk3_resource_check_button_new("Go64Mode",
            "Always switch to C64 mode on reset");
}

/** \brief  Sync "Glue Logic" widget with the associated resource
 *
 */
static void c64_glue_widget_sync(void)
{
    int glue;
    GtkWidget *radio;

    resources_get_int("GlueLogic", &glue);
    radio = (glue == 0) ? c64_discrete_radio : c64_custom_radio;
    if (radio) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
    }
}

/** \brief  Create left-aligned label with Pango markup
 *
 * \param[in]   text    text for label using Pango markup
 *
 * \return  GtkLabel
 */
static GtkWidget *label_helper(const char *text)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}

/** \brief  Create widget to select C64SC Glue Logic
 *
 * \return  GtkGrid
 */
static GtkWidget *create_c64_glue_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *radio;
    GSList *group = NULL;

    int glue;

    resources_get_int("GlueLogic", &glue);

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new("Glue logic");
    gtk_widget_set_margin_start(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    c64_discrete_radio = gtk_radio_button_new_with_label(group, "Discrete");
    c64_custom_radio = gtk_radio_button_new_with_label(group, "Custom IC");
    gtk_radio_button_join_group(GTK_RADIO_BUTTON(c64_custom_radio),
                                GTK_RADIO_BUTTON(c64_discrete_radio));

    radio = glue == 0 ? c64_discrete_radio : c64_custom_radio;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);

    g_signal_connect(c64_discrete_radio,
                     "toggled",
                     G_CALLBACK(on_c64_glue_toggled),
                     GINT_TO_POINTER(0));
    g_signal_connect(c64_custom_radio,
                     "toggled",
                     G_CALLBACK(on_c64_glue_toggled),
                     GINT_TO_POINTER(1));

    gtk_grid_attach(GTK_GRID(grid), c64_discrete_radio, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), c64_custom_radio, 1, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create 'misc' widget for C64/C64SC/SCPU64
 *
 * \return  GtkGrid
 */
static GtkWidget *create_c64_misc_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *iec_widget;
    GtkWidget *glue_widget = NULL;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = label_helper("<b>Miscellaneous</b>");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    iec_widget = create_reset_with_iec_widget();
    gtk_grid_attach(GTK_GRID(grid), iec_widget, 0, 1, 1, 1);

    /*
     * GlueLogic seems to cause timing issues when set to 'custom' on x64, so
     * don't show the widget for x64 until it is fixed (if ever)
     */
    if (machine_class == VICE_MACHINE_C64SC ||
            machine_class == VICE_MACHINE_SCPU64) {
        glue_widget = create_c64_glue_widget();
        gtk_grid_attach(GTK_GRID(grid), glue_widget, 0, 2, 1, 1);
    }

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Synchronize Glue logic and IEC widget with their resources
 */
static void c64_misc_widget_sync(void)
{
    c64_glue_widget_sync();
    c64_reset_with_iec_sync();
}

/** \brief  Create 'misc' widget for C128
 *
 * \return  GtkGrid
 */
static GtkWidget *create_c128_misc_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *go64_widget;

    /* no row spacing, we use a margin since more widgets might be added in
     * the future for which we don't want extra row spacing */
    grid = gtk_grid_new();

    label = label_helper("<b>Miscellaneous</b>");
    /* we do the extra spacing here */
    gtk_widget_set_margin_bottom(label, 8);

    go64_widget = create_go64_widget();

    gtk_grid_attach(GTK_GRID(grid), label,       0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), go64_widget, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create widget to select DTV revision
 *
 * Creates a grid with a label and a vice_gtk3_resource_radiogroup widget.
 *
 * \return  GtkGrid
 */
static GtkWidget *create_c64dtv_revision_widget(void)
{
    GtkWidget *grid;
    GtkWidget *group;
    GtkWidget *label;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>DTV Revision</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);

    group = vice_gtk3_resource_radiogroup_new("DtvRevision",
                                              c64dtv_revisions,
                                              GTK_ORIENTATION_VERTICAL);
    vice_gtk3_resource_radiogroup_add_callback(group, dtv_revision_callback);

    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create widget layout for C64/C64SC
 *
 * \param[in,out]   grid    GtkGrid to use for layout
 *
 * \return  \a grid
 */
static GtkWidget *create_c64_layout(GtkWidget *grid)
{
    GtkWidget *misc_widget;

    /* add machine widget */
    gtk_grid_attach(GTK_GRID(grid), machine_widget, 0, 0, 1, 2);

    /* VIC-II model widget */
    video_widget = video_model_widget_create(machine_widget);
    video_model_widget_set_callback(video_widget, video_model_callback);
    gtk_grid_attach(GTK_GRID(grid), video_widget, 1, 0, 1, 1);

    /* SID widget */
    sid_widget = sid_model_widget_create(machine_widget);
    sid_model_widget_set_callback(sid_widget, sid_model_callback);
    gtk_grid_attach(GTK_GRID(grid), sid_widget, 1, 1, 1, 1);

    /* CIA1 & CIA2 widget */
    cia_widget = cia_model_widget_create(2);
    cia_model_widget_set_callback(cia_widget, cia_model_callback);
    gtk_widget_set_margin_top(cia_widget, 16);
    gtk_grid_attach(GTK_GRID(grid), cia_widget, 0, 2, 2, 1);

    /* Kernal revision widget */
    if (machine_class != VICE_MACHINE_SCPU64) {
        kernal_widget = kernal_revision_widget_create();
        gtk_grid_attach(GTK_GRID(grid), kernal_widget, 2, 0, 1, 1);
        /* add custom callback */
        kernal_revision_widget_add_callback(kernal_revision_callback);
    }

    /* machine power frequency */
    power_frequency_widget = machine_power_frequency_widget_new();
    machine_power_frequency_widget_add_callback(power_frequency_widget,
                                                power_frequency_callback);
    if (machine_class == VICE_MACHINE_SCPU64) {
        gtk_grid_attach(GTK_GRID(grid), power_frequency_widget, 2, 0, 1, 1);
    } else {
        gtk_grid_attach(GTK_GRID(grid), power_frequency_widget, 2, 1, 1, 1);
    }

    /* C64 misc. model settings */
    misc_widget = create_c64_misc_widget();
    if (machine_class == VICE_MACHINE_SCPU64) {
        gtk_grid_attach(GTK_GRID(grid), misc_widget, 2, 1, 1, 1);
    } else {
        gtk_grid_attach(GTK_GRID(grid), misc_widget, 2, 2, 1, 1);
    }

    return grid;
}

/** \brief  Create widget layout for C128
 *
 * \param[in,out]   grid    GtkGrid to use for layout
 *
 * \return  \a grid
 */
static GtkWidget *create_c128_layout(GtkWidget *grid)
{
    GtkWidget *machine_type_widget;
    GtkWidget *misc_widget;

    /* wrap machine model and machine type widgets in a single widget */
    /* machine_wrapper = vice_gtk3_grid_new_spaced(0, 16); */

    /* add machine model widget (2 rows) */
    gtk_grid_attach(GTK_GRID(grid), machine_widget, 0, 0, 1, 2);

    /* add machine type widget (2 rows) */
    machine_type_widget = c128_machine_type_widget_create();
    gtk_widget_set_margin_top(machine_type_widget, 16);
    gtk_grid_attach(GTK_GRID(grid), machine_type_widget, 0, 2, 1, 2);

    /* VIC-II model widget */
    video_widget = video_model_widget_create(machine_widget);
    video_model_widget_set_callback(video_widget, video_model_callback);
    gtk_grid_attach(GTK_GRID(grid), video_widget, 1, 0, 1, 1);

    /* SID widget */
    sid_widget = sid_model_widget_create(machine_widget);
    sid_model_widget_set_callback(sid_widget, sid_model_callback);
    gtk_grid_attach(GTK_GRID(grid), sid_widget, 1, 1, 1, 1);

    /* VDC model widget */
    vdc_widget = vdc_model_widget_create();
    vdc_model_widget_set_revision_callback(vdc_revision_callback);
    vdc_model_widget_set_ram_callback(vdc_ram_callback);
    gtk_grid_attach(GTK_GRID(grid), vdc_widget, 2, 0, 1, 2);

    /* CIA1 & CIA2 widget */
    cia_widget = cia_model_widget_create(2);
    gtk_widget_set_margin_top(cia_widget, 16);
    cia_model_widget_set_callback(cia_widget, cia_model_callback);
    gtk_grid_attach(GTK_GRID(grid), cia_widget, 1, 2, 2, 1);

    /* machine power frequency */
    power_frequency_widget = machine_power_frequency_widget_new();
    machine_power_frequency_widget_add_callback(power_frequency_widget,
                                                power_frequency_callback);
    gtk_grid_attach(GTK_GRID(grid), power_frequency_widget, 1, 3, 2, 1);

    /* Misc widget */
    misc_widget = create_c128_misc_widget();
    gtk_grid_attach(GTK_GRID(grid), misc_widget, 1, 4, 2, 1);
    return grid;
}

/** \brief  Create C64DTV model settings widget layout
 *
 * \param[in]   grid    GtkGrid to attach widgets to
 *
 * \return  \a grid
 */
static GtkWidget *create_c64dtv_layout(GtkWidget *grid)
{
    GtkWidget *luma_widget;

    /** add machine widget */
    gtk_grid_attach(GTK_GRID(grid), machine_widget, 0, 0, 1, 2);

    /* add video widget */
    video_widget = video_model_widget_create(machine_widget);
    video_model_widget_set_callback(video_widget, dtv_video_callback);
    gtk_grid_attach(GTK_GRID(grid), video_widget, 1, 0, 1, 1);

    /* create revision widget */
    c64dtv_rev_widget = create_c64dtv_revision_widget();
    gtk_widget_set_margin_top(c64dtv_rev_widget, 16);
    gtk_grid_attach(GTK_GRID(grid), c64dtv_rev_widget, 1, 1, 1, 1);

    /* SID widget */
    sid_widget = sid_model_widget_create(machine_widget);
    sid_model_widget_set_callback(sid_widget, sid_model_callback);
    gtk_widget_set_margin_top(sid_widget, 16);
    gtk_grid_attach(GTK_GRID(grid), sid_widget, 0, 2, 1, 1);

    /* Luma fix widget */
    luma_widget = vice_gtk3_resource_check_button_new("VICIINewLuminances",
            "Enable LumaFix (use new VICII luminances)");
    gtk_widget_set_margin_top(luma_widget, 16);
    gtk_grid_attach(GTK_GRID(grid), luma_widget, 0, 3, 2, 1);

    /* Hummer ADC widget */
    c64dtv_hummer_adc_widget = vice_gtk3_resource_check_button_new("HummerADC",
                                                                   "Enable Hummer ADC");
    vice_gtk3_resource_check_button_add_callback(c64dtv_hummer_adc_widget,
                                                 c64dtv_hummer_adc_callback);
    gtk_grid_attach(GTK_GRID(grid), c64dtv_hummer_adc_widget, 0, 4, 2, 1);

    return grid;
}

/** \brief  Create VIC20 model settings widget layout
 *
 * \param[in]   grid    GtkGrid to attach widgets to
 *
 * \return  \a grid
 */
static GtkWidget *create_vic20_layout(GtkWidget *grid)
{
    /* add machine widget */
    gtk_grid_attach(GTK_GRID(grid), machine_widget, 0, 0, 1, 1);

    /* VIC model widget */
    video_widget = video_model_widget_create(machine_widget);
    video_model_widget_set_callback(video_widget, vic20_video_callback);
    gtk_grid_attach(GTK_GRID(grid), video_widget, 0, 1, 1, 1);

    ram_widget = vic20_memory_expansion_widget_create();
    gtk_grid_attach(GTK_GRID(grid), ram_widget, 1, 0, 1, 2);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create Plus4 model settings widget layout
 *
 * \param[in]   grid    GtkGrid to attach widgets to
 *
 * \return  \a grid
 */
static GtkWidget *create_plus4_layout(GtkWidget *grid)
{
    GtkWidget *checkboxes;
    int hack = 0;

    /* add machine widget */
    gtk_grid_attach(GTK_GRID(grid), machine_widget, 0, 0, 1, 1);

    /* Plus4 video widget */
    video_widget = video_model_widget_create(machine_widget);
    video_model_widget_set_callback(video_widget, plus4_video_callback);
    gtk_widget_set_margin_top(video_widget, 16);
    gtk_grid_attach(GTK_GRID(grid), video_widget, 0, 1, 1, 1);

    /* memory size */
    ram_widget = plus4_memory_size_widget_create();
    plus4_memory_size_widget_add_callback(plus4_mem_size_callback);
    gtk_grid_attach(GTK_GRID(grid), ram_widget, 1, 0, 1, 1);

    /* memory expansion hacks */
    memhack_widget = plus4_memory_expansion_widget_create();
    plus4_memory_expansion_widget_add_callback(plus4_mem_hack_callback);
    gtk_grid_attach(GTK_GRID(grid), memhack_widget, 2, 0, 1, 1);

    resources_get_int("MemoryHack", &hack);
    gtk_widget_set_sensitive(ram_widget, hack == MEMORY_HACK_NONE);

    /* Wrap the check boxes in a grid to get them vertically aligned properly.
     * If we make the TED widget span two rows and add the check boxes next to
     * it, the check boxes "spread out" */
    checkboxes = gtk_grid_new();
    gtk_widget_set_margin_top(checkboxes, 16);

    /* ACIA widget */
    acia_widget = plus4_acia_widget_create();
    plus4_acia_widget_add_callback(plus4_acia_widget_callback);
    gtk_grid_attach(GTK_GRID(checkboxes), acia_widget, 0, 0, 1, 1);

    /* V364 speech widget */
    speech_widget = v364_speech_widget_create();
    v364_speech_widget_add_callback(v364_speech_widget_callback);
    gtk_grid_attach(GTK_GRID(checkboxes), speech_widget, 0, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), checkboxes, 1, 1, 2, 1);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Creat PET layout
 *
 * Create PET layout using a GtkStack and GtkStackSwitcher to reduce space.
 *
 * \param[in,out]   grid    main grid to add widgets to
 *
 * \return  \a grid
 */
static GtkWidget *create_pet_layout(GtkWidget *grid)
{
    GtkWidget *stack;
    GtkWidget *switcher;
    GtkWidget *pet_grid;
    GtkWidget *superpet_grid;
    GtkWidget *ramrom_grid;

    pet_grid = gtk_grid_new();

    /* PET model list */
    gtk_grid_attach(GTK_GRID(pet_grid), machine_widget, 0, 0, 1, 3);

    pet_keyboard_widget = pet_keyboard_type_widget_create();
    pet_keyboard_type_widget_set_callback(pet_keyboard_widget,
                                          pet_keyboard_type_callback);
    gtk_grid_attach(GTK_GRID(pet_grid), pet_keyboard_widget, 1, 0, 1, 1);

    pet_video_size_widget = pet_video_size_widget_create();
    pet_video_size_widget_set_callback(pet_video_size_callback);
    gtk_widget_set_margin_top(pet_video_size_widget, 16);
    gtk_grid_attach(GTK_GRID(pet_grid), pet_video_size_widget, 1, 1, 1, 1);

    ram_widget = pet_ram_size_widget_create();
    pet_ram_size_widget_set_callback(ram_widget, pet_ram_size_callback);
    gtk_grid_attach(GTK_GRID(pet_grid), ram_widget, 2, 0, 1, 1);

    pet_io_widget = pet_io_size_widget_create();
    pet_io_size_widget_set_callback(pet_io_callback);
    gtk_widget_set_margin_top(pet_io_widget, 16);
    gtk_grid_attach(GTK_GRID(pet_grid), pet_io_widget, 2, 1, 1, 1);

    /* wrap RAM/ROM 9XXX/AXXX in a grid */
    ramrom_grid = gtk_grid_new();

    pet_ram9_widget = pet_ram9_widget_create();
    pet_ram9_widget_set_callback(pet_ram9_callback);
    gtk_grid_attach(GTK_GRID(ramrom_grid), pet_ram9_widget, 0, 0, 1, 1);

    pet_rama_widget = pet_rama_widget_create();
    pet_rama_widget_set_callback(pet_rama_callback);
    pet_set_ram9a_sensitivity();
    gtk_grid_attach(GTK_GRID(ramrom_grid), pet_rama_widget, 0, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(pet_grid), ramrom_grid, 3, 0, 1, 1);

    pet_misc_widget = pet_misc_widget_create();
    pet_misc_widget_set_crtc_callback(pet_crtc_callback);
    pet_misc_widget_set_blank_callback(pet_blank_callback);
    pet_misc_widget_set_screen2001_callback(pet_screen2001_callback);
    gtk_widget_set_margin_top(pet_misc_widget, 16);
    gtk_grid_attach(GTK_GRID(pet_grid), pet_misc_widget, 1, 2, 3, 1);

    /* SuperPET widgets */
    superpet_grid = superpet_widget_create();
    pet_superpet_widget_set_superpet_enable_callback(superpet_enable_callback);

    stack = gtk_stack_new();

    gtk_stack_add_titled(GTK_STACK(stack), pet_grid, "PET", "PET");
    gtk_stack_add_titled(GTK_STACK(stack), superpet_grid, "SuperPET", "SuperPET");
    gtk_stack_set_transition_type(GTK_STACK(stack),
                                  GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 500);
    gtk_stack_set_homogeneous(GTK_STACK(stack), TRUE);

    switcher = gtk_stack_switcher_new();
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack));
    gtk_widget_set_halign(switcher, GTK_ALIGN_CENTER);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(switcher),
                                   GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_bottom(switcher, 8);
    gtk_widget_show_all(stack);
    gtk_widget_show_all(switcher);

    gtk_stack_set_visible_child_name(GTK_STACK(stack), "PET");
    gtk_grid_attach(GTK_GRID(grid), switcher, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), stack, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create CBM-II/5x0 model settings widget layout
 *
 * \param[in]   grid    GtkGrid to attach widgets to
 *
 * \return  \a grid
 */
static GtkWidget *create_cbm5x0_layout(GtkWidget *grid)
{
    GtkWidget *switches_widget;
    GtkWidget *bank15_widget;

    /* add machine widget */
    gtk_grid_attach(GTK_GRID(grid), machine_widget, 0, 0, 1, 3);

    /* add video widget */
    video_widget = video_model_widget_create(machine_widget);
    video_model_widget_set_callback(video_widget, cbm5x0_video_callback);
    gtk_grid_attach(GTK_GRID(grid), video_widget, 1, 0, 1, 1);

    /* Hardwired I/O port model switches */
    switches_widget = cbm2_hardwired_switches_widget_create();
    cbm2_hardwired_switches_widget_set_callback(switches_widget,
            cbm2_switches_callback);
    gtk_grid_attach(GTK_GRID(grid), switches_widget, 2, 0, 1, 1);

    /* machine power frequency */
    power_frequency_widget = machine_power_frequency_widget_new();
    machine_power_frequency_widget_add_callback(power_frequency_widget,
                                                power_frequency_callback);
    gtk_grid_attach(GTK_GRID(grid), power_frequency_widget, 2, 1, 2, 1);

    /* SID widget */
    sid_widget = sid_model_widget_create(machine_widget);
    sid_model_widget_set_callback(sid_widget, sid_model_callback);
    gtk_widget_set_margin_top(sid_widget, 16);
    gtk_grid_attach(GTK_GRID(grid), sid_widget, 1, 1, 1, 1);

    /* CIA1 widget */
    cia_widget = cia_model_widget_create(1);
    cia_model_widget_set_callback(cia_widget, cia_model_callback);
    gtk_widget_set_margin_top(cia_widget, 16);
    gtk_grid_attach(GTK_GRID(grid), cia_widget, 1, 2, 2, 1);

    /* RAM size widget */
    ram_widget = cbm2_memory_size_widget_create();
    cbm2_memory_size_widget_set_callback(ram_widget, cbm2_memory_size_callback);
    gtk_widget_set_margin_top(ram_widget, 16);
    gtk_grid_attach(GTK_GRID(grid), ram_widget, 0, 3, 1, 1);

    /* Mapping RAM into bank 15 */
    bank15_widget = cbm2_ram_mapping_widget_create();
    gtk_widget_set_margin_top(bank15_widget, 16);
    gtk_grid_attach(GTK_GRID(grid), bank15_widget, 1, 3, 2, 1);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create CBM-II/6x0-7x0 model settings widget layout
 *
 * \param[in]   grid    GtkGrid to attach widgets to
 *
 * \return  \a grid
 */
static GtkWidget *create_cbm6x0_layout(GtkWidget *grid)
{
    GtkWidget *switches_widget;
    GtkWidget *bank15_widget;

    /* add machine widget */
    gtk_grid_attach(GTK_GRID(grid), machine_widget, 0, 0, 1, 3);

    /* add video widget */
    video_widget = video_model_widget_create(machine_widget);
    video_model_widget_set_callback(video_widget, cbm2_video_callback);
    gtk_grid_attach(GTK_GRID(grid), video_widget, 1, 0, 1, 1);

    /* Hardwired I/O port model switches */
    switches_widget = cbm2_hardwired_switches_widget_create();
    cbm2_hardwired_switches_widget_set_callback(switches_widget,
                                                cbm2_switches_callback);
    gtk_grid_attach(GTK_GRID(grid), switches_widget, 2, 0, 1, 1);

    /* machine power frequency */
    power_frequency_widget = machine_power_frequency_widget_new();
    machine_power_frequency_widget_add_callback(power_frequency_widget,
                                                power_frequency_callback);
    gtk_grid_attach(GTK_GRID(grid), power_frequency_widget, 2, 1, 2, 1);

    /* SID widget */
    sid_widget = sid_model_widget_create(machine_widget);
    sid_model_widget_set_callback(sid_widget, sid_model_callback);
    gtk_grid_attach(GTK_GRID(grid), sid_widget, 1, 1, 1, 1);

    /* CIA1 widget */
    cia_widget = cia_model_widget_create(1);
    cia_model_widget_set_callback(cia_widget, cia_model_callback);
    gtk_grid_attach(GTK_GRID(grid), cia_widget, 1, 2, 2, 1);

    /* RAM size widget */
    ram_widget = cbm2_memory_size_widget_create();
    cbm2_memory_size_widget_set_callback(ram_widget, cbm2_memory_size_callback);
    gtk_widget_set_margin_top(ram_widget, 16);
    gtk_grid_attach(GTK_GRID(grid), ram_widget, 0, 3, 1, 1);

    /* Mapping RAM into bank 15 */
    bank15_widget = cbm2_ram_mapping_widget_create();
    gtk_widget_set_margin_top(bank15_widget, 16);
    gtk_grid_attach(GTK_GRID(grid), bank15_widget, 1, 3, 2, 1);

    gtk_widget_show_all(grid);
    return grid;
}

/** \brief  Create VSID layout
 *
 * \param[in,out]   grid    GtkGrid to use for layout
 *
 * \return  \a grid
 */
static GtkWidget *create_vsid_layout(GtkWidget *grid)
{
    /* VIC-II model widget */
    video_widget = video_model_widget_create(machine_widget);
    gtk_grid_attach(GTK_GRID(grid), video_widget, 1, 0, 1, 1);

    sid_widget = sid_model_widget_create(machine_widget);
    /*sid_model_widget_set_callback(sid_widget, sid_model_callback);*/
    gtk_grid_attach(GTK_GRID(grid), sid_widget, 0, 0, 1, 1);
    return grid;
}

/** \brief  Create machine-specific layout
 *
 * Creates a machine-specific layout, including creating the required widgets
 *
 * \return  GtkGrid
 */
static GtkWidget *create_layout(void)
{
    GtkWidget *grid = gtk_grid_new();

    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 0);

    /* Can't add the machine model widget here, since it differs a lot in size
     * depending on the machine, which may result in odd layouts. So we need
     * to determine per machine how many grid rows the machine model widget
     * needs to occupy */

    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:
            return create_c64_layout(grid);
        case VICE_MACHINE_C64DTV:
            return create_c64dtv_layout(grid);
        case VICE_MACHINE_C128:
            return create_c128_layout(grid);
        case VICE_MACHINE_VIC20:
            return create_vic20_layout(grid);
        case VICE_MACHINE_PLUS4:
            return create_plus4_layout(grid);
        case VICE_MACHINE_PET:
            return create_pet_layout(grid);
        case VICE_MACHINE_CBM5x0:
            return create_cbm5x0_layout(grid);
        case VICE_MACHINE_CBM6x0:
            return create_cbm6x0_layout(grid);
        case VICE_MACHINE_VSID:
            return create_vsid_layout(grid);
        default:
            /* shouldn't get here */
            fprintf(stderr, "Aargs! machine %d does not exist!", machine_class);
            archdep_vice_exit(1);
            return NULL;    /* unlike with ordinary exit(), GCC doesn't see
                               this will never be reached */
    }
}


/** \brief  Create 'Model' widget for the settings UI
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 */
GtkWidget *settings_model_widget_create(GtkWidget *parent)
{
    GtkWidget *layout;

    cia_widget    = NULL;
    kernal_widget = NULL;
    sid_widget    = NULL;
    vdc_widget    = NULL;
    video_widget  = NULL;

    /* every machine has a machine model widget */
    machine_widget = machine_model_widget_create();

    /* create machine-specific layout */
    layout = create_layout();

    /* connect signal handlers */
    machine_model_widget_connect_signals(machine_widget);
    if ((machine_class != VICE_MACHINE_PET)) {
        /*
         * PET only has a simple CRTC, so no video widget used
         * (Is this still valid? I had to remove the cbm2 from this branch)
         */
        video_model_widget_connect_signals(video_widget);
    }

    /* add callback */
    machine_model_widget_set_callback(machine_model_callback);

    /* get current video standard */
    resources_get_int("MachineVideoStandard", &mvs_current);

    gtk_widget_show_all(layout);
    return layout;
}


/** \brief  Set function pointer to function that determines if the model
 *          settings indicate a valid model
 *
 * \param[in]   func    function pointer
 */
void settings_model_widget_set_model_func(int (*func)(void))
{
    get_model_func = func;
}


/** \brief  Set function to get a memory hack description
 *
 * \param[in]   func    function to get a string for a memhack ID (int)
 */
void settings_model_widget_set_memhack_func(const char *(*func)(int))
{
    get_memhack_func = func;
}
