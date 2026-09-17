/**
 * \file render_thread.h
 * \brief Centralised management of render threads enabling controlled shutdown.
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
#ifndef VICE_RENDER_THREAD_H
#define VICE_RENDER_THREAD_H

#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum render_job {
    render_thread_init = 1, /* Else looks like NULL when pushed to the queue */
    render_thread_render,
    render_thread_shutdown
} render_job_t;

typedef void (*render_thread_callback_t)(void *thread_context, void *job_context);

typedef struct render_thread_s *render_thread_t;

render_thread_t render_thread_create(render_thread_callback_t callback, void *thread_context);
void render_thread_initiate_shutdown(render_thread_t render_thread);
void render_thread_join(render_thread_t render_thread);
void render_thread_shutdown_and_join_all(void);

void render_thread_push_job(render_thread_t render_thread, render_job_t job_context);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* #ifndef VICE_RENDER_THREAD_H */
