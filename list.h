/* $Id: list.h,v 1.8 2004/08/31 19:29:08 rjoseph Exp $ */
#ifndef __LIST_H
#define __LIST_H

#include "ea.h"
#include "util.h"

typedef struct list_entry {
   void *data;
   struct list_entry *next;
   struct list_entry *prev;
   int pri;
} l_ent;

/* next time you knock macros, check out this badass shit */
#define donodeswap(t, x, y) do { l_ent *t2; t2 = t->x->x; \
t->x->y = t->y; if (t->y) t->y->x = t->x; if (t2) t2->y = t; \
t->y = t->x; t->x->x = t; t->x = t2; } while (0)

/* the list_pri_ functions are named as such to denote that they 
   mess with the l_ent.pri element -- they can be used on regular 
   lists, but know that doing so will most likely cause the list to
   reorder itself at some point. */
extern l_ent  *list_init();
extern l_ent  *list_add(l_ent **);
extern l_ent  *list_add_with_data(l_ent **, void *);
extern l_ent  *list_find(l_ent *, void *, int (*)(void *, void *));
extern l_ent  *list_delete(l_ent **, void *, int (*)(void *, void *));
extern l_ent  *list_pri_add(l_ent **, int);
extern l_ent  *list_pri_add_wd(l_ent **, int, void *);
extern void    list_pri_shuffle(l_ent **);
extern void    list_pri_adjust(l_ent **, l_ent *);
extern void    list_pri_sort(l_ent **, int (*)(void *));
extern size_t  list_size(l_ent *);
extern void    list_free(l_ent *, void (*)(void *));
extern void    list_debug_print(l_ent *, char *(*)(void *));
extern void    list_free_noop(void *);
extern void    list_free_generic(void *);

#endif
