#ifndef NET_UTIL_H
#define NET_UTIL_H

struct request_info
{
   int type;
   off_t size;
   char *path;
   char **argv;
};

/* parsing */
#define HTTP_OK 200
#define BAD_REQUEST 400
#define PERMISSION_DENIED 403
#define NOT_FOUND 404
#define INTERNAL_ERROR 500
#define NOT_IMPLEMENTED 501

int path_filter(char **pathp);
int parse_request(char *request, struct request_info *req);
void print_request(int req_type, char *pathname);

/* sending data */
int send_error(int fd, int err_code);
int send_data(int fd, struct request_info *req);

#endif
