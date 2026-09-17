/** \file   hotkeys.c
 * \brief   SDL custom hotkeys handling
 *
 * Provides custom keyboard shortcuts for the SDL UI.
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
 */

#include "vice.h"
#include "vice_sdl.h"
#include <stdbool.h>
#include <stdint.h>

#include "kbd.h"
#include "lib.h"
#include "log.h"
#include "uiapi.h"
#include "uihotkeys.h"
#include "uimenu.h"

#include "hotkeys.h"

/* #define DEBUG_HOTKEYS */


/** \brief  Determine number of elements in array \a arr */
#define ARR_ELEMENTS(arr)   (sizeof arr / sizeof arr[0])

/** \def ALLOWED_SDL_MODS
 * \brief  Allowed SDL modifiers
 *
 * Allowed keyboard modifiers for hotkeys, excluding any Num Lock, Scroll Lock,
 * Caps Lock etc keys. Also only allow left Alt since right Alt can be Alt-Gr.
 */
#ifdef USE_SDL2UI
#define ALLOWED_SDL_MODS    (KMOD_SHIFT|KMOD_CTRL|KMOD_LALT|KMOD_GUI)
#else
#define ALLOWED_SDL_MODS    (KMOD_SHIFT|KMOD_CTRL|KMOD_LALT)
#endif

/** \brief  SDL keycode to VICE keysym mapping
 */
typedef struct keysym_s {
    uint32_t arch_keysym;   /**< SDL keycode */
    uint32_t vice_keysym;   /**< VICE keysym */
} keysym_t;


/** \brief  Mappings of SDL keycodes to VICE keysyms
 */
