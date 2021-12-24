/*
 * Copyright (c) 2004 Magnus Lind.
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

#include "gt-pc.h"
#include "gt-log.h"
#include <stdlib.h>


static struct expr unset_value[1];
static struct expr *s_pc1;
static int s_pc2;

void pc_dump(int level)
{
}

void pc_set(int pc)
{
    s_pc1 = NULL;
    s_pc2 = pc;
}

void pc_set_expr(struct expr *pc)
{
    s_pc1 = pc;
    s_pc2 = 0;
}

struct expr *pc_get()
{
    struct expr *old_pc1;

    if(s_pc1 == unset_value)
    {
        LOG(LOG_ERROR, ("PC must be set by a .org(pc) call.\n"));
        exit(-1);
    }
    if(s_pc1 == NULL || s_pc2 != 0)
    {
        old_pc1 = s_pc1;
        s_pc1 = new_expr_number(s_pc2);
        s_pc2 = 0;
        if(old_pc1 != NULL)
        {
            s_pc1 = new_expr_op2(PLUS, s_pc1, old_pc1);
        }
    }

    return s_pc1;
}

void pc_add(int offset)
{
    if(s_pc1 != unset_value)
    {
        s_pc2 += offset;
    }
}

void pc_add_expr(struct expr *pc)
{
    struct expr *old_pc1;

    if(s_pc1 != unset_value)
    {
        old_pc1 = s_pc1;
        s_pc1 = pc;
        if(old_pc1 != NULL)
        {
            s_pc1 = new_expr_op2(PLUS, s_pc1, old_pc1);
        }
    }
}

void pc_unset()
{
    pc_set_expr(unset_value);
}
