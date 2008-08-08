/* $Id: socket.c,v 1.17 2004/09/01 20:16:01 rjoseph Exp $
   the socket handling routines */
#include "socket.h"

/* static so only socket.c can see the symbols */
static l_ent *socket_list;
static pthread_mutex_t socket_list_lock;
static pthread_t main_thread;
static sinfo *main_socket;
static char main_thread_run;

/* the following three functions are callbacks for the list code */
static int socketinfo_compare(void *d1, void *d2) {
   return ((sinfo *)d1)->sd == ((sinfo *)d2)->sd;
}

/* the calling function MUST free the returned pointer
   list_debug_print will do this for you, but otherwise, watch out!
   see the si_debug() macro in ea.h for a safe way to use this */
char *socketinfo_print(void *d) {
   sinfo *si = (sinfo *)d;
   char *ret = (char *)malloc(MAX_BUF_SIZE);

   if (si) {
      snprintf(ret, MAX_BUF_SIZE, "[<sd:%d thread:%lu auth:%d> %s:%d]",
       si->sd, si->thread, si->authed, si_str(si), ntohs(si->addr.sin_port));
   } else snprintf(ret, MAX_BUF_SIZE, "[No data!]");

   return ret;
}

sinfo *create_sinfo(int sd, struct sockaddr_in *sa) {
   sinfo *info = (sinfo *)chkmalloc(sizeof(sinfo));

   memset(info, 0, sizeof(sinfo));
   memcpy(&info->addr, sa, sizeof(struct sockaddr_in));
   info->sd = sd;
   time(&info->bday);

   return info;
}

/* each call will be a thread */
static void *do_socket_conn(void *data) {
   sinfo *info = (sinfo *)data;
   char buf[MAX_BUF_SIZE], *t;
   int rrv = 0, i;

   si_debug("In do_socket_conn %s\n", info);
   sndsock(info, "Welcome to %s %s\n", NAME, VERSION);

   while (1) {
      memset(buf, 0, MAX_BUF_SIZE);
      rrv = recv(info->sd, buf, MAX_BUF_SIZE, 0);

      if (rrv < 0) {
	 si_debug("recv() error on %s, disconnecting\n", info);
	 goto EXIT;
      } else if (!rrv) {
	 si_debug("%s disconnected\n", info);
	 goto EXIT;
      } else {
	 comm_exec(buf, info, socket_list);
	 sndsock(info, "\r\n");
      }

      usleep(U_SLEEP);
   }

 EXIT:
   close_sock_thread(info);
   return NULL;
}

/* does all the cleanup necessary when a client leaves, including
   removing the client from the socket_list */
void close_sock_thread(void *d) {
   sinfo *si = (sinfo *)d;
   int ret;

   si_debug("close_sock_thread(%s)\n", si);
   if (ret = close(si->sd))
      debug("close(%d) returned %d, errno:%d\n", si->sd, ret, errno);

   if (si != main_socket) {
      pthread_mutex_lock(&socket_list_lock);
      list_delete(&socket_list, si, socketinfo_compare);
      pthread_mutex_unlock(&socket_list_lock);

      debug("pthread_cancel(%lu)\n", si->thread);
      pthread_cancel(si->thread);
   } else free(si);
}

void halt_main_thread() {main_thread_run = 0;}

/* each call will be a thread, but there should only be one thread
   of this function in the program! */
static void *listen_loop(void *data) {
   int sd = data, count = 0, cli_sd = 0, clen = 0;
   struct sockaddr_in cli_addr;

   /* all this setup has to be done before we hit an accept() call
      because we can't have clients running commands on uninitialized
      data structures! */
   socket_list = list_init();
   init_playlist();
   audio_search(".");
   pthread_mutex_init(&socket_list_lock, NULL);
   clen = sizeof(cli_addr);

   /* socket is set to non-blocking, but this doesn't work in OS X, so we may
      need to figure out a different way to shut the server down... could possibly
      just have the calling thread wait and cancel itself last, canceling all
      others first */
   while (main_thread_run) {
      cli_sd = accept(sd, (struct sockaddr *)&cli_addr, &clen);

      if (cli_sd < 0 && errno != EAGAIN)
	 debug("accept() failed, sd:%d errno:%d\n", cli_sd, errno);
      else if (cli_sd > -1) {
	 debug("Accepted connection on sd:%d, errno:%d\n", cli_sd, errno);
	 sinfo *t = create_sinfo(cli_sd, &cli_addr);

	 pthread_mutex_lock(&socket_list_lock);
	 list_add_with_data(&socket_list, (void *)t);
	 pthread_mutex_unlock(&socket_list_lock);
	 
	 pthread_create(&(letosi(socket_list))->thread, NULL,
	  do_socket_conn, (void *)letosi(socket_list));
      }

      usleep(U_SLEEP);
   }

   /* this should be the only exit vector in the entire program */
   debug("Freeing search data structures...\n");
   list_free(socket_list, close_sock_thread);
   close_sock_thread(main_socket);
   free_search_trees();
   destroy_playlist();

   debug("All done, pthread_exit()ing main thread (id:%lu)\n", main_thread);
   pthread_exit(NULL);
}   

int setup_listen_socket(int port, pthread_t *thread) {
   int sd = 0, tid = 0;
   struct sockaddr_in serv_addr;
   
   sd = socket(AF_INET, SOCK_STREAM, 0);
   if (sd < 0) {
      perror("socket() failed");
      return sd;
   }

   /* this fcntl call does NOT work on OS X, need to figure out why! */
   fcntl(sd, F_SETFL, O_NONBLOCK);
   /* ignore SIGPIPE broken pipes */
   signal(SIGPIPE, SIG_IGN);

   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   serv_addr.sin_port = htons(port);

   if ((tid = bind(sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) {
      perror("bind() failed");
      return tid;
   }

   debug("Socket bound to port %d\n", port);
   listen(sd, LISTEN_BACKLOG);

   debug("Creating listen thread, sd is %d\n", sd);
   main_thread_run = 1;
   tid = pthread_create(thread, NULL, listen_loop, (void *)sd);
  
   /* save some important shiz */
   main_socket = create_sinfo(sd, &serv_addr);
   main_socket->thread = main_thread = *thread;
   si_debug("%s is main_socket\n", main_socket);

   if (tid) {
      perror("pthread_create() failed!");
      return tid;
   }

   sleep(2);
   debug("Leaving setup_listen_socket()\n");

   return 0;
}