static const keysym_t keysym_table[]= {
#ifdef USE_SDL2UI
    /* 0-9 */
    { SDLK_0,           VHK_KEY_0 },
    { SDLK_1,           VHK_KEY_1 },
    { SDLK_2,           VHK_KEY_2 },
    { SDLK_3,           VHK_KEY_3 },
    { SDLK_4,           VHK_KEY_4 },
    { SDLK_5,           VHK_KEY_5 },
    { SDLK_6,           VHK_KEY_6 },
    { SDLK_7,           VHK_KEY_7 },
    { SDLK_8,           VHK_KEY_8 },
    { SDLK_9,           VHK_KEY_9 },

    /* a-z, SDL doesn't have A-Z */
    { SDLK_a,           VHK_KEY_a },
    { SDLK_b,           VHK_KEY_b },
    { SDLK_c,           VHK_KEY_c },
    { SDLK_d,           VHK_KEY_d },
    { SDLK_e,           VHK_KEY_e },
    { SDLK_f,           VHK_KEY_f },
    { SDLK_g,           VHK_KEY_g },
    { SDLK_h,           VHK_KEY_h },
    { SDLK_i,           VHK_KEY_i },
    { SDLK_j,           VHK_KEY_j },
    { SDLK_k,           VHK_KEY_k },
    { SDLK_l,           VHK_KEY_l },
    { SDLK_m,           VHK_KEY_m },
    { SDLK_n,           VHK_KEY_n },
    { SDLK_o,           VHK_KEY_o },
    { SDLK_p,           VHK_KEY_p },
    { SDLK_q,           VHK_KEY_q },
    { SDLK_r,           VHK_KEY_r },
    { SDLK_s,           VHK_KEY_s },
    { SDLK_t,           VHK_KEY_t },
    { SDLK_u,           VHK_KEY_u },
    { SDLK_v,           VHK_KEY_v },
    { SDLK_w,           VHK_KEY_w },
    { SDLK_x,           VHK_KEY_x },
    { SDLK_y,           VHK_KEY_y },
    { SDLK_z,           VHK_KEY_z },

    /* Puncuation marks and mathematical operators */
    { SDLK_AMPERSAND,   VHK_KEY_ampersand },
    { SDLK_ASTERISK,    VHK_KEY_asterisk },
    { SDLK_AT,          VHK_KEY_at },
    { SDLK_BACKQUOTE,   VHK_KEY_grave },        /* ` */
    { SDLK_BACKSLASH,   VHK_KEY_backslash },
    { SDLK_CARET,       VHK_KEY_asciicircum },  /* ^ */
    { SDLK_COLON,       VHK_KEY_colon },
    { SDLK_COMMA,       VHK_KEY_comma },
    { SDLK_DOLLAR,      VHK_KEY_dollar },
    { SDLK_EXCLAIM,     VHK_KEY_exclam },
    { SDLK_GREATER,     VHK_KEY_greater },
    { SDLK_HASH,        VHK_KEY_numbersign },   /* # */
    { SDLK_LEFTBRACKET, VHK_KEY_bracketleft },
    { SDLK_LEFTPAREN,   VHK_KEY_parenleft },
    { SDLK_LESS,        VHK_KEY_less },
    { SDLK_MINUS,       VHK_KEY_minus },
    { SDLK_PERCENT,     VHK_KEY_percent },
    { SDLK_PERIOD,      VHK_KEY_period },
    { SDLK_PLUS,        VHK_KEY_plus },
    { SDLK_QUESTION,    VHK_KEY_question },
    { SDLK_QUOTE,       VHK_KEY_apostrophe },   /* ' */
    { SDLK_QUOTEDBL,    VHK_KEY_quotedbl },     /* " */
    { SDLK_RIGHTBRACKET,    VHK_KEY_bracketright },
    { SDLK_RIGHTPAREN,  VHK_KEY_parenright },
    { SDLK_SEMICOLON,   VHK_KEY_semicolon },
    { SDLK_SLASH,       VHK_KEY_slash },
    { SDLK_UNDERSCORE,  VHK_KEY_underscore },

    /* Navigation */
    { SDLK_BACKSPACE,   VHK_KEY_BackSpace },
    { SDLK_CLEAR,       VHK_KEY_Clear },
    { SDLK_DELETE,      VHK_KEY_Delete },
    { SDLK_DOWN,        VHK_KEY_Down },
    { SDLK_END,         VHK_KEY_End },
    { SDLK_ESCAPE,      VHK_KEY_Escape },
    { SDLK_HOME,        VHK_KEY_Home },
    { SDLK_INSERT,      VHK_KEY_Insert },
    { SDLK_LEFT,        VHK_KEY_Left },
    { SDLK_PAGEDOWN,    VHK_KEY_Page_Down },
    { SDLK_PAGEUP,      VHK_KEY_Page_Up },
    { SDLK_PAUSE,       VHK_KEY_Pause },
    { SDLK_PRINTSCREEN, VHK_KEY_Print },
    { SDLK_RETURN,      VHK_KEY_Return },
    { SDLK_RIGHT,       VHK_KEY_Right },
    { SDLK_SCROLLLOCK,  VHK_KEY_Scroll_Lock },
    { SDLK_SYSREQ,      VHK_KEY_Sys_Req },
    { SDLK_TAB,         VHK_KEY_Tab },
    { SDLK_UP,          VHK_KEY_Up },

    /* Keypad */
    { SDLK_KP_0,        VHK_KEY_KP_0 },
    { SDLK_KP_1,        VHK_KEY_KP_1 },
    { SDLK_KP_2,        VHK_KEY_KP_2 },
    { SDLK_KP_3,        VHK_KEY_KP_3 },
    { SDLK_KP_4,        VHK_KEY_KP_4 },
    { SDLK_KP_5,        VHK_KEY_KP_5 },
    { SDLK_KP_6,        VHK_KEY_KP_6 },
    { SDLK_KP_7,        VHK_KEY_KP_7 },
    { SDLK_KP_8,        VHK_KEY_KP_8 },
    { SDLK_KP_9,        VHK_KEY_KP_9 },
    { SDLK_KP_COMMA,    VHK_KEY_KP_Separator },
    { SDLK_KP_DECIMAL,  VHK_KEY_KP_Decimal },
    { SDLK_KP_DIVIDE,   VHK_KEY_KP_Divide },
    { SDLK_KP_ENTER,    VHK_KEY_KP_Enter },
    { SDLK_KP_EQUALS,   VHK_KEY_KP_Equal },
    { SDLK_KP_MINUS,    VHK_KEY_KP_Subtract },
    { SDLK_KP_MULTIPLY, VHK_KEY_KP_Multiply },
    { SDLK_KP_PLUS,     VHK_KEY_KP_Add },
    { SDLK_KP_SPACE,    VHK_KEY_KP_Space },
    { SDLK_KP_TAB,      VHK_KEY_KP_Tab },

    /* Function keys */
    { SDLK_F1,          VHK_KEY_F1 },
    { SDLK_F2,          VHK_KEY_F2 },
    { SDLK_F3,          VHK_KEY_F3 },
    { SDLK_F4,          VHK_KEY_F4 },
    { SDLK_F5,          VHK_KEY_F5 },
    { SDLK_F6,          VHK_KEY_F6 },
    { SDLK_F7,          VHK_KEY_F7 },
    { SDLK_F8,          VHK_KEY_F8 },
    { SDLK_F9,          VHK_KEY_F9 },
    { SDLK_F10,         VHK_KEY_F10 },
    { SDLK_F11,         VHK_KEY_F11 },
    { SDLK_F12,         VHK_KEY_F12 },
    { SDLK_F13,         VHK_KEY_F13 },
    { SDLK_F14,         VHK_KEY_F14 },
    { SDLK_F15,         VHK_KEY_F15 },
    { SDLK_F16,         VHK_KEY_F16 },
    { SDLK_F17,         VHK_KEY_F17 },
    { SDLK_F18,         VHK_KEY_F18 },
    { SDLK_F19,         VHK_KEY_F19 },
    { SDLK_F20,         VHK_KEY_F20 },
#else   /* ifdef USE_SDLUI */

    /* SDL 1.x */

    /* NOTE: some keys are commented out, because the SDL defines do not exist,
       and we don't know the correct raw values */

    /* 0-9 */
    { SDLK_0,           VHK_KEY_0 },
    { SDLK_1,           VHK_KEY_1 },
    { SDLK_2,           VHK_KEY_2 },
    { SDLK_3,           VHK_KEY_3 },
    { SDLK_4,           VHK_KEY_4 },
    { SDLK_5,           VHK_KEY_5 },
    { SDLK_6,           VHK_KEY_6 },
    { SDLK_7,           VHK_KEY_7 },
    { SDLK_8,           VHK_KEY_8 },
    { SDLK_9,           VHK_KEY_9 },

    /* a-z, SDL doesn't have A-Z */
    { SDLK_a,           VHK_KEY_a },
    { SDLK_b,           VHK_KEY_b },
    { SDLK_c,           VHK_KEY_c },
    { SDLK_d,           VHK_KEY_d },
    { SDLK_e,           VHK_KEY_e },
    { SDLK_f,           VHK_KEY_f },
    { SDLK_g,           VHK_KEY_g },
    { SDLK_h,           VHK_KEY_h },
    { SDLK_i,           VHK_KEY_i },
    { SDLK_j,           VHK_KEY_j },
    { SDLK_k,           VHK_KEY_k },
    { SDLK_l,           VHK_KEY_l },
    { SDLK_m,           VHK_KEY_m },
    { SDLK_n,           VHK_KEY_n },
    { SDLK_o,           VHK_KEY_o },
    { SDLK_p,           VHK_KEY_p },
    { SDLK_q,           VHK_KEY_q },
    { SDLK_r,           VHK_KEY_r },
    { SDLK_s,           VHK_KEY_s },
    { SDLK_t,           VHK_KEY_t },
    { SDLK_u,           VHK_KEY_u },
    { SDLK_v,           VHK_KEY_v },
    { SDLK_w,           VHK_KEY_w },
    { SDLK_x,           VHK_KEY_x },
    { SDLK_y,           VHK_KEY_y },
    { SDLK_z,           VHK_KEY_z },

    /* Puncuation marks and mathematical operators */
    { SDLK_AMPERSAND,   VHK_KEY_ampersand },
    { SDLK_ASTERISK,    VHK_KEY_asterisk },
    { SDLK_AT,          VHK_KEY_at },
    { SDLK_BACKQUOTE,   VHK_KEY_grave },        /* ` */
    { SDLK_BACKSLASH,   VHK_KEY_backslash },
    { SDLK_CARET,       VHK_KEY_asciicircum },  /* ^ */
    { SDLK_COLON,       VHK_KEY_colon },
    { SDLK_COMMA,       VHK_KEY_comma },
    { SDLK_DOLLAR,      VHK_KEY_dollar },
    { SDLK_EXCLAIM,     VHK_KEY_exclam },
    { SDLK_GREATER,     VHK_KEY_greater },
    { SDLK_HASH,        VHK_KEY_numbersign },   /* # */
    { SDLK_LEFTBRACKET, VHK_KEY_bracketleft },
    { SDLK_LEFTPAREN,   VHK_KEY_parenleft },
    { SDLK_LESS,        VHK_KEY_less },
    { SDLK_MINUS,       VHK_KEY_minus },
    /* { SDLK_PERCENT,     VHK_KEY_percent }, */
    { SDLK_PERIOD,      VHK_KEY_period },
    { SDLK_PLUS,        VHK_KEY_plus },
    { SDLK_QUESTION,    VHK_KEY_question },
    { SDLK_QUOTE,       VHK_KEY_apostrophe },   /* ' */
    { SDLK_QUOTEDBL,    VHK_KEY_quotedbl },     /* " */
    { SDLK_RIGHTBRACKET,    VHK_KEY_bracketright },
    { SDLK_RIGHTPAREN,  VHK_KEY_parenright },
    { SDLK_SEMICOLON,   VHK_KEY_semicolon },
    { SDLK_SLASH,       VHK_KEY_slash },
    { SDLK_UNDERSCORE,  VHK_KEY_underscore },

    /* Navigation */
    { SDLK_BACKSPACE,   VHK_KEY_BackSpace },
    { SDLK_CLEAR,       VHK_KEY_Clear },
    { SDLK_DELETE,      VHK_KEY_Delete },
    { SDLK_DOWN,        VHK_KEY_Down },
    { SDLK_END,         VHK_KEY_End },
    { SDLK_ESCAPE,      VHK_KEY_Escape },
    { SDLK_HOME,        VHK_KEY_Home },
    { SDLK_INSERT,      VHK_KEY_Insert },
    { SDLK_LEFT,        VHK_KEY_Left },
    { SDLK_PAGEDOWN,    VHK_KEY_Page_Down },
    { SDLK_PAGEUP,      VHK_KEY_Page_Up },
    { SDLK_PAUSE,       VHK_KEY_Pause },
    { 316,              VHK_KEY_Print },
    { SDLK_RETURN,      VHK_KEY_Return },
    { SDLK_RIGHT,       VHK_KEY_Right },
    { 302,              VHK_KEY_Scroll_Lock },
    { SDLK_SYSREQ,      VHK_KEY_Sys_Req },
    { SDLK_TAB,         VHK_KEY_Tab },
    { SDLK_UP,          VHK_KEY_Up },

    /* Keypad */
    { SDLK_KP0,         VHK_KEY_KP_0 },
    { SDLK_KP1,         VHK_KEY_KP_1 },
    { SDLK_KP2,         VHK_KEY_KP_2 },
    { SDLK_KP3,         VHK_KEY_KP_3 },
    { SDLK_KP4,         VHK_KEY_KP_4 },
    { SDLK_KP5,         VHK_KEY_KP_5 },
    { SDLK_KP6,         VHK_KEY_KP_6 },
    { SDLK_KP7,         VHK_KEY_KP_7 },
    { SDLK_KP8,         VHK_KEY_KP_8 },
    { SDLK_KP9,         VHK_KEY_KP_9 },
    /* { SDLK_KP_COMMA,    VHK_KEY_KP_Separator }, */
    /* { SDLK_KP_DECIMAL,  VHK_KEY_KP_Decimal }, */
    { SDLK_KP_DIVIDE,   VHK_KEY_KP_Divide },
    { SDLK_KP_ENTER,    VHK_KEY_KP_Enter },
    { SDLK_KP_EQUALS,   VHK_KEY_KP_Equal },
    { SDLK_KP_MINUS,    VHK_KEY_KP_Subtract },
    { SDLK_KP_MULTIPLY, VHK_KEY_KP_Multiply },
    { SDLK_KP_PLUS,     VHK_KEY_KP_Add },
    /* { SDLK_KP_SPACE,    VHK_KEY_KP_Space }, */
    /* { SDLK_KP_TAB,      VHK_KEY_KP_Tab } */

    /* Function keys */
    { 282,              VHK_KEY_F1 },
    { 283,              VHK_KEY_F2 },
    { 284,              VHK_KEY_F3 },
    { 285,              VHK_KEY_F4 },
    { 286,              VHK_KEY_F5 },
    { 287,              VHK_KEY_F6 },
    { 288,              VHK_KEY_F7 },
    { 289,              VHK_KEY_F8 },
    { 290,              VHK_KEY_F9 },
    { 291,              VHK_KEY_F10 },
    { 292,              VHK_KEY_F11 },
    { 293,              VHK_KEY_F12 },
    { 294,              VHK_KEY_F13 },
    { 295,              VHK_KEY_F14 },
    { 296,              VHK_KEY_F15 },
#endif
};


