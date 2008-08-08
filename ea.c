/* $Id: ea.c,v 1.1 2004/08/31 19:29:08 rjoseph Exp $ */
#include "ea.h"

int main(int argc, char **argv) {
   int port, rv;
   pthread_t thread;

   if (argc < 2) port = LISTEN_PORT;
   else port = atoi(argv[1]);

   if (rv = setup_listen_socket(port, &thread))
      perror("setup_listen_socket returned non-zero");

   debug("listen thread created, leaving main()...\n", thread);
   pthread_exit(NULL);
   return 0;
}
