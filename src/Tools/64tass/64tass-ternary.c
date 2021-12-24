/* ternary.c - Ternary Search Trees
   Copyright (C) 2001 Free Software Foundation, Inc.

   Contributed by Daniel Berlin (dan@cgsoftware.com)

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */
#include <stdlib.h>
#include "64tass-ternary.h"
#include "64tass-misc.h"

/* Non-recursive so we don't waste stack space/time on large
   insertions. */

void *ternary_insert(ternary_tree *root, const char *s, void *data, int replace)
{
  int32_t diff;
  uint32_t spchar;
  ternary_tree curr, *pcurr;
  spchar = *s;
  if (spchar & 0x80) s += utf8in((uint8_t *)s, &spchar); else s++;

  /* Start at the root. */
  pcurr = root;
  /* Loop until we find the right position */
  while ((curr = *pcurr))
    {
      /* Calculate the difference */
      diff = spchar - curr->splitchar;
      /* Handle current char equal to node splitchar */
      if (diff == 0)
	{
	  /* Handle the case of a string we already have */
	  if (spchar == 0)
	    {
	      if (replace)
		curr->eqkid = (ternary_tree) data;
	      return (void *) curr->eqkid;
	    }
          spchar = *s;
          if (spchar & 0x80) s += utf8in((uint8_t *)s, &spchar); else s++;
	  pcurr = &(curr->eqkid);
	}
      /* Handle current char less than node splitchar */
      else if (diff < 0)
	{
	  pcurr = &(curr->lokid);
	}
      /* Handle current char greater than node splitchar */
      else
	{
	  pcurr = &(curr->hikid);
	}
    }
  /* It's not a duplicate string, and we should insert what's left of
     the string, into the tree rooted at curr */
  for (;;)
    {
      /* Allocate the memory for the node, and fill it in */
      *pcurr = (ternary_tree) malloc (sizeof (ternary_node));
      if (!pcurr) return NULL;
      curr = *pcurr;
      curr->splitchar = spchar;
      curr->lokid = curr->hikid = curr->eqkid = 0;

      /* Place nodes until we hit the end of the string.
         When we hit it, place the data in the right place, and
         return.
       */
      if (spchar == 0)
	{
	  curr->eqkid = (ternary_tree) data;
	  return data;
	}
      spchar = *s;
      if (spchar & 0x80) s += utf8in((uint8_t *)s, &spchar); else s++;
      pcurr = &(curr->eqkid);
    }
}

/* Free the ternary search tree rooted at p. */
void ternary_cleanup (ternary_tree p)
{
  if (p)
    {
      ternary_cleanup (p->lokid);
      if (p->splitchar)
	ternary_cleanup (p->eqkid);
      else free(p->eqkid);
      ternary_cleanup (p->hikid);
      free (p);
    }
}

/* Non-recursive find of a string in the ternary tree */
void *ternary_search (const ternary_node *p, const char *s)
{
  const ternary_node *curr;
  int32_t diff;
  uint32_t spchar;
  const ternary_node *last = NULL, *last2 = NULL;
  spchar = *s;
  if (spchar & 0x80) s += utf8in((uint8_t *)s, &spchar); else s++;
  curr = p;
  /* Loop while we haven't hit a NULL node or returned */
  while (curr)
    {
      /* Calculate the difference */
      diff = spchar - curr->splitchar;
      last2 = last;
      last = curr;
      /* Handle the equal case */
      if (diff == 0)
	{
	  if (spchar == 0)
	    return (void *) curr->eqkid;
          spchar = *s;
          if (spchar & 0x80) s += utf8in((uint8_t *)s, &spchar); else s++;
	  curr = curr->eqkid;
	}
      /* Handle the less than case */
      else if (diff < 0)
	curr = curr->lokid;
      /* All that's left is greater than */
      else
	curr = curr->hikid;
    }
  if (last2 && !last2->splitchar) return (void *)last2->eqkid;
  if (last && !last->splitchar) return (void *)last->eqkid;
  return NULL;
}

