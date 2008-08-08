/* $Id: util.c,v 1.4 2004/08/21 00:45:38 rjoseph Exp $ */
#include "util.h"

char *getcwd_malloc() {
   char *cwdbuf = (char *)malloc(MAX_BUF_SIZE);
   int i;

   for (i = 1; (!(cwdbuf = getcwd(cwdbuf, (MAX_BUF_SIZE * i)))
    && errno == ERANGE); i++)
      cwdbuf = (char *)realloc(cwdbuf, (MAX_BUF_SIZE * i));

   return cwdbuf;
}

int is_spec_dir(mode_t mode, char *name) {
   return (S_ISDIR(mode) && (!(strcmp(name, ".")) ||
    !(strcmp(name, "..")))) ? 1 : 0;
}

void *chkmalloc(size_t s) {
   void *r;

   if (!(r = malloc(s))) {
      perror("Out of memory!");
      exit(ENOMEM);
   }

   return r;
}
