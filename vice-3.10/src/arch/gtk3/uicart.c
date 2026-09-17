/** \file   uicart.c
 * \brief   Widget to attach carts
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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
#include <gtk/gtk.h>
#include <string.h>

#include "machine.h"
#include "resources.h"
#include "debug_gtk3.h"
#include "basewidgets.h"
#include "widgethelpers.h"
#include "basedialogs.h"
#include "filechooserhelpers.h"
#include "lastdir.h"
#include "log.h"
#include "openfiledialog.h"
#include "savefiledialog.h"
#include "cartridge.h"
#include "vsync.h"
#include "ui.h"
#include "uiactions.h"
#include "uimachinewindow.h"
#include "crtpreviewwidget.h"

#include "uicart.h"


/** \brief  Enum with various cart types, independent from cartridge.h
 *
 * The define's in cartridge.h don't provide the values I need, so this will
 * have to do.
 */
typedef enum ui_cart_type_e {

    /* C64 cart types */
    UICART_C64_SMART = 0,
    UICART_C64_GENERIC,
    UICART_C64_FREEZER,
    UICART_C64_GAME,
    UICART_C64_UTIL,

    /* C128 cart types */
    UICART_C128_FUNCROM,

    /* VIC20 cart types */
    UICART_VIC20_SMART,
    UICART_VIC20_GENERIC,
    /* UICART_VIC20_ADD_GENERIC, */
    UICART_VIC20_FREEZER,
    UICART_VIC20_GAME,
    UICART_VIC20_UTIL,

    /* Plus4 cart types */
    UICART_PLUS4_SMART,
    UICART_PLUS4_GENERIC,
    UICART_PLUS4_FREEZER,
    UICART_PLUS4_GAME,
    UICART_PLUS4_UTIL,

    /* CBM2 cart types */
    UICART_CBM2_SMART,
    UICART_CBM2_GENERIC,
    UICART_CBM2_FREEZER,
    UICART_CBM2_GAME,
    UICART_CBM2_UTIL

} ui_cart_type_t;


/** \brief  Indici of filename patterns
 */
enum {
    UICART_PATTERN_CRT = 0, /* '*.crt' */
    /*UICART_PATTERN_BIN,*/     /* '*.bin' */
    UICART_PATTERN_BIN_PRG, /* '*.bin;*.prg' */
    UICART_PATTERN_ALL      /* '*' */
};


/** \brief  Simple (text,id) data structure for the cart type model
 */
typedef struct cart_type_list_s {
    const char *name;   /**< cartridge name */
    int id;             /**< cartridge id */
} cart_type_list_t;


/** \brief  Available C64 cart types
 *
 * When the 'type' is freezer, games or utilities, a second combo box will
 * be populated with cartridges which fall in that category.
 */
static const cart_type_list_t c64_cart_types[] = {
    { "Smart-attach",   UICART_C64_SMART },
    { "Generic",        UICART_C64_GENERIC },
    { "Freezer",        UICART_C64_FREEZER },
    { "Games",          UICART_C64_GAME },
    { "Utilities",      UICART_C64_UTIL },
    { NULL,             -1 }
};

/** \brief  Available C128 cart types
 *
 * When the 'type' is freezer, games or utilities, a second combo box will
 * be populated with cartridges which fall in that category.
 */
static const cart_type_list_t c128_cart_types[] = {
    { "Smart-attach",   UICART_C64_SMART },
    { "Generic",        UICART_C64_GENERIC },
    { "Freezer",        UICART_C64_FREEZER },
    { "Games",          UICART_C64_GAME },
    { "Utilities",      UICART_C64_UTIL },
    { NULL,             -1 }
};


/** \brief  List of VIC-20 'main' cart types
 */
static const cart_type_list_t vic20_cart_types[] = {
    { "Smart-attach",               UICART_VIC20_SMART },
    { "Generic",                    UICART_VIC20_GENERIC },
    { "Freezer",                    UICART_VIC20_FREEZER },
    { "Games",                      UICART_VIC20_GAME },
    { "Utilities",                  UICART_VIC20_UTIL },
    { NULL, -1 }
};


