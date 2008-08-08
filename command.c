/* $Id: command.c,v 1.18 2004/09/01 20:16:01 rjoseph Exp $ */
#include "command.h"
#include "playlist.h"

static int __quit(char **a, sinfo *si, l_ent *h) {
   sndsock(si, "%s signing off\n", NAME);
   close_sock_thread(si);

   return RET_SUCCESS;
}   

static int __shutdown(char **a, sinfo *si, l_ent *h) {
   l_ent *tmp;
   char when[MAX_BUF_SIZE];
   int s_time = 0;

   si_debug("%s requested shutdown\n", si);

   if (!si->authed) return RET_ERRAUTH;
   debug("User was authorized, server is going down!\n");

   if (a[1]) {
      s_time = atoi(a[1]);
      snprintf(when, MAX_BUF_SIZE, "in %s seconds!", a[1]);
   }

   for (tmp = h; tmp; tmp = tmp->next)
      sndsock(letosi(tmp), "Server is shutting down %s\n",
	      (a[1] ? when : "NOW!"));

   sleep(s_time);
   halt_main_thread();

   return RET_SUCCESS;
}

static int __info(char **a, sinfo *si, l_ent *h) {
   l_ent *tmp;
   char t[MAX_BUF_SIZE];
   int cnt = 0;

   for (tmp = h; tmp; tmp = tmp->next, cnt++) {
      ctime_r(&(letosi(tmp))->bday, t);
      chomp(t);

      if (si->authed) {
	 sndsock(si, "client #%d%-8s %s:%d connected %s%s\n", cnt+1,
		 (tmp->data == si ? " (YOU):" : ":"), 
	      si_str(letosi(tmp)), ntohs(letosi(tmp)->addr.sin_port), t,
	      (letosi(tmp)->authed ? ", authorized" : ""));
      } else
	 sndsock(si, "client #%d%-8s %s connected %s\n", cnt+1,
		 (tmp->data == si ? " (YOU):" : ":"),
		 si_str(letosi(tmp)), t);
   }

   return RET_SUCCESS;
}

/* seriously weak authentication, may be changed in the future */     
static int __auth(char **a, sinfo *si, l_ent *h) {
   if (a[1] && !strcmp(a[1], WEAK_PASS) && (si->authed = 1)) {
      sndsock(si, "Now authorized.\n");
      return RET_SUCCESS;
   } else return RET_BADAUTH;
}

static int __help(char **a, sinfo *si, l_ent *h) {
   struct command temp;
   int cnt = 0, cnt2 = 0;
   char **arg;

   sndsock(si, "** Available commands (arguments in [brackets]):\n");
   for (temp = comm_table[cnt]; temp.desc && temp.name;
	temp = comm_table[++cnt]) {
      sndsock(si, "* %s ", temp.name);

      for (arg = temp.args, cnt2 = 0; *arg; arg++, cnt2++)
	 sndsock(si, "[%s] ", *arg);
      
      sndsock(si, "\n  -- %s\n", temp.desc);
   }

   return RET_SUCCESS;
}

static int __search(char **a, sinfo *si, l_ent *h) {
   l_ent *t;
   af_ent *af;
   int size;

   if (!a[1]) {
      sndsock(si, "Need a string to search with!\n");
      return RET_BADARGS;
   }

   t = begin_tree_search(a[1]);
   size = list_size(t);
   sndsock(si, "Search returned %d result(s)\n", list_size(t));
   
   for (; t && t->data; t = t->next) {
      af = (af_ent *)t->data;
      sndsock(si, "%30s: %-30s\n", af->fname, (char *)af->path);
   }

   end_tree_search();
   return size ? RET_SUCCESS : RET_NORES;
}

static int __list(char **a, sinfo *si, l_ent *h) {
   l_ent *t = global_playlist;
   pl_ent *pl;
   int cnt = 0;

   sndsock(si, "%5s%8s%15s\n", "Id", "Pri", "Name");
   for (; t && t->data; t = t->next, cnt++) {
      pl = (pl_ent *)t->data;
      sndsock(si, "%5d%8d%15s\n", pl->id, t->pri, pl->af->fname);
   }

   return cnt ? RET_SUCCESS : RET_NORES;
}

static int __add(char **a, sinfo *si, l_ent *h) {
   int pri, ret = RET_SUCCESS;
   pl_ent *pl;

   if (!a[1] && (ret = RET_BADARGS))
      sndsock(si, "Need a filename!\n");
   else if (!a[2] && (ret = RET_BADARGS))
      sndsock(si, "Need a priority!\n");
   else {
      pri = atoi(a[2]);
      pl = (pl_ent *)malloc(sizeof(pl_ent));

      if (!(pl->af = tree_find(a[1]))) ret = RET_FNF;
      else {
	 pthread_mutex_lock(&global_pl_lock);
	 pl->id = global_pl_counter++;
	 list_pri_add_wd(&global_playlist, pri, pl);
	 pthread_mutex_unlock(&global_pl_lock);
      }
   }

   return ret;
}

/* first arg is an int, second is a pl_ent * */
static int __plid_comp(void *one, void *two) {
   return ((int)one) == ((pl_ent *)two)->id;
}

