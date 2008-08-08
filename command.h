/* $Id: command.h,v 1.5 2004/08/31 19:29:08 rjoseph Exp $ */
#ifndef __COMMAND_H
#define __COMMAND_H

#include "ea.h"
#include "socket.h"
#include "search.h"

/* return codes for commands.  anything < 200 is considered a
   successuful return and the description is prefixed with '+OK'.
   > 200 is an error and is prefixed with '-ERR' */
#define RET_SUCCESS       100
#define RET_NORES         101
#define RET_UNKNOWN       200
#define RET_ERRAUTH       201
#define RET_FNF           202
#define RET_BADARGS       203
#define RET_BADAUTH       204
#define RET_ELENF         205

/* args[10] is declared as such because GCC no longer allows
   flexible arrays to be initialized inline as our comm_table
   array is -- I sincerly doubt there'll be more than 10 args
   to any command, so while this isn't great, it should work,
   especially since it's only for descriptive purposes */
typedef struct command {
   char *name;
   int (*func)(char **, sinfo *, l_ent *);
   char *desc;
   char *args[10];
} cmd_s;

typedef struct comm_ret {
   int err_num;
   char *desc;
} cmd_r;

#define chkauth(x) if (!si->authed) return RET_ERRAUTH;

extern cmd_s comm_table[];
extern cmd_r comm_returns[];

extern int comm_exec(char *, sinfo *, l_ent *);
extern const char *find_return_string(int);

#endif