/** \brief  List of Plus4/C16/C116 cart types
 */
static const cart_type_list_t plus4_cart_types[] = {
    { "Smart-attach",               UICART_PLUS4_SMART },
    { "Generic",                    UICART_PLUS4_GENERIC },
    { "Freezer",                    UICART_PLUS4_FREEZER },
    { "Games",                      UICART_PLUS4_GAME },
    { "Utilities",                  UICART_PLUS4_UTIL },
    { NULL, -1 }
};


/** \brief  List of CBM-II cart types
 */
static const cart_type_list_t cbm2_cart_types[] = {
    { "Smart-attach",               UICART_CBM2_SMART },
    { "Generic",                    UICART_CBM2_GENERIC },
    { "Freezer",                    UICART_CBM2_FREEZER },
    { "Games",                      UICART_CBM2_GAME },
    { "Utilities",                  UICART_CBM2_UTIL },
    { NULL, -1 }
};


/** \brief  File filter pattern for CRT images */
static const char *pattern_crt[] = { "*.crt", NULL };


/** \brief  File filter pattern for raw images */
/*static const char *pattern_bin[] = { "*.bin", NULL };*/

/** \brief  File filter pattern for raw images */
static const char *pattern_bin_prg[] = { "*.bin", "*.prg", NULL };

/* FIXME: for vic20 we should add .vrt(?), for plus4 .prt(?) */

/** \brief  File type filters for the dialog
 */
static ui_file_filter_t filters[] = {
    { "CRT images", pattern_crt },
    /*{ "Raw images", pattern_bin },*/
    { "Raw images", pattern_bin_prg },
    { "All files", file_chooser_pattern_all },
    { NULL, NULL }
};


/** \brief  Last used directory
 */
static gchar *last_dir = NULL;

/** \brief  Last used filename
 */
static gchar *last_file = NULL;


/* References to widgets used in various event handlers */

/** \brief  Reference to the cart dialog */
static GtkWidget *cart_dialog = NULL;


/** \brief  Reference to the cart dialog */
static GtkWidget *cart_type_widget = NULL;


/** \brief  Reference to the cart-id widget */
static GtkWidget *cart_id_widget = NULL;

/** \brief  Reference to the cart content 'preview' widget
 *
 * This widget shows the contents of the cart selected in the dialog.
 */
static GtkWidget *cart_preview_widget = NULL;


/** \brief  Reference to the cart-set-default widget */
static GtkWidget *cart_set_default_widget = NULL;

/** \brief  Reference to the cart add widget */
static GtkWidget *cart_add_widget = NULL;


/** \brief  Reference to the cart ID widget */
static GtkWidget *cart_id_label = NULL;

/** \brief  Reference to the dialog file filter showing only .crt images */
 GtkFileFilter *flt_crt = NULL;


/** \brief  Reference to the dialog file filter showing only .bin images */
/*static GtkFileFilter *flt_bin = NULL;*/

/** \brief  Reference to the dialog file filter showing .bin + .prg images */
static GtkFileFilter *flt_bin_prg = NULL;

/** \brief  Reference to the dialog file filter showing all files */
static GtkFileFilter *flt_all = NULL;


/* forward declarations of functions */
static GtkListStore *create_cart_id_model(unsigned int flags);
static int get_cart_type(void);
static int get_cart_id(void);
static int attach_cart_image(int type, int id, const char *path);

/** \brief  Optional extra callback
 *
 * This function is triggered when a cartidge is attached.
 */
static void (*extra_attach_callback)(void) = NULL;


/** \brief  Handler for the 'destroy' event of the dialog
 *
 * \param[in]   dialog  dialog
 * \param[in]   data    extra event data (unused)
 */
static void on_destroy(GtkWidget *dialog, gpointer data)
{
    ui_action_finish(ACTION_CART_ATTACH);
}


/** \brief  Handler for the "response" event of the dialog
 *
 * \param[in]   dialog      dialog
 * \param[in]   response_id response ID
 * \param[in]   data        extra event data (unused)
 */
