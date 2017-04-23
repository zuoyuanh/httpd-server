#ifndef BASIC_UTIL_H
#define BASIC_UTIL_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

/* memory allocation */
void *checked_malloc(size_t size);
void *checked_realloc(void *p, size_t size);

/* I/O functions */
char *readline(FILE *f, int *eof);

/* string processing */
char *my_strsep(char **stringp, char *delim);
int my_strtrim(char **stringp, char *to_trim);
int my_strreplace(char *s, char *key, char *replace);
int is_substring_firstn(char *s1, char *s2, char st_filter);

#endif
