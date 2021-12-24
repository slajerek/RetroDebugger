/*
 * menu_video.c - SDL video settings menus.
 *
 * Written by
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
#include "types.h"

#include "fullscreenarch.h"
#include "lib.h"
#include "machine.h"
#include "menu_common.h"
#include "menu_video.h"
#include "resources.h"
#include "ui.h"
#include "uifilereq.h"
#include "uimenu.h"
#include "vicii.h"
#include "videoarch.h"

/* Border mode menu */

UI_MENU_DEFINE_RADIO(VICIIBorderMode)
UI_MENU_DEFINE_RADIO(VICBorderMode)
UI_MENU_DEFINE_RADIO(TEDBorderMode)

static const ui_menu_entry_t vicii_border_menu[] = {
    { "Normal",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIBorderMode_callback,
      (ui_callback_data_t)VICII_NORMAL_BORDERS },
    { "Full",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIBorderMode_callback,
      (ui_callback_data_t)VICII_FULL_BORDERS },
    { "Debug",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIBorderMode_callback,
      (ui_callback_data_t)VICII_DEBUG_BORDERS },
    { "None",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VICIIBorderMode_callback,
      (ui_callback_data_t)VICII_NO_BORDERS },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vic_border_menu[] = {
//    { "Normal",
//      MENU_ENTRY_RESOURCE_RADIO,
//      radio_VICBorderMode_callback,
//      (ui_callback_data_t)VIC_NORMAL_BORDERS },
//    { "Full",
//      MENU_ENTRY_RESOURCE_RADIO,
//      radio_VICBorderMode_callback,
//      (ui_callback_data_t)VIC_FULL_BORDERS },
//    { "Debug",
//      MENU_ENTRY_RESOURCE_RADIO,
//      radio_VICBorderMode_callback,
//      (ui_callback_data_t)VIC_DEBUG_BORDERS },
//    { "None",
//      MENU_ENTRY_RESOURCE_RADIO,
//      radio_VICBorderMode_callback,
//      (ui_callback_data_t)VIC_NO_BORDERS },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ted_border_menu[] = {
//    { "Normal",
//      MENU_ENTRY_RESOURCE_RADIO,
//      radio_TEDBorderMode_callback,
//      (ui_callback_data_t)TED_NORMAL_BORDERS },
//    { "Full",
//      MENU_ENTRY_RESOURCE_RADIO,
//      radio_TEDBorderMode_callback,
//      (ui_callback_data_t)TED_FULL_BORDERS },
//    { "Debug",
//      MENU_ENTRY_RESOURCE_RADIO,
//      radio_TEDBorderMode_callback,
//      (ui_callback_data_t)TED_DEBUG_BORDERS },
//    { "None",
//      MENU_ENTRY_RESOURCE_RADIO,
//      radio_TEDBorderMode_callback,
//      (ui_callback_data_t)TED_NO_BORDERS },
   SDL_MENU_LIST_END
};

/* audio leak */

UI_MENU_DEFINE_TOGGLE(VICIIAudioLeak)
UI_MENU_DEFINE_TOGGLE(VDCAudioLeak)
UI_MENU_DEFINE_TOGGLE(CrtcAudioLeak)
UI_MENU_DEFINE_TOGGLE(TEDAudioLeak)
UI_MENU_DEFINE_TOGGLE(VICAudioLeak)

/* CRT emulation menu */

UI_MENU_DEFINE_SLIDER(VICPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER(VICPALBlur, 0, 1000)
UI_MENU_DEFINE_SLIDER(VICPALOddLinePhase, 0, 2000)
UI_MENU_DEFINE_SLIDER(VICPALOddLineOffset, 0, 2000)

UI_MENU_DEFINE_SLIDER(VDCPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER(VDCPALBlur, 0, 1000)
UI_MENU_DEFINE_SLIDER(VDCPALOddLinePhase, 0, 2000)
UI_MENU_DEFINE_SLIDER(VDCPALOddLineOffset, 0, 2000)

UI_MENU_DEFINE_SLIDER(VICIIPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER(VICIIPALBlur, 0, 1000)
UI_MENU_DEFINE_SLIDER(VICIIPALOddLinePhase, 0, 2000)
UI_MENU_DEFINE_SLIDER(VICIIPALOddLineOffset, 0, 2000)

UI_MENU_DEFINE_SLIDER(TEDPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER(TEDPALBlur, 0, 1000)
UI_MENU_DEFINE_SLIDER(TEDPALOddLinePhase, 0, 2000)
UI_MENU_DEFINE_SLIDER(TEDPALOddLineOffset, 0, 2000)

UI_MENU_DEFINE_SLIDER(CrtcPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER(CrtcPALBlur, 0, 1000)
UI_MENU_DEFINE_SLIDER(CrtcPALOddLinePhase, 0, 2000)
UI_MENU_DEFINE_SLIDER(CrtcPALOddLineOffset, 0, 2000)

#define VICE_SDL_CRTEMU_MENU_ITEMS(chip)                        \
    { "Scanline shade",                                         \
      MENU_ENTRY_RESOURCE_INT,                                  \
      slider_##chip##PALScanLineShade_callback,                 \
      (ui_callback_data_t)"Set PAL shade (0-1000)" },           \
    { "Blur",                                                   \
      MENU_ENTRY_RESOURCE_INT,                                  \
      slider_##chip##PALBlur_callback,                          \
      (ui_callback_data_t)"Set PAL blur (0-1000)" },            \
    { "Oddline phase",                                          \
      MENU_ENTRY_RESOURCE_INT,                                  \
      slider_##chip##PALOddLinePhase_callback,                  \
      (ui_callback_data_t)"Set PAL oddline phase (0-2000)" },   \
    { "Oddline offset",                                         \
      MENU_ENTRY_RESOURCE_INT,                                  \
      slider_##chip##PALOddLineOffset_callback,                 \
      (ui_callback_data_t)"Set PAL oddline offset (0-2000)" }

static const ui_menu_entry_t vic_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_MENU_ITEMS(VIC),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vicii_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_MENU_ITEMS(VICII),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vdc_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_MENU_ITEMS(VDC),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ted_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_MENU_ITEMS(TED),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t crtc_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_MENU_ITEMS(Crtc),
    SDL_MENU_LIST_END
};


/* Color menu */

UI_MENU_DEFINE_SLIDER(VICColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER(VICColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER(VICColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER(VICColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER(VICColorBrightness, 0, 2000)

UI_MENU_DEFINE_SLIDER(VICIIColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER(VICIIColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER(VICIIColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER(VICIIColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER(VICIIColorBrightness, 0, 2000)

UI_MENU_DEFINE_SLIDER(VDCColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER(VDCColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER(VDCColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER(VDCColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER(VDCColorBrightness, 0, 2000)

UI_MENU_DEFINE_SLIDER(TEDColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER(TEDColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER(TEDColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER(TEDColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER(TEDColorBrightness, 0, 2000)

UI_MENU_DEFINE_SLIDER(CrtcColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER(CrtcColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER(CrtcColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER(CrtcColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER(CrtcColorBrightness, 0, 2000)

#define VICE_SDL_COLOR_MENU_ITEMS(chip)                        \
    { "Gamma",                                                 \
      MENU_ENTRY_RESOURCE_INT,                                 \
      slider_##chip##ColorGamma_callback,                      \
      (ui_callback_data_t)"Set gamma (0-4000)" },              \
    { "Tint",                                                  \
      MENU_ENTRY_RESOURCE_INT,                                 \
      slider_##chip##ColorTint_callback,                       \
      (ui_callback_data_t)"Set tint (0-2000)" },               \
    { "Saturation",                                            \
      MENU_ENTRY_RESOURCE_INT,                                 \
      slider_##chip##ColorSaturation_callback,                 \
      (ui_callback_data_t)"Set saturation (0-2000)" },         \
    { "Contrast",                                              \
      MENU_ENTRY_RESOURCE_INT,                                 \
      slider_##chip##ColorContrast_callback,                   \
      (ui_callback_data_t)"Set contrast (0-2000)" },           \
    { "Brightness",                                            \
      MENU_ENTRY_RESOURCE_INT,                                 \
      slider_##chip##ColorBrightness_callback,                 \
      (ui_callback_data_t)"Set brightness (0-2000)" }

static const ui_menu_entry_t vic_color_controls_menu[] = {
    VICE_SDL_COLOR_MENU_ITEMS(VIC),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vicii_color_controls_menu[] = {
    VICE_SDL_COLOR_MENU_ITEMS(VICII),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vdc_color_controls_menu[] = {
    VICE_SDL_COLOR_MENU_ITEMS(VDC),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ted_color_controls_menu[] = {
    VICE_SDL_COLOR_MENU_ITEMS(TED),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t crtc_color_controls_menu[] = {
    VICE_SDL_COLOR_MENU_ITEMS(Crtc),
    SDL_MENU_LIST_END
};

/* Size menu template */

UI_MENU_DEFINE_INT(SDLCustomWidth)
UI_MENU_DEFINE_INT(SDLCustomHeight)
UI_MENU_DEFINE_RADIO(SDLLimitMode)

#define VICE_SDL_SIZE_MENU_DOUBLESIZE(chip)         \
    { "Double size",                                \
      MENU_ENTRY_RESOURCE_TOGGLE,                   \
      toggle_##chip##DoubleSize_callback,           \
      NULL },

#define VICE_SDL_SIZE_MENU_STRETCHVERTICAL(chip)    \
    { "Stretch vertically",                         \
      MENU_ENTRY_RESOURCE_TOGGLE,                   \
      toggle_##chip##StretchVertical_callback,      \
      NULL },

#define VICE_SDL_SIZE_MENU_ITEMS(chip)              \
    { "Double scan",                                \
      MENU_ENTRY_RESOURCE_TOGGLE,                   \
      toggle_##chip##DoubleScan_callback,           \
      NULL },                                       \
    { "Fullscreen",                                 \
      MENU_ENTRY_RESOURCE_TOGGLE,                   \
      toggle_##chip##Fullscreen_callback,           \
      NULL },                                       \
    SDL_MENU_ITEM_SEPARATOR,                        \
    SDL_MENU_ITEM_TITLE("Fullscreen mode"),         \
    { "Automatic",                                  \
      MENU_ENTRY_RESOURCE_RADIO,                    \
      radio_##chip##SDLFullscreenMode_callback,     \
      (ui_callback_data_t)FULLSCREEN_MODE_AUTO },   \
    { "Custom",                                     \
      MENU_ENTRY_RESOURCE_RADIO,                    \
      radio_##chip##SDLFullscreenMode_callback,     \
      (ui_callback_data_t)FULLSCREEN_MODE_CUSTOM }, \
    SDL_MENU_ITEM_SEPARATOR,                        \
    SDL_MENU_ITEM_TITLE("Custom resolution"),       \
    { "Width",                                      \
      MENU_ENTRY_RESOURCE_INT,                      \
      int_SDLCustomWidth_callback,                  \
      (ui_callback_data_t)"Set width" },            \
    { "Height",                                     \
      MENU_ENTRY_RESOURCE_INT,                      \
      int_SDLCustomHeight_callback,                 \
      (ui_callback_data_t)"Set height" },           \
    SDL_MENU_ITEM_SEPARATOR,                        \
    SDL_MENU_ITEM_TITLE("Resolution limit mode"),   \
    { "Off",                                        \
      MENU_ENTRY_RESOURCE_RADIO,                    \
      radio_SDLLimitMode_callback,                  \
      (ui_callback_data_t)SDL_LIMIT_MODE_OFF },     \
    { "Max",                                        \
      MENU_ENTRY_RESOURCE_RADIO,                    \
      radio_SDLLimitMode_callback,                  \
      (ui_callback_data_t)SDL_LIMIT_MODE_MAX },     \
    { "Fixed",                                      \
      MENU_ENTRY_RESOURCE_RADIO,                    \
      radio_SDLLimitMode_callback,                  \
      (ui_callback_data_t)SDL_LIMIT_MODE_FIXED },

#ifdef HAVE_HWSCALE

UI_MENU_DEFINE_RADIO(SDLGLAspectMode)

static const ui_menu_entry_t aspect_menu[] = {
    { "Off",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_SDLGLAspectMode_callback,
      (ui_callback_data_t)SDL_ASPECT_MODE_OFF },
    { "Custom",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_SDLGLAspectMode_callback,
      (ui_callback_data_t)SDL_ASPECT_MODE_CUSTOM },
    { "True",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_SDLGLAspectMode_callback,
      (ui_callback_data_t)SDL_ASPECT_MODE_TRUE },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(VICIIHwScale)
UI_MENU_DEFINE_TOGGLE(VDCHwScale)
UI_MENU_DEFINE_TOGGLE(CrtcHwScale)
UI_MENU_DEFINE_TOGGLE(TEDHwScale)
UI_MENU_DEFINE_TOGGLE(VICHwScale)
UI_MENU_DEFINE_STRING(AspectRatio)
UI_MENU_DEFINE_TOGGLE(SDLGLFlipX)
UI_MENU_DEFINE_TOGGLE(SDLGLFlipY)

#define VICE_SDL_SIZE_MENU_OPENGL_ITEMS(chip)  \
    SDL_MENU_ITEM_SEPARATOR,                   \
    SDL_MENU_ITEM_TITLE("OpenGL"),             \
    { "OpenGL free scaling",                   \
      MENU_ENTRY_RESOURCE_TOGGLE,              \
      toggle_##chip##HwScale_callback,         \
      NULL },                                  \
    { "Fixed aspect ratio",                    \
      MENU_ENTRY_SUBMENU,                      \
      submenu_radio_callback,                  \
      (ui_callback_data_t)aspect_menu },       \
    { "Custom aspect ratio",                   \
      MENU_ENTRY_RESOURCE_STRING,              \
      string_AspectRatio_callback,             \
      (ui_callback_data_t)"Set aspect ratio (0.5 - 2.0)" }, \
    { "Flip X",                                \
      MENU_ENTRY_RESOURCE_TOGGLE,              \
      toggle_SDLGLFlipX_callback,              \
      NULL },                                  \
    { "Flip Y",                                \
      MENU_ENTRY_RESOURCE_TOGGLE,              \
      toggle_SDLGLFlipY_callback,              \
      NULL },
#endif


/* VICII size menu */

UI_MENU_DEFINE_TOGGLE(VICIIDoubleSize)
UI_MENU_DEFINE_TOGGLE(VICIIDoubleScan)
UI_MENU_DEFINE_TOGGLE(VICIIFullscreen)
UI_MENU_DEFINE_RADIO(VICIISDLFullscreenMode)

static const ui_menu_entry_t vicii_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(VICII)
    VICE_SDL_SIZE_MENU_ITEMS(VICII)
#ifdef HAVE_HWSCALE
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(VICII)
#endif
    SDL_MENU_LIST_END
};


/* VDC size menu */

UI_MENU_DEFINE_TOGGLE(VDCDoubleSize)
UI_MENU_DEFINE_TOGGLE(VDCStretchVertical)
UI_MENU_DEFINE_TOGGLE(VDCDoubleScan)
UI_MENU_DEFINE_TOGGLE(VDCFullscreen)
UI_MENU_DEFINE_RADIO(VDCSDLFullscreenMode)

static const ui_menu_entry_t vdc_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(VDC)
    VICE_SDL_SIZE_MENU_STRETCHVERTICAL(VDC)
    VICE_SDL_SIZE_MENU_ITEMS(VDC)
#ifdef HAVE_HWSCALE
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(VDC)
#endif
    SDL_MENU_LIST_END
};


/* Crtc size menu */

UI_MENU_DEFINE_TOGGLE(CrtcFullscreen)
UI_MENU_DEFINE_RADIO(CrtcSDLFullscreenMode)
UI_MENU_DEFINE_TOGGLE(CrtcDoubleSize)
UI_MENU_DEFINE_TOGGLE(CrtcStretchVertical)
UI_MENU_DEFINE_TOGGLE(CrtcDoubleScan)

static const ui_menu_entry_t crtc_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(Crtc)
    VICE_SDL_SIZE_MENU_STRETCHVERTICAL(Crtc)
    VICE_SDL_SIZE_MENU_ITEMS(Crtc)
#ifdef HAVE_HWSCALE
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(Crtc)
#endif
    SDL_MENU_LIST_END
};


/* TED size menu */

UI_MENU_DEFINE_TOGGLE(TEDDoubleSize)
UI_MENU_DEFINE_TOGGLE(TEDDoubleScan)
UI_MENU_DEFINE_TOGGLE(TEDFullscreen)
UI_MENU_DEFINE_RADIO(TEDSDLFullscreenMode)

static const ui_menu_entry_t ted_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(TED)
    VICE_SDL_SIZE_MENU_ITEMS(TED)
#ifdef HAVE_HWSCALE
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(TED)
#endif
    SDL_MENU_LIST_END
};


/* VIC size menu */

UI_MENU_DEFINE_TOGGLE(VICDoubleSize)
UI_MENU_DEFINE_TOGGLE(VICDoubleScan)
UI_MENU_DEFINE_TOGGLE(VICFullscreen)
UI_MENU_DEFINE_RADIO(VICSDLFullscreenMode)

static const ui_menu_entry_t vic_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(VIC)
    VICE_SDL_SIZE_MENU_ITEMS(VIC)
#ifdef HAVE_HWSCALE
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(VIC)
#endif
    SDL_MENU_LIST_END
};


/* Output Rendering Filter */
UI_MENU_DEFINE_RADIO(VICIIFilter)
UI_MENU_DEFINE_RADIO(TEDFilter)
UI_MENU_DEFINE_RADIO(VICFilter)
UI_MENU_DEFINE_RADIO(VDCFilter)
UI_MENU_DEFINE_RADIO(CrtcFilter)

#define VICE_SDL_FILTER_MENU_ITEMS(chip)  \
    { "None",                                  \
      MENU_ENTRY_RESOURCE_RADIO,               \
      radio_##chip##Filter_callback,           \
      (ui_callback_data_t)VIDEO_FILTER_NONE }, \
    { "CRT Emulation",                         \
      MENU_ENTRY_RESOURCE_RADIO,               \
      radio_##chip##Filter_callback,           \
      (ui_callback_data_t)VIDEO_FILTER_CRT }

#define VICE_SDL_FILTER_MENU_SCALE2X_ITEMS(chip)  \
    { "Scale2x",                               \
      MENU_ENTRY_RESOURCE_RADIO,               \
      radio_##chip##Filter_callback,           \
      (ui_callback_data_t)VIDEO_FILTER_SCALE2X }

static const ui_menu_entry_t vicii_filter_menu[] = {
    VICE_SDL_FILTER_MENU_ITEMS(VICII),
    VICE_SDL_FILTER_MENU_SCALE2X_ITEMS(VICII),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ted_filter_menu[] = {
    VICE_SDL_FILTER_MENU_ITEMS(TED),
    VICE_SDL_FILTER_MENU_SCALE2X_ITEMS(TED),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vic_filter_menu[] = {
    VICE_SDL_FILTER_MENU_ITEMS(VIC),
    VICE_SDL_FILTER_MENU_SCALE2X_ITEMS(VIC),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t crtc_filter_menu[] = {
    VICE_SDL_FILTER_MENU_ITEMS(Crtc),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vdc_filter_menu[] = {
    VICE_SDL_FILTER_MENU_ITEMS(VDC),
    SDL_MENU_LIST_END
};

/* Misc. callbacks */
UI_MENU_DEFINE_TOGGLE(VICIIVideoCache)
UI_MENU_DEFINE_TOGGLE(VICIINewLuminances)
UI_MENU_DEFINE_TOGGLE(VDCVideoCache)
UI_MENU_DEFINE_TOGGLE(CrtcVideoCache)
UI_MENU_DEFINE_TOGGLE(TEDVideoCache)
UI_MENU_DEFINE_TOGGLE(VICVideoCache)
UI_MENU_DEFINE_TOGGLE(VICIIExternalPalette)
UI_MENU_DEFINE_TOGGLE(VDCExternalPalette)
UI_MENU_DEFINE_TOGGLE(CrtcExternalPalette)
UI_MENU_DEFINE_TOGGLE(TEDExternalPalette)
UI_MENU_DEFINE_TOGGLE(VICExternalPalette)
UI_MENU_DEFINE_RADIO(MachineVideoStandard)

static UI_MENU_CALLBACK(restore_size_callback)
{
//    if (activated) {
//        sdl_video_restore_size();
//    }
    return NULL;
}

UI_MENU_DEFINE_FILE_STRING(VICIIPaletteFile)
UI_MENU_DEFINE_FILE_STRING(VDCPaletteFile)
UI_MENU_DEFINE_FILE_STRING(CrtcPaletteFile)
UI_MENU_DEFINE_FILE_STRING(TEDPaletteFile)
UI_MENU_DEFINE_FILE_STRING(VICPaletteFile)


/* C128 video menu */

static UI_MENU_CALLBACK(radio_VideoOutput_c128_callback)
{
//    int value = vice_ptr_to_int(param);
//
//    if (activated) {
//        sdl_video_canvas_switch(value);
//    } else {
//        if (value == sdl_active_canvas->index) {
//            return sdl_menu_text_tick;
//        }
//    }
    return NULL;
}

const ui_menu_entry_t c128_video_menu[] = {
    SDL_MENU_ITEM_TITLE("Video output"),
    { "VICII (40 cols)",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VideoOutput_c128_callback,
      (ui_callback_data_t)0 },
    { "VDC (80 cols)",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_VideoOutput_c128_callback,
      (ui_callback_data_t)1 },
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_size_menu },
    { "VDC size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vdc_size_menu },
    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIVideoCache_callback,
      NULL },
#ifndef DINGOO_NATIVE
    { "VICII border mode",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_border_menu },
#endif
    { "VICII New luminances",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIINewLuminances_callback,
      NULL },
    { "VICII Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_color_controls_menu },
    { "VICII CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_crt_controls_menu },
    { "VICII render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_filter_menu },
    { "External VICII palette",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIExternalPalette_callback,
      NULL },
    { "VICII external palette file",
      MENU_ENTRY_DIALOG,
      file_string_VICIIPaletteFile_callback,
      (ui_callback_data_t)"Choose VICII palette file" },
    { "VICII Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIAudioLeak_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "VDC Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VDCVideoCache_callback,
      NULL },
    { "VDC Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vdc_color_controls_menu },
    { "VDC CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vdc_crt_controls_menu },
    { "VDC render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vdc_filter_menu },
    { "External VDC palette",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VDCExternalPalette_callback,
      NULL },
    { "VDC external palette file",
      MENU_ENTRY_DIALOG,
      file_string_VDCPaletteFile_callback,
      (ui_callback_data_t)"Choose VDC palette file" },
    { "VDC Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VDCAudioLeak_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Video Standard"),
    { "PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_PAL },
    { "NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_NTSC },
    SDL_MENU_LIST_END
};


/* C64 video menu */

const ui_menu_entry_t c64_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_size_menu },
    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII border mode",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_border_menu },
    SDL_MENU_ITEM_SEPARATOR,
    { "New luminances",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIINewLuminances_callback,
      NULL },
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_color_controls_menu },
    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_filter_menu },
    SDL_MENU_ITEM_SEPARATOR,
    { "External palette",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIExternalPalette_callback,
      NULL },
    { "External palette file",
      MENU_ENTRY_DIALOG,
      file_string_VICIIPaletteFile_callback,
      (ui_callback_data_t)"Choose palette file" },
    { "VICII Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIAudioLeak_callback,
      NULL },
    SDL_MENU_LIST_END
};


/* C64SC video menu */

const ui_menu_entry_t c64sc_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_size_menu },
    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII border mode",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_border_menu },
    SDL_MENU_ITEM_SEPARATOR,
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_color_controls_menu },
    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_filter_menu },
    SDL_MENU_ITEM_SEPARATOR,
    { "External palette",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIExternalPalette_callback,
      NULL },
    { "External palette file",
      MENU_ENTRY_DIALOG,
      file_string_VICIIPaletteFile_callback,
      (ui_callback_data_t)"Choose palette file" },
    { "VICII Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIAudioLeak_callback,
      NULL },
    SDL_MENU_LIST_END
};


/* C64DTV video menu */

const ui_menu_entry_t c64dtv_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_size_menu },
    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII border mode",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_border_menu },
    SDL_MENU_ITEM_SEPARATOR,
    { "Colorfix",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIINewLuminances_callback,
      NULL },
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_color_controls_menu },
    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_filter_menu },
#if 0   /* disabled until there are external DTV palette files available */
    { "External palette",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIExternalPalette_callback,
      NULL },
    { "External palette file",
      MENU_ENTRY_DIALOG,
      file_string_VICIIPaletteFile_callback,
      (ui_callback_data_t)"Choose palette file" },
#endif
    { "VICII Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIAudioLeak_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Video Standard"),
    { "PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_PAL },
    { "NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_NTSC },
    SDL_MENU_LIST_END
};


/* CBM-II 5x0 video menu */

const ui_menu_entry_t cbm5x0_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_size_menu },
    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "VICII border mode",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_border_menu },
    SDL_MENU_ITEM_SEPARATOR,
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_color_controls_menu },
    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vicii_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vicii_filter_menu },
    SDL_MENU_ITEM_SEPARATOR,
    { "External palette",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIExternalPalette_callback,
      NULL },
    { "External palette file",
      MENU_ENTRY_DIALOG,
      file_string_VICIIPaletteFile_callback,
      (ui_callback_data_t)"Choose palette file" },
    { "VICII Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICIIAudioLeak_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Video Standard"),
    { "PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_PAL },
    { "NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_NTSC },
    SDL_MENU_LIST_END
};


/* CBM-II 6x0/7x0 video menu */

const ui_menu_entry_t cbm6x0_7x0_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)crtc_size_menu },
    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_CrtcVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)crtc_color_controls_menu },
    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)crtc_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)crtc_filter_menu },
    { "External palette",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_CrtcExternalPalette_callback,
      NULL },
    { "External palette file",
      MENU_ENTRY_DIALOG,
      file_string_CrtcPaletteFile_callback,
      (ui_callback_data_t)"Choose palette file" },
    { "CRTC Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_CrtcAudioLeak_callback,
      NULL },
    SDL_MENU_LIST_END
};


/* PET video menu */

const ui_menu_entry_t pet_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)crtc_size_menu },
    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_CrtcVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)crtc_color_controls_menu },
    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)crtc_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)crtc_filter_menu },
    { "External palette",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_CrtcExternalPalette_callback,
      NULL },
    { "External palette file",
      MENU_ENTRY_DIALOG,
      file_string_CrtcPaletteFile_callback,
      (ui_callback_data_t)"Choose palette file" },
    { "CRTC Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_CrtcAudioLeak_callback,
      NULL },
    SDL_MENU_LIST_END
};


/* PLUS4 video menu */

const ui_menu_entry_t plus4_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)ted_size_menu },
    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_TEDVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "TED border mode",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)ted_border_menu },
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)ted_color_controls_menu },
    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)ted_crt_controls_menu },
    { "Render filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)ted_filter_menu },
    SDL_MENU_ITEM_SEPARATOR,
    { "External palette",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_TEDExternalPalette_callback,
      NULL },
    { "External palette file",
      MENU_ENTRY_DIALOG,
      file_string_TEDPaletteFile_callback,
      (ui_callback_data_t)"Choose palette file" },
    { "TED Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_TEDAudioLeak_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Video Standard"),
    { "PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_PAL },
    { "NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_callback,
      (ui_callback_data_t)MACHINE_SYNC_NTSC },
    SDL_MENU_LIST_END
};


/* VIC-20 video menu */

static UI_MENU_CALLBACK(radio_MachineVideoStandard_vic20_callback)
{
    if (activated) {
        int value = vice_ptr_to_int(param);
        resources_set_int("MachineVideoStandard", value);
        machine_trigger_reset(MACHINE_RESET_MODE_HARD);
        return sdl_menu_text_exit_ui;
    }
    return sdl_ui_menu_radio_helper(activated, param, "MachineVideoStandard");
}

const ui_menu_entry_t vic20_video_menu[] = {
    { "Size settings",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vic_size_menu },
    { "Restore window size",
      MENU_ENTRY_OTHER,
      restore_size_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Video cache",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICVideoCache_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "VIC border mode",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vic_border_menu },
    { "Color controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vic_color_controls_menu },
    { "CRT emulation controls",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)vic_crt_controls_menu },
    { "Rrender filter",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)vic_filter_menu },
    SDL_MENU_ITEM_SEPARATOR,
    { "External palette",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICExternalPalette_callback,
      NULL },
    { "External palette file",
      MENU_ENTRY_DIALOG,
      file_string_VICPaletteFile_callback,
      (ui_callback_data_t)"Choose palette file" },
    { "VIC Audio Leak emulation",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_VICAudioLeak_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Video Standard"),
    { "PAL",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_vic20_callback,
      (ui_callback_data_t)MACHINE_SYNC_PAL },
    { "NTSC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MachineVideoStandard_vic20_callback,
      (ui_callback_data_t)MACHINE_SYNC_NTSC },
    SDL_MENU_LIST_END
};