static void on_response(GtkWidget *dialog, gint response_id, gpointer data)
{
    gchar *filename;

    switch (response_id) {
        case GTK_RESPONSE_DELETE_EVENT:
            gtk_widget_destroy(dialog);
            break;
        case GTK_RESPONSE_ACCEPT:
            lastdir_update(dialog, &last_dir, &last_file);
            filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            if (filename != NULL) {
                gchar *filename_locale = file_chooser_convert_to_locale(filename);
                gboolean result;

                result = attach_cart_image(get_cart_type(), get_cart_id(), filename_locale);
                if (!result) {
                    vice_gtk3_message_error(GTK_WINDOW(dialog),
                                            "VICE Error",
                                            "Failed to attach image '%s'",
                                            filename);
                    /* we don't destroy the dialog here to allow the user to
                     * select another (hopefully valid) image. */
                } else {
                    /* call optional extra callback */
                    if (extra_attach_callback != NULL) {
                        extra_attach_callback();
                    }
                    gtk_widget_destroy(dialog);
                }
                g_free(filename);
                g_free(filename_locale);
            }
            break;
    }

    /* FIXME: hack to avoid the 'normal' dialog (via the menu) triggering the
     *        extra callback meant for the default cart setting page.
     *
     *      Didn't I fix this? (cpx-2021-03-14)
     */
    extra_attach_callback = NULL;
}


/** \brief  Set the file filter pattern for the dialog
 *
 * \param[in]   pattern UICART_PATTERN_\* enum value
 */
static void set_pattern(int pattern)
{
    GtkFileFilter *filter = NULL;

    switch (pattern) {
        case UICART_PATTERN_CRT:
            filter = flt_crt;
            break;
        case UICART_PATTERN_BIN_PRG:
            filter = flt_bin_prg;
            break;
        default:
            filter = flt_all;
            break;
    }
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(cart_dialog), filter);
}


/** \brief  Handler for the "changed" event of the cart type combo box
 *
 * \param[in]   combo   cart type combo
 * \param[in]   data    extra event data (unused)
 */
static void on_cart_type_changed(GtkComboBox *combo, gpointer data)
{
    GtkListStore *id_model;     /* cart 'ID' model */
    unsigned int mask = ~0;
    int pattern = UICART_PATTERN_BIN_PRG;
    int crt_type;

    crt_type = get_cart_type();
    if (crt_type < 0) {
        return;
    }

    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_PLUS4:    /* fall through */
        case VICE_MACHINE_CBM5x0:   /* fall through */
        case VICE_MACHINE_CBM6x0:   /* fall through */
        case VICE_MACHINE_VIC20:
            switch (crt_type) {
                case UICART_C64_SMART:      /* fall through */
                case UICART_PLUS4_SMART:    /* fall through */
                case UICART_VIC20_SMART:    /* fall through */
                case UICART_CBM2_SMART:     /* fall through */
                    pattern = UICART_PATTERN_CRT;
                    break;
                case UICART_C64_GENERIC:    /* fall through */
                case UICART_PLUS4_GENERIC:  /* fall through */
                case UICART_VIC20_GENERIC:  /* fall through */
                case UICART_CBM2_GENERIC:   /* fall through */
                    mask = CARTRIDGE_GROUP_GENERIC;
                    break;
                case UICART_C64_FREEZER:    /* fall through */
                case UICART_PLUS4_FREEZER:  /* fall through */
                case UICART_VIC20_FREEZER:  /* fall through */
                case UICART_CBM2_FREEZER:   /* fall through */
                    mask = CARTRIDGE_GROUP_FREEZER;
                    break;
                case UICART_C64_GAME:       /* fall through */
                case UICART_PLUS4_GAME:     /* fall through */
                case UICART_VIC20_GAME:     /* fall through */
                case UICART_CBM2_GAME:      /* fall through */
                    mask = CARTRIDGE_GROUP_GAME;
                    break;
                case UICART_C64_UTIL:       /* fall through */
                case UICART_PLUS4_UTIL:     /* fall through */
                case UICART_VIC20_UTIL:     /* fall through */
                case UICART_CBM2_UTIL:      /* fall through */
                    mask = CARTRIDGE_GROUP_UTIL;
                    break;
                default:
                    mask = 0x0;
                    break;
            }

            /* update cart ID model and set it */
            id_model = create_cart_id_model(mask);
            gtk_combo_box_set_model(GTK_COMBO_BOX(cart_id_widget), GTK_TREE_MODEL(id_model));
            gtk_combo_box_set_active(GTK_COMBO_BOX(cart_id_widget), 0);
            /* only show cart ID list when needed */
            if ((pattern == UICART_PATTERN_CRT) || (mask == 0x0)) {
                gtk_widget_hide(GTK_WIDGET(cart_id_widget));
                gtk_widget_hide(GTK_WIDGET(cart_id_label));
                gtk_widget_hide(GTK_WIDGET(cart_add_widget));
            } else {
                gtk_widget_show(GTK_WIDGET(cart_id_widget));
                gtk_widget_show(GTK_WIDGET(cart_id_label));
            }

            /* update filename pattern */
            set_pattern(pattern);

            break;

        default:
            break;
    }
}


