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

#include "gt-vec.h"
#include <stdlib.h>


#define VEC_FLAG_SORTED 1

void vec_init(struct vec *p, size_t elsize)
{
    p->elsize = elsize;
    membuf_init(&p->buf);
    p->flags = VEC_FLAG_SORTED;
}

void vec_clear(struct vec *p, cb_free * f)
{
    struct vec_iterator i;
    void *d;

    vec_get_iterator(p, &i);

    if (f != NULL)
    {
        while ((d = vec_iterator_next(&i)) != NULL)
        {
            f(d);
        }
    }
    membuf_clear(&p->buf);
    p->flags = VEC_FLAG_SORTED;
}

void vec_free(struct vec *p, cb_free * f)
{
    vec_clear(p, f);
    membuf_free(&p->buf);
}

int vec_count(struct vec *p)
{
    int count;
    count = membuf_memlen(&p->buf) / p->elsize;
    return count;
}

void *vec_get(struct vec *p, int index)
{
    char *buf;

    buf = (char *) membuf_get(&p->buf);
    buf += index * p->elsize;

    return (void *)buf;
}

void *vec_insert(struct vec *p, int index, const void *in)
{
    void *buf;

    buf = membuf_insert(&p->buf, index * p->elsize, in, p->elsize);

    return buf;
}

void *vec_push(struct vec *p, const void *in)
{
    void *out;
    out = membuf_append(&p->buf, in, p->elsize);
    p->flags &= ~VEC_FLAG_SORTED;

    return out;
}

int vec_find(struct vec *p, cb_cmp * f, const void *in)
{
    int lo;

    lo = -1;
    if(p->flags & VEC_FLAG_SORTED)
    {
        int hi;
        lo = 0;
        hi = vec_count(p) - 1;
        while(lo <= hi)
        {
            int next;
            int val;

            next = (lo + hi) / 2;
            val = f(in, vec_get(p, next));
            if(val == 0)
            {
                /* match */
                lo = -(next + 2);
                break;
            }
            else if(val < 0)
            {
                hi = next - 1;
            }
            else
            {
                lo = next + 1;
            }
        }
    }
    return -(lo + 2);
}

void *vec_find2(struct vec *p, cb_cmp * f, const void *key)
{
    void *out = NULL;
    int pos = vec_find(p, f, key);
    if(pos >= 0)
    {
        out = vec_get(p, pos);
    }
    return out;
}

int vec_insert_uniq(struct vec *p, cb_cmp * f, const void *in, void **outp)
{
    int val = 0;
    void *out;

    val = vec_find(p, f, in);
    if(val != -1)
    {
        if(val < 0)
        {
            /* not there */
            out = vec_insert(p, -(val + 2), in);
            val = 1;
        }
        else
        {
            out = vec_get(p, val);
            val = 0;
        }

        if(outp != NULL)
        {
            *outp = out;
        }
    }

    return val;
}

void vec_sort(struct vec *p, cb_cmp * f)
{
    qsort(membuf_get(&p->buf), vec_count(p), p->elsize, f);
    p->flags |= VEC_FLAG_SORTED;
}


void vec_get_iterator(struct vec *p, struct vec_iterator *i)
{
    i->vec = p;
    i->pos = 0;
}

void *vec_iterator_next(struct vec_iterator *i)
{
    void *out;
    int count = vec_count(i->vec);
    if (i->pos >= count)
    {
        return NULL;
    }
    out = vec_get(i->vec, i->pos);
    i->pos += 1;
    return out;
}