static int __chgpri(char **a, sinfo *si, l_ent *h) {
   int ret;
   l_ent *ple;

   if (!a[1] && (ret = RET_BADARGS))
      sndsock(si, "Need a playlist ID!\n");
   else if (!a[2] && (ret = RET_BADARGS))
      sndsock(si, "Need a new priority!\n");
   else {
      if (!(ple = list_find(global_playlist, (void *)atoi(a[1]), __plid_comp)))
	 return RET_ELENF;
      
      ple->pri = atoi(a[2]);
      list_pri_adjust(&global_playlist, ple);
   }

   return RET_SUCCESS;
}

static int __del(char **a, sinfo *si, l_ent *h) {
   if (!a[1]) return RET_BADARGS;
   list_delete(&global_playlist, (void *)atoi(a[1]), __plid_comp);

   return RET_SUCCESS;
}

static int __shuf(char **a, sinfo *si, l_ent *h) {
   /* this is probably a huge-ass memory leak */
   list_pri_shuffle(&global_playlist);
   return RET_SUCCESS;
}

static int __by_id(void *data) {return ((pl_ent *)data)->id;}
static int __by_rev_id(void *data) {return -(__by_id(data));}

static int __sort(char **a, sinfo *si, l_ent *h) {
   list_pri_sort(&global_playlist, __by_id);
   return RET_SUCCESS;
}

static int __revsort(char **a, sinfo *si, l_ent *h) {
   list_pri_sort(&global_playlist, __by_rev_id);
   return RET_SUCCESS;
}

/* Add new commands here!
   Make sure the whole array is NULL-terminated! */
static cmd_s comm_table[] = {
   {"quit", __quit, "Disconnect", 
    {NULL}
   },
   {"shutdown", __shutdown, 
    "Shutdown the server (requires 'authorize')",
    {"delay in seconds", NULL}
   },
   {"info", __info, "List connected clients", 
    {NULL}
   },
   {"auth", __auth, "Authorize your connection",
    {"password", NULL}
   },
   {"help", __help, "This help", 
    {NULL}
   },
   {"search", __search,
    "Search for audio files, case insensitive", 
    {"search string", NULL}
   },
   {"list", __list,
    "Print the current playlist", 
    {NULL}
   },
   {"add", __add, "Add a file to the playlist",
    {"filename", "priority (lower is better)",
     NULL},
   },
   {"chgpri", __chgpri, "Change a playlist entry's priority",
    {"entry ID", "new priority", NULL},
   },
   {"del", __del, "Delete a playlist entry",
    {"entry ID", NULL},
   },
   {"shuf", __shuf, "Shuffle the playlist",
    {NULL},
   },
   {"sort", __sort, "Sort list by ID (modifies priorities)",
    {NULL},
   },
   {"revsort", __revsort, "Same as sort, but by reverse ID",
    {NULL},
   },
   {NULL, NULL, NULL, {NULL}},
};

static cmd_r comm_returns[] = {
   {RET_SUCCESS, "Success"}, 
   {RET_NORES, "No results found."},
   {RET_UNKNOWN, "Command unknown."},
   {RET_ERRAUTH, "Connection not authorized."},
   {RET_FNF, "File not found."},
   {RET_BADARGS, "Bad command arguments."},
   {RET_BADAUTH, "Bad authorization attempt."},
   {RET_ELENF, "Requested element not found."},
   {-1, NULL},
};

static const char *find_return_string(int ret) {
   cmd_r temp;
   int cnt;

   for (cnt = 0, temp = comm_returns[cnt]; temp.desc &&
    temp.err_num != ret; temp = comm_returns[cnt++]) {}
   
   return temp.desc;
}

/* shanty-ass function, don't use it! */
static void chomp(char *str) {
   char *t;
   if (t = strchr(str, '\n')) *t = '\0';
   if (t = strchr(str, '\r')) *t = '\0';
}

static char **parse_command(char *str) {
   char *buf, *t = NULL, **ret;
   int cnt = 0;
   
   chomp(str);
   ret = (char **)malloc(sizeof(char *) * (strlen(str) + 1));
   if (t = strtok_r(str, " ", &buf)) ret[cnt++] = t;

   while (t = strtok_r(NULL, " ", &buf))
      ret[cnt++] = t;

   ret[cnt] = NULL;
   return ret;
}

int comm_exec(char *str, sinfo *si, l_ent *h) {
   char *command, **arr;
   int cnt = 0;
   struct command temp;
   int ret;
   char orig_comm[strlen(str) + 1];
   
   strncpy(orig_comm, str, strlen(str) + 1);
   chomp(orig_comm);
   arr = parse_command(str);
   command = arr[0];

   for (temp = comm_table[cnt]; temp.name && temp.func &&
    strcmp(temp.name, command); temp = comm_table[++cnt]) {}

   ret = temp.func ? (*temp.func)(arr, si, h) : RET_UNKNOWN;
   sndsock(si, "%s %d %s\n", (ret < 200 ? "+OK" : "-ERR"), ret,
	   find_return_string(ret));
   
   debug("%s '%s' %d %s\n", si_str(si), orig_comm,
	 ret, find_return_string(ret));
   free(arr);
   return ret;
}

