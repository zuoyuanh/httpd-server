#define _BSD_SOURCE 1

#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include "net_util.h"
#include "basic_util.h"
#define DEFAULT_LIST_LEN 10
#define MAX_BUF_SIZE 4096

int send_head(int fd, int head_type, off_t length)
{
   char buf1[] = "Content-Type: text/html\r\n";
   char *buf2 = checked_malloc(21 + 10);
   if (head_type == HTTP_OK) {
      char buf[] = "HTTP/1.0 200 OK\r\n";
      send(fd, buf, sizeof(buf)-1, MSG_NOSIGNAL);
   } else if (head_type == BAD_REQUEST) {
      char buf[] = "HTTP/1.0 400 Bad Request\r\n";
      send(fd, buf, sizeof(buf)-1, MSG_NOSIGNAL);
   } else if (head_type == PERMISSION_DENIED) {
      char buf[] = "HTTP/1.0 403 Permission Denied\r\n";
      send(fd, buf, sizeof(buf)-1, MSG_NOSIGNAL);
   } else if (head_type == NOT_FOUND) {
      char buf[] = "HTTP/1.0 404 Not Found\r\n";
      send(fd, buf, sizeof(buf)-1, MSG_NOSIGNAL);
   } else if (head_type == INTERNAL_ERROR) {
      char buf[] = "HTTP/1.0 500 Internal Error\r\n";
      send(fd, buf, sizeof(buf)-1, MSG_NOSIGNAL);
   } else {
      char buf[] = "HTTP/1.0 501 Not Implemented\r\n";
      send(fd, buf, sizeof(buf)-1, MSG_NOSIGNAL);
   }
   send(fd, buf1, strlen(buf1), MSG_NOSIGNAL);
   sprintf(buf2, "Content-Length: %ld\r\n\r\n", (long int)length);
   send(fd, buf2, strlen(buf2), MSG_NOSIGNAL);
   free(buf2);
   return 0;
}

int send_error(int fd, int err_code)
{
   if (err_code == BAD_REQUEST) {
      char buf[] = "<b>Bad Request</b>";
      send(fd, buf, strlen(buf), MSG_NOSIGNAL);
   } else if ((err_code==PERMISSION_DENIED) || (err_code==EACCES)) {
      char buf[] = "<b>Permission Denied</b>";
      send_head(fd, err_code, strlen(buf));
      send(fd, buf, strlen(buf), MSG_NOSIGNAL);
   } else if ((err_code==NOT_FOUND) || (err_code==EEXIST)) {
      char buf[] = "<b>Not Found</b>";
      send_head(fd, err_code, strlen(buf));
      send(fd, buf, strlen(buf), MSG_NOSIGNAL);
   } else if (err_code == INTERNAL_ERROR) {
      char buf[] = "<b>Internal Error</b>";
      send_head(fd, err_code, strlen(buf));
      send(fd, buf, strlen(buf), MSG_NOSIGNAL);
   } else {
      char buf[] = "<b>Not Implemented</b>";
      send_head(fd, err_code, strlen(buf));
      send(fd, buf, strlen(buf), MSG_NOSIGNAL);
   }
   return 0;
}

int scheck_file(char *path)
{
   struct stat st;
   if (stat(path, &st) >= 0) {
      if (!S_ISREG(st.st_mode)) 
         return PERMISSION_DENIED;
   } else {
      switch (errno) {
         case ENOENT: return NOT_FOUND;
         case EACCES: return PERMISSION_DENIED;
         default:     return NOT_IMPLEMENTED;
      }
   }
   return HTTP_OK;
}

int check_file(char *path, struct request_info *req)
{
   struct stat st;
   if (stat(path, &st) >= 0) {
      if (!S_ISREG(st.st_mode)) 
         return PERMISSION_DENIED;
      req->size = st.st_size;
   } else {
      switch (errno) {
         case ENOENT: return NOT_FOUND;
         case EACCES: return PERMISSION_DENIED;
         default:     return NOT_IMPLEMENTED;
      }
   }
   return HTTP_OK;
}

