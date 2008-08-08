/* $Id: socket.h,v 1.5 2004/09/01 20:16:01 rjoseph Exp $ */
#ifndef __SOCKET_H
#define __SOCKET_H

#include "ea.h"
#include "list.h"

typedef struct socket_info {
   int sd;
   time_t bday;
   char authed;
   pthread_t thread;
   struct sockaddr_in addr;
} sinfo;

/* convert l_ent ptr into sinfo ptr */
#define letosi(x) ((sinfo *)x->data)
/* get the address string from an sinfo ptr */
#define si_str(x) (inet_ntoa((x)->addr.sin_addr))

/* safely print using socketinfo_print */
#define si_debug(str, x) \
do { char *__tmp = socketinfo_print((x)); \
     debug(str, __tmp); free(__tmp); \
} while(0)

/* convience macro to send to a socket. */
#define sndsock(si, fmt, args...) \
do { char *__msg = (char *)chkmalloc(MAX_BUF_SIZE); int r;\
     snprintf(__msg, MAX_BUF_SIZE, fmt, ## args); \
     if ((r = send(si->sd, __msg, strlen(__msg), 0)) < 0 \
      || errno == EPIPE) { \
        debug("send() error %d or EPIPE errno (%d) - closing!\n", \
         r, errno); free(__msg); close_sock_thread(si); \
     } else free(__msg); \
} while (0)
/* yes I like macros, wanna fight about it? */

extern int setup_listen_socket(int, pthread_t *);
extern sinfo *create_sinfo(int, struct sockaddr_in *);
extern void close_sock_thread(void *);
extern void halt_main_thread();

#endif
