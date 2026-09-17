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

#include "fullscreenarch.h"
#include "lib.h"
#include "machine.h"
#include "menu_common.h"
#include "palette.h"
#include "resources.h"
#include "ted.h"
#include "types.h"
#include "ui.h"
#include "uiactions.h"
#include "uifilereq.h"
#include "uimenu.h"
#include "util.h"
#include "vic.h"
#include "vicii.h"
#include "videoarch.h"

#include "menu_video.h"


static ui_menu_entry_t *palette_dyn_menu1 = NULL;
static ui_menu_entry_t *palette_dyn_menu2 = NULL;

static ui_menu_entry_t palette_menu1[4];
static ui_menu_entry_t palette_menu2[4];

#define UI_MENU_DEFINE_SLIDER_VIDEO(resource, min, max)                                  \
    static UI_MENU_CALLBACK(slider_##resource##_callback)                          \
    {                                                                              \
        return sdl_ui_menu_video_slider_helper(activated, param, #resource, min, max);   \
    }

/* Border mode menu */

static const ui_menu_entry_t vicii_border_menu[] = {
    {   .action   = ACTION_BORDER_MODE_NORMAL,
        .string   ="Normal",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .data     = (ui_callback_data_t)VICII_NORMAL_BORDERS,
        .resource = "VICIIBorderMode"
    },
    {   .action   = ACTION_BORDER_MODE_FULL,
        .string   = "Full",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .data     = (ui_callback_data_t)VICII_FULL_BORDERS,
        .resource = "VICIIBorderMode"
    },
    {   .action   = ACTION_BORDER_MODE_DEBUG,
        .string   = "Debug",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .data     = (ui_callback_data_t)VICII_DEBUG_BORDERS,
        .resource = "VICIIBorderMode"
    },
    {   .action   = ACTION_BORDER_MODE_NONE,
        .string   = "None",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .data     = (ui_callback_data_t)VICII_NO_BORDERS,
        .resource = "VICIIBorderMode"
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vic_border_menu[] = {
    {   .action   = ACTION_BORDER_MODE_NORMAL,
        .string   ="Normal",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .data     = (ui_callback_data_t)VIC_NORMAL_BORDERS,
        .resource = "VICBorderMode"
    },
    {   .action   = ACTION_BORDER_MODE_FULL,
        .string   = "Full",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .data     = (ui_callback_data_t)VIC_FULL_BORDERS,
        .resource = "VICBorderMode"
    },
    {   .action   = ACTION_BORDER_MODE_DEBUG,
        .string   = "Debug",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .data     = (ui_callback_data_t)VIC_DEBUG_BORDERS,
        .resource = "VICBorderMode"
    },
    {   .action   = ACTION_BORDER_MODE_NONE,
        .string   = "None",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .data     = (ui_callback_data_t)VIC_NO_BORDERS,
        .resource = "VICBorderMode"
    },
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ted_border_menu[] = {
    {   .action   = ACTION_BORDER_MODE_NORMAL,
        .string   ="Normal",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .data     = (ui_callback_data_t)TED_NORMAL_BORDERS,
        .resource = "TEDBorderMode"
    },
    {   .action   = ACTION_BORDER_MODE_FULL,
        .string   = "Full",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .data     = (ui_callback_data_t)TED_FULL_BORDERS,
        .resource = "TEDBorderMode"
    },
    {   .action   = ACTION_BORDER_MODE_DEBUG,
        .string   = "Debug",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .data     = (ui_callback_data_t)TED_DEBUG_BORDERS,
        .resource = "TEDBorderMode"
    },
    {   .action   = ACTION_BORDER_MODE_NONE,
        .string   = "None",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .data     = (ui_callback_data_t)TED_NO_BORDERS,
        .resource = "TEDBorderMode"
    },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(VICIIVSPBug)

/* audio leak */

UI_MENU_DEFINE_TOGGLE(VICIIAudioLeak)
UI_MENU_DEFINE_TOGGLE(TEDAudioLeak)
UI_MENU_DEFINE_TOGGLE(VICAudioLeak)
UI_MENU_DEFINE_TOGGLE(VDCAudioLeak)
UI_MENU_DEFINE_TOGGLE(CrtcAudioLeak)

/* delayline type */

UI_MENU_DEFINE_TOGGLE(VICIIPALDelaylineType)
UI_MENU_DEFINE_TOGGLE(TEDPALDelaylineType)
UI_MENU_DEFINE_TOGGLE(VICPALDelaylineType)

/* CRT emulation menu */

UI_MENU_DEFINE_SLIDER_VIDEO(VICPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICPALBlur, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICPALOddLinePhase, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICPALOddLineOffset, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(VDCPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(VDCPALBlur, 0, 1000)

UI_MENU_DEFINE_SLIDER_VIDEO(VICIIPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIPALBlur, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIPALOddLinePhase, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIPALOddLineOffset, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(TEDPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDPALBlur, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDPALOddLinePhase, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDPALOddLineOffset, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(CrtcPALScanLineShade, 0, 1000)
UI_MENU_DEFINE_SLIDER_VIDEO(CrtcPALBlur, 0, 1000)

#define VICE_SDL_CRTEMU_MENU_ITEMS(chip)                                    \
    {   .string   = "Scanline shade",                                       \
        .type     = MENU_ENTRY_RESOURCE_INT,                                \
        .callback = slider_##chip##PALScanLineShade_callback,               \
        .data     = (ui_callback_data_t)"Set PAL shade (0-1000)"            \
    },                                                                      \
    {   .string   = "Blur",                                                 \
        .type     = MENU_ENTRY_RESOURCE_INT,                                \
        .callback = slider_##chip##PALBlur_callback,                        \
        .data     = (ui_callback_data_t)"Set PAL blur (0-1000)"             \
    }

#define VICE_SDL_CRTEMU_PALNTSC_MENU_ITEMS(chip)                            \
    {   .string   = "Scanline shade",                                       \
        .type     = MENU_ENTRY_RESOURCE_INT,                                \
        .callback = slider_##chip##PALScanLineShade_callback,               \
        .data     = (ui_callback_data_t)"Set PAL shade (0-1000)"            \
    },                                                                      \
    {   .string   = "Blur",                                                 \
        .type     = MENU_ENTRY_RESOURCE_INT,                                \
        .callback = slider_##chip##PALBlur_callback,                        \
        .data     = (ui_callback_data_t)"Set PAL blur (0-1000)"             \
    },                                                                      \
    {   .string   = "Oddline phase",                                        \
        .type     = MENU_ENTRY_RESOURCE_INT,                                \
        .callback = slider_##chip##PALOddLinePhase_callback,                \
        .data     = (ui_callback_data_t)"Set PAL oddline phase (0-2000)"    \
    },                                                                      \
    {   .string   = "Oddline offset",                                       \
        .type     = MENU_ENTRY_RESOURCE_INT,                                \
        .callback = slider_##chip##PALOddLineOffset_callback,               \
        .data     = (ui_callback_data_t)"Set PAL oddline offset (0-2000)"   \
    },                                                                      \
    {   .string   = "U-only Delayline",                                     \
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,                             \
        .callback = toggle_##chip##PALDelaylineType_callback                \
    }

static const ui_menu_entry_t vic_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_PALNTSC_MENU_ITEMS(VIC),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vicii_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_PALNTSC_MENU_ITEMS(VICII),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t vdc_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_MENU_ITEMS(VDC),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t ted_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_PALNTSC_MENU_ITEMS(TED),
    SDL_MENU_LIST_END
};

static const ui_menu_entry_t crtc_crt_controls_menu[] = {
    VICE_SDL_CRTEMU_MENU_ITEMS(Crtc),
    SDL_MENU_LIST_END
};


/* Color menu */

UI_MENU_DEFINE_SLIDER_VIDEO(VICColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICColorBrightness, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(VICIIColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VICIIColorBrightness, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(VDCColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER_VIDEO(VDCColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VDCColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VDCColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(VDCColorBrightness, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(TEDColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(TEDColorBrightness, 0, 2000)

UI_MENU_DEFINE_SLIDER_VIDEO(CrtcColorGamma, 0, 4000)
UI_MENU_DEFINE_SLIDER_VIDEO(CrtcColorTint, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(CrtcColorSaturation, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(CrtcColorContrast, 0, 2000)
UI_MENU_DEFINE_SLIDER_VIDEO(CrtcColorBrightness, 0, 2000)

#define VICE_SDL_COLOR_MENU_ITEMS(chip)                                     \
    {   .string   = "Gamma",                                                \
        .type     = MENU_ENTRY_RESOURCE_INT,                                \
        .callback = slider_##chip##ColorGamma_callback,                     \
        .data     = (ui_callback_data_t)"Set gamma (0-4000)"                \
    },                                                                      \
    {   .string   = "Tint",                                                 \
        .type     = MENU_ENTRY_RESOURCE_INT,                                \
        .callback = slider_##chip##ColorTint_callback,                      \
        .data     = (ui_callback_data_t)"Set tint (0-2000)"                 \
    },                                                                      \
    {   .string   = "Saturation",                                           \
        .type     = MENU_ENTRY_RESOURCE_INT,                                \
        .callback = slider_##chip##ColorSaturation_callback,                \
        .data     = (ui_callback_data_t)"Set saturation (0-2000)"           \
    },                                                                      \
    {   .string   = "Contrast",                                             \
        .type     = MENU_ENTRY_RESOURCE_INT,                                \
        .callback = slider_##chip##ColorContrast_callback,                  \
        .data     = (ui_callback_data_t)"Set contrast (0-2000)"             \
    },                                                                      \
    {   .string   = "Brightness",                                           \
        .type     = MENU_ENTRY_RESOURCE_INT,                                \
        .callback = slider_##chip##ColorBrightness_callback,                \
        .data     = (ui_callback_data_t)"Set brightness (0-2000)"           \
    }

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

UI_MENU_DEFINE_INT(TEDFullscreenCustomWidth)
UI_MENU_DEFINE_INT(TEDFullscreenCustomHeight)
UI_MENU_DEFINE_INT(VICFullscreenCustomWidth)
UI_MENU_DEFINE_INT(VICFullscreenCustomHeight)
UI_MENU_DEFINE_INT(VDCFullscreenCustomWidth)
UI_MENU_DEFINE_INT(VDCFullscreenCustomHeight)
UI_MENU_DEFINE_INT(VICIIFullscreenCustomWidth)
UI_MENU_DEFINE_INT(VICIIFullscreenCustomHeight)
UI_MENU_DEFINE_INT(CrtcFullscreenCustomWidth)
UI_MENU_DEFINE_INT(CrtcFullscreenCustomHeight)

#ifndef USE_SDL2UI
UI_MENU_DEFINE_RADIO(SDLLimitMode)
#endif
UI_MENU_DEFINE_INT(Window0Width)
UI_MENU_DEFINE_INT(Window0Height)
UI_MENU_DEFINE_TOGGLE(StartMinimized)
UI_MENU_DEFINE_TOGGLE(StartMaximized)

#define VICE_SDL_SIZE_MENU_DOUBLESIZE(chip)                         \
    {   .string   = "Double size",                                  \
        .type     =  MENU_ENTRY_RESOURCE_TOGGLE,                    \
        .callback = toggle_##chip##DoubleSize_callback              \
    },

#define VICE_SDL_SIZE_MENU_STRETCHVERTICAL(chip)                    \
    {   .string   = "Stretch vertically",                           \
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,                     \
        .callback = toggle_##chip##StretchVertical_callback         \
    },

#define VICE_SDL_SIZE_MENU_ITEMS_SHARED(chip)                       \
    {   .string   = "Double scan",                                  \
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,                     \
        .callback = toggle_##chip##DoubleScan_callback              \
    },                                                              \
    {   .action   = ACTION_FULLSCREEN_TOGGLE,                       \
        .string   = "Fullscreen",                                   \
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,                     \
        .resource = #chip"Fullscreen"                               \
    },                                                              \
    SDL_MENU_ITEM_SEPARATOR,                                        \
    SDL_MENU_ITEM_TITLE("Fullscreen mode"),                         \
    {   .string   = "Automatic",                                    \
        .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
        .callback = radio_##chip##FullscreenMode_callback,          \
        .data     = (ui_callback_data_t)FULLSCREEN_MODE_AUTO        \
    },                                                              \
    {   .string   = "Custom",                                       \
        .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
        .callback = radio_##chip##FullscreenMode_callback,          \
        .data     = (ui_callback_data_t)FULLSCREEN_MODE_CUSTOM      \
    },                                                              \
    SDL_MENU_ITEM_SEPARATOR,                                        \
    SDL_MENU_ITEM_TITLE("Custom resolution"),                       \
    {   .string   = "Width",                                        \
        .type     = MENU_ENTRY_RESOURCE_INT,                        \
        .callback = int_##chip##FullscreenCustomWidth_callback,     \
        .data     = (ui_callback_data_t)"Set width"                 \
    },                                                              \
    {   .string   = "Height",                                       \
        .type     = MENU_ENTRY_RESOURCE_INT,                        \
        .callback = int_##chip##FullscreenCustomHeight_callback,    \
        .data     = (ui_callback_data_t)"Set height"                \
    },
#ifndef USE_SDL2UI
#define VICE_SDL_SIZE_MENU_ITEMS_LIMIT(chip)                        \
    SDL_MENU_ITEM_SEPARATOR,                                        \
    SDL_MENU_ITEM_TITLE("Resolution limit mode"),                   \
    {   .string   = "Off",                                          \
        .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
        .callback = radio_SDLLimitMode_callback,                    \
        .data     = (ui_callback_data_t)SDL_LIMIT_MODE_OFF          \
    },                                                              \
    {   .string   = "Max",                                          \
        .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
        .callback = radio_SDLLimitMode_callback,                    \
        .data     = (ui_callback_data_t)SDL_LIMIT_MODE_MAX          \
    },                                                              \
    {   .string   ="Fixed",                                         \
        .type     = MENU_ENTRY_RESOURCE_RADIO,                      \
        .callback = radio_SDLLimitMode_callback,                    \
        .data     = (ui_callback_data_t)SDL_LIMIT_MODE_FIXED        \
    },
#else
#define VICE_SDL_SIZE_MENU_ITEMS_LIMIT(chip)
#endif
#define VICE_SDL_SIZE_MENU_ITEMS_LATER_SHARED(chip)                 \
    SDL_MENU_ITEM_SEPARATOR,                                        \
    SDL_MENU_ITEM_TITLE("Initial resolution"),                      \
    {   .string   = "Width",                                        \
        .type     = MENU_ENTRY_RESOURCE_INT,                        \
        .callback = int_Window0Width_callback,                      \
        .data     = (ui_callback_data_t)"Set width"                 \
    },                                                              \
    {   .string   = "Height",                                       \
        .type     = MENU_ENTRY_RESOURCE_INT,                        \
        .callback = int_Window0Height_callback,                     \
        .data     = (ui_callback_data_t)"Set height"                \
    },

#define VICE_SDL_SIZE_MENU_ITEMS(chip)              \
VICE_SDL_SIZE_MENU_ITEMS_SHARED(chip)               \
VICE_SDL_SIZE_MENU_ITEMS_LIMIT(chip)                \
VICE_SDL_SIZE_MENU_ITEMS_LATER_SHARED(chip)

#if defined(HAVE_HWSCALE) || defined(USE_SDL2UI)

UI_MENU_DEFINE_RADIO(VICIIGLFilter)
UI_MENU_DEFINE_RADIO(TEDGLFilter)
UI_MENU_DEFINE_RADIO(VICGLFilter)
UI_MENU_DEFINE_RADIO(VDCGLFilter)
UI_MENU_DEFINE_RADIO(CrtcGLFilter)

#define VICE_SDL_VIDEO_FILTERS(chip)                                    \
static const ui_menu_entry_t chip##filter_menu[] = {                    \
    {   .string   = "Nearest",                                          \
        .type     = MENU_ENTRY_RESOURCE_RADIO,                          \
        .callback = radio_##chip##GLFilter_callback,                    \
        .data     = (ui_callback_data_t)VIDEO_GLFILTER_NEAREST          \
    },                                                                  \
    {   .string   = "Linear",                                           \
        .type     = MENU_ENTRY_RESOURCE_RADIO,                          \
        .callback = radio_##chip##GLFilter_callback,                    \
        .data     = (ui_callback_data_t)VIDEO_GLFILTER_BILINEAR         \
    },                                                                  \
    SDL_MENU_LIST_END                                                   \
};

VICE_SDL_VIDEO_FILTERS(VICII)
VICE_SDL_VIDEO_FILTERS(TED)
VICE_SDL_VIDEO_FILTERS(VIC)
VICE_SDL_VIDEO_FILTERS(VDC)
VICE_SDL_VIDEO_FILTERS(Crtc)

UI_MENU_DEFINE_TOGGLE(VICIIFlipX)
UI_MENU_DEFINE_TOGGLE(TEDFlipX)
UI_MENU_DEFINE_TOGGLE(VICFlipX)
UI_MENU_DEFINE_TOGGLE(VDCFlipX)
UI_MENU_DEFINE_TOGGLE(CrtcFlipX)

UI_MENU_DEFINE_TOGGLE(VICIIFlipY)
UI_MENU_DEFINE_TOGGLE(TEDFlipY)
UI_MENU_DEFINE_TOGGLE(VICFlipY)
UI_MENU_DEFINE_TOGGLE(VDCFlipY)
UI_MENU_DEFINE_TOGGLE(CrtcFlipY)

UI_MENU_DEFINE_TOGGLE(VICIIRotate)
UI_MENU_DEFINE_TOGGLE(TEDRotate)
UI_MENU_DEFINE_TOGGLE(VICRotate)
UI_MENU_DEFINE_TOGGLE(VDCRotate)
UI_MENU_DEFINE_TOGGLE(CrtcRotate)

UI_MENU_DEFINE_TOGGLE(VICIIVSync)
UI_MENU_DEFINE_TOGGLE(TEDVSync)
UI_MENU_DEFINE_TOGGLE(VICVSync)
UI_MENU_DEFINE_TOGGLE(VDCVSync)
UI_MENU_DEFINE_TOGGLE(CrtcVSync)

#define VICE_SDL_SIZE_MENU_OPENGL_ITEMS(chip)               \
    SDL_MENU_ITEM_SEPARATOR,                                \
    SDL_MENU_ITEM_TITLE("OpenGL"),                          \
    {   .string   = "VSync",                                \
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,             \
        .callback = toggle_##chip##VSync_callback,          \
    },                                                      \
    {   .string   = "GL Filter",                            \
        .type     = MENU_ENTRY_SUBMENU,                     \
        .callback = submenu_radio_callback,                 \
        .data     = (ui_callback_data_t)chip##filter_menu   \
    },                                                      \
    {   .string   = "Flip X",                               \
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,             \
        .callback = toggle_##chip##FlipX_callback,          \
    },                                                      \
    {   .string   = "Flip Y",                               \
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,             \
        .callback = toggle_##chip##FlipY_callback,          \
    },                                                      \
    {   .string   = "Rotate 90degr",                        \
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,             \
        .callback = toggle_##chip##Rotate_callback,         \
    },

#endif  /* defined(HAVE_HWSCALE) || defined(USE_SDL2UI) */

#define VICE_SDL_SIZE_MENU_ASPECT(chip)                                 \
    SDL_MENU_ITEM_SEPARATOR,                                            \
    SDL_MENU_ITEM_TITLE("Aspect Ratio Mode"),                           \
    {   .string   = "off",                                              \
        .type     = MENU_ENTRY_RESOURCE_RADIO,                          \
        .callback = radio_##chip##AspectMode_callback,                  \
        .data     = (ui_callback_data_t)VIDEO_ASPECT_MODE_NONE          \
    },                                                                  \
    {   .string   = "custom",                                           \
        .type     = MENU_ENTRY_RESOURCE_RADIO,                          \
        .callback = radio_##chip##AspectMode_callback,                  \
        .data     = (ui_callback_data_t)VIDEO_ASPECT_MODE_CUSTOM        \
    },                                                                  \
    {   .string   = "true",                                             \
        .type     = MENU_ENTRY_RESOURCE_RADIO,                          \
        .callback = radio_##chip##AspectMode_callback,                  \
        .data     = (ui_callback_data_t)VIDEO_ASPECT_MODE_TRUE          \
    },                                                                  \
    {   .string   = "Custom aspect ratio",                              \
        .type     = MENU_ENTRY_RESOURCE_STRING,                         \
        .callback = string_##chip##AspectRatio_callback,                \
        .data     = (ui_callback_data_t)"Set aspect ratio (0.5 - 2.0)"  \
    },

/* VICII size menu */

UI_MENU_DEFINE_TOGGLE(VICIIDoubleSize)
UI_MENU_DEFINE_TOGGLE(VICIIDoubleScan)

#if defined(HAVE_HWSCALE) || defined(USE_SDL2UI)
UI_MENU_DEFINE_RADIO(VICIIAspectMode)
UI_MENU_DEFINE_STRING(VICIIAspectRatio)
#endif

UI_MENU_DEFINE_RADIO(VICIIFullscreenMode)

static const ui_menu_entry_t vicii_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(VICII)
    VICE_SDL_SIZE_MENU_ITEMS(VICII)
#if defined(HAVE_HWSCALE) || defined(USE_SDL2UI)
    VICE_SDL_SIZE_MENU_ASPECT(VICII)
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(VICII)
#endif
    SDL_MENU_LIST_END
};


/* VDC size menu */

UI_MENU_DEFINE_TOGGLE(VDCDoubleSize)
UI_MENU_DEFINE_TOGGLE(VDCStretchVertical)

#if defined(HAVE_HWSCALE) || defined(USE_SDL2UI)
UI_MENU_DEFINE_RADIO(VDCAspectMode)
UI_MENU_DEFINE_STRING(VDCAspectRatio)
#endif

UI_MENU_DEFINE_TOGGLE(VDCDoubleScan)
UI_MENU_DEFINE_RADIO(VDCFullscreenMode)

static const ui_menu_entry_t vdc_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(VDC)
    VICE_SDL_SIZE_MENU_STRETCHVERTICAL(VDC)
    VICE_SDL_SIZE_MENU_ITEMS(VDC)
#if defined(HAVE_HWSCALE) || defined(USE_SDL2UI)
    VICE_SDL_SIZE_MENU_ASPECT(VDC)
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(VDC)
#endif
    SDL_MENU_LIST_END
};


/* Crtc size menu */

UI_MENU_DEFINE_RADIO(CrtcFullscreenMode)
UI_MENU_DEFINE_TOGGLE(CrtcDoubleSize)
UI_MENU_DEFINE_TOGGLE(CrtcStretchVertical)
UI_MENU_DEFINE_TOGGLE(CrtcDoubleScan)

#if defined(HAVE_HWSCALE) || defined(USE_SDL2UI)
UI_MENU_DEFINE_RADIO(CrtcAspectMode)
UI_MENU_DEFINE_STRING(CrtcAspectRatio)
#endif

static const ui_menu_entry_t crtc_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(Crtc)
    VICE_SDL_SIZE_MENU_STRETCHVERTICAL(Crtc)
    VICE_SDL_SIZE_MENU_ITEMS(Crtc)
#if defined(HAVE_HWSCALE) || defined(USE_SDL2UI)
    VICE_SDL_SIZE_MENU_ASPECT(Crtc)
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(Crtc)
#endif
    SDL_MENU_LIST_END
};


/* TED size menu */

UI_MENU_DEFINE_TOGGLE(TEDDoubleSize)
UI_MENU_DEFINE_TOGGLE(TEDDoubleScan)

#if defined(HAVE_HWSCALE) || defined(USE_SDL2UI)
UI_MENU_DEFINE_RADIO(TEDAspectMode)
UI_MENU_DEFINE_STRING(TEDAspectRatio)
#endif

UI_MENU_DEFINE_RADIO(TEDFullscreenMode)

static const ui_menu_entry_t ted_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(TED)
    VICE_SDL_SIZE_MENU_ITEMS(TED)
#if defined(HAVE_HWSCALE) || defined(USE_SDL2UI)
    VICE_SDL_SIZE_MENU_ASPECT(TED)
    VICE_SDL_SIZE_MENU_OPENGL_ITEMS(TED)
#endif
    SDL_MENU_LIST_END
};


/* VIC size menu */

UI_MENU_DEFINE_TOGGLE(VICDoubleSize)
UI_MENU_DEFINE_TOGGLE(VICDoubleScan)

#if defined(HAVE_HWSCALE) || defined(USE_SDL2UI)
UI_MENU_DEFINE_RADIO(VICAspectMode)
UI_MENU_DEFINE_STRING(VICAspectRatio)
#endif

UI_MENU_DEFINE_RADIO(VICFullscreenMode)

static const ui_menu_entry_t vic_size_menu[] = {
    VICE_SDL_SIZE_MENU_DOUBLESIZE(VIC)
    VICE_SDL_SIZE_MENU_ITEMS(VIC)
#if defined(HAVE_HWSCALE) || defined(USE_SDL2UI)
    VICE_SDL_SIZE_MENU_ASPECT(VIC)
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

#define VICE_SDL_FILTER_MENU_ITEMS(chip)                    \
    {   .string    = "None",                                \
        .type     = MENU_ENTRY_RESOURCE_RADIO,              \
        .callback = radio_##chip##Filter_callback,          \
        .data     = (ui_callback_data_t)VIDEO_FILTER_NONE   \
    },                                                      \
    {   .string   = "CRT Emulation",                        \
        .type     = MENU_ENTRY_RESOURCE_RADIO,              \
        .callback = radio_##chip##Filter_callback,          \
        .data     = (ui_callback_data_t)VIDEO_FILTER_CRT    \
    }

#define VICE_SDL_FILTER_MENU_SCALE2X_ITEMS(chip)                \
    {   .string   = "Scale2x",                                  \
        .type     = MENU_ENTRY_RESOURCE_RADIO,                  \
        .callback = radio_##chip##Filter_callback,              \
        .data     = (ui_callback_data_t)VIDEO_FILTER_SCALE2X    \
    }

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
UI_MENU_DEFINE_TOGGLE(VICIICheckSsColl)
UI_MENU_DEFINE_TOGGLE(VICIICheckSbColl)
UI_MENU_DEFINE_FILE_STRING(VICIIPaletteFile)
UI_MENU_DEFINE_FILE_STRING(VDCPaletteFile)
UI_MENU_DEFINE_FILE_STRING(CrtcPaletteFile)
UI_MENU_DEFINE_FILE_STRING(TEDPaletteFile)
UI_MENU_DEFINE_FILE_STRING(VICPaletteFile)


/* C128 video menu */
#define VIDEO_OUTPUT_VICII         0
#define VIDEO_OUTPUT_VDC           1
#define VIDEO_OUTPUT_DUAL_WINDOW   2

#ifndef USE_SDL2UI
static UI_MENU_CALLBACK(radio_VideoOutput_c128_callback)
{
    int value = vice_ptr_to_int(param);

    if (activated) {
        sdl_video_canvas_switch(value);
    } else {
        if (value == sdl_active_canvas->index) {
            return sdl_menu_text_tick;
        }
    }
    return NULL;
}
#else
static UI_MENU_CALLBACK(radio_VideoOutput_c128_callback)
{
    int value = vice_ptr_to_int(param);
    int dual_window = 0;

    resources_get_int("DualWindow", &dual_window);

    if (activated) {
        if (value == VIDEO_OUTPUT_VICII || value == VIDEO_OUTPUT_VDC) {
            sdl_video_canvas_switch(value);
            resources_set_int("DualWindow", 0);
            sdl2_hide_second_window();
        } else if (value == VIDEO_OUTPUT_DUAL_WINDOW) {
            resources_set_int("DualWindow", 1);
            sdl2_show_second_window();
        }
    } else {
        if ((value == VIDEO_OUTPUT_DUAL_WINDOW) && (dual_window == 1)) {
            return sdl_menu_text_tick;
        } else if (dual_window != 1) {
            if (value == sdl_active_canvas->index) {
                return sdl_menu_text_tick;
            }
        }
    }
    return NULL;
}
#endif /* USE_SDL2UI */

const ui_menu_entry_t c128_video_menu[] = {
    SDL_MENU_ITEM_TITLE("Video output"),
    {   .string   = "VICII (40 cols)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VideoOutput_c128_callback,
        .data     = (ui_callback_data_t)VIDEO_OUTPUT_VICII
    },
    {   .string   = "VDC (80 cols)",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VideoOutput_c128_callback,
        .data     = (ui_callback_data_t)VIDEO_OUTPUT_VDC
    },
#ifdef USE_SDL2UI
    {   .string   = "Dual Windows",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_VideoOutput_c128_callback,
        .data     = (ui_callback_data_t)VIDEO_OUTPUT_DUAL_WINDOW
    },
#endif
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VICII border mode",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vicii_border_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VICII host rendering settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_size_menu
    },
    {   .string   = "VDC host rendering settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vdc_size_menu
    },
    {   .action   = ACTION_RESTORE_DISPLAY,
        .string   = "Restore window size",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .string   = "Start with minimized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMinimized_callback,
    },
    {   .string   = "Start with maximized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMaximized_callback,
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VICII Video cache",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIIVideoCache_callback
    },
    {   .string   = "VICII Color controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_color_controls_menu
    },
    {   .string   = "VICII CRT emulation controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_crt_controls_menu
    },
    {   .string   = "VICII render filter",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vicii_filter_menu
    },
    {   .string   = "VICII colors",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)palette_menu1
    },
    {   .string   = "VICII Audio Leak emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIIAudioLeak_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VDC Video cache",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VDCVideoCache_callback
    },
    {   .string   = "VDC Color controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vdc_color_controls_menu
    },
    {   .string   = "VDC CRT emulation controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vdc_crt_controls_menu
    },
    {   .string   = "VDC render filter",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vdc_filter_menu
    },
    {   .string   = "VDC colors",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)palette_menu2
    },
    {   .string   = "VDC Audio Leak emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VDCAudioLeak_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Video Standard"),
    {   .string   = "PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineVideoStandard_callback,
        .data     = (ui_callback_data_t)MACHINE_SYNC_PAL
    },
    {   .string   = "NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineVideoStandard_callback,
        .data     = (ui_callback_data_t)MACHINE_SYNC_NTSC
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Enable Sprite-Sprite collisions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIICheckSsColl_callback
    },
    {   .string   = "Enable Sprite-Background collisions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIICheckSbColl_callback
    },
    SDL_MENU_LIST_END
};


/* C64 video menu */

const ui_menu_entry_t c64_video_menu[] = {
    {   .string   = "Host rendering settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_size_menu
    },
    {   .action   = ACTION_RESTORE_DISPLAY,
        .string   = "Restore window size",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .string   = "Start with minimized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMinimized_callback,
    },
    {   .string   = "Start with maximized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMaximized_callback,
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Video cache",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIIVideoCache_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VICII border mode",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vicii_border_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Color controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_color_controls_menu
    },
    {   .string   = "CRT emulation controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_crt_controls_menu
    },
    {   .string   = "Render filter",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vicii_filter_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VICII colors",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)palette_menu1
    },
    {   .string   = "VICII Audio Leak emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIIAudioLeak_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Enable Sprite-Sprite collisions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIICheckSsColl_callback
    },
    {   .string   = "Enable Sprite-Background collisions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIICheckSbColl_callback
    },
    SDL_MENU_LIST_END
};


/* C64SC video menu */

const ui_menu_entry_t c64sc_video_menu[] = {
    {   .string   = "Host rendering settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_size_menu
    },
    {   .action   = ACTION_RESTORE_DISPLAY,
        .string   = "Restore window size",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .string   = "Start with minimized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMinimized_callback,
    },
    {   .string   = "Start with maximized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMaximized_callback,
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VICII border mode",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vicii_border_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Color controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_color_controls_menu
    },
    {   .string   = "CRT emulation controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_crt_controls_menu
    },
    {   .string   = "Render filter",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vicii_filter_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VICII colors",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)palette_menu1
    },
    {   .string   = "VICII Audio Leak emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIIAudioLeak_callback
    },
    {   .string   = "VICII VSP-bug emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIIVSPBug_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Enable Sprite-Sprite collisions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIICheckSsColl_callback
    },
    {   .string   = "Enable Sprite-Background collisions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIICheckSbColl_callback
    },
    SDL_MENU_LIST_END
};


/* C64DTV video menu */

const ui_menu_entry_t c64dtv_video_menu[] = {
    {   .string   = "Host rendering settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_size_menu
    },
    {   .action   = ACTION_RESTORE_DISPLAY,
        .string   = "Restore window size",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .string   = "Start with minimized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMinimized_callback,
    },
    {   .string   = "Start with maximized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMaximized_callback,
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Video cache",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIIVideoCache_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VICII border mode",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vicii_border_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Color controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_color_controls_menu
    },
    {   .string   = "CRT emulation controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_crt_controls_menu
    },
    {   .string   = "Render filter",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vicii_filter_menu
    },
    {   .string   = "VICII colors",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)palette_menu1
    },
    {   .string   = "VICII Audio Leak emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIIAudioLeak_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Video Standard"),
    {   .string   = "PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineVideoStandard_callback,
        .data     = (ui_callback_data_t)MACHINE_SYNC_PAL
    },
    {   .string   = "NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineVideoStandard_callback,
        .data     = (ui_callback_data_t)MACHINE_SYNC_NTSC
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Enable Sprite-Sprite collisions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIICheckSsColl_callback
    },
    {   .string   = "Enable Sprite-Background collisions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIICheckSbColl_callback
    },
    SDL_MENU_LIST_END
};

/* CBM-II 5x0 video menu */

const ui_menu_entry_t cbm5x0_video_menu[] = {
    {   .string   = "Host rendering settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_size_menu
    },
    {   .action   = ACTION_RESTORE_DISPLAY,
        .string   = "Restore window size",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .string   = "Start with minimized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMinimized_callback,
    },
    {   .string   = "Start with maximized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMaximized_callback,
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Video cache",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIIVideoCache_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VICII border mode",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vicii_border_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Color controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_color_controls_menu
    },
    {   .string   = "CRT emulation controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vicii_crt_controls_menu
    },
    {   .string   = "Render filter",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vicii_filter_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VICII colors",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)palette_menu1
    },
    {   .string   = "VICII Audio Leak emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIIAudioLeak_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Video Standard"),
    {   .string   = "PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineVideoStandard_callback,
        .data     = (ui_callback_data_t)MACHINE_SYNC_PAL
    },
    {   .string   = "NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineVideoStandard_callback,
        .data     = (ui_callback_data_t)MACHINE_SYNC_NTSC
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Enable Sprite-Sprite collisions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIICheckSsColl_callback
    },
    {   .string   = "Enable Sprite-Background collisions",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICIICheckSbColl_callback
    },
    SDL_MENU_LIST_END
};


/* CBM-II 6x0/7x0 video menu */

const ui_menu_entry_t cbm6x0_7x0_video_menu[] = {
    {   .string   = "Host rendering settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)crtc_size_menu
    },
    {   .action   = ACTION_RESTORE_DISPLAY,
        .string   = "Restore window size",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .string   = "Start with minimized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMinimized_callback,
    },
    {   .string   = "Start with maximized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMaximized_callback,
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Video cache",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_CrtcVideoCache_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Color controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)crtc_color_controls_menu
    },
    {   .string   = "CRT emulation controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)crtc_crt_controls_menu
    },
    {   .string   = "Render filter",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)crtc_filter_menu
    },
    {   .string   = "CRTC colors",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)palette_menu1
    },
    {   .string   = "CRTC Audio Leak emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_CrtcAudioLeak_callback
    },
    SDL_MENU_LIST_END
};


/* PET video menu */

const ui_menu_entry_t pet_video_menu[] = {
    {   .string   = "Host rendering settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)crtc_size_menu
    },
    {   .action   = ACTION_RESTORE_DISPLAY,
        .string   = "Restore window size",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .string   = "Start with minimized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMinimized_callback,
    },
    {   .string   = "Start with maximized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMaximized_callback,
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Video cache",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_CrtcVideoCache_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Color controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)crtc_color_controls_menu
    },
    {   .string   = "CRT emulation controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)crtc_crt_controls_menu
    },
    {   .string   = "Render filter",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)crtc_filter_menu
    },
    {   .string   = "CRTC colors",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)palette_menu1
    },
    {   .string   = "CRTC Audio Leak emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_CrtcAudioLeak_callback
    },
    SDL_MENU_LIST_END
};


/* PLUS4 video menu */

const ui_menu_entry_t plus4_video_menu[] = {
    {   .string   = "Host rendering settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ted_size_menu
    },
    {   .action   = ACTION_RESTORE_DISPLAY,
        .string   = "Restore window size",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .string   = "Start with minimized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMinimized_callback,
    },
    {   .string   = "Start with maximized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMaximized_callback,
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Video cache",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_TEDVideoCache_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "TED border mode",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)ted_border_menu
    },
    {   .string   = "Color controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ted_color_controls_menu
    },
    {   .string   = "CRT emulation controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)ted_crt_controls_menu
    },
    {   .string   = "Render filter",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)ted_filter_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "TED colors",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)palette_menu1
    },
    {   .string   = "TED Audio Leak emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_TEDAudioLeak_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Video Standard"),
    {   .string   = "PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineVideoStandard_callback,
        .data     = (ui_callback_data_t)MACHINE_SYNC_PAL
    },
    {   .string   = "NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineVideoStandard_callback,
        .data     = (ui_callback_data_t)MACHINE_SYNC_NTSC
    },
    SDL_MENU_LIST_END
};


/* VIC-20 video menu */

static UI_MENU_CALLBACK(radio_MachineVideoStandard_vic20_callback)
{
    if (activated) {
        int value = vice_ptr_to_int(param);
        resources_set_int("MachineVideoStandard", value);
        machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);
        return sdl_menu_text_exit_ui;
    }
    return sdl_ui_menu_radio_helper(activated, param, "MachineVideoStandard");
}

const ui_menu_entry_t vic20_video_menu[] = {
    {   .string   = "Host rendering settings",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vic_size_menu
    },
    {   .action   = ACTION_RESTORE_DISPLAY,
        .string   = "Restore window size",
        .type     = MENU_ENTRY_OTHER,
    },
    {   .string   = "Start with minimized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMinimized_callback,
    },
    {   .string   = "Start with maximized window",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_StartMaximized_callback,
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "Video cache",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICVideoCache_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VIC border mode",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vic_border_menu
    },
    {   .string   = "Color controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vic_color_controls_menu
    },
    {   .string   = "CRT emulation controls",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)vic_crt_controls_menu
    },
    {   .string   = "Render filter",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_radio_callback,
        .data     = (ui_callback_data_t)vic_filter_menu
    },
    SDL_MENU_ITEM_SEPARATOR,

    {   .string   = "VIC colors",
        .type     = MENU_ENTRY_SUBMENU,
        .callback = submenu_callback,
        .data     = (ui_callback_data_t)palette_menu1
    },
    {   .string   = "VIC Audio Leak emulation",
        .type     = MENU_ENTRY_RESOURCE_TOGGLE,
        .callback = toggle_VICAudioLeak_callback
    },
    SDL_MENU_ITEM_SEPARATOR,

    SDL_MENU_ITEM_TITLE("Video Standard"),
    {   .string   = "PAL",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineVideoStandard_vic20_callback,
        .data     = (ui_callback_data_t)MACHINE_SYNC_PAL
    },
    {   .string   = "NTSC",
        .type     = MENU_ENTRY_RESOURCE_RADIO,
        .callback = radio_MachineVideoStandard_vic20_callback,
        .data     = (ui_callback_data_t)MACHINE_SYNC_NTSC
    },
    SDL_MENU_LIST_END
};

/* static int palette_dyn_menu_init = 0; */

static char *video_chip1_used = NULL;
static char *video_chip2_used = NULL;

static UI_MENU_CALLBACK(external_palette_file1_callback)
{
    const char *external_file_name;
    char *name = (char *)param;

    if (activated) {
        resources_set_string_sprintf("%sPaletteFile", name, video_chip1_used);
    } else {
        resources_get_string_sprintf("%sPaletteFile", &external_file_name, video_chip1_used);
        if (external_file_name) {
            if (!strcmp(external_file_name, name)) {
                return MENU_CHECKMARK_CHECKED_STRING;
            }
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(external_palette_file2_callback)
{
    const char *external_file_name;
    char *name = (char *)param;

    if (activated) {
        resources_set_string_sprintf("%sPaletteFile", name, video_chip2_used);
    } else {
        resources_get_string_sprintf("%sPaletteFile", &external_file_name, video_chip2_used);
        if (external_file_name) {
            if (!strcmp(external_file_name, name)) {
                return MENU_CHECKMARK_CHECKED_STRING;
            }
        }
    }
    return NULL;
}

static int countgroup(const palette_info_t *palettelist, char *chip)
{
    int num = 0;

    while (palettelist->name) {
        if (palettelist->chip && !strcmp(palettelist->chip, chip)) {
            ++num;
        }
        ++palettelist;
    }
    return num;
}

typedef struct name2func_s {
    const char *name;
    const char *(*toggle_func)(int activated, ui_callback_data_t param);
    const char *(*file_func)(int activated, ui_callback_data_t param);
} name2func_t;

static const name2func_t name2func[] = {
    { "VICII",  toggle_VICIIExternalPalette_callback,   file_string_VICIIPaletteFile_callback },
    { "VDC",    toggle_VDCExternalPalette_callback,     file_string_VDCPaletteFile_callback },
    { "Crtc",   toggle_CrtcExternalPalette_callback,    file_string_CrtcPaletteFile_callback },
    { "TED",    toggle_TEDExternalPalette_callback,     file_string_TEDPaletteFile_callback },
    { "VIC",    toggle_VICExternalPalette_callback,     file_string_VICPaletteFile_callback },
    { NULL, NULL, NULL }
};

void uipalette_menu_create(char *chip1_name, char *chip2_name)
{
    int num;
    const palette_info_t *palettelist = palette_get_info_list();
    int i;
    const char *(*toggle_func1)(int activated, ui_callback_data_t param) = NULL;
    const char *(*file_func1)(int activated, ui_callback_data_t param) = NULL;
    const char *(*toggle_func2)(int activated, ui_callback_data_t param) = NULL;
    const char *(*file_func2)(int activated, ui_callback_data_t param) = NULL;

    video_chip1_used = chip1_name;
    video_chip2_used = chip2_name;

    for (i = 0; name2func[i].name; ++i) {
        if (!strcmp(video_chip1_used, name2func[i].name)) {
            toggle_func1 = name2func[i].toggle_func;
            file_func1 = name2func[i].file_func;
        }
    }

    if (video_chip2_used) {
        for (i = 0; name2func[i].name; ++i) {
            if (!strcmp(video_chip2_used, name2func[i].name)) {
                toggle_func2 = name2func[i].toggle_func;
                file_func2 = name2func[i].file_func;
            }
        }
    }

    num = countgroup(palettelist, video_chip1_used);

    palette_dyn_menu1 = lib_malloc(sizeof(ui_menu_entry_t) * (num + 1));

    i = 0;

    while (palettelist->name) {
        if (palettelist->chip && !strcmp(palettelist->chip, video_chip1_used)) {
            const char *current;
            char *current2;
            const char *ext;
            palette_dyn_menu1[i].action   = ACTION_NONE;
            palette_dyn_menu1[i].string   = lib_strdup(palettelist->name);
            palette_dyn_menu1[i].type     = MENU_ENTRY_OTHER_TOGGLE;
            palette_dyn_menu1[i].callback = external_palette_file1_callback;
            palette_dyn_menu1[i].data     = NULL;
            resources_get_string_sprintf("%sPaletteFile", &current, video_chip1_used);
            if (current) {
                /* if the <CHIP>PaletteFile resource points to a file,
                - create an alternative name without extension, if the file has a
                    .vpl extension
                - create an alternative name with .vpl extension, if the file has no
                    extension
                */
                ext = util_get_extension(current);
                if ((ext != NULL) && (strcmp(ext, "vpl") == 0)) {
                    /* current has .vpl extension, so make sure the menu entry has it */
                    if ((ext = util_get_extension(palettelist->file)) == NULL) {
                        current2 = lib_strdup(palettelist->file);
                        util_add_extension(&current2, "vpl");
                        palette_dyn_menu1[i].data = current2;
                    }
                } else if (ext == NULL) {
                    /* current has no extension, so remove it from menu entry too */
                    ext = util_get_extension(palettelist->file);
                    if ((ext != NULL) && (strcmp(ext, "vpl") == 0)) {
                        size_t n = strlen(palettelist->file);
                        current2 = lib_strdup(palettelist->file);
                        current2[n - 4] = 0;
                        palette_dyn_menu1[i].data = current2;
                    }
                }
            }
            if (palette_dyn_menu1[i].data == NULL) {
                palette_dyn_menu1[i].data = (ui_callback_data_t)lib_strdup(palettelist->file);
            }
            ++i;
        }
        ++palettelist;
    }
    palette_dyn_menu1[i].string   = NULL;

    if (video_chip2_used) {
        palettelist = palette_get_info_list();
        num = countgroup(palettelist, video_chip2_used);
        palette_dyn_menu2 = lib_malloc(sizeof(ui_menu_entry_t) * (num + 1));

        i = 0;

        while (palettelist->name) {
            if (palettelist->chip && !strcmp(palettelist->chip, video_chip2_used)) {
                const char *current;
                char *current2;
                const char *ext;
                palette_dyn_menu2[i].action   = ACTION_NONE;
                palette_dyn_menu2[i].string   = lib_strdup(palettelist->name);
                palette_dyn_menu2[i].type     = MENU_ENTRY_OTHER_TOGGLE;
                palette_dyn_menu2[i].callback = external_palette_file2_callback;
                palette_dyn_menu2[i].data     = NULL;
                resources_get_string_sprintf("%sPaletteFile", &current, video_chip2_used);
                if (current) {
                    /* if the <CHIP>PaletteFile resource points to a file,
                    - create an alternative name without extension, if the file has a
                        .vpl extension
                    - create an alternative name with .vpl extension, if the file has no
                        extension
                    */
                    ext = util_get_extension(current);
                    if ((ext != NULL) && (strcmp(ext, "vpl") == 0)) {
                        /* current has .vpl extension, so make sure the menu entry has it */
                        if ((ext = util_get_extension(palettelist->file)) == NULL) {
                            current2 = lib_strdup(palettelist->file);
                            util_add_extension(&current2, "vpl");
                            palette_dyn_menu2[i].data = current2;
                        }
                    } else if (ext == NULL) {
                        /* current has no extension, so remove it from menu entry too */
                        ext = util_get_extension(palettelist->file);
                        if ((ext != NULL) && (strcmp(ext, "vpl") == 0)) {
                            size_t n = strlen(palettelist->file);
                            current2 = lib_strdup(palettelist->file);
                            current2[n - 4] = 0;
                            palette_dyn_menu2[i].data = current2;
                        }
                    }
                }
                if (palette_dyn_menu2[i].data == NULL) {
                    palette_dyn_menu2[i].data = (ui_callback_data_t)lib_strdup(palettelist->file);
                }
                ++i;
            }
            ++palettelist;
        }
        palette_dyn_menu2[i].string   = NULL;
    }

    palette_menu1[0].action   = ACTION_NONE;
    palette_menu1[0].string   = "External palette";
    palette_menu1[0].type     = MENU_ENTRY_RESOURCE_TOGGLE;
    palette_menu1[0].callback = toggle_func1;
    palette_menu1[0].data     = NULL;

    palette_menu1[1].action   = ACTION_NONE;
    palette_menu1[1].string   = "Available palette files";
    palette_menu1[1].type     = MENU_ENTRY_SUBMENU;
    palette_menu1[1].callback = submenu_callback;
    palette_menu1[1].data     = (ui_callback_data_t)palette_dyn_menu1;

    palette_menu1[2].action   = ACTION_NONE;
    palette_menu1[2].string   = "Custom palette file";
    palette_menu1[2].type     = MENU_ENTRY_DIALOG;
    palette_menu1[2].callback = file_func1;
    palette_menu1[2].data     = (ui_callback_data_t)"Choose palette file";

    palette_menu1[3].string   = NULL;

    if (video_chip2_used) {
        palette_menu2[0].action   = ACTION_NONE;
        palette_menu2[0].string   = "External palette";
        palette_menu2[0].type     = MENU_ENTRY_RESOURCE_TOGGLE;
        palette_menu2[0].callback = toggle_func2;
        palette_menu2[0].data     = NULL;

        palette_menu2[1].action   = ACTION_NONE;
        palette_menu2[1].string   = "Available palette files";
        palette_menu2[1].type     = MENU_ENTRY_SUBMENU;
        palette_menu2[1].callback = submenu_callback;
        palette_menu2[1].data     = (ui_callback_data_t)palette_dyn_menu2;

        palette_menu2[2].action   = ACTION_NONE;
        palette_menu2[2].string   = "Custom palette file";
        palette_menu2[2].type     = MENU_ENTRY_DIALOG;
        palette_menu2[2].callback = file_func2;
        palette_menu2[2].data     = (ui_callback_data_t)"Choose palette file";

        palette_menu2[3].string   = NULL;
    }
}


/** \brief  Free memory used by the items of \a menu and \a menu itself
 *
 * \param[in,out]   menu    heap-allocated sub menu
 *
 * \note    the \a menu must be terminated with an empty entry
 */
static void palette_dyn_menu_free(ui_menu_entry_t *menu)
{
    ui_menu_entry_t *item = menu;
    while (item->string != NULL) {
        lib_free(item->string);
        lib_free(item->data);
        item++;
    }
    lib_free(menu);
}


/** \brief  Clean up memory used by the palette sub menu(s)
 */
void uipalette_menu_shutdown(void)
{
    if (palette_dyn_menu1 != NULL) {
        palette_dyn_menu_free(palette_dyn_menu1);
    }
    if (palette_dyn_menu2 != NULL) {
        palette_dyn_menu_free(palette_dyn_menu2);
    }
}