#define TYPE_HEAD 0
#define TYPE_HEAD_CGI 1
#define TYPE_GET 2
#define TYPE_CGI_LIKE 3

int read_request(char *request, char **path)
{
   int cgi_enabled = 0;
   char *type, *filename;
   type = my_strsep(&request, "WS");
   my_strtrim(&type, "WS");
   my_strtrim(&request, "WS");
   filename = my_strsep(&request, "WS");
   my_strtrim(&filename, "WS");
   if ((request==NULL || strlen(request)==0) 
        || (filename==NULL || strlen(filename)==0)) {
      *path = NULL;
      return BAD_REQUEST;
   }
   *path = filename;
   if (is_substring_firstn(filename, "cgi-like/", '/'))
      cgi_enabled = 1;
   if ((type==NULL) || type[0]=='\0') {
      *path = NULL;
      return BAD_REQUEST;
   }
   if (strcmp(type, "HEAD") == 0) {
      if (cgi_enabled)
         return TYPE_HEAD_CGI;
      return TYPE_HEAD;
   } else if (strcmp(type, "GET") == 0) {
      if (cgi_enabled)
         return TYPE_CGI_LIKE;
      return TYPE_GET;
   } else {
      *path = NULL;
      return NOT_IMPLEMENTED;
   }
   return 0;
}

int send_html(int fd, struct request_info *req)
{
   int fd2;
   off_t size = req->size;
   char *path = req->path;
   if ((fd2 = open(path, O_RDONLY)) < 0) {
      send_error(errno, fd);
      close(fd2);
      return -1;
   } else {
      send_head(fd, HTTP_OK, size);
      if (size > MAX_BUF_SIZE) {
         ssize_t bytes_sent = 0;
         ssize_t bytes_read = 0;
         ssize_t total_bytes_read = 0;
         ssize_t total_bytes_sent = 0;
         char buf[MAX_BUF_SIZE];
         dup2(fd2, 0);
         close(fd2);
         while ((bytes_read = read(0, buf, MAX_BUF_SIZE)) > 0) {
            total_bytes_read += bytes_read;
            bytes_sent = send(fd, buf, (size_t)bytes_read, MSG_NOSIGNAL);
            if (bytes_sent <= 0) break;
            total_bytes_sent += bytes_sent;
         }
         if (total_bytes_sent != total_bytes_read)
            return -1;
      } else {
         ssize_t bytes_sent;
         ssize_t *buf = checked_malloc(size);
         dup2(fd2, 0);
         close(fd2);
         read(0, buf, (size_t)size);
         bytes_sent = send(fd, buf, (size_t)size, MSG_NOSIGNAL);
         free(buf);
         if (bytes_sent != size)
            return -1;
      }
   }
   return 0;
}

int send_head_info(int fd, struct request_info *req)
{
   int fd2;
   off_t size = req->size;
   char *path = req->path;
   if ((fd2 = open(path, O_RDONLY)) < 0) {
      send_error(errno, fd);
      close(fd2);
      return -1;
   } else {
      send_head(fd, HTTP_OK, size);
      close(fd2);
   }
   return 0;
}

void print_argv(char **argv)
{
   int i;
   printf("CGI Arguments Parse Result: { ");
   for (i=0; argv[i]; i++)
      printf("[%s] ", argv[i]);
   printf("}\n");
}

char **parse_cgi(char *request)
{
   char *tok;
   my_strtrim(&request, ".");
   my_strtrim(&request, "/");
   tok = my_strsep(&request, "/");
   my_strtrim(&tok, "/");
   if ((tok = my_strsep(&request, "?")) != NULL) {
      int size = 1;
      int lsize = DEFAULT_LIST_LEN;
      char **argv = checked_malloc(lsize * sizeof(char *));
      my_strtrim(&request, "/");
      argv[0] = tok;
      while ((tok = my_strsep(&request, "&")) != NULL) {
         if (size+1 >= lsize) {
            lsize *= 2;
            argv = checked_realloc(argv, lsize * sizeof(char *));
         }
         argv[size++] = tok;
      }
      argv[size] = NULL;
      return argv;
   }
   return NULL;
}