/** \brief  Get the ID of the model for the 'cart type' combo box
 *
 * \return  ID or -1 on error
 */
static int get_cart_type(void)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkComboBox *combo;
    int crt_type = -1;

    combo = GTK_COMBO_BOX(cart_type_widget);

    if (gtk_combo_box_get_active(combo) >= 0) {
        model = gtk_combo_box_get_model(combo);
        if (gtk_combo_box_get_active_iter(combo, &iter)) {
            gtk_tree_model_get(model, &iter, 1, &crt_type, -1);
        }
    }
    return crt_type;
}


/** \brief  Handler for the "changed" event of the cart ID combo box
 *
 * \param[in]   combo   cart ID combo
 * \param[in]   data    extra event data (unused)
 */
static void on_cart_id_changed(GtkComboBox *combo, gpointer data)
{
    int lasttype = cartridge_get_id(0);
    int id = get_cart_id();
    int generic_add = 0;

    /* enable the "add to generic cartridge" checkbox only when a generic
       cartridge is selected AND currently inserted! */
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_PLUS4:    /* fall through */
        case VICE_MACHINE_CBM5x0:   /* fall through */
        case VICE_MACHINE_CBM6x0:
            break;
        /* currently we only use this on vic20 */
        case VICE_MACHINE_VIC20:
            if (id >= CARTRIDGE_VIC20_DETECT) {
                if (lasttype == CARTRIDGE_VIC20_GENERIC) {
                    generic_add = 1;
                }
            }
            break;
    }

    if (generic_add) {
        gtk_widget_show(cart_add_widget);
    } else {
        gtk_widget_hide(cart_add_widget);
    }
}


/** \brief  Get the ID of the model for the 'cart ID' combo box
 *
 * \return  ID or -1 on error
 */
static int get_cart_id(void)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkComboBox *combo;
    int crt_id = -1;

    if (cart_id_widget == NULL) {
        return crt_id;
    }

    combo = GTK_COMBO_BOX(cart_id_widget);
    if (gtk_combo_box_get_active(combo) >= 0) {
        model = gtk_combo_box_get_model(combo);
        if (gtk_combo_box_get_active_iter(combo, &iter)) {
            gtk_tree_model_get(model, &iter, 1, &crt_id, -1);
        }
    }
    return crt_id;
}


/** \brief  Cart attach handler
 *
 * \param[in]   type    cartridge type
 * \param[in]   id      cartridge ID
 * \param[in]   path    path to cartrige image
 *
 * \return  bool
 */
