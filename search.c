/* $Id: search.c,v 1.17 2004/09/01 20:16:01 rjoseph Exp $ */
#include "search.h"
#include "util.h"
#include "socket.h"

/* list of valid music file extensions... as different formats are
   added, this may need to be an array of structs that contain a
   function ptr to an appropriate decoding function */
static char *__audio_file_extns[] = {
   ".mp3", ".MP3", ".o", NULL,
};

/* root of the tree to store af_ent structs */
static void *file_tree = NULL;
static l_ent *file_paths = NULL, *search_res = NULL;
static pthread_mutex_t __search_lock;
static char *search_string = NULL;

static int af_path_comp(void *one, void *two) {
   return one && two && !strcmp((char *)one, (char *)two);
}

/* for tree insertion, simply compares names lexigraphically */
static int af_ent_comp(const void *one, const void *two) {
   return strcmp(((af_ent *)one)->fname, ((af_ent *)two)->fname);
}

static int audio_file_handler(struct dirent *sdir) {
   char **extn_ptr = __audio_file_extns, *t;
   af_ent *af;
   l_ent *le = NULL;
   
   if (!file_paths)
      file_paths = list_init();

   for (; *extn_ptr; extn_ptr++) {
      if (strstr(sdir->d_name, *extn_ptr) &&
	  strlen(rindex(sdir->d_name, '.')) == strlen(*extn_ptr)) {
	 af = (af_ent *)chkmalloc(sizeof(af_ent));
	 memset(af, 0, sizeof(af_ent));

	 af->fname = (char *)chkmalloc(strlen(sdir->d_name)+1);
	 strncpy(af->fname, sdir->d_name, strlen(sdir->d_name)+1);

	 t = getcwd_malloc();
	 if (!(le = list_find(file_paths, t, af_path_comp)))
	    le = list_add_with_data(&file_paths, t);
	 else free(t);

	 af->path = le->data;
	 
	 if (!tsearch((void *)af, &file_tree, af_ent_comp))
	    debug("Can't insert into tree: %s\n", strerror(errno));

	 return 1;
      }
   }

   return 0;
}

static void free_node(void *node) {
   free(((af_ent *)node)->fname); free((af_ent *)node);
}

static void free_path_ent(void *le) {free(le);}

static void walk_search(const void *n, const VISIT v, const int d) {
   af_ent *af = *(af_ent **)n;
   char *tmp;

   if (!search_res || !search_string)
      return;

   switch (v) {
   case postorder:
   case leaf:
      tmp = (char *)malloc(strlen(af->fname) + 1);
      strlower(strncpy(tmp, af->fname, strlen(af->fname) + 1));
      
      if (strstr(tmp, search_string))
	 list_add_with_data(&search_res, af);

      free(tmp);
      break;
   }
}

af_ent *tree_find(char *srch) {
   af_ent **ret, tmp;
   
   tmp.fname = srch;
   pthread_mutex_lock(&__search_lock);

   ret = (af_ent **)tfind((void *)&tmp, &file_tree, af_ent_comp);
   pthread_mutex_unlock(&__search_lock);

   return ret ? *ret : NULL;
}

l_ent *begin_tree_search(char *srch) {
   pthread_mutex_lock(&__search_lock);

   search_res = list_init();
   search_string = (char *)chkmalloc(strlen(srch) + 1);

   strncpy(search_string, srch, strlen(srch) + 1);
   strlower(search_string);

   twalk(file_tree, walk_search);
   free(search_string);
   return search_res;
}

void end_tree_search() {
   list_free(search_res, NULL);
   pthread_mutex_unlock(&__search_lock);
}

void free_search_trees() {
#ifdef _GNU_SOURCE
   tdestroy(file_tree, free_node); file_tree = NULL;
#endif
   list_free(file_paths, free_path_ent); file_paths = NULL;
}

/* recursive DFS of directories: second argument is a function to call
   upon locating a regular file.  Returns a count of files recognized
   by the callback function 'f' ("recognized" being whatever that means
   to the particular callback */
int search_dir(char *name, int (*f)(struct dirent *)) {
   struct dirent *sdir;
   struct stat buf;
   DIR *mydir;
   char *orig_wd = getcwd_malloc(), *t;
   int spec_dir = 0, ret_cnt = -1;  /* return values ignored for now */

   debug("search_dir looking in %s\n", name);
   if ((mydir = opendir(name)) && !chdir(name)) {
      while ((sdir = readdir(mydir))) {
	 spec_dir = 0;
	 
	 /* if directory, then recurse and search it, otherwise call f */
	 if (!lstat(sdir->d_name, &buf)) {
	    if (S_ISDIR(buf.st_mode) &&
	     !(spec_dir = is_spec_dir(buf.st_mode, sdir->d_name)))
	       search_dir(sdir->d_name, f);
	    else if (!spec_dir)
	       (*f)(sdir);
	 } else
	    debug("lstat died: %s\n", strerror(errno));
      }
   
      closedir(mydir);
      if (chdir(orig_wd))
	 debug("chdir failed: %s\n", strerror(errno));
   }

   free(orig_wd);
   return ret_cnt;
}

int audio_search(char *n) {
   /* do this here because... well, I can't think of a better place! */
   pthread_mutex_init(&__search_lock, NULL);
   return search_dir(n, audio_file_handler);
}
