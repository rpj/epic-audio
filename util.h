/* $Id: util.h,v 1.4 2004/08/31 19:29:08 rjoseph Exp $ */
#ifndef __UTIL_H
#define __UTIL_H

#include "ea.h"

#define strlower(x) \
do { char *__t = (char *)x; for(; *__t; __t++) { \
*(__t) = tolower(*__t);}} while (0)

extern char *getcwd_malloc();
extern int is_spec_dir(mode_t, char *);
extern void *chkmalloc(size_t);

#endif