static int attach_cart_image(int type, int id, const char *path)
{
    /* printf("attach_cart_image type: %d id: %d path: %s\n", type, id, path); */
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_SCPU64:
            switch (type) {
                case UICART_C64_SMART:
                    id = CARTRIDGE_CRT;
                    break;
                case UICART_C128_FUNCROM:
                    id = CARTRIDGE_C128_MAKEID(CARTRIDGE_C128_GENERIC);
                    break;
                case UICART_C64_FREEZER:    /* fall through */
                case UICART_C64_GAME:       /* fall through */
                case UICART_C64_UTIL:
                    /* id is correct */
                    break;
                default:
                    debug_gtk3("error: shouldn't get here.");
                    break;
            }
            break;

        case VICE_MACHINE_VIC20:
            switch (type) {
                case UICART_VIC20_SMART:
                    /* id = CARTRIDGE_VIC20_DETECT; */
                    id = CARTRIDGE_CRT;
                    break;
                case UICART_VIC20_GENERIC:
                    /* we also want to select an id for generic type. some day
                       we need to fix "generic" vs "add to generic" */
                    /* id = CARTRIDGE_VIC20_GENERIC; */
                    /* id is correct */
#if 0
                case UICART_VIC20_ADD_GENERIC:
                    /* id is correct */
                    break;
#endif
#if 0
                case UICART_VIC20_BEHRBONZ:
                    id = CARTRIDGE_VIC20_BEHRBONZ;
                    break;
               case UICART_VIC20_MEGACART:
                    id = CARTRIDGE_VIC20_MEGACART;
                    break;
               case UICART_VIC20_FINALEXP:
                    id = CARTRIDGE_VIC20_FINAL_EXPANSION;
                    break;
               case UICART_VIC20_ULTIMEM:
                    id = CARTRIDGE_VIC20_UM;
                    break;
               case UICART_VIC20_FLASHPLUGIN:
                    id = CARTRIDGE_VIC20_FP;
                    break;
#endif
                case UICART_VIC20_FREEZER:    /* fall through */
                case UICART_VIC20_GAME:       /* fall through */
                case UICART_VIC20_UTIL:
                    /* id is correct */
                    break;
                default:
                    debug_gtk3("error: shouldn't get here.");
                    break;
            }
            break;

        case VICE_MACHINE_PLUS4:
            switch (type) {
                case UICART_PLUS4_SMART:
                    id = CARTRIDGE_CRT;
                    break;
#if 0
                case UICART_PLUS4_16KB_C1LO:
                    id = CARTRIDGE_PLUS4_GENERIC_C1LO;
                    break;
                case UICART_PLUS4_16KB_C1HI:
                    id = CARTRIDGE_PLUS4_GENERIC_C1HI;
                    break;
                case UICART_PLUS4_16KB_C2LO:
                    id = CARTRIDGE_PLUS4_GENERIC_C2LO;
                    break;
                case UICART_PLUS4_16KB_C2HI:
                    id = CARTRIDGE_PLUS4_GENERIC_C2HI;
                    break;
                case UICART_PLUS4_32KB_C1:
                    id = CARTRIDGE_PLUS4_GENERIC_C1;
                    break;
                case UICART_PLUS4_32KB_C2:
                    id = CARTRIDGE_PLUS4_GENERIC_C2;
                    break;
#endif
                case UICART_PLUS4_FREEZER:    /* fall through */
                case UICART_PLUS4_GAME:       /* fall through */
                case UICART_PLUS4_UTIL:
                    /* id is correct */
                    break;
                default:
                    /* oops */
                    debug_gtk3("error: shouldn't get here.");
                    break;
            }
            break;

        case VICE_MACHINE_CBM5x0:   /* fall through */
        case VICE_MACHINE_CBM6x0:
            switch (type) {
                case UICART_CBM2_SMART:
                    /*return (crt_attach_func(CARTRIDGE_CBM2_DETECT, path) == 0);*/
                    id = CARTRIDGE_CRT;
                    break;
#if 0
                case UICART_CBM2_4KB_1000:
                    id = CARTRIDGE_CBM2_GENERIC_C1;
                    break;
                case UICART_CBM2_8KB_2000:
                    id = CARTRIDGE_CBM2_GENERIC_C2;
                    break;
                case UICART_CBM2_8KB_4000:
                    id = CARTRIDGE_CBM2_GENERIC_C4;
                    break;
                case UICART_CBM2_8KB_6000:
                    id = CARTRIDGE_CBM2_GENERIC_C6;
                    break;
#endif
                case UICART_CBM2_FREEZER:    /* fall through */
                case UICART_CBM2_GAME:       /* fall through */
                case UICART_CBM2_UTIL:
                    /* id is correct */
                    break;
                default:
                    /* oops */
                    debug_gtk3("error: shouldn't get here.");
                    break;
            }
            break;

        default:
            debug_gtk3("very oops: type = %d, id = %d, path = '%s'.",
                    type, id, path);
            return 0;
            break;
    }

    /* add to cartridge */
    if ((cart_add_widget != NULL) &&
            (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cart_add_widget)))) {
        if ((cartridge_attach_add_image(id, path) == 0)) {
            return 1;
        }
    }

    /* printf("call cartridge_attach_image(id:%d path:%s)\n", id, path); */
    if ((cartridge_attach_image(id, path) == 0)) {
        /* check 'set default' */
        if ((cart_set_default_widget != NULL) &&
                (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cart_set_default_widget)))) {
            /* set cart as default, there's no return value, so let's assume
             * this works */
            cartridge_set_default();
        }
        return 1;
    }
    return 0;
}


