/* $Id: ea.h,v 1.1 2004/08/31 19:29:08 rjoseph Exp $
   main header file for epicaudio
*/
#ifndef __EA_H
#define __EA_H

/* system includes */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>    /* str functions */
#include <time.h>
#include <fcntl.h>     /* non-blocking sockets */
#include <errno.h>
#include <signal.h>    /* SIGPIPE ignore */
#include <search.h>    /* tree and hash table stuff */
#include <sys/stat.h>  /* lstat */
#include <dirent.h>    /* directory stuff */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <id3tag.h>
#include <mad.h>

/* string defines */
#define VERSION      "$Revision: 1.1 $"
#define NAME         "Epic Audio"
#define WEAK_PASS    "mitchell"

/* numercial defines */
#define LISTEN_PORT        4177
#define LISTEN_BACKLOG     1024
#define MAX_BUF_SIZE       4096
#define U_SLEEP            400
#define HTABLE_SIZE        3

#ifdef DEBUG
#define debug(fmt, args...) \
do { printf("%12s: " fmt, __FILE__, ## args); fsync(stdout); } while(0)
#else
#define debug(fmt, args...)
#endif

#endif