/******************************************************************************
 *      Hotkeys API virtual method implementations (currently just stubs)     *
 *****************************************************************************/

/** \brief  Translate VICE modifier to SDL modifier
 *
 * \param[in]   vice_mod    VICE modifier
 *
 * \return  SDL modifier
 *
 * \note    The return values are OR'ed SDL modifiers of the left and right
 *          modifier keys, eg for \c VHK_MOD_SHIFT, \c KMOD_SHIFT is returned,
 *          which is \c KMOD_LSHIFT|KMOD_RSHIFT. This is not the case for
 *          \c VHK_MOD_ALT, in which case \c KMOD_LALT is returned since the
 *          right Alt key can be Alt-Gr.
 */
uint32_t ui_hotkeys_arch_modifier_to_arch(uint32_t vice_mod)
{
    switch (vice_mod) {
        case VHK_MOD_ALT:       /* fall through */
        case VHK_MOD_OPTION:
            return KMOD_LALT;
#ifdef USE_SDL2UI
        case VHK_MOD_COMMAND:   /* fall through */
        case VHK_MOD_META:
            return KMOD_GUI;
#endif
        case VHK_MOD_CONTROL:
            return KMOD_CTRL;
        case VHK_MOD_SHIFT:
            return KMOD_SHIFT;
        case VHK_MOD_HYPER:     /* fall through */
        case VHK_MOD_SUPER:     /* fall through */
        default:
            return KMOD_NONE;
    }
}

