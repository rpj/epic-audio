/* $Id: list.c,v 1.17 2004/09/01 20:16:01 rjoseph Exp $
   simple doubly-linked list implementation with flexible callbacks
   since the list is not circular, all new nodes are added to the front
*/
#include "list.h"

static l_ent *list_init_struct(size_t s) {
   l_ent *ret = (l_ent *)chkmalloc(s);
   memset(ret, 0, s);

   ret->next = ret->prev = NULL;
   return ret;
}

l_ent *list_init() {return list_init_struct(sizeof(l_ent));}

size_t list_size(l_ent *head) {
   int cnt = 0;
   l_ent *t = head;
   
   for (; t; t = t->next, cnt++) {}

   return head->data ? cnt : cnt - 1;
}

l_ent *__list_add(l_ent **head, l_ent *new) {
   new->next = *head;
   new->prev = NULL;
   (*head)->prev = new;
   *head = new;
   return new;
}

/* returns newly-added node, which also happens to be the
   new head node.  however, this may not always be the case, so it
   is advised to grab the return value if you really need to mess
   with the new node, instead of just using the head node */
l_ent *list_add(l_ent **head) {
   l_ent *ret;
   
   if (!(*head)->data) return *head;
   ret = list_init_struct(sizeof(l_ent));

   return __list_add(head, ret);
}

l_ent *list_add_with_data(l_ent **head, void *data) {
   l_ent *t = list_add(head);
   t->data = data;
   return t;
}

/* works like a charm now! */
void list_pri_adjust(l_ent **head, l_ent *node) {
   l_ent *t = node;
   int cond = 0;

   while ((t->next && t->pri > t->next->pri) ||
	  (t->prev && t->pri < t->prev->pri && (cond = 1))) {
      if (cond) {
	 donodeswap(t, prev, next);
	 if (!t->prev) *head = t;
      } else {
	 donodeswap(t, next, prev);
	 if (t == *head) *head = t->prev;
      }
    }
}

l_ent *list_pri_add(l_ent **head, int pri) {
   l_ent *ret, *t, *t2;

   /* if the list is "empty" */
   if (!(*head)->data) { 
      (*head)->pri = pri;
      return *head;
   }

   ret = list_init_struct(sizeof(l_ent));
   __list_add(head, ret);
   ret->pri = pri;

   list_pri_adjust(head, ret);
   return ret;
}

l_ent *list_pri_add_wd(l_ent **head, int pri, void *data) {
   l_ent *p = list_pri_add(head, pri);
   p->data = data;
   return p;
}

void list_free_noop(void *data) {}
void list_free_generic(void *data) {free(data);}

void __list_to_array(l_ent *head, l_ent **arr) {
   l_ent *t;
   int cnt = 0;

   for (t = head; t && (*(arr++) = t); t = t->next) {}
}

void list_pri_shuffle(l_ent **head) {
   int max_pri = 0, cnt = 0, size = list_size(*head);
   l_ent *arr[size], *t;
   
   for (t = *head; t; t = t->next)
      max_pri = abs(t->pri) > max_pri ? abs(t->pri) : max_pri;

   srand(time(NULL));
   __list_to_array(*head, arr);

   for (cnt = 0; cnt < size; cnt++) {
      arr[cnt]->pri = rand() % max_pri + 2;
      list_pri_adjust(head, arr[cnt]);
   }
}

/* takes a function that returns an int and takes the data pointer:
   the return will be the node's new priority */
void list_pri_sort(l_ent **head, int (*f)(void *d)) {
   int size = list_size(*head), cnt = 0;
   l_ent *arr[size], *t;

   __list_to_array(*head, arr);

   for (; cnt < size; cnt++) {
      arr[cnt]->pri = (*f)(arr[cnt]->data);
      list_pri_adjust(head, arr[cnt]);
   }
}

/* f should return non-zero (true) on successful comparison */
l_ent *list_find(l_ent *head, void *d, int (*f)(void *, void *)) {
   l_ent *ret = NULL;

   for (ret = head; ret && !(*f)(d, ret->data); ret = ret->next) {}
   return ret;
}

/* needs a comparison function 'f' for the data, 'd' is the search data
   function 'f' should return non-zero if the value is found */
l_ent *list_delete(l_ent **head, void *d, int (*f)(void *, void *)) {
   l_ent *t, *ret = *head;

   for (t = *head; t; t = t->next) {
      if (d && t->data && (*f)(d, t->data)) {
	 debug("removing 0x%x (head 0x%x) from list\n", t, *head);
	 
	 if (t->prev)
	    t->prev->next = t->next;	 
	 if (t->next) 
	    t->next->prev = t->prev;

	 if (t == *head && t->next)
	    ret = t->next;

	 free(t->data);
	 t->data = NULL;
	 if (t->prev || t->next) free(t);

	 break;
      }
   }

   *head = ret;
   return ret;
}

static void __list_free(l_ent *head) {
   l_ent *t1, *t2;

   t1 = head->next;
   while (t1) {
      t2 = t1->next;
      free(t1);
      t1 = t2;
   }

   free(head);
}

/* moves through the list running 'f' to cleanup, then frees the list
   l_ent->data is passed as the argument to f */
void list_free(l_ent *head, void (*f)(void *)) {
   l_ent *t;

   for (t = head; f && t; t = t->next)
      (*f)(t->data);

   __list_free(head);
}

/* the void * passed to p is the current list element */
void list_debug_print(l_ent *head, char *(*p)(void *)) {
   l_ent *t;
   int c = 0;
   char *ptr;

   for (t = head; t; t = t->next, c++)
      debug("0x%x <- 0x%x -> 0x%x: %s\n",
       t->prev, t, t->next, (ptr = (*p)(t->data)));
 
   free(ptr);
}
