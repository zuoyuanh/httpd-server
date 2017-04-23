#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "handler.h"
#include "net_util.h"
#include "simple_net.h"
#include "limit_fork.h"
#define DEFAULT_MAX_CONN 20

int processes_lim()
{
   int max;
   struct rlimit rl;
   if (getrlimit(RLIMIT_NPROC, &rl)) {
      perror("getrlimit");
      return DEFAULT_MAX_CONN;
   }
   max = rl.rlim_cur;
   return max;
}

int main(int argc, char *argv[])
{
   int fd;
   int port;
   int conn = 0;
   int MAX_CONN;
   if (argc < 2) {
      fprintf(stderr, "httpd: missing port\n");
      fprintf(stderr, "Usage: ./httpd [port]\n");
      exit(-1);
   }
   port = atoi(argv[1]);
   if (port < 1024 || port > 65535) {
      fprintf(stderr, "httpd: invalid port number\n");
      fprintf(stderr, "Port number must be greater than (or equals to) 1024 and smaller than 65535\n");
      exit(-1);
   }
   MAX_CONN = 3 * processes_lim() / 8;
   MAX_CONN = (MAX_CONN>1) ? MAX_CONN : 1;
   fd = create_service(port, MAX_CONN);
   if (fd < 0) {
      fprintf(stderr, "httpd: failed to launch service\n");
      fprintf(stderr, "Please check whether the port provided is available\n");
      exit(-1);
   }
   limit_fork(2 * MAX_CONN);
   while (1) {
      int new_fd;
      if (conn < MAX_CONN) {
         if ((new_fd = accept_connection(fd)) < 0) {
            printf("error: accept failed newfd=%d fd=%d\n", new_fd, fd);
            continue;
         }
         handle_request(new_fd, &conn);
         if (close(new_fd) < 0) {
            perror("");
            continue;
         }
      }
   }
   return 0;
}
