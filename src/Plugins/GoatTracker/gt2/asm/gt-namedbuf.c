/*
 * Copyright (c) 2005 Magnus Lind.
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

#include "gt-namedbuf.h"
#include "gt-log.h"
#include "gt-vec.h"
#include "gt-membufio.h"

#include <stdlib.h>


struct sbe
{
    const char *name;
    struct membuf mb[1];
};

static struct vec s_sbe_table[1];

static int sbe_cmp(const void *a, const void *b)
{
    struct sbe *sbe_a;
    struct sbe *sbe_b;
    int val;

    sbe_a = (struct sbe*)a;
    sbe_b = (struct sbe*)b;

    val = strcmp(sbe_a->name, sbe_b->name);

    return val;
}

void sbe_free(struct sbe *e)
{
    membuf_free(e->mb);
}

void named_buffer_init()
{
    vec_init(s_sbe_table, sizeof(struct sbe));
}

void named_buffer_free()
{
    typedef void cb_free(void *a);

    vec_free(s_sbe_table, (cb_free*)sbe_free);
}

struct membuf *new_named_buffer(const char *name)
{
    int pos;
    struct sbe e[1];
    struct sbe *ep;
    struct membuf *mp;

    /* name is already strdup:ped */
    e->name = name;
    pos = vec_find(s_sbe_table, sbe_cmp, e);
    if(pos >= 0)
    {
        /* found */
        LOG(LOG_ERROR, ("buffer already exists.\n"));
        exit(-1);
    }
    membuf_init(e->mb);
    ep = vec_insert(s_sbe_table, -(pos + 2), e);
    mp = ep->mb;

    return mp;
}

struct membuf *get_named_buffer(const char *name)
{
    int pos;
    struct sbe e[1];
    struct sbe *ep;
    struct membuf *mp;

    /* name is already strdup:ped */
    e->name = name;
    pos = vec_find(s_sbe_table, sbe_cmp, e);
    if(pos >= 0)
    {
        /* found */
        ep = vec_get(s_sbe_table, pos);
    }
    else
    {
        membuf_init(e->mb);
        read_file(name, e->mb);
        ep = vec_insert(s_sbe_table, -(pos + 2), e);
    }
    mp = ep->mb;

    return mp;
}
