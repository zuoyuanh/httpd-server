#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include "handler.h"
#include "net_util.h"
#include "basic_util.h"
#define DEFAULT_BUF_SIZE 128

int *cnt_conn;

void handle_action(int signo)
{
   pid_t pid;
   if ((pid = wait(NULL)) > 0)
      *cnt_conn = *cnt_conn - 1;
}

void setup(int signo)
{
   struct sigaction action;
   if (sigemptyset(&action.sa_mask) == -1) {
      perror("");
      exit(-1);
   }
   action.sa_flags = SA_RESTART;
   action.sa_handler = handle_action;
   if (sigaction(signo, &action, NULL) == -1) {
      perror("");
      exit(-1);
   }
}

int find_line(char *buf)
{
   int i;
   for (i=0; i<DEFAULT_BUF_SIZE; i++) {
      if (buf[i] == '\n') {
         buf[i] = '\0';
         return 1;
      }
   }
   return 0;
}

char *read_from_socket(int fd)
{
   int bytes_read;
   int size = DEFAULT_BUF_SIZE;
   int size_left = DEFAULT_BUF_SIZE;
   char *ori_buf;
   char *buf = checked_malloc(size);
   char tmp[DEFAULT_BUF_SIZE];
   buf[0] = '\0';
   ori_buf = buf;
   while ((bytes_read = read(fd, buf, size_left)) > 0) {
      if (!find_line(buf)) {
         size_left -= bytes_read;
         buf += bytes_read;
         if (size_left <= 0) {
            size += DEFAULT_BUF_SIZE;
            ori_buf = checked_realloc(ori_buf, size);
            buf++;
            size_left = DEFAULT_BUF_SIZE;
         }
      } else break;
   }
   while (recv(fd, tmp, DEFAULT_BUF_SIZE, MSG_DONTWAIT) > 0);
   return ori_buf;
}

void run_child(int fd)
{
   int ck_code;
   char *request;
   struct request_info req;
   request = read_from_socket(fd);
   if ((ck_code = parse_request(request, &req)) != HTTP_OK) {
      send_error(fd, ck_code);
      free(request);
      exit(-1);
   }
   send_data(fd, &req);
   if (req.argv) free(req.argv);
   free(request);
   if (close(fd) < 0) {
      perror("");
      exit(-1);
   }
   exit(0);
}

void handle_request(int fd, int *conn)
{
   pid_t pid;
   setup(SIGCHLD);
   cnt_conn = conn;
   *conn = *conn + 1;
   if ((pid = fork()) < 0) {
      perror("");
      send_error(fd, INTERNAL_ERROR);
      return;
   } else if (pid == 0) {
      run_child(fd);
   }
}
