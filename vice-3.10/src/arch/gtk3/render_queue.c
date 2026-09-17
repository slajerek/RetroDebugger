/**
 * \file render_queue.c
 * \brief Manage a queue of rendered backbuffers to display
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

#include "vice.h"
#include "render_queue.h"

#include <assert.h>
#include <pthread.h>
#include <string.h>

#include "lib.h"
#include "vsyncapi.h"

#define LOCK() pthread_mutex_lock(&rq->lock)
#define UNLOCK() pthread_mutex_unlock(&rq->lock)

typedef struct vice_render_queue_s {
    pthread_mutex_t lock;

    /** Holds all currently unused backbuffers */
    backbuffer_t *backbuffer_stack[RENDER_QUEUE_MAX_BACKBUFFERS];

    /** How many unused backbuffers in the stack */
    unsigned int backbuffer_stack_size;

    /** Holds the queue of backbuffers ready to render */
    backbuffer_t *render_queue[RENDER_QUEUE_MAX_BACKBUFFERS];

    /** How many backbuffers in render queue */
    unsigned int render_queue_length;

    /** Index of next backbuffer to render in render_queue */
    unsigned int render_queue_next;

    /** Allows discarding of late buffer returns */
    unsigned int backbuffer_generation;
} render_queue_t;

static void free_backbuffer(backbuffer_t *backbuffer) {
    lib_free(backbuffer->pixel_data);
    lib_free(backbuffer);
}

/****/

/** \brief Allocate, initialise and return a new render queue. */
void *render_queue_create(void)
{
    render_queue_t *rq;
    backbuffer_t *bb;
    int i;

    rq = lib_calloc(1, sizeof(render_queue_t));
    pthread_mutex_init(&rq->lock, NULL);

    /* Seed the pool with the maximum number of backbuffers */
    for (i = 0; i < RENDER_QUEUE_MAX_BACKBUFFERS; i++) {

        bb = lib_malloc(sizeof(backbuffer_t));
        bb->pixel_data = lib_malloc(0);
        bb->pixel_data_size_bytes = 0;
        bb->width = 0;
        bb->height = 0;
        bb->pixel_aspect_ratio = 0.0f;

        rq->backbuffer_stack[rq->backbuffer_stack_size++] = bb;
    }

    return rq;
}

/** \brief Destroy a render queue. */
void render_queue_destroy(void *render_queue)
{
    render_queue_t *rq = (render_queue_t *)render_queue;
    int i;

    /* The unused backbuffers */
    for (i = 0; i < rq->backbuffer_stack_size; i++) {
        free_backbuffer(rq->backbuffer_stack[i]);
    }

    /* The backbuffers queued for rendering */
    for (i = 0; i < rq->render_queue_length; i++) {
        free_backbuffer(rq->render_queue[rq->render_queue_next++]);
        rq->render_queue_next = rq->render_queue_next % RENDER_QUEUE_MAX_BACKBUFFERS;
    }

    pthread_mutex_destroy(&rq->lock);
    lib_free(render_queue);
}

/****/

/** Obtain unused backbuffer for offscreen rendering, or NULL if none available */
backbuffer_t *render_queue_get_from_pool(void *render_queue, int pixel_data_size_bytes)
{
    render_queue_t *rq = (render_queue_t *)render_queue;
    backbuffer_t *bb;

    LOCK();

    if (!rq->backbuffer_stack_size) {
        /* no buffers available, skip this frame */
        UNLOCK();
        return NULL;
    }

    bb = rq->backbuffer_stack[rq->backbuffer_stack_size - 1];
    rq->backbuffer_stack_size--;

    UNLOCK();

    /* Make sure there's at least the requested size in bytes */
    if (bb->pixel_data_size_bytes < pixel_data_size_bytes) {
        lib_free(bb->pixel_data);
        bb->pixel_data = lib_calloc(pixel_data_size_bytes, 1);
        bb->pixel_data_size_bytes = pixel_data_size_bytes;
    }

    bb->width = 0;
    bb->height = 0;
    bb->pixel_aspect_ratio = 0.0f;

    return bb;
}

/** Add backbuffer to the queue of backbuffers to be displayed */
void render_queue_enqueue_for_display(void *render_queue, backbuffer_t *backbuffer)
{
    render_queue_t *rq = (render_queue_t *)render_queue;

    LOCK();

    assert(rq->render_queue_length < RENDER_QUEUE_MAX_BACKBUFFERS);

    rq->render_queue[(rq->render_queue_next + rq->render_queue_length) % RENDER_QUEUE_MAX_BACKBUFFERS] = backbuffer;
    rq->render_queue_length++;

    UNLOCK();
}

unsigned int render_queue_length(void *render_queue)
{
    render_queue_t *rq = (render_queue_t *)render_queue;

    unsigned int render_queue_length;

    LOCK();

    render_queue_length = rq->render_queue_length;

    UNLOCK();

    return render_queue_length;
}

/** Obtain rendered backbuffer for display, or NULL if none available */
backbuffer_t *render_queue_dequeue_for_display(void *render_queue)
{
    render_queue_t *rq = (render_queue_t *)render_queue;
    void *backbuffer;

    LOCK();

    /* Are there any available? */
    if (!rq->render_queue_length) {
        UNLOCK();
        return NULL;
    }

    backbuffer = rq->render_queue[rq->render_queue_next];
    rq->render_queue_next = (rq->render_queue_next + 1) % RENDER_QUEUE_MAX_BACKBUFFERS;
    rq->render_queue_length--;

    UNLOCK();

    return backbuffer;
}

void render_queue_return_to_pool(void *render_queue, backbuffer_t *backbuffer)
{
    render_queue_t *rq = (render_queue_t *)render_queue;

    LOCK();

    assert(rq->backbuffer_stack_size < RENDER_QUEUE_MAX_BACKBUFFERS);

    rq->backbuffer_stack[rq->backbuffer_stack_size] = backbuffer;
    rq->backbuffer_stack_size++;

    UNLOCK();
}
