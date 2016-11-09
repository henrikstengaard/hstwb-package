/* path.c:
 *
 * Path lists.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1993,1994 by Frank Munkert.
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 */

#include <stdlib.h>
#include <string.h>

#include <exec/memory.h>

#include <clib/exec_protos.h>

#ifdef LATTICE
#include <pragmas/exec_pragmas.h>
#endif

#include "generic.h"

typedef struct path_node {
  unsigned long references;
  char *name;
  struct path_node *next;
} t_path_node;

/* Append p_name to path list: */

t_path_list Append_Path_List (t_path_list p_list, char *p_name)
{
  t_path_node *node;

  if (!(node = AllocMem (sizeof (*node), MEMF_PUBLIC)))
    return NULL;

  node->references = 1;
  if (!(node->name = AllocMem (strlen (p_name) + 1, MEMF_PUBLIC))) {
    FreeMem (node, sizeof (*node));
    return NULL;
  }

  strcpy (node->name, p_name);
  node->next = Copy_Path_List (p_list, FALSE);
  
  return node;
}

t_path_list Copy_Path_List (t_path_list p_src, int p_strip)
{
  t_path_node *node, *start;

  if (!p_src)
    return NULL;

  start = p_strip ? p_src->next : p_src;

  for (node = start; node; node = node->next) {
    node->references++;
  }
  
  return start;
}

void Free_Path_List (t_path_list p_list)
{
  t_path_node *node, *next;

  if (!p_list)
    return;

  for (node = p_list; node; node = next) {
    next = node->next;
    if (--node->references == 0) {
      FreeMem (node->name, strlen (node->name) + 1);
      FreeMem (node, sizeof (*node));
    }
  }
}

t_bool Path_Name_From_Path_List (t_path_list p_list, char *p_buffer,
			         int p_buffer_length)
{
  int len;
  t_path_node *node;

  if (!p_list) {
    strcpy (p_buffer, ":");
    return TRUE;
  }
  
  /* calculate length: */
  for (len=1, node=p_list; node; node = node->next)
    len += strlen (node->name) + 1;

  if (len > p_buffer_length)
    return FALSE;

  for (node=p_list; node; node = node->next) {
    memcpy (p_buffer + len - strlen (node->name) - 1,
            node->name, strlen (node->name));
    p_buffer[len-1] = (node == p_list) ? 0 : '/';
    len -= strlen (node->name) + 1;
  }
  p_buffer[0] = ':';
  return TRUE;
}
