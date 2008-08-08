/* $Id: playlist.c,v 1.2 2004/08/31 16:30:40 rjoseph Exp $ */
#include "playlist.h"

/* This file is really just a place to declare these varibles
   and get them init'ed, probably won't do much else */
l_ent *global_playlist = NULL;
pthread_mutex_t global_pl_lock;
int global_pl_counter = 0;

void init_playlist() {
   if (!global_playlist) {
      debug("Initializing global playlist\n");
      pthread_mutex_init(&global_pl_lock, NULL);
      pthread_mutex_lock(&global_pl_lock);

      global_playlist = list_init();
      debug("global_playlist is 0x%x\n", global_playlist);
      global_pl_counter = 1;

      pthread_mutex_unlock(&global_pl_lock);
   }
}

void destroy_playlist() {
   list_free(global_playlist, list_free_noop);
}
