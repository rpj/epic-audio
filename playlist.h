/* $Id: playlist.h,v 1.3 2004/08/31 19:29:08 rjoseph Exp $ */
#ifndef __PLAYLIST_H
#define __PLAYLIST_H

#include "ea.h"
#include "socket.h"
#include "list.h"
#include "search.h"

typedef struct playlist_entry {
   af_ent *af;
   int id;
} pl_ent;

extern l_ent *global_playlist;
extern pthread_mutex_t global_pl_lock;
extern int global_pl_counter;

extern void init_playlist();
extern void destroy_playlist();
#endif