/** \brief  Translate SDL modifier to VICE modifier
 *
 * \param[in]   arch_mod    SDL modifier
 *
 * \return  VICE modifier
 *
 * \note    The left and right versions of modifier keys are mapped to the same
 *          VICE modifier since VICE (nor Gtk) differentiates between left and
 *          right modifier keys, so for \c KMOD_LSHIFT, \c KMOD_RSHIFT and
 *          \c KMOD_SHIFT (\c KMOD_LSHIFT|KMOD_RSHIFT) this function will return
 *          \c VHK_MOD_SHIFT. This is not the case for Alt, since right Alt can
 *          be Alt-Gr, a different key with a different meaning from a basic
 *          right Alt.
 */
uint32_t ui_hotkeys_arch_modifier_from_arch(uint32_t arch_mod)
{
    switch (arch_mod) {
        case KMOD_LSHIFT:   /* fall through */
        case KMOD_RSHIFT:   /* fall through */
        case KMOD_SHIFT:
            return VHK_MOD_SHIFT;
        case KMOD_LCTRL:    /* fall through */
        case KMOD_RCTRL:    /* fall through */
        case KMOD_CTRL:
            return VHK_MOD_CONTROL;
        case KMOD_LALT:
#ifdef MACOS_COMPILE
            return VHK_MOD_OPTION;
#else
            return VHK_MOD_ALT;
#endif
#ifdef USE_SDL2UI
        case KMOD_LGUI:     /* fall through */
        case KMOD_RGUI:     /* fall through */
        case KMOD_GUI:
# ifdef MACOS_COMPILE
            return VHK_MOD_COMMAND;
# else
            return VHK_MOD_META;
# endif
#endif
        default:
            return VHK_MOD_NONE;
    }
}

