/* $Id: search.h,v 1.7 2004/08/31 19:29:08 rjoseph Exp $ */
#ifndef __SEARCH_H
#define __SEARCH_H

#include "ea.h"
#include "list.h"

typedef struct audio_file_entry {
   char *fname;
   void *path;   /* pointer to path name in __file_paths */
} af_ent;

extern int search_dir(char *, int (*)(struct dirent *));
extern int audio_search(char *);
extern void free_search_trees();

/* these two functions must be used in conjuction, as they lock
   and unlock a mutex, respectively */
extern l_ent * begin_tree_search(char *);
extern void end_tree_search();

#endif
