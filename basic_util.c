#include <stdio.h>
#include <stdlib.h>
#include "basic_util.h"
#define DEFAULT_LINE_LENGTH 20

void *checked_malloc(size_t size)
{
   void *p = malloc(size);
   if (!p) {
      perror("");
      exit(-1);
   }
   return p;
}

void *checked_realloc(void *p, size_t size)
{
   void *q = realloc(p, size);
   if (!q) {
      perror("");
      exit(-1);
   }
   return q;
}

char *readline(FILE *f, int *eof)
{
   char c;
   int size = 0;
   int str_size = DEFAULT_LINE_LENGTH;
   char *line = checked_malloc(str_size * sizeof(char));
   while ((c = fgetc(f))) {
      if (c == EOF) {
         *eof = 1;
         break;
      }
      if (c == '\n') break;
      if (size+3 >= str_size) {
         str_size *= 2;
         line = checked_realloc(line, str_size * sizeof(char));
      }
      line[size++] = c;
   }
   line[size] = '\0';
   if (size == 0) {
      free(line);
      line = NULL;
   }
   return line;
}

char *my_strsep(char **stringp, char *delim)
{
   int white_space;
   char *tok;
   white_space = strcmp(delim, "WS");
   if (!stringp || !*stringp)
      return NULL;
   tok = *stringp;
   while (**stringp != '\0') {
      if (white_space == 0) {
         if (isspace(**stringp)) {
            **stringp = '\0';
            (*stringp)++;
            return tok;
         }
      } else if (**stringp == *delim) {
         **stringp = '\0';
         (*stringp)++;
         return tok;
      }
      (*stringp)++;
   }
   *stringp = NULL;
   return tok;
}

int check_char_equal(char c, char *criteria)
{
   int white_space = strcmp(criteria, "WS");
   if (white_space == 0) {
      if (isspace(c))
         return 1;
   } else {
      if (c == *criteria)
         return 1;
   }
   return 0;
}

int my_strtrim(char **stringp, char *to_trim)
{
   int cnt = 0;
   int found = 0;
   char *result;
   if (!stringp || !*stringp)
      return -1;
   while (**stringp != '\0') {
      if (!check_char_equal(**stringp, to_trim)) {
         found = 1;
         result = *stringp;
         break;
      }
      (*stringp)++;
   }
   cnt = strlen(result)-1;
   while (cnt >= 0) {
      if (!check_char_equal(result[cnt], to_trim))
         break;
      if (check_char_equal(result[cnt], to_trim) && (cnt-1<0 || result[cnt-1]!=*to_trim))
         result[cnt] = '\0';
      cnt--;
   }
   if (found)
      *stringp = result;
   return 0;
}

int my_strreplace(char *s, char *key, char *replace)
{
   char *substr;
   if (key == NULL || replace == NULL)
      return 0;
   if (strlen(key) != strlen(replace))
      return -1;
   if ((substr = strstr(s, key)) == NULL)
      return 0;
   while (*replace != '\0') {
      *substr = *replace;
      substr++;
      replace++;
   }
   return 1;
}

int is_substring_firstn(char *s1, char *s2, char st_filter)
{
   if (s1 == NULL)
      return 0;
   if (s2 == NULL)
      return 1;
   while (*s1==st_filter && *s1!='\0')
      s1++;
   while (*s1!='\0' && *s2!='\0') {
      if (*s1 != *s2)
         break;
      s1++;
      s2++;
   }
   if (*s2 != '\0')
      return 0;
   return 1;
}