int send_cgi(int fd, int type, struct request_info *req)
{
   pid_t pid;
   char filename[25];
   char **argv = req->argv;
   if (argv==NULL || argv[0]==NULL || argv[0][0]=='\0') {
      send_error(PERMISSION_DENIED, fd);
      return -1;
   }
   sprintf(filename, "%d.tmp", getpid());
   if ((pid = fork()) < 0) {
      send_error(INTERNAL_ERROR, fd);
      return -1;
   } else if (pid == 0) {
      int fd2;
      if ((fd2 = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
         exit(-1);
      dup2(fd2, 1);
      dup2(fd2, 2);
      close(fd2);
      close(fd);
      if (execv(argv[0], argv) < 0) {
         remove(filename);
         exit(-1);
      }
   } else {
      int ck_result;
      struct request_info result;
      waitpid(pid, NULL, 0);
      result.path = filename;
      if ((ck_result = check_file(filename, &result)) != HTTP_OK) {
         send_error(INTERNAL_ERROR, ck_result);
         return -1;
      }
      if (type == TYPE_CGI_LIKE)
         send_html(fd, &result);
      else if (type == TYPE_HEAD_CGI)
         send_head_info(fd, &result);
      remove(filename);
   }
   return 0;
}

int path_filter(char **pathp)
{
   char *path = *pathp;
   path--;
   if (*(path+1) == '/')
      *path = '.';
   else {
      *path = '/';
      path--;
      *path = '.';
   }
   if (my_strreplace(path, "..", "//") == 1)
      return PERMISSION_DENIED;
   *pathp = path;
   return 0;
}

void print_request(int req_type, char *pathname)
{
   printf("Request type: [");
   switch (req_type) {
      case TYPE_HEAD:     printf("HEAD");
                          break;
      case TYPE_GET:      printf("GET");
                          break;
      case TYPE_CGI_LIKE: printf("GET cgi-like");
                          break;
      case TYPE_HEAD_CGI: printf("HEAD cgi-like");
                          break;
      default:            printf("Undefined");
                          break;
   }
   printf("] Path: [%s]\n", pathname);
}

int parse_request(char *request, struct request_info *req)
{
   int req_type;
   char *pathname;
   req->argv = NULL;
   if ((req_type = read_request(request, &pathname)) == BAD_REQUEST)
      return req_type;
   if (req_type == NOT_IMPLEMENTED)
      return req_type;
   if (path_filter(&pathname) == PERMISSION_DENIED)
      return PERMISSION_DENIED;
   req->type = req_type;
   req->path = pathname;
   if (req_type == TYPE_HEAD)
      return check_file(pathname, req);
   else if (req_type == TYPE_GET)
      return check_file(pathname, req);
   else if ((req_type==TYPE_CGI_LIKE) || (req_type==TYPE_HEAD_CGI)) {
      if (chdir("cgi-like") < 0)
         return INTERNAL_ERROR;
      req->argv = parse_cgi(pathname);
      if (req->argv)
         return check_file((req->argv)[0], req);
      else return PERMISSION_DENIED;
   }
   return HTTP_OK;
}

int send_data(int fd, struct request_info *req)
{
   if (req->type == TYPE_GET)
      send_html(fd, req);
   else if (req->type == TYPE_CGI_LIKE)
      send_cgi(fd, req->type, req);
   else if (req->type == TYPE_HEAD)
      send_head_info(fd, req);
   else if (req->type == TYPE_HEAD_CGI)
      send_cgi(fd, req->type, req);
   return 0;
}