/** \brief  Create model for the 'cart type' combo box
 *
 * This depends on the `machine_class`, so for some machines, this may return
 * an empty (useless) model
 *
 * \return  model
 */
static GtkListStore *create_cart_type_model(void)
{
    GtkListStore *model;
    GtkTreeIter iter;
    const cart_type_list_t *types;
    int i;

    model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:
            types = c64_cart_types;
            break;
        case VICE_MACHINE_C128:
            types = c128_cart_types;
            break;
        case VICE_MACHINE_VIC20:
            types = vic20_cart_types;
            break;
        case VICE_MACHINE_PLUS4:
            types = plus4_cart_types;
            break;
        case VICE_MACHINE_CBM5x0:
        case VICE_MACHINE_CBM6x0:
            types = cbm2_cart_types;
            break;
        default:
            return model;
    }

    for (i = 0; types[i].name != NULL; i++) {
        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model, &iter, 0, types[i].name, 1, types[i].id, -1);
    }
    return model;
}


/** \brief  Create a list of cartridges, filtered with \a flags
 *
 * Only valid for c64/c128/scpu
 *
 * \param[in]   flags   flags determining what cart types to use
 *
 * \return  Three-column list store (name, crtid, flags)
 */
static GtkListStore *create_cart_id_model(unsigned int flags)
{
    GtkListStore *model;
    GtkTreeIter iter;
    cartridge_info_t *list;
    int i;

    model = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_UINT);
    list = cartridge_get_info_list();
    if (list == NULL) {
        return model;
    }
    for (i = 0; list[i].name != NULL; i++) {
        if (list[i].flags & flags) {
            gtk_list_store_append(model, &iter);
            gtk_list_store_set(model, &iter,
                    0, list[i].name,        /* cart name */
                    1, list[i].crtid,       /* cart ID */
                    2, list[i].flags,       /* cart flags */
                    -1);
        }
    }
    return model;
}


/** \brief  Create combo box with main cartridge types
 *
 * \return  GtkComboBox
 */
static GtkWidget *create_cart_type_combo_box(void)
{
    GtkWidget *combo;
    GtkListStore *model;
    GtkCellRenderer *renderer;

    model = create_cart_type_model();
    if (model == NULL) {
        return gtk_combo_box_new();
    }
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
    g_object_unref(model);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer,
            "text", 0, NULL);

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

    g_signal_connect_unlocked(G_OBJECT(combo),
                              "changed",
                              G_CALLBACK(on_cart_type_changed),
                              NULL);
    return combo;
}


/** \brief  Create combo box with cartridges with adhere to \a mask
 *
 * \param[in]   mask    bitmask to filter cartridges
 *
 * \return  GtkComboBox
 *
 * \note    Only for x64/x64sc/xscp64/x128/plus4
 */
static GtkWidget *create_cart_id_combo_box(unsigned int mask)
{
    GtkWidget *combo;
    GtkListStore *model;
    GtkCellRenderer *renderer;

    model = create_cart_id_model(mask);
    if (model == NULL) {
        return gtk_combo_box_new();
    }
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
    g_object_unref(model);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer,
            "text", 0, NULL);

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

    g_signal_connect_unlocked(G_OBJECT(combo),
                              "changed",
                              G_CALLBACK(on_cart_id_changed),
                              NULL);
    return combo;
}