/** \brief  Translate VICE keysym to SDL keycode
 *
 * \param[in]   vice_keysym VICE keysym
 *
 * \return  SDL keycode or 0 when not found
 */
uint32_t ui_hotkeys_arch_keysym_to_arch(uint32_t vice_keysym)
{
    for (size_t i = 0; i < ARR_ELEMENTS(keysym_table); i++) {
        if (keysym_table[i].vice_keysym == vice_keysym) {
            return keysym_table[i].arch_keysym;
        }
    }
    return 0;
}


/** \brief  Translate SDL keycode to VICE keysym
 *
 * \param[in]   arch_keysym SDL keycode
 *
 * \return  VICE keysym or 0 when not found
 */
uint32_t ui_hotkeys_arch_keysym_from_arch(uint32_t arch_keysym)
{
    for (size_t i = 0; i < ARR_ELEMENTS(keysym_table); i++) {
        if (keysym_table[i].arch_keysym == arch_keysym) {
            return keysym_table[i].vice_keysym;
        }
    }
    return 0;
}


/** \brief  Translate VICE modifier mask to SDL modifier mask
 *
 * \param[in]   vice_modmask    VICE modifier mask
 *
 * \return  SDL modifier mask
 */
uint32_t ui_hotkeys_arch_modmask_to_arch(uint32_t vice_modmask)
{
    uint32_t mask = 0;

    if ((vice_modmask & VHK_MOD_ALT) || (vice_modmask & VHK_MOD_OPTION)) {
        mask |= KMOD_LALT;
    }
    if (vice_modmask & VHK_MOD_CONTROL) {
        mask |= KMOD_CTRL;
    }
    if (vice_modmask & VHK_MOD_SHIFT) {
        mask |= KMOD_SHIFT;
    }
#ifdef USE_SDL2UI
    if ((vice_modmask & VHK_MOD_META) || (vice_modmask & VHK_MOD_COMMAND)) {
        mask |= KMOD_GUI;
    }
#endif

    return mask;
}


