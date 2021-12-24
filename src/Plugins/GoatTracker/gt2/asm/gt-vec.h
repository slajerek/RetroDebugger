#ifndef ALREADY_INCLUDED_VEC
#define ALREADY_INCLUDED_VEC

/*
 * Copyright (c) 2003 - 2005 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software, alter it and re-
 * distribute it freely for any non-commercial, non-profit purpose subject to
 * the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software in a
 *   product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *   be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any distribution.
 *
 *   4. The names of this software and/or it's copyright holders may not be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 */

#include "gt-callback.h"
#include "gt-membuf.h"
#include <string.h>


#define STATIC_VEC_INIT(EL_SIZE) {(EL_SIZE), STATIC_MEMBUF_INIT, 1}

struct vec {
    size_t elsize;
    struct membuf buf;
    int flags;
};

struct vec_iterator {
    struct vec *vec;
    int pos;
};

void vec_init(struct vec *p, size_t elsize);
void vec_clear(struct vec *p, cb_free * f);
void vec_free(struct vec *p, cb_free * f);

int vec_count(struct vec *p);
void *vec_get(struct vec *p, int index);
void *vec_insert(struct vec *p, int index, const void *in);
void *vec_push(struct vec *p, const void *in);
/**
 * Gets the position where the key is stored in the vector. The vector
 * needs to be sorted for this function to work. Returns the position,
 * -1 on error or a negative number that can be converted to where
 * it should have been if it had been inserted. insert_pos = -(val + 2)
 **/
int vec_find(struct vec *p, cb_cmp * f, const void *key);

/**
 * Gets a pointer to the element that the key points to.
 * Returns a pointer that may be null if not found.
 **/
void *vec_find2(struct vec *p, cb_cmp * f, const void *key);

/**
 * Inserts the in element in its correct position in a sorted vector.
 * returns 1 if insertion is successful, 0 if element is already
 * inserted or -1 on error. If out is not NULL it will be
 * dereferenced and set to the inserted element.
 **/
int vec_insert_uniq(struct vec *p, cb_cmp * f, const void *in, void **out);
void vec_sort(struct vec *p, cb_cmp * f);

void vec_get_iterator(struct vec *p, struct vec_iterator *i);
void *vec_iterator_next(struct vec_iterator *i);


#endif