/** \brief  Create the 'extra' widget for the dialog
 *
 * \param[in]   set_default initial state of the 'set as default' checkbox
 *
 * \return  GtkGrid
 */
static GtkWidget *create_extra_widget(gboolean set_default)
{
    GtkWidget *grid;
    GtkWidget *label;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    label = gtk_label_new("cartridge type");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    cart_type_widget = create_cart_type_combo_box();
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cart_type_widget, 1, 0, 1, 1);

    /* create "Set cartridge as default" check button */
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_PLUS4:    /* fall through */
        case VICE_MACHINE_CBM5x0:   /* fall through */
        case VICE_MACHINE_CBM6x0:   /* fall through */
        case VICE_MACHINE_VIC20:
            cart_set_default_widget = gtk_check_button_new_with_label(
                    "Set cartridge as default");
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(cart_set_default_widget), set_default);

            gtk_grid_attach(GTK_GRID(grid), cart_set_default_widget, 0, 1, 4, 1);
            break;
        default:
            /* Set cart as default is not supported for the current machine */
            break;
    }

    /* create "cartridge ID" combo box */
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_PLUS4:    /* fall through */
        case VICE_MACHINE_CBM5x0:   /* fall through */
        case VICE_MACHINE_CBM6x0:   /* fall through */
        case VICE_MACHINE_VIC20:

            cart_id_label = gtk_label_new("cartridge ID");
            gtk_widget_set_halign(cart_id_label, GTK_ALIGN_START);
            cart_id_widget = create_cart_id_combo_box(0x0);
            gtk_grid_attach(GTK_GRID(grid), cart_id_label, 2, 0, 1, 1);
            gtk_grid_attach(GTK_GRID(grid), cart_id_widget, 3, 0, 1, 1);
            break;
#if 0
        case VICE_MACHINE_VIC20:
            cart_id_label = gtk_label_new("cartridge class");
            gtk_widget_set_halign(cart_id_label, GTK_ALIGN_START);
            cart_id_widget = create_cart_id_combo_box_vic20();
            gtk_grid_attach(GTK_GRID(grid), cart_id_label, 2, 0, 1, 1);
            gtk_grid_attach(GTK_GRID(grid), cart_id_widget, 3, 0, 1, 1);
            break;
#endif
        default:
            break;
    }

    /* create "Set cartridge as default" check button */
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_PLUS4:    /* fall through */
        case VICE_MACHINE_CBM5x0:   /* fall through */
        case VICE_MACHINE_CBM6x0:   /* fall through */
        case VICE_MACHINE_VIC20:
            cart_add_widget = gtk_check_button_new_with_label(
                    "add to cartridge");
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(cart_add_widget), 0);

            gtk_grid_attach(GTK_GRID(grid), cart_add_widget, 4, 0, 1, 1);
            break;
        default:
            /* Set cart as default is not supported for the current machine */
            break;
    }

    gtk_widget_show_all(grid);
    gtk_widget_hide(cart_add_widget);
    return grid;
}

#if 0
/** \brief  Create the 'preview' widget for the dialog
 *
 * \return  GtkGrid
 */
static GtkWidget *create_preview_widget(void)
{
    if ((machine_class != VICE_MACHINE_C64)
            && (machine_class != VICE_MACHINE_C64SC)
            && (machine_class != VICE_MACHINE_SCPU64)
            && (machine_class != VICE_MACHINE_C128)
            && (machine_class != VICE_MACHINE_CBM5x0)
            && (machine_class != VICE_MACHINE_CBM6x0)
            && (machine_class != VICE_MACHINE_PLUS4)
            && (machine_class != VICE_MACHINE_VIC20)) {
        GtkWidget *grid = NULL;
        GtkWidget *label;
        grid = gtk_grid_new();
        gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
        gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

        label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), "<b>Cartridge info</b>");
        gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

        label = gtk_label_new("Error: groepaz was here!");
        gtk_widget_set_margin_start(label, 16);
        gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);

        gtk_widget_show_all(grid);
        return grid;
    } else {
        return crt_preview_widget_create();
    }

}
#endif

/** \brief  Update the 'preview' widget for the dialog
 *
 * \param[in,out]   file_chooser    GtkFileChooser instance
 * \param[in]       data            extra event data (unused)
 *
 * \return  GtkGrid
 */