/** \brief  Translate SDL modifier mask to VICE modifier mask
 *
 * \param[in]   arch_modmask    SDL modifier mask
 *
 * \return  VICE modifier mask
 */
uint32_t ui_hotkeys_arch_modmask_from_arch(uint32_t arch_modmask)
{
    uint32_t mask = 0;

    /* only left Alt, right Alt can be Alt-Gr */
    if (arch_modmask & KMOD_LALT) {
#ifdef MACOS_COMPILE
        mask |= VHK_MOD_OPTION;
#else
        mask |= VHK_MOD_ALT;
#endif
    }
    if (arch_modmask & KMOD_CTRL) {
        mask |= VHK_MOD_CONTROL;
    }
    if (arch_modmask & KMOD_SHIFT) {
        mask |= VHK_MOD_SHIFT;
    }
#ifdef USE_SDL2UI
    if (arch_modmask & KMOD_GUI) {
# ifdef MACOS_COMPILE
        mask |= VHK_MOD_COMMAND;
# else
        mask |= VHK_MOD_META;
# endif
    }
#endif

    return mask;
}

void ui_hotkeys_arch_install_by_map(ui_action_map_t *map)
{
    int key;
#ifdef DEBUG_HOTKEYS
    printf(">> Install hotkey: action ID %d, name '%s'\n",
           map->action, ui_action_get_name(map->action));
    printf(">>                 VICE keysym = %04x, VICE modmask = %04x\n",
           map->vice_keysym, map->vice_modmask);
    printf(">>                 SDL  keysym = %04x, SDL  modmask = %04x\n",
           map->arch_keysym, map->arch_modmask);
#endif
#ifdef USE_SDL2UI
    key = SDL2x_to_SDL1x_Keys(map->arch_keysym);
#else
    key = map->arch_keysym;
#endif
    /* check for the menu key */
    if (key == sdl_ui_menukeys[0] && hotkeys_allowed_modifiers(map->arch_modmask) == 0) {
        char *label = ui_action_get_hotkey_label(map->action);

        log_warning(vhk_log,
                    "Cannot set hotkey for action '%s', hotkey %s is already"
                    " used for activating the menu.",
                    ui_action_get_name(map->action), label);
        lib_free(label);
        ui_action_map_clear_hotkey(map);
    }
}

void ui_hotkeys_arch_remove_by_map(ui_action_map_t *map)
{
}

void ui_hotkeys_arch_init(void)
{
}

void ui_hotkeys_arch_shutdown(void)
{
}


/* for debugging */
static int submenu_count = 0;
static int item_count    = 0;
static int action_count  = 0;

/** \brief  Iterate a (sub)menu and item references in the action mappings
 *
 * \param[in]   menu    (sub)menu
 * \param[in]   indent  initial indentation level for pretty printing (use \<;0
 *                      to disable printing entirely)
 */
