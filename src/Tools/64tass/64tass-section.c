/*

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#include <stdlib.h>
#include <string.h>
#include "64tass-section.h"
#include "64tass-error.h"

struct section_s root_section;
struct section_s *current_section = &root_section;

static int section_compare(const struct avltree_node *aa, const struct avltree_node *bb)
{
    struct section_s *a = avltree_container_of(aa, struct section_s, node);
    struct section_s *b = avltree_container_of(bb, struct section_s, node);

    return strcmp(a->name, b->name);
}

static void section_free(const struct avltree_node *aa)
{
    struct section_s *a = avltree_container_of(aa, struct section_s, node);
    free((char *)a->name);
    avltree_destroy(&a->members);
    free(a);
}

struct section_s *find_new_section(const char* name) {
    const struct avltree_node *b;
    struct section_s *context = current_section;
    struct section_s tmp;
    tmp.name=name;

    while (context) {
        b=avltree_lookup(&tmp.node, &context->members);
        if (b) {
            labelexists=1;
            return avltree_container_of(b, struct section_s, node);
        }
        context = context->parent;
    }
    return new_section(name);
}

static struct section_s *lastsc=NULL;
struct section_s *new_section(const char* name) {
    const struct avltree_node *b;
    struct section_s *tmp;
    if (!lastsc)
	if (!(lastsc=malloc(sizeof(struct section_s)))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
    lastsc->name=name;
    b=avltree_insert(&lastsc->node, &current_section->members);
    if (!b) { //new section
	if (!(lastsc->name=malloc(strlen(name)+1))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
        strcpy((char *)lastsc->name,name);
        lastsc->parent=current_section;
        lastsc->provides=~(uval_t)0;lastsc->requires=lastsc->conflicts=0;
        lastsc->address=lastsc->l_address=0;
        lastsc->dooutput=1;
        lastsc->declared=0;
        avltree_init(&lastsc->members, section_compare, section_free);
	labelexists=0;
	tmp=lastsc;
	lastsc=NULL;
	return tmp;
    }
    labelexists=1;
    return avltree_container_of(b, struct section_s, node);            //already exists
}

void init_section(void) {
    root_section.parent = NULL;
    root_section.name = NULL;
    avltree_init(&root_section.members, section_compare, section_free);
}

void destroy_section(void) {
    free(lastsc);
    avltree_destroy(&root_section.members);
}
// ---------------------------------------------------------------------------
