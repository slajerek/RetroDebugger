/**
 * \file render_queue.h
 * \brief Manage a pool of backbuffers for emu rendering and host display.
 *
 * \author David Hogan <david.q.hogan@gmail.com>
 */

/* This file is part of VICE, the Versatile Commodore Emulator.
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
#ifndef VICE_RENDER_QUEUE_H
#define VICE_RENDER_QUEUE_H

#define RENDER_QUEUE_MAX_BACKBUFFERS 2

#include <stdbool.h>

typedef struct {
    bool interlaced;
    int interlace_field;
    unsigned char *pixel_data;
    unsigned int pixel_data_size_bytes;
    unsigned int width;
    unsigned int height;
    float pixel_aspect_ratio;
} backbuffer_t;

void *render_queue_create(void);
void render_queue_destroy(void *render_queue);

backbuffer_t *render_queue_get_from_pool(void *render_queue, int pixel_data_size_bytes);
void render_queue_enqueue_for_display(void *render_queue, backbuffer_t *backbuffer);
unsigned int render_queue_length(void *render_queue);
backbuffer_t *render_queue_dequeue_for_display(void *render_queue);
void render_queue_return_to_pool(void *render_queue, backbuffer_t *backbuffer);

#endif /* #ifndef VICE_RENDER_QUEUE_H */