static void iter_menu(ui_menu_entry_t *menu, int indent)
{
    while (menu != NULL && menu->string != NULL) {
        ui_menu_entry_type_t  type   = menu->type;
        char                 *string = menu->string;
        void                 *data   = menu->data;
        int                   action = menu->action;

        if (indent >= 0) {
            for (int i = 0; i < indent * 4; i++) {
                putchar(' ');
            }
        }

        /* we don't care about DYNAMIC_SUBMENU since the menu code doesn't
         * allow setting hotkeys for those */
        if (type == MENU_ENTRY_SUBMENU) {
            submenu_count++;
            if (indent >= 0) {
                printf("  %s >>\n", string);
                iter_menu(data, indent + 1);
            } else {
                iter_menu(data, indent);
            }
        } else {
            int c = ' ';
            if (type == MENU_ENTRY_RESOURCE_TOGGLE ||
                type == MENU_ENTRY_OTHER_TOGGLE) {
                c = 'v';
            } else if (type == MENU_ENTRY_RESOURCE_RADIO) {
                c = 'o';
            }

            if (*string == '\0') {
                /* empty string is a separator */
                if (indent >= 0) {
                    printf("  -  -  -  -  - - \n");
                }
            } else if (type == MENU_ENTRY_TEXT && data != NULL) {
                /* inverted text (header) */
                if (indent >= 0) {
                    printf("  [%s]\n", string);
                }
            } else {
                if (indent >= 0) {
                    printf("%c %s", c, string);
                    if (action > 0) {
                        printf("   (%d: %s)", action, ui_action_get_name(action));
                    }
                    printf("\n");
                }
            }
            item_count++;
            if (action > ACTION_NONE) {
                ui_action_map_t *map = ui_action_map_get(action);
                if (map != NULL) {
                    map->menu_item[0] = menu;
                    action_count++;
                }
            }
        }
        menu++;
    }
}


/** \brief  Iterate menus and store references in the action mappings
 *
 * Iterate all (sub)menus and their items and store references to them in the
 * UI action mappings array if a valid action ID is found.
 */
void hotkeys_iterate_menu(void)
{
    /* disable printing of menu structure by passing <0 indentation */
    iter_menu(sdl_ui_get_main_menu(), -1);
#if 0
    printf("%s(): %d (sub)menus with %d items, found %d action IDs.\n",
           __func__, submenu_count, item_count, action_count);
#endif
}


/** \brief  Set hotkey from the menu
 *
 * Set hotkey for UI action of \a item to \a arch_keysym + \a arch_modmask.
 * This is called from the menu when the user presses 'M' to set a hotkey.
 * Unsets the hotkey from any UI action the hotkey might have been associated
 * with before and sets the new hotkey on \a item, overwriting any previously
 * set hotkey on that \a item.
 *
 * \param[in]   arch_keysym     SDL keysym
 * \param[in]   arch_modmask    SDL modifier mask
 *
 * \return  \c true on success
 */
bool hotkeys_set_hotkey_from_menu(uint32_t         arch_keysym,
                                  uint32_t         arch_modmask,
                                  ui_menu_entry_t *item)
{
    ui_action_map_t *map;

    if (item->action <= ACTION_NONE || item->action >= ACTION_ID_COUNT) {
        /* invalid action ID */
        log_warning(vhk_log,
                    "Cannot set hotkey for invalid action ID %d",
                    item->action);
        return false;
    }
    if (arch_keysym == 0) {
        /* invalid keysym */
        log_warning(vhk_log, "Cannot set hotkey for invalid keysym of 0");
        return false;
    }

    /* delete old hotkey mapping, if present */
    map = ui_action_map_get_by_arch_hotkey(arch_keysym, arch_modmask);
    if (map != NULL) {
        ui_action_map_clear_hotkey(map);
    }

    /* set new hotkey */
    map = ui_action_map_get(item->action);
    if (map != NULL) {
        uint32_t  vice_keysym  = ui_hotkeys_arch_keysym_from_arch(arch_keysym);
        uint32_t  vice_modmask = ui_hotkeys_arch_modmask_from_arch(arch_modmask);
#ifdef DEBUG_HOTKEYS
        char     *label;
#endif

        ui_action_map_set_hotkey_by_map(map,
                                        vice_keysym, vice_modmask,
                                        arch_keysym, arch_modmask);
#ifdef DEBUG_HOTKEYS
        label = ui_action_get_hotkey_label(item->action);
        printf("%s(): hotkey assigned %s to action %s\n",
               __func__, label, ui_action_get_name(item->action));
        lib_free(label);
#endif
    } else {
        /* no action handler registered */
        log_warning(vhk_log,
                    "No action handler registered for action %d (%s).",
                    item->action, ui_action_get_name(item->action));
        return false;
    }

    return true;
}


/** \brief  Filter out modifiers we don't allow
 *
 * Filter out modifiers like the *Lock keys and right Alt.
 *
 * \param[in]   modmask SDL modifier mask
 *
 * \return  filtered modifier mask
 */
uint32_t hotkeys_allowed_modifiers(uint32_t modmask)
{
    return modmask & ALLOWED_SDL_MODS;
}