static void update_preview(GtkFileChooser *file_chooser, gpointer data)
{
    gchar *path = NULL;

    path = gtk_file_chooser_get_filename(file_chooser);
    if (path != NULL) {
        gchar *path_locale = file_chooser_convert_to_locale(path);
        crt_preview_widget_update(path_locale);
        g_free(path);
        g_free(path_locale);
    }
}


/** \brief  Create attach dialog
 *
 * \param[in]   set_as_default  initial state of the 'set as default' checkbox
 * \param[in]   callback        extra callback when attach succeeds
 *
 * \return  GtkDialog
 */
static GtkWidget *cart_dialog_internal(gboolean set_as_default,
                                       void (*callback)(void))
{
    GtkWidget *dialog;

    dialog = gtk_file_chooser_dialog_new(
            "Attach a cartridge image",
            ui_get_active_window(),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            /* buttons */
            "Attach", GTK_RESPONSE_ACCEPT,
            "Close", GTK_RESPONSE_DELETE_EVENT,
            NULL, NULL);

    /* set modal so mouse-grab doesn't get triggered */
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    /* set last directory */
    lastdir_set(dialog, &last_dir, &last_file);

    /* add extra widget */
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog),
            create_extra_widget(set_as_default));

    /* add preview widget */
    cart_preview_widget = crt_preview_widget_create();
    gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(dialog),
            cart_preview_widget);
    gtk_file_chooser_set_use_preview_label(GTK_FILE_CHOOSER(dialog), FALSE);

    /* add filters */
    flt_crt = create_file_chooser_filter(filters[UICART_PATTERN_CRT], FALSE);
    /*flt_bin = create_file_chooser_filter(filters[UICART_PATTERN_BIN], FALSE);*/
    flt_bin_prg = create_file_chooser_filter(filters[UICART_PATTERN_BIN_PRG], FALSE);
    flt_all = create_file_chooser_filter(filters[UICART_PATTERN_ALL], TRUE);

    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_CBM5x0:   /* fall through */
        case VICE_MACHINE_CBM6x0:   /* fall through */
        case VICE_MACHINE_PLUS4:
        case VICE_MACHINE_VIC20:
            gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), flt_crt);
            /*gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), flt_bin);*/
            gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), flt_bin_prg);
            gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), flt_all);
            break;
#if 0
        case VICE_MACHINE_VIC20:
            gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), flt_crt);
            gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), flt_bin_prg);
            gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), flt_all);
            break;
#endif
        default:
            break;
    }

    extra_attach_callback = callback;
    cart_dialog           = dialog;

    g_signal_connect(G_OBJECT(dialog),
                     "response",
                     G_CALLBACK(on_response),
                     NULL);
    g_signal_connect_unlocked(G_OBJECT(dialog),
                              "update-preview",
                              G_CALLBACK(update_preview),
                              NULL);
    g_signal_connect_unlocked(G_OBJECT(dialog),
                     "destroy",
                     G_CALLBACK(on_destroy),
                     NULL);

    /* those should be hidden by default */
    if (cart_id_label) {
        gtk_widget_hide(cart_id_label);
    }
    if (cart_id_widget) {
        gtk_widget_hide(cart_id_widget);
    }

    return dialog;
}


/** \brief  Pop up the cart-attach dialog */
void ui_cart_show_dialog(void)
{
    GtkWidget *dialog;

    dialog = cart_dialog_internal(0, NULL);
    gtk_widget_show(dialog);
}


/** \brief  Attach dialog for the settings->default cart page
 *
 * \param[in]   widget      parent widget (unused)
 * \param[in]   callback    function to call on succesfull attach
 *
 */
void ui_cart_default_attach(GtkWidget *widget, void (*callback)(void))
{
    GtkWidget *dialog;

    dialog = cart_dialog_internal(GPOINTER_TO_INT(1), callback);
    gtk_widget_show(dialog);
}


/** \brief  Clean up the last directory string
 */
void ui_cart_shutdown(void)
{
    lastdir_shutdown(&last_dir, &last_file);
}
