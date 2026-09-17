/**
 * \file   videoarch.h
 * \brief  Headless graphics routines
 *
 * \author Ettore Perazzoli
 * \author Teemu Rantanen <tvr@cs.hut.fi>
 * \author Andreas Boose <viceteam@t-online.de>
 * \author Michael C. Martin <mcmartin@gmail.com>
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
#ifndef VICE_VIDEOARCH_H
#define VICE_VIDEOARCH_H

#include "archdep.h"
#include "video.h"

typedef struct video_canvas_s {

    /** \brief Nonzero if the structure has been fully realized. */
    unsigned int created;

    /** \brief Rendering configuration as seen by the emulator
     *         core. */
    struct video_render_config_s *videoconfig;

    /** \brief Tracks color encoding changes */
    int crt_type;

    /** \brief Drawing buffer as seen by the emulator core. */
    struct draw_buffer_s *draw_buffer;

    /** \brief Display window as seen by the emulator core. */
    struct viewport_s *viewport;

    /** \brief Machine screen geometry as seen by the emulator
     *         core. */
    struct geometry_s *geometry;

    /** \brief Color palette for translating display results into
     *         window colors. */
    struct palette_s *palette;

    /** \brief Used to limit frame rate under warp. */
    tick_t warp_next_render_tick;
} video_canvas_t;

typedef struct vice_renderer_backend_s {
} vice_renderer_backend_t;

#endif
